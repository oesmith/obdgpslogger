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
 \brief trip database stuff
 */

#ifndef __TRIPDB_H
#define __TRIPDB_H

#include "sqlite3.h"

/// Create the trip table in the database
int createtriptable(sqlite3 *db);

/// Create a new trip
/** \param starttime the start time of the trip
 \param db the sqlite3 database we're using
 \return opaque value for passing to endtrip()
 */
sqlite3_int64 starttrip(sqlite3 *db, double starttime);

/// End a trip
/** \param endtime the end time of the trip
 \param obdtripid the opaque value returned from starttrip()
 \param db the sqlite3 database we're using
 */
void updatetrip(sqlite3 *db, sqlite3_int64 obdtripid, double endtime);


#endif //__GPSDB_H


