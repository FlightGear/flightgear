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

#include STL_FSTREAM
#include STL_STRING

#include <stdlib.h>
#include <string.h>

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/screen/screen-dump.hxx>

#include <Include/general.hxx>
#include <Aircraft/aircraft.hxx>
#include <Airports/simple.hxx>
#include <Autopilot/auto_gui.hxx>
#include <Autopilot/newauto.hxx>
#include <Cockpit/panel.hxx>
#include <Controls/controls.hxx>
#include <FDM/flight.hxx>
#include <Main/fg_init.hxx>
#include <Main/fg_io.hxx>
#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <Main/options.hxx>

#ifdef FG_NETWORK_OLK
#include <NetworkOLK/network.h>
#endif
   
#if defined( WIN32 ) && !defined( __CYGWIN__ )
#  include <simgear/screen/win32-printer.h>
#  include <simgear/screen/GlBitmaps.h>
#endif

#include "gui.h"
#include "gui_local.hxx"
#include "apt_dlg.hxx"
#include "net_dlg.hxx"
#include "sgVec3Slider.hxx"

SG_USING_STD(string);

#ifndef SG_HAVE_NATIVE_SGI_COMPILERS
SG_USING_STD(cout);
#endif

#ifdef  _MSC_VER
#define  snprintf    _snprintf
#endif   /* _MSC_VER */

// main.cxx hack, should come from an include someplace
extern void fgInitVisuals( void );
extern void fgReshape( int width, int height );
extern void fgRenderFrame( void );

puFont guiFnt = 0;
fntTexFont *guiFntHandle = 0;
int gui_menu_on = 0;
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

// from cockpit.cxx
extern void fgLatLonFormatToggle( puObject *);


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
    if( gui_menu_on ) {
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
    gui_menu_on = ~gui_menu_on;
}
    
/* -----------------------------------------------------------------------
the Gui callback functions 
____________________________________________________________________*/


// Hier Neu :-) This is my newly added code
// Added by David Findlay <nedz@bigpond.com>
// on Sunday 3rd of December

// Start new Save Dialog Box
static puDialogBox     *SaveDialog = 0;
static puFrame         *SaveDialogFrame = 0;
static puText          *SaveDialogMessage = 0;
static puInput         *SaveDialogInput = 0;

static puOneShot       *SaveDialogOkButton = 0;
static puOneShot       *SaveDialogCancelButton = 0;
static puOneShot       *SaveDialogResetButton = 0;

// Default save filename
static char saveFile[256] = "fgfs.sav";

// Cancel Button
void SaveDialogCancel(puObject *) {
    FG_POP_PUI_DIALOG( SaveDialog );
}

// If press OK do this
void SaveDialogOk(puObject*) {

    FG_POP_PUI_DIALOG( SaveDialog );

    char *s;
    SaveDialogInput->getValue(&s);

    ofstream output(s);
    cout << saveFile << endl;
    if (output.good() && fgSaveFlight(output)) {
	output.close();
	mkDialog("Saved flight");
	SG_LOG(SG_INPUT, SG_INFO, "Saved flight");
    } else {
	mkDialog("Cannot save flight");
	SG_LOG(SG_INPUT, SG_ALERT, "Cannot save flight");
    }
}

// Create Dialog
static void saveFlight(puObject *cv) {
    SaveDialog = new puDialogBox (150, 50);
    {
        SaveDialogFrame   = new puFrame           (0,0,350, 150);
        SaveDialogMessage = new puText            (
			(150 - puGetStringWidth( puGetDefaultLabelFont(), "File Name:" ) / 2), 110);
        SaveDialogMessage ->    setLabel          ("File Name:");

        SaveDialogInput   = new puInput           (50, 70, 300, 100);
        SaveDialogInput   ->    setValue          (saveFile);
        SaveDialogInput   ->    acceptInput();

        SaveDialogOkButton     =  new puOneShot   (50, 10, 110, 50);
        SaveDialogOkButton     ->     setLegend   (gui_msg_OK);
        SaveDialogOkButton     ->     setCallback ( SaveDialogOk );
        SaveDialogOkButton     ->     makeReturnDefault(TRUE);

        SaveDialogCancelButton =  new puOneShot   (140, 10, 210, 50);
        SaveDialogCancelButton ->     setLegend   (gui_msg_CANCEL);
        SaveDialogCancelButton ->     setCallback ( SaveDialogCancel );
    }
    FG_FINALIZE_PUI_DIALOG( SaveDialog );
   
    SaveDialog -> reveal();
}

