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
#include <simgear/xgl/xgl.h>

#if defined(FX) && defined(XMESA)
#  include <GL/xmesa.h>
#endif

#include STL_STRING

#include <stdlib.h>
#include <string.h>

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/fgpath.hxx>
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
//#include <Main/views.hxx>
//#include <Network/network.h>
//#include <Time/fg_time.hxx>

#if defined( WIN32 ) && !defined( __CYGWIN__ )
#  include <simgear/screen/win32-printer.h>
#  include <simgear/screen/GlBitmaps.h>
#endif

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
static  int _mVtoggle;

// we break up the glutGetModifiers return mask
// once per loop and stash what we need in these
static int glut_active_shift;
static int glut_active_ctrl;
static int glut_active_alt;

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

void TurnCursorOn( void )
{
    mouse_active = ~0;
#if defined(WIN32_CURSOR_TWEAKS)
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
    glutWarpPointer( fgGetInt("/sim/startup/xsize")/2,
		     fgGetInt("/sim/startup/ysize")/2);
#endif
}

void TurnCursorOff( void )
{
    mouse_active = 0;
#if defined(WIN32_CURSOR_TWEAKS)
    glutSetCursor(GLUT_CURSOR_NONE);
#elif defined(X_CURSOR_TWEAKS)
    glutWarpPointer( fgGetInt("/sim/startup/xsize"),
		     fgGetInt("/sim/startup/ysize"));
#endif
}

void maybeToggleMouse( void )
{
#if defined(WIN32_CURSOR_TWEAKS)
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
	_savedX = fgGetInt("/sim/startup/xsize")/2;
	_savedY = fgGetInt("/sim/startup/ysize")/2;
	_mVtoggle = 0;
	Quat0();
	build_rotmatrix(GuiQuat_mat, curGuiQuat);
	glutSetCursor(GLUT_CURSOR_INHERIT);

	// Is this necessary ??
	if( !gui_menu_on )   TurnCursorOff();

	glutWarpPointer( _savedX, _savedY );
    }
    globals->get_current_view()->set_goal_view_offset(0.0);
    globals->get_current_view()->set_view_offset(0.0);
}


