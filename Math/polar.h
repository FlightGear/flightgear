/**************************************************************************
 * polar.h -- routines to deal with polar math and transformations
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


#ifndef POLAR_H
#define POLAR_H


#include "../Include/types.h"


/* Convert a polar coordinate to a cartesian coordinate.  Lon and Lat
 * must be specified in radians.  The FG convention is for distances
 * to be specified in meters */
struct fgCartesianPoint fgPolarToCart(double lon, double lat, double radius);


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
void fgRotateBatchInit(double Theta, double Phi);


/* Rotates a cartesian point about the center of the earth by Theta
 * (longitude axis) and Phi (latitude axis) */
struct fgCartesianPoint fgRotateCartesianPoint(struct fgCartesianPoint p);


#endif /* POLAR_H */


/* $Log$
/* Revision 1.4  1997/12/15 23:54:55  curt
/* Add xgl wrappers for debugging.
/* Generate terrain normals on the fly.
/*
 * Revision 1.3  1997/07/31 22:52:28  curt
 * Working on redoing internal coordinate systems & scenery transformations.
 *
 * Revision 1.2  1997/07/23 21:52:21  curt
 * Put comments around the text after an #endif for increased portability.
 *
 * Revision 1.1  1997/07/07 21:02:37  curt
 * Initial revision.
 *
 */
