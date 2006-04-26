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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// POSIX
#include <getopt.h>
#include <time.h>

// Own
#include "eins.h"
#include "measure.h"
#include "util.h"

// Mods
#include "ip.h"
#include "tcp.h"
#include "udp.h"
#include "bmi.h"

#define DEF_TRIES 54

#define USAGE "eins [-s] -t [tcp/udp [-p port] [-h header-size] [-f fragsize] [-6]]/[bmi] [-i numtries] [host len]"

static void
usage_and_die(void)
{
    puts(USAGE);
    exit(1);
}

void
parse_args(int argc, char **argv, struct e_args *ea)
{
    char opt;
    while ((opt = getopt(argc, argv, "st:6p:h:i:f:")) != -1) {
	// Mode
	switch (opt) {
	case 's':
	    ea->mode = EINS_SERVER;
	    break;
	    
	    // Mods
	case 't':
	    switch (optarg[0]) {
	    case 't':
		ea->init = tcp_init;
		ea->serve = tcp_serve;
		break;
	    case 'u':
		ea->init = udp_init;
		ea->serve = udp_serve;
		break;
#ifndef NO_BMI
	    case 'b':
		ea->init = bmi_init;
		ea->serve = bmi_serve;
#warning "TODO: Let BMI method be an argument."
		ea->bmi_method = "bmi_tcp";
#endif // NO_BMI
	    }
	    break;

	case 'i':
	    ea->tries = atoi(optarg);
	    break;


	// IP-Based
	case '6':
	    ea->ip_v6 = 1;
	    break;
	case 'p':
	    ea->ip_port = safe_strdup(optarg);
	    break;
	case 'h':
	    ea->ip_hdr_size = atoi(optarg);
	    break;

	// UDP
	case 'f':
	    ea->udp_frag_size = atoi(optarg);
	    break;

	default:
	    usage_and_die();
	}
    }

    if ((ea->mode == EINS_CLIENT) && ((argc != (optind + 2)) || !ea->init)) {
	usage_and_die();
    }
  
    if (ea->mode == EINS_CLIENT) {
	ea->target = argv[optind];
	ea->size = atoi(argv[optind + 1]);
    }

    if (ea->udp_frag_size == 0 || ea->udp_frag_size > ea->size) {
	ea->udp_frag_size = ea->size;
    }
}

/******************************************************************************
 * main()
 */
int
main(int argc, char **argv)
{
    struct e_args ea = { EINS_CLIENT, NULL, NULL, NULL, DEF_TRIES, 0, IP_DEF_PORT, 0 };
    struct ec_mod *em = NULL;

    long long i;

    srand((unsigned int) time(NULL));

    parse_args(argc, argv, &ea);

    if (ea.mode == EINS_SERVER) {
	return !ea.serve(&ea);
    }

    // Set up time-measurement
    double *alltime, measuredelta;
    time_586 ta, tb;

    init_timer();

    alltime = safe_alloc(ea.tries * sizeof(double));

    // Obtain time which is spent on measuring
    gamma_time(ta);
    gamma_time(tb);
    measuredelta = gamma_time_diff(tb, ta);

    // Set up payload
    ea.payload = safe_alloc(ea.size);
    randomize_buffer(ea.payload, ea.size);

    // Set up mod
    em = ea.init(&ea);
    if (!em) {
	puts("Init/Handshake failed.");
	exit(1);
    }

    // Main measure-loop
    for (i = 0; i < ea.tries; i++) {
	alltime[i] = (em->measure() - measuredelta) / 2;
    }

    // Compute and print data
    double min, max, med, var;
    mean_variance(ea.tries, alltime, &min, &max, &med, &var);
    printf("%15d%16f%16f\n", ea.size + ea.ip_hdr_size, med, ea.size / med);

    em->cleanup();
}
