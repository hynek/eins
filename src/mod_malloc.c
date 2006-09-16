/* 
 * eins - A tool for benchmarking networks. (and memory ;))
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

#include <string.h>
#include <stdlib.h>

#include "measure.h"
#include "mods.h"
#include "mod_malloc.h"

static char *Payload;
static size_t Size;

bool
malloc_handle_arg(char opt, char *arg)
{
    return true;
}

double
malloc_measure()
{
    time_586 ta, tb;

    get_time(ta);
    char * buf = malloc(Size);
    memcpy(buf, Payload, Size);
    free(buf);
    get_time(tb);

    return time_diff(tb, ta);;
}

void
malloc_cleanup()
{
}


bool
malloc_init(mod_args *ma)
{
    Payload = ma->payload;
    Size = ma->size;

    return true;
}

bool
malloc_serve(mod_args *ma)
{
    // We're superflous
    return true;
}


const net_mod mod_malloc = { "malloc()",
			     "malloc",
			     MALLOC_OPTS,
			     MALLOC_USAGE,
			     malloc_handle_arg,
			     malloc_init,
			     malloc_measure,
			     malloc_serve,
			     malloc_cleanup };
