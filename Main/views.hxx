/**************************************************************************
 * views.hxx -- data structures and routines for managing and view parameters.
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


#ifndef _VIEWS_HXX
#define _VIEWS_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#include <Include/fg_types.h>
#include <Flight/flight.h>
#include <Math/mat3.h>
#include <Time/fg_time.hxx>
#include <Time/light.hxx>


/* Define a structure containing view information */
struct fgVIEW {
    /* absolute view position */
    struct fgCartesianPoint abs_view_pos;

    /* view position translated to scenery.center */
    struct fgCartesianPoint view_pos;

    /* cartesion coordinates of current lon/lat if at sea level
     * translated to scenery.center*/
    struct fgCartesianPoint cur_zero_elev;

    /* vector in cartesian coordinates from current position to the
     * postion on the earth's surface the sun is directly over */
    MAT3vec to_sun;
    
    /* surface direction to go to head towards sun */
    MAT3vec surface_to_sun;

    /* surface vector heading south */
    MAT3vec surface_south;

    /* surface vector heading east (used to unambiguously align sky with sun) */
    MAT3vec surface_east;

    /* local up vector (normal to the plane tangent to the earth's
     * surface at the spot we are directly above */
    MAT3vec local_up;

    /* up vector for the view (usually point straight up through the
     * top of the aircraft */
    MAT3vec view_up;

    /* the vector pointing straight out the nose of the aircraft */
    MAT3vec view_forward;

    /* the current offset from forward for viewing */
    double view_offset;

    /* the goal view offset for viewing (used for smooth view changes) */
    double goal_view_offset;
};


extern struct fgVIEW current_view;


/* Initialize a view structure */
void fgViewInit(struct fgVIEW *v);

/* Update the view parameters */
void fgViewUpdate(fgFLIGHT *f, struct fgVIEW *v, fgLIGHT *l);


#endif /* _VIEWS_HXX */


/* $Log$
/* Revision 1.3  1998/04/25 22:06:31  curt
/* Edited cvs log messages in source files ... bad bad bad!
/*
 * Revision 1.2  1998/04/24 00:49:22  curt
 * Wrapped "#include <config.h>" in "#ifdef HAVE_CONFIG_H"
 * Trying out some different option parsing code.
 * Some code reorganization.
 *
 * Revision 1.1  1998/04/22 13:25:46  curt
 * C++ - ifing the code.
 * Starting a bit of reorganization of lighting code.
 *
 * Revision 1.11  1998/04/21 17:02:42  curt
 * Prepairing for C++ integration.
 *
 * Revision 1.10  1998/02/07 15:29:45  curt
 * Incorporated HUD changes and struct/typedef changes from Charlie Hotchkiss
 * <chotchkiss@namg.us.anritsu.com>
 *
 * Revision 1.9  1998/01/29 00:50:29  curt
 * Added a view record field for absolute x, y, z position.
 *
 * Revision 1.8  1998/01/27 00:47:58  curt
 * Incorporated Paul Bleisch's <pbleisch@acm.org> new debug message
 * system and commandline/config file processing code.
 *
 * Revision 1.7  1998/01/22 02:59:38  curt
 * Changed #ifdef FILE_H to #ifdef _FILE_H
 *
 * Revision 1.6  1998/01/19 19:27:10  curt
 * Merged in make system changes from Bob Kuehne <rpk@sgi.com>
 * This should simplify things tremendously.
 *
 * Revision 1.5  1997/12/22 04:14:32  curt
 * Aligned sky with sun so dusk/dawn effects can be correct relative to the sun.
 *
 * Revision 1.4  1997/12/17 23:13:36  curt
 * Began working on rendering a sky.
 *
 * Revision 1.3  1997/12/15 23:54:51  curt
 * Add xgl wrappers for debugging.
 * Generate terrain normals on the fly.
 *
 * Revision 1.2  1997/12/10 22:37:48  curt
 * Prepended "fg" on the name of all global structures that didn't have it yet.
 * i.e. "struct WEATHER {}" became "struct fgWEATHER {}"
 *
 * Revision 1.1  1997/08/27 21:31:18  curt
 * Initial revision.
 *
 */
