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

#ifdef FG_MATH_EXCEPTION_CLASH
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

#include STL_FSTREAM
#include STL_STRING

#include <stdlib.h>
#include <string.h>

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/fgpath.hxx>
#include <simgear/screen/screen-dump.hxx>

#include <Include/general.hxx>
#include <Aircraft/aircraft.hxx>
#include <Airports/simple.hxx>
#include <Autopilot/auto_gui.hxx>
#include <Autopilot/newauto.hxx>
#include <Cockpit/panel.hxx>
#include <Controls/controls.hxx>
#include <FDM/flight.hxx>
#include <Main/bfi.hxx>
#include <Main/fg_init.hxx>
#include <Main/fg_io.hxx>
#include <Main/globals.hxx>
#include <Main/options.hxx>
#include <Main/save.hxx>
#ifdef FG_NETWORK_OLK
#include <NetworkOLK/network.h>
#endif

#if defined( WIN32 ) && !defined( __CYGWIN__ )
#  include <simgear/screen/win32-printer.h>
#  include <simgear/screen/GlBitmaps.h>
#endif

/*
 * trackball.h
 * A virtual trackball implementation
 * Written by Gavin Bell for Silicon Graphics, November 1988.
 */
/*
 * Pass the x and y coordinates of the last and current positions of
 * the mouse, scaled so they are from (-1.0 ... 1.0).
 *
 * The resulting rotation is returned as a quaternion rotation in the
 * first paramater.
 */
void
trackball(float q[4], float p1x, float p1y, float p2x, float p2y);

/*
 * Given two quaternions, add them together to get a third quaternion.
 * Adding quaternions to get a compound rotation is analagous to adding
 * translations to get a compound translation.  When incrementally
 * adding rotations, the first argument here should be the new

  * rotation, the second and third the total rotation (which will be
 * over-written with the resulting new total rotation).
 */
void
add_quats(float *q1, float *q2, float *dest);

/*
 * A useful function, builds a rotation matrix in Matrix based on
 * given quaternion.
 */
void
build_rotmatrix(float m[4][4], float q[4]);

/*
 * This function computes a quaternion based on an axis (defined by
 * the given vector) and an angle about which to rotate.  The angle is
 * expressed in radians.  The result is put into the third argument.
 */
void
axis_to_quat(float a[3], float phi, float q[4]);


#include "gui.h"

FG_USING_STD(string);

#ifndef FG_HAVE_NATIVE_SGI_COMPILERS
FG_USING_STD(cout);
#endif

#if defined(WIN32) || defined(__CYGWIN32__)
#define WIN32_CURSOR_TWEAKS
#elif (GLUT_API_VERSION >= 4 || GLUT_XLIB_IMPLEMENTATION >= 9)
#define X_CURSOR_TWEAKS
#endif

// hack, should come from an include someplace
extern void fgInitVisuals( void );
extern void fgReshape( int width, int height );
extern void fgRenderFrame( void );

puFont guiFnt = 0;
fntTexFont *guiFntHandle = 0;

static puMenuBar    *mainMenuBar = 0;
//static puButton     *hideMenuButton = 0;

static puDialogBox  *dialogBox = 0;
static puFrame      *dialogFrame = 0;
static puText       *dialogBoxMessage = 0;
static puOneShot    *dialogBoxOkButton = 0;


static puDialogBox  *YNdialogBox = 0;
static puFrame      *YNdialogFrame = 0;
static puText       *YNdialogBoxMessage = 0;
static puOneShot    *YNdialogBoxOkButton = 0;
static puOneShot    *YNdialogBoxNoButton = 0;

static char msg_OK[]     = "OK";
static char msg_NO[]     = "NO";
static char msg_YES[]    = "YES";
static char msg_CANCEL[] = "Cancel";
static char msg_RESET[]  = "Reset";

char *gui_msg_OK;     // "OK"
char *gui_msg_NO;     // "NO"
char *gui_msg_YES;    // "YES"
char *gui_msg_CANCEL; // "CANCEL"
char *gui_msg_RESET;  // "RESET"

static char global_dialog_string[256];

// from autopilot.cxx
// extern void NewAltitude( puObject *cb );
// extern void NewHeading( puObject *cb );
// extern void fgAPAdjust( puObject * );
// extern void NewTgtAirport( puObject *cb );
// bool fgAPTerrainFollowEnabled( void );
// bool fgAPAltitudeEnabled( void );
// bool fgAPHeadingEnabled( void );
// bool fgAPWayPointEnabled( void );
// bool fgAPAutoThrottleEnabled( void );

// from cockpit.cxx
extern void fgLatLonFormatToggle( puObject *);

/* --------------------------------------------------------------------
Mouse stuff
---------------------------------------------------------------------*/

static int _mX = 0;
static int _mY = 0;
static int _savedX = 0;
static int _savedY = 0;
static int last_buttons = 0 ;
static int mouse_active = 0;
static int menu_on = 0;
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

static float lastquat[4];
static float curquat[4];
static float _quat0[4];
float quat_mat[4][4];

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

typedef enum {
    MOUSE_POINTER,
    MOUSE_YOKE,
    MOUSE_VIEW
} MouseMode;

MouseMode mouse_mode = MOUSE_POINTER;

static double aileron_sensitivity = 1.0/500.0;
static double elevator_sensitivity = 1.0/500.0;
static double brake_sensitivity = 1.0/250.0;
static double throttle_sensitivity = 1.0/250.0;
static double rudder_sensitivity = 1.0/500.0;
static double trim_sensitivity = 1.0/1000.0;

static inline void Quat0( void ) {
    curquat[0] = _quat0[0];
    curquat[1] = _quat0[1];
    curquat[2] = _quat0[2];
    curquat[3] = _quat0[3];
}

static inline int left_button( void ) {
    return( last_buttons & (1 << GLUT_LEFT_BUTTON) );
}

static inline int middle_button( void ) {
    return( last_buttons & (1 << GLUT_MIDDLE_BUTTON) );
}

static inline int right_button( void ) {
    return( last_buttons & (1 << GLUT_RIGHT_BUTTON) );
}

static inline void TurnCursorOn( void )
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
    glutWarpPointer( globals->get_current_view()->get_winWidth()/2,
		     globals->get_current_view()->get_winHeight()/2);
#endif
}

