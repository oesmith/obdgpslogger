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

#ifndef __EXAMINETRIPS_H
#define __EXAMINETRIPS_H

#include "sqlite3.h"

/// Get the distance between these co-ordinates, in km
double haversine_dist(double latA, double lonA, double latB, double lonB);

/// How much petrol do we think was drunk this trip?
double petrolusage(sqlite3 *db, int trip);

/// Total length of this trip
double tripdist(sqlite3 *db, int trip);

/// Weighted median,mean of lat and lon for this trip
int tripmeanmedian(sqlite3 *db, int trip, double *meanlat, double *meanlon,
	double *medianlat, double *medianlon);

#endif // __EXAMINETRIPS_H

