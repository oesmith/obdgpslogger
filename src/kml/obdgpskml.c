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
 \brief OBD GPS KML main entrypoint
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "obdconfig.h"
#include "obdgpskml.h"
#include "justgps.h"
#include "singleheight.h"
#include "heightandcolor.h"

#include "sqlite3.h"

/// If the user wants progress
static int show_progress;

/// Check for the trip columns
/** \return 0 for success, -1 for invalid */
static int checktripcolumns(sqlite3 *db);

/// Check for speed colum in gps table
/** \return 0 for success, -1 for invalid */
static int checkgps_speedtime(sqlite3 *db);

int main(int argc, char **argv) {

	/// Output file
	FILE *outfile;

	/// Database to dump
	sqlite3 *db;

	/// outfile filename
	char *outfilename = NULL;

	/// Database file to open
	char *databasename = NULL;

	/// Name for kml folder
	char *kmlfoldername = NULL;

	/// Max altitiude to chart to
	int maxaltitude = DEFAULT_MAXALTITUDE;

	/// getopt's current option
	int optc;

	/// might get set during option parsing. Exit when done parsing
	int mustexit = 0;

	while ((optc = getopt_long (argc, argv, kmlshortopts, kmllongopts, NULL)) != -1) {
		switch (optc) {
			case 'h':
				kmlprinthelp(argv[0]);
				mustexit = 1;
				break;
			case 'v':
				kmlprintversion();
				mustexit = 1;
				break;
			case 'p':
				show_progress = 1;
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
			case 'n':
				if(NULL != kmlfoldername) {
					free(kmlfoldername);
				}
				kmlfoldername = strdup(optarg);
				break;
			case 'a':
				maxaltitude = atoi(optarg);
				break;
			default:
				kmlprinthelp(argv[0]);
				mustexit = 1;
				break;
		}
	}
	if(mustexit) exit(0);

	if(NULL == databasename) {
		databasename = OBD_DEFAULT_DATABASE;
	}

	if(NULL == outfilename) {
		outfilename = DEFAULT_OUTFILENAME;
	}

	if(NULL == kmlfoldername) {
		kmlfoldername = DEFAULT_KMLFOLDERNAME;
	}


	// sqlite return status
	int rc;
	rc = sqlite3_open_v2(databasename, &db, SQLITE_OPEN_READONLY, NULL);
	if( SQLITE_OK != rc ) {
		fprintf(stderr, "Can't open database %s: %s\n", databasename, sqlite3_errmsg(db));
		sqlite3_close(db);
		exit(1);
	}

	if(0 != checktripcolumns(db)) {
		fprintf(stderr, "Error with trip columns. Exiting\n");
		sqlite3_close(db);
		exit(1);
	}

	outfile = fopen(outfilename, "w");
	if(NULL == outfile) {
		perror(outfilename);
		exit(1);
	}

	fprintf(outfile, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		"<kml xmlns=\"http://www.opengis.net/kml/2.2\">\n"
		"<Folder>\n"
		"<name>%s</name>\n"
		"<description>OBD GPS Logger [http://icculus.org/obdgpslogger] was used to log a car journey and export this kml file</description>\n",
		kmlfoldername);

	writekmlgraphs(db,outfile,maxaltitude);


	fprintf(outfile,"</Folder>\n</kml>\n\n");

	fclose(outfile);
	sqlite3_close(db);

	return 0;
}

