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
#include "mod_accept.h"
#include "util.h"
#include "measure.h"

static struct accept_prefs {
    ip_prefs ip;
} Prefs = { { false, IP_DEF_PORT, 0 } };

char* Host;

bool
accept_handle_arg(char opt, char *arg)
{
    return ip_handle_arg((ip_prefs *) &Prefs, opt, arg);
}

double
accept_measure()
{
    time_586 ta, tb;
    
    struct addrinfo *ai, *tmp;

    // Resolve, connect
    struct addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    if (Prefs.ip.v6) hints.ai_family = AF_INET6; // enforce IPv6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    int rc = getaddrinfo(Host, Prefs.ip.port, &hints, &ai);
    if (rc != 0) {
        L(gai_strerror(rc));
        exit(1);
    }
	
    int sd;
    for (tmp = ai; tmp; tmp = tmp->ai_next) {
        sd = socket(tmp->ai_family, tmp->ai_socktype, tmp->ai_protocol);

        get_time(ta);
        if (connect(sd, tmp->ai_addr, tmp->ai_addrlen) == 0)
            break;
        close(sd);
    }

	get_time(tb);

    if (tmp == NULL) {
        LE("Client: connect"); 
        return 0.0;
    }
    close (sd);

    freeaddrinfo(ai);

    return time_diff(tb, ta);
}

void
accept_cleanup()
{
    free(Host);
}


bool
accept_init(mod_args *ma)
{
    Host = safe_alloc(strlen(ma->target)+1);
    strncpy(Host, ma->target, strlen(ma->target)+1);
    return true;
}

bool
accept_serve(mod_args *ma)
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
        LE("Server: bind");
        return false;
    }

    freeaddrinfo(ai);

    if (listen(sd, 5) == -1) {
        LE("Server: listen");
        return false;
    }

    unsigned int partner_addr_len = sizeof(struct sockaddr);
    struct sockaddr partner;
    int asd;
    while (1) {		
        if ((asd = accept(sd, &partner, &partner_addr_len)) == -1) {
            LE("Server: accept");
            return false;
        }
	
        close(asd);
    }

    return true;
}

const net_mod mod_accept = { "ACCEPT",
                             "accept",
                             IP_OPTS,
                             IP_USAGE,
                             accept_handle_arg,
                             accept_init,
                             accept_measure,
                             accept_serve,
                             accept_cleanup };
