/* 
 * eins - A tool for benchmarking networks.
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
#include "modules.h"
#include "util.h"

#define DEF_TRIES 54

#define USAGE "Client: eins -t type [-i numtries] host len\nServer: eins -s -t type"

static void
usage_and_die(void)
{
    puts(USAGE);

    puts("\nAvailable modules with options. The word in brackets is the type for the -t option.\n"
	 "The -t option _must_ come _before_ any network specific options.");

    for (int i = 0; Modules[i]; i++) {
	printf("\n%s (%s):\n%s\n", Modules[i]->name, Modules[i]->type , Modules[i]->usage);
    }

    exit(1);
}

#define DEF_OPTS "st:i:"
static char *
build_optstr(const net_mod *m[])
{
    size_t len = strlen(DEF_OPTS);
    for (int i = 0; m[i]; i++) {
	len += strlen(m[i]->opts);
    }
    
    char *str = safe_alloc(len + 1);
    strcpy(str, DEF_OPTS);

    for (int i = 0; m[i]; i++) {
	// Double entries seem to be okay as long they don't
        // contradict.
	strcat(str, m[i]->opts);
    }

    return str;
}

const net_mod *
parse_args(int argc, char * argv[], mod_args *ma)
{
    const net_mod *nm = NULL;

    char *optstr = build_optstr(Modules);

    char opt;
    while ((opt = getopt(argc, argv, optstr)) != -1) {
	// Mode
	switch (opt) {
	case 's':
	    ma->mode = EINS_SERVER;
	    break;
	    
	    // Mods
	case 't':
	    for (int i = 0; Modules[i]; i++) {
		if (!strncmp(Modules[i]->type, optarg, strlen(optarg))) {
		    nm = Modules[i];
		}
	    }
	    break;

	case 'i':
	    ma->tries = atoi(optarg);
	    break;

	default:
	    if (nm) {
		if (!nm->handle_arg(opt, optarg)) {
		    L("Invalid option for `%s'.", nm->name);
		    return NULL;
		}
	    } else {
		L("No network type specified.");
		return NULL;
	    }
	}
    }

    free(optstr); // Possible memleak but neglectible

    if (!nm) {
	usage_and_die();
    }

    if ((ma->mode == EINS_CLIENT) && ((argc != (optind + 2)))) {
	usage_and_die();
    }
  
    if (ma->mode == EINS_CLIENT) {
	ma->target = argv[optind];
	ma->size = atoi(argv[optind + 1]);
    }

    return nm;
}

/******************************************************************************
 * main()
 */
int
main(int argc, char **argv)
{
    mod_args ma = { EINS_CLIENT, NULL, DEF_TRIES, 0, NULL };
    const net_mod *nm;
    long long i;

    srand((unsigned int) time(NULL));

    if (!log_open(NULL)) {
	puts("Failed to open log file.");
	exit(1);
    }

    nm = parse_args(argc, argv, &ma);
    if (!nm) exit(1);

    if (ma.mode == EINS_SERVER) {
	return !nm->serve(&ma);
    }

    // Set up time-measurement
    double *alltime, measuredelta;
    time_586 ta, tb;

    init_timer();

    alltime = safe_alloc(ma.tries * sizeof(double));

    // Obtain time which is spent on measuring
    gamma_time(ta);
    gamma_time(tb);
    measuredelta = gamma_time_diff(tb, ta);

    // Set up payload
    ma.payload = safe_alloc(ma.size);
    randomize_buffer(ma.payload, ma.size);

    // Set up mod
    if (!nm->init(&ma))
	XL("Init/Handshake failed.");

    // Main measure-loop
    for (i = 0; i < ma.tries; i++) {
	alltime[i] = (nm->measure() - measuredelta) / 2;
    }

    // Compute and print data
    double min, max, med, var;
    mean_variance(ma.tries, alltime, &min, &max, &med, &var);

    // `bytes / 1000^-2 * s = bytes * 1000^2 / s',
    // but we want `bytes * 1024^2 / s'
    double bw = ((ma.size * 1000 * 1000 ) / med) / (1024*1024);

    printf("%15d%16f%16f\n", ma.size, med, bw);

    nm->cleanup();

    log_close();
}
