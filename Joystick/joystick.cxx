// joystick.cxx -- joystick support
//
// Written by Curtis Olson, started October 1998.
//
// Copyright (C) 1998  Curtis L. Olson - curt@me.umn.edu
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


#include <Aircraft/aircraft.hxx>
#include <Debug/fg_debug.h>
#include <Joystick/js.hxx>

#include "joystick.hxx"


// joystick classes
static jsJoystick *js0;
static jsJoystick *js1;

// these will hold the values of the axes
static float *js_ax0, *js_ax1;


// Initialize the joystick(s)
int fgJoystickInit( void ) {

    fgPrintf( FG_INPUT, FG_INFO, "Initializing joystick\n");

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

	fgPrintf ( FG_INPUT, FG_INFO, 
		   "  Joystick 0 detected with %d axes\n",
		   js0->getNumAxes() );
    }

    if ( js1->notWorking () ) {
	// not working
    } else {
	// allocate storage for axes values
	js_ax1 = new float [ js1->getNumAxes() ];

	// configure
	js1->setDeadBand( 0, 0.1 );
	js1->setDeadBand( 1, 0.1 );

	fgPrintf ( FG_INPUT, FG_INFO,
		   "  Joystick 1 detected with %d axes\n",
		   js1->getNumAxes() );
    }

    if ( js0->notWorking() && js1->notWorking() ) {
	fgPrintf ( FG_INPUT, FG_INFO, "  No joysticks detected\n" );
	return 0;
    }

    return 1;
}


// update the control parameters based on joystick intput
int fgJoystickRead( void ) {
    fgCONTROLS *c;
    int b;

    c = current_aircraft.controls;

    if ( ! js0->notWorking() ) {
	js0->read( &b, js_ax0 ) ;
	fgAileronSet( js_ax0[0] );
	fgElevSet( -js_ax0[1] );
    }

    if ( ! js1->notWorking() ) {
	js1->read( &b, js_ax1 ) ;
	fgRudderSet( js_ax1[0] );
	fgThrottleSet(FG_Throttle_All, -js_ax1[1] * 1.05 );
    }

    return 1;
}


// $Log$
// Revision 1.2  1998/10/25 10:56:25  curt
// Completely rewritten to use Steve Baker's joystick interface class.
//
// Revision 1.1  1998/10/24 22:28:16  curt
// Renamed joystick.[ch] to joystick.[ch]xx
// Added js.hxx which is Steve's joystick interface class.
//
// Revision 1.7  1998/04/25 22:06:29  curt
// Edited cvs log messages in source files ... bad bad bad!
//
// Revision 1.6  1998/04/18 04:14:05  curt
// Moved fg_debug.c to it's own library.
//
// Revision 1.5  1998/02/12 21:59:44  curt
// Incorporated code changes contributed by Charlie Hotchkiss
// <chotchkiss@namg.us.anritsu.com>
//
// Revision 1.4  1998/02/03 23:20:20  curt
// Lots of little tweaks to fix various consistency problems discovered by
// Solaris' CC.  Fixed a bug in fg_debug.c with how the fgPrintf() wrapper
// passed arguments along to the real printf().  Also incorporated HUD changes
// by Michele America.
//
// Revision 1.3  1998/01/27 00:47:54  curt
// Incorporated Paul Bleisch's <pbleisch@acm.org> new debug message
// system and commandline/config file processing code.
//
// Revision 1.2  1997/12/30 20:47:40  curt
// Integrated new event manager with subsystem initializations.
//
// Revision 1.1  1997/08/29 18:06:54  curt
// Initial revision.
//

