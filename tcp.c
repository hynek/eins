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

// POSIX
#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// Own
#include "eins.h"
#include "ip.h"
#include "tcp.h"
#include "util.h"

// Source-scope-vars for the client
static char *Payload;
static int Sock;
static unsigned int hdr_Size;
static char *hdr_Buffer;
static struct iovec hdr_Vec[2];
static struct e_handshake Handshake;

double
tcp_measure()
{
    return ip_measure(Sock, Payload, Handshake.size, Handshake.tries, hdr_Size, hdr_Vec, Handshake.size);
}

void
tcp_cleanup()
{
    close(Sock);
    free(hdr_Buffer);
}


struct ec_mod *
tcp_init(struct e_args *ea)
{
    struct ec_mod *em = safe_alloc(sizeof(struct ec_mod));

    // Setup hooks
    em->measure = tcp_measure;
    em->cleanup = tcp_cleanup;

    // Set prefs
    Payload = ea->payload;
    Handshake.size = ea->size;
    Handshake.tries = ea->tries;
    Handshake.frag_size = ea->udp_frag_size;
    hdr_Size = ea->ip_hdr_size;

    // If needed, set up buffer for the header + vector.
    if (hdr_Size) {
	hdr_Buffer = safe_alloc(hdr_Size);

	randomize_buffer(hdr_Buffer, hdr_Size);

	hdr_Vec[0].iov_base = hdr_Buffer;
	hdr_Vec[0].iov_len = hdr_Size;
	hdr_Vec[1].iov_base = Payload;
	hdr_Vec[1].iov_len = Handshake.size;
    }

    // Resolve, connect
    struct addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    if (ea->ip_v6) hints.ai_family = AF_INET6; // enforce IPv6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    Sock = ip_connect(ea->target, ea->ip_port, &hints);
    if (!Sock) {
	puts("Failed to connect to target.");
	return 0;
    }

    // Socket options
    int socket_buff_len = IP_DEF_SOCKET_BUFF;
    int one = 1;
    setsockopt(Sock, SOL_SOCKET, SO_SNDBUF, &socket_buff_len, sizeof(size_t));
    setsockopt(Sock, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));

    return ip_handshake_client(Sock, &Handshake) ? em : 0;
}

int
tcp_serve(struct e_args *ea)
{
    struct addrinfo hints, *ai;
    int rc;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = ea->ip_v6? AF_INET6 : AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    rc = getaddrinfo(NULL, ea->ip_port, &hints, &ai);
    if (rc != 0) {
	puts(gai_strerror(rc));
	exit(1);
    }

    int sd;
    int socket_buff_len = IP_DEF_SOCKET_BUFF;
    int one = 1;
    sd = socket(ai->ai_family, SOCK_STREAM, IPPROTO_TCP);
    setsockopt(sd, SOL_SOCKET, SO_SNDBUF, &socket_buff_len, sizeof(size_t));
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    setsockopt(sd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
    if (bind(sd, (struct sockaddr *) ai->ai_addr, ai->ai_addrlen) == -1) {
	perror("server, bind");
	exit(1);
    }

    freeaddrinfo(ai);

    if (listen(sd, 5) == -1) {
	perror("server, listen");
	exit(1);
    }

    unsigned int partner_addr_len = sizeof(struct sockaddr);
    struct sockaddr partner;
    int asd;
    char *data = NULL;
    while (1) {		
	if ((asd = accept(sd, &partner, &partner_addr_len)) == -1) {
	    perror("server, accept");
	    exit(1);
	}
	
	struct e_handshake eh;
	if (!ip_handshake_server(asd, &eh)) {
	    puts("Handshake failed");
	    continue;
	}

	data = safe_alloc(eh.size);
	
	int i;
	for (i = 0; i < eh.tries; i++) {
	    int bytes = 0;
	    while (bytes < eh.size) {
		int rc = recv(asd, data, eh.size, 0);
		if (rc == -1) {
		    perror("server, recv");
		    exit(1);
		}
		bytes += rc;
	    }
	    send(asd, data, eh.size, 0);
	}
	close(asd);
	
	free(data);
    }

    return 1;
}
