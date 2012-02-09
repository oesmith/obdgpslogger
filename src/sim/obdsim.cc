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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>

#include "obdconfig.h"
#include "obdservicecommands.h"

#include "obdsim.h"
#include "simport.h"
#include "datasource.h"
#include "mainloop.h"

#ifdef OBDPLATFORM_POSIX
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

#include "posixsimport.h"
#endif //OBDPLATFORM_POSIX

#ifdef OBDPLATFORM_WINDOWS
#include <windows.h>

#include "windowssimport.h"
#endif //OBDPLATFORM_WINDOWS

#ifdef HAVE_BLUETOOTH
#include "bluetoothsimport.h"
#endif //HAVE_BLUETOOTH


// Adding your plugin involves two edits here.
// First, add an extern like the others
// Second, add it to the available_generators immediately after

#ifdef OBDSIMGEN_RANDOM
extern struct obdsim_generator obdsimgen_random;
#endif //OBDSIMGEN_RANDOM
#ifdef OBDSIMGEN_CYCLE
extern struct obdsim_generator obdsimgen_cycle;
#endif //OBDSIMGEN_CYCLE
#ifdef OBDSIMGEN_LOGGER
extern struct obdsim_generator obdsimgen_logger;
#endif //OBDSIMGEN_LOGGER
#ifdef OBDSIMGEN_DBUS
extern struct obdsim_generator obdsimgen_dbus;
#endif //OBDSIMGEN_DBUS
#ifdef OBDSIMGEN_MEGASQUIRT
extern struct obdsim_generator obdsimgen_megasquirt;
#endif //OBDSIMGEN_MEGASQUIRT
#ifdef OBDSIMGEN_DLOPEN
extern struct obdsim_generator obdsimgen_dlopen;
#endif //OBDSIMGEN_DLOPEN
#ifdef OBDSIMGEN_GUI_FLTK
extern struct obdsim_generator obdsimgen_gui_fltk;
#endif //OBDSIMGEN_GUI_FLTK
#ifdef OBDSIMGEN_SOCKET
extern struct obdsim_generator obdsimgen_socket;
#endif //OBDSIMGEN_SOCKET
#ifdef OBDSIMGEN_ERROR
extern struct obdsim_generator obdsimgen_error;
#endif //OBDSIMGEN_ERROR

/// A list of all available generators in this build
static struct obdsim_generator *available_generators[] = {
#ifdef OBDSIMGEN_RANDOM
	&obdsimgen_random,
#endif //OBDSIMGEN_RANDOM
#ifdef OBDSIMGEN_CYCLE
	&obdsimgen_cycle,
#endif //OBDSIMGEN_CYCLE
#ifdef OBDSIMGEN_LOGGER
	&obdsimgen_logger,
#endif //OBDSIMGEN_LOGGER
#ifdef OBDSIMGEN_DBUS
	&obdsimgen_dbus,
#endif //OBDSIMGEN_DBUS
#ifdef OBDSIMGEN_MEGASQUIRT
	&obdsimgen_megasquirt,
#endif //OBDSIMGEN_MEGASQUIRT
#ifdef OBDSIMGEN_DLOPEN
	&obdsimgen_dlopen,
#endif //OBDSIMGEN_DLOPEN
#ifdef OBDSIMGEN_SOCKET
	&obdsimgen_socket,
#endif //OBDSIMGEN_SOCKET
#ifdef OBDSIMGEN_GUI_FLTK
	&obdsimgen_gui_fltk,
#endif //OBDSIMGEN_GUI_FLTK
#ifdef OBDSIMGEN_ERROR
	&obdsimgen_error
#endif //OBDSIMGEN_ERROR
};

/// Default sim generator for windows users is the gui
#if defined(OBDSIMGEN_GUI_FLTK) && defined(OBDPLATFORM_WINDOWS)
#undef DEFAULT_SIMGEN
#define DEFAULT_SIMGEN "gui_fltk"
#endif


/// Initialise the variables in an ECU
void obdsim_initialiseecu(struct obdgen_ecu *e) {
	e->simgen = NULL;
	e->ecu_num = 0;
	e->seed = NULL;
	e->lasterrorcount = 0;
	e->ffcount = 0;
	e->dg = 0;
	e->customdelay = 0;
	memset(e->ff, 0, sizeof(e->ff));
}

