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

#include "sqlite3.h"

#include "obdrepair.h"

/// Internal function for checkindices
/** \return 0 if we changed nothing. -1 for error. >0 if we changed stuff. */
int checkindex(sqlite3 *db, const char *indexname, const char *tablename, const char *indexcolumn, int unique);


int analyze(sqlite3 *db) {
	
	char *errmsg;

	if(SQLITE_OK != sqlite3_exec(db, "ANALYZE", NULL, NULL, &errmsg)) {
		fprintf(stderr, "ANALYZE error. SQL reported: %s\n", errmsg);
		sqlite3_free(errmsg);
		return -1;
	} else {
		printf("Ran ANALYZE\n");
		return 0;
	}
}

int checkintegrity(sqlite3 *db) {
	sqlite3_stmt *stmt;
	int rc;
	char pragma_sql[] = "PRAGMA integrity_check(1000)";

	if(SQLITE_OK != (rc = sqlite3_prepare_v2(db, pragma_sql, -1, &stmt, NULL))) {
		fprintf(stderr,"Error preparing SQL: (%i) %s\nSQL: \"%s\"\n", rc, sqlite3_errmsg(db), pragma_sql);
		return -1;
	}

	int found_error = 0;
	while(SQLITE_ROW == sqlite3_step(stmt)) {
		const char *integ = sqlite3_column_text(stmt, 0);
		if(0 != strcmp(integ, "ok")) found_error++;
		printf("Integrity response: %s\n", integ);
	}

	if(found_error > 0) {
		printf("Found error in integrity check. Cannot safely continue\n");
	}

	sqlite3_finalize(stmt);

	return found_error;
}

int checkobdecu(sqlite3 *db) {
	int retvalue = 0;
	int rc = 0;
	char *errmsg = NULL;

	sqlite3_stmt *stmt;

	char pragma_sql[256];
	snprintf(pragma_sql, sizeof(pragma_sql), "PRAGMA table_info(obd)");

	if(SQLITE_OK != (rc = sqlite3_prepare_v2(db, pragma_sql, -1, &stmt, NULL))) {
		fprintf(stderr,"Error preparing SQL: (%i) %s\nSQL: \"%s\"\n", rc, sqlite3_errmsg(db), pragma_sql);
		return -1;
	}

	int found_ecu_col = 0;
	while(SQLITE_ROW == sqlite3_step(stmt)) {
		if(0 == strcmp("ecu",sqlite3_column_text(stmt, 1))) {
			found_ecu_col = 1;
		}
	}

	sqlite3_finalize(stmt);

	if(0 == found_ecu_col) {
		char addcol_sql[256];
		snprintf(addcol_sql, sizeof(addcol_sql), "ALTER TABLE obd ADD ecu INTEGER DEFAULT 0");

		if(SQLITE_OK != sqlite3_exec(db, addcol_sql, NULL, NULL, &errmsg)) {
			fprintf(stderr, "ALTER db. SQL reported: %s\nSQL: \"%s\"\n", errmsg, addcol_sql);
			sqlite3_free(errmsg);
			return -1;
		} else {
			printf("Ran ALTER sql: \"%s\"\n", addcol_sql);
			retvalue++;
		}
	}

	return retvalue;
}

int checkindices(sqlite3 *db) {
	int retvalue = 0;

	int rc;
	rc = checkindex(db, "IDX_GPSTIME", "gps", "time", 0);
	if(-1 == rc) return -1;
	if(rc > 0) retvalue++;

	checkindex(db, "IDX_GPSTRIP", "gps", "trip", 0);
	if(-1 == rc) return -1;
	if(rc > 0) retvalue++;

	checkindex(db, "IDX_OBDTIME", "obd", "time", 0);
	if(-1 == rc) return -1;
	if(rc > 0) retvalue++;

	checkindex(db, "IDX_OBDTRIP", "obd", "trip", 0);
	if(-1 == rc) return -1;
	if(rc > 0) retvalue++;

	checkindex(db, "IDX_VINECU", "ecu", "vin,ecu", 1);
	if(-1 == rc) return -1;
	if(rc > 0) retvalue++;

	return retvalue;
}

int checkindex(sqlite3 *db, const char *indexname, const char *tablename, const char *indexcolumn, int unique) {
	int retvalue = 0;

	int rc;

	char idx_list_sql[64];

	snprintf(idx_list_sql, sizeof(idx_list_sql), "PRAGMA index_list(%s)", tablename);
	sqlite3_stmt *idx_list_stmt;

	if(SQLITE_OK != (rc = sqlite3_prepare_v2(db, idx_list_sql, -1, &idx_list_stmt, NULL))) {
		fprintf(stderr,"Error preparing SQL: (%i) %s\nSQL: \"%s\"\n", rc, sqlite3_errmsg(db), idx_list_sql);
		return -1;
	}

	int idx_found = 0;
	while(SQLITE_ROW == sqlite3_step(idx_list_stmt)) {
		const char *c = (const char *)sqlite3_column_text(idx_list_stmt, 1);
		if(0 == strcmp(indexname,c)) {
			printf("Found %s on table %s\n", indexname, tablename);
			idx_found++;
			break;
		}
	}

	sqlite3_finalize(idx_list_stmt);

	if(0 == idx_found) {
		char create_idx_sql[128];
		snprintf(create_idx_sql, sizeof(create_idx_sql), "CREATE %s INDEX %s ON %s (%s)",
			unique?"UNIQUE":"", indexname, tablename, indexcolumn);

		char *errmsg;
		if(SQLITE_OK != (rc = sqlite3_exec(db, create_idx_sql, NULL, NULL, &errmsg))) {
			fprintf(stderr, "Error creating idx. sqlite reported: %s\nSQL: \"%s\"\n", errmsg, create_idx_sql);
			sqlite3_free(errmsg);
			retvalue = -1;
		} else {
			printf("Added idx to %s(%s): \"%s\"\n", tablename, indexcolumn, create_idx_sql);
			retvalue++;
		}
	}

	return retvalue;
}