// Load Dialog Start
static puDialogBox     *LoadDialog = 0;
static puFrame         *LoadDialogFrame = 0;
static puText          *LoadDialogMessage = 0;
static puInput         *LoadDialogInput = 0;

static puOneShot       *LoadDialogOkButton = 0;
static puOneShot       *LoadDialogCancelButton = 0;
static puOneShot       *LoadDialogResetButton = 0;

// Default load filename
static char loadFile[256] = "fgfs.sav";

// Do this if the person click okay
void LoadDialogOk(puObject *) {

    FG_POP_PUI_DIALOG( LoadDialog );

    char *l;
    LoadDialogInput->getValue(&l);

    ifstream input(l);
    if (input.good() && fgLoadFlight(input)) {
	input.close();
	mkDialog("Loaded flight");
	SG_LOG(SG_INPUT, SG_INFO, "Restored flight");
    } else {
	mkDialog("Failed to load flight");
	SG_LOG(SG_INPUT, SG_ALERT, "Cannot load flight");
    }
}

// Do this is the person presses cancel
void LoadDialogCancel(puObject *) {
    FG_POP_PUI_DIALOG( LoadDialog );
}

// Create Load Dialog
static void loadFlight(puObject *cb)
{
    LoadDialog = new puDialogBox (150, 50);
    {
        LoadDialogFrame   = new puFrame           (0,0,350, 150);
        LoadDialogMessage = new puText            (
			(150 - puGetStringWidth( puGetDefaultLabelFont(), "File Name:" ) / 2), 110);
        LoadDialogMessage ->    setLabel          ("File Name:");

        LoadDialogInput   = new puInput           (50, 70, 300, 100);
        LoadDialogInput   ->    setValue          (loadFile);
        LoadDialogInput   ->    acceptInput();

        LoadDialogOkButton     =  new puOneShot   (50, 10, 110, 50);
        LoadDialogOkButton     ->     setLegend   (gui_msg_OK);
        LoadDialogOkButton     ->     setCallback ( LoadDialogOk );
        LoadDialogOkButton     ->     makeReturnDefault(TRUE);

        LoadDialogCancelButton =  new puOneShot   (140, 10, 210, 50);
        LoadDialogCancelButton ->     setLegend   (gui_msg_CANCEL);
        LoadDialogCancelButton ->     setCallback ( LoadDialogCancel );
    }
    FG_FINALIZE_PUI_DIALOG( LoadDialog );
   
    LoadDialog -> reveal();
}

// This is the accessor function
void guiTogglePanel(puObject *cb)
{
  if (fgGetBool("/sim/panel/visibility"))
    fgSetBool("/sim/panel/visibility", false);
  else
    fgSetBool("/sim/panel/visibility", true);

  fgReshape(fgGetInt("/sim/startup/xsize"),
	    fgGetInt("/sim/startup/ysize"));
}
    
//void MenuHideMenuCb(puObject *cb)
void hideMenuCb (puObject *cb)
{
    guiToggleMenu();
}

