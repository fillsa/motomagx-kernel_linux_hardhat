/*
 *	Fusion Kernel Module
 *
 *	(c) Copyright 2002  Convergence GmbH
 *
 *      Written by Denis Oliver Kropp <dok@directfb.org>
 *
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 */

#ifndef __FUSION__TYPES_H__
#define __FUSION__TYPES_H__

#ifdef bool
# undef bool
#endif

#ifdef false
# undef false
#endif

#ifdef true
# undef true
#endif

typedef enum {
	false = 0,
	true = !false
} bool;

#endif