static inline void TurnCursorOff( void )
{
    mouse_active = 0;
#if defined(WIN32_CURSOR_TWEAKS)
    glutSetCursor(GLUT_CURSOR_NONE);
#elif defined(X_CURSOR_TWEAKS)
    glutWarpPointer( globals->get_current_view()->get_winWidth(),
		     globals->get_current_view()->get_winHeight());
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

int guiGetMouseButton(void)
{
    return last_buttons;
}

void guiGetMouse(int *x, int *y)
{
    *x = _mX;
    *y = _mY;
};

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
        
        ww = globals->get_current_view()->get_winWidth();
        wh = globals->get_current_view()->get_winHeight();
        
        switch (mouse_mode) {
            case MOUSE_YOKE:
                if( !mouse_joystick_control ) {
                    mouse_joystick_control = 1;
                    globals->get_options()->set_control_mode( FGOptions::FG_MOUSE );
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
                // try to get FG_PI movement in each half of screen
                // do spherical pan
                W = ww;
                H = wh;
                if( middle_button() ) {
                    trackball(lastquat,
                              (2.0f * _mX - W) / W,
                              0, //(H - 2.0f * y) / H,         // 3
                              (2.0f * x - W) / W,
                              0 //(H - 2.0f * _mY) / H       // 1
                             );
                    x = _mX;
                    y = _mY;
                    need_warp = 1;
                } else {
                    trackball(lastquat,
                              0, //(2.0f * _mX - W) / W,  // 0
                              (H - 2.0f * y) / H,         // 3
                              0, //(2.0f * x - W) / W,    // 2
                              (H - 2.0f * _mY) / H        // 1 
                             );
                }
                add_quats(lastquat, curquat, curquat);
                build_rotmatrix(quat_mat, curquat);
                
                // do horizontal pan
                // this could be done in above quat
                // but requires redoing view pipeline
                offset = globals->get_current_view()->get_goal_view_offset();
                offset += ((_mX - x) * FG_2PI / W );
                while (offset < 0.0) {
                    offset += FG_2PI;
                }
                while (offset > FG_2PI) {
                    offset -= FG_2PI;
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
                        curquat[0] = _quat[0];
                        curquat[1] = _quat[1];
                        curquat[2] = _quat[2];
                        curquat[3] = _quat[3];
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
                        _quat[0] = curquat[0];
                        _quat[1] = curquat[1];
                        _quat[2] = curquat[2];
                        _quat[3] = curquat[3];
                        x = globals->get_current_view()->get_winWidth()/2;
                        y = globals->get_current_view()->get_winHeight()/2;
                        Quat0();
                        _view_offset = globals->get_current_view()->get_goal_view_offset();
                        globals->get_current_view()->set_goal_view_offset(0.0);
#ifdef NO_SMOOTH_MOUSE_VIEW
                        globals->get_current_view()->set_view_offset(0.0);
#endif
                    }
                    glutWarpPointer( x , y);
                    build_rotmatrix(quat_mat, curquat);
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
                    _mX = globals->get_current_view()->get_winWidth()/2;
                    _mY = globals->get_current_view()->get_winHeight()/2;
                    
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
                    globals->get_options()->set_control_mode( FGOptions::FG_JOYSTICK );
                    x = globals->get_current_view()->get_winWidth()/2;
                    y = globals->get_current_view()->get_winHeight()/2;
                    _mVtoggle = 0;
                    Quat0();
                    build_rotmatrix(quat_mat, curquat);
                    glutSetCursor(GLUT_CURSOR_LEFT_RIGHT);
                    FG_LOG( FG_INPUT, FG_INFO, "Mouse in view mode" );
                    break;
                    
                case MOUSE_VIEW:
                    mouse_mode = MOUSE_POINTER;
                    x = _savedX;
                    y = _savedY;
#ifdef RESET_VIEW_ON_LEAVING_MOUSE_VIEW
                    Quat0();
                    build_rotmatrix(quat_mat, curquat);
                    globals->get_current_view()->set_goal_view_offset(0.0);
#ifdef NO_SMOOTH_MOUSE_VIEW
                    globals->get_current_view()->set_view_offset(0.0);
#endif
#endif      // RESET_VIEW_ON_LEAVING_MOUSE_VIEW
                    glutSetCursor(GLUT_CURSOR_INHERIT);
                    
                    if(!menu_on)
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

/* ================ General Purpose Functions ================ */

// Intercept the Escape Key
void ConfirmExitDialog(void)
{
    FG_PUSH_PUI_DIALOG( YNdialogBox );
}

// General Purpose Message Box
void mkDialog (const char *txt)
{
    strncpy(global_dialog_string, txt, 256);
    dialogBoxMessage->setLabel(global_dialog_string);
    FG_PUSH_PUI_DIALOG( dialogBox );
}

// Toggle the Menu and Mouse display state
void guiToggleMenu(void)
{
    if( menu_on ) {
        // printf("Hiding Menu\n");
        mainMenuBar->hide  ();
#if defined(WIN32_CURSOR_TWEAKS)
        if( mouse_mode == MOUSE_POINTER )
            TurnCursorOff();
#endif // #ifdef WIN32_CURSOR_TWEAKS
    } else {
        // printf("Showing Menu\n");
        mainMenuBar->reveal();
#ifdef WIN32
        TurnCursorOn();
#endif // #ifdef WIN32
    }
    menu_on = ~menu_on;
}
    
/* -----------------------------------------------------------------------
the Gui callback functions 
____________________________________________________________________*/

static void saveFlight(puObject *cv)
{
    BusyCursor(0);
    ofstream output("fgfs.sav");
    if (output.good() && fgSaveFlight(output)) {
      output.close();
      mkDialog("Saved flight to ./fgfs.sav");
      FG_LOG(FG_INPUT, FG_INFO, "Saved flight to fgfs.sav");
    } else {
      mkDialog("Cannot save flight to ./fgfs.sav");
      FG_LOG(FG_INPUT, FG_ALERT, "Cannot save flight to fgfs.sav");
    }
    BusyCursor(1);
}

static void loadFlight(puObject *cb)
{
    BusyCursor(0);
    ifstream input("fgfs.sav");
    if (input.good() && fgLoadFlight(input)) {
      input.close();
      mkDialog("Loaded flight from fgfs.sav");
      FG_LOG(FG_INPUT, FG_INFO, "Restored flight from ./fgfs.sav");
    } else {
      mkDialog("Failed to load flight from fgfs.sav");
      FG_LOG(FG_INPUT, FG_ALERT, "Cannot load flight from ./fgfs.sav");
    }
    BusyCursor(1);
}

void reInit(puObject *cb)
{
    BusyCursor(0);
    Quat0();
    build_rotmatrix(quat_mat, curquat);
    fgReInitSubsystems();
    BusyCursor(1);
}

static void toggleClouds(puObject *cb)
{
    FGBFI::setClouds( !FGBFI::getClouds() );
}
	
// This is the accessor function
void guiTogglePanel(puObject *cb)
{
    globals->get_options()->toggle_panel();
}
    
//void MenuHideMenuCb(puObject *cb)
void hideMenuCb (puObject *cb)
{
    guiToggleMenu();
}

void goodBye(puObject *)
{
    // FG_LOG( FG_INPUT, FG_ALERT,
    //      "Program exiting normally at user request." );
    cout << "Program exiting normally at user request." << endl;

#ifdef FG_NETWORK_OLK    
    if ( globals->get_options()->get_network_olk() ) {
	if ( net_is_registered == 0 ) fgd_send_com( "8", FGFS_host);
    }
#endif

    // close all external I/O connections
    fgIOShutdownAll();

    exit(0);
}


void goAwayCb (puObject *me)
{
    FG_POP_PUI_DIALOG( dialogBox );
}

void mkDialogInit (void)
{
    //  printf("mkDialogInit\n");
    int x = (globals->get_options()->get_xsize()/2 - 400/2);
    int y = (globals->get_options()->get_ysize()/2 - 100/2);
    dialogBox = new puDialogBox (x, y); // 150, 50
    {
        dialogFrame = new puFrame (0,0,400,100);
        dialogBoxMessage  =  new puText         (10, 70);
        dialogBoxMessage  -> setLabel           ("");
        dialogBoxOkButton =  new puOneShot      (180, 10, 240, 50);
        dialogBoxOkButton -> setLegend          (gui_msg_OK);
        dialogBoxOkButton -> makeReturnDefault  (TRUE );
        dialogBoxOkButton -> setCallback        (goAwayCb);
    }
    FG_FINALIZE_PUI_DIALOG( dialogBox );
}

void MayBeGoodBye(puObject *)
{
    ConfirmExitDialog(); 
}

void goAwayYesNoCb(puObject *me)
{
    FG_POP_PUI_DIALOG( YNdialogBox);
}

void ConfirmExitDialogInit(void)
{
    char msg[] = "Really Quit";
    char *s;

    //  printf("ConfirmExitDialogInit\n");
    int len = 200 - puGetStringWidth( puGetDefaultLabelFont(), msg )/2;

    int x = (globals->get_options()->get_xsize()/2 - 400/2);
    int y = (globals->get_options()->get_ysize()/2 - 100/2);
	
    YNdialogBox = new puDialogBox (x, y); // 150, 50
    //  YNdialogBox = new puDialogBox (150, 50);
    {
        YNdialogFrame = new puFrame (0,0,400, 100);
        
        YNdialogBoxMessage  =  new puText         (len, 70);
        YNdialogBoxMessage  -> setDefaultValue    (msg);
        YNdialogBoxMessage  -> getDefaultValue    (&s);
        YNdialogBoxMessage  -> setLabel           (s);
        
        YNdialogBoxOkButton =  new puOneShot      (100, 10, 160, 50);
        YNdialogBoxOkButton -> setLegend          (gui_msg_OK);
        YNdialogBoxOkButton -> makeReturnDefault  (TRUE );
        YNdialogBoxOkButton -> setCallback        (goodBye);
        
        YNdialogBoxNoButton =  new puOneShot      (240, 10, 300, 50);
        YNdialogBoxNoButton -> setLegend          (gui_msg_NO);
        YNdialogBoxNoButton -> setCallback        (goAwayYesNoCb);
    }
    FG_FINALIZE_PUI_DIALOG( YNdialogBox );
}

void notCb (puObject *)
{
    mkDialog ("This function isn't implemented yet");
}

void helpCb (puObject *)
{
    string command;
	
#if defined(FX) && !defined(WIN32)
#  if defined(XMESA_FX_FULLSCREEN) && defined(XMESA_FX_WINDOW)
    if ( global_fullscreen ) {
        global_fullscreen = false;
        XMesaSetFXmode( XMESA_FX_WINDOW );
    }
#  endif
#endif
	
#if !defined(WIN32)
    string url = "http://www.flightgear.org/Docs/InstallGuide/getstart.html";
	
    if ( system("xwininfo -name Netscape > /dev/null 2>&1") == 0 ) {
        command = "netscape -remote \"openURL(" + url + ")\" &";
    } else {
        command = "netscape " + url + " &";
    }
#else
    command = "webrun.bat";
#endif
	
    system( command.c_str() );
    //string text = "Help started in netscape window.";

    //mkDialog (text.c_str());
    mkDialog ("Help started in netscape window.");
}

#if defined( WIN32 ) && !defined( __CYGWIN__)

static void rotateView( double roll, double pitch, double yaw )
{
	// rotate view
}

static GlBitmap *b1 = NULL;
extern FGInterface cur_view_fdm;
GLubyte *hiResScreenCapture( int multiplier )
{
	float oldfov = globals->get_options()->get_fov();
	float fov = oldfov / multiplier;
	FGViewer *v = globals->get_current_view();
	globals->get_options()->set_fov(fov);
	v->force_update_fov_math();
    fgInitVisuals();
    int cur_width = globals->get_current_view()->get_winWidth( );
    int cur_height = globals->get_current_view()->get_winHeight( );
	if (b1) delete( b1 );
	// New empty (mostly) bitmap
	b1 = new GlBitmap( GL_RGB, 1, 1, (unsigned char *)"123" );
	int x,y;
	for ( y = 0; y < multiplier; y++ )
	{
		for ( x = 0; x < multiplier; x++ )
		{
			fgReshape( cur_width, cur_height );
			// pan to tile
			rotateView( 0, (y*fov)-((multiplier-1)*fov/2), (x*fov)-((multiplier-1)*fov/2) );
			fgRenderFrame();
			// restore view
			GlBitmap b2;
			b1->copyBitmap( &b2, cur_width*x, cur_height*y );
		}
	}
	globals->get_current_view()->UpdateViewParams(cur_view_fdm);
	globals->get_options()->set_fov(oldfov);
	v->force_update_fov_math();
	return b1->getBitmap();
}
#endif


#if defined( WIN32 ) && !defined( __CYGWIN__)
// win32 print screen function
void printScreen ( puObject *obj ) {
    bool show_pu_cursor = false;
    TurnCursorOff();
    if ( !puCursorIsHidden() ) {
	show_pu_cursor = true;
	puHideCursor();
    }
    BusyCursor( 0 );
    mainMenuBar->hide();

    CGlPrinter p( CGlPrinter::PRINT_BITMAP );
    int cur_width = globals->get_current_view()->get_winWidth( );
    int cur_height = globals->get_current_view()->get_winHeight( );
    p.Begin( "FlightGear", cur_width*3, cur_height*3 );
	p.End( hiResScreenCapture(3) );

    if( menu_on ) {
	mainMenuBar->reveal();
    }
    BusyCursor(1);
    if ( show_pu_cursor ) {
	puShowCursor();
    }
    TurnCursorOn();
}
#endif // #ifdef WIN32


void dumpSnapShot ( puObject *obj ) {
    fgDumpSnapShot();
}


// do a screen snap shot
void fgDumpSnapShot () {
    bool show_pu_cursor = false;

    int freeze = globals->get_freeze();
    if(!freeze)
        globals->set_freeze( true );

    mainMenuBar->hide();
    TurnCursorOff();
    if ( !puCursorIsHidden() ) {
	show_pu_cursor = true;
	puHideCursor();
    }

    fgInitVisuals();
    fgReshape( globals->get_current_view()->get_winWidth(),
	       globals->get_current_view()->get_winHeight() );

    // we need two render frames here to clear the menu and cursor
    // ... not sure why but doing an extra fgFenderFrame() shoulnd't
    // hurt anything
    fgRenderFrame();
    fgRenderFrame();

    my_glDumpWindow( "fgfs-screen.ppm", 
		     globals->get_options()->get_xsize(), 
		     globals->get_options()->get_ysize() );
    
    mkDialog ("Snap shot saved to fgfs-screen.ppm");

    if ( show_pu_cursor ) {
	puShowCursor();
    }

    TurnCursorOn();
    if( menu_on ) {
	mainMenuBar->reveal();
    }

    if(!freeze)
        globals->set_freeze( false );
}


/// The beginnings of teleportation :-)
//  Needs cleaning up but works
//  These statics should disapear when this is a class
static puDialogBox     *AptDialog = 0;
static puFrame         *AptDialogFrame = 0;
static puText          *AptDialogMessage = 0;
static puInput         *AptDialogInput = 0;

static char NewAirportId[16];
static char NewAirportLabel[] = "Enter New Airport ID"; 

static puOneShot       *AptDialogOkButton = 0;
static puOneShot       *AptDialogCancelButton = 0;
static puOneShot       *AptDialogResetButton = 0;

void AptDialog_Cancel(puObject *)
{
    FG_POP_PUI_DIALOG( AptDialog );
}

void AptDialog_OK (puObject *)
{
    FGPath path( globals->get_options()->get_fg_root() );
    path.append( "Airports" );
    path.append( "simple.mk4" );
    FGAirports airports( path.c_str() );

    FGAirport a;
    
    int freeze = globals->get_freeze();
    if(!freeze)
        globals->set_freeze( true );

    char *s;
    AptDialogInput->getValue(&s);
    string AptId(s);

	cout << "AptDialog_OK " << AptId << " " << AptId.length() << endl;
    
    AptDialog_Cancel( NULL );
    
    if ( AptId.length() ) {
        // set initial position from airport id
        FG_LOG( FG_GENERAL, FG_INFO,
                "Attempting to set starting position from airport code "
                << AptId );

        if ( airports.search( AptId, &a ) )
        {
            globals->get_options()->set_airport_id( AptId.c_str() );
            globals->get_options()->set_altitude( -9999.0 );
	    // fgSetPosFromAirportID( AptId );
	    fgSetPosFromAirportIDandHdg( AptId, 
					 cur_fdm_state->get_Psi() * RAD_TO_DEG);
            BusyCursor(0);
            fgReInitSubsystems();
            BusyCursor(1);
        } else {
            AptId  += " not in database.";
            mkDialog(AptId.c_str());
        }
    }
    if(!freeze)
        globals->set_freeze( false );
}

void AptDialog_Reset(puObject *)
{
    //  strncpy( NewAirportId, globals->get_options()->get_airport_id().c_str(), 16 );
    sprintf( NewAirportId, "%s", globals->get_options()->get_airport_id().c_str() );
    AptDialogInput->setValue ( NewAirportId );
    AptDialogInput->setCursor( 0 ) ;
}

void NewAirport(puObject *cb)
{
    //  strncpy( NewAirportId, globals->get_options()->get_airport_id().c_str(), 16 );
    sprintf( NewAirportId, "%s", globals->get_options()->get_airport_id().c_str() );
//	cout << "NewAirport " << NewAirportId << endl;
    AptDialogInput->setValue( NewAirportId );

    FG_PUSH_PUI_DIALOG( AptDialog );
}

static void NewAirportInit(void)
{
    sprintf( NewAirportId, "%s", globals->get_options()->get_airport_id().c_str() );
    int len = 150 - puGetStringWidth( puGetDefaultLabelFont(),
                                      NewAirportLabel ) / 2;

    AptDialog = new puDialogBox (150, 50);
    {
        AptDialogFrame   = new puFrame           (0,0,350, 150);
        AptDialogMessage = new puText            (len, 110);
        AptDialogMessage ->    setLabel          (NewAirportLabel);

        AptDialogInput   = new puInput           (50, 70, 300, 100);
        AptDialogInput   ->    setValue          (NewAirportId);
        AptDialogInput   ->    acceptInput();

        AptDialogOkButton     =  new puOneShot   (50, 10, 110, 50);
        AptDialogOkButton     ->     setLegend   (gui_msg_OK);
        AptDialogOkButton     ->     setCallback (AptDialog_OK);
        AptDialogOkButton     ->     makeReturnDefault(TRUE);

        AptDialogCancelButton =  new puOneShot   (140, 10, 210, 50);
        AptDialogCancelButton ->     setLegend   (gui_msg_CANCEL);
        AptDialogCancelButton ->     setCallback (AptDialog_Cancel);

        AptDialogResetButton  =  new puOneShot   (240, 10, 300, 50);
        AptDialogResetButton  ->     setLegend   (gui_msg_RESET);
        AptDialogResetButton  ->     setCallback (AptDialog_Reset);
    }
	cout << "NewAirportInit " << NewAirportId << endl;
    FG_FINALIZE_PUI_DIALOG( AptDialog );
}

#ifdef FG_NETWORK_OLK

/// The beginnings of networking :-)
//  Needs cleaning up but works
//  These statics should disapear when this is a class
static puDialogBox     *NetIdDialog = 0;
static puFrame         *NetIdDialogFrame = 0;
static puText          *NetIdDialogMessage = 0;
static puInput         *NetIdDialogInput = 0;

static char NewNetId[16];
static char NewNetIdLabel[] = "Enter New Callsign"; 
extern char *fgd_callsign;

static puOneShot       *NetIdDialogOkButton = 0;
static puOneShot       *NetIdDialogCancelButton = 0;

void NetIdDialog_Cancel(puObject *)
{
    FG_POP_PUI_DIALOG( NetIdDialog );
}

void NetIdDialog_OK (puObject *)
{
    string NetId;
    
    bool freeze = globals->get_freeze();
    if(!freeze)
        globals->set_freeze( true );
/*  
   The following needs some cleanup because 
   "string options.NetId" and "char *net_callsign" 
*/
    NetIdDialogInput->getValue(&net_callsign);
    NetId = net_callsign;
    
    NetIdDialog_Cancel( NULL );
    globals->get_options()->set_net_id( NetId.c_str() );
    strcpy( fgd_callsign, net_callsign);
//    strcpy( fgd_callsign, globals->get_options()->get_net_id().c_str());
/* Entering a callsign indicates : user wants Net HUD Info */
    net_hud_display = 1;

    if(!freeze)
        globals->set_freeze( false );
}

void NewCallSign(puObject *cb)
{
    sprintf( NewNetId, "%s", globals->get_options()->get_net_id().c_str() );
//    sprintf( NewNetId, "%s", fgd_callsign );
    NetIdDialogInput->setValue( NewNetId );

    FG_PUSH_PUI_DIALOG( NetIdDialog );
}

static void NewNetIdInit(void)
{
    sprintf( NewNetId, "%s", globals->get_options()->get_net_id().c_str() );
//    sprintf( NewNetId, "%s", fgd_callsign );
    int len = 150 - puGetStringWidth( puGetDefaultLabelFont(),
                                      NewNetIdLabel ) / 2;

    NetIdDialog = new puDialogBox (150, 50);
    {
        NetIdDialogFrame   = new puFrame           (0,0,350, 150);
        NetIdDialogMessage = new puText            (len, 110);
        NetIdDialogMessage ->    setLabel          (NewNetIdLabel);

        NetIdDialogInput   = new puInput           (50, 70, 300, 100);
        NetIdDialogInput   ->    setValue          (NewNetId);
        NetIdDialogInput   ->    acceptInput();

        NetIdDialogOkButton     =  new puOneShot   (50, 10, 110, 50);
        NetIdDialogOkButton     ->     setLegend   (gui_msg_OK);
        NetIdDialogOkButton     ->     setCallback (NetIdDialog_OK);
        NetIdDialogOkButton     ->     makeReturnDefault(TRUE);

        NetIdDialogCancelButton =  new puOneShot   (240, 10, 300, 50);
        NetIdDialogCancelButton ->     setLegend   (gui_msg_CANCEL);
        NetIdDialogCancelButton ->     setCallback (NetIdDialog_Cancel);

    }
    FG_FINALIZE_PUI_DIALOG( NetIdDialog );
}

static void net_display_toggle( puObject *cb)
{
	net_hud_display = (net_hud_display) ? 0 : 1;
        printf("Toggle net_hud_display : %d\n", net_hud_display);
}

static void net_register( puObject *cb)
{
	fgd_send_com( "1", FGFS_host );
        net_is_registered = 0;
        printf("Registering to deamon\n");
}

static void net_unregister( puObject *cb)
{
	fgd_send_com( "8", FGFS_host );
        net_is_registered = -1;
        printf("Unregistering from deamon\n");
}


/*************** Deamon communication **********/

//  These statics should disapear when this is a class
static puDialogBox     *NetFGDDialog = 0;
static puFrame         *NetFGDDialogFrame = 0;
static puText          *NetFGDDialogMessage = 0;
//static puInput         *NetFGDDialogInput = 0;

//static char NewNetId[16];
static char NewNetFGDLabel[] = "Scan for deamon                        "; 
static char NewFGDHost[64] = "olk.mcp.de"; 
static int  NewFGDPortLo = 10000;
static int  NewFGDPortHi = 10001;
 
//extern char *fgd_callsign;
extern u_short base_port, end_port;
extern int fgd_ip, verbose, current_port;
extern char *fgd_host;


static puOneShot       *NetFGDDialogOkButton = 0;
static puOneShot       *NetFGDDialogCancelButton = 0;
static puOneShot       *NetFGDDialogScanButton = 0;

static puInput         *NetFGDHostDialogInput = 0;
static puInput         *NetFGDPortLoDialogInput = 0;
static puInput         *NetFGDPortHiDialogInput = 0;

void NetFGDDialog_Cancel(puObject *)
{
    FG_POP_PUI_DIALOG( NetFGDDialog );
}

void NetFGDDialog_OK (puObject *)
{
    char *NetFGD;    

    bool freeze = globals->get_freeze();
    if(!freeze)
        globals->set_freeze( true );
    NetFGDHostDialogInput->getValue( &NetFGD );
    strcpy( fgd_host, NetFGD);
    NetFGDPortLoDialogInput->getValue( (int *) &base_port );
    NetFGDPortHiDialogInput->getValue( (int *) &end_port );
    NetFGDDialog_Cancel( NULL );
    if(!freeze)
        globals->set_freeze( false );
}

void NetFGDDialog_SCAN (puObject *)
{
    char *NetFGD;
    int fgd_port;
    
    bool freeze = globals->get_freeze();
    if(!freeze)
        globals->set_freeze( true );
//    printf("Vor getvalue %s\n");
    NetFGDHostDialogInput->getValue( &NetFGD );
//    printf("Vor strcpy %s\n", (char *) NetFGD);
    strcpy( fgd_host, NetFGD);
    NetFGDPortLoDialogInput->getValue( (int *) &base_port );
    NetFGDPortHiDialogInput->getValue( (int *) &end_port );
    printf("FGD: %s  Port-Start: %d Port-End: %d\n", fgd_host, 
                 base_port, end_port);
    net_resolv_fgd(fgd_host);
    printf("Resolve : %d\n", net_r);
    if(!freeze)
        globals->set_freeze( false );
    if ( net_r == 0 ) {
      fgd_port = 10000;
      strcpy( fgd_name, "");
      for( current_port = base_port; ( current_port <= end_port); current_port++) {
          fgd_send_com("0" , FGFS_host);
          sprintf( NewNetFGDLabel , "Scanning for deamon Port: %d", current_port); 
          printf("FGD: searching %s\n", fgd_name);
          if ( strcmp( fgd_name, "") != 0 ) {
             sprintf( NewNetFGDLabel , "Found %s at Port: %d", 
                                              fgd_name, current_port);
             fgd_port = current_port;
             current_port = end_port+1;
          }
      }
      current_port = end_port = base_port = fgd_port;
    }
    NetFGDDialog_Cancel( NULL );
}


void net_fgd_scan(puObject *cb)
{
    NewFGDPortLo = base_port;
    NewFGDPortHi = end_port;
    strcpy( NewFGDHost, fgd_host);
    NetFGDPortLoDialogInput->setValue( NewFGDPortLo );
    NetFGDPortHiDialogInput->setValue( NewFGDPortHi );
    NetFGDHostDialogInput->setValue( NewFGDHost );

    FG_PUSH_PUI_DIALOG( NetFGDDialog );
}


static void NewNetFGDInit(void)
{
//    sprintf( NewNetId, "%s", globals->get_options()->get_net_id().c_str() );
//    sprintf( NewNetId, "%s", fgd_callsign );
    int len = 170 - puGetStringWidth( puGetDefaultLabelFont(),
                                      NewNetFGDLabel ) / 2;

    NetFGDDialog = new puDialogBox (310, 30);
    {
        NetFGDDialogFrame   = new puFrame           (0,0,320, 170);
        NetFGDDialogMessage = new puText            (len, 140);
        NetFGDDialogMessage ->    setLabel          (NewNetFGDLabel);

        NetFGDPortLoDialogInput   = new puInput           (50, 70, 127, 100);
        NetFGDPortLoDialogInput   ->    setValue          (NewFGDPortLo);
        NetFGDPortLoDialogInput   ->    acceptInput();

        NetFGDPortHiDialogInput   = new puInput           (199, 70, 275, 100);
        NetFGDPortHiDialogInput   ->    setValue          (NewFGDPortHi);
        NetFGDPortHiDialogInput   ->    acceptInput();

        NetFGDHostDialogInput   = new puInput           (50, 100, 275, 130);
        NetFGDHostDialogInput   ->    setValue          (NewFGDHost);
        NetFGDHostDialogInput   ->    acceptInput();

        NetFGDDialogScanButton     =  new puOneShot   (130, 10, 200, 50);
        NetFGDDialogScanButton     ->     setLegend   ("Scan");
        NetFGDDialogScanButton     ->     setCallback (NetFGDDialog_SCAN);
        NetFGDDialogScanButton     ->     makeReturnDefault(FALSE);

        NetFGDDialogOkButton     =  new puOneShot   (50, 10, 120, 50);
        NetFGDDialogOkButton     ->     setLegend   (gui_msg_OK);
        NetFGDDialogOkButton     ->     setCallback (NetFGDDialog_OK);
        NetFGDDialogOkButton     ->     makeReturnDefault(TRUE);

        NetFGDDialogCancelButton =  new puOneShot   (210, 10, 280, 50);
        NetFGDDialogCancelButton ->     setLegend   (gui_msg_CANCEL);
        NetFGDDialogCancelButton ->     setCallback (NetFGDDialog_Cancel);

    }
    FG_FINALIZE_PUI_DIALOG( NetFGDDialog );
}

/*
static void net_display_toggle( puObject *cb)
{
	net_hud_display = (net_hud_display) ? 0 : 1;
        printf("Toggle net_hud_display : %d\n", net_hud_display);
}

*/

#endif

/***************  End Networking  **************/



/* -----------------------------------------------------------------------
The menu stuff 
---------------------------------------------------------------------*/
char *fileSubmenu               [] = {
    "Exit", /* "Close", "---------", */
#if defined( WIN32 ) && !defined( __CYGWIN__)
    "Print",
#endif
    "Snap Shot",
    "---------", 
    "Reset", 
    "Load flight",
    "Save flight",
    NULL
};
puCallback fileSubmenuCb        [] = {
    MayBeGoodBye, /* hideMenuCb, NULL, */
#if defined( WIN32 ) && !defined( __CYGWIN__)
    printScreen, 
#endif
    /* NULL, notCb, */
    dumpSnapShot,
    NULL,
    reInit, 
    loadFlight,
    saveFlight,
    NULL
};

/*
char *editSubmenu               [] = {
    "Edit text", NULL
};
puCallback editSubmenuCb        [] = {
    notCb, NULL
};
*/

extern void fgHUDalphaAdjust( puObject * );
char *viewSubmenu               [] = {
    "HUD Alpha",
    /* "Cockpit View > ", "View >","------------", */
    "Toggle Panel...", NULL
};
puCallback viewSubmenuCb        [] = {
    fgHUDalphaAdjust,
    /* notCb, notCb, NULL, */
    guiTogglePanel, NULL
};

//  "---------", 

char *autopilotSubmenu           [] = {
    "Toggle HUD Format", "Adjust AP Settings",
    "---------", 
    "Clear Route", "Skip Current Waypoint", "Add Waypoint",
    "---------", 
    "Set Altitude", "Set Heading",
    NULL
};

puCallback autopilotSubmenuCb    [] = {
    fgLatLonFormatToggle, fgAPAdjust,
    NULL,
    ClearRoute, PopWayPoint, AddWayPoint,
    NULL,
    NewAltitude, NewHeading,
    /* notCb, */ NULL
};

char *environmentSubmenu        [] = {
    "Toggle Clouds",
    "Goto Airport", /* "Terrain", "Weather", */ NULL
};
puCallback environmentSubmenuCb [] = {
    toggleClouds,
    NewAirport, /* notCb, notCb, */ NULL
};

/*
char *optionsSubmenu            [] = {
    "Preferences", "Realism & Reliablity...", NULL
};
puCallback optionsSubmenuCb     [] = {
    notCb, notCb, NULL
};
*/

#ifdef FG_NETWORK_OLK
char *networkSubmenu            [] = {
    "Unregister from FGD ", /* "Send MSG to All", "Send MSG", "Show Pilots", */
    "Register to FGD",
    "Scan for Deamons", "Enter Callsign", /* "Display Netinfos", */
    "Toggle Display", NULL
};
puCallback networkSubmenuCb     [] = {
    /* notCb, notCb, notCb, notCb, */ 
    net_unregister, 
    net_register, 
    net_fgd_scan, NewCallSign, 
    net_display_toggle, NULL
};
#endif

char *helpSubmenu               [] = {
    /* "About...", */ "Help", NULL
};
puCallback helpSubmenuCb        [] = {
    /* notCb, */ helpCb, NULL
};


/* -------------------------------------------------------------------------
init the gui
_____________________________________________________________________*/


void guiInit()
{
    char *mesa_win_state;

    // Initialize PUI
    puInit();
    puSetDefaultStyle         ( PUSTYLE_SMALL_BEVELLED ); //PUSTYLE_DEFAULT
    puSetDefaultColourScheme  (0.8, 0.8, 0.8, 0.4);

    // Initialize our GLOBAL GUI STRINGS
    gui_msg_OK     = msg_OK;     // "OK"
    gui_msg_NO     = msg_NO;     // "NO"
    gui_msg_YES    = msg_YES;    // "YES"
    gui_msg_CANCEL = msg_CANCEL; // "CANCEL"
    gui_msg_RESET  = msg_RESET;  // "RESET"

    // Next check home directory
    FGPath fntpath;
    char* envp = ::getenv( "FG_FONTS" );
    if ( envp != NULL ) {
        fntpath.set( envp );
    } else {
        fntpath.set( globals->get_options()->get_fg_root() );
	fntpath.append( "Fonts" );
    }

    // Install our fast fonts
    fntpath.append( "typewriter.txf" );
    guiFntHandle = new fntTexFont ;
    guiFntHandle -> load ( (char *)fntpath.c_str() ) ;
    puFont GuiFont ( guiFntHandle, 15 ) ;
    puSetDefaultFonts( GuiFont, GuiFont ) ;
    guiFnt = puGetDefaultLabelFont();
  
    if ( globals->get_options()->get_mouse_pointer() == 0 ) {
        // no preference specified for mouse pointer, attempt to autodetect...
        // Determine if we need to render the cursor, or if the windowing
        // system will do it.  First test if we are rendering with glide.
        if ( strstr ( general.get_glRenderer(), "Glide" ) ) {
            // Test for the MESA_GLX_FX env variable
            if ( (mesa_win_state = getenv( "MESA_GLX_FX" )) != NULL) {
                // test if we are fullscreen mesa/glide
                if ( (mesa_win_state[0] == 'f') ||
                     (mesa_win_state[0] == 'F') ) {
                    puShowCursor ();
                }
            }
        }
        mouse_active = ~mouse_active;
    } else if ( globals->get_options()->get_mouse_pointer() == 1 ) {
        // don't show pointer
    } else if ( globals->get_options()->get_mouse_pointer() == 2 ) {
        // force showing pointer
        puShowCursor();
        mouse_active = ~mouse_active;
    }
	
    // MOUSE_VIEW mode stuff
    trackball(_quat0, 0.0, 0.0, 0.0, 0.0);  
    Quat0();
    build_rotmatrix(quat_mat, curquat);

    // Set up our Dialog Boxes
    ConfirmExitDialogInit();
    NewAirportInit();
#ifdef FG_NETWORK_OLK
    NewNetIdInit();
    NewNetFGDInit();
#endif
    mkDialogInit();
    
    // Make the menu bar
    mainMenuBar = new puMenuBar ();
    mainMenuBar -> add_submenu ("File", fileSubmenu, fileSubmenuCb);
    // mainMenuBar -> add_submenu ("Edit", editSubmenu, editSubmenuCb);
    mainMenuBar -> add_submenu ("View", viewSubmenu, viewSubmenuCb);
    mainMenuBar -> add_submenu ("Environment", environmentSubmenu, environmentSubmenuCb);
    mainMenuBar -> add_submenu ("Autopilot", autopilotSubmenu, autopilotSubmenuCb);
    // mainMenuBar -> add_submenu ("Options", optionsSubmenu, optionsSubmenuCb);
#ifdef FG_NETWORK_OLK
    if ( globals->get_options()->get_network_olk() ) {
    	mainMenuBar -> add_submenu ("Network", networkSubmenu, networkSubmenuCb);
    }
#endif
    mainMenuBar -> add_submenu ("Help", helpSubmenu, helpSubmenuCb);
    mainMenuBar-> close ();
    // Set up menu bar toggle
    menu_on = ~0;
}



/*
 * Trackball code:
 *
 * Implementation of a virtual trackball.
 * Implemented by Gavin Bell, lots of ideas from Thant Tessman and
 *   the August '88 issue of Siggraph's "Computer Graphics," pp. 121-129.
 *
 * Vector manip code:
 *
 * Original code from:
 * David M. Ciemiewicz, Mark Grossman, Henry Moreton, and Paul Haeberli
 *
 * Much mucking with by:
 * Gavin Bell
 */
#if defined(_WIN32) && !defined( __CYGWIN32__ )
#pragma warning (disable:4244)          /* disable bogus conversion warnings */
#endif
#include <math.h>
#include <stdio.h>
//#include "trackball.h"

/*
 * This size should really be based on the distance from the center of
 * rotation to the point on the object underneath the mouse.  That
 * point would then track the mouse as closely as possible.  This is a
 * simple example, though, so that is left as an Exercise for the
 * Programmer.
 */
#define TRACKBALLSIZE  (0.8f)
#define SQRT(x) sqrt(x)

/*
 * Local function prototypes (not defined in trackball.h)
 */
static float tb_project_to_sphere(float, float, float);
static void normalize_quat(float [4]);

static void
vzero(float *v)
{
    v[0] = 0.0;
    v[1] = 0.0;
    v[2] = 0.0;
}

static void
vset(float *v, float x, float y, float z)
{
    v[0] = x;
    v[1] = y;
    v[2] = z;
}

static void
vsub(const float *src1, const float *src2, float *dst)
{
    dst[0] = src1[0] - src2[0];
    dst[1] = src1[1] - src2[1];
    dst[2] = src1[2] - src2[2];
}

static void
vcopy(const float *v1, float *v2)
{
    register int i;
    for (i = 0 ; i < 3 ; i++)
        v2[i] = v1[i];
}

static void
vcross(const float *v1, const float *v2, float *cross)
{
    float temp[3];

    temp[0] = (v1[1] * v2[2]) - (v1[2] * v2[1]);
    temp[1] = (v1[2] * v2[0]) - (v1[0] * v2[2]);
    temp[2] = (v1[0] * v2[1]) - (v1[1] * v2[0]);
    vcopy(temp, cross);
}

static float
vlength(const float *v)
{
    float tmp = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
    return SQRT(tmp);
}

static void
vscale(float *v, float div)
{
    v[0] *= div;
    v[1] *= div;
    v[2] *= div;
}

static void
vnormal(float *v)
{
    vscale(v,1.0/vlength(v));
}

static float
vdot(const float *v1, const float *v2)
{
    return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
}

static void
vadd(const float *src1, const float *src2, float *dst)
{
    dst[0] = src1[0] + src2[0];
    dst[1] = src1[1] + src2[1];
    dst[2] = src1[2] + src2[2];
}

/*
 *  Given an axis and angle, compute quaternion.
 */
void
axis_to_quat(float a[3], float phi, float q[4])
{
    double sinphi2, cosphi2;
    double phi2 = phi/2.0;
    sinphi2 = sin(phi2);
    cosphi2 = cos(phi2);
    vnormal(a);
    vcopy(a,q);
    vscale(q,sinphi2);
    q[3] = cosphi2;
}

/*
 * Project an x,y pair onto a sphere of radius r OR a hyperbolic sheet
 * if we are away from the center of the sphere.
 */
static float
tb_project_to_sphere(float r, float x, float y)
{
    float d, t, z, tmp;

    tmp = x*x + y*y;
    d = SQRT(tmp);
    if (d < r * 0.70710678118654752440) {    /* Inside sphere */
        tmp = r*r - d*d;
        z = SQRT(tmp);
    } else {           /* On hyperbola */
        t = r / 1.41421356237309504880;
        z = t*t / d;
    }
    return z;
}

/*
 * Quaternions always obey:  a^2 + b^2 + c^2 + d^2 = 1.0
 * If they don't add up to 1.0, dividing by their magnitued will
 * renormalize them.
 *
 * Note: See the following for more information on quaternions:
 *
 * - Shoemake, K., Animating rotation with quaternion curves, Computer
 *   Graphics 19, No 3 (Proc. SIGGRAPH'85), 245-254, 1985.
 * - Pletinckx, D., Quaternion calculus as a basic tool in computer
 *   graphics, The Visual Computer 5, 2-13, 1989.
 */
static void
normalize_quat(float q[4])
{
    int i;
    float mag, tmp;

    tmp = q[0]*q[0] + q[1]*q[1] + q[2]*q[2] + q[3]*q[3];
    mag = 1.0 / SQRT(tmp);
    for (i = 0; i < 4; i++)
        q[i] *= mag;
}

/*
 * Ok, simulate a track-ball.  Project the points onto the virtual
 * trackball, then figure out the axis of rotation, which is the cross
 * product of P1 P2 and O P1 (O is the center of the ball, 0,0,0)
 * Note:  This is a deformed trackball-- is a trackball in the center,
 * but is deformed into a hyperbolic sheet of rotation away from the
 * center.  This particular function was chosen after trying out
 * several variations.
 *
 * It is assumed that the arguments to this routine are in the range
 * (-1.0 ... 1.0)
 */
void
trackball(float q[4], float p1x, float p1y, float p2x, float p2y)
{
    float a[3]; /* Axis of rotation */
    float phi;  /* how much to rotate about axis */
    float p1[3], p2[3], d[3];
    float t;

    if (p1x == p2x && p1y == p2y) {
        /* Zero rotation */
        vzero(q);
        q[3] = 1.0;
        return;
    }

    /*
     * First, figure out z-coordinates for projection of P1 and P2 to
     * deformed sphere
     */
    vset(p1,p1x,p1y,tb_project_to_sphere(TRACKBALLSIZE,p1x,p1y));
    vset(p2,p2x,p2y,tb_project_to_sphere(TRACKBALLSIZE,p2x,p2y));

    /*
     *  Now, we want the cross product of P1 and P2
     */
    vcross(p2,p1,a);

    /*
     *  Figure out how much to rotate around that axis.
     */
    vsub(p1,p2,d);
    t = vlength(d) / (2.0*TRACKBALLSIZE);

    /*
     * Avoid problems with out-of-control values...
     */
    if (t > 1.0) t = 1.0;
    if (t < -1.0) t = -1.0;
    phi = 2.0 * asin(t);

    axis_to_quat(a,phi,q);
}

/*
 * Given two rotations, e1 and e2, expressed as quaternion rotations,
 * figure out the equivalent single rotation and stuff it into dest.
 *
 * This routine also normalizes the result every RENORMCOUNT times it is
 * called, to keep error from creeping in.
 *
 * NOTE: This routine is written so that q1 or q2 may be the same
 * as dest (or each other).
 */

#define RENORMCOUNT 97

void
add_quats(float q1[4], float q2[4], float dest[4])
{
    static int count=0;
    float t1[4], t2[4], t3[4];
    float tf[4];

#if 0
printf("q1 = %f %f %f %f\n", q1[0], q1[1], q1[2], q1[3]);
printf("q2 = %f %f %f %f\n", q2[0], q2[1], q2[2], q2[3]);
#endif

    vcopy(q1,t1);
    vscale(t1,q2[3]);

    vcopy(q2,t2);
    vscale(t2,q1[3]);

    vcross(q2,q1,t3);
    vadd(t1,t2,tf);
    vadd(t3,tf,tf);
    tf[3] = q1[3] * q2[3] - vdot(q1,q2);

#if 0
printf("tf = %f %f %f %f\n", tf[0], tf[1], tf[2], tf[3]);
#endif

    dest[0] = tf[0];
    dest[1] = tf[1];
    dest[2] = tf[2];
    dest[3] = tf[3];

    if (++count > RENORMCOUNT) {
        count = 0;
        normalize_quat(dest);
    }
}

/*
 * Build a rotation matrix, given a quaternion rotation.
 *
 */
void
build_rotmatrix(float m[4][4], float q[4])
{
//#define TRANSPOSED_QUAT
#ifndef TRANSPOSED_QUAT
    m[0][0] = 1.0 - 2.0 * (q[1] * q[1] + q[2] * q[2]);
    m[0][1] = 2.0 * (q[0] * q[1] - q[2] * q[3]);
    m[0][2] = 2.0 * (q[2] * q[0] + q[1] * q[3]);
    m[0][3] = 0.0;

    m[1][0] = 2.0 * (q[0] * q[1] + q[2] * q[3]);
    m[1][1]= 1.0 - 2.0 * (q[2] * q[2] + q[0] * q[0]);
    m[1][2] = 2.0 * (q[1] * q[2] - q[0] * q[3]);
    m[1][3] = 0.0;

    m[2][0] = 2.0 * (q[2] * q[0] - q[1] * q[3]);
    m[2][1] = 2.0 * (q[1] * q[2] + q[0] * q[3]);
    m[2][2] = 1.0 - 2.0 * (q[1] * q[1] + q[0] * q[0]);
    
    m[2][3] = 0.0;
    m[3][0] = 0.0;
    m[3][1] = 0.0;
    m[3][2] = 0.0;
    m[3][3] = 1.0;
#else //  TRANSPOSED_QUAT
    m[0][0] = 1.0 - 2.0 * (q[1] * q[1] + q[2] * q[2]);
    m[0][1] = 2.0 * (q[0] * q[1] + q[2] * q[3]);
    m[0][2] = 2.0 * (q[2] * q[0] - q[1] * q[3]);
    m[0][3] = 0.0;

    m[1][0] = 2.0 * (q[0] * q[1] - q[2] * q[3]);
    m[1][1] = 1.0 - 2.0 * (q[2] * q[2] + q[0] * q[0]);
    m[1][2] = 2.0 * (q[1] * q[2] + q[0] * q[3]);
    m[1][3] = 0.0;

    m[2][0] = 2.0 * (q[2] * q[0] + q[1] * q[3]);
    m[2][1] = 2.0 * (q[1] * q[2] - q[0] * q[3]);
    m[2][2] = 1.0 - 2.0 * (q[1] * q[1] + q[0] * q[0]);
    m[2][3] = 0.0;
    
    m[3][0] = 0.0;
    m[3][1] = 0.0;
    m[3][2] = 0.0;
    m[3][3] = 1.0;
#endif // 0
}

