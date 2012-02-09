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

#include "justgps.h"

#include "sqlite3.h"

void gpsposvel(sqlite3 *db, FILE *f, int height, int defaultvis, double start, double end, int trip) {
	int rc; // return from sqlite
	sqlite3_stmt *stmt; // sqlite statement
	const char *dbend; // ignored handle for sqlite

	// And the actual output
	char select_sql[2048]; // the select statement

	snprintf(select_sql,sizeof(select_sql),
					"SELECT lon,lat,gpstime,speed FROM gps "
					"WHERE time>%f AND time<%f AND trip=%i AND speed IS NOT NULL "
					"GROUP BY gpstime "
					"ORDER BY gpstime "
					, start, end, trip);
	rc = sqlite3_prepare_v2(db, select_sql, -1, &stmt, &dbend);

	if(SQLITE_OK != rc) {
		printf("SQL Error in gpsposvel: %i, %s\n", rc, sqlite3_errmsg(db));
		printf("SQL: %s\n", select_sql);
		return;
	} else {

		fprintf(f,
			"<Document>\n"
			"<Style>\n"
			"<ListStyle><listItemType>checkHideChildren</listItemType></ListStyle>\n"
			"</Style>\n"
			"<visibility>%i</visibility>\n"
			"<name>Just GPS trip %i</name>\n"
			,defaultvis,trip);

		fprintf(f,"<Placemark>\n"
			"<name>chart</name>\n"
			"<LineString>\n"
			"<extrude>1</extrude>\n"
			"<tessellate>1</tessellate>\n"
			"<altitudeMode>relativeToGround</altitudeMode>\n"
			"<coordinates>");

		double firstpos[2] = {0,0};
		double lastpos[2] = {0,0};
		double lasttime = 0;

		double currpos[2] = {0,0};
		double currspeed = 0;
		double currtime = 0;

		int rowcount = 0;
		while(SQLITE_ROW == sqlite3_step(stmt)) {
			currpos[0] = sqlite3_column_double(stmt, 0);
			currpos[1] = sqlite3_column_double(stmt, 1);
			currtime = sqlite3_column_double(stmt, 2);
			currspeed = sqlite3_column_double(stmt, 3);

			if(0 == rowcount) {
				firstpos[0] = currpos[0];
				firstpos[1] = currpos[1];
				firstpos[2] = currpos[2];
			}

			fprintf(f, "%f,%f,%f\n", currpos[0],currpos[1],currspeed * 100);

			lastpos[1] = currpos[1];
			lastpos[0] = currpos[0];
			lasttime = currtime;

			rowcount++;
		}

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
				firstpos[0],firstpos[1],0.0);

		fprintf(f, "<Placemark>\n"
			"<name>End %i (%s)</name>\n"
			"<Point>\n"
			"<coordinates>\n"
			"%f,%f,%f"
			"</coordinates>\n"
			"</Point>\n"
			"</Placemark>\n", trip, ctime(&endt),
				lastpos[0],lastpos[1],0.0);

		fprintf(f,"</Document>\n");
	}

	sqlite3_finalize(stmt);
}




