/* Copyright 2009 Gary Briggs, Michael Carpenter

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
 \brief OBD Simulator Main Entrypoint
*/
#ifndef __OBDSIM_H
#define __OBDSIM_H

#include <getopt.h>
#include <stdlib.h>

#include "obdservicecommands.h"

/// This is the elm prompt
#define ELM_PROMPT ">"

/// Default hide headers
#define ELM_HEADERS 0

/// Default show spaces
#define ELM_SPACES 1

/// Default echo
#define ELM_ECHO 1

/// Default linefeed
#define ELM_LINEFEED 1

/// Default timeout, milliseconds
#define ELM_TIMEOUT 200

/// Adaptive timing [0,1,2]
#define ELM_ADAPTIVETIMING 1

/// DLC Dispaly
#define ELM_DISPLAYDLC 0

/// ELM "don't know" prompt
#define ELM_QUERY_PROMPT "?"

/// ELM "OK" prompt
#define ELM_OK_PROMPT "OK"

/// ELM "NO DATA" prompt
#define ELM_NODATA_PROMPT "NO DATA"

/// Default sim generator
#define DEFAULT_SIMGEN "Cycle"

/// Default windows port
#define DEFAULT_WINPORT "CNCA0"

/// Length of time to sleep between nonblocking reads [us]
#define OBDSIM_SLEEPTIME 1000

/// Print out benchmarks every this often [seconds]
#define OBDSIM_BENCHMARKTIME 10

/// Default car battery voltage
#define OBDSIM_BATTERYV 11.8

/// Hardcode maximum number of ECUs/generators
#define OBDSIM_MAXECUS 6

/// Max number of frames for freeze frame
#define OBDSIM_MAXFREEZEFRAMES 5



/// This is a frozen frame
struct freezeframe {
	unsigned int values[sizeof(obdcmds_mode1)/sizeof(obdcmds_mode1[0])][4]; //< Up to four values for each pid
	unsigned int valuecount[sizeof(obdcmds_mode1)/sizeof(obdcmds_mode1[0])]; //< Number of values stored for each pid
};


/// An array of these is created, each for a different ECU
struct obdgen_ecu {
	struct obdsim_generator *simgen; //< The actual data generator
	unsigned int ecu_num; //< The ECU that this will respond as
	char *seed; //< The seed used to create this simgen
	int lasterrorcount; //< Number of errors last time the number of frozen frames changed
	int ffcount; //< Current number of frozen frames
	struct freezeframe ff[OBDSIM_MAXFREEZEFRAMES]; //< Frozen frames
	void *dg; //< The generator created by this ecu
	int customdelay; //< This ECU takes this long to respond, ms
};

/// This is used for customdelays
/** It implements the order that ecus
respond in, and the delay before each one responds */
struct obdgen_ecudelays {
	struct obdgen_ecu *ecu; //< A pointer to the ecu
	long delay; //< How long to wait [after the last ecu in this list responded]
};

/// All of the settings relating to the sim go into this
struct simsettings {
	int e_headers; // Whether to show headers
	int e_spaces; // Whether to show spaces
	int e_echo; // Whether to echo commands
	int e_linefeed; // Whether to print \n as well as \r
	int e_timeout; // The timeout on requests
	int e_adaptive; // "Adaptive timing" [improves responsiveness]
	int e_dlc; // Display DLC

	int e_autoprotocol; // Whether or not we put the "A" and "Auto, " prefix on DP/DPN
	struct obdiiprotocol *e_protocol; // Starting protocol

	int benchmark; // Benchmark frequency
	float e_currentvoltage; // The current battery voltage

	char *device_identifier;
	char *elm_device;
	char *elm_version;

	struct obdgen_ecu ecus[OBDSIM_MAXECUS]; // All the ECUs
	int ecu_count;

	struct obdgen_ecudelays ecudelays[OBDSIM_MAXECUS]; // ECUs are queried in this order
};


// Fix "variable obdcmds defined but not used" warnings
#ifdef __GNUC__
#define VARIABLE_IS_NOT_USED __attribute__ ((unused))
#else
#define VARIABLE_IS_NOT_USED
#endif

/// There are 11 or so protocols, but fewer types of header layout
enum protocol_headertype {
	OBDHEADER_NULL, // Not recognised headers
	OBDHEADER_J1850PWM,
	OBDHEADER_J1850VPW,
	OBDHEADER_14230,
	OBDHEADER_CAN29,
	OBDHEADER_CAN11
};

/// To a reset on the elm device [ATZ/D/WS]
void obdsim_elmreset(struct simsettings *s);

/// Each OBDII Protocol has a number and description
struct obdiiprotocol {
	char protocol_num;
	const char *protocol_desc;
	enum protocol_headertype headertype;
};

