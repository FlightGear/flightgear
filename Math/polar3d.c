/**************************************************************************
 * polar.c -- routines to deal with polar math and transformations
 *
 * Written by Curtis Olson, started June 1997.
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


#include <math.h>
#include <stdio.h>

#include <Include/fg_constants.h>

#include "polar3d.h"


/* Convert a polar coordinate to a cartesian coordinate.  Lon and Lat
 * must be specified in radians.  The FG convention is for distances
 * to be specified in meters */
fgCartesianPoint3d fgPolarToCart3d(fgPolarPoint3d p) {
    fgCartesianPoint3d pnew;

    pnew.x = cos(p.lon) * cos(p.lat) * p.radius;
    pnew.y = sin(p.lon) * cos(p.lat) * p.radius;
    pnew.z = sin(p.lat) * p.radius;

    return(pnew);
}


/* Convert a cartesian coordinate to polar coordinates (lon/lat
 * specified in radians.  Distances are specified in meters. */
fgPolarPoint3d fgCartToPolar3d(fgCartesianPoint3d cp) {
    fgPolarPoint3d pp;

    pp.lon = atan2( cp.y, cp.x );
    pp.lat = FG_PI_2 - atan2( sqrt(cp.x*cp.x + cp.y*cp.y), cp.z );
    pp.radius = sqrt(cp.x*cp.x + cp.y*cp.y + cp.z*cp.z);

    /* printf("lon = %.2f  lat = %.2f  radius = %.2f\n", 
              pp.lon, pp.lat, pp.radius); */
    return(pp);
}


/* $Log$
/* Revision 1.2  1998/05/03 00:45:49  curt
/* Commented out a debugging printf.
/*
 * Revision 1.1  1998/05/02 01:50:11  curt
 * polar.[ch] renamed to polar3d.[ch]
 *
 * Revision 1.6  1998/04/25 22:06:23  curt
 * Edited cvs log messages in source files ... bad bad bad!
 *
 * Revision 1.5  1998/01/27 00:48:00  curt
 * Incorporated Paul Bleisch's <pbleisch@acm.org> new debug message
 * system and commandline/config file processing code.
 *
 * Revision 1.4  1998/01/19 19:27:12  curt
 * Merged in make system changes from Bob Kuehne <rpk@sgi.com>
 * This should simplify things tremendously.
 *
 * Revision 1.3  1997/12/15 23:54:54  curt
 * Add xgl wrappers for debugging.
 * Generate terrain normals on the fly.
 *
 * Revision 1.2  1997/07/31 22:52:27  curt
 * Working on redoing internal coordinate systems & scenery transformations.
 *
 * Revision 1.1  1997/07/07 21:02:36  curt
 * Initial revision.
 * */
