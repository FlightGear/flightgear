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


#ifndef _FG_TYPES_H
#define _FG_TYPES_H


/* A simple 3d cartesian point */
typedef struct {
    union {
	double x;
	double lon;
    };
    union {
	double y;
	double lat;
    };
    union {
	double z;
	double radius;
    };
} fgPoint3d;


/* A simple 3d polar point */
typedef struct {
    double lon, lat, radius;
} fgPolarPoint3dOld;


/* A simple geodetic point */
typedef struct {
    double lon, lat, elev;
} fgGeodeticPoint3d;


#endif /* _FG_TYPES_H */


/* $Log$
/* Revision 1.4  1998/07/08 14:36:29  curt
/* Changed name of EQUATORIAL_RADIUS_KM and RESQ_KM to "M" since they were
/* in meters anyways.
/*
/* Unified fgCartesianPoint3d and fgPolarPoint3d in a single struct called
/* fgPoint3d.
/*
 * Revision 1.3  1998/05/02 01:48:39  curt
 * typedef-ified fgCartesianPoint3d
 *
 * Revision 1.2  1998/04/08 23:35:33  curt
 * Tweaks to Gnu automake/autoconf system.
 *
 * Revision 1.1  1998/01/27 00:46:51  curt
 * prepended "fg_" on the front of these to avoid potential conflicts with
 * system include files.
 *
 * Revision 1.2  1998/01/22 02:59:36  curt
 * Changed #ifdef FILE_H to #ifdef _FILE_H
 *
 * Revision 1.1  1997/12/15 21:02:17  curt
 * Moved to .../FlightGear/Src/Include/
 *
 * Revision 1.2  1997/07/23 21:52:12  curt
 * Put comments around the text after an #endif for increased portability.
 *
 * Revision 1.1  1997/07/07 21:03:30  curt
 * Initial revision.
 *
 */
