/**************************************************************************
 * scenery.c -- data structures and routines for managing scenery.
 *
 * Written by Curtis Olson, started May 1997.
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


#include <GL/glut.h>

#include "scenery.h"
#include "parsevrml.h"


/* Temporary hack until we get the scenery management system running */
GLint mesh_hack;


/* Shared structure to hold current scenery parameters */
struct scenery_params cur_scenery_params;


/* Initialize the Scenery Management system */
void fgSceneryInit() {
    /* set the default terrain detail level */
    cur_scenery_params.terrain_skip = 5;
}


/* Tell the scenery manager where we are so it can load the proper data, and
 * build the proper structures. */
void fgSceneryUpdate(double lon, double lat, double elev) {
    /* a hardcoded hack follows */

    /* this routine should parse the file, and make calls back to the
     * scenery management system to build the appropriate structures */
    fgParseVRML("mesa-e.wrl");
}


/* Render out the current scene */
void fgSceneryRender() {
    glPushMatrix();
    glCallList(mesh_hack);
    glPopMatrix();
}


/* $Log$
/* Revision 1.6  1997/07/14 16:26:05  curt
/* Testing/playing -- placed objects randomly across the entire terrain.
/*
 * Revision 1.5  1997/07/11 03:23:19  curt
 * Solved some scenery display/orientation problems.  Still have a positioning
 * (or transformation?) problem.
 *
 * Revision 1.4  1997/07/11 01:30:03  curt
 * More tweaking of terrian floor.
 *
 * Revision 1.3  1997/06/29 21:16:50  curt
 * More twiddling with the Scenery Management system.
 *
 * Revision 1.2  1997/06/27 20:03:37  curt
 * Working on Makefile structure.
 *
 * Revision 1.1  1997/06/27 02:26:30  curt
 * Initial revision.
 *
 */
