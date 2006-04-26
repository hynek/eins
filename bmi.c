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
 
// PVFS
#include <bmi.h>

// Own
#include "bmi.h"
#include "util.h"
#include "measure.h"

// Source-scope-vars for the client
static char *buf_Snd;
static char *buf_Rcv;
static unsigned int Tries;
static unsigned int Size;
static bmi_context_id Context;
static PVFS_BMI_addr_t Server;

double
bmi_measure()
{
    time_586 ta, tb;
    int outcount = 0, error_code;
    bmi_size_t actual_size;

    bmi_op_id_t bmi_id;
    gamma_time(ta);
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
	fprintf(stderr, "Client: BMI send error.\n");
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
	fprintf(stderr, "Client: BMI recv error.\n");
	return (-1);
    }

    gamma_time(tb);
    
    return gamma_time_diff(tb, ta);
}

void
bmi_cleanup()
{
    BMI_close_context(Context);
    BMI_finalize();
}

struct ec_mod *
bmi_init(struct e_args *ea)
{
    struct ec_mod *em = safe_alloc(sizeof(struct ec_mod));
    int ret;

    // Setup hooks
    em->measure = bmi_measure;
    em->cleanup = bmi_cleanup;

    // Set prefs
    Tries = ea->tries;
    Size = ea->size;

    // Init & Resolve
#warning "TODO: proper error-handling"
    assert(BMI_initialize(ea->bmi_method, NULL, 0) == 0);
    assert(BMI_open_context(&Context) == 0);
    assert(BMI_addr_lookup(&Server, ea->target) == 0);

    buf_Snd = BMI_memalloc(Server, ea->size, BMI_SEND);
    memcpy(buf_Snd, ea->payload, ea->size);
    buf_Rcv = BMI_memalloc(Server, ea->size, BMI_RECV);    

    
    // Handshake
    bmi_op_id_t bmi_id;

    // Send size and num of tries
    struct e_handshake eh = { ea->tries, ea->size, 0 };
    BMI_post_sendunexpected(&bmi_id, Server, &eh, sizeof(eh), BMI_EXT_ALLOC, 0, NULL, Context);

    int error_code, outcount;
    bmi_size_t actual_size;
    do {
        ret = BMI_test(bmi_id, &outcount, &error_code,
                       &actual_size, NULL, 100, Context);
    } while (ret == 0 && outcount == 0);

    // Wait for ok
/*     struct BMI_unexpected_info u_info; */
/*     do { */
/*         ret = BMI_testunexpected(1, &outcount, &u_info, 100); */
/*     } while (ret == 0 && outcount == 0); */
    
    int answer;
    BMI_post_recv(&bmi_id, Server, &answer, sizeof(answer), &actual_size, BMI_EXT_ALLOC, 0, NULL, Context);
    do {
	ret = BMI_test(bmi_id, &outcount, &error_code, &actual_size,
		       NULL, 100, Context);
    } while (ret == 0 && outcount == 0);

    if (ret < 0 || error_code != 0) {
	fprintf(stderr, "Client: BMI receive error while handshake.\n");
	return 0;
    }

    return answer == 1 ? em : 0;
}

int
bmi_serve(struct e_args *ea)
{
#warning "TODO: Make method supplyable"
    char * method = "bmi_tcp";
    struct BMI_unexpected_info u_info;
#warning "TODO: Make local_address dynamic"
    char *local_address = "tcp://kato:3334";
    int outcount = 0, error_code = 0, ret;
    bmi_context_id context;

    // Init
    assert(BMI_initialize(method, local_address, BMI_INIT_SERVER) == 0);
    assert(BMI_open_context(&context) == 0);

    while (1) {
	// test unexpected (ie. wait for client)
	do {
	    ret = BMI_testunexpected(1, &outcount, &u_info, 100);
    	} while (ret == 0 && outcount == 0);
	if (ret < 0)
	    return (-1);
	
	PVFS_BMI_addr_t client = u_info.addr;
	struct e_handshake *eh = u_info.buffer;

#warning "TODO: Check handshake for sense"
	int reply = 1;
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
	    fprintf(stderr, "Server: BMI send error while handshake: %d\n", error_code);
	    return (-1);
	}
		
	// Alloc buffers
	char *buf_snd = BMI_memalloc(client, eh->size, BMI_SEND);
	char *buf_rcv = BMI_memalloc(client, eh->size, BMI_RECV);
	//randomize_buffer(buf_rcv, eh->size);
	
	for (int i = 0; i < eh->tries; i++) {
	    ret = BMI_post_recv(&bmi_id, client, buf_rcv,
				eh->size, &actual_size, BMI_PRE_ALLOC, 0,
				NULL, context);
	    error_code = 0;
	    if (ret == 0) {
		do {
		    ret = BMI_test(bmi_id, &outcount, &error_code, &actual_size,
				   NULL, 100, context);
		} while (ret == 0 && outcount == 0);
	    }
	    if (ret < 0 || error_code != 0) {
		fprintf(stderr, "Server: BMI recv error: %d.\n", error_code);
		return (-1);
	    }

	    // In the first iteration, we copy the buffer over so we really send
	    // the same data. The slightly worse (well: O(N))
            // measure will be filtered out anyway.
	    if (__unlikely(i == 0)) memcpy(buf_snd, buf_rcv, eh->size);
			
	    ret = BMI_post_send(&bmi_id, client, buf_snd,
	    			eh->size, BMI_PRE_ALLOC, 0, NULL, context);
	    error_code = 0;
	    if (ret == 0) {
		do {
		    ret = BMI_test(bmi_id, &outcount, &error_code, &actual_size,
				   NULL, 100, context);
		} while (ret == 0 && outcount == 0);
	    }
	    if (ret < 0 || error_code != 0) {
        	fprintf(stderr, "Server: BMI send error.\n");
		return (-1);
	    }
	}

	// Clean up
	BMI_memfree(client, buf_snd, eh->size, BMI_SEND);
	BMI_memfree(client, buf_rcv, eh->size, BMI_RECV);
    }

    BMI_close_context(context);
    BMI_finalize();

    return 0;
}
