/* Copyright 2012 Olly Smith

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
  \brief Tools to open the sim port
*/
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
	  
#include "simport.h"
#include "obdsim.h"
#include "tcpsimport.h"

TCPSimPort::TCPSimPort(unsigned short listen_port) {
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(listen_port);
	addr.sin_addr.s_addr = 0L;
	
	readbuf_pos = 0;
	memset(readbuf, '\0', sizeof(readbuf));
	memset(lastread, '\0', sizeof(lastread));
	snprintf(portname, sizeof(portname), "0.0.0.0:%hu", listen_port);
	
	comm_fd = -1;
	listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(-1 == listen_fd) {
		perror("Unable to open listen socket");
		return;
	}
	if(-1 == bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr))) {
		perror("Unable to bind listen socket to port");
		return;
	}	
	if(-1 == listen(listen_fd, 1)) {
		perror("Unable to listen for new connections");
		return;
	}
	
	mUsable = 1;
}

TCPSimPort::~TCPSimPort() {
	if(-1 != comm_fd) {
		close(comm_fd);
	}
	if(-1 != listen_fd) {
		close(listen_fd);
	}
}

char* TCPSimPort::getPort() {
	return portname;
}

char* TCPSimPort::readLine() {
	fd_set read_fds;
	fd_set err_fds;
	
	struct timeval timeout;
	memset(&timeout, '\0', sizeof(timeout));

	if(-1 == comm_fd) {
		// try to accept() a new connection
		FD_ZERO(&read_fds);
		FD_SET(listen_fd, &read_fds);
		FD_ZERO(&err_fds);
		FD_SET(listen_fd, &err_fds);
		int waiting = select(listen_fd+1, &read_fds, NULL, &err_fds, &timeout);
		if(waiting == -1) {
			perror("select() on listen port failed");
			return NULL;
		}
		else if(waiting > 0) {
			struct sockaddr_in addr;
			socklen_t addr_len = sizeof(addr);
			comm_fd = accept(listen_fd, (struct sockaddr*)&addr, &addr_len);
			if(comm_fd == -1) {
				perror("accept() failed");
				return NULL;
			}
		}
	}

	if(-1 != comm_fd) {
		// try to read() some data
		FD_ZERO(&read_fds);
		FD_SET(comm_fd, &read_fds);
		FD_ZERO(&err_fds);
		FD_SET(comm_fd, &err_fds);
		int waiting = select(comm_fd+1, &read_fds, NULL, &err_fds, &timeout);
		if(waiting == -1) {
			perror("select() on comms port failed");
			return NULL;
		}
		else if(waiting > 0) {
			size_t n_read = read(comm_fd, readbuf+readbuf_pos, sizeof(readbuf)-readbuf_pos);
			if(n_read == -1) {
				perror("read() on comms port failed");
				close(comm_fd);
				comm_fd = -1;
				return NULL;
			}
			else if(n_read == 0) {
				// closed
				close(comm_fd);
				comm_fd = -1;
				return NULL;
			}
			else {
				readbuf_pos += n_read;
			}
		}
	}

	if(readbuf_pos > 0) {
		char* line_end = strnstr(readbuf, "\r", readbuf_pos);
		if(line_end == NULL) {
			line_end = strnstr(readbuf, "\n", readbuf_pos);
		}
		
		if(line_end != NULL) {
			int length = line_end - readbuf;
			strncpy(lastread, readbuf, length);
			lastread[length] = '\0';
			
			while(*line_end == '\r' || *line_end == '\n') {
				line_end++;
			}
			memmove(readbuf, line_end, sizeof(readbuf) - (line_end - readbuf));
			readbuf_pos -= (line_end - readbuf);

			return lastread;
		}
	}
	return NULL;
}

void TCPSimPort::writeData(const char *line, int log) {
	if(log) writeLog(line);
	if(comm_fd != -1) {
		write(comm_fd, line, strlen(line));
	}
}
