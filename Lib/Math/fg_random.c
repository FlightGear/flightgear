// fg_random.c -- routines to handle random number generation
//
// Written by Curtis Olson, started July 1997.
//
// Copyright (C) 1997  Curtis L. Olson  - curt@infoplane.com
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>         // for random(), srandom()
#include <time.h>           // for time() to seed srandom()        

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


// Seed the random number generater with time() so we don't see the
// same sequence every time
void fg_srandom(void) {
    // fgPrintf( FG_MATH, FG_INFO, "Seeding random number generater\n");

#ifdef HAVE_RAND
    srand(time(NULL));
#else
    srandom(time(NULL));
#endif                                       
}


// return a random number between [0.0, 1.0)
double fg_random(void) {
#ifdef HAVE_RAND
    return(rand() / (double)RAND_MAX);
#else
    return(random() / (double)RAND_MAX);
#endif
}


