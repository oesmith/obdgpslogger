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
 \brief main obdlogger entrypoint
 */

// Some references:
// mpg calculation: http://www.mp3car.com/vbulletin/engine-management-obd-ii-engine-diagnostics-etc/75138-calculating-mpg-vss-maf-obd2.html
// function list: http://www.kbmsystems.net/obd_tech.htm

#include "obdconfig.h"
#include "main.h"
#include "obdservicecommands.h"
#include "database.h"
#include "obddb.h"
#include "gpsdb.h"
#include "ecudb.h"
#include "tripdb.h"
#include "obdserial.h"
#include "gpscomm.h"
#include "supportedcommands.h"

#include "obdconfigfile.h"

#ifdef HAVE_GPSD
#include "gps.h"

/// IP address of gpsd
#define GPSD_ADDR "127.0.0.1"

/// Port of gpsd
#define GPSD_PORT "2947"
#endif //HAVE_GPSD

#ifdef HAVE_DBUS
#include "obddbus.h"
#endif //HAVE_DBUS

#include "sqlite3.h"

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif // HAVE_SIGNAL_H

/// Commit transactions once every this many seconds
#define TRANSACTIONTIME 8

/// Set when we catch a signal we want to exit on
static int receive_exitsignal = 0;

/// If we catch a signal to start the trip, set this
static int sig_starttrip = 0;

#ifdef OBDPLATFORM_POSIX
/// Daemonise. Returns 0 for success, or nonzero on failure.
static int obddaemonise();
#endif //OBDPLATFORM_POSIX

/// Set up signal handling
static void install_signalhandlers();

static void catch_quitsignal(int sig) {
	receive_exitsignal = 1;
}

static void catch_tripstartsignal(int sig) {
	sig_starttrip = 1;
}

