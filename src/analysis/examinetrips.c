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
#include <stdlib.h>
#include <math.h>
#include "sqlite3.h"
#include "examinetrips.h"

double haversine_dist(double latA, double lonA, double latB, double lonB) {
	// Haversine formula
	// R = earth radius ~= 6,371km
	// delta lat = lat2 − lat1
	// delta lon = lon2 − lon1
	// a = sin2(delta lat/2) + cos(lat1) * cos(lat2) * sin2(delta lon/2)
	// c = 2 * atan2(sqrt(a), sqrt(1−a))
	// d = R * c

	double R = 6371;
	double dlat = latB-latA;
	double dlon = lonB-lonA;
	double sinlat = sin((dlat/2) * (M_PI/180));
	double sinlon = sin((dlon/2) * (M_PI/180));

	double a=sinlat*sinlat + sinlon*sinlon * cos(latA * (M_PI/180)) * cos(latB * (M_PI/180));
	double c = 2 * atan2(sqrt(a), sqrt(1-a));
	double d = R*c;

	return d;
}

double petrolusage(sqlite3 *db, int trip) {
	int rc;

	const char mafselect_sql[] = "SELECT SUM(a.maf*(a.time-b.time)), count(a.maf), max(a.time) - min(a.time) "
			"FROM obd a LEFT JOIN obd b "
			"ON a.rowid=b.rowid+1 "
			"WHERE a.trip=? AND b.trip=a.trip";

	sqlite3_stmt *mafstmt;

	rc = sqlite3_prepare_v2(db, mafselect_sql, -1, &mafstmt, NULL);
	if(SQLITE_OK != rc) {
		fprintf(stderr, "Cannot prepare select statement maf (%i): %s\n", rc, sqlite3_errmsg(db));
		return -1;
	}

	sqlite3_bind_int(mafstmt, 1, trip);
	printf("Trip %i ", trip);

	double trip_dist = tripdist(db, trip);
	double total_maf;
	double delta_time;
	int maf_count;
	if(SQLITE_ROW == sqlite3_step(mafstmt)) {
		total_maf = sqlite3_column_double(mafstmt,0);
		maf_count = sqlite3_column_int(mafstmt,1);
		delta_time = sqlite3_column_double(mafstmt,2);
	}

	/* const float ratio = 0.147;
	const float petrol_density = 737.22; //  kg/m^3
	const float cubic_meters_to_gallons = 264.172052;
	const float km_to_miles = 0.621371192;

	double petrol_kilograms = ratio * total_maf / 1000.0;
	double petrol_m3 = petrol_kilograms/petrol_density;
	double petrol_gal = petrol_m3 * cubic_meters_to_gallons;

	double trip_miles = trip_dist * km_to_miles;

	printf("(%.0f sec): %.2fg petrol, %.3fgal, %.2fmiles, %.1fmpg\n",
				delta_time, 1000*petrol_kilograms,
				petrol_gal, trip_miles, trip_miles/petrol_gal);
	*/

	// Magic. http://www.mp3car.com/vbulletin/engine-management-obd-ii-engine-diagnostics-etc/75138-calculating-mpg-vss-maf-obd2.html
	const double magic_number = 710.7; 

	double average_speed = trip_dist/(delta_time/3600);
	double average_maf = total_maf/(delta_time/3600);

	double mpg = magic_number * average_speed / (average_maf/100);

	printf("%.3fmaf, %.3fkm/h, %.0f sec, %.2fkm, %.1fmpg\n", average_maf, average_speed, delta_time, trip_dist, mpg);

	sqlite3_finalize(mafstmt);

	return total_maf;
}

double tripdist(sqlite3 *db, int trip) {
	int rc;

	double total_dst = 0;

	const char dstselect_sql[] = "SELECT a.lat,a.lon,b.lat,b.lon "
			"FROM gps a LEFT JOIN gps b "
			"ON b.rowid=a.rowid+1 "
			"WHERE a.trip=? AND b.trip=a.trip";

	sqlite3_stmt *dststmt;

	rc = sqlite3_prepare_v2(db, dstselect_sql, -1, &dststmt, NULL);
	if(SQLITE_OK != rc) {
		fprintf(stderr, "Cannot prepare select statement dst (%i): %s\n", rc, sqlite3_errmsg(db));
		return -1;
	}

	sqlite3_bind_int(dststmt, 1, trip);

	while(SQLITE_ROW == sqlite3_step(dststmt)) {
		total_dst += haversine_dist( sqlite3_column_double(dststmt, 0),
							sqlite3_column_double(dststmt, 1),
							sqlite3_column_double(dststmt, 2),
							sqlite3_column_double(dststmt, 3) );
	}

	sqlite3_finalize(dststmt);

	return total_dst;
}

int tripmeanmedian(sqlite3 *db, int trip, double *meanlat, double *meanlon,
	double *medianlat, double *medianlon) {

	int rc;

	const char dstselect_sql[] = "SELECT a.lat,a.lon,b.lat,b.lon "
			"FROM gps a LEFT JOIN gps b "
			"ON b.rowid=a.rowid+1 "
			"WHERE a.trip=? AND b.trip=a.trip";

	sqlite3_stmt *dststmt;

	rc = sqlite3_prepare_v2(db, dstselect_sql, -1, &dststmt, NULL);
	if(SQLITE_OK != rc) {
		fprintf(stderr, "Cannot prepare select statement dst (%i): %s\n", rc, sqlite3_errmsg(db));
		return -1;
	}

	sqlite3_bind_int(dststmt, 1, trip);

	double total_lat = 0;
	double total_lon = 0;

	int count = 0;

	double total_len = 0;

	while(SQLITE_ROW == sqlite3_step(dststmt)) {
		double latA = sqlite3_column_double(dststmt, 0);
		double lonA = sqlite3_column_double(dststmt, 1);
		double latB = sqlite3_column_double(dststmt, 2);
		double lonB = sqlite3_column_double(dststmt, 3);

		double delta = haversine_dist( latA, lonA, latB, lonB );

		total_len += delta;

		total_lat += delta * latA;
		total_lon += delta * lonA;

		count++;
	}

	sqlite3_reset(dststmt);

	double half_len = total_len / 2;
	while(SQLITE_ROW == sqlite3_step(dststmt)) {
		double latA = sqlite3_column_double(dststmt, 0);
		double lonA = sqlite3_column_double(dststmt, 1);
		double latB = sqlite3_column_double(dststmt, 2);
		double lonB = sqlite3_column_double(dststmt, 3);

		double delta = haversine_dist( latA, lonA, latB, lonB );

		half_len -= delta;

		if(half_len < 0) {
			*medianlat = latA;
			*medianlon = lonA;
			break;
		}
	}

	sqlite3_finalize(dststmt);

	if(count == 0 || total_len == 0) {
		fprintf(stderr, "Trip % i had no points; can't calculate weighted mean\n", trip);
		return -1;
	}

	*meanlat = total_lat / total_len;
	*meanlon = total_lon / total_len;

	return 0;
}

