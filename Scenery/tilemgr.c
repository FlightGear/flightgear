/**************************************************************************
 * tilemgr.c -- routines to handle dynamic management of scenery tiles
 *
 * Written by Curtis Olson, started January 1998.
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


#ifdef WIN32
#  include <windows.h>
#endif

#include <GL/glut.h>
#include "../XGL/xgl.h"

#include "tileutils.h"

#include "../Aircraft/aircraft.h"


/* here's where we keep the array of closest (potentially viewable) tiles */
static long int tile[49];


/* Initialize the Tile Manager subsystem */
void fgTileMgrInit() {
    printf("Initializing Tile Manager subsystem.\n");
    /* fgTileCacheInit(); */
}


/* given the current lon/lat, fill in the array of local chunks.  If
 * the chunk isn't already in the cache, then read it from disk. */
void fgTileMgrUpdate() {
    struct fgFLIGHT *f;
    struct bucket p;

    f = &current_aircraft.flight;

    find_bucket(FG_Longitude, FG_Latitude, &p);
    gen_idx_array(&p, tile, 7, 7);
}


/* $Log$
/* Revision 1.1  1998/01/07 23:50:51  curt
/* "area" renamed to "tile"
/*
 * Revision 1.2  1998/01/07 03:29:29  curt
 * Given an arbitrary lat/lon, we can now:
 *   generate a unique index for the chunk containing the lat/lon
 *   generate a path name to the chunk file
 *   build a list of the indexes of all the nearby areas.
 *
 * Revision 1.1  1998/01/07 02:05:48  curt
 * Initial revision.
 * */