void writekmlgraphs(sqlite3 *db, FILE *f, int maxaltitude) {
	// Before entering this function, you should have written all the xml fluff
	//  that comes at the top of the kml file, and be ready to dump the other fluff afterwards
	
	sqlite3_stmt *trip_stmt;
	char select_trip_sql[] =
		"SELECT trip.tripid AS tripid,trip.start AS start,trip.end AS end "
		",COUNT(trip.tripid) AS gpscount "
		"FROM trip LEFT JOIN gps ON trip.tripid=gps.trip "
		"GROUP BY trip.tripid "
		"ORDER BY trip.tripid";
	int rc;
	const char *dbend;

	// Cheesy progress
	if(show_progress) {
		printf("5.0\n");
		fflush(stdout);
	}

	rc = sqlite3_prepare_v2(db, select_trip_sql, -1, &trip_stmt, &dbend);
	if(rc != SQLITE_OK) {
		fprintf(stderr,"SQL Error in trip select(%i): %s\n", rc, sqlite3_errmsg(db));
		return;
	}

	sqlite3_reset(trip_stmt);

	if(0 == checkgps_speedtime(db)) {
		fprintf(f,"<Folder>\n"
			"<name>Speed and Position [Just GPS]</name>\n"
			"<description>Height == speed</description>\n");
		while(SQLITE_ROW == (rc = sqlite3_step(trip_stmt))) {
			if(2 > sqlite3_column_int(trip_stmt, 3)) {
				printf("Warning: Trip %i doesn't have gps\n", sqlite3_column_int(trip_stmt, 0));
			} else {
				char graphname[64];
				snprintf(graphname, sizeof(graphname), "Trip #%i", sqlite3_column_int(trip_stmt, 0));
	
				fprintf(stderr, "Writing justgps %s\n", graphname);
	
				gpsposvel(db,f, maxaltitude, 0,
					sqlite3_column_double(trip_stmt, 1), sqlite3_column_double(trip_stmt, 2),
					sqlite3_column_int(trip_stmt, 0));
			}
		}
		if(rc != SQLITE_ROW && rc != SQLITE_DONE && rc != SQLITE_OK) {
			fprintf(stderr, "Error stepping database statement (%i):\n\t%s\n", rc,
				sqlite3_errmsg(db));
		}
		fprintf(f, "</Folder>\n");

		sqlite3_reset(trip_stmt);
	} else {
		fprintf(stderr, "Couldn't find speed column in gps table. Not rendering justgps data\n");
	}

	fprintf(f,"<Folder>\n"
		"<name>RPM and Position</name>\n"
		"<description>Height indicates engine revs</description>\n");
	// Do a simple RPM vs position one first:
	while(SQLITE_ROW == (rc = sqlite3_step(trip_stmt))) {
		if(2 > sqlite3_column_int(trip_stmt, 3)) {
			printf("Warning: Trip %i doesn't have gps\n", sqlite3_column_int(trip_stmt, 0));
		} else {
			char graphname[64];
			snprintf(graphname, sizeof(graphname), "Trip #%i", sqlite3_column_int(trip_stmt, 0));

			fprintf(stderr, "Writing RPM %s\n", graphname);

			kmlvalueheight(db,f, graphname, "", "rpm", maxaltitude, 0,
				sqlite3_column_double(trip_stmt, 1), sqlite3_column_double(trip_stmt, 2),
				sqlite3_column_int(trip_stmt, 0));
		}
	}
	if(rc != SQLITE_ROW && rc != SQLITE_DONE && rc != SQLITE_OK) {
		fprintf(stderr, "Error stepping database statement (%i):\n\t%s\n", rc,
			sqlite3_errmsg(db));
	}
	fprintf(f, "</Folder>\n");

	sqlite3_reset(trip_stmt);

	if(show_progress) {
		printf("33.0\n");
		fflush(stdout);
	}

	fprintf(f,"<Folder>\n"
		"<name>MPG, Speed and Position</name>\n"
		"<description>Height indicates speed, color indicates mpg [green == better]</description>\n");
	while(SQLITE_ROW == (rc = sqlite3_step(trip_stmt))) {
		if(2 > sqlite3_column_int(trip_stmt, 3)) {
			printf("Warning: Trip %i doesn't have gps\n", sqlite3_column_int(trip_stmt, 0));
		} else {
			char graphname[64];
			snprintf(graphname, sizeof(graphname), "Trip #%i", sqlite3_column_int(trip_stmt, 0));

			fprintf(stderr, "Writing MPG,Speed %s\n", graphname);

			kmlvalueheightcolor(db,f,graphname, "",
				"vss",maxaltitude, "(710.7*vss/maf)", 5, 1,
				sqlite3_column_double(trip_stmt, 1), sqlite3_column_double(trip_stmt, 2),
				sqlite3_column_int(trip_stmt,0));
		}
	}
	if(rc != SQLITE_ROW && rc != SQLITE_DONE && rc != SQLITE_OK) {
		fprintf(stderr, "Error stepping database statement (%i):\n\t%s\n", rc,
			sqlite3_errmsg(db));
	}
	fprintf(f, "</Folder>\n");

	if(show_progress) {
		printf("66.0\n");
		fflush(stdout);
	}

	sqlite3_reset(trip_stmt);
	fprintf(f,"<Folder>\n"
		"<name>Gear and Position</name>\n"
		"<description>Height indicates ratio between rpm and speed. While you're in gear, a line should be flat</description>\n");
	while(SQLITE_ROW == (rc = sqlite3_step(trip_stmt))) {
		if(2 > sqlite3_column_int(trip_stmt, 3)) {
			printf("Warning: Trip %i doesn't have gps\n", sqlite3_column_int(trip_stmt, 0));
		} else {
			char graphname[64];
			snprintf(graphname, sizeof(graphname), "Trip #%i", sqlite3_column_int(trip_stmt, 0));

			fprintf(stderr, "Writing Gear Ratio %s\n", graphname);

			kmlvalueheight(db,f, graphname, "", "(vss/rpm)", maxaltitude, 0,
				sqlite3_column_double(trip_stmt, 1), sqlite3_column_double(trip_stmt, 2),
				sqlite3_column_int(trip_stmt,0));
		}
	}
	if(rc != SQLITE_ROW && rc != SQLITE_DONE && rc != SQLITE_OK) {
		fprintf(stderr, "Error stepping database statement (%i):\n\t%s\n", rc,
			sqlite3_errmsg(db));
	}
	fprintf(f, "</Folder>\n");

	if(show_progress) {
		printf("100.0\n");
		fflush(stdout);
	}

	sqlite3_reset(trip_stmt);


	sqlite3_finalize(trip_stmt);
}

