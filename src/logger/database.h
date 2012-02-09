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
   \brief Database open and close functions
*/
#ifndef __DATABASE_H
#define __DATABASE_H

#include "sqlite3.h"

/// Open the sqlite database
/** This will create the table "odb" if it does not exist.
 \param dbfilename filename of the database
 \return an sqlite3 handle if successful, NULL otherwise
*/
sqlite3 *opendb(const char *dbfilename);

/// Close the sqlite database
void closedb(sqlite3 *db);


#endif //__DATABASE_H

