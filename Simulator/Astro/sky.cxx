// sky.cxx -- model sky with an upside down "bowl"
//
// Written by Curtis Olson, started December 1997.
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

#include <math.h>

#include <GL/glut.h>
#include <XGL/xgl.h>

#include <Aircraft/aircraft.hxx>
#include <Debug/logstream.hxx>
#include <FDM/flight.hxx>
#include <Include/fg_constants.h>
#include <Main/views.hxx>
#include <Math/fg_random.h>
#include <Time/event.hxx>
#include <Time/fg_time.hxx>

#include "sky.hxx"


// in meters of course
#define CENTER_ELEV   25000.0

#define INNER_RADIUS  50000.0
#define INNER_ELEV    20000.0

#define MIDDLE_RADIUS 70000.0
#define MIDDLE_ELEV    8000.0

#define OUTER_RADIUS  80000.0
#define OUTER_ELEV        0.0

#define BOTTOM_RADIUS 50000.0
#define BOTTOM_ELEV   -2000.0


static float inner_vertex[12][3];
static float middle_vertex[12][3];
static float outer_vertex[12][3];
static float bottom_vertex[12][3];

static float inner_color[12][4];
static float middle_color[12][4];
static float outer_color[12][4];


// Calculate the sky structure vertices
void fgSkyVerticesInit( void ) {
    float theta;
    int i;

    FG_LOG(FG_ASTRO, FG_INFO, "  Generating the sky dome vertices.");

    for ( i = 0; i < 12; i++ ) {
	theta = (i * 30.0) * DEG_TO_RAD;
	
	inner_vertex[i][0] = cos(theta) * INNER_RADIUS;
	inner_vertex[i][1] = sin(theta) * INNER_RADIUS;
	inner_vertex[i][2] = INNER_ELEV;
	
	// printf("    %.2f %.2f\n", cos(theta) * INNER_RADIUS, 
	//        sin(theta) * INNER_RADIUS);

	middle_vertex[i][0] = cos((double)theta) * MIDDLE_RADIUS;
	middle_vertex[i][1] = sin((double)theta) * MIDDLE_RADIUS;
	middle_vertex[i][2] = MIDDLE_ELEV;
	    
	outer_vertex[i][0] = cos((double)theta) * OUTER_RADIUS;
	outer_vertex[i][1] = sin((double)theta) * OUTER_RADIUS;
	outer_vertex[i][2] = OUTER_ELEV;
	    
	bottom_vertex[i][0] = cos((double)theta) * BOTTOM_RADIUS;
	bottom_vertex[i][1] = sin((double)theta) * BOTTOM_RADIUS;
	bottom_vertex[i][2] = BOTTOM_ELEV;
    }
}


