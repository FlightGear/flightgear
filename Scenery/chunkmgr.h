/**************************************************************************
 * chunkmgr.h -- top level scenery chunk management system.
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


#ifndef CHUNKMGR_H
#define CHUNKMGR_H


#include "mesh.h"


#define MAX_CHUNK_LIST  1000
#define MAX_CHUNK_CACHE 100


struct chunk {
    /* path name to the data file for this chunk */
    char path[1024];

    /* coordinates (in arc seconds) */
    double lon, lat;

    /* coordinates (cartesian, units = km) */
    double x, y, z;

    /* last calculated distance from current viewpoint */
    double dist;

    /* pointer to elevation grid array for this chunk*/
    struct mesh *terrain_ptr;
};


/* initialize the chunk management system */
void chunk_init();


/* load all chunks within radius distance of the specified point if
 * they are not already loaded or cached.  Remove or cache any chunks that are
 * out of range. */
void chunk_update(double lat, double lon, double radius);


#endif /* CHUNKMGR_H */


/* $Log$
/* Revision 1.1  1997/07/31 23:19:34  curt
/* Initial revision.
/*
 */
