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

#include "polar.h"
#include "../Include/constants.h"


/* we can save these values between calls for efficiency */
static double st, ct, sp, cp;


/* Convert a polar coordinate to a cartesian coordinate.  Lon and Lat
 * must be specified in radians.  The FG convention is for distances
 * to be specified in meters */
struct fgCartesianPoint fgPolarToCart(double lon, double lat, double radius) {
    struct fgCartesianPoint pnew;

    pnew.x = cos(lon) * cos(lat) * radius;
    pnew.y = sin(lon) * cos(lat) * radius;
    pnew.z = sin(lat) * radius;

    return(pnew);
}


/* Precalculate as much as possible so we can do a batch of transforms
 * through the same angles, will rotates a cartesian point about the
 * center of the earth by Theta (longitude axis) and Phi (latitude
 * axis) */

/* Here are the unoptimized transformation equations 

   x' = cos(Phi) * cos(Theta) * x + cos(Phi) * sin(Theta) * y + 
	     sin(Phi) * z
   y' = -sin(Theta) * x + cos(Theta) * y
   z' = -sin(Phi) * sin(Theta) * y - sin(Phi) * cos(Theta) * x + 
	     cos(Phi) * z;

 */
void fgRotateBatchInit(double Theta, double Phi) {
    printf("Theta = %.3f, Phi = %.3f\n", Theta, Phi);

    st = sin(Theta);
    ct = cos(Theta);
    sp = sin(-Phi);
    cp = cos(-Phi);
}

/* Rotates a cartesian point about the center of the earth by Theta
 * (longitude axis) and Phi (latitude axis) */
struct fgCartesianPoint fgRotateCartesianPoint(struct fgCartesianPoint p) {
    struct fgCartesianPoint p1, p2;

    /* printf("start = %.3f %.3f %.3f\n", p.x, p.y, p.z); */

    /* rotate about the z axis */
    p1.x = ct * p.x - st * p.y;
    p1.y = st * p.x + ct * p.y;
    p1.z = p.z;

    /* printf("step 1 = %.3f %.3f %.3f\n", p1.x, p1.y, p1.z); */

    /* rotate new point about y axis */
    p2.x = cp * p1.x + sp * p1.z;
    p2.y = p1.y;
    p2.z = cp * p1.z - sp * p1.x;

    /* printf("cp = %.5f, sp = %.5f\n", cp, sp); */
    /* printf("(1) = %.5f, (2) = %.5f\n", cp * p1.z, sp * p1.x); */

    /* printf("step 2 = %.3f %.3f %.3f\n", p2.x, p2.y, p2.z); */

    return(p2);
}


/* $Log$
/* Revision 1.3  1997/12/15 23:54:54  curt
/* Add xgl wrappers for debugging.
/* Generate terrain normals on the fly.
/*
 * Revision 1.2  1997/07/31 22:52:27  curt
 * Working on redoing internal coordinate systems & scenery transformations.
 *
 * Revision 1.1  1997/07/07 21:02:36  curt
 * Initial revision.
 * */
