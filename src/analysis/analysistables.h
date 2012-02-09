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

#ifndef __ANALYSISTABLES_H
#define __ANALYSISTABLES_H

#include "sqlite3.h"

/// Create the analysis tables we need in this database
int createAnalysisTables(sqlite3 *db);

/// Purge any analyiss data from the tables we've created
int resetTripAnalysisTables(sqlite3 *db);

/// Insert [or update] these parameters for this trip
int insertTripAnalysis(sqlite3 *db, int trip, double length,
	double meanlat, double meanlon, double medianlat, double medianlon);

/// Get the parameters for the requested trip
int getTripAnalysis(sqlite3 *db, int trip, double *length,
	double *meanlat, double *meanlon, double *medianlat, double *medianlon);

/// Populate median-median mean-mean O(n^2) table
int meanMedianDistances(sqlite3 *db);

/// Populate the analysis tables
int fillAnalysisTables(sqlite3 *db);

/// Export gps analysis to CSV
int exportGpsCSV(sqlite3 *db, FILE *f);

#endif // __ANALYSISTABLES_H


