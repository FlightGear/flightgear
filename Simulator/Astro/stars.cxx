// stars.cxx -- data structures and routines for managing and rendering stars.
//
// Written by Curtis Olson, started August 1997.
//
// Copyright (C) 1997  Curtis L. Olson  - curt@me.umn.edu
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
//*************************************************************************/


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include "Include/compiler.h"
#ifdef FG_HAVE_STD_INCLUDES
#  include <cmath>
#  include <cstdio>
#  include <cstring>
#  include <ctime>
#else
#  include <math.h>
#  include <stdio.h>
#  include <string.h>
#  include <time.h>
#endif

#include <string>

#include <GL/glut.h>
#include <XGL/xgl.h>

#include <Aircraft/aircraft.hxx>
#include <Debug/logstream.hxx>
#include <Include/fg_constants.h>
#include <Misc/fgstream.hxx>
#include <Main/options.hxx>
#include <Main/views.hxx>
#include <Misc/stopwatch.hxx>
#include <Time/fg_time.hxx>
#include "Misc/stopwatch.hxx"

#include "stars.hxx"

FG_USING_STD(getline);

#define EpochStart           (631065600)
#define DaysSinceEpoch(secs) (((secs)-EpochStart)*(1.0/(24*3600)))
#define FG_MAX_STARS 3500

// Define four structures, each with varying amounts of stars
static GLint stars[FG_STAR_LEVELS];


// Initialize the Star Management Subsystem
int fgStarsInit( void ) {
    Point3D starlist[FG_MAX_STARS];
    // struct CelestialCoord pltPos;
    double right_ascension, declination, magnitude;
    double min_magnitude[FG_STAR_LEVELS];
    // double ra_save, decl_save;
    // double ra_save1, decl_save1;
    int i, j, starcount, count;

    FG_LOG( FG_ASTRO, FG_INFO, "Initializing stars" );

    if ( FG_STAR_LEVELS < 4 ) {
	FG_LOG( FG_ASTRO, FG_ALERT, "Big whups in stars.cxx" );
	exit(-1);
    }

    // build the full path name to the stars data base file
    string path = current_options.get_fg_root() + "/Astro/stars" + ".gz";

    FG_LOG( FG_ASTRO, FG_INFO, "  Loading stars from " << path );

    fg_gzifstream in( path );
    if ( ! in ) {
	FG_LOG( FG_ASTRO, FG_ALERT, "Cannot open star file: " << path );
	exit(-1);
    }

    starcount = 0;

    StopWatch timer;
    timer.start();

    // read in each line of the file
    while ( ! in.eof() && starcount < FG_MAX_STARS )
    {
	in >> skipcomment;
	string name;
	getline( in, name, ',' );
	in >> starlist[starcount];
	++starcount;
    }

    timer.stop();
    FG_LOG( FG_ASTRO, FG_INFO, 
	    "Loaded " << starcount << " stars in "
	    << timer.elapsedSeconds() << " seconds" );

    min_magnitude[0] = 4.2;
    min_magnitude[1] = 3.6;
    min_magnitude[2] = 3.0;
    min_magnitude[3] = 2.4;
    min_magnitude[4] = 1.8;
    min_magnitude[5] = 1.2;
    min_magnitude[6] = 0.6;
    min_magnitude[7] = 0.0;

    // build the various star display lists
    for ( i = 0; i < FG_STAR_LEVELS; i++ ) {

	stars[i] = xglGenLists(1);
	xglNewList( stars[i], GL_COMPILE );
	xglBegin( GL_POINTS );

	count = 0;

	for ( j = 0; j < starcount; j++ ) {
	    magnitude = starlist[j].z();
	    // printf("magnitude = %.2f\n", magnitude);

	    if ( magnitude < min_magnitude[i] ) {
		right_ascension = starlist[j].x();
		declination = starlist[j].y();

		count++;

		// scale magnitudes to (0.0 - 1.0)
		magnitude = (0.0 - magnitude) / 5.0 + 1.0;
		
		// scale magnitudes again so they look ok
		if ( magnitude > 1.0 ) { magnitude = 1.0; }
		if ( magnitude < 0.0 ) { magnitude = 0.0; }
		// magnitude =
		//     magnitude * 0.7 + (((FG_STAR_LEVELS - 1) - i) * 0.042);
		  
		magnitude = magnitude * 0.9 + 
		    (((FG_STAR_LEVELS - 1) - i) * 0.014);
		// printf("  Found star: %d %s, %.3f %.3f %.3f\n", count,
		//        name, right_ascension, declination, magnitude);
		    
		xglColor3f( magnitude, magnitude, magnitude );
		//xglColor3f(0,0,0);*/
		xglVertex3f( 50000.0*cos(right_ascension)*cos(declination),
			     50000.0*sin(right_ascension)*cos(declination),
			     50000.0*sin(declination) );
	    }
	} // while

	xglEnd();

	/*
	xglBegin(GL_LINE_LOOP);
        xglColor3f(1.0, 0.0, 0.0);
	xglVertex3f( 50000.0 * cos(ra_save-0.2) * cos(decl_save-0.2),
		    50000.0 * sin(ra_save-0.2) * cos(decl_save-0.2),
		    50000.0 * sin(decl_save-0.2) );
	xglVertex3f( 50000.0 * cos(ra_save+0.2) * cos(decl_save-0.2),
		    50000.0 * sin(ra_save+0.2) * cos(decl_save-0.2),
		    50000.0 * sin(decl_save-0.2) );
 	xglVertex3f( 50000.0 * cos(ra_save+0.2) * cos(decl_save+0.2),
		    50000.0 * sin(ra_save+0.2) * cos(decl_save+0.2),
		    50000.0 * sin(decl_save+0.2) );
 	xglVertex3f( 50000.0 * cos(ra_save-0.2) * cos(decl_save+0.2),
		    50000.0 * sin(ra_save-0.2) * cos(decl_save+0.2),
		    50000.0 * sin(decl_save+0.2) );
	xglEnd();
	*/

	/*
	xglBegin(GL_LINE_LOOP);
        xglColor3f(0.0, 1.0, 0.0);
	xglVertex3f( 50000.0 * cos(ra_save1-0.2) * cos(decl_save1-0.2),
		    50000.0 * sin(ra_save1-0.2) * cos(decl_save1-0.2),
		    50000.0 * sin(decl_save1-0.2) );
	xglVertex3f( 50000.0 * cos(ra_save1+0.2) * cos(decl_save1-0.2),
		    50000.0 * sin(ra_save1+0.2) * cos(decl_save1-0.2),
		    50000.0 * sin(decl_save1-0.2) );
 	xglVertex3f( 50000.0 * cos(ra_save1+0.2) * cos(decl_save1+0.2),
		    50000.0 * sin(ra_save1+0.2) * cos(decl_save1+0.2),
		    50000.0 * sin(decl_save1+0.2) );
 	xglVertex3f( 50000.0 * cos(ra_save1-0.2) * cos(decl_save1+0.2),
		    50000.0 * sin(ra_save1-0.2) * cos(decl_save1+0.2),
		    50000.0 * sin(decl_save1+0.2) );
	xglEnd();
	*/

	xglEndList();
	    
	FG_LOG( FG_ASTRO, FG_INFO,
		"  Loading " << count << " stars brighter than " 
		<< min_magnitude[i] );
    }

    return 1;  // OK, we got here because initialization worked.
}