// (Re)calculate the sky colors at each vertex
void fgSkyColorsInit( void ) {
    fgLIGHT *l;
    double sun_angle, diff;
    double outer_param[3], outer_amt[3], outer_diff[3];
    double middle_param[3], middle_amt[3], middle_diff[3];
    int i, j;

    l = &cur_light_params;

    FG_LOG( FG_ASTRO, FG_INFO, 
	    "  Generating the sky colors for each vertex." );

    // setup for the possibility of sunset effects
    sun_angle = l->sun_angle * RAD_TO_DEG;
    // fgPrintf( FG_ASTRO, FG_INFO, 
    //           "  Sun angle in degrees = %.2f\n", sun_angle);

    if ( (sun_angle > 80.0) && (sun_angle < 100.0) ) {
	// 0.0 - 0.4
	outer_param[0] = (10.0 - fabs(90.0 - sun_angle)) / 20.0;
	outer_param[1] = (10.0 - fabs(90.0 - sun_angle)) / 40.0;
	outer_param[2] = -(10.0 - fabs(90.0 - sun_angle)) / 30.0;
	// outer_param[2] = 0.0;

	middle_param[0] = (10.0 - fabs(90.0 - sun_angle)) / 40.0;
	middle_param[1] = (10.0 - fabs(90.0 - sun_angle)) / 80.0;
	middle_param[2] = 0.0;

	outer_diff[0] = outer_param[0] / 6.0;
	outer_diff[1] = outer_param[1] / 6.0;
	outer_diff[2] = outer_param[2] / 6.0;

	middle_diff[0] = middle_param[0] / 6.0;
	middle_diff[1] = middle_param[1] / 6.0;
	middle_diff[2] = middle_param[2] / 6.0;
    } else {
	outer_param[0] = outer_param[1] = outer_param[2] = 0.0;
	middle_param[0] = middle_param[1] = middle_param[2] = 0.0;

	outer_diff[0] = outer_diff[1] = outer_diff[2] = 0.0;
	middle_diff[0] = middle_diff[1] = middle_diff[2] = 0.0;
    }
    // printf("  outer_red_param = %.2f  outer_red_diff = %.2f\n", 
    //        outer_red_param, outer_red_diff);

    // calculate transition colors between sky and fog
    for ( j = 0; j < 3; j++ ) {
	outer_amt[j] = outer_param[j];
	middle_amt[j] = middle_param[j];
    }

    for ( i = 0; i < 6; i++ ) {
	for ( j = 0; j < 3; j++ ) {
	    diff = l->sky_color[j] - l->fog_color[j];

	    // printf("sky = %.2f  fog = %.2f  diff = %.2f\n", 
	    //        l->sky_color[j], l->fog_color[j], diff);

	    inner_color[i][j] = l->sky_color[j] - diff * 0.3;
	    middle_color[i][j] = l->sky_color[j] - diff * 0.9 + middle_amt[j];
	    outer_color[i][j] = l->fog_color[j] + outer_amt[j];

	    if ( inner_color[i][j] > 1.00 ) { inner_color[i][j] = 1.00; }
	    if ( inner_color[i][j] < 0.10 ) { inner_color[i][j] = 0.10; }
	    if ( middle_color[i][j] > 1.00 ) { middle_color[i][j] = 1.00; }
	    if ( middle_color[i][j] < 0.10 ) { middle_color[i][j] = 0.10; }
	    if ( outer_color[i][j] > 1.00 ) { outer_color[i][j] = 1.00; }
	    if ( outer_color[i][j] < 0.10 ) { outer_color[i][j] = 0.10; }
	}
	inner_color[i][3] = middle_color[i][3] = outer_color[i][3] = 
	    l->sky_color[3];

	for ( j = 0; j < 3; j++ ) {
	    outer_amt[j] -= outer_diff[j];
	    middle_amt[j] -= middle_diff[j];
	}

	/*
	printf("inner_color[%d] = %.2f %.2f %.2f %.2f\n", i, inner_color[i][0],
	       inner_color[i][1], inner_color[i][2], inner_color[i][3]);
	printf("middle_color[%d] = %.2f %.2f %.2f %.2f\n", i, 
	       middle_color[i][0], middle_color[i][1], middle_color[i][2], 
	       middle_color[i][3]);
	printf("outer_color[%d] = %.2f %.2f %.2f %.2f\n", i, 
	       outer_color[i][0], outer_color[i][1], outer_color[i][2], 
	       outer_color[i][3]);
	*/
    }

    for ( j = 0; j < 3; j++ ) {
	outer_amt[j] = 0.0;
	middle_amt[j] = 0.0;
    }

    for ( i = 6; i < 12; i++ ) {

	for ( j = 0; j < 3; j++ ) {
	    diff = l->sky_color[j] - l->fog_color[j];

	    // printf("sky = %.2f  fog = %.2f  diff = %.2f\n", 
	    //        l->sky_color[j], l->fog_color[j], diff);

	    inner_color[i][j] = l->sky_color[j] - diff * 0.3;
	    middle_color[i][j] = l->sky_color[j] - diff * 0.9 + middle_amt[j];
	    outer_color[i][j] = l->fog_color[j] + outer_amt[j];

	    if ( inner_color[i][j] > 1.00 ) { inner_color[i][j] = 1.00; }
	    if ( inner_color[i][j] < 0.10 ) { inner_color[i][j] = 0.10; }
	    if ( middle_color[i][j] > 1.00 ) { middle_color[i][j] = 1.00; }
	    if ( middle_color[i][j] < 0.10 ) { middle_color[i][j] = 0.10; }
	    if ( outer_color[i][j] > 1.00 ) { outer_color[i][j] = 1.00; }
	    if ( outer_color[i][j] < 0.15 ) { outer_color[i][j] = 0.15; }
	}
	inner_color[i][3] = middle_color[i][3] = outer_color[i][3] = 
	    l->sky_color[3];

	for ( j = 0; j < 3; j++ ) {
	    outer_amt[j] += outer_diff[j];
	    middle_amt[j] += middle_diff[j];
	}

	/*
	printf("inner_color[%d] = %.2f %.2f %.2f %.2f\n", i, inner_color[i][0],
	       inner_color[i][1], inner_color[i][2], inner_color[i][3]);
	printf("middle_color[%d] = %.2f %.2f %.2f %.2f\n", i, 
	       middle_color[i][0], middle_color[i][1], middle_color[i][2], 
	       middle_color[i][3]);
	printf("outer_color[%d] = %.2f %.2f %.2f %.2f\n", i, 
	       outer_color[i][0], outer_color[i][1], outer_color[i][2], 
	       outer_color[i][3]);
	*/
    }
}


