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
#include <sys/types.h>
#include <unistd.h>

// Own
#include "eins.h"
#include "mod_neon.h"
#include "util.h"
#include "measure.h"
#include <neon.h>
#include <mpi.h>

static int myId, numProcs;
static int targets[1];
static neon_tag_t tag[] = {1, 2};
static neon_tag_t tagShake = 3;
static neon_handle_t Post;
static int otherPE;
static size_t Size;
static unsigned char *recv_buffer;
static unsigned char *send_buffer;

typedef struct handshake {
    size_t size;
    size_t tries;
} handshake_t;

bool
neon_handle_arg(char opt, char *arg)
{
    return true;
}

double
neon_measure()
{
    neon_status_t status;
    neon_handle_t put;

    time_586 ta, tb;

    get_time(ta);
    
    put = NEON_Put(send_buffer, Size, 0, otherPE, tag[otherPE], 0, &status);
    NEON_Wait(&put, &status);

    NEON_Repost(Post, &status);
    NEON_Wait(&Post, &status);

    get_time(tb);

    return time_diff(tb, ta);
}

void
neon_cleanup()
{
/*     MPI_Barrier(MPI_COMM_WORLD); */

/*     NEON_Exit(); */
}

bool neon_serve(mod_args *);
bool
neon_init(mod_args *ma)
{
    Size = ma->size;

    static bool been_here = false;
    if (__unlikely(!been_here)) {
	been_here = true;

	char **fake_argv = { "eins" };
	int fake_argc = 1;

	int rc = NEON_Init(&fake_argc,  &fake_argv, &myId, &numProcs);
	if (0 == rc) {
	    otherPE = (myId + 1) % 2;
	    targets[0] = otherPE;
	    
	    if (0 == myId) {
		// Usage wasn't intended like that...but who cares. ;)
		neon_serve(ma);
		exit(1); // neon_serve() never returns
	    }
	} else {
	    return false;
	}
    }

    send_buffer = safe_alloc(Size);
    recv_buffer = safe_alloc(Size);
    memcpy(send_buffer, ma->payload, Size);

    // Perform handshake
    handshake_t h = { ma->size, ma->tries };
	    
    neon_status_t status;
    neon_handle_t put;
    
    put = NEON_Put(&h, sizeof(h), 0, otherPE, tagShake, 0, &status);
    NEON_Wait(&put, &status);

    // Warmup
    put = NEON_Put(send_buffer, h.size, 0, otherPE, tag[otherPE], 0, &status);
    NEON_Wait(&put, &status);

    Post = NEON_Post(recv_buffer, h.size, targets, tag[myId], &status);
    NEON_Wait(&Post, &status);       
    
    return true;
}

bool
neon_serve(mod_args *ma)
{
    if (0 == numProcs) XL("Serving is implicit for NEON! Don't run `eins -s -t n' by hand.");

    neon_status_t status;
    neon_handle_t post, put, sp;

    handshake_t h;
    sp = NEON_Post(&h, sizeof(h), targets, tagShake, &status);

    while (true) {	
	// Wait for size
	NEON_Wait(&sp, &status);

	char *buf = safe_alloc(h.size);

	// Warmup
	post = NEON_Post(buf, h.size, targets, tag[myId], &status);
	NEON_Wait(&post, &status);

	put = NEON_Put(buf, h.size, 0, otherPE, tag[otherPE], 0, &status);
	NEON_Wait(&put, &status);
    
	for (size_t i = 0; i < h.tries; i++) {
	    NEON_Repost(post, &status);
	    NEON_Wait(&post, &status);
	    
	    put = NEON_Put(buf, h.size, 0, otherPE, tag[otherPE], 0, &status);
	    NEON_Wait(&put, &status);
	}

/* 	MPI_Barrier(MPI_COMM_WORLD); */
/* 	NEON_Exit(); */
/* 	exit(0); */

	NEON_Repost(sp, &status);
    }
}

const net_mod mod_neon = { "NEON",
			  "neon",
			  "",
			  "",
			  neon_handle_arg,
			  neon_init,
			  neon_measure,
			  neon_serve,
			  neon_cleanup };
