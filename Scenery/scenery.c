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


#ifdef WIN32
#  include <windows.h>
#endif

#include <GL/glut.h>
#include "../XGL/xgl.h"

#include <stdio.h>
#include <string.h>

#include "../Include/general.h"

#include "obj.h"
#include "scenery.h"


/* Temporary hack until we get the scenery management system running */
GLint area_terrain;


/* Shared structure to hold current scenery parameters */
struct fgSCENERY scenery;


/* Initialize the Scenery Management system */
void fgSceneryInit( void ) {
    printf("Initializing scenery subsystem\n");

    /* set the default terrain detail level */
    scenery.terrain_skip = 6;
}


/* Tell the scenery manager where we are so it can load the proper data, and
 * build the proper structures. */
void fgSceneryUpdate(double lon, double lat, double elev) {
    struct fgGENERAL *g;
    char path[1024];

    g = &general;

    /* a hardcoded hack follows */

    /* this routine should parse the file, and make calls back to the
     * scenery management system to build the appropriate structures */
    path[0] = '\0';
    strcat(path, g->root_dir);
    strcat(path, "/Scenery/");
    strcat(path, "mesa-e.obj");

    printf("  Loading Scenery: %s\n", path);

    area_terrain = fgObjLoad(path, &scenery.center);
}


/* Render out the current scene */
void fgSceneryRender( void ) {
    static GLfloat terrain_color[4] = { 0.6, 0.8, 0.4, 1.0 };
    static GLfloat terrain_ambient[4];
    static GLfloat terrain_diffuse[4];
    int i;

    for ( i = 0; i < 4; i++ ) {
	terrain_ambient[i] = terrain_color[i] * 0.5;
	terrain_diffuse[i] = terrain_color[i];
    }

    xglMaterialfv(GL_FRONT, GL_AMBIENT, terrain_ambient);
    xglMaterialfv(GL_FRONT, GL_DIFFUSE, terrain_diffuse);

    xglCallList(area_terrain);
}


/* $Log$
/* Revision 1.32  1998/01/19 18:40:37  curt
/* Tons of little changes to clean up the code and to remove fatal errors
/* when building with the c++ compiler.
/*
 * Revision 1.31  1998/01/13 00:23:11  curt
 * Initial changes to support loading and management of scenery tiles.  Note,
 * there's still a fair amount of work left to be done.
 *
 * Revision 1.30  1998/01/07 03:22:29  curt
 * Moved astro stuff to .../Src/Astro/
 *
 * Revision 1.29  1997/12/30 20:47:52  curt
 * Integrated new event manager with subsystem initializations.
 *
 * Revision 1.28  1997/12/15 23:55:02  curt
 * Add xgl wrappers for debugging.
 * Generate terrain normals on the fly.
 *
 * Revision 1.27  1997/12/12 21:41:30  curt
 * More light/material property tweaking ... still a ways off.
 *
 * Revision 1.26  1997/12/12 19:52:58  curt
 * Working on lightling and material properties.
 *
 * Revision 1.25  1997/12/10 22:37:51  curt
 * Prepended "fg" on the name of all global structures that didn't have it yet.
 * i.e. "struct WEATHER {}" became "struct fgWEATHER {}"
 *
 * Revision 1.24  1997/12/08 22:51:18  curt
 * Enhanced to handle ccw and cw tri-stripe winding.  This is a temporary
 * admission of defeat.  I will eventually go back and get all the stripes
 * wound the same way (ccw).
 *
 * Revision 1.23  1997/11/25 19:25:37  curt
 * Changes to integrate Durk's moon/sun code updates + clean up.
 *
 * Revision 1.22  1997/10/28 21:00:22  curt
 * Changing to new terrain format.
 *
 * Revision 1.21  1997/10/25 03:24:24  curt
 * Incorporated sun, moon, and star positioning code contributed by Durk Talsma.
 *
 * Revision 1.20  1997/10/25 03:18:27  curt
 * Incorporated sun, moon, and planet position and rendering code contributed
 * by Durk Talsma.
 *
 * Revision 1.19  1997/09/05 14:17:30  curt
 * More tweaking with stars.
 *
 * Revision 1.18  1997/09/05 01:35:59  curt
 * Working on getting stars right.
 *
 * Revision 1.17  1997/08/29 17:55:27  curt
 * Worked on properly aligning the stars.
 *
 * Revision 1.16  1997/08/27 21:32:29  curt
 * Restructured view calculation code.  Added stars.
 *
 * Revision 1.15  1997/08/27 03:30:32  curt
 * Changed naming scheme of basic shared structures.
 *
 * Revision 1.14  1997/08/25 20:27:24  curt
 * Merged in initial HUD and Joystick code.
 *
 * Revision 1.13  1997/08/22 21:34:41  curt
 * Doing a bit of reorganizing and house cleaning.
 *
 * Revision 1.12  1997/08/19 23:55:08  curt
 * Worked on better simulating real lighting.
 *
 * Revision 1.11  1997/08/13 20:24:22  curt
 * Changed default detail level.
 *
 * Revision 1.10  1997/08/06 00:24:30  curt
 * Working on correct real time sun lighting.
 *
 * Revision 1.9  1997/08/04 17:08:11  curt
 * Testing cvs on IRIX 6.x
 *
 * Revision 1.8  1997/07/18 23:41:27  curt
 * Tweaks for building with Cygnus Win32 compiler.
 *
 * Revision 1.7  1997/07/16 20:04:52  curt
 * Minor tweaks to aid Win32 port.
 *
 * Revision 1.6  1997/07/14 16:26:05  curt
 * Testing/playing -- placed objects randomly across the entire terrain.
 *
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
