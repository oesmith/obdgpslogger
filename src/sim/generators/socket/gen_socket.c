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
 \brief Generate socket data
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "datasource.h"

struct socket_gen {
	int handle; // Actual handle
};

// Do a command
/** \param cmd the command to send
   \param ret shove the returned stuff in this
   \param n allocated size of ret
   \return number of bytes copied into ret, or -1 on failure
*/
static int gen_dosockcmd(struct socket_gen *g, const char *cmd, char *ret, int n);

const char *socket_simgen_name() {
	return "Socket";
}

const char *socket_simgen_longdesc() {
	return "Connect to a wifi/network OBDII device\n"
		"Seed: <host:port>";
}

int socket_simgen_create(void **gen, const char *seed) {
	if(NULL == seed || '\0' == *seed) {
		fprintf(stderr, "Must pass \"ip-or-hostname:port\" as the seed\n");
		return 1;
	}

	struct socket_gen *g = malloc(sizeof(struct socket_gen));
	if(NULL == g) {
		fprintf(stderr, "Couldn't allocate memory for socket generator\n");
		return 1;
	}

	// Parse the seed
	char *seedcopy = strdup(seed); // Because strtok needs non-const.

	char *node;
	node = strtok(seedcopy, ": "); // Allow them to separate with a space
	char *service;
	service = strtok(NULL, ": ");


	// Actually get the address for connecting
	struct addrinfo hints;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = (AI_V4MAPPED | AI_ADDRCONFIG);
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;

	struct addrinfo *res; // From getaddrinfo

	if(0 != getaddrinfo(node, service, &hints, &res)) {
		perror("getaddrinfo error");
		free(seedcopy);
		free(g);
		return 1;
	}

	// Done with those for now. node and service are now invalid
	free(seedcopy);

	if(NULL == res) {
		fprintf(stderr, "getaddrinfo didn't return anything for \"%s\"\n", seed);
		free(g);
		return 1;
	}

	// Create the socket
	if(0 > (g->handle = socket(res->ai_family, res->ai_socktype, res->ai_protocol))) {
		perror("Couldn't create socket");
		freeaddrinfo(res);
		free(g);
		return 1;
	}

	// Attempt to connect
	if(0 > connect(g->handle, res->ai_addr, res->ai_addrlen)) {
		perror("Couldn't connect to host");
		freeaddrinfo(res);
		free(g);
		return 1;
	}

	// Don't need this anymore
	freeaddrinfo(res);

	// Should be connected if we're here. Do some commands

	char atretvalue[1024];

	if(-1 == gen_dosockcmd(g, "ATZ\n", atretvalue, sizeof(atretvalue))) {
		close(g->handle);
		free(g);
		return -1;
	}

	if(-1 == gen_dosockcmd(g, "ATE0\n", atretvalue, sizeof(atretvalue))) {
		close(g->handle);
		free(g);
		return -1;
	}

	if(-1 == gen_dosockcmd(g, "ATS0\n", atretvalue, sizeof(atretvalue))) {
		close(g->handle);
		free(g);
		return -1;
	}

	*gen = g;
	return 0;
}

void socket_simgen_destroy(void *gen) {
	struct socket_gen *g = gen;
	if(-1 == close(g->handle)) {
		perror("Error closing socket");
	}
	free(gen);
}

int socket_simgen_getvalue(void *gen, unsigned int mode, unsigned int PID, unsigned int *A, unsigned int *B, unsigned int *C, unsigned int *D) {
	char cmd[32];
	snprintf(cmd, sizeof(cmd), "%02X %02X\n", mode, PID);

	char retbuf[1024];
	if(-1 == gen_dosockcmd((struct socket_gen *)gen, cmd, retbuf, sizeof(retbuf))) {
		return -1;
	}

	unsigned int response;
	unsigned int retmode;
	int count = sscanf(retbuf, "%2x %2x %2x %2x %2x %2x", &response, &retmode,
			A,B,C,D);

	return (count<2?0:count-2);
}

int socket_simgen_idle(void *gen, int idlems) {
	return 0;
}

// Declare our obdsim_generator. This is pulled in as an extern in obdsim.c
struct obdsim_generator obdsimgen_socket = {
	socket_simgen_name,
	socket_simgen_longdesc,
	socket_simgen_create,
	socket_simgen_destroy,
	socket_simgen_getvalue,
	socket_simgen_idle,
	NULL,
	NULL
};

static int gen_dosockcmd(struct socket_gen *g, const char *cmd, char *ret, int n) {
	printf("Sending \"%s\" to network socket\n", cmd);

	if(write(g->handle, cmd, strlen(cmd)) != strlen(cmd)) {
		perror("Couldn't write to network socket");
		return -1;
	}

	memset(ret, '\0', n);

	int done = 0;
	int totalread = 0;
	char *curr_ret = ret;
	while(!done && totalread < n) {
		int nread = read(g->handle, curr_ret, n-totalread);
		if(-1 == nread) {
			perror("Network read error");
			return -1;
		}
		totalread += nread;
		if(NULL != strstr(ret, ">")) {
			done = 1;
		}
	}

	printf("Socket returned: \"%s\"\n", ret);

	return totalread;
}

