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

#ifndef EINS_H
#define EINS_H

#include <sys/types.h>

enum eins_mode { EINS_CLIENT, EINS_SERVER };

// Program arguments
struct e_args {
    // Mode
    enum eins_mode mode;

    // Mod to use
    struct ec_mod *(*init)(struct e_args *);
    int (*serve)(struct e_args *);

    // General
    char *target;
    ssize_t tries;
    ssize_t size;

    // IP-/socket-based
    char *ip_port;
    ssize_t ip_hdr_size;
    short ip_v6;

    // UDP
    ssize_t udp_frag_size;

    // BMI-based
    char *bmi_method;

    // Payload
    char *payload;
};

// Network-measuring module
struct ec_mod {
    // Function for benchmarking - returns the measured time
    double (*measure)();

    // Clean up as appropriate
    void (*cleanup)();
};

// Handshake
struct e_handshake {
    ssize_t tries;
    ssize_t size;
    ssize_t frag_size;
};

#endif // EINS_H
