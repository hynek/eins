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
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// POSIX
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// Own
#include "eins.h"
#include "util_ip.h"
#include "mod_udp.h"
#include "util.h"

static struct udp_prefs {
    ip_prefs ip;

    size_t frag_size;
} Prefs = { { false, IP_DEF_PORT, 0 }, 0 };

typedef struct {
    handshake h;
    ip_handshake ip;
    
    size_t frag_size;
} udp_handshake;


// Source-scope-vars for the client
static char *Payload;
static int Sock;
static udp_handshake Handshake;
static char *hdr_Buffer;
static struct iovec hdr_Vec[2];

bool
udp_handle_arg(char opt, char *arg)
{
    switch (opt) {
    case 'F':
	Prefs.frag_size = atoi(arg);
	return true;
	
    default:       
	return ip_handle_arg((ip_prefs *) &Prefs, opt, arg);
    }
}


double
udp_measure()
{
    return ip_measure(Sock, Payload, Handshake.h.size, Handshake.h.tries, Prefs.ip.hdr_size, hdr_Vec, Handshake.frag_size);
}

void
udp_cleanup()
{
    close(Sock);
    free(hdr_Buffer);
}

bool
udp_init(mod_args *ma)
{
    // Set prefs
    Payload = ma->payload;
    Handshake.h.size = ma->size;
    Handshake.h.tries = ma->tries;
    Handshake.frag_size = Prefs.frag_size ? Prefs.frag_size : ma->size;

    // If needed, set up buffer for the header + vector.
    if (Prefs.ip.hdr_size) {
	hdr_Buffer = safe_alloc(Prefs.ip.hdr_size);

	randomize_buffer(hdr_Buffer, Prefs.ip.hdr_size);

	hdr_Vec[0].iov_base = hdr_Buffer;
	hdr_Vec[0].iov_len = Prefs.ip.hdr_size;
	hdr_Vec[1].iov_base = Payload;
	hdr_Vec[1].iov_len = Handshake.h.size;
    }

    // Resolve, connect
    struct addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    if (Prefs.ip.v6) hints.ai_family = AF_INET6; // enforce IPv6
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    Sock = ip_connect(ma->target, Prefs.ip.port, &hints);
    if (!Sock) {
	L("Client: Failed to connect to target.");
	return 0;
    }

    // Socket options
    int socket_buff_len = IP_DEF_SOCKET_BUFF;
    setsockopt(Sock, SOL_SOCKET, SO_SNDBUF, &socket_buff_len, sizeof(size_t));

    return ip_handshake_client(Sock, (handshake *) &Handshake, sizeof(Handshake));
}

bool
udp_serve(mod_args *ma)
{
    struct sockaddr *servaddr;

    int sd;
    socklen_t sock_size, tmp_sock_size;

    if (Prefs.ip.v6) {
	struct sockaddr_in6 *sin = safe_alloc(sizeof(*sin));

	memset(sin, 0, sizeof(*sin));
	sin->sin6_family = AF_INET6;
	sin->sin6_addr = in6addr_any;
	sin->sin6_port = htons(atoi(Prefs.ip.port));

	servaddr = (struct sockaddr *) sin;
	sock_size = sizeof(struct sockaddr_in6);
    } else {
	struct sockaddr_in *sin = safe_alloc(sizeof(*sin));

	memset(sin, 0, sizeof(*sin));
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = htonl(INADDR_ANY);
	sin->sin_port = htons(atoi(Prefs.ip.port));
  
	servaddr = (struct sockaddr *) sin;
	sock_size = sizeof(struct sockaddr_in);
    }
    sd = socket(servaddr->sa_family, SOCK_DGRAM, IPPROTO_UDP);
    if (sd == -1) XLE("Server: socket");

    int socket_buff_len = IP_DEF_SOCKET_BUFF;
    int one = 1;
    setsockopt(sd, SOL_SOCKET, SO_SNDBUF, &socket_buff_len, sizeof(size_t));
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    if (bind(sd, servaddr, sock_size) == -1) {
	XLE("Server: bind");
    }
	
    struct sockaddr *from = safe_alloc(sock_size);
    while (1) {		
	char *data;

	memset(from, 0, sock_size);

	// Handshake
	udp_handshake uh;
	tmp_sock_size = sock_size;
	if (recvfrom(sd, &uh, sizeof(uh), 0, from, &tmp_sock_size) == -1) {
	    LE("Server: recvfrom");
	    return 0;
	}

	int response;
	if (uh.h.size > 0 && uh.h.tries > 0 && uh.frag_size > 0 && uh.frag_size <= uh.h.size) {
	    data = safe_alloc(uh.h.size);

	    response = 1;
	    if (sendto(sd, &response, sizeof(response), 0, from, sock_size) == -1) {
		LE("Server: sendto");
		return false;
	    }
	    
	} else {
	    response = 0;
	    if (sendto(sd, &response, sizeof(response), 0, from, sock_size) == -1) {
		LE("Server: sendto");
		return false;
	    }

	    L("Server: handshake failed!");

	    continue;
	}
	
	// Measure-loop
	for (size_t i = 0; i < uh.h.tries; i++) {
	    size_t bytes;
	    ssize_t rc;

	    for (bytes = 0; bytes < uh.h.size; bytes += rc) {
		tmp_sock_size = sock_size;
		rc = recvfrom(sd, data + bytes, uh.h.size - bytes,
			      0, from, &tmp_sock_size);
		if (rc == -1) {
		    LE("Server: recvfrom");
		    return false;
		}
	    }

	    for (bytes = 0; bytes < uh.h.size; bytes += rc) {
		rc = sendto(sd, data + bytes, 
			   (bytes + uh.frag_size) > uh.h.size ? uh.h.size - bytes : uh.frag_size, 
			    0, from, sock_size);
		if (rc == -1) {
		    LE("Server: sendto");
		    return false;
		}
	    }
	}
	
	free(data);
    }

    return 1;
}

const net_mod mod_udp = { "UDP",
			  "udp",
			  IP_OPTS"F:",
			  IP_USAGE
			  "\n\t-F size: Send packets fragmented to `size'",
			  udp_handle_arg,
			  udp_init,
			  udp_measure,
			  udp_serve,
			  udp_cleanup };
