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

#include "Include/compiler.h"

#ifdef FG_MATH_EXCEPTION_CLASH
#  define exception c_exception
#endif

#ifdef FG_HAVE_STD_INCLUDES
#  include <cmath>
#else
#  include <math.h>
#endif

#include <string>
FG_USING_STD(string);

#include <Aircraft/aircraft.hxx>
#include <Debug/logstream.hxx>
#include <Include/fg_constants.h>
#include <Main/options.hxx>
#include <Main/views.hxx>
#include <Math/fg_geodesy.hxx>
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
    FG_LOG( FG_EVENT, FG_INFO, 
	    "Initializing Lighting interpolation tables." );

    // build the path name to the ambient lookup table
    string path = current_options.get_fg_root();
    string ambient = path + "/Lighting/ambient";
    string diffuse = path + "/Lighting/diffuse";
    string sky     = path + "/Lighting/sky";

    // initialize ambient table
    ambient_tbl = new fgINTERPTABLE( ambient );

    // initialize diffuse table
    diffuse_tbl = new fgINTERPTABLE( diffuse );
    
    // initialize sky table
    sky_tbl = new fgINTERPTABLE( sky );
}


// update lighting parameters based on current sun position
void fgLIGHT::Update( void ) {
    FGInterface *f;
    fgTIME *t;
    // if the 4th field is 0.0, this specifies a direction ...
    GLfloat white[4] = { 1.0, 1.0, 1.0, 1.0 };
    // base sky color
    GLfloat base_sky_color[4] =        {0.60, 0.60, 0.90, 1.0};
    // base fog color
    GLfloat base_fog_color[4] = {0.90, 0.90, 1.00, 1.0};
    double deg, ambient, diffuse, sky_brightness;

    f = current_aircraft.fdm_state;
    t = &cur_time_params;

    FG_LOG( FG_EVENT, FG_INFO, "Updating light parameters." );

    // calculate lighting parameters based on sun's relative angle to
    // local up

    deg = sun_angle * 180.0 / FG_PI;
    FG_LOG( FG_EVENT, FG_INFO, "  Sun angle = " << deg );

    ambient = ambient_tbl->interpolate( deg );
    diffuse = diffuse_tbl->interpolate( deg );
    sky_brightness = sky_tbl->interpolate( deg );

    FG_LOG( FG_EVENT, FG_INFO, 
	    "  ambient = " << ambient << "  diffuse = " << diffuse 
	    << "  sky = " << sky_brightness );

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

    // set sky color
    sky_color[0] = base_sky_color[0] * sky_brightness;
    sky_color[1] = base_sky_color[1] * sky_brightness;
    sky_color[2] = base_sky_color[2] * sky_brightness;
    sky_color[3] = base_sky_color[3];

    // set fog color
    fog_color[0] = base_fog_color[0] * sky_brightness;
    fog_color[1] = base_fog_color[1] * sky_brightness;
    fog_color[2] = base_fog_color[2] * sky_brightness;
    fog_color[3] = base_fog_color[3];
}


// calculate fog color adjusted for sunrise/sunset effects
void fgLIGHT::UpdateAdjFog( void ) {
    FGInterface *f;
    double sun_angle_deg, rotation, param1[3], param2[3];

    f = current_aircraft.fdm_state;

    FG_LOG( FG_EVENT, FG_DEBUG, "Updating adjusted fog parameters." );

    // set fog color (we'll try to match the sunset color in the
    // direction we are looking

    // first determine the difference between our view angle and local
    // direction to the sun
    rotation = -(sun_rotation + FG_PI) 
	- (f->get_Psi() - current_view.get_view_offset());
    while ( rotation < 0 ) {
	rotation += FG_2PI;
    }
    while ( rotation > FG_2PI ) {
	rotation -= FG_2PI;
    }
    rotation *= RAD_TO_DEG;
    // fgPrintf( FG_EVENT, FG_INFO, 
    //           "  View to sun difference in degrees = %.2f\n", rotation);

    // next check if we are in a sunset/sunrise situation
    sun_angle_deg = sun_angle * RAD_TO_DEG;
    if ( (sun_angle_deg > 80.0) && (sun_angle_deg < 100.0) ) {
	/* 0.0 - 0.6 */
	param1[0] = (10.0 - fabs(90.0 - sun_angle_deg)) / 20.0;
	param1[1] = (10.0 - fabs(90.0 - sun_angle_deg)) / 40.0;
	param1[2] = (10.0 - fabs(90.0 - sun_angle_deg)) / 30.0;
	// param2[2] = -(10.0 - fabs(90.0 - sun_angle)) / 30.0;
    } else {
	param1[0] = param1[1] = param1[2] = 0.0;
    }

    if ( rotation - 180.0 <= 0.0 ) {
	param2[0] = param1[0] * (180.0 - rotation) / 180.0;
	param2[1] = param1[1] * (180.0 - rotation) / 180.0;
	param2[2] = param1[2] * (180.0 - rotation) / 180.0;
	// printf("param1[0] = %.2f param2[0] = %.2f\n", param1[0], param2[0]);
    } else {
	param2[0] = param1[0] * (rotation - 180.0) / 180.0;
	param2[1] = param1[1] * (rotation - 180.0) / 180.0;
	param2[2] = param1[2] * (rotation - 180.0) / 180.0;
	// printf("param1[0] = %.2f param2[0] = %.2f\n", param1[0], param2[0]);
    }

    adj_fog_color[0] = fog_color[0] + param2[0];
    if ( adj_fog_color[0] > 1.0 ) { adj_fog_color[0] = 1.0; }

    adj_fog_color[1] = fog_color[1] + param2[1];
    if ( adj_fog_color[1] > 1.0 ) { adj_fog_color[1] = 1.0; }

    adj_fog_color[2] = fog_color[2] + param2[2];
    if ( adj_fog_color[2] > 1.0 ) { adj_fog_color[2] = 1.0; }

    adj_fog_color[3] = fog_color[3];
}


