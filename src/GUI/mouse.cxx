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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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

#include <GL/glut.h>
#include <GL/gl.h>

#if defined(FX) && defined(XMESA)
#  include <GL/xmesa.h>
#endif

#include STL_STRING

#include <stdlib.h>
#include <string.h>

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/screen/screen-dump.hxx>

#include <Include/general.hxx>
//#include <Include/fg_memory.h>
#include <Aircraft/aircraft.hxx>
#include <Airports/simple.hxx>
//#include <Autopilot/auto_gui.hxx>
#include <Autopilot/newauto.hxx>
#include <Cockpit/panel.hxx>
#include <Controls/controls.hxx>
#include <FDM/flight.hxx>
#include <Main/fg_init.hxx>
#include <Main/fg_props.hxx>
#include <Main/viewmgr.hxx>

#include "gui.h"
#include "gui_local.hxx"

SG_USING_STD(string);

#ifndef SG_HAVE_NATIVE_SGI_COMPILERS
SG_USING_STD(cout);
#endif

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

// we break up the glutGetModifiers return mask
// once per loop and stash what we need in these
static int glut_active_shift = 0;
static int glut_active_ctrl = 0;
static int glut_active_alt = 0;

static int MOUSE_XSIZE = 0;
static int MOUSE_YSIZE = 0;

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
    return( last_buttons & (1 << GLUT_LEFT_BUTTON) );
}

static inline int middle_button( void ) {
    return( last_buttons & (1 << GLUT_MIDDLE_BUTTON) );
}

static inline int right_button( void ) {
    return( last_buttons & (1 << GLUT_RIGHT_BUTTON) );
}

static inline void set_goal_view_offset( float offset )
{
	globals->get_current_view()->set_goal_view_offset(offset);
}

static inline void set_view_offset( float offset )
{
	globals->get_current_view()->set_view_offset(offset);
}

static inline float get_view_offset() {
	return globals->get_current_view()->get_view_offset();
}

static inline float get_goal_view_offset() {
	return globals->get_current_view()->get_goal_view_offset();
}

static inline void set_goal_view_tilt( float tilt )
{
	globals->get_current_view()->set_goal_view_tilt(tilt);
}

static inline void set_view_tilt( float tilt )
{
	globals->get_current_view()->set_view_tilt(tilt);
}

static inline float get_view_tilt() {
	return globals->get_current_view()->get_view_tilt();
}

static inline void move_brake(float offset) {
	globals->get_controls()->move_brake(FGControls::ALL_WHEELS, offset);
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
	return current_autopilot->get_HeadingEnabled();
}

static inline bool AP_AltitudeEnabled() {
	return current_autopilot->get_AltitudeEnabled();
}

void TurnCursorOn( void )
{
    mouse_active = ~0;
#if defined(WIN32)
    switch (mouse_mode) {
        case MOUSE_POINTER:
            glutSetCursor(GLUT_CURSOR_INHERIT);
            break;
        case MOUSE_YOKE:
            glutSetCursor(GLUT_CURSOR_CROSSHAIR);
            break;
        case MOUSE_VIEW:
            glutSetCursor(GLUT_CURSOR_LEFT_RIGHT);
            break;
    }
#endif
#if defined(X_CURSOR_TWEAKS)
    glutWarpPointer( MOUSE_XSIZE/2,
		     MOUSE_YSIZE/2);
#endif
}