int main(int argc, char** argv) {
	/// Serial port full path to open
	char *serialport = NULL;

	/// Database file to open
	char *databasename = NULL;

	/// List of columsn to log
	char *log_columns = NULL;

	/// Number of samples to take
	int samplecount = -1;

	/// Number of samples per second
	int samplespersecond = 1;

	/// Ask to show the capabilities of the OBD device then exit
	int showcapabilities = 0;

	/// Set if the user wishes to upgrade the baudrate
	long baudrate_upgrade = -1;

	/// Time between samples, measured in microseconds
	long frametime = 0;

	/// Spam all readings to stdout
	int spam_stdout = 0;

	/// Enable elm optimisations
	int enable_optimisations = 0;

	/// Enable serial logging
	int enable_seriallog = 0;

	/// Serial log filename
	char *seriallogname = NULL;

#ifdef OBDPLATFORM_POSIX
	/// Daemonise
	int daemonise = 0;
#endif //OBDPLATFORM_POSIX

	/// Requested baudrate
	long requested_baud = -1;

	// Config File
	struct OBDGPSConfig *obd_config = obd_loadConfig(0);

	if(NULL != obd_config) {
		samplespersecond = obd_config->samplerate;
		enable_optimisations = obd_config->optimisations;
		requested_baud = obd_config->baudrate;
		baudrate_upgrade = obd_config->baudrate_upgrade;
	}

	// Do not attempt to buffer stdout at all
	setvbuf(stdout, (char *)NULL, _IONBF, 0);

	int optc;
	int mustexit = 0;
	while ((optc = getopt_long (argc, argv, shortopts, longopts, NULL)) != -1) {
		switch (optc) {
			case 'h':
				printhelp(argv[0]);
				mustexit = 1;
				break;
			case 'v':
				printversion();
				mustexit = 1;
				break;
			case 's':
				if(NULL != serialport) {
					free(serialport);
				}
				serialport = strdup(optarg);
				break;
			case 'o':
				enable_optimisations = 1;
				break;
			case 't':
				spam_stdout = 1;
				break;
			case 'u':
				{
					int newout = open(optarg, O_CREAT|O_RDWR|O_APPEND,
						S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
					if(-1 == newout) {
						perror(optarg);
					} else {
						printf("Redirecting output to %s\n", optarg);
						close(STDOUT_FILENO);
						close(STDERR_FILENO);
						dup2(newout, STDOUT_FILENO);
						dup2(newout, STDERR_FILENO);
					}
				}
				break;
#ifdef OBDPLATFORM_POSIX
			case 'm':
				daemonise = 1;
				break;
#endif //OBDPLATFORM_POSIX
			case 'c':
				samplecount = atoi(optarg);
				break;
			case 'b':
				requested_baud = strtol(optarg, (char **)NULL, 10);
				break;
			case 'B':
				baudrate_upgrade = strtol(optarg, (char **)NULL, 10);
				break;
			case 'd':
				if(NULL != databasename) {
					free(databasename);
				}
				databasename = strdup(optarg);
				break;
			case 'i':
				if(NULL != log_columns) {
					free(log_columns);
				}
				log_columns = strdup(optarg);
				break;
			case 'a':
				samplespersecond = atoi(optarg);
				break;
			case 'l':
				enable_seriallog = 1;
				if(NULL != seriallogname) {
					free(seriallogname);
				}
				seriallogname = strdup(optarg);
				break;
			case 'p':
				showcapabilities = 1;
				break;
			default:
				mustexit = 1;
				break;
		}
	}

	if(mustexit) exit(0);

	if(0 >= samplespersecond) {
		frametime = 0;
	} else {
		frametime = 1000000 / samplespersecond;
	}

	if(NULL == serialport) {
		if(NULL != obd_config && NULL != obd_config->obd_device) {
			serialport = strdup(obd_config->obd_device);
		} else {
			serialport = strdup(OBD_DEFAULT_SERIALPORT);
		}
	}
	if(NULL == databasename) {
		if(NULL != obd_config && NULL != obd_config->log_file) {
			databasename = strdup(obd_config->log_file);
		} else {
			databasename = strdup(OBD_DEFAULT_DATABASE);
		}
	}
	if(NULL == log_columns) {
		if(NULL != obd_config && NULL != obd_config->log_columns) {
			log_columns = strdup(obd_config->log_columns);
		} else {
			log_columns = strdup(OBD_DEFAULT_COLUMNS);
		}
	}


	if(enable_seriallog && NULL != seriallogname) {
		startseriallog(seriallogname);
	}


	// Open the serial port.
	int obd_serial_port = openserial(serialport, requested_baud, baudrate_upgrade);

	if(-1 == obd_serial_port) {
		fprintf(stderr, "Couldn't open obd serial port. Attempting to continue.\n");
	} else {
		fprintf(stderr, "Successfully connected to serial port. Will log obd data\n");
	}

	// Just figure out our car's OBD port capabilities and print them
	if(showcapabilities) {
		printobdcapabilities(obd_serial_port);
		
		printf("\n");

/*
		unsigned int retvals[50];
		int vals_returned;
		getobderrorcodes(obd_serial_port,
        		retvals, sizeof(retvals)/sizeof(retvals[0]), &vals_returned);

		int q = 0;
		int c = retvals[0];
		for(q=1;q<1+2*c && q+1<vals_returned;q+=2) {
			printf("Error: %s\n", obderrconvert(retvals[q], retvals[q+1]));
		}

*/

		closeserial(obd_serial_port);
		exit(0);
	}


#ifdef HAVE_GPSD
	// Open the gps device
	struct gps_data_t *gpsdata;
	gpsdata = opengps(GPSD_ADDR, GPSD_PORT);

	if(NULL == gpsdata) {
		fprintf(stderr, "Couldn't open gps port on startup.\n");
	} else {
		fprintf(stderr, "Successfully connected to gpsd. Will log gps data\n");
	}

#endif //HAVE_GPSD

	if(-1 == obd_serial_port
#ifdef HAVE_GPSD
		&& NULL == gpsdata
#endif //HAVE_GPSD
	) {
		fprintf(stderr, "Couldn't find either gps or obd to log. Exiting.\n");
		exit(1);
	}

#ifdef HAVE_DBUS
	obdinitialisedbus();
#endif //HAVE_DBUS

	// sqlite database
	sqlite3 *db;

	// sqlite statement
	sqlite3_stmt *obdinsert;

	// number of columns in the insert
	int obdnumcols;

	// sqlite return status
	int rc;

	// Open the database and create the obd table
	if(NULL == (db = opendb(databasename))) {
		closeserial(obd_serial_port);
		exit(1);
	}

	// Disable sqlite's synchronous pragma.
	/* char *zErrMsg;
	rc = sqlite3_exec(db, "PRAGMA synchronous=OFF",
					NULL, NULL, &zErrMsg);
	if(rc != SQLITE_OK) {
		printf("SQLite error %i: %s\n", rc, zErrMsg);
		sqlite3_free(zErrMsg);
	} */

	// Wishlist of commands from config file
	struct obdservicecmd **wishlist_cmds = NULL;
	obd_configCmds(log_columns, &wishlist_cmds);

	void *obdcaps = getobdcapabilities(obd_serial_port,wishlist_cmds);

	obd_freeConfigCmds(wishlist_cmds);
	wishlist_cmds=NULL;

	createobdtable(db,obdcaps);

	// Create the insert statement. On success, we'll have the number of columns
	if(0 == (obdnumcols = createobdinsertstmt(db,&obdinsert, obdcaps)) || NULL == obdinsert) {
		closedb(db);
		closeserial(obd_serial_port);
		exit(1);
	}

	createtriptable(db);

	createecutable(db);

	// All of these have obdnumcols-1 since the last column is time
	int cmdlist[obdnumcols-1]; // Commands to send [index into obdcmds_mode1]

	int i,j;
	for(i=0,j=0; i<sizeof(obdcmds_mode1)/sizeof(obdcmds_mode1[0]); i++) {
		if(NULL != obdcmds_mode1[i].db_column) {
			if(isobdcapabilitysupported(obdcaps,i)) {
				cmdlist[j] = i;
				j++;
			}
		}
	}

	freeobdcapabilities(obdcaps);
	// We create the gps table even if gps is disabled, so that other
	//  SQL commands expecting the table to at least exist will work.

	// sqlite statement
	sqlite3_stmt *gpsinsert;

	// number of columns in the insert
	int gpsnumcols;

	creategpstable(db);

	if(0 == (gpsnumcols = creategpsinsertstmt(db, &gpsinsert) || NULL == gpsinsert)) {
		closedb(db);
		closeserial(obd_serial_port);
		exit(1);
	}

#ifdef OBDPLATFORM_POSIX
	if(daemonise) {
		if(0 != obddaemonise()) {
			fprintf(stderr,"Couldn't daemonise, exiting\n");
			closeserial(obd_serial_port);
			exit(1);
		}
	}
#endif //OBDPLATFORM_POSIX

#ifdef HAVE_GPSD
	// Ping a message to stdout the first time we get
	//   enough of a satellite lock to begin logging
	int have_gps_lock = 0;
#endif //HAVE_GPSD


	install_signalhandlers();


	// The current thing returned by starttrip
	sqlite3_int64 currenttrip = 0;

	// Set when we're actually inside a trip
	int ontrip = 0;

	// The current time we're inserting
	double time_insert;

	// The last time we tried to check the gps daemon
	double time_lastgpscheck = 0;

	// Number of samples per transaction
	const int basetransactioncount = TRANSACTIONTIME * (0==samplespersecond?10:samplespersecond);

	// Store a few seconds worth of samples per transaction
	int transactioncount = 0;

	obdbegintransaction(db);

	while(samplecount == -1 || samplecount-- > 0) {

		struct timeval starttime; // start time through loop
		struct timeval endtime; // end time through loop
		struct timeval selecttime; // =endtime-starttime [for select()]

		if(0 != gettimeofday(&starttime,NULL)) {
			perror("Couldn't gettimeofday");
			break;
		}

#ifdef HAVE_DBUS
		enum obd_dbus_message msg_ret;
		while(OBD_DBUS_NOMESSAGE != (msg_ret = obdhandledbusmessages())) {
			switch(msg_ret) {
				case OBD_DBUS_STARTTRIP:
					if(!ontrip) {
						currenttrip = starttrip(db, time_insert);
						fprintf(stderr,"Created a new trip (%i)\n", (int)currenttrip);
						ontrip = 1;
					}
					break;
				case OBD_DBUS_NOMESSAGE:
				default:
					break;
			}
		}
#endif //HAVE_DBUS

		time_insert = (double)starttime.tv_sec+(double)starttime.tv_usec/1000000.0f;

		if(sig_starttrip) {
			if(ontrip) {
				fprintf(stderr,"Ending current trip\n");
				updatetrip(db, currenttrip, time_insert);
				ontrip = 0;
			}
			currenttrip = starttrip(db, time_insert);
			fprintf(stderr,"Created a new trip (%i)\n", (int)currenttrip);
			ontrip = 1;
			sig_starttrip = 0;
		}

		enum obd_serial_status obdstatus;
		if(-1 < obd_serial_port) {

			// Get all the OBD data
			for(i=0; i<obdnumcols-1; i++) {
				float val;
				unsigned int cmdid = obdcmds_mode1[cmdlist[i]].cmdid;
				int numbytes = enable_optimisations?obdcmds_mode1[cmdlist[i]].bytes_returned:0;
				OBDConvFunc conv = obdcmds_mode1[cmdlist[i]].conv;

				obdstatus = getobdvalue(obd_serial_port, cmdid, &val, numbytes, conv);
				if(OBD_SUCCESS == obdstatus) {
#ifdef HAVE_DBUS
					obddbussignalpid(&obdcmds_mode1[cmdlist[i]], val);
#endif //HAVE_DBUS
					if(spam_stdout) {
						printf("%s=%f\n", obdcmds_mode1[cmdlist[i]].db_column, val);
					}
					sqlite3_bind_double(obdinsert, i+1, (double)val);
					// printf("cmd: %02X, val: %f\n",obdcmds_mode1[cmdlist[i]].cmdid,val);
				} else {
					break;
				}
			}

			if(obdstatus == OBD_SUCCESS) {
				// If they're not on a trip but the engine is going, start a trip
				if(0 == ontrip) {
					printf("Creating a new trip\n");
					currenttrip = starttrip(db, time_insert);
					ontrip = 1;
				}
				sqlite3_bind_double(obdinsert, i+1, time_insert);
				sqlite3_bind_int64(obdinsert, i+2, currenttrip);

				// Do the OBD insert
				rc = sqlite3_step(obdinsert);
				if(SQLITE_DONE != rc) {
					printf("sqlite3 obd insert failed(%i): %s\n", rc, sqlite3_errmsg(db));
				}
			} else if(OBD_ERROR == obdstatus) {
				fprintf(stderr, "Received OBD_ERROR from serial read. Exiting\n");
				receive_exitsignal = 1;
			} else {
				// If they're on a trip, and the engine has desisted, stop the trip
				if(ontrip) {
					printf("Ending current trip\n");
					updatetrip(db, currenttrip, time_insert);
					ontrip = 0;
				}
			}
			sqlite3_reset(obdinsert);
		}

		// Constantly update the trip
		updatetrip(db, currenttrip, time_insert);

#ifdef HAVE_GPSD
		// Get the GPS data
		double lat,lon,alt,speed,course,gpstime;

		int gpsstatus = -1;
		if(NULL != gpsdata) {
			gpsstatus = getgpsposition(gpsdata, &lat, &lon, &alt, &speed, &course, &gpstime);
		} else {
			if(time_insert - time_lastgpscheck > 10) { // Try again once in a while
				gpsdata = opengps(GPSD_ADDR, GPSD_PORT);
				if(NULL != gpsdata) {
					printf("Delayed connection to gps achieved\n");
				} else {
					// fprintf(stderr, "Delayed connection to gps failed\n");
				}
				time_lastgpscheck = time_insert;
			}
		}
		if(gpsstatus < 0 || NULL == gpsdata) {
			// Nothing yet
		} else if(gpsstatus >= 0) {
			if(0 == have_gps_lock) {
				fprintf(stderr,"GPS acquisition complete\n");
				have_gps_lock = 1;
			}

			sqlite3_bind_double(gpsinsert, 1, lat);
			sqlite3_bind_double(gpsinsert, 2, lon);
			if(gpsstatus >= 1) {
				sqlite3_bind_double(gpsinsert, 3, alt);
			} else {
				sqlite3_bind_null(gpsinsert, 3);
			}
			sqlite3_bind_double(gpsinsert, 4, speed);
			sqlite3_bind_double(gpsinsert, 5, course);
			sqlite3_bind_double(gpsinsert, 6, gpstime);

			if(spam_stdout) {
				printf("gpspos=%f,%f,%f,%f,%f\n",
					lat, lon, (gpsstatus>=1?alt:-1000.0), speed, course);
			}

			// Use time worked out before.
			//  This makes table joins reliable, but the time itself may be wrong depending on gpsd lagginess
			sqlite3_bind_double(gpsinsert, 7, time_insert);

			sqlite3_bind_int64(gpsinsert, 8, currenttrip);

			// Do the GPS insert
			rc = sqlite3_step(gpsinsert);
			if(SQLITE_DONE != rc) {
				printf("sqlite3 gps insert failed(%i): %s\n", rc, sqlite3_errmsg(db));
			}
			sqlite3_reset(gpsinsert);
		}
#endif //HAVE_GPSD

		if(0 != gettimeofday(&endtime,NULL)) {
			perror("Couldn't gettimeofday");
			break;
		}

		// Set via the signal handler
		if(receive_exitsignal) {
			break;
		}


		// Commit this if we've done more than a certain number
		transactioncount++;
		transactioncount%=basetransactioncount;
		if(0 == transactioncount) {
			obdcommittransaction(db);
			obdbegintransaction(db);
		}

		
		// usleep() not as portable as select()

		if(0 < frametime) {
			selecttime.tv_sec = endtime.tv_sec - starttime.tv_sec;
			if (selecttime.tv_sec != 0) {
					endtime.tv_usec += 1000000*selecttime.tv_sec;
					selecttime.tv_sec = 0;
			}
			selecttime.tv_usec = (frametime) - 
					(endtime.tv_usec - starttime.tv_usec);
			if(selecttime.tv_usec < 0) {
					selecttime.tv_usec = 1;
			}
			select(0,NULL,NULL,NULL,&selecttime);
		}
	}

	obdcommittransaction(db);

	if(0 != ontrip) {
		updatetrip(db, currenttrip, time_insert);
		ontrip = 0;
	}

	sqlite3_finalize(obdinsert);
	sqlite3_finalize(gpsinsert);

	closeserial(obd_serial_port);
#ifdef HAVE_GPSD
	if(NULL != gpsdata) {
		gps_close(gpsdata);
	}
#endif //HAVE_GPSD
	closedb(db);

	if(enable_seriallog) {
		closeseriallog();
	}

	if(NULL != log_columns) free(log_columns);
	if(NULL != databasename) free(databasename);
	if(NULL != serialport) free(serialport);

	obd_freeConfig(obd_config);
	return 0;
}

#ifdef OBDPLATFORM_POSIX
// *sniff sniff*
// Smells like Stevens.
static int obddaemonise() {
	int fd;
	pid_t pid = fork();

	switch (pid) {
		case -1:
			perror("Couldn't fork");
			return -1;
		case 0: // child
			break;
		default: // Parent
			exit(0);
	}

	if (setsid() == -1)
		return -1;

	if (chdir("/") == -1)
		return -1;

	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);
	if ((fd = open("/dev/null", O_RDWR, 0)) != -1) {
		dup2(fd, STDIN_FILENO);
		dup2(fd, STDOUT_FILENO);
		dup2(fd, STDERR_FILENO);
	}

	return 0;
}
#endif //OBDPLATFORM_POSIX

