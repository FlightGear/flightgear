//
// light.cxx -- lighting routines
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

#include GLUT_H

#include <simgear/compiler.h>

#ifdef SG_MATH_EXCEPTION_CLASH
#  define exception c_exception
#endif

#ifdef SG_HAVE_STD_INCLUDES
#  include <cmath>
#else
#  include <math.h>
#endif

#include <string>
SG_USING_STD(string);

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/math/interpolater.hxx>
#include <simgear/math/polar3d.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/screen/colors.hxx>
#include <simgear/sky/sky.hxx>

#include <Aircraft/aircraft.hxx>
#include <Main/globals.hxx>
#include <Main/viewer.hxx>

#include "light.hxx"
#include "sunpos.hxx"

extern SGSky *thesky;           // FIXME: from main.cxx

fgLIGHT cur_light_params;


// Constructor
fgLIGHT::fgLIGHT( void ) {
}


// initialize lighting tables
void fgLIGHT::Init( void ) {
    SG_LOG( SG_EVENT, SG_INFO, 
	    "Initializing Lighting interpolation tables." );

    // build the path name to the ambient lookup table
    SGPath path( globals->get_fg_root() );
    SGPath ambient = path;
    ambient.append( "Lighting/ambient" );
    SGPath diffuse = path;
    diffuse.append( "Lighting/diffuse" );
    SGPath specular = path;
    specular.append( "Lighting/specular" );
    SGPath sky = path;
    sky.append( "Lighting/sky" );

    // initialize ambient table
    ambient_tbl = new SGInterpTable( ambient.str() );

    // initialize diffuse table
    diffuse_tbl = new SGInterpTable( diffuse.str() );
    
    // initialize diffuse table
    specular_tbl = new SGInterpTable( specular.str() );
    
    // initialize sky table
    sky_tbl = new SGInterpTable( sky.str() );
}


// update lighting parameters based on current sun position
void fgLIGHT::Update( void ) {
    FGInterface *f;
    // if the 4th field is 0.0, this specifies a direction ...
    GLfloat white[4] = { 1.0, 1.0, 1.0, 1.0 };
    // base sky color
    GLfloat base_sky_color[4] = { 0.392, 0.539, 0.752, 1.0 };
    // base fog color
    GLfloat base_fog_color[4] = { 0.90, 0.93, 1.0, 1.0 };
    double deg, ambient, diffuse, specular, sky_brightness;

    f = current_aircraft.fdm_state;

    SG_LOG( SG_EVENT, SG_INFO, "Updating light parameters." );

    // first, correct the colors for system specific gamma settings
    // gamma_correct( (float *)&base_sky_color );
    // gamma_correct( (float *)&base_fog_color );

    // calculate lighting parameters based on sun's relative angle to
    // local up

    deg = sun_angle * SGD_RADIANS_TO_DEGREES;
    SG_LOG( SG_EVENT, SG_INFO, "  Sun angle = " << deg );

    ambient = ambient_tbl->interpolate( deg );
    diffuse = diffuse_tbl->interpolate( deg );
    specular = specular_tbl->interpolate( deg );
    sky_brightness = sky_tbl->interpolate( deg );

    SG_LOG( SG_EVENT, SG_INFO, 
	    "  ambient = " << ambient << "  diffuse = " << diffuse 
	    << "  specular = " << specular << "  sky = " << sky_brightness );

    // sky_brightness = 0.15;  // used to force a dark sky (when testing)

    // if ( ambient < 0.02 ) { ambient = 0.02; }
    // if ( diffuse < 0.0 ) { diffuse = 0.0; }
    // if ( sky_brightness < 0.1 ) { sky_brightness = 0.1; }

    scene_ambient[0] = white[0] * ambient;
    scene_ambient[1] = white[1] * ambient;
    scene_ambient[2] = white[2] * ambient;
    scene_ambient[3] = 1.0;
    gamma_correct( (float *)&scene_ambient );

    scene_diffuse[0] = white[0] * diffuse;
    scene_diffuse[1] = white[1] * diffuse;
    scene_diffuse[2] = white[2] * diffuse;
    scene_diffuse[3] = 1.0;
    gamma_correct( (float *)&scene_diffuse );

    scene_specular[0] = white[0] * specular;
    scene_specular[1] = white[1] * specular;
    scene_specular[2] = white[2] * specular;
    scene_specular[3] = 1.0;
    gamma_correct( (float *)&scene_specular );

    // set sky color
    sky_color[0] = base_sky_color[0] * sky_brightness;
    sky_color[1] = base_sky_color[1] * sky_brightness;
    sky_color[2] = base_sky_color[2] * sky_brightness;
    sky_color[3] = base_sky_color[3];
    gamma_correct( (float *)&sky_color );

    // set cloud and fog color
    cloud_color[0] = fog_color[0] = base_fog_color[0] * sky_brightness;
    cloud_color[1] = fog_color[1] = base_fog_color[1] * sky_brightness;
    cloud_color[2] = fog_color[2] = base_fog_color[2] * sky_brightness;
    cloud_color[3] = fog_color[3] = base_fog_color[3];
    gamma_correct( (float *)&cloud_color );

    // update the cloud colors for sunrise/sunset effects (darken them)
    if (sun_angle > 1.0) {
       float sun2 = sqrt(sun_angle);
       cloud_color[0] /= sun2;
       cloud_color[1] /= sun2;
       cloud_color[2] /= sun2;
    }
    gamma_correct( (float *)&cloud_color );
}


