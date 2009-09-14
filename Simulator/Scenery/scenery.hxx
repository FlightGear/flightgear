// scenery.hxx -- data structures and routines for managing scenery.
//
// Written by Curtis Olson, started May 1997.
//
// Copyright (C) 1997  Curtis L. Olson  - curt@infoplane.com
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$
// (Log is kept at end of this file)


#ifndef _SCENERY_HXX
#define _SCENERY_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#include <Math/point3d.hxx>


// Define a structure containing global scenery parameters
struct fgSCENERY {
    // center of current scenery chunk
    Point3D center;

    // next center of current scenery chunk
    Point3D next_center;

    // angle of sun relative to current local horizontal
    double sun_angle;

    // elevation of terrain at our current lat/lon (based on the
    // actual drawn polygons)
    double cur_elev;
};

extern struct fgSCENERY scenery;


// Initialize the Scenery Management system
int fgSceneryInit( void );


// Tell the scenery manager where we are so it can load the proper
// data, and build the proper structures.
void fgSceneryUpdate(double lon, double lat, double elev);


// Render out the current scene
void fgSceneryRender( void );


#endif // _SCENERY_HXX


// $Log$
// Revision 1.6  1998/10/16 23:27:59  curt
// C++-ifying.
//
// Revision 1.5  1998/10/16 00:55:44  curt
// Converted to Point3D class.
//
// Revision 1.4  1998/07/12 03:18:28  curt
// Added ground collision detection.  This involved:
// - saving the entire vertex list for each tile with the tile records.
// - saving the face list for each fragment with the fragment records.
// - code to intersect the current vertical line with the proper face in
//   an efficient manner as possible.
// Fixed a bug where the tiles weren't being shifted to "near" (0,0,0)
//
// Revision 1.3  1998/07/08 14:47:22  curt
// Fix GL_MODULATE vs. GL_DECAL problem introduced by splash screen.
// polare3d.h renamed to polar3d.hxx
// fg{Cartesian,Polar}Point3d consolodated.
// Added some initial support for calculating local current ground elevation.
//
// Revision 1.2  1998/05/02 01:52:16  curt
// Playing around with texture coordinates.
//
// Revision 1.1  1998/04/30 12:35:31  curt
// Added a command line rendering option specify smooth/flat shading.
//
// Revision 1.21  1998/04/25 22:06:31  curt
// Edited cvs log messages in source files ... bad bad bad!
//
// Revision 1.20  1998/04/22 13:22:45  curt
// C++ - ifing the code a bit.
//
// Revision 1.19  1998/04/21 17:02:43  curt
// Prepairing for C++ integration.
//
// Revision 1.18  1998/03/14 00:30:51  curt
// Beginning initial terrain texturing experiments.
//
// Revision 1.17  1998/02/20 00:16:24  curt
// Thursday's tweaks.
//
// Revision 1.16  1998/01/27 00:48:03  curt
// Incorporated Paul Bleisch's <pbleisch@acm.org> new debug message
// system and commandline/config file processing code.
//
// Revision 1.15  1998/01/22 02:59:41  curt
// Changed #ifdef FILE_H to #ifdef _FILE_H
//
// Revision 1.14  1998/01/19 19:27:17  curt
// Merged in make system changes from Bob Kuehne <rpk@sgi.com>
// This should simplify things tremendously.
//
// Revision 1.13  1998/01/19 18:40:38  curt
// Tons of little changes to clean up the code and to remove fatal errors
// when building with the c++ compiler.
//
// Revision 1.12  1997/12/15 23:55:03  curt
// Add xgl wrappers for debugging.
// Generate terrain normals on the fly.
//
// Revision 1.11  1997/12/10 22:37:52  curt
// Prepended "fg" on the name of all global structures that didn't have it yet.
// i.e. "struct WEATHER {}" became "struct fgWEATHER {}"
//
// Revision 1.10  1997/09/04 02:17:37  curt
// Shufflin' stuff.
//
// Revision 1.9  1997/08/27 03:30:33  curt
// Changed naming scheme of basic shared structures.
//
// Revision 1.8  1997/08/06 00:24:30  curt
// Working on correct real time sun lighting.
//
// Revision 1.7  1997/07/23 21:52:27  curt
// Put comments around the text after an #endif for increased portability.
//
// Revision 1.6  1997/07/11 01:30:03  curt
// More tweaking of terrian floor.
//
// Revision 1.5  1997/06/26 22:14:57  curt
// Beginning work on a scenery management system.
//
// Revision 1.4  1997/06/21 17:57:21  curt
// directory shuffling ...
//
// Revision 1.2  1997/05/23 15:40:43  curt
// Added GNU copyright headers.
//
// Revision 1.1  1997/05/16 16:07:07  curt
// Initial revision.
//