int checkgps_speedtime(sqlite3 *db) {
	char pragma_sql[] = "PRAGMA table_info(gps)";

	sqlite3_stmt *pragma_stmt;
	int rc;

	if(SQLITE_OK != (rc = sqlite3_prepare_v2(db, pragma_sql, -1, &pragma_stmt, NULL))) {
		fprintf(stderr, "Error preparing stmt \"%s\" (%i): %s\n", pragma_sql, rc, sqlite3_errmsg(db));
		return -1;
	}

	int found_speed = 0;

	while(SQLITE_ROW == sqlite3_step(pragma_stmt)) {
		if(0 == strcmp("speed", sqlite3_column_text(pragma_stmt, 1))) {
			found_speed = 1;
		}
	}

	sqlite3_finalize(pragma_stmt);

	if(found_speed) return 0;

	return -1;
}

static int checktripcolumns_internal(sqlite3 *db, const char *tablename) {
	char pragma_sql[512];
	snprintf(pragma_sql, sizeof(pragma_sql), "PRAGMA table_info(%s)", tablename);

	sqlite3_stmt *pragma_stmt;
	int rc;

	if(SQLITE_OK != (rc = sqlite3_prepare_v2(db, pragma_sql, -1, &pragma_stmt, NULL))) {
		fprintf(stderr, "Error preparing stmt \"%s\" (%i): %s\n", pragma_sql, rc, sqlite3_errmsg(db));
		return -1;
	}

	int found_trip = 0;

	while(SQLITE_ROW == sqlite3_step(pragma_stmt)) {
		if(0 == strcmp("trip", sqlite3_column_text(pragma_stmt, 1))) {
			found_trip = 1;
		}
	}

	sqlite3_finalize(pragma_stmt);

	if(found_trip) return 0;

	return -1;
}

static int checktripcolumns(sqlite3 *db) {
	int retvalue = 0;
	if(-1 == checktripcolumns_internal(db, "obd")) {
		fprintf(stderr, "trip column missing from obd table:\n  "
			"Please run obdlogrepair on this database\n");
		retvalue = -1;
	}
	if(-1 == checktripcolumns_internal(db, "gps")) {
		fprintf(stderr, "trip column missing from gps table:\n  "
			"Please run obdlogrepair on this database\n");
		retvalue = -1;
	}
	return retvalue;
}

void kmlprinthelp(const char *argv0) {
	printf("Usage: %s [params]\n"
		"   [-o|--out[=" DEFAULT_OUTFILENAME "]\n"
		"   [-d|--db[=" OBD_DEFAULT_DATABASE "]]\n"
		"   [-n|--name[=" DEFAULT_KMLFOLDERNAME "]]\n"
		"   [-a|--altitude[=%i]]\n"
		"   [-p|--progress]\n"
		"   [-v|--version] [-h|--help]\n", argv0, DEFAULT_MAXALTITUDE);
}

void kmlprintversion() {
	printf("Version: %i.%i\n", OBDGPSLOGGER_MAJOR_VERSION, OBDGPSLOGGER_MINOR_VERSION);
}


