/**************************************************************************
 * scenery.h -- data structures and routines for managing scenery.
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


#ifndef SCENERY_H
#define SCENERY_H


#include "../types.h"


/* Define a structure containing global scenery parameters */
struct SCENERY {
    /* number of terrain data points to skip */
    int terrain_skip;

    /* center of current scenery chunk */
    struct fgCartesianPoint center;

    /* angle of sun relative to current local horizontal */
    double sun_angle;
};

extern struct SCENERY scenery;


/* Initialize the Scenery Management system */
void fgSceneryInit();


/* Tell the scenery manager where we are so it can load the proper data, and
 * build the proper structures. */
void fgSceneryUpdate(double lon, double lat, double elev);


/* Render out the current scene */
void fgSceneryRender();


#endif /* SCENERY_H */


/* $Log$
/* Revision 1.10  1997/09/04 02:17:37  curt
/* Shufflin' stuff.
/*
 * Revision 1.9  1997/08/27 03:30:33  curt
 * Changed naming scheme of basic shared structures.
 *
 * Revision 1.8  1997/08/06 00:24:30  curt
 * Working on correct real time sun lighting.
 *
 * Revision 1.7  1997/07/23 21:52:27  curt
 * Put comments around the text after an #endif for increased portability.
 *
 * Revision 1.6  1997/07/11 01:30:03  curt
 * More tweaking of terrian floor.
 *
 * Revision 1.5  1997/06/26 22:14:57  curt
 * Beginning work on a scenery management system.
 *
 * Revision 1.4  1997/06/21 17:57:21  curt
 * directory shuffling ...
 *
 * Revision 1.2  1997/05/23 15:40:43  curt
 * Added GNU copyright headers.
 *
 * Revision 1.1  1997/05/16 16:07:07  curt
 * Initial revision.
 *
 */
