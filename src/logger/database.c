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
 \brief database stuff
 */

#include "database.h"

#include "sqlite3.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


sqlite3 *opendb(const char *dbfilename) {
	// sqlite database
	sqlite3 *db;
	// sqlite return status
	int rc;

	rc = sqlite3_open(dbfilename, &db);
	if( SQLITE_OK != rc ) {
		fprintf(stderr, "Can't open database %s: %s\n", dbfilename, sqlite3_errmsg(db));
		sqlite3_close(db);
		return NULL;
	}

	// Just a little timeout to give sqlite a chance
	//     to fix things if someone's scraping the database
	sqlite3_busy_timeout(db, 1);
	return db;
}

void closedb(sqlite3 *db) {
	sqlite3_close(db);
}