/// Initialse all variables in a simsettings
void obdsim_initialisesimsettings(struct simsettings *s) {
	s->e_autoprotocol = 1;
	set_obdprotocol(OBDSIM_DEFAULT_PROTOCOLNUM, s);

	s->benchmark = OBDSIM_BENCHMARKTIME;
	s->e_currentvoltage = OBDSIM_BATTERYV;

	s->device_identifier = strdup("ChunkyKs");
	s->elm_device = strdup(OBDSIM_ELM_DEVICE_STRING);
	s->elm_version = strdup(OBDSIM_ELM_VERSION_STRING);

	s->ecu_count = 0;
	int i;
	for(i = 0; i<OBDSIM_MAXECUS; i++) {
		obdsim_initialiseecu(&s->ecus[i]);
		s->ecudelays[i].ecu = NULL;
		s->ecudelays[i].delay = 0;
	}

	obdsim_elmreset(s);
}

/// Do an elm reset [ATZ or similar]
void obdsim_elmreset(struct simsettings *s) {
	s->e_headers = ELM_HEADERS;
	s->e_spaces = ELM_SPACES;
	s->e_echo = ELM_ECHO;
	s->e_linefeed = ELM_LINEFEED;
	s->e_timeout = ELM_TIMEOUT;
	s->e_adaptive = ELM_ADAPTIVETIMING;
	s->e_dlc = ELM_DISPLAYDLC;
}

/// Create a sorted list for the ECUs to respond in, based on their delays
void ecudelay_order(struct simsettings *ss) {
	int i, j;
	for(i=0;i<ss->ecu_count;i++) {
		ss->ecudelays[i].ecu = &ss->ecus[i];
		ss->ecudelays[i].delay = ss->ecus[i].customdelay;
	}

	/* OH THE HORROR. BUBBLESORT [on the plus side, you only do it
	     once, and you're only sorting six items]. Plus I haven't
	     implemented a bubble sort in years and kinda felt like doing it. */
	for(i=0;i<ss->ecu_count;i++) {
		for(j=i+1;j<ss->ecu_count;j++) {
			if(ss->ecudelays[i].delay > ss->ecudelays[j].delay) {
				struct obdgen_ecudelays tmp;
				memcpy(&tmp,&ss->ecudelays[i],sizeof(tmp));
				memcpy(&ss->ecudelays[i],&ss->ecudelays[j],sizeof(tmp));
				memcpy(&ss->ecudelays[j],&tmp,sizeof(tmp));
			}
		}
	}

	/* for(i=0;i<ss->ecu_count;i++) {
		printf("ecudelay %i, num %i, delay %i\n", i,
			ss->ecudelays[i].ecu->ecu_num, ss->ecudelays[i].delay);
	} */

	// Now go through and convert all the delays to delta since the last one
	for(i=ss->ecu_count-1;i>0;i--) {
		ss->ecudelays[i].delay -= ss->ecudelays[i-1].delay;
	}

	/* for(i=0;i<ss->ecu_count;i++) {
		printf("ecudelay delta %i, num %i, delay %i\n", i,
			ss->ecudelays[i].ecu->ecu_num, ss->ecudelays[i].delay);
	} */
}

#ifdef OBDPLATFORM_POSIX
/// Launch obdgpslogger connected to the pty
int spawnlogger(char *ptyname);

/// Launch screen connected to the pty
int spawnscreen(char *ptyname);
#endif // OBDPLATFORM_POSIX

/// Find the generator of the given name
static struct obdsim_generator *find_generator(const char *gen_name);

/// Print the long description provided by the generator
void show_genhelp(struct obdsim_generator *gen);

/// Print the genrators this was linked with
void printgenerator(int verbose);

