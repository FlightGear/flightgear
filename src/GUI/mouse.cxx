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
#include <Autopilot/auto_gui.hxx>
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

// Call with FALSE to init and TRUE to restore
void BusyCursor( int restore )
{
    static int cursor = MOUSE_CURSOR_POINTER;
    if( restore ) {
        fgSetMouseCursor(cursor);
    } else {
        cursor = fgGetMouseCursor();
#if defined(WIN32_CURSOR_TWEAKS)
        TurnCursorOn();
#endif
        fgSetMouseCursor( MOUSE_CURSOR_WAIT );
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
	fgSetMouseCursor(MOUSE_CURSOR_POINTER);

	// Is this necessary ??
#if defined(FG_OLD_MENU)
	if( !gui_menu_on )   TurnCursorOff();
#endif

	fgWarpMouse( _savedX, _savedY );
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
#if defined(FG_OLD_MENU)
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
#endif
        puMouse ( x, y ) ;
        fgRequestRedraw () ;
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
                // try to get SGD_PI movement in each half of screen
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
                offset = get_goal_view_offset();
                offset += ((_mX - x) * SGD_2PI / W );
                while (offset < 0.0) {
                    offset += SGD_2PI;
                }
                while (offset > SGD_2PI) {
                    offset -= SGD_2PI;
                }
                set_goal_view_offset(offset);
                set_goal_view_tilt(asin( GuiQuat_mat[1][2]) * SGD_RADIANS_TO_DEGREES );
#ifdef NO_SMOOTH_MOUSE_VIEW
                set_view_offset(offset);
                set_view_tilt(asin( GuiQuat_mat[1][2]) * SGD_RADIANS_TO_DEGREES );
#endif
                break;
            
            default:
                break;
        }
    }
    if( need_warp)
        fgWarpMouse(x, y);
    
    // Record the new mouse position.
    _mX = x;
    _mY = y;
}


void guiMouseFunc(int button, int updown, int x, int y)
{

    // private MOUSE_VIEW state variables
    // to allow alternate left clicks in MOUSE_VIEW mode
    // to toggle between current offsets and straight ahead
    // uses _mVtoggle
    static int _mVx, _mVy, _Vx, _Vy;
    static float _quat[4];
    static double _view_offset;
    
    // Was the left button pressed?
    if ( updown == MOUSE_BUTTON_DOWN ) {
        if( button == MOUSE_BUTTON_LEFT)
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
                        set_goal_view_tilt(0.0);
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
                        set_goal_view_tilt(0.0);
#ifdef NO_SMOOTH_MOUSE_VIEW
                        set_view_offset(0.0);
                        set_view_tilt(0.0);
#endif
                    }
                    fgWarpMouse( x , y);
                    build_rotmatrix(GuiQuat_mat, curGuiQuat);
                    _mVtoggle = ~_mVtoggle;
                    break;
            }
        } else if ( button == MOUSE_BUTTON_RIGHT) {
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
                    
                    fgSetMouseCursor(MOUSE_CURSOR_CROSSHAIR);
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
                    fgSetMouseCursor(MOUSE_CURSOR_LEFTRIGHT);
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
                    set_goal_view_tilt(0.0);
#ifdef NO_SMOOTH_MOUSE_VIEW
                    set_view_offset(0.0);
                    set_view_tilt(0.0);
#endif // NO_SMOOTH_MOUSE_VIEW
#endif // RESET_VIEW_ON_LEAVING_MOUSE_VIEW
                    fgSetMouseCursor(MOUSE_CURSOR_POINTER);

#if defined(FG_OLD_MENU)                    
#if defined(WIN32_CURSOR_TWEAKS_OFF)
                    if(!gui_menu_on)
                        TurnCursorOff();
#endif // WIN32_CURSOR_TWEAKS_OFF
#endif // FG_OLD_MENU
                    break;
            } // end switch (mouse_mode)
            fgWarpMouse( x, y );
        } // END RIGHT BUTTON
    } // END MOUSE DOWN
    
    // Update the button state
    if ( updown == MOUSE_BUTTON_DOWN ) {
        last_buttons |=  ( 1 << button ) ;
    } else {
        last_buttons &= ~( 1 << button ) ;
    }
    
    // If we're in pointer mode, let PUI
    // know what's going on.
    if (mouse_mode == MOUSE_POINTER) {
      if (!puMouse (button, updown , x,y)) {
        if ( globals->get_current_panel() != NULL ) {
          globals->get_current_panel()->doMouseAction(button, updown, x, y);
        }
      }
    }
    
    // Register the new position (if it
    // hasn't been registered already).
    _mX = x;
    _mY = y;
    
    fgRequestRedraw ();
}




