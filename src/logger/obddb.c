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
 \brief obd database stuff
 */

#include "obddb.h"
#include "obdservicecommands.h"
#include "supportedcommands.h"

#include "sqlite3.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>



/// Create the obd table in the database
int createobdtable(sqlite3 *db, void *obdcaps) {
		// TODO calculate buffer size and create correct sized one,
		//   otherwise this could overflow if obdservicecommands contains a *lot* of non-NULL fields
	int i;

	/// sqlite3 return status
	int rc;
	/// sqlite3 error message
	char *errmsg;

	sqlite3_stmt *pragma_stmt;
	rc = sqlite3_prepare_v2(db, "PRAGMA table_info(obd)", -1, &pragma_stmt, NULL);

	int obdtable_rows = 0;

	if(SQLITE_OK == rc) {
		while(SQLITE_ROW==sqlite3_step(pragma_stmt)) {
			obdtable_rows++;
		}
	} else {
		printf("Couldn't prepare pragma stmt (%i): %s\n", rc, sqlite3_errmsg(db));
	}

	if(0 == obdtable_rows) { // ie, if the table didn't exist

		char create_stmt[4096] = "CREATE TABLE obd (";
		for(i=0; i<sizeof(obdcmds_mode1)/sizeof(obdcmds_mode1[0]); i++) {
			if(NULL != obdcmds_mode1[i].db_column && isobdcapabilitysupported(obdcaps,i)) {
				strcat(create_stmt,obdcmds_mode1[i].db_column);
				strcat(create_stmt," REAL,");
			}
		}
		strcat(create_stmt,"time REAL, trip INTEGER, ecu INTEGER DEFAULT 0)");

		// printf("Create_stmt:\n  %s\n", create_stmt);

		if(SQLITE_OK != (rc = sqlite3_exec(db, create_stmt, NULL, NULL, &errmsg))) {
			fprintf(stderr, "sqlite error on statement %s (%i): %s\n", create_stmt, rc, errmsg);
			sqlite3_free(errmsg);
			return 1;
		}

	} else { // If the table already existed
		for(i=0; i<sizeof(obdcmds_mode1)/sizeof(obdcmds_mode1[0]); i++) {
			if(NULL != obdcmds_mode1[i].db_column && isobdcapabilitysupported(obdcaps,i)) {
				sqlite3_reset(pragma_stmt);
				int found_row = 0;

				while(SQLITE_ROW == sqlite3_step(pragma_stmt)) {
					if(0 == strcmp(obdcmds_mode1[i].db_column ,sqlite3_column_text(pragma_stmt, 1))) {
						found_row = 1;
					}
				}

				if(found_row) {
					// printf("Found row %s already in database\n", obdcmds_mode1[i].db_column);
				} else {
					char sql[512];
					snprintf(sql, sizeof(sql), "ALTER TABLE obd ADD %s REAL", obdcmds_mode1[i].db_column);
					if(SQLITE_OK != (rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg))) {
						fprintf(stderr, "Unable to add column %s to database (%i): %s\n", obdcmds_mode1[i].db_column, rc, errmsg);
						sqlite3_free(errmsg);
					} else {
						printf("Added column %s to database\n", obdcmds_mode1[i].db_column);
					}
				}
			}
		}
	}

	sqlite3_finalize(pragma_stmt);

	// Create the table index
	char create_idx_sql[] = "CREATE INDEX IF NOT EXISTS IDX_OBDTIME ON obd (time)";

	if(SQLITE_OK != (rc = sqlite3_exec(db, create_idx_sql, NULL, NULL, &errmsg))) {
		fprintf(stderr, "Not Fatal: sqlite error creating index %s: %s\n", create_idx_sql, errmsg);
		sqlite3_free(errmsg);
	}

	char create_idxtrip_sql[] = "CREATE INDEX IF NOT EXISTS IDX_OBDTRIP ON obd (trip)";

	if(SQLITE_OK != (rc = sqlite3_exec(db, create_idxtrip_sql, NULL, NULL, &errmsg))) {
		fprintf(stderr, "Not Fatal: sqlite error creating index %s: %s\n", create_idxtrip_sql, errmsg);
		sqlite3_free(errmsg);
	}

	return 0;
}
 
int createobdinsertstmt(sqlite3 *db,sqlite3_stmt **ret_stmt, void *obdcaps) {
		// TODO calculate buffer size and create correct sized one,
		//   otherwise this could overflow if obdservicecommands contains a *lot* of non-NULL fields
	int i;

	int columncount = 0;
	char insert_sql[4096] = "INSERT INTO obd (";
	for(i=0; i<sizeof(obdcmds_mode1)/sizeof(obdcmds_mode1[0]); i++) {
		if(NULL != obdcmds_mode1[i].db_column  && isobdcapabilitysupported(obdcaps,i)) {
			strcat(insert_sql,obdcmds_mode1[i].db_column);
			strcat(insert_sql,",");
			columncount++;
		}
	}
	strcat(insert_sql,"time,trip) VALUES (");
	for(i=0; i<columncount; i++) {
		strcat(insert_sql,"?,");
	}
	strcat(insert_sql,"?,?)");

	columncount++; // for time
	// printf("insert_sql:\n  %s\n", insert_sql);

	int rc;
	const char *zTail;
	rc = sqlite3_prepare_v2(db,insert_sql,-1,ret_stmt,&zTail);
	if(SQLITE_OK != rc) {
		fprintf(stderr, "Can't prepare statement %s: %s\n", insert_sql, sqlite3_errmsg(db));
		return 0;
	}

	return columncount;
}


int obdbegintransaction(sqlite3 *db) {
	int rc;
	char *errmsg;

	if(SQLITE_OK != (rc = sqlite3_exec(db, "BEGIN", NULL, NULL, &errmsg))) {
		fprintf(stderr, "Not Fatal: couldn't begin transaction: %s\n", errmsg);
		sqlite3_free(errmsg);
	}

	return (SQLITE_OK!=rc);
}


/// Commit a transaction
int obdcommittransaction(sqlite3 *db) {
	int rc;
	char *errmsg;

	if(SQLITE_OK != (rc = sqlite3_exec(db, "COMMIT", NULL, NULL, &errmsg))) {
		fprintf(stderr, "Not Fatal: couldn't commit transaction: %s\n", errmsg);
		sqlite3_free(errmsg);
	}

	return (SQLITE_OK!=rc);
}

