/* 
 * eins - A tool for measuring network-bandwidths and -latencies.
 * Copyright (C) 2006  Hynek Schlawack <hs+eins@ox.cx>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

// ISO
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// POSIX
#include <netdb.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/types.h>
#include <unistd.h>

// Own
#include "eins.h"
#include "ip.h"
#include "util.h"
#include "measure.h"

int
ip_connect(char *host, char *port, struct addrinfo *hints)
{
    struct addrinfo *ai, *tmp;

    int rc = getaddrinfo(host, port, hints, &ai);
    if (rc != 0) {
	puts(gai_strerror(rc));
	exit(1);
    }
	
    int sd;
    for (tmp = ai; tmp; tmp = tmp->ai_next) {
	sd = socket(tmp->ai_family, tmp->ai_socktype, tmp->ai_protocol);
	if (connect(sd, tmp->ai_addr, tmp->ai_addrlen) == 0)
	    break;
	close(sd);
    }

    if (tmp == NULL) {
	perror("client, connect"); 
	return 0;
    }

    freeaddrinfo(ai);

    return sd;
}

double
ip_measure(int sd, char *payload, int size, int tries, int hdr_size, struct iovec *hdr_vec, ssize_t frag_size)
{
    time_586 ta, tb;
    size_t bytes;
    
    if (__unlikely(hdr_size)) { // Use writev()/readv()
	gamma_time(ta);

	if (__unlikely(writev(sd, hdr_vec, 2) == -1)) {
	    perror("client, writev"); 
	    exit(1);
	}
	
	if (__unlikely((bytes = readv(sd, hdr_vec, 2)) == -1)) {
	    perror("client, readv"); 
	    exit(1);
	}

	gamma_time(tb);
#warning "Header code broken ATM."
	assert(bytes == (size + hdr_size));
    } else {
	gamma_time(ta);

	ssize_t rc;
	for (bytes = 0; bytes < size; bytes += rc) {
	    rc = send(sd, payload + bytes,
		      (bytes + frag_size) > size ? size - bytes : frag_size,
		      0);
	    if (rc == -1) {
		perror("measure, send"); 
		exit(1);
	    }
	}

	for (bytes = 0; bytes < size; bytes += rc) {
	    rc = recv(sd, payload + bytes, size - bytes, 0);
	    if(rc == -1) {
		perror("measure, recv"); 
		exit(1);
	    }
	}

	gamma_time(tb);
    }

    return gamma_time_diff(tb, ta);
}

int
ip_handshake_client(int sd, struct e_handshake *eh)
{
    int response;

    if (send(sd, eh, sizeof(struct e_handshake) + 1, 0) == -1) {
	perror("handshake, send"); 
	exit(1);
    }
    if (recv(sd, &response, sizeof(response), 0) == -1) {
	perror("handshake, recv"); 
	exit(1);
    }

    return response;
}

int
ip_handshake_server(int sd, struct e_handshake *eh)
{
    int response;

    if (recv(sd, eh, sizeof(struct e_handshake), 0) == -1) {
	perror("server, recv");
	return 0;
    }
    if (eh->size > 0 && eh->tries > 0) {
	response = 1;
    } else {
	response = 0;
    }

    if(send(sd, &response, sizeof(response), 0) == -1) {
	perror("server, send");
	return 0;
    }

    return response;
}
