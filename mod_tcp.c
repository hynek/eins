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
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// Own
#include "eins.h"
#include "util_ip.h"
#include "mod_tcp.h"
#include "util.h"

const net_mod mod_tcp = { "TCP",
			  "tcp",
			  IP_OPTS,
			  IP_USAGE,
			  tcp_handle_arg,
			  tcp_init,
			  tcp_measure,
			  tcp_serve,
			  tcp_cleanup };

static struct tcp_prefs {
    ip_prefs ip;
} Prefs = { { false, IP_DEF_PORT, 0 } };

typedef struct {
    handshake h;
    ip_handshake ip;    
} tcp_handshake;

// Source scope variables for the client
static char *Payload;
static int Sock;
static char *hdr_Buffer;
static struct iovec hdr_Vec[2];
static tcp_handshake Handshake;

bool
tcp_handle_arg(char opt, char *arg)
{
    // TCP has only IP specific options
    return ip_handle_arg((ip_prefs *) &Prefs, opt, arg);
}

double
tcp_measure()
{
    return ip_measure(Sock, Payload, Handshake.h.size, Handshake.h.tries, Prefs.ip.hdr_size, hdr_Vec, Handshake.h.size);
}

void
tcp_cleanup()
{
    close(Sock);
    free(hdr_Buffer);
}


bool
tcp_init(mod_args *ma)
{
    // Set prefs
    Payload = ma->payload;
    Handshake.h.size = ma->size;
    Handshake.h.tries = ma->tries;

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
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    Sock = ip_connect(ma->target, Prefs.ip.port, &hints);
    if (!Sock) {
	L("Failed to connect to target.");
	return 0;
    }

    // Socket options
    int socket_buff_len = IP_DEF_SOCKET_BUFF;
    int one = 1;
    setsockopt(Sock, SOL_SOCKET, SO_SNDBUF, &socket_buff_len, sizeof(size_t));
    setsockopt(Sock, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));

    return ip_handshake_client(Sock, (handshake *) &Handshake, sizeof(Handshake));
}

bool
tcp_serve(mod_args *ma)
{
    struct addrinfo hints, *ai;
    int rc;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = Prefs.ip.v6? AF_INET6 : AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    rc = getaddrinfo(NULL, Prefs.ip.port, &hints, &ai);
    if (rc != 0) {
	L(gai_strerror(rc));
	return false;
    }

    int sd;
    int socket_buff_len = IP_DEF_SOCKET_BUFF;
    int one = 1;
    sd = socket(ai->ai_family, SOCK_STREAM, IPPROTO_TCP);
    setsockopt(sd, SOL_SOCKET, SO_SNDBUF, &socket_buff_len, sizeof(size_t));
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    setsockopt(sd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
    if (bind(sd, (struct sockaddr *) ai->ai_addr, ai->ai_addrlen) == -1) {
	LE("server, bind");
	return false;
    }

    freeaddrinfo(ai);

    if (listen(sd, 5) == -1) {
	LE("server, listen");
	return false;
    }

    unsigned int partner_addr_len = sizeof(struct sockaddr);
    struct sockaddr partner;
    int asd;
    char *data = NULL;
    while (1) {		
	if ((asd = accept(sd, &partner, &partner_addr_len)) == -1) {
	    LE("server, accept");
	    return false;
	}
	
	tcp_handshake eh;
	if (!ip_handshake_server(asd, (handshake *) &eh, sizeof(eh))) {
	    L("Handshake failed");
	    continue;
	}

	data = safe_alloc(eh.h.size);
	
	int i;
	for (i = 0; i < eh.h.tries; i++) {
	    int bytes = 0;
	    while (bytes < eh.h.size) {
		int rc = recv(asd, data, eh.h.size, 0);
		if (rc == -1) {
		    LE("server, recv");
		    return false;
		}
		bytes += rc;
	    }
	    send(asd, data, eh.h.size, 0);
	}
	close(asd);
	
	free(data);
    }

    return true;
}