int main(int argc, char **argv) {
	// The "seed" passed in. Generator-specific
	char *seedstr = NULL;

#ifdef OBDPLATFORM_POSIX
	// Whether to launch obdgpslogger attached to this sim
	int launch_logger = 0;

	// Whether to launch screen attached to this sim
	int launch_screen = 0;

	// If you should open a real device instead of a pty
	char *tty_device = NULL;
#endif //OBDPLATFORM_POSIX

#ifdef HAVE_BLUETOOTH
	// Set if they wanted a bluetooth connection
	int bluetooth_requested = 0;
#endif //HAVE_BLUETOOTH

	// Store all settings in here
	struct simsettings ss;
	obdsim_initialisesimsettings(&ss);

	// Logfilen name
	char *logfile_name = NULL;
	
#ifdef OBDPLATFORM_WINDOWS
	// Windows port to open
	char *winport = NULL;
#endif //OBDPLATFORM_WINDOWS
	
	// Iterator/index into ecus
	int current_ecu = 0;

	int optc;
	int mustexit = 0;
	while ((optc = getopt_long (argc, argv, shortopts, longopts, NULL)) != -1) {
		switch (optc) {
			case 'h':
				printhelp(argv[0]);
				printgenerator(1);
				mustexit = 1;
				break;
			case 'e': {
				struct obdsim_generator *h_gen = find_generator(optarg);
				show_genhelp(h_gen);
				mustexit = 1;
				break;
			}
			case 'L':
				printobdprotocols();
				mustexit = 1;
				break;
			case 'p':
				if(0 != set_obdprotocol(optarg, &ss)) {
					printf("Couldn't find protocol \"%s\", exiting\n", optarg);
					mustexit = 1;
				}
				break;
			case 'l':
				printgenerator(0);
				mustexit = 1;
				break;
			case 'n':
				ss.benchmark = atoi(optarg);
				break;
			case 'V':
				if(NULL != ss.elm_version) {
					free(ss.elm_version);
				}
				ss.elm_version = strdup(optarg);
				break;
			case 'D':
				if(NULL != ss.elm_device) {
					free(ss.elm_device);
				}
				ss.elm_device = strdup(optarg);
				break;
			case 'v':
				printversion();
				mustexit = 1;
				break;
			case 'g':
				if(OBDSIM_MAXECUS <= current_ecu) {
					fprintf(stderr, "Only support %i ECUs in this build\n", OBDSIM_MAXECUS);
					mustexit = 1;
				} else {
					ss.ecus[current_ecu].simgen = find_generator(optarg);
					ss.ecus[current_ecu].ecu_num = current_ecu;
					if(NULL == ss.ecus[current_ecu].simgen) {
						fprintf(stderr, "Couldn't find generator \"%s\"\n", optarg);
						mustexit = 1;
					}
					current_ecu++;
				}
				break;
			case 's':
				if(current_ecu == 0) {
					fprintf(stderr, "The seed must come after the generator\n");
					mustexit=1;
				}
				if(NULL != ss.ecus[current_ecu-1].seed) {
					fprintf(stderr, "Already provided a seed for generator %i\n", current_ecu);
					mustexit=1;
				}
				ss.ecus[current_ecu-1].seed = strdup(optarg);
				break;
			case 'd':
				if(current_ecu == 0) {
					fprintf(stderr, "The custom delay must come after the generator\n");
					mustexit=1;
				}
				if(0 != ss.ecus[current_ecu-1].customdelay) {
					fprintf(stderr, "Already provided a delay for generator %i\n", current_ecu);
					mustexit=1;
				}
				ss.ecus[current_ecu-1].customdelay = atoi(optarg);
				break;
			case 'q':
				if(NULL != logfile_name) {
					fprintf(stderr, "Warning! Multiple logs specified. Only last one will be used\n");
					free(seedstr);
				}
				logfile_name = strdup(optarg);
				break;
#ifdef HAVE_BLUETOOTH
			case 'b':
				bluetooth_requested = 1;
				break;
#endif //HAVE_BLUETOOTH
#ifdef OBDPLATFORM_POSIX
			case 'o':
				launch_logger = 1;
				break;
			case 'c':
				launch_screen = 1;
				break;
			case 't':
				if(NULL != tty_device) {
					free(tty_device);
				}
				tty_device = strdup(optarg);
				break;
#endif //OBDPLATFORM_POSIX
#ifdef OBDPLATFORM_WINDOWS
			case 'w':
				if(NULL != winport) {
					fprintf(stderr, "Warning! Multiple com port specified. Only last one will be used\n");
					free(winport);
				}
				winport = strdup(optarg);
				break;
#endif //OBDPLATFORM_WINDOWS
			default:
				mustexit = 1;
				break;
		}
	}

	ss.ecu_count = current_ecu;

	if(NULL == ss.e_protocol) {
		fprintf(stderr, "Couldn't find initial protocol %s\n", OBDSIM_DEFAULT_PROTOCOLNUM);
		return 1;
	}

#ifdef OBDPLATFORM_POSIX
	if(launch_logger && launch_screen) {
		fprintf(stderr, "Error: Cannot attach both screen and logger to same sim session\n");
		mustexit = 1;
	}
#endif // OBDPLATFORM_POSIX

	if(0 == ss.ecu_count) {
		ss.ecus[0].ecu_num = 0;
		ss.ecus[0].simgen = find_generator(DEFAULT_SIMGEN);
		if(NULL == ss.ecus[0].simgen) {
			fprintf(stderr, "Couldn't find default generator \"%s\"\n", DEFAULT_SIMGEN);
			mustexit = 1;
		}
		ss.ecu_count++;
	}

	if(mustexit) return 0;

	
	int initialisation_errors = 0;
	int i;
	for(i=0;i<ss.ecu_count;i++) {
		if(0 != ss.ecus[i].simgen->create(&ss.ecus[i].dg, ss.ecus[i].seed)) {
			fprintf(stderr,"Couldn't initialise generator \"%s\" using seed \"%s\"\n",
							ss.ecus[i].simgen->name(), ss.ecus[i].seed);
			initialisation_errors++;
		}
	}
	if(initialisation_errors > 0) {
		return 1;
	}

	/* Getting here means all the ECUs are up and running. Now alter
	    the order they're queried in based on customdelay */
	ecudelay_order(&ss);

	// The sim port
	OBDSimPort *sp = NULL;

#ifdef HAVE_BLUETOOTH
	if(bluetooth_requested) {
		sp = new BluetoothSimPort();
	} else {
#endif //HAVE_BLUETOOTH

#ifdef OBDPLATFORM_POSIX
		sp = new PosixSimPort(tty_device);
#endif //OBDPLATFORM_POSIX

#ifdef OBDPLATFORM_WINDOWS
		if(NULL == winport) {
			winport = strdup(DEFAULT_WINPORT);
		}
		sp = new WindowsSimPort(winport);
#endif //OBDPLATFORM_WINDOWS

#ifdef HAVE_BLUETOOTH
	}
#endif //HAVE_BLUETOOTH

	if(NULL == sp || !sp->isUsable()) {
		fprintf(stderr,"Error creating virtual port\n");
		return 1;
	}

	if(NULL != logfile_name) {
		sp->startLog(logfile_name);
	}

	char *slave_name = sp->getPort();
	if(NULL == slave_name) {
		printf("Couldn't get slave name for pty\n");
		delete sp;
		return 1;
	}

	printf("SimPort name: %s\n", slave_name);

#ifdef OBDPLATFORM_POSIX
	if(launch_logger) {
		spawnlogger(slave_name);
	}
	if(launch_screen) {
		spawnscreen(slave_name);
	}
#endif //OBDPLATFORM_POSIX

	printf("Successfully initialised obdsim, entering main loop\n");
	main_loop(sp, &ss);

	for(i=0;i<ss.ecu_count;i++) {
		ss.ecus[i].simgen->destroy(ss.ecus[i].dg);
	}

	delete sp;

	if(NULL != logfile_name) {
		free(logfile_name);
	}

	if(NULL != seedstr) {
		free(seedstr);
	}

	if(NULL != ss.elm_version) {
		free(ss.elm_version);
	}

	if(NULL != ss.elm_device) {
		free(ss.elm_device);
	}

#ifdef OBDPLATFORM_WINDOWS
	if(NULL != winport) {
		free(winport);
	}
#endif //OBDPLATFORM_WINDOWS

#ifdef OBDPLATFORM_POSIX
	if(NULL != tty_device) {
		free(tty_device);
	}
#endif //OBDPLATFORM_POSIX

	return 0;
}

