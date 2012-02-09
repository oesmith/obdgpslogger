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
 \brief obd database stuff
 */

#ifndef __OBDDB_H
#define __OBDDB_H

#include "sqlite3.h"

/// Create the obd table in the database
/** \param obdcaps the obdcapabilities returned from getobdcapabilities
 */
int createobdtable(sqlite3 *db, void *obdcaps);

/// Prepare the sqlite3 insert statement for the obd table
/**
 \param db the database handle this is for
 \param ret_stmt the prepared statement is placed in this value
 \param obdcaps the obdcapabilities returned from getobdcapabilities
 \return number of columns in the insert statement, or zero on fail
 */
int createobdinsertstmt(sqlite3 *db, sqlite3_stmt **ret_stmt, void *obdcaps);

/// Begin a transaction
int obdbegintransaction(sqlite3 *db);

/// Commit a transaction
int obdcommittransaction(sqlite3 *db);


#endif // __OBDDB_H

