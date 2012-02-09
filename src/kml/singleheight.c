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
 \brief Dump a single value in a db as height in a KML document
 */


#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#include "singleheight.h"

#include "sqlite3.h"

/// A distance greater than this is considered to be not zero
#define EPSILONDIST 0.000001

void kmlvalueheight(sqlite3 *db, FILE *f, const char *name, const char *desc, const char *columnname, int height, int defaultvis, double start, double end, int trip) {
	int rc; // return from sqlite
	sqlite3_stmt *stmt; // sqlite statement
	const char *dbend; // ignored handle for sqlite

	// For normalising the data
	float normalfactor = 1;
	sqlite3_stmt *normal_stmt;
	char normal_sql[1024];
	
	snprintf(normal_sql, sizeof(normal_sql),
			"SELECT %i/(SELECT MAX(%s) FROM obd WHERE trip=%i)",
			height, columnname, trip
			);
	rc = sqlite3_prepare_v2(db, normal_sql, -1, &normal_stmt, &dbend);
	if(SQLITE_OK != rc) {
		printf("SQL Error in valueheight: %i, %s\n", rc, sqlite3_errmsg(db));
		printf("SQL: %s\n", normal_sql);
		return;
	} else {
		if(SQLITE_ROW != sqlite3_step(normal_stmt)) {
			printf("SQL Error stepping: %i, %s\n", rc, sqlite3_errmsg(db));
			printf("SQL: %s\n", normal_sql);
			return;
		}
		normalfactor=sqlite3_column_double(normal_stmt, 0);
	}
	sqlite3_finalize(normal_stmt);
	
	// And the actual output
	char select_sql[2048]; // the select statement

	snprintf(select_sql,sizeof(select_sql),
					"SELECT T1.obdkmlthing AS height,gps.lat,gps.lon "
					"FROM (SELECT %s AS obdkmlthing,time FROM obd WHERE trip=%i) AS T1 "
					"INNER JOIN gps "
					"ON T1.time=gps.time "
					"WHERE gps.trip=%i ",
					columnname, trip, trip);

	rc = sqlite3_prepare_v2(db, select_sql, -1, &stmt, &dbend);

	if(SQLITE_OK != rc) {
		printf("SQL Error in valueheight: %i, %s\n", rc, sqlite3_errmsg(db));
		printf("SQL: %s\n", select_sql);
		return;
	} else {

		fprintf(f,
			"<Document>\n"
			"<Style>\n"
			"<ListStyle><listItemType>checkHideChildren</listItemType></ListStyle>\n"
			"</Style>\n"
			"<visibility>%i</visibility>\n"
			"<name>%s</name>\n"
			"<description>%s</description>\n",defaultvis,name,desc);

		fprintf(f,"<Placemark>\n"
			"<name>chart</name>\n"
			"<LineString>\n"
			"<extrude>1</extrude>\n"
			"<tessellate>1</tessellate>\n"
			"<altitudeMode>relativeToGround</altitudeMode>\n"
			"<coordinates>");

		int ismoving=1; // Set when the car is moving

		int have_firstpos = 0;
		double firstpos[3] = {0,0,0};
		double lastpos[3] = {0,0,0};

		// Number of rows, total
		long rowcount = 0;
		// Number of rows output to kml
		long outputcount = 0;

		double totalheight = 0;
		while(SQLITE_ROW == sqlite3_step(stmt)) {
			rowcount++;

			if(0 == have_firstpos) {
				firstpos[2] = sqlite3_column_double(stmt, 2);
				firstpos[1] = sqlite3_column_double(stmt, 1);
				firstpos[0] = sqlite3_column_double(stmt, 0);
				have_firstpos = 1;
			}

			double currpos[3];
			currpos[2] = sqlite3_column_double(stmt, 2);
			currpos[1] = sqlite3_column_double(stmt, 1);
			currpos[0] = sqlite3_column_double(stmt, 0);

			float delta = sqrt((currpos[2] - lastpos[2]) * (currpos[2] - lastpos[2]) +
					(currpos[1] - lastpos[1]) * (currpos[1] - lastpos[1]));
			float height = normalfactor * sqlite3_column_double(stmt, 0);
			if(delta > EPSILONDIST) {
				ismoving = 1;
			}
			if(ismoving) {
				outputcount++;
				fprintf(f, "%f,%f,%f\n", currpos[2],currpos[1],height);
				totalheight += height;
			}
			if(delta < EPSILONDIST) {
				ismoving = 0;
			}

			lastpos[2] = currpos[2];
			lastpos[1] = currpos[1];
			lastpos[0] = currpos[0];

		}

		printf("Total db rows: %li. KML rows: %li. Ignored rows: %li %s\n", rowcount, outputcount,
						rowcount - outputcount,
						outputcount<(rowcount-outputcount)?"\nOutput rows seems low":"");
		fprintf(f,"</coordinates>\n"
			"</LineString>\n"
			"</Placemark>\n");

		// Now print start and end beacons
		time_t endt = (time_t)floor(end);
		time_t startt = (time_t)floor(start);

		fprintf(f, "<Placemark>\n"
			"<name>Start %i (%s)</name>\n"
			"<Point>\n"
			"<coordinates>\n"
			"%f,%f,%f"
			"</coordinates>\n"
			"</Point>\n"
			"</Placemark>\n", trip, ctime(&startt),
				firstpos[2],firstpos[1],firstpos[0]);

		fprintf(f, "<Placemark>\n"
			"<name>End %i (%s)</name>\n"
			"<Point>\n"
			"<coordinates>\n"
			"%f,%f,%f"
			"</coordinates>\n"
			"</Point>\n"
			"</Placemark>\n", trip, ctime(&endt),
				lastpos[2],lastpos[1],lastpos[0]);

		fprintf(f,"</Document>\n");
	}

	sqlite3_finalize(stmt);
}




