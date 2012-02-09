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
 \brief Tool-wide configuration
 */
#ifndef __OBDCONFIGFILE_H
#define __OBDCONFIGFILE_H

#include "obdservicecommands.h"

#ifdef __cplusplus
extern "C" {
#endif //  __cplusplus

/// This is the config we create
struct OBDGPSConfig {
	const char *obd_device; //< Full path to the obd device
	const char *gps_device; //< Full path to the gps device
	int samplerate; //< SampleRate [number-per-second]
	int optimisations; //< Enable Optimsations
	const char *log_columns; //< Columns to log [comma-separated]
	long baudrate; //< Baudrate
	long baudrate_upgrade; //< Upgrade Baudrate
	const char *log_file; //< Log to this file
};

/// Load a config, return a struct. Must be free'd using freeOBDGPSConfig
struct OBDGPSConfig *obd_loadConfig(int verbose);

/// Free a config created by loadOBDGPSConfig
void obd_freeConfig(struct OBDGPSConfig *c);

/// Write the passed config struct to the config file
/** \return 0 on success, non-zero on failure */
int obd_writeConfig(struct OBDGPSConfig *c);

/// Get a list of obdservicecommands.
/** My god. It's full of stars
 \param log_columns comma-separated list of columns
 \param cmds when returned, this will point to a list of obdservicecmds, terminated with a NULL
 \return number of commands populated [not counting NULL sentinel] or -1 on error
 */
int obd_configCmds(const char *log_columns, struct obdservicecmd ***cmds);

/// Free a list of service commands allocated by obd_configCmds
void obd_freeConfigCmds(struct obdservicecmd **cmds);

#ifdef __cplusplus
}
#endif //  __cplusplus

#endif //__OBDCONFIGFILE_H

