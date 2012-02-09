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
#include "sqlite3.h"

#include "analysistables.h"
#include "examinetrips.h"

int createAnalysisTables(sqlite3 *db) {
	int rc;
	char *errmsg;


	const char attachsql[] = "ATTACH DATABASE \":memory:\" AS analysis";

	if(SQLITE_OK != (rc = sqlite3_exec(db, attachsql, NULL, NULL, &errmsg))) {
		fprintf(stderr, "sqlite error on statement %s (%i): %s\n", attachsql, rc, errmsg);
		sqlite3_free(errmsg);
		return -1;
	}

	const char gpssql[] = "CREATE TABLE IF NOT EXISTS analysis.gpsanalysis "
				"(trip INTEGER UNIQUE, length REAL, "
				"meanlat REAL, meanlon REAL, "
				"medianlat REAL, medianlon REAL)";

	if(SQLITE_OK != (rc = sqlite3_exec(db, gpssql, NULL, NULL, &errmsg))) {
		fprintf(stderr, "sqlite error on statement %s (%i): %s\n", gpssql, rc, errmsg);
		sqlite3_free(errmsg);
		return -1;
	}

	const char obdsql[] = "CREATE TABLE IF NOT EXISTS analysis.obdanalysis "
				"(trip INTEGER UNIQUE, petrolusage REAL)";

	if(SQLITE_OK != (rc = sqlite3_exec(db, obdsql, NULL, NULL, &errmsg))) {
		fprintf(stderr, "sqlite error on statement %s (%i): %s\n", obdsql, rc, errmsg);
		sqlite3_free(errmsg);
		return -1;
	}

	const char clustersql[] = "CREATE TABLE IF NOT EXISTS analysis.clusterdistance "
				"(tripA INTEGER, tripB INTEGER, meandist REAL, mediandist REAL)";

	if(SQLITE_OK != (rc = sqlite3_exec(db, clustersql, NULL, NULL, &errmsg))) {
		fprintf(stderr, "sqlite error on statement %s (%i): %s\n", obdsql, rc, errmsg);
		sqlite3_free(errmsg);
		return -1;
	}

	return 0;
}

int resetTripAnalysisTables(sqlite3 *db) {
	int rc;
	char *errmsg;


	const char gpssql[] = "DELETE FROM analysis.gpsanalysis WHERE 1";

	if(SQLITE_OK != (rc = sqlite3_exec(db, gpssql, NULL, NULL, &errmsg))) {
		fprintf(stderr, "sqlite error on statement %s (%i): %s\n", gpssql, rc, errmsg);
		sqlite3_free(errmsg);
		return -1;
	}

	const char obdsql[] = "DELETE FROM analysis.obdanalysis WHERE 1";

	if(SQLITE_OK != (rc = sqlite3_exec(db, obdsql, NULL, NULL, &errmsg))) {
		fprintf(stderr, "sqlite error on statement %s (%i): %s\n", obdsql, rc, errmsg);
		sqlite3_free(errmsg);
		return -1;
	}

	const char clustersql[] = "DELETE FROM analysis.clusteranalysis WHERE 1";

	if(SQLITE_OK != (rc = sqlite3_exec(db, clustersql, NULL, NULL, &errmsg))) {
		fprintf(stderr, "sqlite error on statement %s (%i): %s\n", clustersql, rc, errmsg);
		sqlite3_free(errmsg);
		return -1;
	}

	return 0;
}

int meanMedianDistances(sqlite3 *db) {
	const char tripsql[] = "SELECT trip,meanlat,meanlon,medianlat,medianlon FROM "
				"analysis.gpsanalysis WHERE trip>?";

	const char clustersql[] = "INSERT INTO analysis.clusterdistance "
				"(tripA, tripB, meandist, mediandist) VALUES "
				"(?,?,?,?) ";

	int rc;
	sqlite3_stmt *tripselect1, *tripselect2;
	sqlite3_stmt *insertcluster;

	rc = sqlite3_prepare_v2(db, tripsql, -1, &tripselect1, NULL);

	if(SQLITE_OK != rc) {
		fprintf(stderr, "Couldn't create select statement \"%s\" (%i): %s\n",
						tripsql, rc, sqlite3_errmsg(db));
		return -1;
	}

	rc = sqlite3_prepare_v2(db, tripsql, -1, &tripselect2, NULL);

	if(SQLITE_OK != rc) {
		fprintf(stderr, "Couldn't create select statement \"%s\" (%i): %s\n",
						tripsql, rc, sqlite3_errmsg(db));
		return -1;
	}

	rc = sqlite3_prepare_v2(db, clustersql, -1, &insertcluster, NULL);

	if(SQLITE_OK != rc) {
		fprintf(stderr, "Couldn't create insert statement \"%s\" (%i): %s\n",
						clustersql, rc, sqlite3_errmsg(db));
		return -1;
	}
	sqlite3_bind_int(tripselect1, 1, -1);

	while(SQLITE_ROW == sqlite3_step(tripselect1)) {
		int tripA = sqlite3_column_int(tripselect1, 0);
		double meanlatA = sqlite3_column_double(tripselect1, 1);
		double meanlonA = sqlite3_column_double(tripselect1, 2);
		double medianlatA = sqlite3_column_double(tripselect1, 3);
		double medianlonA = sqlite3_column_double(tripselect1, 4);

		sqlite3_reset(tripselect2);
		sqlite3_bind_int(tripselect2, 1, tripA);
		while(SQLITE_ROW == sqlite3_step(tripselect2)) {
			int tripB = sqlite3_column_int(tripselect2, 0);
			double meanlatB = sqlite3_column_double(tripselect2, 1);
			double meanlonB = sqlite3_column_double(tripselect2, 2);
			double medianlatB = sqlite3_column_double(tripselect2, 3);
			double medianlonB = sqlite3_column_double(tripselect2, 4);

			double meanCluster = haversine_dist(meanlatA, meanlonA, meanlatB, meanlonB);
			double medianCluster = haversine_dist(medianlatA, medianlonA, medianlatB, medianlonB);

			sqlite3_reset(insertcluster);
			sqlite3_bind_int(insertcluster, 1, tripA);
			sqlite3_bind_int(insertcluster, 2, tripB);
			sqlite3_bind_double(insertcluster, 3, meanCluster);
			sqlite3_bind_double(insertcluster, 3, medianCluster);

			sqlite3_step(insertcluster);
		}
	}

	sqlite3_finalize(insertcluster);
	sqlite3_finalize(tripselect1);
	sqlite3_finalize(tripselect2);

	return 0;
}

