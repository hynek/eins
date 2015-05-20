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

#ifndef MODULES_H
#define MODULES_H

#include "mods.h"

#include "mod_tcp.h"
#include "mod_accept.h"
#include "mod_udp.h"
#include "mod_malloc.h"
#ifdef WITH_BMI
#include "mod_bmi.h"
#endif // WITH_BMI
#ifdef WITH_NEON
#include "mod_neon.h"
#endif // WITH_NEON
#ifdef WITH_SCTP
#include "mod_sctp.h"
#endif // WITH_SCTP

const net_mod *Modules[] = { &mod_tcp,
			     &mod_udp,
			     &mod_accept,
			     &mod_malloc,
#ifdef WITH_BMI
			     &mod_bmi,
#endif // WITH_BMI
#ifdef WITH_NEON
			     &mod_neon,
#endif // WITH_NEON
#ifdef WITH_SCTP
			     &mod_sctp,
#endif // WITH_SCTP
			     NULL };

#endif // MODULES_H
