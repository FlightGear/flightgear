/**************************************************************************
 * types.h -- various data structure definitions
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


#ifndef TYPES_H
#define TYPES_H


/* A simple cartesian point */
struct fgCartesianPoint {
    double x, y, z;
};


/* A simple geodetic point */
struct fgGeodeticPoint {
    double lon, lat, elev;
};


#endif /* TYPES_H */


/* $Log$
/* Revision 1.2  1997/07/23 21:52:12  curt
/* Put comments around the text after an #endif for increased portability.
/*
 * Revision 1.1  1997/07/07 21:03:30  curt
 * Initial revision.
 *
 */
