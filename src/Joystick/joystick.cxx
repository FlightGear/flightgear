// joystick.cxx -- joystick support
//
// Written by Curtis Olson, started October 1998.
// Amended by Alexander Perry, started May 2000, for lots of game controllers.
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

#include <simgear/debug/logstream.hxx>

#include <Aircraft/aircraft.hxx>
#include <Main/options.hxx>
#include <Main/views.hxx>

#if defined( ENABLE_PLIB_JOYSTICK )
#  include <plib/js.h>		// plib include
#elif defined( ENABLE_GLUT_JOYSTICK )
#  include <GL/glut.h>
#  include <simgear/xgl.h>
#endif


#include "joystick.hxx"


#if defined( ENABLE_PLIB_JOYSTICK )

// Maximum number of joystick devices
#define jsN 8

// joystick classes
static jsJoystick *( js[jsN] );

// these will hold the values of the axes
// the first points to all the axes, the second by device
static int    js_axes;
static float *js_ax_all, *( js_ax[jsN] );
static int    js_buttons[jsN];

// these will point to elements of that float array
static float *to_elevator, *to_aileron, *to_rudder, *to_throttle;
static float *to_viewhat, *to_brakeL, *to_brakeR;

// these will point to elements of that digital button array
static int *to_gearmove, *to_flapup, *to_flapdn, *to_eltrimup, *to_eltrimdn;
static int msk_gearmove, msk_flapup, msk_flapdn, msk_eltrimup, msk_eltrimdn;

// this finds the first unused occasion of a specific channel number
// on a controller with a known number of channels.  Remember, when
// you use USB devices, they appear in a semi-random order.  Sigh.
void to_find_A ( int firstcall, float **ptr, int axes, int axis )
{	int a;

	if (firstcall) {
	    (*ptr) = NULL;
	}

	for ( a = 0; a < jsN; a++ ) {
	    if ( NULL != js[a] ) {
		if ( NULL == *ptr ) {
		    if ( axes == js[a]->getNumAxes() ) {
			cout << "axis[" << a << "][" << axis << "] = "
			     << (js_ax[a])[axis] << endl;
			if ( (js_ax[a])[axis] > 0.5 ) {
			    ( *ptr) = (js_ax[a]) + axis;
			    (**ptr) = 0;
			}
		    }
		}
	    }
	}
}

// this finds a specific button on a given controller device
static int to_find_D_zero;
void to_find_D ( int butnum, float *ana, int **butptr, int *mask )
{	int a;
	to_find_D_zero = 0;
	(*butptr) = & to_find_D_zero;
	(*mask) = ( (int) 1 ) << butnum;
	if ( NULL != ana )
	 for ( a=0; a < jsN; a++ )
	  if ( ( NULL != js[a] )
	    && ( ana >= js_ax[a] )
	    && ( ana -  js_ax[a] <= js[a]->getNumAxes() )
	       ){	(*butptr) = & ( js_buttons[a] );
//			printf ( "Button %i uses mask %i\n", butnum, *mask );
		}
}

// this decides whether we believe the throttle is safe to use
static bool sync_throttle=false;
static float throttle_tmp=0;

#define SYNC_TOLERANCE 0.02

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

    int i, j;

    FG_LOG( FG_INPUT, FG_INFO, "Initializing joystick" );