int checktripends(sqlite3 *db) {
	int retvalue = 0;
	char *errmsg = NULL;

	const char update_sql[] = "UPDATE trip SET end="
			"(SELECT (t2.start-0.01) AS newstart from trip t2 WHERE trip.tripid=t2.tripid+1) "
		"WHERE trip.end = -1";

	if(SQLITE_OK != sqlite3_exec(db, update_sql, NULL, NULL, &errmsg)) {
		fprintf(stderr, "UPDATE db. SQL reported: %s\nSQL: \"%s\"\n", errmsg, update_sql);
		sqlite3_free(errmsg);
		return -1;
	} else {
		printf("Ran UPDATE sql: \"%s\"\n", update_sql);
	}

	return retvalue;
}

int checktripids(sqlite3 *db, const char *table_name) {
	int retvalue = 0;
	int rc = 0;
	char *errmsg = NULL;

	sqlite3_stmt *stmt;

	char pragma_sql[256];
	snprintf(pragma_sql, sizeof(pragma_sql), "PRAGMA table_info(%s)", table_name);

	if(SQLITE_OK != (rc = sqlite3_prepare_v2(db, pragma_sql, -1, &stmt, NULL))) {
		fprintf(stderr,"Error preparing SQL: (%i) %s\nSQL: \"%s\"\n", rc, sqlite3_errmsg(db), pragma_sql);
		return -1;
	}

	int found_trip_col = 0;
	while(SQLITE_ROW == sqlite3_step(stmt)) {
		if(0 == strcmp("trip",sqlite3_column_text(stmt, 1))) {
			found_trip_col = 1;
		}
	}

	sqlite3_finalize(stmt);

	if(0 == found_trip_col) {
		char addcol_sql[256];
		snprintf(addcol_sql, sizeof(addcol_sql), "ALTER TABLE %s ADD trip INTEGER", table_name);

		if(SQLITE_OK != sqlite3_exec(db, addcol_sql, NULL, NULL, &errmsg)) {
			fprintf(stderr, "ALTER db. SQL reported: %s\nSQL: \"%s\"\n", errmsg, addcol_sql);
			sqlite3_free(errmsg);
			return -1;
		} else {
			printf("Ran ALTER sql: \"%s\"\n", addcol_sql);
		}

		sqlite3_stmt *trip_stmt;
		sqlite3_stmt *update_stmt;
		
		const char trip_sql[] = "SELECT tripid, start, end FROM trip";
		char update_sql[1024];
		snprintf(update_sql, sizeof(update_sql), "UPDATE %s SET trip=? WHERE time>? AND time<?", table_name);

		if(SQLITE_OK != (rc = sqlite3_prepare_v2(db, update_sql, -1, &update_stmt, NULL))) {
			fprintf(stderr, "UPDATE db. SQL reported: %s\nSQL: \"%s\"\n", sqlite3_errmsg(db), update_sql);
			return -1;
		}

		if(SQLITE_OK != (rc = sqlite3_prepare_v2(db, trip_sql, -1, &trip_stmt, NULL))) {
			fprintf(stderr, "Trip select: %s\nSQL: \"%s\"\n", sqlite3_errmsg(db), trip_sql);
			return -1;
		}

		while(SQLITE_ROW == sqlite3_step(trip_stmt)) {
			printf("Updating trip; trip %i: %f<time<%f\n",
				sqlite3_column_int(trip_stmt, 0),
				sqlite3_column_double(trip_stmt, 1),
				sqlite3_column_double(trip_stmt, 2));

			sqlite3_reset(update_stmt);
			sqlite3_bind_int(update_stmt, 1, sqlite3_column_int(trip_stmt, 0));
			sqlite3_bind_double(update_stmt, 2, sqlite3_column_double(trip_stmt, 1));
			sqlite3_bind_double(update_stmt, 3, sqlite3_column_double(trip_stmt, 2));
			sqlite3_step(update_stmt);
		
			retvalue++;
		}

		sqlite3_finalize(update_stmt);
		sqlite3_finalize(trip_stmt);
	}
	
	return retvalue;
}

int checktimesagainstgps(sqlite3 *db) {
	char getdelta_sql[] = "SELECT trip,ROUND(AVG(gpstime-time)) FROM gps WHERE gpstime IS NOT NULL AND trip IS NOT NULL GROUP BY trip";
	char applydelta_gps_sql[] = "UPDATE gps SET time=time+? WHERE trip=?";
	char applydelta_obd_sql[] = "UPDATE obd SET time=time+? WHERE trip=?";
	char applydelta_trip_sql[] = "UPDATE trip SET start=start+?,end=end+? WHERE trip=?";
}

