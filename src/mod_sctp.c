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

#include <arpa/inet.h>

// SCTP
#include <netinet/sctp.h>

// Own
#include "eins.h"
#include "util_ip.h"
#include "mod_sctp.h"
#include "util.h"

#define SCTP_OPTS "6P:H:a:"
#define SCTP_USAGE "\t-6: Use IPv6\n" \
    "\t-a addresses: Bind or connect to multiple `addresses' given by a comma seperated list without spaces\n" \
    "\t-P port: Use `port'\n" \
    "\t-H size: Prepend with a header of `size' bytes"

#define SCTP_INPUT_STREAMS 1
#define SCTP_OUTPUT_STREAMS 1

static struct sctp_prefs {
    ip_prefs ip;

    size_t frag_size;
} Prefs = { { false, IP_DEF_ADDRESS, IP_DEF_PORT, 0 }, 0 };

typedef struct {
    handshake h;
    ip_handshake ip;    

    size_t frag_size;
} sctp_handshake;

// Source scope variables for the client
static char *Payload;
static int Sock;
static char *hdr_Buffer;
static struct iovec hdr_Vec[2];
static sctp_handshake Handshake;

bool
sctp_handle_arg(char opt, char *arg)
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
sctp_measure()
{
    return ip_measure(Sock, Payload, Handshake.h.size, Handshake.h.tries, Prefs.ip.hdr_size, hdr_Vec, Handshake.h.size);
}

void
sctp_cleanup()
{
    close(Sock);
    free(hdr_Buffer);
}


