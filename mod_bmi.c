/*
 *  eins - A tool for measuring network-bandwidths and -latencies.
 *  Copyright (C) 2006  Hynek Schlawack <hs+eins@ox.cx>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

// ISO
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdbool.h>
 
// PVFS
#include <bmi.h>

// Own
#include "mod_bmi.h"
#include "util.h"
#include "measure.h"
#include "log.h"

// Source scope variables for the client
static char *buf_Snd;
static char *buf_Rcv;
static unsigned int Tries;
static unsigned int Size;
static bmi_context_id Context;
static PVFS_BMI_addr_t Server;

bool
bmi_handle_arg(char opt, char *arg)
{
    // No BMI specific options
    return false;
}

double
bmi_measure()
{
    time_586 ta, tb;
    int outcount = 0, error_code;
    bmi_size_t actual_size;

    bmi_op_id_t bmi_id;
    get_time(ta);
    int ret = BMI_post_send(&bmi_id, Server, buf_Snd,
			    Size, BMI_PRE_ALLOC, 0, NULL, Context);
    error_code = 0;	    
    if (ret == 0) {
	do {
	    ret = BMI_test(bmi_id, &outcount, &error_code, &actual_size,
			   NULL, 100, Context);
	} while (ret == 0 && outcount == 0);
    }
    if (ret < 0 || error_code != 0) {
	L("Client: BMI send error.\n");
	return (-1);
    }

    ret = BMI_post_recv(&bmi_id, Server, buf_Rcv,
			Size, &actual_size, BMI_PRE_ALLOC, 0,
			NULL, Context);
    error_code = 0;	    
    if (ret == 0) {
	do {
	    ret = BMI_test(bmi_id, &outcount, &error_code, &actual_size,
			   NULL, 100, Context);
	} while (ret == 0 && outcount == 0);
    }
    if (ret < 0 || error_code != 0) {
	L("Client: BMI recv error.\n");
	return (-1);
    }

    get_time(tb);
    
    return time_diff(tb, ta);
}

void
bmi_cleanup()
{
    BMI_close_context(Context);
//    BMI_finalize();
#warning "BMI_finalize() not called."
}

bool
bmi_init(mod_args *ma)
{
    int ret;

    // Set prefs
    Tries = ma->tries;
    Size = ma->size;

    // Init & Resolve
    char *method;
    if (!strncmp(ma->target, "tcp", 3)) {
	method = "bmi_tcp";
    } else {
	L("Client: Unknown BMI method.");
	return false;
    }

    static bool first = true;
    if (first) {
	if (BMI_initialize(method, NULL, 0) != 0) {
	    L("Client: BMI_initialize() failed.");
	    return false;
	}
	first = false;
    }

    if (BMI_open_context(&Context) != 0) {
	L("Client: BMI_open_context() failed.");
	return false;
    }


    if (BMI_addr_lookup(&Server, ma->target) != 0) {
	L("Client: BMI_addr_lookup() failed.");
	return false;
    }

    buf_Snd = BMI_memalloc(Server, ma->size, BMI_SEND);
    memcpy(buf_Snd, ma->payload, ma->size);
    buf_Rcv = BMI_memalloc(Server, ma->size, BMI_RECV);    

    
    // Handshake
    bmi_op_id_t bmi_id;

    // Send size and num of tries
    handshake eh = { ma->tries, ma->size };
    ret = BMI_post_sendunexpected(&bmi_id, Server, &eh, sizeof(eh), BMI_EXT_ALLOC, 0, NULL, Context);
    if (ret < 0) {
	L("Client: BMI_post_sendunexpected() failed while handshake.");
	return false;
    }

    int error_code, outcount;
    bmi_size_t actual_size;
    if (ret == 0) {
	do {
	    ret = BMI_test(bmi_id, &outcount, &error_code,
			   &actual_size, NULL, 100, Context);
	} while (ret == 0 && outcount == 0);
	if (ret < 0) {
	    L("Client: BMI_test() failed while handshake");
	    return false;
	}
    }

    // Wait for ok
    int answer;
    BMI_post_recv(&bmi_id, Server, &answer, sizeof(answer), &actual_size, BMI_EXT_ALLOC, 0, NULL, Context);
    do {
	ret = BMI_test(bmi_id, &outcount, &error_code, &actual_size,
		       NULL, 100, Context);
    } while (ret == 0 && outcount == 0);

    if (ret < 0 || error_code != 0) {
	L( "Client: BMI receive error while handshake.\n");
	return 0;
    }

    return answer == 1;
}

bool
bmi_serve(mod_args *ma)
{
#warning "TODO: Make method supplyable"
    char *method = "bmi_tcp";
    struct BMI_unexpected_info u_info;
#warning "TODO: Make local_address dynamic"
    char *local_address = "tcp://127.0.0.1:3334";
    int outcount = 0, error_code = 0, ret;
    bmi_context_id context;

    // Init
    if (BMI_initialize(method, local_address, BMI_INIT_SERVER) != 0) {
	L("Server: BMI_initialize() failed.");
	return false;
    }

    if (BMI_open_context(&context) != 0) {
	L("Server: BMI_open_context() failed.");
	return false;
    }

    while (1) {
	// test unexpected (ie. wait for client)
	do {
	    ret = BMI_testunexpected(1, &outcount, &u_info, 100);
    	} while (ret == 0 && outcount == 0);
	if (ret < 0)
	    return false;
	
	PVFS_BMI_addr_t client = u_info.addr;
	handshake *h = u_info.buffer;

	int reply;

	if (h->size > 0 && h->tries > 0) {
	    reply = 1;
	} else {
	    reply = 0;
	}

	bmi_op_id_t bmi_id;
	bmi_size_t actual_size;

	ret = BMI_post_send(&bmi_id, client, &reply,
			    sizeof(reply), BMI_EXT_ALLOC, 0, NULL, context);
	error_code = 0;
	if (ret == 0) {
	    do {
		ret = BMI_test(bmi_id, &outcount, &error_code, &actual_size,
			       NULL, 100, context);
	    } while (ret == 0 && outcount == 0);
	}
	if (ret < 0 || error_code != 0) {
	    L("Server: BMI send error while handshake: %d\n", error_code);
	    return false;
	}

	if (!reply) {
	    L("Server: Handshake failed due to invalid values.");
	    continue;
	}
		
	// Alloc buffers
	char *buf_snd = BMI_memalloc(client, h->size, BMI_SEND);
	char *buf_rcv = BMI_memalloc(client, h->size, BMI_RECV);
	
	for (int i = 0; i < h->tries; i++) {
	    ret = BMI_post_recv(&bmi_id, client, buf_rcv,
				h->size, &actual_size, BMI_PRE_ALLOC, 0,
				NULL, context);
	    error_code = 0;
	    if (ret == 0) {
		do {
		    ret = BMI_test(bmi_id, &outcount, &error_code, &actual_size,
				   NULL, 100, context);
		} while (ret == 0 && outcount == 0);
	    }
	    if (ret < 0 || error_code != 0) {
		L("Server: BMI recv error: %d.\n", error_code);
		return false;
	    }

	    // In the first iteration, we copy the buffer over so we really send
	    // the same data. The slightly worse (well: O(N))
            // measure will be filtered out anyway.
	    if (__unlikely(i == 0)) memcpy(buf_snd, buf_rcv, h->size);
			
	    ret = BMI_post_send(&bmi_id, client, buf_snd,
	    			h->size, BMI_PRE_ALLOC, 0, NULL, context);
	    error_code = 0;
	    if (ret == 0) {
		do {
		    ret = BMI_test(bmi_id, &outcount, &error_code, &actual_size,
				   NULL, 100, context);
		} while (ret == 0 && outcount == 0);
	    }
	    if (ret < 0 || error_code != 0) {
        	L("Server: BMI send error.");
		return false;
	    }
	}

	// Clean up
	BMI_memfree(client, buf_snd, h->size, BMI_SEND);
	BMI_memfree(client, buf_rcv, h->size, BMI_RECV);
    }

    BMI_close_context(context);
    BMI_finalize();

    return true;
}

const net_mod mod_bmi = { "BMI",
			  "bmi",
			  "",
			  "\tNo specific options.",
			  bmi_handle_arg,
			  bmi_init,
			  bmi_measure,
			  bmi_serve,
			  bmi_cleanup };