void guiMotionFunc ( int x, int y )
{
    int ww, wh, need_warp = 0;
    float W, H;
    double offset;

    if (mouse_mode == MOUSE_POINTER) {
        puMouse ( x, y ) ;
        glutPostRedisplay () ;
    } else {
        if( x == _mX && y == _mY)
            return;
        
        // reset left click MOUSE_VIEW toggle feature
        _mVtoggle = 0;
        
        ww = fgGetInt("/sim/startup/xsize");
        wh = fgGetInt("/sim/startup/ysize");
        
        switch (mouse_mode) {
            case MOUSE_YOKE:
                if( !mouse_joystick_control ) {
                    mouse_joystick_control = 1;
		    fgSetString("/sim/control-mode", "mouse");
                } else {
                    if ( left_button() ) {
                        offset = (_mX - x) * brake_sensitivity;
                        controls.move_brake(FGControls::ALL_WHEELS, offset);
                        offset = (_mY - y) * throttle_sensitivity;
                        controls.move_throttle(FGControls::ALL_ENGINES, offset);
                    } else if ( right_button() ) {
                        if( ! current_autopilot->get_HeadingEnabled() ) {
                            offset = (x - _mX) * rudder_sensitivity;
                            controls.move_rudder(offset);
                        }
                        if( ! current_autopilot->get_AltitudeEnabled() ) {
                            offset = (_mY - y) * trim_sensitivity;
                            controls.move_elevator_trim(offset);
                        }
                    } else {
                        if( ! current_autopilot->get_HeadingEnabled() ) {
                            offset = (x - _mX) * aileron_sensitivity;
                            controls.move_aileron(offset);
                        }
                        if( ! current_autopilot->get_AltitudeEnabled() ) {
                            offset = (_mY - y) * elevator_sensitivity;
                            controls.move_elevator(offset);
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
#define CONTRAINED_MOUSE_VIEW_Y
#ifdef CONTRAINED_MOUSE_VIEW_Y
                    y = 1;
#else
                    y = wh-2;
#endif // CONTRAINED_MOUSE_VIEW_Y
                    need_warp = 1;
                } else if( y >= wh-1) {
#ifdef CONTRAINED_MOUSE_VIEW_Y
                    y = wh-2;
#else
                    y = 1;
#endif // CONTRAINED_MOUSE_VIEW_Y
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
                // try to get SG_PI movement in each half of screen
                // do spherical pan
                W = ww;
                H = wh;
                if( middle_button() ) {
                    trackball(lastGuiQuat,
                              (2.0f * _mX - W) / W,
                              0, //(H - 2.0f * y) / H,         // 3
                              (2.0f * x - W) / W,
                              0 //(H - 2.0f * _mY) / H       // 1
                             );
                    x = _mX;
                    y = _mY;
                    need_warp = 1;
                } else {
                    trackball(lastGuiQuat,
                              0, //(2.0f * _mX - W) / W,  // 0
                              (H - 2.0f * y) / H,         // 3
                              0, //(2.0f * x - W) / W,    // 2
                              (H - 2.0f * _mY) / H        // 1 
                             );
                }
                add_quats(lastGuiQuat, curGuiQuat, curGuiQuat);
                build_rotmatrix(GuiQuat_mat, curGuiQuat);
                
                // do horizontal pan
                // this could be done in above quat
                // but requires redoing view pipeline
                offset = globals->get_current_view()->get_goal_view_offset();
                offset += ((_mX - x) * SG_2PI / W );
                while (offset < 0.0) {
                    offset += SG_2PI;
                }
                while (offset > SG_2PI) {
                    offset -= SG_2PI;
                }
                globals->get_current_view()->set_goal_view_offset(offset);
#ifdef NO_SMOOTH_MOUSE_VIEW
                globals->get_current_view()->set_view_offset(offset);
#endif
                break;
            
            default:
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
                        curGuiQuat[0] = _quat[0];
                        curGuiQuat[1] = _quat[1];
                        curGuiQuat[2] = _quat[2];
                        curGuiQuat[3] = _quat[3];
                        globals->get_current_view()->set_goal_view_offset(_view_offset);
#ifdef NO_SMOOTH_MOUSE_VIEW
                        globals->get_current_view()->set_view_offset(_view_offset);
#endif
                    } else {
                        // center view
                        _mVx = _mX;
                        _mVy = _mY;
                        _Vx = x;
                        _Vy = y;
                        _quat[0] = curGuiQuat[0];
                        _quat[1] = curGuiQuat[1];
                        _quat[2] = curGuiQuat[2];
                        _quat[3] = curGuiQuat[3];
                        x = fgGetInt("/sim/startup/xsize")/2;
                        y = fgGetInt("/sim/startup/ysize")/2;
                        Quat0();
                        _view_offset =
			    globals->get_current_view()->get_goal_view_offset();
                        globals->get_current_view()->set_goal_view_offset(0.0);
#ifdef NO_SMOOTH_MOUSE_VIEW
                        globals->get_current_view()->set_view_offset(0.0);
#endif
                    }
                    glutWarpPointer( x , y);
                    build_rotmatrix(GuiQuat_mat, curGuiQuat);
                    _mVtoggle = ~_mVtoggle;
                    break;
            }
        }else if ( button == GLUT_RIGHT_BUTTON) {
            switch (mouse_mode) {
                case MOUSE_POINTER:
                    mouse_mode = MOUSE_YOKE;
                    mouse_joystick_control = 0;
                    _savedX = x;
                    _savedY = y;
                    // start with zero point in center of screen
                    _mX = fgGetInt("/sim/startup/xsize")/2;
                    _mY = fgGetInt("/sim/startup/ysize")/2;
                    
                    // try to have the MOUSE_YOKE position
                    // reflect the current stick position
                    offset = controls.get_aileron();
                    x = _mX - (int)(offset * aileron_sensitivity);
                    offset = controls.get_elevator();
                    y = _mY - (int)(offset * elevator_sensitivity);
                    
                    glutSetCursor(GLUT_CURSOR_CROSSHAIR);
                    FG_LOG( FG_INPUT, FG_INFO, "Mouse in yoke mode" );
                    break;
                    
                case MOUSE_YOKE:
                    mouse_mode = MOUSE_VIEW;
		    fgSetString("/sim/control-mode", "joystick");
                    x = fgGetInt("/sim/startup/xsize")/2;
                    y = fgGetInt("/sim/startup/ysize")/2;
                    _mVtoggle = 0;
                    Quat0();
                    build_rotmatrix(GuiQuat_mat, curGuiQuat);
                    glutSetCursor(GLUT_CURSOR_LEFT_RIGHT);
                    FG_LOG( FG_INPUT, FG_INFO, "Mouse in view mode" );
                    break;
                    
                case MOUSE_VIEW:
                    mouse_mode = MOUSE_POINTER;
                    x = _savedX;
                    y = _savedY;
#ifdef RESET_VIEW_ON_LEAVING_MOUSE_VIEW
                    Quat0();
                    build_rotmatrix(GuiQuat_mat, curGuiQuat);
                    globals->get_current_view()->set_goal_view_offset(0.0);
#ifdef NO_SMOOTH_MOUSE_VIEW
                    globals->get_current_view()->set_view_offset(0.0);
#endif
#endif      // RESET_VIEW_ON_LEAVING_MOUSE_VIEW
                    glutSetCursor(GLUT_CURSOR_INHERIT);
                    
                    if(!gui_menu_on)
                        TurnCursorOff();
                    
                    FG_LOG( FG_INPUT, FG_INFO, "Mouse in pointer mode" );
                    break;
            }     
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
	current_panel->doMouseAction(button, updown, x, y);
      }
    }
    
    // Register the new position (if it
    // hasn't been registered already).
    _mX = x;
    _mY = y;
    
    glutPostRedisplay ();
}