void goodBye(puObject *)
{
    // SG_LOG( SG_INPUT, SG_ALERT,
    //      "Program exiting normally at user request." );
    cout << "Program exiting normally at user request." << endl;

#ifdef FG_NETWORK_OLK    
    if ( fgGetBool("/sim/networking/network-olk") ) {
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
    int x = (fgGetInt("/sim/startup/xsize")/2 - 400/2);
    int y = (fgGetInt("/sim/startup/ysize")/2 - 100/2);
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

    int x = (fgGetInt("/sim/startup/xsize")/2 - 400/2);
    int y = (fgGetInt("/sim/startup/ysize")/2 - 100/2);
	
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
    float oldfov = fgGetDouble("/sim/field-of-view");
    float fov = oldfov / multiplier;
    FGViewer *v = globals->get_current_view();
    fgSetDouble("/sim/field-of-view", fov);
    fgInitVisuals();
    int cur_width = fgGetInt("/sim/startup/xsize");
    int cur_height = fgGetInt("/sim/startup/ysize");
    if (b1) delete( b1 );
    // New empty (mostly) bitmap
    b1 = new GlBitmap( GL_RGB, 1, 1, (unsigned char *)"123" );
    int x,y;
    for ( y = 0; y < multiplier; y++ ) {
	for ( x = 0; x < multiplier; x++ ) {
	    fgReshape( cur_width, cur_height );
	    // pan to tile
	    rotateView( 0, (y*fov)-((multiplier-1)*fov/2), (x*fov)-((multiplier-1)*fov/2) );
	    fgRenderFrame();
	    // restore view
	    GlBitmap b2;
	    b1->copyBitmap( &b2, cur_width*x, cur_height*y );
	}
    }
    fgSetDouble("/sim/field-of-view", oldfov);
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
    int cur_width = fgGetInt("/sim/startup/xsize");
    int cur_height = fgGetInt("/sim/startup/ysize");
    p.Begin( "FlightGear", cur_width*3, cur_height*3 );
	p.End( hiResScreenCapture(3) );

    if( gui_menu_on ) {
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
    char *filename = new char [24];
    string message;
    static int count = 1;

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
    fgReshape( fgGetInt("/sim/startup/xsize"),
	       fgGetInt("/sim/startup/ysize") );

    // we need two render frames here to clear the menu and cursor
    // ... not sure why but doing an extra fgRenderFrame() shouldn't
    // hurt anything
    fgRenderFrame();
    fgRenderFrame();

    while (count < 1000) {
        FILE *fp;
        snprintf(filename, 24, "fgfs-screen-%03d.ppm", count++);
        if ( (fp = fopen(filename, "r")) == NULL )
            break;
        fclose(fp);
    }

    if ( sg_glDumpWindow( filename,
			  fgGetInt("/sim/startup/xsize"), 
			  fgGetInt("/sim/startup/ysize")) ) {
	message = "Snap shot saved to ";
	message += filename;
    } else {
        message = "Failed to save to ";
	message += filename;
    }

    mkDialog (message.c_str());

    delete [] filename;

    if ( show_pu_cursor ) {
	puShowCursor();
    }

    TurnCursorOn();
    if( gui_menu_on ) {
	mainMenuBar->reveal();
    }

    if(!freeze)
        globals->set_freeze( false );
}

#ifdef FG_NETWORK_OLK
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

extern void net_fgd_scan(puObject *cb);
#endif // #ifdef FG_NETWORK_OLK

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
    "Pilot Offset",
    /* "Cockpit View > ", "View >","------------", */
    "Toggle Panel...", NULL
};
puCallback viewSubmenuCb        [] = {
    fgHUDalphaAdjust,
    PilotOffsetAdjust,
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
    "Goto Airport", /* "Terrain", "Weather", */ NULL
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
    SGPath fntpath;
    char* envp = ::getenv( "FG_FONTS" );
    if ( envp != NULL ) {
        fntpath.set( envp );
    } else {
        fntpath.set( globals->get_fg_root() );
	fntpath.append( "Fonts" );
    }

    // Install our fast fonts
    fntpath.append( "typewriter.txf" );
    guiFntHandle = new fntTexFont ;
    guiFntHandle -> load ( (char *)fntpath.c_str() ) ;
    puFont GuiFont ( guiFntHandle, 15 ) ;
    puSetDefaultFonts( GuiFont, GuiFont ) ;
    guiFnt = puGetDefaultLabelFont();
  
    if (!fgHasValue("/sim/startup/mouse-pointer")) {
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
//        mouse_active = ~mouse_active;
    } else if ( !fgGetBool("/sim/startup/mouse-pointer") ) {
        // don't show pointer
    } else {
        // force showing pointer
        puShowCursor();
//        mouse_active = ~mouse_active;
    }
	
    // MOUSE_VIEW mode stuff
	initMouseQuat();

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
    if ( fgGetBool("/sim/networking/network-olk") ) {
    	mainMenuBar -> add_submenu ("Network", networkSubmenu, networkSubmenuCb);
    }
#endif
    mainMenuBar -> add_submenu ("Help", helpSubmenu, helpSubmenuCb);
    mainMenuBar-> close ();
    // Set up menu bar toggle
    gui_menu_on = ~0;
}