// Draw the Stars
void fgStarsRender( void ) {
    FGInterface *f;
    fgLIGHT *l;
    fgTIME *t;
    int i;

    f = current_aircraft.fdm_state;
    l = &cur_light_params;
    t = &cur_time_params;

    // FG_PI_2 + 0.1 is about 6 degrees after sundown and before sunrise

    // t->sun_angle = 3.0; // to force stars to be drawn (for testing)

    // render the stars
    if ( l->sun_angle > (FG_PI_2 + 5 * DEG_TO_RAD ) ) {
	// determine which star structure to draw
	if ( l->sun_angle > (FG_PI_2 + 10.0 * DEG_TO_RAD ) ) {
	    i = 0;
	} else if ( l->sun_angle > (FG_PI_2 + 8.8 * DEG_TO_RAD ) ) {
	    i = 1;
	} else if ( l->sun_angle > (FG_PI_2 + 7.5 * DEG_TO_RAD ) ) {
	    i = 2;
	} else if ( l->sun_angle > (FG_PI_2 + 7.0 * DEG_TO_RAD ) ) {
	    i = 3;
	} else if ( l->sun_angle > (FG_PI_2 + 6.5 * DEG_TO_RAD ) ) {
	    i = 4;
	} else if ( l->sun_angle > (FG_PI_2 + 6.0 * DEG_TO_RAD ) ) {
	    i = 5;
	} else if ( l->sun_angle > (FG_PI_2 + 5.5 * DEG_TO_RAD ) ) {
	    i = 6;
	} else {
	    i = 7;
	}

	// printf("RENDERING STARS = %d (night)\n", i);

	xglCallList(stars[i]);
    } else {
	// printf("not RENDERING STARS (day)\n");
    }
}


