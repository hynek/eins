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

#ifndef MODS_H
#define MODS_H

// ISO
#include <stdbool.h>
#include <sys/types.h>

enum eins_mode { EINS_CLIENT, EINS_SERVER };
typedef struct {
    // Mode
    enum eins_mode mode;

    // General
    char *target;
    size_t tries;
    size_t size;

    // Payload
    char *payload;
} mod_args;


typedef struct {
    // Human readable name
    char *name;

    // Shortcut for -t option
    char *type;

    // For getopt()
    char *opts;

    // Module specific usage instructions
    char *usage;

    // Handle a specific option
    // Is called repeatedly for module-args
    bool (*handle_arg)(char opt, char *arg);

    // Connect, handshake et al
    bool (*init)(mod_args *ma);

    // Function for benchmarking - returns the measured time
    double (*measure)();

    bool (*serve)(mod_args *ma);

    // Clean up as appropriate
    void (*cleanup)();
} net_mod;

// Handshake
typedef struct {
    size_t tries;
    size_t size;
} handshake;

#endif //MODS_H
