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
#include "udp.h"
#include "util.h"


// Source-scope-vars for the client
static char *Payload;
static int Sock;
static struct e_handshake Handshake;
static unsigned int hdr_Size;
static char *hdr_Buffer;
static struct iovec hdr_Vec[2];

double
udp_measure()
{
    return ip_measure(Sock, Payload, Handshake.size, Handshake.tries, hdr_Size, hdr_Vec, Handshake.frag_size);
}

void
udp_cleanup()
{
    close(Sock);
    free(hdr_Buffer);
}

struct ec_mod *
udp_init(struct e_args *ea)
{
    struct ec_mod *em = safe_alloc(sizeof(struct ec_mod));

    // Setup hooks
    em->measure = udp_measure;
    em->cleanup = udp_cleanup;
    
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
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    Sock = ip_connect(ea->target, ea->ip_port, &hints);
    if (!Sock) {
	puts("Failed to connect to target.");
	return 0;
    }

    // Socket options
    int socket_buff_len = IP_DEF_SOCKET_BUFF;
    setsockopt(Sock, SOL_SOCKET, SO_SNDBUF, &socket_buff_len, sizeof(size_t));

    return ip_handshake_client(Sock, &Handshake) ? em : 0;
}

int
udp_serve(struct e_args *ea)
{
    struct sockaddr_in servaddr;

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = ea->ip_v6? AF_INET6 : AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(atoi(ea->ip_port));
  
    int sd = socket(servaddr.sin_family, SOCK_DGRAM, IPPROTO_UDP);

    int socket_buff_len = IP_DEF_SOCKET_BUFF;
    int one = 1;
    setsockopt(sd, SOL_SOCKET, SO_SNDBUF, &socket_buff_len, sizeof(size_t));
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    if (bind(sd, (struct sockaddr *) &servaddr, sizeof(servaddr)) == -1) {
	perror("server, bind");
	exit(1);
    }
	
    while (1) {		
	char *data;
	struct sockaddr from;
	socklen_t sock_size;

	memset(&from, 0, sizeof(from));
	sock_size = sizeof(struct sockaddr);

	// Handshake
	struct e_handshake eh;
	if (recvfrom(sd, &eh, sizeof(struct e_handshake), 0, &from, &sock_size) == -1) {
	    perror("server, recvfrom");
	    return 0;
	}

	int response;
	if (eh.size > 0 && eh.tries > 0 && eh.frag_size > 0) {
	    data = safe_alloc(eh.size);

	    response = 1;
	    if (sendto(sd, &response, sizeof(response), 0, &from, sizeof(struct sockaddr)) == -1) {
		perror("server, sendto");
		return 0;
	    }
	    
	} else {
	    response = 0;
	    if (sendto(sd, &response, sizeof(response), 0, &from, sizeof(struct sockaddr)) == -1) {
		perror("server, sendto");
		return 0;
	    }

	    puts("server, handshake failed!");

	    return 0;
	}
	
	// Measure-loop
	for (int i = 0; i < eh.tries; i++) {
	    size_t bytes;
	    ssize_t rc;

	    for (bytes = 0; bytes < eh.size; bytes += rc) {
		sock_size = sizeof(struct sockaddr);
		rc = recvfrom(sd, data + bytes, eh.size - bytes,
			      0, &from, &sock_size);
		if (rc == -1) {
		    perror("server, recvfrom");
		    exit(1);
		}
	    }

	    for (bytes = 0; bytes < eh.size; bytes += rc) {
		rc = sendto(sd, data + bytes, 
			   (bytes + eh.frag_size) > eh.size ? eh.size - bytes : eh.frag_size, 
			    0, &from, sizeof(struct sockaddr));
		if (rc == -1) {
		    perror("server, sendto");
		    exit(1);
		}
	    }
	}
	
	free(data);
    }

    return 1;
}