int insertTripAnalysis(sqlite3 *db, int trip, double length,
			double meanlat, double meanlon, double medianlat, double medianlon) {
	const char sql[] = "INSERT OR REPLACE INTO analysis.gpsanalysis "
				"(trip, length, meanlat, meanlon, medianlat, medianlon) "
				"VALUES "
				"(?,?,?,?,?,?)";

	int rc;
	sqlite3_stmt *stmt;

	rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

	if(SQLITE_OK != rc) {
		fprintf(stderr, "Couldn't create insert statement \"%s\" (%i): %s\n",
						sql, rc, sqlite3_errmsg(db));
		return -1;
	}

	sqlite3_bind_int(stmt, 1, trip);
	sqlite3_bind_double(stmt, 2, length);
	sqlite3_bind_double(stmt, 3, meanlat);
	sqlite3_bind_double(stmt, 4, meanlon);
	sqlite3_bind_double(stmt, 5, medianlat);
	sqlite3_bind_double(stmt, 6, medianlon);

	rc = sqlite3_step(stmt);

	sqlite3_finalize(stmt);
	return 0;
}

int getTripAnalysis(sqlite3 *db, int trip, double *length,
			double *meanlat, double *meanlon, double *medianlat, double *medianlon) {

	const char sql[] = "SELECT length, meanlat, meanlon, medianlat, medianlon "
				"FROM analysis.gpsanalysis WHERE trip=?";

	int rc;
	sqlite3_stmt *stmt;

	rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

	if(SQLITE_OK != rc) {
		fprintf(stderr, "Couldn't create select statement \"%s\" (%i): %s\n",
						sql, rc, sqlite3_errmsg(db));
		return -1;
	}

	sqlite3_bind_int(stmt, 1, trip);

	int retvalue = -1;

	if(SQLITE_ROW == (rc = sqlite3_step(stmt))) {
		if(NULL != length) {
			*length = sqlite3_column_double(stmt, 0);
		}
		if(NULL != meanlat) {
			*meanlat = sqlite3_column_double(stmt, 1);
		}
		if(NULL != meanlon) {
			*meanlon = sqlite3_column_double(stmt, 2);
		}
		if(NULL != medianlat) {
			*medianlat = sqlite3_column_double(stmt, 3);
		}
		if(NULL != medianlon) {
			*medianlon = sqlite3_column_double(stmt, 4);
		}

		retvalue = 0;
	}

	sqlite3_finalize(stmt);
	return retvalue;
}

int fillAnalysisTables(sqlite3 *db) {
	const char sql[] = "SELECT DISTINCT tripid FROM trip ORDER BY tripid";

	int rc;
	sqlite3_stmt *stmt;

	rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

	if(SQLITE_OK != rc) {
		fprintf(stderr, "Couldn't create select statement \"%s\" (%i): %s\n",
						sql, rc, sqlite3_errmsg(db));
		return -1;
	}

	while(SQLITE_ROW == (rc = sqlite3_step(stmt))) {
		int trip = sqlite3_column_int(stmt, 0);

		double length=tripdist(db, trip);
		double meanlat=0;
		double meanlon=0;
		double medianlat=0;
		double medianlon=0;

		int status = tripmeanmedian(db, trip, &meanlat, &meanlon, &medianlat, &medianlon);
		if(0 == status) {
			insertTripAnalysis(db, trip, length, meanlat, meanlon, medianlat, medianlon);
		}
	}

	sqlite3_finalize(stmt);

	return 0;
}

int exportGpsCSV(sqlite3 *db, FILE *f) {
	const char sql[] = "SELECT * FROM analysis.gpsanalysis ORDER BY trip";

	int rc;
	sqlite3_stmt *stmt;

	rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

	if(SQLITE_OK != rc) {
		fprintf(stderr, "Couldn't create select statement \"%s\" (%i): %s\n",
						sql, rc, sqlite3_errmsg(db));
		return -1;
	}

	if(SQLITE_ROW != (sqlite3_step(stmt))) {
		fprintf(stderr, "No rows returned from gps analysis table\n");
		return -1;
	}

	int i;
	for(i=0;i<sqlite3_column_count(stmt);i++) {
		fprintf(f, "%s,", sqlite3_column_name(stmt, i));
	}
	fprintf(f, "\n");

	sqlite3_reset(stmt);

	while(SQLITE_ROW == (rc = sqlite3_step(stmt))) {
		for(i=0;i<sqlite3_column_count(stmt);i++) {
			fprintf(f, "%f,", sqlite3_column_double(stmt, i));
		}
		fprintf(f, "\n");
	}

	sqlite3_finalize(stmt);

	return 0;
}


