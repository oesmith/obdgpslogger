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
 \brief ECU database stuff
 */

#include "ecudb.h"

#include <stdio.h>
#include <string.h>

#include "sqlite3.h"

int createecutable(sqlite3 *db) {
	char create_sql[] = "CREATE TABLE IF NOT EXISTS ecu (ecuid INTEGER PRIMARY KEY, vin TEXT, ecu INTEGER, ecudesc TEXT)";

	/// sqlite3 return status
	int rc;
	/// sqlite3 error message
	char *errmsg;

	if(SQLITE_OK != (rc = sqlite3_exec(db, create_sql, NULL, NULL, &errmsg))) {
		fprintf(stderr, "sqlite error on statement %s: %s\n", create_sql, errmsg);
		sqlite3_free(errmsg);
		return 1;
	}

	// Create the table index
	char create_idx_sql[] = "CREATE UNIQUE INDEX IF NOT EXISTS IDX_VINECU ON ecu (vin,ecu)";

	if(SQLITE_OK != (rc = sqlite3_exec(db, create_idx_sql, NULL, NULL, &errmsg))) {
		fprintf(stderr, "Not Fatal: sqlite error creating index %s: %s\n", create_idx_sql, errmsg);
		sqlite3_free(errmsg);
	}

	return 0;
}

sqlite3_int64 getecuid(sqlite3 *db, const char *vin, long ecu) {
	char select_sql[] = "SELECT ecuid FROM ecu WHERE vin=? AND ecu=?";
	int rc;
	sqlite3_stmt *stmt;

	rc = sqlite3_prepare_v2(db, select_sql, -1, &stmt, NULL);

	if(SQLITE_OK != rc) {
		fprintf(stderr, "Can't prepare statement %s: %s\n", select_sql, sqlite3_errmsg(db));
		return -1;
	}

	if(NULL != vin) {
		sqlite3_bind_text(stmt, 1, vin, strlen(vin), NULL);
	} else {
		sqlite3_bind_text(stmt, 1, "", 0, NULL);
	}

	sqlite3_bind_int64(stmt, 2, ecu);

	sqlite3_int64 retvalue = -1;

	while(SQLITE_ROW == (rc = sqlite3_step(stmt))) {
		retvalue = sqlite3_column_int64(stmt, 0);
	}

	if(SQLITE_DONE != rc && SQLITE_OK != rc) {
		fprintf(stderr, "Error stepping select statement(%i): %s\n", rc, sqlite3_errmsg(db));
	}

	sqlite3_finalize(stmt);

	return -1;
}

sqlite3_int64 createecu(sqlite3 *db, const char *vin, long ecu, const char *ecudesc) {
	sqlite3_int64 foundecu = getecuid(db, vin, ecu);
	if(0 < foundecu) {
		return foundecu;
	}

	char insert_sql[] = "INSERT INTO ecu (vin,ecu,ecudesc) VALUES (?,?,?)";
	int rc;
	sqlite3_stmt *stmt;

	rc = sqlite3_prepare_v2(db, insert_sql, -1, &stmt, NULL);

	if(SQLITE_OK != rc) {
		fprintf(stderr, "Can't prepare statement %s: %s\n", insert_sql, sqlite3_errmsg(db));
		return -1;
	}

	if(NULL != vin) {
		sqlite3_bind_text(stmt, 1, vin, strlen(vin), NULL);
	} else {
		sqlite3_bind_text(stmt, 1, "", 0, NULL);
	}

	sqlite3_bind_int64(stmt, 2, ecu);

	if(NULL != ecudesc) {
		sqlite3_bind_text(stmt, 3, ecudesc, strlen(ecudesc), NULL);
	} else {
		sqlite3_bind_text(stmt, 3, "", 0, NULL);
	}

	rc = sqlite3_step(stmt);
	if(SQLITE_OK != rc && SQLITE_DONE != rc) {
		fprintf(stderr, "Error stepping ecu insert(%i): %s\n", rc, sqlite3_errmsg(db));
	}

	sqlite3_finalize(stmt);

	return sqlite3_last_insert_rowid(db);
}

int updateecudesc(sqlite3 *db, sqlite3_int64 ecuid, const char *ecudesc) {
	if(NULL == ecudesc) return 0;

	char update_sql[] = "UPDATE ecu SET ecudesc=? WHERE ecuid=?";
	int rc;
	sqlite3_stmt *stmt;

	rc = sqlite3_prepare_v2(db, update_sql, -1, &stmt, NULL);

	if(SQLITE_OK != rc) {
		fprintf(stderr, "Can't prepare statement %s: %s\n", update_sql, sqlite3_errmsg(db));
		return -1;
	}

	sqlite3_bind_text(stmt, 1, ecudesc, strlen(ecudesc), NULL);

	sqlite3_bind_int64(stmt, 2, ecuid);

	int retvalue = 0;

	rc = sqlite3_step(stmt);
	if(SQLITE_OK != rc && SQLITE_DONE != rc) {
		fprintf(stderr, "Error stepping ecu update(%i): %s\n", rc, sqlite3_errmsg(db));
		retvalue = -1;
	}

	sqlite3_finalize(stmt);

	return retvalue;
}


