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
#include <math.h>

// POSIX
#include <getopt.h>
#include <time.h>

// Own
#include "eins.h"
#include "measure.h"
#include "modules.h"
#include "util.h"

#define DEF_TRIES 54

#define GENERAL_USAGE \
    "Client: eins -t type [-i numtries] [-n] [-q] [-u until [-b step by]] host len\n" \
    "\t-t `type': Type of network we are benchmarking\n" \
    "\t-i `numtries': Number of iterations for measuring (default: 54)\n" \
    "\t-n: Don't print results (useful for profiling)\n" \
    "\t-q: Be quiet, omit progress indicator\n" \
    "\t-u `until': Iterate from `len' until the given value\n" \
    "\t-b `step by': When iterating using `-u', step by given value (default: 1)\n" \
    "\thost: Communicate with `host'\n" \
    "\tlen: Send packets of size `len'. If `-u' is used, start with this value.\n" \
    "\n"\
    "Server: eins -s -t type\n" \
    "\t-s: Start as server"

#define DEF_OPTS "st:i:nu:b:q"

static void
usage_and_die(void)
{
    puts(GENERAL_USAGE);

    puts("\nAvailable modules with options. The word in brackets is the type for the -t option."
         "\nThe -t option _must_ come _before_ any network specific options.");

    for (int i = 0; Modules[i]; i++) {
        printf("\n%s (%s):\n%s\n", Modules[i]->name, Modules[i]->type , Modules[i]->usage);
    }

    exit(1);
}

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
parse_args(int argc, char * argv[], mod_args *ma, prefs *p)
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
            ma->tries = abs(atoi(optarg));
            if (ma->tries <= MIN_VALS)
                XL("Value of `-i' has to be bigger than %u.", MIN_VALS);
            break;

        case 'n':
            p->no_time = true;
            break;

        case 'u':
            p->until = atoi(optarg);
            break;

        case 'b':
            p->step = atoi(optarg);
            break;

        case 'q':
            p->quiet = true;
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

    if (!p->until)
        p->until = ma->size;

    return nm;
}

/******************************************************************************
 * main()
 */
int
main(int argc, char **argv)
{
    mod_args ma = { EINS_CLIENT, NULL, DEF_TRIES, 0, NULL };
    prefs p = { 0, 1, false, false };
    const net_mod *nm;

    srand((unsigned int) time(NULL));

    if (!log_open(NULL)) {
        puts("Failed to open log file.");
        exit(1);
    }

    nm = parse_args(argc, argv, &ma, &p);
    if (!nm) exit(1);

    if (ma.mode == EINS_SERVER) {
        return !nm->serve(&ma);
    }

    // Set up time-measurement
    double *alltime, measuredelta = 0.0;
    time_586 ta, tb;

    init_timer();

    if (!p.no_time) {
        // Obtain time which is spent on measuring
        get_time(&ta);
        get_time(&tb);
        measuredelta = time_diff(tb, ta);
    }

    unsigned int progress = 0;
    for (; ma.size <= p.until; ma.size += p.step) {

        alltime = safe_alloc(ma.tries * sizeof(double));

        // Set up payload
        ma.payload = safe_alloc(ma.size);
        randomize_buffer(ma.payload, ma.size);

        // Set up mod
        if (!nm->init(&ma)) XL("Init/Handshake failed.");

        // Main measure-loop
        for (size_t try = 0; try < ma.tries; try++) {
            alltime[try] = (nm->measure() - measuredelta) / 2;
        }

        if (!p.no_time) {
            // Compute and print data
            double min, max, med, var;
            mean_variance(ma.tries, alltime, &min, &max, &med, &var);

	    // `bytes / 1000^-2 * s = bytes * 1000^2 / s',
	    // but we want `bytes * 1024^2 / s'
	    // 15625 / 16384 = (1000*1000) / (1024*1024)
            // Obsoleted by: Networks use Base10!! Lesson learned ;-)
            // => 1000B/s = 1kB/s; 1000kB/s = 1MB/s
	    double bw = ma.size / med;
	    //bw *= (double) 15625 / 16384;

            printf("%15d%16f%16f%16f\n", (int)ma.size, med, bw, sqrt(var));
        }

        nm->cleanup();

        free(alltime);
        free(ma.payload);

        unsigned int progress_new = ((double) ma.size / p.until) * 100;
        if (!p.quiet && progress_new > progress && !(progress_new == 100 && progress == 0 )) {
            progress = progress_new;
            L("%u%% (%u Bytes) done.", progress, ma.size);
        }
    }

    log_close();
}
