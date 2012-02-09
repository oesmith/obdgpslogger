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

void kmlvalueheightcolor(sqlite3 *db, FILE *f, const char *name, const char *desc, const char *columnname, int height, const char *col, int numcols, int defaultvis, double start, double end, int trip) {
	int rc; // return from sqlite
	sqlite3_stmt *stmt; // sqlite statement
	const char *dbend; // ignored handle for sqlite

	char select_sql[2048]; // the select statement

	snprintf(select_sql,sizeof(select_sql),
					"SELECT %i*%s/(SELECT MAX(%s) FROM obd "
						"WHERE trip=%i) "
					"AS height,gps.lat, gps.lon, %s "
					"FROM obd INNER JOIN gps ON obd.time=gps.time "
					"WHERE obd.trip=%i",
					height, columnname, columnname, trip, col, trip);

	// printf("select sql:\n%s\n", select_sql);

	rc = sqlite3_prepare_v2(db, select_sql, -1, &stmt, &dbend);

	if(rc != SQLITE_OK) {
		printf("SQL Error in valueheightcolor: %i, %s\n", rc, sqlite3_errmsg(db));
		return;
	} else {

		// First, we have to establish the relevant percentiles
		double percentileposition[numcols+1];
		percentileposition[0] = 0.001;
		int i;
		for(i=1;i<=numcols;i++) {
			char percentile_sql[2048]; // the actual sql
			snprintf(percentile_sql, sizeof(percentile_sql),
				"SELECT %s AS ckobd FROM obd "
				"WHERE vss>0 AND obd.trip=%i "
				"ORDER BY ckobd "
				"LIMIT 1 OFFSET (SELECT %i*COUNT(*)/100 FROM obd "
					"WHERE vss>0 AND obd.trip=%i)",
				col, trip, i*100/numcols, trip);

			// printf("Percentile sql:\n%s\n", percentile_sql);

			sqlite3_stmt *pstmt; // the percentile statement
			rc = sqlite3_prepare_v2(db, percentile_sql, -1, &pstmt, &dbend);

			if(rc != SQLITE_OK) {
				printf("SQL Error in valueheightcolor percentile prepare: %i, %s\n", rc, sqlite3_errmsg(db));
				return;
			}
			sqlite3_step(pstmt);
			percentileposition[i] = sqlite3_column_double(pstmt, 0);
			sqlite3_finalize(pstmt);
		}

		fprintf(f,
			"<Document>\n"
			"<Style>\n"
			"<ListStyle><listItemType>checkHideChildren</listItemType></ListStyle>\n"
			"</Style>\n"
			"<visibility>%i</visibility>\n"
			"<name>%s</name>\n"
			"<description>%s</description>\n",defaultvis,name,desc);

		char styleprefix[]="obdgpsStyle";
		fprintf(f, "<Style id=\"%s0\">\n"
			"<LineStyle>\n"
			"<color>00000000</color>\n"
			"</LineStyle>\n"
			"</Style>\n", styleprefix);

		for(i=1;i<=numcols;i++) {
			fprintf(f, "<Style id=\"%s%i\">\n"
				"<PolyStyle>\n"
				"<color>ff00%02x%02x</color>\n"
				"</PolyStyle>\n"
				"<LineStyle>\n"
				"<color>ff00%02x%02x</color>\n"
				"</LineStyle>\n"
				"</Style>\n", styleprefix, i,
				(i-1)*0xFF/(numcols-1),
				0xFF-(i-1)*0xFF/(numcols-1),
				(i-1)*0xFF/(numcols-1),
				0xFF-(i-1)*0xFF/(numcols-1));
		}

		int lastpercentile = -1;
		double lastpos[3] = {0,0,0};

		int have_firstpos = 0;
		double firstpos[3] = {0,0,0};

		char placehead[] = "<Placemark>\n"
			"<name>chart</name>\n"
			"<styleUrl>#%s%i</styleUrl>\n"
			"<LineString>\n"
			"<extrude>1</extrude>\n"
			"<tessellate>1</tessellate>\n"
			"<altitudeMode>relativeToGround</altitudeMode>\n"
			"<coordinates>\n"; // At the top of any placemark

		char placetail[] = "</coordinates>\n"
			"</LineString>\n"
			"</Placemark>\n"; // At the tail of any placemark

		fprintf(f, placehead, styleprefix, 0);

		while(SQLITE_DONE != sqlite3_step(stmt)) {
			if(0 == have_firstpos) {
				firstpos[2] = sqlite3_column_double(stmt, 2);
				firstpos[1] = sqlite3_column_double(stmt, 1);
				firstpos[0] = sqlite3_column_double(stmt, 0);
				have_firstpos = 1;
			}
			double perc = sqlite3_column_double(stmt, 3);
			for(i=0;i<numcols;i++) {
				if(percentileposition[i] > perc) break;
			}

			// i contains the current percentile
			if (lastpercentile != i) {
				if(0 <= lastpercentile) {
					fprintf(f, "%s", placetail);
					fprintf(f, placehead, styleprefix, i);
					fprintf(f, "%f,%f,%f\n", lastpos[2], lastpos[1], lastpos[0]);
				}
			}
			fprintf(f, "%f,%f,%f\n", sqlite3_column_double(stmt, 2),sqlite3_column_double(stmt, 1),sqlite3_column_double(stmt, 0));
			lastpos[2] = sqlite3_column_double(stmt, 2);
			lastpos[1] = sqlite3_column_double(stmt, 1);
			lastpos[0] = sqlite3_column_double(stmt, 0);

			lastpercentile = i;
		}

		fprintf(f, "%s", placetail);


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




