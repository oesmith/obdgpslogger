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
 \brief ECU database stuff
 */

#ifndef __ECUDB_H
#define __ECUDB_H

#include "sqlite3.h"

/// Create the ecu table in the database
int createecutable(sqlite3 *db);

/// Get the ecuid for this vin/ecu combo
/** \return -1 on error, (opaque) ecuid otherwise
*/
sqlite3_int64 getecuid(sqlite3 *db, const char *vin, long ecu);

/// Create this ecu in the database
/** will attempt to select it first. If select succeeds, will not update desc
  \return -1 on error, (opaque) ecuid otherwise
*/
sqlite3_int64 createecu(sqlite3 *db, const char *vin, long ecu, const char *ecudesc);

/// Update the passed ecu with the passed description
/** \return 0 on success, -1 on error
*/
int updateecudesc(sqlite3 *db, sqlite3_int64 ecuid, const char *ecudesc);

#endif //__ECUDB_H


