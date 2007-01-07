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

#ifdef SG_MATH_EXCEPTION_CLASH
#  include <math.h>
#endif

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
#include "gui_local.hxx"

SG_USING_STD(string);
SG_USING_STD(cout);

/* --------------------------------------------------------------------
Mouse stuff
---------------------------------------------------------------------*/

static int _mX = 0;
static int _mY = 0;
static int _savedX = 0;
static int _savedY = 0;
static int last_buttons = 0 ;
static int mouse_active = 0;
static int mouse_joystick_control = 0;

//static time_t mouse_off_time;
//static int mouse_timed_out;

// to allow returning to previous view
// on second left click in MOUSE_VIEW mode
// This has file scope so that it can be reset
// if the little rodent is moved  NHV
static  int _mVtoggle = 0;

static int MOUSE_XSIZE = 0;
static int MOUSE_YSIZE = 0;

// uncomment this for view to exactly follow mouse in MOUSE_VIEW mode
// else smooth out the view panning to .01 radian per frame
// see view_offset smoothing mechanism in main.cxx
#define NO_SMOOTH_MOUSE_VIEW

// uncomment following to
#define RESET_VIEW_ON_LEAVING_MOUSE_VIEW

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

static double aileron_sensitivity = 1.0/500.0;
static double elevator_sensitivity = 1.0/500.0;
static double brake_sensitivity = 1.0/250.0;
static double throttle_sensitivity = 1.0/250.0;
static double rudder_sensitivity = 1.0/500.0;
static double trim_sensitivity = 1.0/1000.0;

void guiInitMouse(int width, int height)
{
	MOUSE_XSIZE = width;
	MOUSE_YSIZE = height;
}

static inline int guiGetMouseButton(void)
{
	return last_buttons;
}

static inline void guiGetMouse(int *x, int *y)
{
	*x = _mX;
	*y = _mY;
};

static inline int left_button( void ) {
    return( last_buttons & (1 << MOUSE_BUTTON_LEFT) );
}

static inline int middle_button( void ) {
    return( last_buttons & (1 << MOUSE_BUTTON_MIDDLE) );
}

static inline int right_button( void ) {
    return( last_buttons & (1 << MOUSE_BUTTON_RIGHT) );
}

static inline void set_goal_view_offset( float offset )
{
	globals->get_current_view()->setGoalHeadingOffset_deg(offset * SGD_RADIANS_TO_DEGREES);
}

static inline void set_view_offset( float offset )
{
	globals->get_current_view()->setHeadingOffset_deg(offset * SGD_RADIANS_TO_DEGREES);
}

static inline void set_goal_view_tilt( float tilt )
{
	globals->get_current_view()->setGoalPitchOffset_deg(tilt);
}

static inline void set_view_tilt( float tilt )
{
	globals->get_current_view()->setPitchOffset_deg(tilt);
}

static inline float get_view_offset() {
	return globals->get_current_view()->getHeadingOffset_deg() * SGD_DEGREES_TO_RADIANS;
}

static inline float get_goal_view_offset() {
	return globals->get_current_view()->getGoalHeadingOffset_deg() * SGD_DEGREES_TO_RADIANS;
}

static inline void move_brake(float offset) {
	globals->get_controls()->move_brake_left(offset);
	globals->get_controls()->move_brake_right(offset);
}

static inline void move_throttle(float offset) {
	globals->get_controls()->move_throttle(FGControls::ALL_ENGINES, offset);
}

static inline void move_rudder(float offset) {
	globals->get_controls()->move_rudder(offset);
}

static inline void move_elevator_trim(float offset) {
	globals->get_controls()->move_elevator_trim(offset);
}

static inline void move_aileron(float offset) {
	globals->get_controls()->move_aileron(offset);
}

static inline void move_elevator(float offset) {
	globals->get_controls()->move_elevator(offset);
}

static inline float get_aileron() {
	return globals->get_controls()->get_aileron();
}

static inline float get_elevator() {
	return globals->get_controls()->get_elevator();
}

static inline bool AP_HeadingEnabled() {
    static const SGPropertyNode *heading_enabled
        = fgGetNode("/autopilot/locks/heading");
    return ( strcmp( heading_enabled->getStringValue(), "" ) != 0 );
}

static inline bool AP_AltitudeEnabled() {
    static const SGPropertyNode *altitude_enabled
        = fgGetNode("/autopilot/locks/altitude");
    return ( strcmp( altitude_enabled->getStringValue(), "" ) != 0 );
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

//#define TRANSLATE_HUD
// temporary hack until pitch_offset is added to view pipeline
void fgTranslateHud( void ) {
#ifdef TRANSLATE_HUD
    if(mouse_mode == MOUSE_VIEW) {

        int ww = MOUSE_XSIZE;
        int wh = MOUSE_YSIZE;

        float y = 4*(_mY-(wh/2));// * ((wh/SGD_PI)*SG_RADIANS_TO_DEGREES);
	
        float x =  get_view_offset() * SG_RADIANS_TO_DEGREES;

        if( x < -180 )	    x += 360;
        else if( x > 180 )	x -= 360;

        x *= ww/90.0;
        //	x *= ww/180.0;
        //	x *= ww/360.0;

        //	glTranslatef( x*ww/640, y*wh/480, 0 );
        glTranslatef( x*640/ww, y*480/wh, 0 );
    }
#endif // TRANSLATE_HUD
}

