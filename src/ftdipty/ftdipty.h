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

#ifndef __FTDIPTY_H
#define __FTDIPTY_H

#include <getopt.h>

/// getopt() long options
static const struct option longopts[] = {
	{ "help", no_argument, NULL, 'h' }, ///< Print the help text
	{ "version", no_argument, NULL, 'v' }, ///< Print the version text
	{ "daemonise", no_argument, NULL, 'd' }, ///< Daemonise
	{ "baud", required_argument, NULL, 'b' }, ///< Set the baudrate
	{ "modifyconf", no_argument, NULL, 'c' }, ///< Modify the config file
	{ "world-accessible", no_argument, NULL, 'w' }, ///< Make the pty world accessible
	{ "deviceid", required_argument, NULL, 'D' }, ///< Specify the device ID
	{ "vendorid", required_argument, NULL, 'V' }, ///< Specify the vendor ID
	{ NULL, 0, NULL, 0 } ///< End
};

/// getopt() short options
static const char shortopts[] = "hvwb:cdV:D:";


/// Print Help for --help
/** \param argv0 your program's argv[0]
 */
void printhelp(const char *argv0);

/// Print the version string
void printversion();

#endif // __FTDIPTY_H


