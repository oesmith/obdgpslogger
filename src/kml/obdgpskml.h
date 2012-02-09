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
 \brief OBD GPS KML main entrypoint
 */
#ifndef __OBDGPSKML_H
#define __OBDGPSKML_H

#include <getopt.h>
#include "sqlite3.h"

/// Default out filename
#define DEFAULT_OUTFILENAME "./obdlogger.kml"

/// Default name for KML folder
#define DEFAULT_KMLFOLDERNAME "Output from <a href=\"http://icculus.org/obdgpslogger/\">OBD GPS Logger</a>"

/// Default max altitude
#define DEFAULT_MAXALTITUDE 1000

/// getopt_long long options
static const struct option kmllongopts[] = {
	{ "help", no_argument, NULL, 'h' }, ///< Print the help text
	{ "version", no_argument, NULL, 'v' }, ///< Print the version text
	{ "progress", no_argument, NULL, 'p' }, ///< Show progress
	{ "db", required_argument, NULL, 'd' }, ///< Database file
	{ "out", required_argument, NULL, 'o' }, ///< Output file
	{ "name", required_argument, NULL, 'n' }, ///< The "name" for this kml file
	{ "altitude", required_argument, NULL, 'a' }, ///< Max altitude
	{ NULL, 0, NULL, 0 } ///< End
};


/// getopt() short options
static const char kmlshortopts[] = "hvpd:o:a:n:";


/// Write the actual graphs
/** \param db a valid, open, sqlite3 database
 \param f an open file handle, ready to fprintf() KML folders
 \param maxaltitude altitude to normalise to
*/
void writekmlgraphs(sqlite3 *db, FILE *f, int maxaltitude);

/// Print Help for --help
/** \param argv0 your program's argv[0]
 */
void kmlprinthelp(const char *argv0);

/// Print the version string
void kmlprintversion();


#endif //__OBDGPSKML_H