// Initialize the sky structure and colors
void fgSkyInit( void ) {
    FG_LOG( FG_ASTRO, FG_INFO, "Initializing the sky" );

    fgSkyVerticesInit();

    // regester fgSkyColorsInit() as an event to be run periodically
    global_events.Register( "fgSkyColorsInit()", fgSkyColorsInit, 
		            fgEVENT::FG_EVENT_READY, 30000);
}


// Draw the Sky
void fgSkyRender( void ) {
    FGInterface *f;
    fgLIGHT *l;
    float inner_color[4];
    float middle_color[4];
    float outer_color[4];
    double diff;
    int i;

    f = current_aircraft.fdm_state;
    l = &cur_light_params;

    // printf("Rendering the sky.\n");

    // calculate the proper colors
    for ( i = 0; i < 3; i++ ) {
	diff = l->sky_color[i] - l->adj_fog_color[i];

	// printf("sky = %.2f  fog = %.2f  diff = %.2f\n", 
	//        l->sky_color[j], l->adj_fog_color[j], diff);

	inner_color[i] = l->sky_color[i] - diff * 0.3;
	middle_color[i] = l->sky_color[i] - diff * 0.9;
	outer_color[i] = l->adj_fog_color[i];
    }
    inner_color[3] = middle_color[3] = outer_color[3] = l->adj_fog_color[3];

    xglPushMatrix();

    // Translate to view position
    Point3D zero_elev = current_view.get_cur_zero_elev();
    xglTranslatef( zero_elev.x(), zero_elev.y(), zero_elev.z() );
    // printf("  Translated to %.2f %.2f %.2f\n", 
    //        zero_elev.x, zero_elev.y, zero_elev.z );

    // Rotate to proper orientation
    // printf("  lon = %.2f  lat = %.2f\n", FG_Longitude * RAD_TO_DEG,
    //        FG_Latitude * RAD_TO_DEG);
    xglRotatef( f->get_Longitude() * RAD_TO_DEG, 0.0, 0.0, 1.0 );
    xglRotatef( 90.0 - f->get_Latitude() * RAD_TO_DEG, 0.0, 1.0, 0.0 );
    xglRotatef( l->sun_rotation * RAD_TO_DEG, 0.0, 0.0, 1.0 );

    // Draw inner/center section of sky*/
    xglBegin( GL_TRIANGLE_FAN );
    xglColor4fv(l->sky_color);
    xglVertex3f(0.0, 0.0, CENTER_ELEV);
    for ( i = 11; i >= 0; i-- ) {
	xglColor4fv( inner_color );
	xglVertex3fv( inner_vertex[i] );
    }
    xglColor4fv( inner_color );
    xglVertex3fv( inner_vertex[11] );
    xglEnd();

    // Draw the middle ring
    xglBegin( GL_TRIANGLE_STRIP );
    for ( i = 0; i < 12; i++ ) {
	xglColor4fv( middle_color );
	// printf("middle_color[%d] = %.2f %.2f %.2f %.2f\n", i, 
	//        middle_color[i][0], middle_color[i][1], middle_color[i][2], 
	//        middle_color[i][3]);
	// xglColor4f(1.0, 0.0, 0.0, 1.0);
	xglVertex3fv( middle_vertex[i] );
	xglColor4fv( inner_color );
	// printf("inner_color[%d] = %.2f %.2f %.2f %.2f\n", i, 
	//        inner_color[i][0], inner_color[i][1], inner_color[i][2], 
	//        inner_color[i][3]);
	// xglColor4f(0.0, 0.0, 1.0, 1.0);
	xglVertex3fv( inner_vertex[i] );
    }
    xglColor4fv( middle_color );
    // xglColor4f(1.0, 0.0, 0.0, 1.0);
    xglVertex3fv( middle_vertex[0] );
    xglColor4fv( inner_color );
    // xglColor4f(0.0, 0.0, 1.0, 1.0);
    xglVertex3fv( inner_vertex[0] );
    xglEnd();

    // Draw the outer ring
    xglBegin( GL_TRIANGLE_STRIP );
    for ( i = 0; i < 12; i++ ) {
	xglColor4fv( outer_color );
	xglVertex3fv( outer_vertex[i] );
	xglColor4fv( middle_color );
	xglVertex3fv( middle_vertex[i] );
    }
    xglColor4fv( outer_color );
    xglVertex3fv( outer_vertex[0] );
    xglColor4fv( middle_color );
    xglVertex3fv( middle_vertex[0] );
    xglEnd();

    // Draw the bottom skirt
    xglBegin( GL_TRIANGLE_STRIP );
    xglColor4fv( outer_color );
    for ( i = 0; i < 12; i++ ) {
	xglVertex3fv( bottom_vertex[i] );
	xglVertex3fv( outer_vertex[i] );
    }
    xglVertex3fv( bottom_vertex[0] );
    xglVertex3fv( outer_vertex[0] );
    xglEnd();

    xglPopMatrix();
}


