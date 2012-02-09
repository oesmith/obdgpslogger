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
#include <string.h>
#include <stdlib.h>

#include "obdrepair.h"

#include "sqlite3.h"

void printhelp(const char *argv0);

int main(int argc, const char **argv) {
	if(argc < 2 || 0 == strcmp("--help", argv[1]) ||
				0 == strcmp("-h", argv[1])) {
		printhelp(argv[0]);
		exit(0);
	}

	sqlite3 *db;
	int rc;

	if(SQLITE_OK != (rc = sqlite3_open_v2(argv[1], &db, SQLITE_OPEN_READWRITE, NULL))) {
		fprintf(stderr, "Can't open database %s: %s\n", argv[1], sqlite3_errmsg(db));
		sqlite3_close(db);
		exit(1);
	}

	printf("About to check trip ends\n");
	checktripends(db);
	printf("Done checking trip ends\n");

	printf("About to check trip ids on obd table\n");
	checktripids(db, "obd");
	printf("About to check trip ids on gps table\n");
	checktripids(db, "gps");
	printf("Done checking tripids\n");

	printf("About to check indices\n");
	checkindices(db);
	printf("Done checking indices\n");

	printf("About to check ecu column on obd table\n");
	checkobdecu(db);
	printf("Done checking ecu column on obd table\n");

	printf("About to run analyze\n");
	analyze(db);
	printf("Done running analyze\n");

	printf("About to integrity check\n");
	checkintegrity(db);
	printf("Done running integrity check\n");

	sqlite3_close(db);

	printf("Done\n");
	return 0;
}

void printhelp(const char *argv0) {
	printf("Usage: %s <database>\n"
			"Take a few best guesses at repairing an obdgpslogger log\n", argv0);
}

