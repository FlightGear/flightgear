//
// light.hxx -- lighting routines
//
// Written by Curtis Olson, started April 1998.
//
// Copyright (C) 1998  Curtis L. Olson  - curt@me.umn.edu
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
#include <XGL/xgl.h>

#include <string.h>

#include <Debug/fg_debug.h>
#include <Include/fg_constants.h>
#include <Main/options.hxx>
#include <Main/views.hxx>
#include <Math/fg_geodesy.h>
#include <Math/interpolater.hxx>
#include <Math/mat3.h>
#include <Math/polar3d.hxx>

#include "fg_time.hxx"
#include "light.hxx"
#include "sunpos.hxx"


fgLIGHT cur_light_params;


// Constructor
fgLIGHT::fgLIGHT( void ) {
}


// initialize lighting tables
void fgLIGHT::Init( void ) {
    char path[256];

    fgPrintf( FG_EVENT, FG_INFO, 
	     "Initializing Lighting interpolation tables.\n" );

    // build the path name to the ambient lookup table
    current_options.get_fg_root(path);
    strcat(path, "/Scenery/");
    strcat(path, "Ambient");
    // initialize ambient table
    ambient_tbl = new fgINTERPTABLE(path);

    // build the path name to the diffuse lookup table
    current_options.get_fg_root(path);
    strcat(path, "/Scenery/");
    strcat(path, "Diffuse");
    // initialize diffuse table
    diffuse_tbl = new fgINTERPTABLE(path);
    
    // build the path name to the sky lookup table
    current_options.get_fg_root(path);
    strcat(path, "/Scenery/");
    strcat(path, "Sky");
    // initialize sky table
    sky_tbl = new fgINTERPTABLE(path);
}


// update lighting parameters based on current sun position
void fgLIGHT::Update( void ) {
    fgTIME *t;
    fgVIEW *v;
    // if the 4th field is 0.0, this specifies a direction ...
    GLfloat white[4] = { 1.0, 1.0, 1.0, 1.0 };
    // base sky color
    GLfloat base_sky_color[4] =        {0.60, 0.60, 0.90, 1.0};
    // base fog color
    GLfloat base_fog_color[4] = {0.90, 0.90, 1.00, 1.0};
    double deg, ambient, diffuse, sky_brightness;

    t = &cur_time_params;
    v = &current_view;

    fgPrintf( FG_EVENT, FG_INFO, "Updating light parameters.\n" );

    // calculate lighting parameters based on sun's relative angle to
    // local up

    deg = sun_angle * 180.0 / FG_PI;
    fgPrintf( FG_EVENT, FG_INFO, "  Sun angle = %.2f.\n", deg );

    ambient = ambient_tbl->interpolate( deg );
    diffuse = diffuse_tbl->interpolate( deg );
    sky_brightness = sky_tbl->interpolate( deg );

    fgPrintf( FG_EVENT, FG_INFO, 
	      "  ambient = %.2f  diffuse = %.2f  sky = %.2f\n", 
	      ambient, diffuse, sky_brightness );

    // sky_brightness = 0.15;  // used to force a dark sky (when testing)

    // if ( ambient < 0.02 ) { ambient = 0.02; }
    // if ( diffuse < 0.0 ) { diffuse = 0.0; }
    // if ( sky_brightness < 0.1 ) { sky_brightness = 0.1; }

    scene_ambient[0] = white[0] * ambient;
    scene_ambient[1] = white[1] * ambient;
    scene_ambient[2] = white[2] * ambient;

    scene_diffuse[0] = white[0] * diffuse;
    scene_diffuse[1] = white[1] * diffuse;
    scene_diffuse[2] = white[2] * diffuse;

    // set fog color
    fog_color[0] = base_fog_color[0] * sky_brightness;
    fog_color[1] = base_fog_color[1] * sky_brightness;
    fog_color[2] = base_fog_color[2] * sky_brightness;
    fog_color[3] = base_fog_color[3];

    // set sky color
    sky_color[0] = base_sky_color[0] * sky_brightness;
    sky_color[1] = base_sky_color[1] * sky_brightness;
    sky_color[2] = base_sky_color[2] * sky_brightness;
    sky_color[3] = base_sky_color[3];
}


// Destructor
fgLIGHT::~fgLIGHT( void ) {
}


// wrapper function for updating light parameters via the event scheduler
void fgLightUpdate ( void ) {
    fgLIGHT *l;
    l = &cur_light_params;
   
    l->Update();
}


// $Log$
// Revision 1.11  1998/07/13 21:02:08  curt
// Wrote access functions for current fgOPTIONS.
//
// Revision 1.10  1998/07/08 14:48:38  curt
// polar3d.h renamed to polar3d.hxx
//
// Revision 1.9  1998/05/29 20:37:51  curt
// Renamed <Table>.table to be <Table> so we can add a .gz under DOS.
//
// Revision 1.8  1998/05/20 20:54:16  curt
// Converted fgLIGHT to a C++ class.
//
// Revision 1.7  1998/05/13 18:26:50  curt
// Root path info moved to fgOPTIONS.
//
// Revision 1.6  1998/05/11 18:18:51  curt
// Made fog color slightly bluish.
//
// Revision 1.5  1998/05/03 00:48:38  curt
// polar.h -> polar3d.h
//
// Revision 1.4  1998/04/28 01:22:18  curt
// Type-ified fgTIME and fgVIEW.
//
// Revision 1.3  1998/04/26 05:10:04  curt
// "struct fgLIGHT" -> "fgLIGHT" because fgLIGHT is typedef'd.
//
// Revision 1.2  1998/04/24 00:52:30  curt
// Wrapped "#include <config.h>" in "#ifdef HAVE_CONFIG_H"
// Fog color fixes.
// Separated out lighting calcs into their own file.
//
// Revision 1.1  1998/04/22 13:24:06  curt
// C++ - ifiing the code a bit.
// Starting to reorginize some of the lighting calcs to use a table lookup.
//