// calculate fog color adjusted for sunrise/sunset effects
void fgLIGHT::UpdateAdjFog( void ) {

    double heading = globals->get_current_view()->getHeading_deg()
                     * SGD_DEGREES_TO_RADIANS;
    double heading_offset = globals->get_current_view()->getHeadingOffset_deg()
                            * SGD_DEGREES_TO_RADIANS;

    SG_LOG( SG_EVENT, SG_DEBUG, "Updating adjusted fog parameters." );

    // set fog color (we'll try to match the sunset color in the
    // direction we are looking

    // Do some sanity checking ...
    if ( sun_rotation < -2.0 * SGD_2PI || sun_rotation > 2.0 * SGD_2PI ) {
	SG_LOG( SG_EVENT, SG_ALERT, "Sun rotation bad = " << sun_rotation );
	exit(-1);
    }

    if ( heading < -2.0 * SGD_2PI || heading > 2.0 * SGD_2PI ) {
	SG_LOG( SG_EVENT, SG_ALERT, "Psi rotation bad = " << heading );
	exit(-1);
    }

    if ( heading_offset < -2.0 * SGD_2PI || heading_offset > 2.0 * SGD_2PI ) {
	SG_LOG( SG_EVENT, SG_ALERT, "current view()->view offset bad = " 
		                     << heading_offset );
	exit(-1);
    }

    double rotation;

    // first determine the difference between our view angle and local
    // direction to the sun
    rotation = -(sun_rotation + SGD_PI) - heading - heading_offset;
    while ( rotation < 0 ) {
	rotation += SGD_2PI;
    }
    while ( rotation > SGD_2PI ) {
	rotation -= SGD_2PI;
    }

#ifdef USE_OLD_SUNSET_CODE

    double sun_angle_deg, param1[3], param2[3];

    rotation *= SGD_RADIANS_TO_DEGREES;
    // fgPrintf( SG_EVENT, SG_INFO, 
    //           "  View to sun difference in degrees = %.2f\n", rotation);

    // next check if we are in a sunset/sunrise situation
    sun_angle_deg = sun_angle * SGD_RADIANS_TO_DEGREES;
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

#else

    float rf1 = fabs((rotation - SG_PI) / SG_PI);		// 0.0 .. 1.0
    float rf2 = rf1 * rf1;
    float rf3 = 1.0 - rf1;

    float *sun_color = thesky->get_sun_color();
    float s_red =   fog_color[0] * (1.25 - sun_color[0]/4.0);	// 100% red
    float s_green = fog_color[1] * (0.48 + sun_color[1]/1.923);	//  40% green
    float s_blue =  fog_color[2] * sun_color[2];		//   0% blue

    float f_brightness = (sun_angle > 1.0) ? sqrt(sun_angle) : 1.0;
    float f_red =   fog_color[0] / f_brightness;
    float f_green = fog_color[1] / f_brightness;
    float f_blue =  (fog_color[2] / f_brightness) * pow(sun_color[2], 1/3);

    adj_fog_color[0] = rf3 * f_red   + rf2 * s_red;
    adj_fog_color[1] = rf3 * f_green + rf2 * s_green;
    adj_fog_color[2] = rf3 * f_blue  + rf2 * s_blue;

    gamma_correct( (float *)&adj_fog_color );
#endif
}


// Destructor
fgLIGHT::~fgLIGHT( void ) {
}