#ifdef OBDPLATFORM_POSIX
int spawnlogger(char *ptyname) {
	int pid = fork();

	if(-1 == pid) {
		perror("Couldn't fork");
		return 1;
	}

	if(0 < pid) {
		return 0;
	}

	// In child
	execlp("obdgpslogger", "obdgpslogger",
		"--baud", "-1",                      // Don't modify baudrate
		"--modifybaud", "-1",                // Don't upgrade baudrate
		"--serial", ptyname,                 // Connect to this pty
		"--db", "./obdgpsloggertmp.db",      // Dump to this database
		"--serial-log", "./serialcomms.txt", // Log serial comms to this file
		"--spam-stdout",                     // Spam stdout
		"--enable-optimisations",            // Enable elm optimisations
		NULL);
	perror("Couldn't exec obdgpslogger");
	exit(0);
}

int spawnscreen(char *ptyname) {
	int pid = fork();

	if(-1 == pid) {
		perror("Couldn't fork");
		return 1;
	}

	if(0 < pid) {
		// To avoid seeing console spam in screen, dump stdout
		int fd;
		close(STDOUT_FILENO);
		if ((fd = open("/dev/null", O_RDWR, 0)) != -1) {
			dup2(fd, STDOUT_FILENO);
		}

		return 0;
	}

	// In child
	execlp("screen", "screen",
		ptyname,                 // Connect to this pty
		NULL);
	perror("Couldn't exec screen");
	exit(0);
}
#endif //OBDPLATFORM_POSIX