void printhelp(const char *argv0) {
	printf("Usage: %s [params]\n"
				"   [-s|--serial <" OBD_DEFAULT_SERIALPORT ">]\n"
				"   [-c|--count <infinite>]\n"
				"   [-i|--log-columns <" OBD_DEFAULT_COLUMNS ">]\n"
				"   [-t|--spam-stdout]\n"
				"   [-p|--capabilities]\n"
				"   [-o|--enable-optimisations]\n"
				"   [-u|--output-log <filename>]\n"
#ifdef OBDPLATFORM_POSIX
				"   [-m|--daemonise]\n"
#endif //OBDPLATFORM_POSIX
				"   [-b|--baud <number>]\n"
				"   [-B|--modifybaud <number>]\n"
				"   [-l|--serial-log <filename>]\n"
				"   [-a|--samplerate [1]]\n"
				"   [-d|--db <" OBD_DEFAULT_DATABASE ">]\n"
				"   [-v|--version] [-h|--help]\n", argv0);
}

void printversion() {
	printf("Version: %i.%i\n", OBDGPSLOGGER_MAJOR_VERSION, OBDGPSLOGGER_MINOR_VERSION);
}


void install_signalhandlers() {
	// Set up signal handling

#ifdef HAVE_SIGACTION
	struct sigaction sa_new;
	memset(&sa_new, 0, sizeof(sa_new));

	// Exit on ctrl+c or SIGTERM
	sa_new.sa_handler = catch_quitsignal;
	sigemptyset(&sa_new.sa_mask);
	sigaddset(&sa_new.sa_mask, SIGINT);
	sigaddset(&sa_new.sa_mask, SIGTERM);
	sigaction(SIGINT, &sa_new, NULL);
	sigaction(SIGTERM, &sa_new, NULL);

#ifdef SIGUSR1
	// Start a trip on USR1
	sa_new.sa_handler = catch_tripstartsignal;
	sa_new.sa_flags &= ~(SA_RESETHAND);
	sigemptyset(&sa_new.sa_mask);
	sigaddset(&sa_new.sa_mask, SIGUSR1);
	sigaction(SIGUSR1, &sa_new, NULL);
#endif //SIGUSR1

#else // HAVE_SIGACTION

// If your unix implementation doesn't have sigaction, we can fall
//  back to the older [bad, unsafe] signal().
#ifdef HAVE_SIGNAL_FUNC

	// Exit on ctrl+c or TERM
	signal(SIGINT, catch_quitsignal);
	signal(SIGTERM, catch_quitsignal);

#ifdef SIGUSR1
	// Start a trip on USR1
	signal(SIGUSR1, catch_tripstartsignal);
#endif //SIGUSR1

#endif // HAVE_SIGNAL_FUNC


#endif //HAVE_SIGACTION

}