// Destructor
fgLIGHT::~fgLIGHT( void ) {
}


// $Log$
// Revision 1.27  1999/04/05 02:14:40  curt
// Fixed a fog coloring bug.
//
// Revision 1.26  1999/02/05 21:29:20  curt
// Modifications to incorporate Jon S. Berndts flight model code.
//
// Revision 1.25  1999/01/07 20:25:36  curt
// Portability changes and updates from Bernie Bright.
//
// Revision 1.24  1998/12/09 18:50:35  curt
// Converted "class fgVIEW" to "class FGView" and updated to make data
// members private and make required accessor functions.
//
// Revision 1.23  1998/12/05 15:54:30  curt
// Renamed class fgFLIGHT to class FGState as per request by JSB.
//
// Revision 1.22  1998/12/03 01:18:42  curt
// Converted fgFLIGHT to a class.
//
// Revision 1.21  1998/11/23 21:49:09  curt
// Borland portability tweaks.
//
// Revision 1.20  1998/11/06 21:18:27  curt
// Converted to new logstream debugging facility.  This allows release
// builds with no messages at all (and no performance impact) by using
// the -DFG_NDEBUG flag.
//
// Revision 1.19  1998/10/20 18:41:53  curt
// Tweaked sunrise/sunset colors.
//
// Revision 1.18  1998/10/17 01:34:30  curt
// C++ ifying ...
//
// Revision 1.17  1998/08/29 13:11:33  curt
// Bernie Bright writes:
//   I've created some new classes to enable pointers-to-functions and
//   pointers-to-class-methods to be treated like objects.  These objects
//   can be registered with fgEVENT_MGR.
//
//   File "Include/fg_callback.hxx" contains the callback class defns.
//
//   Modified fgEVENT and fgEVENT_MGR to use the callback classes.  Also
//   some minor tweaks to STL usage.
//
//   Added file "Include/fg_stl_config.h" to deal with STL portability
//   issues.  I've added an initial config for egcs (and probably gcc-2.8.x).
//   I don't have access to Visual C++ so I've left that for someone else.
//   This file is influenced by the stl_config.h file delivered with egcs.
//
//   Added "Include/auto_ptr.hxx" which contains an implementation of the
//   STL auto_ptr class which is not provided in all STL implementations
//   and is needed to use the callback classes.
//
//   Deleted fgLightUpdate() which was just a wrapper to call
//   fgLIGHT::Update().
//
//   Modified fg_init.cxx to register two method callbacks in place of the
//   old wrapper functions.
//
// Revision 1.16  1998/08/27 17:02:11  curt
// Contributions from Bernie Bright <bbright@c031.aone.net.au>
// - use strings for fg_root and airport_id and added methods to return
//   them as strings,
// - inlined all access methods,
// - made the parsing functions private methods,
// - deleted some unused functions.
// - propogated some of these changes out a bit further.
//
// Revision 1.15  1998/08/25 20:53:33  curt
// Shuffled $FG_ROOT file layout.
//
// Revision 1.14  1998/08/06 12:47:22  curt
// Adjusted dusk/dawn lighting ...
//
// Revision 1.13  1998/07/24 21:42:26  curt
// Output message tweaks.
//
// Revision 1.12  1998/07/22 21:45:38  curt
// fg_time.cxx: Removed call to ctime() in a printf() which should be harmless
//   but seems to be triggering a bug.
// light.cxx: Added code to adjust fog color based on sunrise/sunset effects
//   and view orientation.  This is an attempt to match the fog color to the
//   sky color in the center of the screen.  You see discrepancies at the
//   edges, but what else can be done?
// sunpos.cxx: Caculate local direction to sun here.  (what compass direction
//   do we need to face to point directly at sun)
//
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
