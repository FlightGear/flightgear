/**************************************************************************
 * fg_time.h -- data structures and routines for managing time related stuff.
 *
 * Written by Curtis Olson, started August 1997.
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


#ifndef FG_TIME_H
#define FG_TIME_H


#include "../types.h"


/* Define a structure containing global time parameters */
struct fgTIME {
    /* the point on the earth's surface above which the sun is directly 
     * overhead */
    struct fgCartesianPoint fg_sunpos;  /* in cartesian coordiantes */
    double sun_lon, sun_gc_lat;         /* in geocentric coordinates */
};

extern struct fgTIME cur_time_params;


#endif /* FG_TIME_H */


/* $Log$
/* Revision 1.2  1997/08/27 03:30:36  curt
/* Changed naming scheme of basic shared structures.
/*
 * Revision 1.1  1997/08/13 21:56:00  curt
 * Initial revision.
 *
 */
