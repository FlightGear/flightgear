/**************************************************************************
 * gui.cxx
 *
 * Written 1998 by Durk Talsma, started Juni, 1998.  For the flight gear
 * project.
 *
 * Additional mouse supported added by David Megginson, 1999.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * $Id$
 **************************************************************************/


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <Main/fg_os.hxx>

#if defined(FX) && defined(XMESA)
#  include <GL/xmesa.h>
#endif

#include STL_STRING

#include <stdlib.h>
#include <string.h>

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sg_path.hxx>

#include <Include/general.hxx>
#include <Aircraft/aircraft.hxx>
#include <Aircraft/controls.hxx>
#include <Airports/simple.hxx>
#include <Cockpit/panel.hxx>
#include <FDM/flight.hxx>
#include <Main/fg_init.hxx>
#include <Main/fg_props.hxx>
#include <Main/viewmgr.hxx>

#include "gui.h"

SG_USING_STD(string);
SG_USING_STD(cout);

/* --------------------------------------------------------------------
Mouse stuff
---------------------------------------------------------------------*/

#if defined(WIN32) || defined(__CYGWIN32__)
#define WIN32_CURSOR_TWEAKS
// uncomment this for cursor to turn off when menu is disabled
// #define WIN32_CURSOR_TWEAKS_OFF
#elif (GLUT_API_VERSION >= 4 || GLUT_XLIB_IMPLEMENTATION >= 9)
#define X_CURSOR_TWEAKS
#endif

static int mouse_active = 0;

static int MOUSE_XSIZE = 0;
static int MOUSE_YSIZE = 0;

typedef enum {
	MOUSE_POINTER,
	MOUSE_YOKE,
	MOUSE_VIEW
} MouseMode;

/* --------------------------------------------------------------------
Support for mouse as control yoke (david@megginson.com)

- right button toggles between pointer and yoke
- horizontal drag with no buttons moves ailerons
- vertical drag with no buttons moves elevators
- horizontal drag with left button moves brakes (left=on)
- vertical drag with left button moves throttle (up=more)
- horizontal drag with middle button moves rudder
- vertical drag with middle button moves trim

For the *_sensitivity variables, a lower number means more sensitive.

TODO: figure out how to keep pointer from leaving window in yoke mode.
TODO: add thresholds and null zones
TODO: sensitivity should be configurable at user option.
TODO: allow differential braking (this will be useful if FlightGear
      ever supports tail-draggers like the DC-3)
---------------------------------------------------------------------*/

MouseMode mouse_mode = MOUSE_POINTER;

void guiInitMouse(int width, int height)
{
	MOUSE_XSIZE = width;
	MOUSE_YSIZE = height;
}

void TurnCursorOn( void )
{
    mouse_active = ~0;
#if defined(WIN32)
    switch (mouse_mode) {
        case MOUSE_POINTER:
            fgSetMouseCursor(MOUSE_CURSOR_POINTER);
            break;
        case MOUSE_YOKE:
            fgSetMouseCursor(MOUSE_CURSOR_CROSSHAIR);
            break;
        case MOUSE_VIEW:
            fgSetMouseCursor(MOUSE_CURSOR_LEFTRIGHT);
            break;
    }
#endif
#if defined(X_CURSOR_TWEAKS)
    fgWarpMouse( MOUSE_XSIZE/2, MOUSE_YSIZE/2 );
#endif
}

void TurnCursorOff( void )
{
    mouse_active = 0;
#if defined(WIN32_CURSOR_TWEAKS)
    fgSetMouseCursor(MOUSE_CURSOR_NONE);
#elif defined(X_CURSOR_TWEAKS)
    fgWarpMouse( MOUSE_XSIZE, MOUSE_YSIZE );
#endif
}

void maybeToggleMouse( void )
{
#if defined(WIN32_CURSOR_TWEAKS_OFF)
    static int first_time = ~0;
    static int mouse_changed = 0;

    if ( first_time ) {
        if(!mouse_active) {
            mouse_changed = ~mouse_changed;
            TurnCursorOn();
        }
    } else {
        if( mouse_mode != MOUSE_POINTER )
            return;
        if( mouse_changed ) {
            mouse_changed = ~mouse_changed;
            if(mouse_active) {
                TurnCursorOff();
            }
        }
    }
    first_time = ~first_time;
#endif // #ifdef WIN32
}

