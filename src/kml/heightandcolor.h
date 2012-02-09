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

#ifndef __HEIGHTANDCOLOR_H
#define __HEIGHTANDCOLOR_H

#include <stdlib.h>
#include <stdio.h>

#include "sqlite3.h"


/// print db column as height in kml, normalised to maximum height
/**  Additionally, color it based on column col with style prefix stylepref
 \param db the sqlite3 database the data is in
 \param f the file to output the data to
 \param name the name of the document to output this as
 \param desc the description of the document to output this as
 \param columnname the columnname to dump it as
 \param height the max height to normalise everything to
 \param col the column to use for coloring
 \param numcols the number of colors we have, 0..numcols
 \param defaultvis default visibility [1 for on, 0 for off]
 \param start the start time we want to pull data for
 \param end the end time we want to pull data for
 */
void kmlvalueheightcolor(sqlite3 *db, FILE *f, const char *name, const char *desc, const char *columnname, int height, const char *col, int numcols, int defaultvis, double start, double end, int trip);


#endif //__HEIGHTANDCOLOR_H


