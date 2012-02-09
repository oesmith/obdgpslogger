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
 \brief trip database stuff
 */

#include "tripdb.h"

#include <stdio.h>

#include "sqlite3.h"

int createtriptable(sqlite3 *db) {
	char create_sql[] = "CREATE TABLE IF NOT EXISTS trip (tripid INTEGER PRIMARY KEY, start REAL, end REAL DEFAULT -1)";

	/// sqlite3 return status
	int rc;
	/// sqlite3 error message
	char *errmsg;

	if(SQLITE_OK != (rc = sqlite3_exec(db, create_sql, NULL, NULL, &errmsg))) {
		fprintf(stderr, "sqlite error on statement %s: %s\n", create_sql, errmsg);
		sqlite3_free(errmsg);
		return 1;
	}
	return 0;
}


sqlite3_int64 starttrip(sqlite3 *db, double starttime) {
	char insert_sql[] = "INSERT INTO trip (start) VALUES (?)";
	sqlite3_stmt *trip_stmt;

	int rc;
	const char *zTail;

	rc = sqlite3_prepare_v2(db,insert_sql,-1,&trip_stmt,&zTail);
	if(SQLITE_OK != rc) {
		fprintf(stderr, "Can't prepare statement %s: %s\n", insert_sql, sqlite3_errmsg(db));
		return -1;
	}

	sqlite3_bind_double(trip_stmt, 1, starttime);

	rc = sqlite3_step(trip_stmt);
	if(SQLITE_DONE != rc) {
		fprintf(stderr, "sqlite3 trip insert failed(%i): %s\n", rc, sqlite3_errmsg(db));
		return -1;
	}

	sqlite3_finalize(trip_stmt);
	return sqlite3_last_insert_rowid(db);

}

void updatetrip(sqlite3 *db, sqlite3_int64 obdtripid, double endtime) {
	char update_sql[] = "UPDATE trip SET end=? WHERE tripid=?";
	sqlite3_stmt *trip_stmt;

	if(-1 == obdtripid) {
		// error returned from starttrip
		return;
	}

	int rc;
	const char *zTail;

	rc = sqlite3_prepare_v2(db,update_sql,-1,&trip_stmt,&zTail);
	if(SQLITE_OK != rc) {
		fprintf(stderr, "Can't prepare statement %s: %s\n", update_sql, sqlite3_errmsg(db));
		sqlite3_finalize(trip_stmt);
		return;
	}

	sqlite3_bind_double(trip_stmt, 1, endtime);
	sqlite3_bind_int64(trip_stmt, 2, obdtripid);

	rc = sqlite3_step(trip_stmt);
	if(SQLITE_DONE != rc) {
		fprintf(stderr, "sqlite3 trip update failed(%i): %s\n", rc, sqlite3_errmsg(db));
	}

	sqlite3_finalize(trip_stmt);

}

