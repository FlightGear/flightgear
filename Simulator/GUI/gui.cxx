/**************************************************************************
 * gui.cxx
 *
 * Written 1998 by Durk Talsma, started Juni, 1998.  For the flight gear
 * project.
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

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <Include/compiler.h>

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
#include <FDM/flight.hxx>
#include <Main/options.hxx>
#include <Main/fg_init.hxx>
#include <Time/fg_time.hxx>

#include "gui.h"

FG_USING_STD(string);

static puMenuBar    *mainMenuBar;
static puButton     *hideMenuButton;

static puDialogBox  *dialogBox;
static puFrame      *dialogFrame;
static puText       *dialogBoxMessage;
static puOneShot    *dialogBoxOkButton;

extern void fgAPAdjust( puObject * );
// extern void fgLatLonFormatToggle( puObject *);
/* --------------------------------------------------------------------
Mouse stuff
---------------------------------------------------------------------*/

static int _mX = 0;
static int  _mY = 0;
static int last_buttons = 0 ;

void guiMotionFunc ( int x, int y )
{
    _mX = x;
    _mY = y;
    puMouse ( x, y ) ;
    glutPostRedisplay () ;
}


void guiMouseFunc(int button, int updown, int x, int y)
{
    _mX = x;
    _mY = y;
    if ( updown == PU_DOWN )
	last_buttons |=  ( 1 << button ) ;
    else
	last_buttons &= ~( 1 << button ) ;
	
    puMouse (button, updown, x,y);
    glutPostRedisplay ();
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

/* -----------------------------------------------------------------------
  the Gui callback functions 
  ____________________________________________________________________*/

void reInit(puObject *cb)
{
    fgReInitSubsystems();
}

void guiToggleMenu(void)
{
    hideMenuButton -> 
	setValue ((int) !(hideMenuButton -> getValue() ) );
    hideMenuButton -> invokeCallback();
}
	

void MenuHideMenuCb(puObject *cb)
{
    mainMenuBar -> hide  ();
    //	printf("Hiding Menu\n");
    hideMenuButton -> setLegend ("Show Menu");
#if defined ( WIN32 ) || defined(__CYGWIN32__)
    glutSetCursor(GLUT_CURSOR_NONE);
#else  // I guess this is what we want to do ??
    //	puHideCursor(); // does not work with VooDoo
#endif
}

void hideMenuCb (puObject *cb)
{
    if (cb -> getValue () )
	{
	    mainMenuBar -> reveal();
	    // printf("Showing Menu\n");
	    hideMenuButton -> setLegend ("Hide Menu");
#if defined ( WIN32 ) || defined(__CYGWIN32__)
	    glutSetCursor(GLUT_CURSOR_INHERIT);
#else  // I guess this is what we want to do ??
	    //	  puShowCursor(); // does not work with VooDoo
#endif
	}
    else
	{
	    mainMenuBar -> hide  ();
	    // printf("Hiding Menu\n");
	    hideMenuButton -> setLegend ("Show Menu");
#if defined ( WIN32 ) || defined(__CYGWIN32__)
	    glutSetCursor(GLUT_CURSOR_NONE);
#else  // I guess this is what we want to do ??
	    //	  puHideCursor(); // does not work with VooDoo
#endif
	}
}

void goodBye(puObject *)
{
    //	FG_LOG( FG_INPUT, FG_ALERT, 
    //			"Program exiting normally at user request." );
    cout << "Program exiting normally at user request." << endl;

    //	if(gps_bug)
    //		fclose(gps_bug);
				
    exit(-1);
}


void goAwayCb (puObject *me)
{
    delete dialogBoxOkButton;
    dialogBoxOkButton = NULL;
	
    delete dialogBoxMessage;
    dialogBoxMessage = NULL;
	
    delete dialogFrame;
    dialogFrame = NULL;

    delete dialogBox;
    dialogBox = NULL;
}

void mkDialog (char *txt)
{
    dialogBox = new puDialogBox (150, 50);
    {
	dialogFrame = new puFrame (0,0,400, 100);
	dialogBoxMessage  =  new puText         (10, 70);
	dialogBoxMessage  -> setLabel           (txt);
	dialogBoxOkButton =  new puOneShot      (180, 10, 240, 50);
	dialogBoxOkButton -> setLegend          ("OK");
	dialogBoxOkButton -> makeReturnDefault  (TRUE );
	dialogBoxOkButton -> setCallback        (goAwayCb);
    }
    dialogBox -> close();
    dialogBox -> reveal();
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
    string text = "Help started in netscape window.";
	
    mkDialog (text.c_str());
}

/// The beginnings of teleportation :-)
//  Needs cleaning up but works
//  These statics should disapear when this is a class
static puDialogBox     *AptDialog;
static puFrame         *AptDialogFrame;
static puText          *AptDialogMessage;
static puInput         *AptDialogInput;

static puOneShot       *AptDialogOkButton;
static puOneShot       *AptDialogCancelButton;
static puOneShot       *AptDialogResetButton;

static string AptDialog_OldAptId;
static string AptDialog_NewAptId;
static int    AptDialog_ValidAptId;

static void validateApt (puObject *inpApt)
{
    char *s;
    AptDialog_ValidAptId = 0;

    inpApt->getValue(&s);

    AptDialog_NewAptId = s;

    if ( AptDialog_NewAptId.length() ) {
	// set initial position from airport id

	fgAIRPORTS airports;
	fgAIRPORT a;

	FG_LOG( FG_GENERAL, FG_INFO, 
		"Attempting to set starting position from airport code "
		<< s );

	airports.load("apt_simple");
	if ( ! airports.search( AptDialog_NewAptId, &a ) ) {
	    string err_string = "Failed to find ";
	    err_string += s;
	    err_string += " in database.";
	    mkDialog(err_string.c_str());
	    FG_LOG( FG_GENERAL, FG_ALERT,
		    "Failed to find " << s << " in database." );
	} else {
	    AptDialog_ValidAptId = 1;
	    AptDialog_OldAptId = s;
	    current_options.set_airport_id(AptDialog_NewAptId);
	}
    }
	
    if( AptDialog_ValidAptId ) {
	fgReInitSubsystems();
    }
}

void AptDialog_Cancel(puObject *)
{
    FGTime *t = FGTime::cur_time_params;
	
    delete AptDialogResetButton;
    AptDialogResetButton = NULL;

    delete AptDialogCancelButton;
    AptDialogCancelButton = NULL;

    delete AptDialogOkButton;
    AptDialogOkButton = NULL;

    delete AptDialogInput;
    AptDialogInput = NULL;

    delete AptDialogMessage;
    AptDialogMessage = NULL;

    delete AptDialogFrame;
    AptDialogFrame = NULL;

    delete AptDialog;
    AptDialog = NULL;
	
    t->togglePauseMode();
}

void AptDialog_OK (puObject *me)
{
    validateApt(AptDialogInput);
    AptDialog_Cancel(me);
}

void AptDialog_Reset(puObject *)
{
    AptDialogInput->setValue ( AptDialog_OldAptId.c_str() );
    AptDialogInput->setCursor( 0 ) ;
}

void NewAirportInit(puObject *cb)
{
    FGInterface *f;
    FGTime *t;
	
    f = current_aircraft.fdm_state;
    t = FGTime::cur_time_params;
	
    char *AptLabel = "Enter New Airport ID";
    int len = 350/2 - puGetStringWidth(NULL, AptLabel)/2;

    AptDialog_OldAptId = current_options.get_airport_id();
    char *s            = AptDialog_OldAptId.c_str();

    AptDialog = new puDialogBox (150, 50);
    {
	AptDialogFrame   = new puFrame           (0,0,350, 150);
	AptDialogMessage = new puText            (len, 110);
	AptDialogMessage ->    setLabel          (AptLabel);

	AptDialogInput   = new puInput           ( 50, 70, 300, 100 );
	AptDialogInput   ->    setValue          ( s );
	// Uncomment the next line to have input active on startup
	AptDialogInput   ->    acceptInput       ( );
	// cursor at begining or end of line ?
	//len = strlen(s);
	len = 0;
	AptDialogInput   ->    setCursor         ( len );
	AptDialogInput   ->    setSelectRegion   ( 5, 9 );
		
	AptDialogOkButton     =  new puOneShot         (50, 10, 110, 50);
	AptDialogOkButton     ->     setLegend         ("OK");
	AptDialogOkButton     ->     makeReturnDefault (TRUE );
	AptDialogOkButton     ->     setCallback       (AptDialog_OK);
		
	AptDialogCancelButton =  new puOneShot         (140, 10, 210, 50);
	AptDialogCancelButton ->     setLegend         ("Cancel");
	AptDialogCancelButton ->     makeReturnDefault (TRUE );
	AptDialogCancelButton ->     setCallback       (AptDialog_Cancel);
		
	AptDialogResetButton  =  new puOneShot         (240, 10, 300, 50);
	AptDialogResetButton  ->     setLegend         ("Reset");
	AptDialogResetButton  ->     makeReturnDefault (TRUE );
	AptDialogResetButton  ->     setCallback       (AptDialog_Reset);
    }
    AptDialog -> close();
    AptDialog -> reveal();
}


/* -----------------------------------------------------------------------
   The menu stuff 
   ---------------------------------------------------------------------*/
char *fileSubmenu        [] = {
    "Exit", "Close", "---------", "Print", "---------", "Save", "Reset", NULL };
char *editSubmenu        [] = {
    "Edit text", NULL };
char *viewSubmenu        [] = {
    "Cockpit View > ", "View >","------------", "View options...", NULL };
char *aircraftSubmenu    [] = {
    "Autopilot ...", "Engine ...", "Navigation", "Communication", NULL};
char *environmentSubmenu [] = {
    "Airport", "Terrain", "Weather", NULL};
char *optionsSubmenu     [] = {
    "Preferences", "Realism & Reliablity...", NULL};
char *helpSubmenu        [] = {
    "About...", "Help", NULL };

puCallback fileSubmenuCb        [] = { goodBye, MenuHideMenuCb, NULL, notCb, NULL, notCb, reInit, NULL};
puCallback editSubmenuCb        [] = { notCb, NULL };
puCallback viewSubmenuCb        [] = { notCb, notCb, NULL, notCb, NULL };
puCallback aircraftSubmenuCb    [] = { fgAPAdjust, notCb, notCb, notCb, NULL };
puCallback environmentSubmenuCb [] = { NewAirportInit, notCb, notCb, NULL };
puCallback optionsSubmenuCb     [] = { notCb, notCb, NULL};
puCallback helpSubmenuCb        [] = { notCb, helpCb, NULL };

 

/* -------------------------------------------------------------------------
   init the gui
   _____________________________________________________________________*/



void guiInit()
{
    char *mesa_win_state;

    // Initialize PUI
    puInit();

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
    } else if ( current_options.get_mouse_pointer() == 1 ) {
	// don't show pointer
    } else if ( current_options.get_mouse_pointer() == 2 ) {
	// force showing pointer
	puShowCursor();
    }

    // puSetDefaultStyle         ( PUSTYLE_SMALL_BEVELLED );
    puSetDefaultStyle         ( PUSTYLE_DEFAULT );
    //    puSetDefaultColourScheme  (0.2, 0.4, 0.8, 0.5);
    puSetDefaultColourScheme  (0.8, 0.8, 0.8, 0.4);
    /* OK the rest is largely put in here to mimick Steve Baker's
       "complex" example It should change in future versions */

    /* Make a button to hide the menu bar */
    hideMenuButton = new puButton       (10,10, 150, 50);
    hideMenuButton -> setValue          (TRUE);
    hideMenuButton -> setLegend         ("Hide Menu");
    hideMenuButton -> setCallback       (hideMenuCb);
    hideMenuButton -> makeReturnDefault (TRUE);
    hideMenuButton -> hide();

    // Make the menu bar
    mainMenuBar = new puMenuBar ();
    mainMenuBar -> add_submenu ("File", fileSubmenu, fileSubmenuCb);
    mainMenuBar -> add_submenu ("Edit", editSubmenu, editSubmenuCb);
    mainMenuBar -> add_submenu ("View", viewSubmenu, viewSubmenuCb);
    mainMenuBar -> add_submenu ("Aircraft", aircraftSubmenu, aircraftSubmenuCb);
    mainMenuBar -> add_submenu ("Environment", environmentSubmenu,
				environmentSubmenuCb);
    mainMenuBar -> add_submenu ("Options", optionsSubmenu, optionsSubmenuCb);
    mainMenuBar -> add_submenu ("Help", helpSubmenu, helpSubmenuCb);
    mainMenuBar-> close ();
}
