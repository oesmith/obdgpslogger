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
 \brief obd2gps main headers
 */
#ifndef __OBD2GPX_H
#define __OBD2GPX_H

#include <getopt.h>

/// Default out filename
#define DEFAULT_OUTFILENAME "./obdgpslogger.gpx"

/// getopt_long long options
static const struct option gpxlongopts[] = {
	{ "help", no_argument, NULL, 'h' }, ///< Print the help text
	{ "version", no_argument, NULL, 'v' }, ///< Print the version text
	{ "db", required_argument, NULL, 'd' }, ///< Database file
	{ "out", required_argument, NULL, 'o' }, ///< Output file
	{ NULL, 0, NULL, 0 } ///< End
};


/// getopt() short options
static const char gpxshortopts[] = "hs:e:vpd:o:";


/// Print gpx header component
void gpx_writeheader(FILE *outfile, const char *filename);

/// Print gpx tail component
void gpx_writetail(FILE *outfile);

/// Print gpx track header component
void gpx_starttrip(FILE *outfile, int tripnum);

/// Print gpx track tail component
void gpx_endtrip(FILE *outfile);

/// Print Help for --help
/** \param argv0 your program's argv[0]
 */
void gpxprinthelp(const char *argv0);

/// Print the version string
void gpxprintversion();


#endif //__OBD2GPX_H

