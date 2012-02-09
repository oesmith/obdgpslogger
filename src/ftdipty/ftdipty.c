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
 \brief Provide a pty to access an ftdi device, in case of missing kernel driver

gcc -o ftdipty ftdipty.c -lftdi
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <paths.h>
#include <unistd.h>
#include <ftdi.h>

#include "obdconfig.h"
#include "obdconfigfile.h"
#include "ftdipty.h"

static int obddaemonise();

int main(int argc, char **argv) {
	int baudrate = -1;
	int mustexit = 0;
	int modifyconf = 0;
	int created_configfile = 0;
	int daemonise = 0;

	int world_accessible = 0;

	int usr_vendorid = -1;
	int usr_deviceid = -1;

	int optc;
	while ((optc = getopt_long (argc, argv, shortopts, longopts, NULL)) != -1) {
		switch(optc) {
			case 'h':
				printhelp(argv[0]);
				mustexit = 1;
				break;
			case 'v':
				printversion();
				mustexit = 1;
				break;
			case 'd':
				daemonise = 1;
				break;
			case 'c':
				modifyconf = 1;
				break;
			case 'w':
				world_accessible = 1;
				break;
			case 'b':
				baudrate = atoi(optarg);
				break;
			case 'D':
				sscanf(optarg, "%X", &usr_deviceid);
				break;
			case 'V':
				sscanf(optarg, "%X", &usr_vendorid);
				break;
		}
	}

	if(mustexit) {
		exit(0);
	}

	int ret;

	// Create an ftdi context
	struct ftdi_context *ftdic;
	if (NULL == (ftdic = ftdi_new())) {
		fprintf(stderr, "ftdi_new failed\n");
		return 1;
	}

	int vendorid = 0x0403;

	if(usr_vendorid > 0) {
		vendorid = usr_vendorid;
	}

	int possibleproducts[] = {
		0x6001, //<FT232
		0x6010, //<FT2232
		0x6006  //<FT
	};
	
	int i;
	int product;
	int found_dev = 0;
	if(usr_deviceid > 0) {
		if (0 == (ret = ftdi_usb_open(ftdic, vendorid, usr_deviceid))) {
			printf("Found ftdi device with productid 0x%X\n", product);
			found_dev = 1;
		} else {
			fprintf(stderr, "Unable to open ftdi device %04X:%04X: %d (%s)\n", vendorid, usr_deviceid, ret, ftdi_get_error_string(ftdic));
		}
	} else {
		for(i=0;i<sizeof(possibleproducts)/sizeof(possibleproducts[0]); i++) {
			// Open the ftdi device
			product = possibleproducts[i];

			if (0 == (ret = ftdi_usb_open(ftdic, vendorid, product))) {
				printf("Found ftdi device with productid 0x%X\n", product);
				found_dev = 1;
				break;
			} else {
				// fprintf(stderr, "Unable to open ftdi device %04X:%04X: %d (%s)\n", vendorid, product, ret, ftdi_get_error_string(ftdic));
				// ftdi_free(ftdic);
				// return 1;
			}
		}
	}

	if(!found_dev) {
		fprintf(stderr, "Couldn't find any FTDI devices attached to system\n");
		ftdi_free(ftdic);
		return 1;
	}

	if(baudrate > -1) {
		if(0 > (ret = ftdi_set_baudrate(ftdic, baudrate))) {
			fprintf(stderr, "unable to open ftdi device: %d (%s)\n", ret, ftdi_get_error_string(ftdic));
			ftdi_free(ftdic);
			return 1;
		}
	}

	// Open the pseudoterminal
	int fd = posix_openpt(O_RDWR | O_NOCTTY);
	if(-1 == fd) {
		perror("Couldn't posix_openpt");
		return 1;
	}
	grantpt(fd);
	unlockpt(fd);
	fcntl(fd,F_SETFL,O_NONBLOCK);

	// Print the pty slave name
	static char ptyname[1024];
#ifdef HAVE_PTSNAME_R
	if(0 != ptsname_r(fd, ptyname, sizeof(ptyname))) {
		perror("Couldn't get pty slave name");
		return 1;
	}
#else
	char *ptsname_val = ptsname(fd);
	if(NULL == ptsname_val) {
		perror("Couldn't get pty slave name");
		return 1;
	}
	strncpy(ptyname, ptsname_val, sizeof(ptyname));
#endif //HAVE_PTSNAME_R

	printf("%s successfully opened pty. Name: %s\n", argv[0], ptyname);

	if(world_accessible) {
		if(-1 == chmod(ptyname, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)) {
			perror("Couldn't set ugo+rw on device");
		}
	}

	if(modifyconf) {
		FILE *f;
		if(NULL != (f = fopen(OBD_FTDIPTY_DEVICE, "w"))) {
			fprintf(f, "obddevice=%s\n", ptyname);
			fclose(f);
			created_configfile = 1;
		}
	}

	if(daemonise) {
		if(0 != obddaemonise()) {
			fprintf(stderr,"Couldn't daemonise, exiting\n");
			exit(1);
		}
	}

	// Seriously, how cheesy is this.
	while(1) {
		char buf[4096];
		int nbytes;

		// printf("About to read from the pty\n");
		if(0 < (nbytes = read(fd, buf, sizeof(buf)))) {
			// printf("About to write to the ftdi\n");
			if(0 > ftdi_write_data(ftdic, buf, nbytes)) {
				fprintf(stderr, "Error writing to ftdi: %s\n", ftdi_get_error_string(ftdic));
				break;
			}
			// write(STDOUT_FILENO, buf, nbytes);
		}

		// printf("About to usleep\n");
		// Just to stop it using 100% cpu
		usleep(10000);

		// printf("About to read from the ftdi\n");
		nbytes = ftdi_read_data(ftdic, buf, sizeof(buf));
		if(0 < nbytes) {
			// printf("About to write to the pty\n");
			write(fd, buf, nbytes);
			// write(STDOUT_FILENO, buf, nbytes);
		} else if(0 > nbytes) {
			fprintf(stderr, "Error reading from ftdi: %s\n", ftdi_get_error_string(ftdic));
			break;
		}
	}

	// Delete the file we created
	if(created_configfile) {
		unlink(OBD_FTDIPTY_DEVICE);
	}

	// Close the pty
	close(fd);

	// Close the ftdi device
	if (0 > (ret = ftdi_usb_close(ftdic))) {
		fprintf(stderr, "unable to close ftdi device: %d (%s)\n", ret, ftdi_get_error_string(ftdic));
		return EXIT_FAILURE;
	}

	// Free the ftdi context
	ftdi_free(ftdic);

	return 0;
}

// *sniff sniff*
// Smells like Stevens.
int obddaemonise() {
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

	if (setsid() == -1) {
		fprintf(stderr, "Couldn't setsid()\n");
		return -1;
	}

	if (chdir("/") == -1) {
		fprintf(stderr, "Couldn't chdir(/)\n");
		return -1;
	}

	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);
	if ((fd = open(_PATH_DEVNULL, O_RDWR, 0)) != -1) {
		dup2(fd, STDIN_FILENO);
		dup2(fd, STDOUT_FILENO);
		dup2(fd, STDERR_FILENO);
	}

	return 0;
}

void printhelp(const char *argv0) {
	printf("Usage: %s [params]\n"
		"   [-c|--modifyconf]\n"
		"   [-d|--daemonise]\n"
		"   [-w|--world-accessible]\n"
		"   [-b|--baud <number>]\n"
		"   [-V|--vendorid <hex vendor id>]\n"
		"   [-D|--deviceid <hex device id>]\n"
		"   [-v|--version] [-h|--help]\n", argv0);
}

void printversion() {
        printf("Version: %i.%i\n", OBDGPSLOGGER_MAJOR_VERSION, OBDGPSLOGGER_MINOR_VERSION);
}

