/**************************************************************************
 * fg_random.c -- routines to handle random number generation
 *
 * Written by Curtis Olson, started July 1997.
 *
 * Copyright (C) 1997  Curtis L. Olson  - curt@infoplane.com
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id$
 * (Log is kept at end of this file)
 **************************************************************************/


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>         /* for random(), srandom() */
#include <time.h>           /* for time() to seed srandom() */        

#include <Debug/fg_debug.h>

#include "fg_random.h"

#ifndef HAVE_RAND
#  ifdef sgi
#    undef RAND_MAX
#    define RAND_MAX 2147483647
#  endif
#endif

#ifdef __SUNPRO_CC
    extern "C" {
	long int random(void);
	void srandom(unsigned int seed);
    }
#endif


/* Seed the random number generater with time() so we don't see the
 * same sequence every time */
void fg_srandom(void) {
    fgPrintf( FG_MATH, FG_INFO, "Seeding random number generater\n");

#ifdef HAVE_RAND
    srand(time(NULL));
#else
    srandom(time(NULL));
#endif                                       
}


/* return a random number between [0.0, 1.0) */
double fg_random(void) {
#ifdef HAVE_RAND
    return(rand() / (double)RAND_MAX);
#else
    return(random() / (double)RAND_MAX);
#endif
}


/* $Log$
/* Revision 1.8  1998/04/25 22:06:23  curt
/* Edited cvs log messages in source files ... bad bad bad!
/*
 * Revision 1.7  1998/04/24 00:43:13  curt
 * Wrapped "#include <config.h>" in "#ifdef HAVE_CONFIG_H"
 *
 * Revision 1.6  1998/04/18 03:53:42  curt
 * Miscellaneous Tweaks.
 *
 * Revision 1.5  1998/04/03 22:10:29  curt
 * Converting to Gnu autoconf system.
 *
 * Revision 1.4  1998/02/03 23:20:28  curt
 * Lots of little tweaks to fix various consistency problems discovered by
 * Solaris' CC.  Fixed a bug in fg_debug.c with how the fgPrintf() wrapper
 * passed arguments along to the real printf().  Also incorporated HUD changes
 * by Michele America.
 *
 * Revision 1.3  1998/01/27 00:47:59  curt
 * Incorporated Paul Bleisch's <pbleisch@acm.org> new debug message
 * system and commandline/config file processing code.
 *
 * Revision 1.2  1997/12/30 20:47:48  curt
 * Integrated new event manager with subsystem initializations.
 *
 * Revision 1.1  1997/07/30 16:04:09  curt
 * Moved random routines from Utils/ to Math/
 *
 * Revision 1.1  1997/07/19 22:31:57  curt
 * Initial revision.
 *
 */