bool
sctp_init(mod_args *ma)
{
	int i;
	int len;
	char *seperator = ",";
	char *all_addresses;
	char **list;
	struct sockaddr_in addr;
	struct sockaddr_in *addresses;
	int addr_size = sizeof(struct sockaddr_in);
	int number_of_addresses;
    struct sctp_initmsg initmsg;
    struct sctp_status status;
	
    // Set prefs
    Payload = ma->payload;
    Handshake.h.size = ma->size;
    Handshake.h.tries = ma->tries;
    Handshake.frag_size =  Prefs.frag_size && Prefs.frag_size < ma->size ? Prefs.frag_size : ma->size;

    // If needed, set up buffer for the header + vector.
    if (Prefs.ip.hdr_size) {
	hdr_Buffer = safe_alloc(Prefs.ip.hdr_size);

	randomize_buffer(hdr_Buffer, Prefs.ip.hdr_size);

	hdr_Vec[0].iov_base = hdr_Buffer;
	hdr_Vec[0].iov_len = Prefs.ip.hdr_size;
	hdr_Vec[1].iov_base = Payload;
	hdr_Vec[1].iov_len = Handshake.h.size;
    }

    /* concatenate target and options */
    if (Prefs.ip.address != NULL){
    	all_addresses = (char *)calloc(strlen(ma->target) + strlen(Prefs.ip.address) + strlen(seperator), sizeof(char));
		strcpy(all_addresses, ma->target);
		strncat(all_addresses, seperator, strlen(seperator));
		strncat(all_addresses, Prefs.ip.address, strlen(Prefs.ip.address));   
    }
    else {
    	all_addresses = (char *)calloc(strlen(ma->target), sizeof(char));
		strcpy(all_addresses, ma->target);
    }    
         
	/* split target addresses from given input */
    number_of_addresses = split(&all_addresses, seperator, &list);	
    
    free(all_addresses);

	/* allocate address array */
	addresses = safe_alloc(addr_size * number_of_addresses);

	/* create endpoint */
	Sock= socket(AF_INET, SOCK_STREAM,
				  IPPROTO_SCTP);
	if (Sock < 0) {
		L("socket error");
		exit(1);
	}
	
	/* create all addresses */
	for(i=0; i < number_of_addresses; i++){
		addr.sin_family = AF_INET;
		if (Prefs.ip.v6) addr.sin_family = AF_INET6; // enforce IPv6
		addr.sin_addr.s_addr = inet_addr(list[i]);
		addr.sin_port = atoi(Prefs.ip.port); // htons(Prefs.ip.port);
		memcpy((void *)((int)addresses+(i*addr_size)), &addr, addr_size);
	}
	
    memset(&initmsg, 0, sizeof(initmsg));
    initmsg.sinit_max_instreams = SCTP_INPUT_STREAMS;
    initmsg.sinit_num_ostreams = SCTP_OUTPUT_STREAMS;
    L("Asking for: input streams: %d, output streams: %d\n",
				initmsg.sinit_max_instreams,
                initmsg.sinit_num_ostreams);

    if (setsockopt(Sock, IPPROTO_SCTP,
                   SCTP_INITMSG, &initmsg, sizeof(initmsg))) {
		L("set socket options\n");
    }
	
	/* connect */
    if (sctp_connectx(Sock, (struct sockaddr *) addresses, 1) < 0) {
		L("connectx");
        exit(1);
    }

    memset(&status, 0, sizeof(status));
    len = sizeof(status);
    status.sstat_assoc_id = 1;

    if (getsockopt(Sock, IPPROTO_SCTP, SCTP_STATUS, &status, &len) == -1) {
		L("get sock opt\n");
    }
    L("Got: input streams: %d, output streams: %d\n",
           status.sstat_instrms,
           status.sstat_outstrms);
    
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
sctp_serve(mod_args *ma)
{
	int i;
	int len;
	char *seperator = ",";
	char *all_addresses;
	char **list;
	struct sockaddr_in addr;
	struct sockaddr_in *addresses;
	int addr_size = sizeof(struct sockaddr_in);
	int number_of_addresses;
    struct sctp_initmsg initmsg;
    struct sctp_status status;
	int sd;
	int socket_buff_len = IP_DEF_SOCKET_BUFF;
	int one = 1;
	
	/* create endpoint */
	sd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	if (sd < 0) {
		L(NULL);
		exit(1);
	}
	setsockopt(sd, SOL_SOCKET, SO_SNDBUF, &socket_buff_len, sizeof(size_t));
	setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    /* concatenate target and options */
    if (ma->target == NULL)
    {
		/* 
		 * sctp_bindx(): needs at least a single ip 
		 * todo: what about ipv6
		 */ 
		ma->target = "127.0.0.1";
		//L("you need to specifiy at least one address for sctp_bindx()");
		//exit(1);
    }    
    if (Prefs.ip.address != NULL){
    	all_addresses = (char *)calloc(strlen(ma->target) + strlen(Prefs.ip.address) + strlen(seperator), sizeof(char));
		strcpy(all_addresses, ma->target);
		strncat(all_addresses, seperator, strlen(seperator));
		strncat(all_addresses, Prefs.ip.address, strlen(Prefs.ip.address));   
    }	
    else {
    	all_addresses = (char *)calloc(strlen(ma->target), sizeof(char));
		strcpy(all_addresses, ma->target);
    }    
         
	/* split target addresses from given input */
    number_of_addresses = split(&all_addresses, seperator, &list);	
    
    free(all_addresses);

	/* allocate address array */
	addresses = safe_alloc(addr_size * number_of_addresses);
	for(i=0; i < number_of_addresses; i++){
		addr.sin_family = AF_INET;
		if (Prefs.ip.v6) addr.sin_family = AF_INET6; // enforce IPv6
		addr.sin_addr.s_addr = inet_addr(list[i]);
		addr.sin_port = atoi(Prefs.ip.port); // htons(Prefs.ip.port);		
		L("Bind to adress: %s", list[i]);        
		memcpy((void *)((int)addresses+(i*addr_size)), &addr, addr_size);
	}		

	if (sctp_bindx(sd, (struct sockaddr *) addresses, number_of_addresses,
                 SCTP_BINDX_ADD_ADDR) == -1) {
		LE("Server: sctp_bindx");
		return false;
	}

    memset(&initmsg, 0, sizeof(initmsg));
    initmsg.sinit_max_instreams = SCTP_INPUT_STREAMS;
    initmsg.sinit_num_ostreams = SCTP_OUTPUT_STREAMS;
    L("Asking for: input streams: %d, output streams: %d\n",
				initmsg.sinit_max_instreams,
                initmsg.sinit_num_ostreams);

	if (setsockopt(sd, IPPROTO_SCTP,
                 SCTP_INITMSG, &initmsg, sizeof(initmsg))) {
		L("set sock opt\n");
	}

	/* specify queue */
    if (listen(sd, 5) == -1) {
		LE("Server: listen");
		return false;
    }

    unsigned int partner_addr_len = sizeof(struct sockaddr);
    struct sockaddr partner;
    int asd;
    char *data = NULL;
    while (1) {		
	if ((asd = accept(sd, &partner, &partner_addr_len)) == -1) {
	    LE("Server: accept");
	    return false;
	}

	memset(&status, 0, sizeof(status));
	len = sizeof(status);
	status.sstat_assoc_id = 0;

	if (getsockopt(asd, IPPROTO_SCTP,
				SCTP_STATUS, &status, &len) == -1) {
		L("get sock opt\n");
	}

	L("Got: input streams: %d, output streams: %d\n",
		status.sstat_instrms,
		status.sstat_outstrms);

	
	sctp_handshake th;
	/*if (!ip_handshake_server(asd, (handshake *) &th, sizeof(th))) {
	    L("Server: Handshake failed");
	    continue;
        }*/

	if (recv(asd, &th, sizeof(th), MSG_WAITALL) == -1) {
	    LE("Server: recvfrom");
	    return 0;
	}

	int response;
	if (th.h.size > 0 && th.h.tries > 0 && th.frag_size > 0 && th.frag_size <= th.h.size) {
	    data = safe_alloc(th.h.size);

	    response = 1;
	    if (send(asd, &response, sizeof(response), 0) == -1) {
		LE("Server: sendto");
		return false;
	    }
	    
	} else {
	    response = 0;
	    if (send(asd, &response, sizeof(response), 0) == -1) {
		LE("Server: sendto");
		return false;
	    }

	    L("Server: handshake failed!");

	    continue;
	}

	data = safe_alloc(th.h.size);
	
	int i;
	for (i = 0; i < th.h.tries; i++) {
	    int rc;
	    size_t bytes;
	    for (bytes = 0; bytes < th.h.size; bytes += rc) {
		rc = recv(asd, data + bytes, (bytes + th.frag_size) > th.h.size ? th.h.size - bytes : th.frag_size, 0);
		if (rc == -1) {
		    LE("Server: recv");
		    return false;
		}		
	    }

	    for (bytes = 0; bytes < th.h.size; bytes += rc) {
		rc = send(asd, data + bytes, 
                  (bytes + th.frag_size) > th.h.size ? th.h.size - bytes : th.frag_size, 0);
		if (rc == -1) {
		    LE("Server: send");
		    return false;
		}
	    }

	}
	close(asd);
	
	free(data);
    }

    return true;
}

const net_mod mod_sctp = { "SCTP",
			  "sctp",
			  SCTP_OPTS,
			  SCTP_USAGE,
			  sctp_handle_arg,
			  sctp_init,
			  sctp_measure,
			  sctp_serve,
			  sctp_cleanup };
