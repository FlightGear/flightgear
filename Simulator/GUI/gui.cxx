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
#include <Time/fg_time.hxx>

#include "gui.h"

FG_USING_STD(string);

// Pui hides low level access to the mouse and menu display states
// so for now we track these in case we want to know about them
puFont guiFnt = 0;               // Our High Level Gui Font
fntTexFont *guiFntHandle = 0;    // lower level access to guiFnt's Font
static int _mX = 0;              // current mouse x
static int  _mY = 0;             //               y
static int last_buttons = 0 ;    // mouse button state
static int mouse_active = 0;     // is mouse displayed
static int menu_on = 0;          //    menu  displayed


// PUI objects
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

// Accessor CallBacks for external PUI Objects
extern void NewAltitude( puObject *cb );
extern void NewHeading( puObject *cb );
extern void fgAPAdjust( puObject * );
extern void fgLatLonFormatToggle( puObject *);

/* --------------------------------------------------------------------
Mouse stuff
---------------------------------------------------------------------*/
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

static inline void TurnCursorOn( void )
{
    mouse_active = ~0;
#if defined ( WIN32 ) || defined(__CYGWIN32__)
    glutSetCursor(GLUT_CURSOR_INHERIT);
#endif
#if (GLUT_API_VERSION >= 4 || GLUT_XLIB_IMPLEMENTATION >= 9)
    glutWarpPointer( glutGet(GLUT_SCREEN_WIDTH)/2, glutGet(GLUT_SCREEN_HEIGHT)/2);
#endif
}

static inline void TurnCursorOff( void )
{
    mouse_active = 0;
#if defined ( WIN32 ) || defined(__CYGWIN32__)
    glutSetCursor(GLUT_CURSOR_NONE);
#else  // I guess this is what we want to do ??
 #if (GLUT_API_VERSION >= 4 || GLUT_XLIB_IMPLEMENTATION >= 9)
    glutWarpPointer( glutGet(GLUT_SCREEN_WIDTH), glutGet(GLUT_SCREEN_HEIGHT));
 #endif
#endif
}

// If the gui isn't active try to hide the mouse
// an if the qui requested show the mouse
// this can get confused if it rerequesting the gui object
// should resync things.
// see PUI macros in gui.hxx for normal use
void maybeToggleMouse( void )
{
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
}

// Call with FALSE to init and TRUE to restore
void BusyCursor( int restore )
{
    static int cursor = 0;
    if( restore ) {
        glutSetCursor(cursor);
    } else {
        cursor = glutGet( GLUT_WINDOW_CURSOR );
        TurnCursorOn();
        glutSetCursor( GLUT_CURSOR_WAIT );
    }
}
/* ================ General Purpose Functions ================ */

// Intercept the Escape Key
void ConfirmExitDialog(void)
{
    string Msg("Really Quit");
    YNdialogBoxMessage -> setLabel(Msg.c_str());    
    YNdialogBoxNoButton-> makeReturnDefault  (TRUE );
    FG_PUSH_PUI_DIALOG( YNdialogBox );
}

// General Purpose Message Box
void mkDialog (char *txt)
{
    void goAwayCb(puObject *);
    dialogBoxMessage->setLabel(txt);
    dialogBoxOkButton -> setLegend          ("OK");
    dialogBoxOkButton -> makeReturnDefault  (TRUE );
    dialogBoxOkButton -> setCallback        (goAwayCb);
    FG_PUSH_PUI_DIALOG( dialogBox );
}

// Repair any damage done to the Panel by other Gui Items
// see PUI macros in gui.hxx for use
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
        TurnCursorOff();
    } else {
        // printf("Showing Menu\n");
        mainMenuBar->reveal();
        TurnCursorOn();
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

// This is the accessor function
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
    FG_POP_PUI_DIALOG( dialogBox );
}

