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

#include <Include/compiler.h>

#ifdef FG_MATH_EXCEPTION_CLASH
#  include <math.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <GL/glut.h>
#include <XGL/xgl.h>

#if defined(FX) && defined(XMESA)
#  include <GL/xmesa.h>
#endif

#include STL_STRING

#include <stdlib.h>
#include <string.h>

#include <Include/general.hxx>
#include <Debug/logstream.hxx>
#include <Aircraft/aircraft.hxx>
#include <Airports/simple.hxx>
#include <Cockpit/panel.hxx>
#include <FDM/flight.hxx>
#include <Main/options.hxx>
#include <Main/fg_init.hxx>
#include <Main/views.hxx>
#include <Misc/fgpath.hxx>
#include <Network/network.h>
#include <Screen/screen-dump.hxx>
#include <Time/fg_time.hxx>

#ifdef WIN32
#  include <Screen/win32-printer.h>
#endif

#include "gui.h"

FG_USING_STD(string);

#ifndef FG_HAVE_NATIVE_SGI_COMPILERS
FG_USING_STD(cout);
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

extern void NewAltitude( puObject *cb );
extern void NewHeading( puObject *cb );
extern void fgAPAdjust( puObject * );
extern void fgLatLonFormatToggle( puObject *);
extern void NewTgtAirport( puObject *cb );

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
static float aileron_sensitivity = 500.0;
static float elevator_sensitivity = 500.0;
static float brake_sensitivity = 250.0;
static float throttle_sensitivity = 250.0;
static float rudder_sensitivity = 500.0;
static float trim_sensitivity = 1000.0;

void guiMotionFunc ( int x, int y )
{
    if (mouse_mode == MOUSE_POINTER) {
      puMouse ( x, y ) ;
      glutPostRedisplay () ;
    } else {
      int ww = current_view.get_winWidth();
      int wh = current_view.get_winHeight();

				// Mouse as yoke
      if (mouse_mode == MOUSE_YOKE) {
	if (last_buttons & (1 << GLUT_LEFT_BUTTON)) {
	  float brake_offset = (_mX - x) / brake_sensitivity;
	  float throttle_offset = (_mY - y) / throttle_sensitivity;
	  controls.move_brake(FGControls::ALL_WHEELS, brake_offset);
	  controls.move_throttle(FGControls::ALL_ENGINES, throttle_offset);
	} else if (last_buttons & (1 << GLUT_MIDDLE_BUTTON)) {
	  float rudder_offset = (x - _mX) / rudder_sensitivity;
	  float trim_offset = (_mY - y) / trim_sensitivity;
	  controls.move_rudder(rudder_offset);
	  controls.move_elevator_trim(trim_offset);
	} else {
	  float aileron_offset = (x - _mX) / aileron_sensitivity;
	  float elevator_offset = (_mY - y) / elevator_sensitivity;
	  controls.move_aileron(aileron_offset);
	  controls.move_elevator(elevator_offset);
	}

				// Mouse as view
      } else {
	FGView * v = &current_view;
	double offset = v->get_goal_view_offset();
	double full = FG_PI * 2.0;
	offset += (_mX - x) / 500.0;
	while (offset < 0) {
	  offset += full;
	}
	while (offset > full) {
	  offset -= full;
	}
	v->set_view_offset(offset);
	v->set_goal_view_offset(offset);
      }

				// Keep the mouse in the window.
      if (x < 5 || x > ww-5 || y < 5 || y > wh-5) {
	_mX = x = ww / 2;
	_mY = y = wh / 2;
	glutWarpPointer(x, y);
      }
    }

				// Record the new mouse position.
    _mX = x;
    _mY = y;
}

