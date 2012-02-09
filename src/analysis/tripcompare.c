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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "sqlite3.h"

#include "examinetrips.h"
#include "analysistables.h"

/// Print help
void printhelp(const char *argv0);

int main(int argc, char *argv[]) {
	// Do not attempt to buffer stdout
	setvbuf(stdout, (char *)NULL, _IONBF, 0);

	if(argc < 2 || 0 == strcmp("--help", argv[1]) ||
			0 == strcmp("-h", argv[1])) {
		printhelp(argv[0]);
		exit(0);
	}

	sqlite3 *db;

	int rc;
	rc = sqlite3_open_v2(argv[1], &db, SQLITE_OPEN_READWRITE, NULL);
	if(SQLITE_OK != rc) {
		fprintf(stderr, "Can't open database %s: %s\n", argv[1], sqlite3_errmsg(db));
		sqlite3_close(db);
		exit(1);
	}

	if(0 != createAnalysisTables(db)) {
		fprintf(stderr, "Couldn't create analysis tables, exiting\n");
		sqlite3_close(db);
		exit(1);
	}

	if(0 != fillAnalysisTables(db)) {
		fprintf(stderr, "Couldn't populate analysis tables, exiting\n");
		sqlite3_close(db);
		exit(1);
	}

	if(0 != meanMedianDistances(db)) {
		fprintf(stderr, "Couldn't populate cluster tables, exiting\n");
		sqlite3_close(db);
		exit(1);
	}

	exportGpsCSV(db, stdout);

	exit(0);

	// Find out weighted mean position of each trip
	sqlite3_stmt *gpstripstmt;
	const char gpstrip_sql[] = "SELECT DISTINCT trip FROM gps ORDER BY TRIP";

	rc = sqlite3_prepare_v2(db, gpstrip_sql, -1, &gpstripstmt, NULL);
	if(SQLITE_OK != rc) {
		fprintf(stderr, "Cannot prepare select statement gpstrips (%i): %s\n", rc, sqlite3_errmsg(db));
		sqlite3_close(db);
		exit(1);
	}


	// Find out how much petrol each trip burned
	sqlite3_stmt *obdtripstmt;
	const char obdtrip_sql[] = "SELECT DISTINCT trip FROM obd ORDER BY TRIP";

	rc = sqlite3_prepare_v2(db, obdtrip_sql, -1, &obdtripstmt, NULL);
	if(SQLITE_OK != rc) {
		fprintf(stderr, "Cannot prepare select statement obdtrips (%i): %s\n", rc, sqlite3_errmsg(db));
		sqlite3_close(db);
		exit(1);
	}

	while(SQLITE_ROW == sqlite3_step(obdtripstmt)) {
		petrolusage(db, sqlite3_column_int(obdtripstmt, 0));
	}

	sqlite3_finalize(obdtripstmt);
	sqlite3_finalize(gpstripstmt);

	sqlite3_close(db);
	return 0;
}

void printhelp(const char *argv0) {
	printf("Usage: %s <database>\n"
		"Guess which trips are the same for comparing\n", argv0);
}