void TurnCursorOff( void )
{
    mouse_active = 0;
#if defined(WIN32_CURSOR_TWEAKS)
    glutSetCursor(GLUT_CURSOR_NONE);
#elif defined(X_CURSOR_TWEAKS)
    glutWarpPointer( MOUSE_XSIZE,
		     MOUSE_YSIZE);
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

// Call with FALSE to init and TRUE to restore
void BusyCursor( int restore )
{
    static GLenum cursor = (GLenum) 0;
    if( restore ) {
        glutSetCursor(cursor);
    } else {
        cursor = (GLenum) glutGet( (GLenum) GLUT_WINDOW_CURSOR );
#if defined(WIN32_CURSOR_TWEAKS)
        TurnCursorOn();
#endif
        glutSetCursor( GLUT_CURSOR_WAIT );
    }
}


// Center the view offsets
void CenterView( void ) {
    if( mouse_mode == MOUSE_VIEW ) {
	mouse_mode = MOUSE_POINTER;
	_savedX = MOUSE_XSIZE/2;
	_savedY = MOUSE_YSIZE/2;
	_mVtoggle = 0;
	Quat0();
	build_rotmatrix(GuiQuat_mat, curGuiQuat);
	glutSetCursor(GLUT_CURSOR_INHERIT);

	// Is this necessary ??
	if( !gui_menu_on )   TurnCursorOff();

	glutWarpPointer( _savedX, _savedY );
    }
    set_goal_view_offset(0.0);
    set_view_offset(0.0);
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

void guiMotionFunc ( int x, int y )
{
    int ww, wh, need_warp = 0;
    float W, H;
    double offset;

    ww = MOUSE_XSIZE;
    wh = MOUSE_YSIZE;

    if (mouse_mode == MOUSE_POINTER) {
        // TURN MENU ON IF MOUSE AT TOP
        if( y < 1 ) {
            if( !gui_menu_on )
                guiToggleMenu();			
        }
        // TURN MENU OFF IF MOUSE AT BOTTOM
        else if( y > wh-2 ) {
            if( gui_menu_on )
                guiToggleMenu();			
        }
        puMouse ( x, y ) ;
        glutPostRedisplay () ;
    } else {
        if( x == _mX && y == _mY)
            return;
        
        // reset left click MOUSE_VIEW toggle feature
        _mVtoggle = 0;
        
        switch (mouse_mode) {
            case MOUSE_YOKE:
                if( !mouse_joystick_control ) {
                    mouse_joystick_control = 1;
		    fgSetString("/sim/control-mode", "mouse");
                } else {
                    if ( left_button() ) {
                        move_brake(   (_mX - x) * brake_sensitivity);
                        move_throttle((_mY - y) * throttle_sensitivity);
                    } else if ( right_button() ) {
                        if( ! AP_HeadingEnabled() ) {
                            move_rudder((x - _mX) * rudder_sensitivity);
                        }
                        if( ! AP_AltitudeEnabled() ) {
                            move_elevator_trim((_mY - y) * trim_sensitivity);
                        }
                    } else {
                        if( ! AP_HeadingEnabled() ) {
                            move_aileron((x - _mX) * aileron_sensitivity);
                        }
                        if( ! AP_AltitudeEnabled() ) {
                            move_elevator((_mY - y) * elevator_sensitivity);
                        }
                    }
                }
                // Keep the mouse in the window.
                if (x < 5 || x > ww-5 || y < 5 || y > wh-5) {
                    x = ww / 2;
                    y = wh / 2;
                    need_warp = 1;
                }
                break;
                
            case MOUSE_VIEW:
                if( y <= 0 ) {
                    y = 1;
                    need_warp = 1;
                } else if( y >= wh-1) {
                    y = wh-2;
                    need_warp = 1;
                }
                // wrap MOUSE_VIEW mode cursor x position
                if ( x <= 0 ) {
                    need_warp = 1;
                    x = ww-2;
                } else if ( x >= ww-1 ) {
                    need_warp = 1;
                    x = 1;
                }

		{
		    float scale = SGD_PI / MOUSE_XSIZE;
		    float dx = (_mX - x) * scale;
		    float dy = (_mY - y) * scale;
		    
		    float newOffset = get_view_offset() + dx;
		    set_goal_view_offset(newOffset);
		    set_view_offset(newOffset);
		    
		    float newTilt = get_view_tilt() + dy;
		    set_goal_view_tilt(newTilt);
		    set_view_tilt(newTilt);
		}
                break;
        }
    }
    if( need_warp)
        glutWarpPointer(x, y);
    
    // Record the new mouse position.
    _mX = x;
    _mY = y;
}


void guiMouseFunc(int button, int updown, int x, int y)
{
    int glutModifiers;

    // private MOUSE_VIEW state variables
    // to allow alternate left clicks in MOUSE_VIEW mode
    // to toggle between current offsets and straight ahead
    // uses _mVtoggle
    static int _mVx, _mVy, _Vx, _Vy;
    static float _quat[4];
    static double _view_offset;
    
    // general purpose variables
    double offset;
            
    glutModifiers = glutGetModifiers();
    glut_active_shift = glutModifiers & GLUT_ACTIVE_SHIFT;
    glut_active_ctrl  = glutModifiers & GLUT_ACTIVE_CTRL; 
    glut_active_alt   = glutModifiers & GLUT_ACTIVE_ALT;
    
    // Was the left button pressed?
    if (updown == GLUT_DOWN ) {
        if( button == GLUT_LEFT_BUTTON)
        {
            switch (mouse_mode) {
                case MOUSE_POINTER:
                    break;
                case MOUSE_YOKE:
                    break;
                case MOUSE_VIEW:
                    if(_mVtoggle) {
                        // resume previous view offsets
                        _mX = _mVx;
                        _mY = _mVy;
                        x = _Vx;
                        y = _Vy;
                        sgCopyVec4(curGuiQuat, _quat);
                        set_goal_view_offset(_view_offset);
#ifdef NO_SMOOTH_MOUSE_VIEW
                        set_view_offset(_view_offset);
#endif
                    } else {
                        // center view
                        _mVx = _mX;
                        _mVy = _mY;
                        _Vx = x;
                        _Vy = y;
                        sgCopyVec4(_quat,curGuiQuat);
                        x = MOUSE_XSIZE/2;
                        y = MOUSE_YSIZE/2;
                        Quat0();
                        _view_offset = get_goal_view_offset();
                        set_goal_view_offset(0.0);
#ifdef NO_SMOOTH_MOUSE_VIEW
                        set_view_offset(0.0);
#endif
                    }
                    glutWarpPointer( x , y);
                    build_rotmatrix(GuiQuat_mat, curGuiQuat);
                    _mVtoggle = ~_mVtoggle;
                    break;
            }
        } else if ( button == GLUT_RIGHT_BUTTON) {
            switch (mouse_mode) {
				
                case MOUSE_POINTER:
                    SG_LOG( SG_INPUT, SG_INFO, "Mouse in yoke mode" );
					
                    mouse_mode = MOUSE_YOKE;
                    mouse_joystick_control = 0;
                    _savedX = x;
                    _savedY = y;
                    // start with zero point in center of screen
                    _mX = MOUSE_XSIZE/2;
                    _mY = MOUSE_YSIZE/2;
                    
                    // try to have the MOUSE_YOKE position
                    // reflect the current stick position
                    x = _mX - (int)(get_aileron() * aileron_sensitivity);
                    y = _mY - (int)(get_elevator() * elevator_sensitivity);
                    
                    glutSetCursor(GLUT_CURSOR_CROSSHAIR);
                    break;
                    
                case MOUSE_YOKE:
                    SG_LOG( SG_INPUT, SG_INFO, "Mouse in view mode" );
					
                    mouse_mode = MOUSE_VIEW;
                    fgSetString("/sim/control-mode", "joystick");
					
					// recenter cursor and reset 
                    x = MOUSE_XSIZE/2;
                    y = MOUSE_YSIZE/2;
                    _mVtoggle = 0;
// #ifndef RESET_VIEW_ON_LEAVING_MOUSE_VIEW
                    Quat0();
                    build_rotmatrix(GuiQuat_mat, curGuiQuat);
// #endif
                    glutSetCursor(GLUT_CURSOR_LEFT_RIGHT);
                    break;
                    
                case MOUSE_VIEW:
                    SG_LOG( SG_INPUT, SG_INFO, "Mouse in pointer mode" );
					
                    mouse_mode = MOUSE_POINTER;
                    x = _savedX;
                    y = _savedY;
#ifdef RESET_VIEW_ON_LEAVING_MOUSE_VIEW
                    Quat0();
                    build_rotmatrix(GuiQuat_mat, curGuiQuat);
                    set_goal_view_offset(0.0);
#ifdef NO_SMOOTH_MOUSE_VIEW
                    set_view_offset(0.0);
#endif // NO_SMOOTH_MOUSE_VIEW
#endif // RESET_VIEW_ON_LEAVING_MOUSE_VIEW
                    glutSetCursor(GLUT_CURSOR_INHERIT);
                    
#if defined(WIN32_CURSOR_TWEAKS_OFF)
                    if(!gui_menu_on)
                        TurnCursorOff();
#endif // WIN32_CURSOR_TWEAKS_OFF
                    break;
            } // end switch (mouse_mode)
            glutWarpPointer( x, y );
        } // END RIGHT BUTTON
    } // END UPDOWN == GLUT_DOWN
    
    // Note which button is pressed.
    if ( updown == GLUT_DOWN ) {
        last_buttons |=  ( 1 << button ) ;
    } else {
        last_buttons &= ~( 1 << button ) ;
    }
    
    // If we're in pointer mode, let PUI
    // know what's going on.
    if (mouse_mode == MOUSE_POINTER) {
      if (!puMouse (button, updown, x,y)) {
        if ( current_panel != NULL ) {
          current_panel->doMouseAction(button, updown, x, y);
        }
      }
    }
    
    // Register the new position (if it
    // hasn't been registered already).
    _mX = x;
    _mY = y;
    
    glutPostRedisplay ();
}

