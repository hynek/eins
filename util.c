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

/* ISO */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static void
assert_alloc(void *p)
{
    if (p == NULL) {
	puts("Out of memory.");
	
	exit(1);
    }
}

void *
safe_alloc(size_t size)
{
    void *p;

    p = malloc(size);
    assert_alloc(p);

    return p;
}

char *
safe_strdup(const char *s)
{
    char *p = strdup(s);
    assert_alloc(p);

    return p;
}

void
randomize_buffer(char *buf, long long size)
{
    int i;

    for (i = 0; i < size; i++) {
	buf[i] = rand();
    }
}