void show_genhelp(struct obdsim_generator *gen) {
	if(NULL == gen) {
		fprintf(stderr, "Cannot print help for nonexistent generator\n");
		return;
	}

	printf("Long help for generator \"%s\":\n\n%s\n\n", gen->name(),
		NULL==gen->longdesc?
			"Generator doesn't offer long description":
			gen->longdesc());
}

static struct obdsim_generator *find_generator(const char *gen_name) {
	size_t i;
	if(NULL == gen_name) return NULL;

	for(i=0; i<sizeof(available_generators)/sizeof(available_generators[0]); i++) {
		if(0 == strcmp(gen_name, available_generators[i]->name())) {
			return available_generators[i];
		}
	}
	return NULL;
}

int set_obdprotocol(const char *prot, struct simsettings *ss) {
	if(NULL == prot) {
		return -1;
	}

	// Allow spaces in various places even though behaviour isn't very well defined
	const char *p = prot;
	while(' ' == *p && '\0' != *p) p++;
	if('\0' == *p) {
		return -1;
	}

	int e_autoprotocol = 0;
	if('A' == *p || 'a' == *p) {
		e_autoprotocol = 1;
		p++;
	}
	while(' ' == *p && '\0' != *p) p++;

	struct obdiiprotocol *e_protocol = find_obdprotocol(p);
	p++;
	if(NULL == e_protocol) {
		return -1;
	}
	while(' ' == *p && '\0' != *p) p++;

	if('A' == *p || 'a' == *p) {
		e_autoprotocol = 1;
	}

	ss->e_protocol = e_protocol;
	ss->e_autoprotocol = e_autoprotocol;

	return 0;
}

struct obdiiprotocol *find_obdprotocol(const char *protocol_num) {
	int i;
	for(i=0; i<sizeof(obdprotocols)/sizeof(obdprotocols[0]); i++) {
		if(*protocol_num == obdprotocols[i].protocol_num) {
			return &(obdprotocols[i]);
		}
	}
	return NULL;
}


void printhelp(const char *argv0) {
	printf("Usage: %s [params]\n"
		"   [-g|--generator=<name of generator>\n"
		"       [-s|--seed=<generator-seed>]\n"
		"       [-d|--customdelay=<ecu delay(ms)>]\n"
		"     ]\n"
		"   [-q|--logfile=<logfilename to write to>]\n"
		"   [-V|--elm-version=<pretend to be this on ATZ>]\n"
		"   [-D|--elm-device=<pretend to be this on AT@1>]\n"
		"   [-L|--list-protocols]\n"
		"   [-p|--protocol=<OBDII protocol>]\n"
#ifdef OBDPLATFORM_POSIX
		"   [-o|--launch-logger]\n"
		"   [-c|--launch-screen] [\"EXIT\" or C-a,k to exit]\n"
		"   [-t|--tty-device=<real /dev/ entry to open>]\n"
#endif //OBDPLATFORM_POSIX
#ifdef OBDPLATFORM_WINDOWS
		"   [-w|--com-port=<windows COM port>]\n"
#endif //OBDPLATFORM_WINDOWS
#ifdef HAVE_BLUETOOTH
		"   [-b|--bluetooth]\n"
#endif //HAVE_BLUETOOTH
		"   [-e|--genhelp=<name of generator>]\n"
		"   [-l|--list-generators]\n"
		"   [-n|--benchmark=<seconds>]\n"
		"   [-v|--version] [-h|--help]\n", argv0);
}

void printversion() {
	printf("Version: %i.%i\n", OBDGPSLOGGER_MAJOR_VERSION, OBDGPSLOGGER_MINOR_VERSION);
}

void printobdprotocols() {
	int i;
	for(i=0; i<sizeof(obdprotocols)/sizeof(obdprotocols[0]); i++) {
		printf("%c  %s\n", obdprotocols[i].protocol_num,  obdprotocols[i].protocol_desc);
	}
}

void printgenerator(int verbose) {
	if(verbose) {
		printf("The generators built into this sim:\n");
	}

	// If we find the one currently #defined as default
	int found_default = 0;

	size_t i;
	for(i=0; i<sizeof(available_generators)/sizeof(available_generators[0]); i++) {
		int is_default = !strcmp(DEFAULT_SIMGEN, available_generators[i]->name());
		if(is_default) found_default = 1;
		if(verbose) {
			printf(" \"%s\"%s\n", 
				available_generators[i]->name(),
				is_default?" (default)":"");
		} else {
			printf("%s%s\n", 
				is_default?"* ":"  ",
				available_generators[i]->name());
		}
	}


	if(0 == found_default && verbose) {
		printf("No default generator\n");
	}
}