#if defined( ENABLE_PLIB_JOYSTICK )

    js_axes = 0;
    for ( i = 0; i < jsN; i ++ )
    {   js[i] = new jsJoystick ( i );
        if ( js[i]->notWorking () ) 
	{   
	    delete js[i];
            js[i] = NULL;
        } else {
	    j = js[i]->getNumAxes();
	    FG_LOG ( FG_INPUT, FG_INFO, 
		 "  Joystick " << i  << " detected with " << j << " axes" );
	    // Count axes and set all the deadbands
	    js_axes += j;
	    while ( j>0 )
		js[i]->setDeadBand( --j, 0.1 );
	}
    }

    // allocate storage for axes values
    js_ax_all = new float [ js_axes + 1 ];
    j = 0;
    for ( i = 0; i < jsN; i ++ )
      if ( js[i] != NULL )
      { // Point to the memory
	js_ax [i] = js_ax_all + j;
	j += js[i]->getNumAxes();
      }

    // Warn user if we didn't find anything in the end
    if ( js_axes == 0 ) {
	FG_LOG ( FG_INPUT, FG_INFO, "  No joysticks detected" );
    }

    // Guess channel assignments; do this once and save nightmares later
    for ( i = 0; i < js_axes; i++ )
    {	js_ax_all[i] = 1.0;
	js_buttons[i] = 0;
    }

    to_find_A ( 1, &to_aileron	, 6, 0 );	// Yoke
    to_find_A ( 0, &to_aileron	, 2, 0 );	// Analog JS
    to_find_A ( 0, &to_aileron	, 4, 0 );	// Gaming JS

    to_find_A ( 1, &to_elevator	, 6, 1 );	// Yoke
    to_find_A ( 0, &to_elevator	, 2, 1 );	// Analog JS
    to_find_A ( 0, &to_elevator	, 4, 1 );	// Gaming JS

    to_find_A ( 1, &to_throttle	, 6, 3 );	// Yoke
    to_find_A ( 0, &to_throttle	, 2, 0 );	// Analog JS, presume second
    to_find_A ( 0, &to_throttle	, 4, 3 );	// Game JS

    to_find_A ( 1, &to_rudder	, 4, 2 );	// Pedals or gaming joystick
    to_find_A ( 0, &to_rudder	, 2, 1 );	// Analog JS, maybe second

    to_find_A ( 1, &to_viewhat	, 6, 4 );	// Yoke

    to_find_A ( 1, &to_brakeL	, 4, 0 );	// Pedals
    to_find_A ( 1, &to_brakeR	, 4, 1 );	// Pedals

    // Derive some of the interesting buttons from the channels
	// 0 is the push-to-talk, 1 is gear switch
    to_find_D (  1, to_viewhat	, &to_gearmove,	&msk_gearmove );
	// 2,3 are rudder trim
	// 4,5 are spare buttons
	// 8,9 are flaps
    to_find_D (  9, to_viewhat	, &to_flapup,	&msk_flapup );
    to_find_D (  8, to_viewhat	, &to_flapdn,	&msk_flapdn );
    to_find_D (  6, to_viewhat	, &to_eltrimup,	&msk_eltrimup );
    to_find_D (  7, to_viewhat	, &to_eltrimdn,	&msk_eltrimdn );

    // I hate doing this sort of thing, but it's overridable from the
    // command line/config file.  If the user hasn't specified an
    // autocoordination preference, and if they only have two axes of
    // joystick, then automatical enable auto_coordination.

    if ( ( current_options.get_auto_coordination() == 
	   fgOPTIONS::FG_AUTO_COORD_NOT_SPECIFIED
         )
      && ( js_axes > 0 )
      && ( js_axes < 3 )
       )
    {
	current_options.set_auto_coordination(fgOPTIONS::FG_AUTO_COORD_ENABLED);
    }
    
    if(current_options.get_trim_mode() > 0) {
        FG_LOG(FG_INPUT, FG_INFO,
	       "  Waiting for user to synchronize throttle lever...");
        sync_throttle=true;
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
    int i;

    // Go and fetch all the readings in one pass
    for ( i = 0; i < jsN; i++ )
	if ( NULL != js[i] )
	    js[i]->read( & ( js_buttons[i] ), js_ax[i] ) ;

    // These are the easy ones for now
    if ( NULL != to_aileron )	controls.set_aileron (   * to_aileron  );
    if ( NULL != to_elevator )	controls.set_elevator( - * to_elevator );
    if ( NULL != to_brakeL )	controls.set_brake   ( 0, * to_brakeL );
    if ( NULL != to_brakeR )	controls.set_brake   ( 1, * to_brakeR );
				// Good! Brakes need half travel to act.

// No gear implemented yet!
//  if ( msk_gearmove & *to_gearmove )	controls.set_gear 
//						( 1 - controls.get_gear());

    if ( msk_flapup & *to_flapup )	controls.move_flaps (   0.02 );
    if ( msk_flapdn & *to_flapdn )	controls.move_flaps ( - 0.02 );
    if ( msk_eltrimup & *to_eltrimup )
				controls.move_elevator_trim (   0.005 );
    if ( msk_eltrimdn & *to_eltrimdn )
				controls.move_elevator_trim ( - 0.005 );

    //  Added by William Riley -- riley@technologist.com
    //  Modified by Alex Perry
    if ( NULL != to_throttle ) {
	    throttle_tmp=(- * to_throttle + 1) / 2;

	    if(sync_throttle == true) {
		if (fabs(controls.get_throttle(0)-throttle_tmp)
		    < SYNC_TOLERANCE)
		{
		    FG_LOG(FG_INPUT, FG_INFO, "  Throttle lever synchronized.");
	            controls.set_throttle(FGControls::ALL_ENGINES,throttle_tmp);
		    sync_throttle=false;
		}    
	    } else {
		controls.set_throttle( FGControls::ALL_ENGINES,throttle_tmp );
	    }
	} 

    if ( NULL != to_rudder )
	{
	    if ( current_options.get_auto_coordination() !=
		  fgOPTIONS::FG_AUTO_COORD_ENABLED ) 
	    {
	        double dead_zone = 0.0; // 0.4;
	        double value = * to_rudder;
		if (value < -dead_zone) {
		  value += dead_zone;
		  value *= 1.0 / (1.0 - dead_zone);
		} else if (value > dead_zone) {
		  value -= dead_zone;
		  value *= 1.0 / (1.0 - dead_zone);
		} else {
		  value = 0.0;
		}
		controls.set_rudder(value);
	    }
	}
    //  End of William's code

    // Use hat to set view direction
    // Alex Perry 2000-05-31, based on concept by dpm
    if ( NULL != to_viewhat )
    {	  double n = * ( to_viewhat + 1 );
	  double e = * ( to_viewhat     );
	  double d;
	  if ( fabs(n) + fabs(e) > 0.1 )
	  {	d = atan2 ( -e, -n );
		if ( d < 0 ) d += 2 * FG_PI;
		current_view.set_goal_view_offset ( d );
	  }
    }

    return 1;
}

#endif // ENABLE_PLIB_JOYSTICK

