/* Copyright 2009 Gary Briggs

This file is part of obdgpslogger.

obdgpslogger is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

obdgpslogger is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with obdgpslogger.  If not, see <http://www.gnu.org/licenses/>.
*/


/** \file
 \brief obd2gpx main entrypoint
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <libgen.h>

#include "obd2gpx.h"
#include "obdconfig.h"

#include "sqlite3.h"

int main(int argc, char **argv) {
	/// Output file
	FILE *outfile;

	/// Database to dump
	sqlite3 *db;

	/// outfile filename
	char *outfilename = strdup(DEFAULT_OUTFILENAME);

	/// Database file to open
	char *databasename = strdup(OBD_DEFAULT_DATABASE);

	/// getopt's current option
	int optc;

	/// might get set during option parsing. Exit when done parsing
	int mustexit = 0;

	while ((optc = getopt_long (argc, argv, gpxshortopts, gpxlongopts, NULL)) != -1) {
		switch (optc) {
			case 'h':
				gpxprinthelp(argv[0]);
				mustexit = 1;
				break;
			case 'v':
				gpxprintversion();
				mustexit = 1;
				break;
			case 'd':
				if(NULL != databasename) {
					free(databasename);
				}
				databasename = strdup(optarg);
				break;
			case 'o':
				if(NULL != outfilename) {
					free(outfilename);
				}
				outfilename = strdup(optarg);
				break;
			default:
				gpxprinthelp(argv[0]);
				mustexit = 1;
				break;
		}
	}
	if(mustexit) exit(0);

	int rc;
	rc = sqlite3_open_v2(databasename, &db, SQLITE_OPEN_READONLY, NULL);
	if( SQLITE_OK != rc ) {
		fprintf(stderr, "Can't open database %s: %s\n", databasename, sqlite3_errmsg(db));
		sqlite3_close(db);
		exit(1);
	}

	char select_sql[4096] = "SELECT lat,lon,alt,time,trip FROM gps WHERE trip IS NOT NULL ORDER BY trip,time";

	sqlite3_stmt *select_stmt; // Our actual select statement
	
	rc = sqlite3_prepare_v2(db, select_sql, -1, &select_stmt, NULL);

	if(SQLITE_OK != rc) {
		printf("Error attempting to prepare select(%i): %s\n", rc, sqlite3_errmsg(db));
		sqlite3_close(db);
		exit(1);
	}


	if(NULL == (outfile = fopen(outfilename, "w"))) {
		perror(outfilename);
		sqlite3_close(db);
		exit(1);
	}

	gpx_writeheader(outfile, basename(outfilename));

	int currtrip = -1;

	while(SQLITE_ROW == sqlite3_step(select_stmt)) {
		double lat = sqlite3_column_double(select_stmt, 0);
		double lon = sqlite3_column_double(select_stmt, 1);
		double alt = sqlite3_column_double(select_stmt, 2);
		time_t datatime = sqlite3_column_int64(select_stmt, 3);
		int trip = sqlite3_column_int64(select_stmt, 4);

		if(currtrip != trip) {
			if(currtrip > -1) gpx_endtrip(outfile);
			printf("Writing trip %i\n", trip);
			gpx_starttrip(outfile, trip);
		}

		fprintf(outfile, "\t\t\t<trkpt lat=\"%f\" lon=\"%f\">\n", lat, lon);
		if(alt > -900) { // -1000 is a crappy way to tell if we had a 3d fix
			fprintf(outfile, "\t\t\t\t<ele>%f</ele>\n", alt);
			fprintf(outfile, "\t\t\t\t<fix>3d</fix>\n");
		} else {
			fprintf(outfile, "\t\t\t\t<fix>2d</fix>\n");
		}

		// xsd:dateTime format: CCYY-MM-DDThh:mm:ss 
		char timestring[128] = "\0";
		struct tm *tostr = localtime(&datatime);
		if(0 < strftime(timestring, sizeof(timestring), "%FT%T", tostr)) {
			fprintf(outfile, "\t\t\t\t<time>%s</time>\n", timestring);
		}

		fprintf(outfile, "\t\t\t</trkpt>\n");
		currtrip = trip;
	}
	gpx_endtrip(outfile);

	gpx_writetail(outfile);
	sqlite3_finalize(select_stmt);

	fclose(outfile);

	sqlite3_close(db);

	free(outfilename);
	free(databasename);

	return 0;
}

void gpx_writeheader(FILE *outfile, const char *filename) {
	fprintf(outfile, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
			"<gpx version=\"1.1\" creator=\"obdgpslogger\"\n"
			"\t\txmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
			"\t\txmlns=\"http://www.topografix.com/GPX/1.1\"\n"
			"\t\txsi:schemaLocation=\"http://www.topografix.com/GPS/1/1\n"
			"\t\thttp://www.topografix.com/GPX/1/1/gpx.xsd\">\n"
			"\t<metadata>\n"
			"\t\t<name>obd2gpx %s</name>\n"
			"\t\t<desc>OBDGPSLogger obd2gpx convert from %s</desc>\n"
			"\t\t<keywords>obdgpslogger,obd2gpx</keywords>\n"
			"\t\t<author>\n"
			"\t\t\t<name>Gary Briggs</name>\n"
			"\t\t\t<email id=\"chunky\" domain=\"icculus.org\" />\n"
			"\t\t\t<link href=\"http://icculus.org/obdgpslogger\"><text>OBDGPSLogger</text></link>\n"
			"\t\t</author>\n"
			"\t</metadata>\n", filename, filename);
}

void gpx_writetail(FILE *outfile) {
	fprintf(outfile, "</gpx>\n");
}

void gpx_starttrip(FILE *outfile, int tripnum) {
	fprintf(outfile, "\t<trk>\n"
		"\t\t<name>Trip %i</name>\n"
		"\t\t<trkseg>\n", tripnum);
}

void gpx_endtrip(FILE *outfile) {
	fprintf(outfile, "\t\t</trkseg>\n"
		"\t</trk>\n");
}

void gpxprinthelp(const char *argv0) {
	printf("Usage: %s [params]\n"
		"   [-o|--out=<" DEFAULT_OUTFILENAME ">]\n"
		"   [-d|--db=<" OBD_DEFAULT_DATABASE ">]\n"
		"   [-v|--version] [-h|--help]\n", argv0);
}

void gpxprintversion() {
	printf("Version: %i.%i\n", OBDGPSLOGGER_MAJOR_VERSION, OBDGPSLOGGER_MINOR_VERSION);
}