void guiMouseFunc(int button, int updown, int x, int y)
{
				// Was the left button pressed?
    if (updown == GLUT_DOWN && button == GLUT_LEFT_BUTTON) {
      switch (mouse_mode) {
      case MOUSE_POINTER:
	break;
      case MOUSE_YOKE:
	break;
      case MOUSE_VIEW:
        current_view.set_view_offset( 0.00 );
        current_view.set_goal_view_offset( 0.00 );
	break;
      }

				// Or was it the right button?
    } else if (updown == GLUT_DOWN && button == GLUT_RIGHT_BUTTON) {
      switch (mouse_mode) {
      case MOUSE_POINTER:
	mouse_mode = MOUSE_YOKE;
	_savedX = x;
	_savedY = y;
	glutSetCursor(GLUT_CURSOR_NONE);
	FG_LOG( FG_INPUT, FG_INFO, "Mouse in yoke mode" );
	break;
      case MOUSE_YOKE:
	mouse_mode = MOUSE_VIEW;
	FG_LOG( FG_INPUT, FG_INFO, "Mouse in view mode" );
	break;
      case MOUSE_VIEW:
	mouse_mode = MOUSE_POINTER;
	_mX = x = _savedX;
	_mY = y = _savedY;
	glutWarpPointer(x, y);
	glutSetCursor(GLUT_CURSOR_INHERIT);
	FG_LOG( FG_INPUT, FG_INFO, "Mouse in pointer mode" );
	break;
      }     
    }

				// Register the new position (if it
				// hasn't been registered already).
    _mX = x;
    _mY = y;

				// Note which button is pressed.
    if ( updown == GLUT_DOWN ) {
        last_buttons |=  ( 1 << button ) ;
    } else {
        last_buttons &= ~( 1 << button ) ;
    }

				// If we're in pointer mode, let PUI
				// know what's going on.
    if (mouse_mode == MOUSE_POINTER) {
	puMouse (button, updown, x,y);
	glutPostRedisplay ();
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

static inline void TurnCursorOn( void )
{
    mouse_active = ~0;
#if defined ( WIN32 ) || defined(__CYGWIN32__)
    glutSetCursor(GLUT_CURSOR_INHERIT);
#endif
#if (GLUT_API_VERSION >= 4 || GLUT_XLIB_IMPLEMENTATION >= 9)
    glutWarpPointer( glutGet((GLenum)GLUT_SCREEN_WIDTH)/2, glutGet((GLenum)GLUT_SCREEN_HEIGHT)/2);
#endif
}

static inline void TurnCursorOff( void )
{
    mouse_active = 0;
#if defined ( WIN32 ) || defined(__CYGWIN32__)
    glutSetCursor(GLUT_CURSOR_NONE);
#else  // I guess this is what we want to do ??
#if (GLUT_API_VERSION >= 4 || GLUT_XLIB_IMPLEMENTATION >= 9)
    glutWarpPointer( glutGet((GLenum)GLUT_SCREEN_WIDTH), glutGet((GLenum)GLUT_SCREEN_HEIGHT));
#endif
#endif
}

void maybeToggleMouse( void )
{
#ifdef WIN32
    static int first_time = ~0;
    static int mouse_changed = 0;

    if ( first_time ) {
        if(!mouse_active) {
            mouse_changed = ~mouse_changed;
            TurnCursorOn();
        }
    } else {
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
    static int cursor = 0;
    if( restore ) {
        glutSetCursor(cursor);
    } else {
        cursor = glutGet( (GLenum)GLUT_WINDOW_CURSOR );
#ifdef WIN32
        TurnCursorOn();
#endif
        glutSetCursor( GLUT_CURSOR_WAIT );
    }
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

// Repair any damage done to the Panel by other Gui Items
void guiFixPanel( void )
{
    int toggle_pause;

    if ( current_options.get_panel_status() ) {
        FGView *v = &current_view;
        FGTime *t = FGTime::cur_time_params;

        if( (toggle_pause = !t->getPause()) )
            t->togglePauseMode();

        // this seems to be the only way to do this :-(
        // problem is the viewport has been mucked with
        xglViewport(0, 0 , (GLint)(v->winWidth), (GLint)(v->winHeight) );
        FGPanel::OurPanel->ReInit(0, 0, 1024, 768);

        if(toggle_pause)
            t->togglePauseMode();
    }
}

// Toggle the Menu and Mouse display state
void guiToggleMenu(void)
{
    if( menu_on ) {
        // printf("Hiding Menu\n");
        mainMenuBar->hide  ();
#ifdef WIN32
        TurnCursorOff();
#endif // #ifdef WIN32
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

void reInit(puObject *cb)
{
    BusyCursor(0);
    fgReInitSubsystems();
    BusyCursor(1);
}
	
// This is the accessor function
void guiTogglePanel(puObject *cb)
{
    current_options.toggle_panel();
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

    //  if(gps_bug)
    //      fclose(gps_bug);

    exit(-1);
}


void goAwayCb (puObject *me)
{
    FG_POP_PUI_DIALOG( dialogBox );
}

void mkDialogInit (void)
{
    //  printf("mkDialogInit\n");
    int x = (current_options.get_xsize()/2 - 400/2);
    int y = (current_options.get_ysize()/2 - 100/2);
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

    int x = (current_options.get_xsize()/2 - 400/2);
    int y = (current_options.get_ysize()/2 - 100/2);
	
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
        YNdialogBoxOkButton -> setCallback        (goodBye);
        
        YNdialogBoxNoButton =  new puOneShot      (240, 10, 300, 50);
        YNdialogBoxNoButton -> setLegend          (gui_msg_NO);
        //      YNdialogBoxNoButton -> makeReturnDefault  (TRUE );
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



#ifdef WIN32
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

    CGlPrinter p;
    p.Begin( "FlightGear" );
    fgInitVisuals();
    fgReshape( p.GetHorzRes(), p.GetVertRes() );
    fgRenderFrame();
    p.End();

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


// do a screen dump
void dumpScreen ( puObject *obj ) {
    bool show_pu_cursor = false;

    mainMenuBar->hide();
    TurnCursorOff();
    if ( !puCursorIsHidden() ) {
	show_pu_cursor = true;
	puHideCursor();
    }

    fgInitVisuals();
    fgReshape( current_options.get_xsize(), current_options.get_ysize() );

    // we need two render frames here to clear the menu and cursor
    // ... not sure why but doing an extra fgFenderFrame() shoulnd't
    // hurt anything
    fgRenderFrame();
    fgRenderFrame();

    my_glDumpWindow( "fgfs-screen.ppm", 
		     current_options.get_xsize(), 
		     current_options.get_ysize() );
    
    mkDialog ("Screen dump saved to fgfs-screen.ppm");

    if ( show_pu_cursor ) {
	puShowCursor();
    }

    TurnCursorOn();
    if( menu_on ) {
	mainMenuBar->reveal();
    }

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
	fgAIRPORTS airports;
	fgAIRPORT a;
    
    FGTime *t = FGTime::cur_time_params;
    int PauseMode = t->getPause();
    if(!PauseMode)
        t->togglePauseMode();

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

        airports.load("apt_simple");
        if ( airports.search( AptId, &a ) )
        {
            current_options.set_airport_id( AptId.c_str() );
            BusyCursor(0);
            fgReInitSubsystems();
            BusyCursor(1);
        } else {
            AptId  += " not in database.";
            mkDialog(AptId.c_str());
        }
    }
    if( PauseMode != t->getPause() )
        t->togglePauseMode();
}

void AptDialog_Reset(puObject *)
{
    //  strncpy( NewAirportId, current_options.get_airport_id().c_str(), 16 );
    sprintf( NewAirportId, "%s", current_options.get_airport_id().c_str() );
    AptDialogInput->setValue ( NewAirportId );
    AptDialogInput->setCursor( 0 ) ;
}

void NewAirport(puObject *cb)
{
    //  strncpy( NewAirportId, current_options.get_airport_id().c_str(), 16 );
    sprintf( NewAirportId, "%s", current_options.get_airport_id().c_str() );
//	cout << "NewAirport " << NewAirportId << endl;
    AptDialogInput->setValue( NewAirportId );

    FG_PUSH_PUI_DIALOG( AptDialog );
}

static void NewAirportInit(void)
{
    sprintf( NewAirportId, "%s", current_options.get_airport_id().c_str() );
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

/// The beginnings of networking :-)
//  Needs cleaning up but works
//  These statics should disapear when this is a class
static puDialogBox     *NetIdDialog = 0;
static puFrame         *NetIdDialogFrame = 0;
static puText          *NetIdDialogMessage = 0;
static puInput         *NetIdDialogInput = 0;

static char NewNetId[16];
static char NewNetIdLabel[] = "Enter New Callsign"; 

static puOneShot       *NetIdDialogOkButton = 0;
static puOneShot       *NetIdDialogCancelButton = 0;

void NetIdDialog_Cancel(puObject *)
{
    FG_POP_PUI_DIALOG( NetIdDialog );
}

void NetIdDialog_OK (puObject *)
{
    string NetId;
    
    FGTime *t = FGTime::cur_time_params;
    int PauseMode = t->getPause();
    if(!PauseMode)
        t->togglePauseMode();
/*  
   The following needs some cleanup because 
   "string options.NetId" and "char *net_callsign" 
*/
    NetIdDialogInput->getValue(&net_callsign);
    NetId = net_callsign;
    
    NetIdDialog_Cancel( NULL );
    current_options.set_net_id( NetId.c_str() );
/* Entering a callsign indicates : user wants Net HUD Info */
    net_hud_display = 1;

    if( PauseMode != t->getPause() )
        t->togglePauseMode();
}

void NewCallSign(puObject *cb)
{
    sprintf( NewNetId, "%s", current_options.get_net_id().c_str() );
    NetIdDialogInput->setValue( NewNetId );

    FG_PUSH_PUI_DIALOG( NetIdDialog );
}

static void NewNetIdInit(void)
{
    sprintf( NewNetId, "%s", current_options.get_net_id().c_str() );
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

static void net_blaster_toggle( puObject *cb)
{
	net_blast_toggle = (net_blast_toggle) ? 0 : -1;
        printf("Toggle net_blast : %d\n", net_blast_toggle);
}

/***************  End Networking  **************/



/* -----------------------------------------------------------------------
The menu stuff 
---------------------------------------------------------------------*/
char *fileSubmenu               [] = {
    "Exit", /* "Close", "---------", */
#ifdef WIN32
    "Print",
#endif
    "Screen Dump",
    /* "---------", "Save", */ 
    "Reset", NULL
};
puCallback fileSubmenuCb        [] = {
    MayBeGoodBye, /* hideMenuCb, NULL, */
#ifdef WIN32
    printScreen, 
#endif
    /* NULL, notCb, */
    dumpScreen,
    reInit, NULL
};

/*
char *editSubmenu               [] = {
    "Edit text", NULL
};
puCallback editSubmenuCb        [] = {
    notCb, NULL
};
*/

char *viewSubmenu               [] = {
    /* "Cockpit View > ", "View >","------------", */ "Toggle Panel...", NULL
};
puCallback viewSubmenuCb        [] = {
    /* notCb, notCb, NULL, guiTogglePanel, */ NULL
};

char *aircraftSubmenu           [] = {
    "Autopilot", "Heading", "Altitude", "Navigation", "Airport", 
    /* "Communication", */ NULL
};
puCallback aircraftSubmenuCb    [] = {
    fgAPAdjust, NewHeading, NewAltitude, fgLatLonFormatToggle, NewTgtAirport, 
    /* notCb, */ NULL
};

char *environmentSubmenu        [] = {
    "Airport", /* "Terrain", "Weather", */ NULL
};
puCallback environmentSubmenuCb [] = {
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
    /* "Unregister from FGD ", "Send MSG to All", "Send MSG", "Show Pilots", */
    /* "Register to FGD", */
    /* "Scan for Deamons", */ "Enter Callsign", /* "Display Netinfos", */
    "Toggle Display",
    "Hyper Blast", NULL
};
puCallback networkSubmenuCb     [] = {
    /* notCb, notCb, notCb, notCb, notCb, notCb, */ NewCallSign, /* notCb, */
    net_display_toggle, net_blaster_toggle, NULL
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
        fntpath.set( current_options.get_fg_root() );
	fntpath.append( "Fonts" );
    }

    // Install our fast fonts
    fntpath.append( "typewriter.txf" );
    guiFntHandle = new fntTexFont ;
    guiFntHandle -> load ( (char *)fntpath.c_str() ) ;
    puFont GuiFont ( guiFntHandle, 15 ) ;
    puSetDefaultFonts( GuiFont, GuiFont ) ;
    guiFnt = puGetDefaultLabelFont();
  
    if ( current_options.get_mouse_pointer() == 0 ) {
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
    } else if ( current_options.get_mouse_pointer() == 1 ) {
        // don't show pointer
    } else if ( current_options.get_mouse_pointer() == 2 ) {
        // force showing pointer
        puShowCursor();
        mouse_active = ~mouse_active;
    }

    // Set up our Dialog Boxes
    ConfirmExitDialogInit();
    NewAirportInit();
#ifdef FG_NETWORK_OLK
    NewNetIdInit();
#endif
    mkDialogInit();
    
    // Make the menu bar
    mainMenuBar = new puMenuBar ();
    mainMenuBar -> add_submenu ("File", fileSubmenu, fileSubmenuCb);
    // mainMenuBar -> add_submenu ("Edit", editSubmenu, editSubmenuCb);
    mainMenuBar -> add_submenu ("View", viewSubmenu, viewSubmenuCb);
    mainMenuBar -> add_submenu ("Aircraft", aircraftSubmenu, aircraftSubmenuCb);
    mainMenuBar -> add_submenu ("Environment", environmentSubmenu, environmentSubmenuCb);
    // mainMenuBar -> add_submenu ("Options", optionsSubmenu, optionsSubmenuCb);
#ifdef FG_NETWORK_OLK
    mainMenuBar -> add_submenu ("Network", networkSubmenu, networkSubmenuCb);
#endif
    mainMenuBar -> add_submenu ("Help", helpSubmenu, helpSubmenuCb);
    mainMenuBar-> close ();
    // Set up menu bar toggle
    menu_on = ~0;
}
