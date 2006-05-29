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
#include <errno.h>

// POSIX
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

// Own
#include "eins.h"
#include "util_ip.h"
#include "util.h"
#include "measure.h"

bool
ip_handle_arg(ip_prefs *p, char opt, char *arg)
{
    switch (opt) {
    case '6':
	p->v6 = true;
	return true;
	    
    case 'P':
	p->port = safe_strdup(arg);
	return true;
	    
    case 'H':
	p->hdr_size = atoi(arg);
	return true;
	
    default:
	return false;
    }
}

int
ip_connect(char *host, char *port, struct addrinfo *hints)
{
    struct addrinfo *ai, *tmp;

    int rc = getaddrinfo(host, port, hints, &ai);
    if (rc != 0) {
	L(gai_strerror(rc));
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
	LE("Client: connect"); 
	return 0;
    }

    freeaddrinfo(ai);

    return sd;
}

double
ip_measure(int sd, char *payload, size_t size, size_t tries, size_t hdr_size, struct iovec *hdr_vec, size_t frag_size)
{
    time_586 ta, tb;
    size_t bytes;
    char *rcv_buf = safe_alloc(size);
    
    if (__unlikely(hdr_size)) { // Use writev()/readv()
	get_time(ta);

	if (__unlikely(writev(sd, hdr_vec, 2) == -1)) {
	    LE("Client: writev"); 
	    exit(1);
	}
	
	if (__unlikely((bytes = readv(sd, hdr_vec, 2)) == -1)) {
	    LE("Client: readv"); 
	    exit(1);
	}

	get_time(tb);
#warning "Header code broken ATM."
	assert(bytes == (size + hdr_size));
    } else {
	get_time(ta);

	ssize_t rc;
	for (bytes = 0; bytes < size; bytes += rc) {
	    rc = send(sd, payload + bytes,
		      (bytes + frag_size) > size ? size - bytes : frag_size,
		      0);
	    if (rc == -1) {
		XLE("Client: send"); 
	    }
	}

	for (bytes = 0; bytes < size; bytes += rc) {
	    rc = recv(sd, rcv_buf + bytes, size - bytes, 0);
	    if(rc == -1) {
		XLE("Client: recv"); 
	    }
	}

	get_time(tb);
    }

    if (memcmp(payload, rcv_buf, size))
	XL("PANIC! Data received doesn't match data sent!");

    return time_diff(tb, ta);
}

bool
ip_handshake_client(int sd, handshake *h, size_t size)
{
    int response;

    if (send(sd, h, size, 0) == -1) {
	XLE("Client: send"); 
    }
    if (recv(sd, &response, sizeof(response), 0) == -1) {
	XLE("Client: recv"); 
    }

    return response;
}

bool
ip_handshake_server(int sd, handshake *eh, size_t size)
{
    int response;

    if (recv(sd, eh, size, 0) == -1) {
	LE("Server: recv");
	return 0;
    }

    if (eh->size > 0 && eh->tries > 0) {
	response = 1;
    } else {
	response = 0;
    }

    if(send(sd, &response, sizeof(response), 0) == -1) {
	LE("Server: send");
	return 0;
    }

    return response;
}
