// joystick.cxx -- joystick support
//
// Written by Curtis Olson, started October 1998.
//
// Copyright (C) 1998 - 1999  Curtis L. Olson - curt@flightgear.org
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

#include <Aircraft/aircraft.hxx>
#include <Debug/logstream.hxx>
#include <Main/options.hxx>

#if defined( ENABLE_PLIB_JOYSTICK )
#  include <plib/js.h>		// plib include
#elif defined( ENABLE_GLUT_JOYSTICK )
#  include <GL/glut.h>
#  include <XGL/xgl.h>
#endif


#include "joystick.hxx"


#if defined( ENABLE_PLIB_JOYSTICK )

// joystick classes
static jsJoystick *js0;
static jsJoystick *js1;

// these will hold the values of the axes
static float *js_ax0, *js_ax1;

#elif defined( ENABLE_GLUT_JOYSTICK )

// Do we want these user settable ??
static float joy_scale = 1./1000;

// play with following to get your desired sensitivity
static int x_dead_zone = 50;
static int y_dead_zone = 2*x_dead_zone;

// Joystick support using glut -- William Riley -- riley@technologist.com

// Joystick fixed values for calibration and scaling
static float joy_x_max = joy_scale;
static float joy_y_max = joy_scale;

static int joy_z_min = 1000, /* joy_z_ctr=0, */ joy_z_max = -1000;
static int joy_z_dead_min = 100, joy_z_dead_max = -100;

#elif defined( MACOS )
#  warning port me: no joystick support
#else
#  error port me: no joystick support
#endif



#if defined( ENABLE_GLUT_JOYSTICK )

// Function called by glutJoystickFunc(), adjusts read values and
// passes them to the necessary aircraft control functions
void joystick(unsigned int buttonMask, int js_x, int js_y, int js_z)
{
    float joy_x, joy_y, joy_z;
    // adjust the values to fgfs's scale and allow a 'dead zone' to
    // reduce jitter code adapted from joystick.c by Michele
    // F. America - nomimarketing@mail.telepac.pt

    if( js_x > -x_dead_zone && js_x < x_dead_zone) {
	joy_x = 0.0;
    } else {
	joy_x = js_x * joy_scale;
    }

    if( js_y > -y_dead_zone && js_y < y_dead_zone) {
	joy_y = 0.0;
    } else {
	joy_y = js_y * joy_scale;
    }

    if( js_z >= joy_z_dead_min && js_z <= joy_z_dead_max ) {
	joy_z = 0.0;
    }
    joy_z = (float)js_z / (float)(joy_z_max - joy_z_min);
    joy_z = (((joy_z*2.0)+1.0)/2);

    // Pass the values to the control routines
    controls.set_elevator( -joy_y );
    controls.set_aileron( joy_x );
    controls.set_throttle( FGControls::ALL_ENGINES, joy_z );
}

#endif // ENABLE_GLUT_JOYSTICK


// Initialize the joystick(s)
int fgJoystickInit( void ) {

    FG_LOG( FG_INPUT, FG_INFO, "Initializing joystick" );

#if defined( ENABLE_PLIB_JOYSTICK )

    js0 = new jsJoystick ( 0 );
    js1 = new jsJoystick ( 1 );

    if ( js0->notWorking () ) {
	// not working
    } else {
	// allocate storage for axes values
	js_ax0 = new float [ js0->getNumAxes() ];

	// configure
	js0->setDeadBand( 0, 0.1 );
	js0->setDeadBand( 1, 0.1 );

	FG_LOG ( FG_INPUT, FG_INFO, 
		 "  Joystick 0 detected with " << js0->getNumAxes() 
		 << " axes" );
    }

    if ( js1->notWorking () ) {
	// not working
    } else {
	// allocate storage for axes values
	js_ax1 = new float [ js1->getNumAxes() ];

	// configure
	js1->setDeadBand( 0, 0.1 );
	js1->setDeadBand( 1, 0.1 );

	FG_LOG ( FG_INPUT, FG_INFO,
		 "  Joystick 1 detected with " << js1->getNumAxes() 
		 << " axes" );
    }

    if ( js0->notWorking() && js1->notWorking() ) {
	FG_LOG ( FG_INPUT, FG_INFO, "  No joysticks detected" );
	return 0;
    }

    // I hate doing this sort of thing, but it's overridable from the
    // command line/config file.  If the user hasn't specified an
    // autocoordination preference, and if they have a single 2 axis
    // joystick, then automatical enable auto_coordination.

    if ( (current_options.get_auto_coordination() == 
	  fgOPTIONS::FG_AUTO_COORD_NOT_SPECIFIED) &&
	 (!js0->notWorking() && js1->notWorking() && (js0->getNumAxes() < 3)
	  )
	 )
    {
	current_options.set_auto_coordination(fgOPTIONS::FG_AUTO_COORD_ENABLED);
    }


#elif defined( ENABLE_GLUT_JOYSTICK )

    glutJoystickFunc(joystick, 100);

#elif defined( MACOS )
#  warning port me: no joystick support
#else
#  error port me: no joystick support
#endif

    return 1;
}


#if defined( ENABLE_PLIB_JOYSTICK )

// update the control parameters based on joystick intput
int fgJoystickRead( void ) {
    int b;

    if ( ! js0->notWorking() ) {
	js0->read( &b, js_ax0 ) ;
	controls.set_aileron( js_ax0[0] );
	controls.set_elevator( -js_ax0[1] );

	//  Added by William Riley -- riley@technologist.com
	if ( js0->getNumAxes() >= 3 ) {
	    controls.set_throttle( FGControls::ALL_ENGINES,
				   ((-js_ax0[2] + 1) / 2) );
	} 
	if ( js0->getNumAxes() > 3 ) {
	    if ( current_options.get_auto_coordination() !=
		  fgOPTIONS::FG_AUTO_COORD_ENABLED ) 
	    {
		controls.set_rudder( js_ax0[3] );
	    }
	}
	//  End of William's code

    }

    if ( ! js1->notWorking() ) {
	js1->read( &b, js_ax1 ) ;
	if ( current_options.get_auto_coordination() !=
	     fgOPTIONS::FG_AUTO_COORD_ENABLED ) 
	{
	    controls.set_rudder( js_ax1[0] );
	}
	controls.set_throttle( FGControls::ALL_ENGINES, -js_ax1[1] * 1.05 );
    }

    return 1;
}

#endif // ENABLE_PLIB_JOYSTICK