void mkDialogInit (void)
{
//  printf("mkDialogInit\n");
    dialogBox = new puDialogBox (150, 50);
    {
        dialogFrame = new puFrame (0,0,400, 100);
        dialogBoxMessage  =  new puText         (10, 70);
        dialogBoxMessage  -> setLabel           ("");
        dialogBoxOkButton =  new puOneShot      (180, 10, 240, 50);
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
//  printf("ConfirmExitDialogInit\n");
    string Msg("Really Quit");
//  int len = 350/2 - puGetStringWidth( puGetDefaultLabelFont(), AptLabel )/2;
    int len = 200 - puGetStringWidth( puGetDefaultLabelFont(), Msg.c_str() )/2;
    
    YNdialogBox = new puDialogBox (150, 50);
    {
        YNdialogFrame = new puFrame (0,0,400, 100);
        
        YNdialogBoxMessage  =  new puText         (len, 70);
        YNdialogBoxMessage  -> setLabel           (Msg.c_str());
        
        YNdialogBoxOkButton =  new puOneShot      (100, 10, 160, 50);
        YNdialogBoxOkButton -> setLegend          ("OK");
        YNdialogBoxOkButton -> setCallback        (goodBye);
        
        YNdialogBoxNoButton =  new puOneShot      (240, 10, 300, 50);
        YNdialogBoxNoButton -> setLegend          ("NO");
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
    string text = "Help started in netscape window.";
	
    mkDialog (text.c_str());
}

/// The beginnings of teleportation :-)
//  Needs cleaning up but works
//  These statics should disapear when this is a class
static puDialogBox     *AptDialog = 0;
static puFrame         *AptDialogFrame = 0;
static puText          *AptDialogMessage = 0;
static puInput         *AptDialogInput = 0;

static puOneShot       *AptDialogOkButton = 0;
static puOneShot       *AptDialogCancelButton = 0;
static puOneShot       *AptDialogResetButton = 0;

void AptDialog_Cancel(puObject *)
{
    AptDialogOkButton->makeReturnDefault(FALSE);
    AptDialogInput->rejectInput();
    FG_POP_PUI_DIALOG( AptDialog );
}

void AptDialog_OK (puObject *)
{
    string AptId;
    
    FGTime *t = FGTime::cur_time_params;
    int PauseMode = t->getPause();
    if(!PauseMode)
        t->togglePauseMode();

    char *s;
    AptDialogInput->getValue(&s);
    AptId = s;
    
    AptDialog_Cancel( NULL );
    
    if ( AptId.length() ) {
        // set initial position from airport id
        
        fgAIRPORTS airports;
        fgAIRPORT a;
        
        FG_LOG( FG_GENERAL, FG_INFO,
                "Attempting to set starting position from airport code "
                << s );
        
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
    AptDialogInput->setValue ( current_options.get_airport_id().c_str() );
    AptDialogInput->setCursor( 0 ) ;
}


// This is the accessor function
void NewAirport(puObject *cb)
{
    string AptLabel("Enter New Airport ID");
    AptDialogMessage ->setLabel( AptLabel.c_str() );
    AptDialogInput   ->setValue( current_options.get_airport_id().c_str() );
    AptDialogInput   ->acceptInput();
    AptDialogOkButton->makeReturnDefault(TRUE);
    
    FG_PUSH_PUI_DIALOG( AptDialog );
}

static void NewAirportInit(void)
{
    cout << "NewAirportInit" << endl;

    string AptLabel("Enter New Airport ID");
//  int len = 350/2 - puGetStringWidth( puGetDefaultLabelFont(), AptLabel )/2;
    int len = 150 - puGetStringWidth( puGetDefaultLabelFont(), AptLabel.c_str() )/2;
    
    AptDialog = new puDialogBox (150, 50);
    {
        AptDialogFrame   = new puFrame           (0,0,350, 150);
        AptDialogMessage = new puText            (len, 110);
        
        AptDialogInput   = new puInput           ( 50, 70, 300, 100 );
        
        AptDialogOkButton     =  new puOneShot   (50, 10, 110, 50);
        AptDialogOkButton     ->     setLegend   ("OK");
        AptDialogOkButton     ->     setCallback (AptDialog_OK);
        
        AptDialogCancelButton =  new puOneShot   (140, 10, 210, 50);
        AptDialogCancelButton ->     setLegend   ("Cancel");
        AptDialogCancelButton ->     setCallback (AptDialog_Cancel);
        
        AptDialogResetButton  =  new puOneShot   (240, 10, 300, 50);
        AptDialogResetButton  ->     setLegend   ("Reset");
        AptDialogResetButton  ->     setCallback (AptDialog_Reset);
    }
    FG_FINALIZE_PUI_DIALOG( AptDialog );
}


/* -----------------------------------------------------------------------
   The menu stuff 
   ---------------------------------------------------------------------*/
char *fileSubmenu               [] = {
    "Exit", "Close", "---------", "Print", "---------", "Save", "Reset", NULL };
puCallback fileSubmenuCb        [] = {
    MayBeGoodBye, hideMenuCb, NULL, notCb, NULL, notCb, reInit, NULL};  
    
char *editSubmenu               [] = {
    "Edit text", NULL };
puCallback editSubmenuCb        [] = {
    notCb, NULL };

char *viewSubmenu               [] = {
    "Cockpit View > ", "View >","------------", "Toggle Panel...", NULL };
puCallback viewSubmenuCb        [] = {
    notCb, notCb, NULL, guiTogglePanel, NULL };
	
char *aircraftSubmenu           [] = {
    "Autopilot", "Heading", "Altitude", "Navigation", "Communication", NULL};
puCallback aircraftSubmenuCb    [] = {
    fgAPAdjust, NewHeading, NewAltitude, fgLatLonFormatToggle, notCb, NULL };

char *environmentSubmenu        [] = {
    "Airport", "Terrain", "Weather", NULL};
puCallback environmentSubmenuCb [] = {
    NewAirport, notCb, notCb, NULL };

char *optionsSubmenu            [] = {
    "Preferences", "Realism & Reliablity...", NULL};
puCallback optionsSubmenuCb     [] = {
    notCb, notCb, NULL};

char *helpSubmenu               [] = {
    "About...", "Help", NULL };
puCallback helpSubmenuCb        [] = {
    notCb, helpCb, NULL };


/* -------------------------------------------------------------------------
   init the gui
   _____________________________________________________________________*/



void guiInit()
{
    char *mesa_win_state;

    // Initialize PUI
    puInit();
    puSetDefaultStyle         ( PUSTYLE_DEFAULT );
    puSetDefaultColourScheme  (0.8, 0.8, 0.8, 0.4);

    // Install our fast fonts
    string fntpath = current_options.get_fg_root() + "/Fonts/" + 
        "typewriter" + ".txf";
    guiFntHandle = new fntTexFont ;
    guiFntHandle -> load ( fntpath.c_str() ) ;
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
    mkDialogInit();
    
    // Make the menu bar
    mainMenuBar = new puMenuBar ();
    mainMenuBar -> add_submenu ("File", fileSubmenu, fileSubmenuCb);
    mainMenuBar -> add_submenu ("Edit", editSubmenu, editSubmenuCb);
    mainMenuBar -> add_submenu ("View", viewSubmenu, viewSubmenuCb);
    mainMenuBar -> add_submenu ("Aircraft", aircraftSubmenu, aircraftSubmenuCb);
    mainMenuBar -> add_submenu ("Environment", environmentSubmenu, environmentSubmenuCb);
    mainMenuBar -> add_submenu ("Options", optionsSubmenu, optionsSubmenuCb);
    mainMenuBar -> add_submenu ("Help", helpSubmenu, helpSubmenuCb);
    mainMenuBar-> close ();
    // Set up menu bar toggle
    menu_on = ~0;
}