/// All the protocols I know or care about 
/** Borrowed from the ELM327 datasheet */
static struct obdiiprotocol VARIABLE_IS_NOT_USED obdprotocols[] = {
	{ '0', "Automatic", OBDHEADER_NULL },
	{ '1', "SAE J1850 PWM", OBDHEADER_J1850PWM }, // 41.6 Kbaud
	{ '2', "SAE J1850 VPW", OBDHEADER_J1850VPW }, // 10.4 Kbaud
	{ '3', "ISO 9141-2", OBDHEADER_J1850VPW }, // 5 baud init, 10.4 Kbaud
	{ '4', "ISO 14230-4 (KWP 5BAUD)", OBDHEADER_14230 }, // 5 baud init, 10.4 Kbaud
	{ '5', "ISO 14230-4 (KWP FAST)", OBDHEADER_14230 }, // fast init, 10.4 Kbaud
	{ '6', "ISO 15765-4 (CAN 11/500)", OBDHEADER_CAN11 }, // 11 bit ID, 500 Kbaud
	{ '7', "ISO 15765-4 (CAN 29/500)", OBDHEADER_CAN29 }, // 29 bit ID, 500 Kbaud
	{ '8', "ISO 15765-4 (CAN 11/250)", OBDHEADER_CAN11 }, // 11 bit ID, 250 Kbaud
	{ '9', "ISO 15765-4 (CAN 29/250)", OBDHEADER_CAN29 }, // 29 bit ID, 250 Kbaud
	{ 'A', "SAE J1939 (CAN 29/250)", OBDHEADER_CAN29 }, // 29 bit ID, 250* Kbaud
	{ 'B', "USER1 CAN", OBDHEADER_CAN11 }, // 11* bit ID, 125* Kbaud
	{ 'C', "USER2 CAN", OBDHEADER_CAN11 }, // 11* bit ID, 50* Kbaud
};

/// Default protocol. Looked up early on - better match something from the above list
#define OBDSIM_DEFAULT_PROTOCOLNUM "8"

/// Print all protocol info to stdout
void printobdprotocols();

/// Given a protocol [A]{0-9}, set the protocol in the struct
/** \return 0 for success, <0 for failure
 */
int set_obdprotocol(const char *prot, struct simsettings *ss);

/// Given the single char, find the protocol for it
struct obdiiprotocol *find_obdprotocol(const char *protocol_num);


/// getopt() long options
static const struct option longopts[] = {
	{ "help", no_argument, NULL, 'h' }, ///< Print the help text
	{ "genhelp", required_argument, NULL, 'e' }, ///< Print the help for a generator
	{ "list-generators", no_argument, NULL, 'l' }, ///< Print a list of generators
	{ "version", no_argument, NULL, 'v' }, ///< Print the version text
	{ "seed", required_argument, NULL, 's' }, ///< Seed
	{ "benchmark", required_argument, NULL, 'n' }, ///< Benchmark seconds
	{ "generator", required_argument, NULL, 'g' }, ///< Choose a generator
	{ "customdelay", required_argument, NULL, 'd' }, ///< Custom delay for this ecu
	{ "logfile", required_argument, NULL, 'q' }, ///< Write to this logfile
	{ "elm-version", required_argument, NULL, 'V' }, ///< Pretend to be this on ATZ
	{ "elm-device", required_argument, NULL, 'D' }, ///< Pretend to be this on AT@1
	{ "protocol", required_argument, NULL, 'p' }, ///< Set the default protocol to this
	{ "list-protocols", no_argument, NULL, 'L' }, ///< List known protocols
#ifdef OBDPLATFORM_POSIX
	{ "launch-logger", no_argument, NULL, 'o' }, ///< Launch obdgpslogger
	{ "launch-screen", no_argument, NULL, 'c' }, ///< Launch screen
	{ "tty-device", required_argument, NULL, 't' }, ///< Open this actual device instead of a pty
#endif //OBDPLATFORM_POSIX
#ifdef OBDPLATFORM_WINDOWS
	{ "com-port", required_argument, NULL, 'w' }, ///< Windows com port to open
#endif //OBDPLATFORM_WINDOWS
#ifdef HAVE_BLUETOOTH
	{ "bluetooth", no_argument, NULL, 'b' }, ///< Listen with bluetooth
#endif //HAVE_BLUETOOTH
	{ NULL, 0, NULL, 0 } ///< End
};

/// getopt() short options
static const char shortopts[] = "hln:e:vs:g:q:V:D:p:Ld:"
#ifdef OBDPLATFORM_POSIX
	"oct:"
#endif //OBDPLATFORM_POSIX
#ifdef OBDPLATFORM_WINDOWS
	"w:"
#endif //OBDPLATFORM_WINDOWS
#ifdef HAVE_BLUETOOTH
	"b"
#endif //HAVE_BLUETOOTH
;


/// Print Help for --help
/** \param argv0 your program's argv[0]
 */
void printhelp(const char *argv0);

/// Print the version string
void printversion();


#endif //__OBDSIM_H


