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
 \brief OBD GPS CSV main entrypoint
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef HAVE_ZLIB
#include "zlib.h"
#endif //HAVE_ZLIB

#include "obdconfig.h"
#include "obdgpscsv.h"

#include "sqlite3.h"

int main(int argc, char **argv) {

	/// Output file
	FILE *outfile;

	/// Database to dump
	sqlite3 *db;

	/// outfile filename
	char *outfilename = NULL;

	/// Database file to open
	char *databasename = NULL;

	/// Progress output
	int show_progress = 0;

	/// getopt's current option
	int optc;

	/// might get set during option parsing. Exit when done parsing
	int mustexit = 0;

	/// Start and end times. If these are >0, we append them to the sql as a WHERE clause
	double starttime = -1;
	double endtime = -1;

#ifdef HAVE_ZLIB
	/// Set if we should actually compress
	int compress_output = 0;

	/// gzip handle
	gzFile *gz_outfile;
#endif //HAVE_ZLIB

	while ((optc = getopt_long (argc, argv, csvshortopts, csvlongopts, NULL)) != -1) {
		switch (optc) {
			case 'h':
				csvprinthelp(argv[0]);
				mustexit = 1;
				break;
			case 'v':
				csvprintversion();
				mustexit = 1;
				break;
			case 'p':
				show_progress = 1;
				break;
			case 'e':
				endtime = atof(optarg);
				break;
			case 's':
				starttime = atof(optarg);
				break;
#ifdef HAVE_ZLIB
			case 'z':
				compress_output = 1;
				break;
#endif //HAVE_ZLIB
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
				csvprinthelp(argv[0]);
				mustexit = 1;
				break;
		}
	}
	if(mustexit) exit(0);

	if(NULL == databasename) {
		databasename = strdup(OBD_DEFAULT_DATABASE);
	}

	if(NULL == outfilename) {
#ifdef HAVE_ZLIB
		// If they don't specify a filename, we'll automatically suffix .gz if appropriate
		if(compress_output) {
			char tmpfn[1024];
			snprintf(tmpfn, sizeof(tmpfn), "%s.gz", DEFAULT_OUTFILENAME);
			outfilename = strdup(tmpfn);
		} else {
			outfilename = strdup(DEFAULT_OUTFILENAME);
		}
#else
		outfilename = strdup(DEFAULT_OUTFILENAME);
#endif //HAVE_ZLIB
	}


	// sqlite return status
	int rc;
	rc = sqlite3_open_v2(databasename, &db, SQLITE_OPEN_READONLY, NULL);
	if( SQLITE_OK != rc ) {
		fprintf(stderr, "Can't open database %s: %s\n", databasename, sqlite3_errmsg(db));
		sqlite3_close(db);
		exit(1);
	}

/*
We're going to put in some extra effort when exporting to CSV, to check
that the columns we need exist, and do some extra stuff with them if
they're there.

if the columns "maf" and "vss" exist, then output an extra
column, "mpg", that is the miles per gallon

*/

// First, get all a list of the columns in the obd table

	sqlite3_stmt *pragma_stmt; // The stmt for gathering table_info
	const char *dbend; // ignored handle for sqlite

	rc = sqlite3_prepare_v2(db, "PRAGMA table_info(obd)", -1, &pragma_stmt, &dbend);

	if(SQLITE_OK != rc) {
		printf("Couldn't get table info in database %s: %s\n", databasename, sqlite3_errmsg(db));
		sqlite3_close(db);
		exit(1);
	}

	int have_vss = 0; // have a column named "vss" [vehicle speed]
	int have_maf = 0; // have a column named "maf" [mass air flow]

	const char *columnnames[0x6C]; // Given we only have 0x4C definitions, I'd hope this is enough...
	int col_count = 0;

	while(SQLITE_ROW == sqlite3_step(pragma_stmt)) {
		const char *columnname = sqlite3_column_text(pragma_stmt, 1);
		char obdcolumn[20];
		if(NULL == columnname) continue;

		snprintf(obdcolumn, sizeof(obdcolumn), "obd.%s", columnname);

		columnnames[col_count++] = strdup(obdcolumn);

		if(0 == strcmp(columnname,"vss")) have_vss = 1;
		if(0 == strcmp(columnname,"maf")) have_maf = 1;
	}
	if(have_vss && have_maf) {
		columnnames[col_count++] = strdup("(7.107*obd.vss/obd.maf) as mpg");
	}
	columnnames[col_count++] = strdup("gps.lon");
	columnnames[col_count++] = strdup("gps.lat");
	columnnames[col_count++] = strdup("gps.alt");
	columnnames[col_count++] = strdup("trip.tripid");

	sqlite3_finalize(pragma_stmt);

// If progress is requested, then calculate [roughly] number of rows we expect
	long num_expected_rows = 0;
	if(show_progress) {
		char progress_sql[] = "SELECT count(*) AS c FROM obd";
		sqlite3_stmt *progress_stmt;

		rc = sqlite3_prepare_v2(db, progress_sql, -1, &progress_stmt, &dbend);

		if(SQLITE_OK != rc) {
			fprintf(stderr,"Couldn't get progress info in database %s: %s\n", databasename, sqlite3_errmsg(db));
		} else {
			while(SQLITE_ROW == sqlite3_step(progress_stmt)) {
				num_expected_rows = (long)sqlite3_column_double(progress_stmt,0);
			}
		}

		sqlite3_finalize(progress_stmt);
	}


// Second, build the SQL SELECT statement to pull the columns we just found

	char select_sql[4096] = "SELECT ";
	// Would rather do a full outer join, but sqlite doesn't support that yet
	char end_select_sql[] = " FROM obd LEFT JOIN gps ON obd.time=gps.time LEFT JOIN trip ON obd.time>trip.start AND obd.time<trip.end ";

	int i;
	for(i=0;i<col_count-1;i++) {
		strncat(select_sql, columnnames[i], sizeof(select_sql)-strlen(columnnames[i])-strlen(select_sql)-1);
		strncat(select_sql, ", ", sizeof(select_sql)-strlen(", ")-strlen(select_sql)-1);
		// Yay C ...
	}
	strncat(select_sql, columnnames[i], sizeof(select_sql)-strlen(columnnames[i])-strlen(select_sql)-1);
	strncat(select_sql, end_select_sql, sizeof(select_sql)-strlen(end_select_sql)-strlen(select_sql)-1);

	char where_clause[1024] = "\0";
	if(starttime>0 && endtime>0) {
		snprintf(where_clause, sizeof(where_clause), " WHERE obd.time>%f AND obd.time<%f ", starttime, endtime);
	} else if(endtime>0) { // Didn't set a starttime
		snprintf(where_clause, sizeof(where_clause), " WHERE obd.time<%f ", endtime);
	} else if(starttime>0) { // Didn't set an endtime
		snprintf(where_clause, sizeof(where_clause), " WHERE obd.time>%f ", starttime);
	}
	strncat(select_sql, where_clause, sizeof(select_sql)-strlen(where_clause)-strlen(select_sql)-1);

	// printf("Select: \n %s\n", select_sql);

	sqlite3_stmt *select_stmt; // Our actual select statement
	
	rc = sqlite3_prepare_v2(db, select_sql, -1, &select_stmt, &dbend);

	if(SQLITE_OK != rc) {
		printf("Error attempting to select:\n%s\n%s\n", select_sql, sqlite3_errmsg(db));
		sqlite3_close(db);
		exit(1);
	}


#ifdef HAVE_ZLIB
	if(compress_output) {
		if(NULL == (gz_outfile = gzopen(outfilename, "wb"))) {
			fprintf(stderr,"Error opening file with gzopen. Exiting\n");
			sqlite3_close(db);
			exit(1);
		}
	} else {
#endif // HAVE_ZLIB
		if(NULL == (outfile = fopen(outfilename, "w"))) {
			perror(outfilename);
			sqlite3_close(db);
			exit(1);
		}
#ifdef HAVE_ZLIB
	}
#endif // HAVE_ZLIB


	/* Getting to here means our SQL select statement is prepared,
	the output file is open for writing, and columnnames[] is packed with
	col_count columns */

	// Line of output
	char out_line[4096] = "\0";
	char out_item[64] = "\0";
	for(i=0;i<col_count;i++) {
		snprintf(out_item, sizeof(out_item), "%s,", columnnames[i]);
		strncat(out_line, out_item, sizeof(out_line)-strlen(out_item)-strlen(out_line)-1);
	}
	strncat(out_line, "\n", sizeof(out_line)-strlen("\n")-strlen(out_line)-1);

#ifdef HAVE_ZLIB
	if(compress_output) {
		gzwrite(gz_outfile, out_line, strlen(out_line));
	} else {
#endif //HAVE_ZLIB
		fprintf(outfile,"%s", out_line);
#ifdef HAVE_ZLIB
	}
#endif //HAVE_ZLIB

// Thirdly, iterate through the whole database dumping to CSV
	long current_row = 0;
	while(SQLITE_ROW == sqlite3_step(select_stmt)) {
		out_line[0] = '\0';
		for(i=0;i<col_count;i++) {
			snprintf(out_item, sizeof(out_item), "%f,", sqlite3_column_double(select_stmt, i));
			strncat(out_line, out_item, sizeof(out_line)-strlen(out_item)-strlen(out_line)-1);
		}
		strncat(out_line, "\n", sizeof(out_line)-strlen("\n")-strlen(out_line)-1);

#ifdef HAVE_ZLIB
		if(compress_output) {
			gzwrite(gz_outfile, out_line, strlen(out_line));
		} else {
#endif //HAVE_ZLIB
			fprintf(outfile,"%s", out_line);
#ifdef HAVE_ZLIB
		}
#endif //HAVE_ZLIB

		if(show_progress) {
			current_row++;
			if(0 == current_row%50) {
				printf("%f\n", 100.0f * current_row/num_expected_rows);
				fflush(stdout);
			}
		}
	}
	sqlite3_finalize(select_stmt);

#ifdef HAVE_ZLIB
	if(compress_output) {
		gzclose(gz_outfile);
	} else {
#endif //HAVE_ZLIB
		fclose(outfile);
#ifdef HAVE_ZLIB
	}
#endif //HAVE_ZLIB

	sqlite3_close(db);

	free(outfilename);
	free(databasename);

	return 0;
}

void csvprinthelp(const char *argv0) {
	printf("Usage: %s [params]\n"
		"   [-o|--out<=" DEFAULT_OUTFILENAME ">]\n"
		"   [-p|--progress]\n"
		"   [-d|--db<=" OBD_DEFAULT_DATABASE ">]\n"
		"   [-s|--start=<time>]\n"
		"   [-e|--end=<time>]\n"
#ifdef HAVE_ZLIB
		"   [-z|--gzip]\n"
#endif //HAVE_ZLIB
		"   [-v|--version] [-h|--help]\n", argv0);
}

void csvprintversion() {
	printf("Version: %i.%i\n", OBDGPSLOGGER_MAJOR_VERSION, OBDGPSLOGGER_MINOR_VERSION);
}

