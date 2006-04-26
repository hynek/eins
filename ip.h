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

#ifndef IP_H
#define IP_H

#include <netdb.h>

#define IP_DEF_PORT "8910"
#define IP_DEF_SOCKET_BUFF 524288

int ip_connect(char *host, char *port, struct addrinfo *hints);
double ip_measure(int sd, char *payload, int size, int tries, int hdr_size, struct iovec *hdr_vec, ssize_t frag_size);
int ip_handshake_client(int sd, struct e_handshake *);
int ip_handshake_server(int sd, struct e_handshake *);

#endif /* IP_H */
