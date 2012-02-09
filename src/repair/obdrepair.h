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

#ifndef __OBDREPAIR_H
#define __OBDREPAIR_H

#include "sqlite3.h"

/// Check indices on tables
/** \return 0 if we changed nothing. -1 for error. >0 if we changed stuff. */
int checkindices(sqlite3 *db);

/// Check ECU column on obd table
/** \return 0 if we changed nothing. -1 for error. >0 if we changed stuff. */
int checkobdecu(sqlite3 *db);

/// Fix ends of trips
/** \return 0 if we changed nothing. -1 for error. >0 if we changed stuff. */
int checktripends(sqlite3 *db);

/// Fix trip ids
/** \return 0 if we changed nothing. -1 for error. >0 if we changed stuff. */
int checktripids(sqlite3 *db, const char *table_name);

/// Run ANALYZE against the db
int analyze(sqlite3 *db);

/// Check the integrity of the database
int checkintegrity(sqlite3 *db);

#endif // __OBDREPAIR_H


