// scenery.cxx -- data structures and routines for managing scenery.
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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <GL/glut.h>
#include <simgear/xgl/xgl.h>

#include <stdio.h>
#include <string.h>

#include <simgear/debug/logstream.hxx>

#include <Main/options.hxx>

#include "scenery.hxx"


// Temporary hack until we get a better texture management system running
GLint area_texture;


// Shared structure to hold current scenery parameters
struct fgSCENERY scenery;


// Initialize the Scenery Management system
int fgSceneryInit( void ) {
    FG_LOG( FG_TERRAIN, FG_INFO, "Initializing scenery subsystem" );

    scenery.cur_elev = -9999;

    return 1;
}


// Tell the scenery manager where we are so it can load the proper data, and
// build the proper structures.
void fgSceneryUpdate(double lon, double lat, double elev) {
    // does nothing;
}


// Render out the current scene
void fgSceneryRender( void ) {
}


