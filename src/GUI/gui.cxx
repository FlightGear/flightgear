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
#include <Main/viewmgr.hxx>

#ifdef FG_NETWORK_OLK
#include <NetworkOLK/network.h>
#endif
   
#if defined( WIN32 ) && !defined( __CYGWIN__ ) && !defined(__MINGW32__)
#  include <simgear/screen/win32-printer.h>
#  include <simgear/screen/GlBitmaps.h>
#endif

#include "gui.h"
#include "gui_local.hxx"
#include "apt_dlg.hxx"
#include "net_dlg.hxx"
#include "sgVec3Slider.hxx"
#include "prop_picker.hxx"

SG_USING_STD(string);

#ifndef SG_HAVE_NATIVE_SGI_COMPILERS
SG_USING_STD(cout);
#endif

// main.cxx hack, should come from an include someplace
extern void fgInitVisuals( void );
extern void fgReshape( int width, int height );
extern void fgRenderFrame( void );

extern void initDialog (void);
extern void mkDialogInit (void);
extern void ConfirmExitDialogInit(void);


puFont guiFnt = 0;
fntTexFont *guiFntHandle = 0;
int gui_menu_on = 0;
puMenuBar    *mainMenuBar = 0;
//static puButton     *hideMenuButton = 0;

// from cockpit.cxx
extern void fgLatLonFormatToggle( puObject *);

// from gui_funcs.cxx
extern void saveFlight(puObject *);
extern void loadFlight(puObject *);
extern void reInit(puObject *);
extern void dumpSnapShot(puObject *);
#ifdef TR_HIRES_SNAP
extern void dumpHiResSnapShot(puObject *);
#endif
#if defined( WIN32 ) && !defined( __CYGWIN__) && !defined(__MINGW32__)
extern void printScreen(puObject *);
#endif
extern void MayBeGoodBye(puObject *);
extern void guiTogglePanel(puObject *);
extern void PilotOffsetAdjust(puObject *);
extern void fgHUDalphaAdjust(puObject *);
extern void prop_pickerView(puObject *);
extern void NewAirport(puObject *);
#ifdef FG_NETWORK_OLK
extern void net_display_toggle(puObject *);
extern void NewCallSign(puObject *);
extern void net_fgd_scan(puObject *);
extern void net_register(puObject *);
extern void net_unregister(puObject *);
#endif
extern void NewAltitude(puObject *);
extern void AddWayPoint(puObject *);
extern void PopWayPoint(puObject *);
extern void ClearRoute(puObject *);
extern void fgAPAdjust(puObject *);
extern void fgLatLonFormatToggle(puObject *);
extern void helpCb(puObject *);

static const struct {
        char *name;
        void (*fn)(puObject *);
} __fg_gui_fn[] = {

        // File
        {"saveFlight", saveFlight},
        {"loadFlight", loadFlight},
        {"reInit", reInit},
#ifdef TR_HIRES_SNAP
        {"dumpHiResSnapShot", dumpHiResSnapShot},
#endif
        {"dumpSnapShot", dumpSnapShot},
#if defined( WIN32 ) && !defined( __CYGWIN__) && !defined(__MINGW32__)
        {"printScreen", printScreen},
#endif
        {"MayBeGoodBye", MayBeGoodBye},

        //View
        {"guiTogglePanel", guiTogglePanel},
        {"PilotOffsetAdjust", PilotOffsetAdjust},
        {"fgHUDalphaAdjust", fgHUDalphaAdjust},
        {"prop_pickerView", prop_pickerView},

        // Environment
        {"NewAirport", NewAirport},

        // Network
#ifdef FG_NETWORK_OLK
        {"net_display_toggle", net_display_toggle},
        {"NewCallSign", NewCallSign},
        {"net_fgd_scan", net_fgd_scan},
        {"net_register", net_register},
        {"net_unregister", net_unregister},
#endif

        // Autopilot
        {"NewAltitude", NewAltitude},
        {"AddWayPoint", AddWayPoint},
        {"PopWayPoint", PopWayPoint},
        {"ClearRoute", ClearRoute},
        {"fgAPAdjust", fgAPAdjust},
        {"fgLatLonFormatToggle", fgLatLonFormatToggle},

        // Help
        {"helpCb", helpCb},

        // Structure termination
        {"", NULL}
};



struct fg_gui_t {
	char *name;
	char **submenu;
	puCallback *cb;
} *Menu;
unsigned int Menu_size;

void initMenu()
{
     SGPropertyNode main;
     SGPath spath( globals->get_fg_root() );
     spath.append( "menu.xml" );

     try {
         readProperties(spath.c_str(), &main);
     } catch (const sg_exception &ex) {
         SG_LOG(SG_GENERAL, SG_ALERT, "Error processing the menu file.");
         return;
     }

     SG_LOG(SG_GENERAL, SG_INFO, "Reading menu entries.");

     // Make the menu bar
     mainMenuBar = new puMenuBar ();

     SGPropertyNode *menu = main.getChild("menu");

     vector<SGPropertyNode_ptr>submenu = menu->getChildren("submenu");

     Menu_size = 1+submenu.size();
     Menu = (fg_gui_t *)calloc(Menu_size, sizeof(fg_gui_t));

     for (unsigned int h = 0; h < submenu.size(); h++) {

         vector<SGPropertyNode_ptr>option = submenu[h]->getChildren("option");

         //
         // Make sure all entries will fit into allocated memory
         //
         Menu[h].submenu = (char **)calloc(1+option.size(), sizeof(char *));
         Menu[h].cb = (puCallback *)calloc(1+option.size(), sizeof(puCallback));

         for (unsigned int i = 0; i < option.size(); i++) {

             SGPropertyNode *name = option[i]->getNode("name");
             SGPropertyNode *call = option[i]->getNode("call");
             SGPropertyNode *sep = option[i]->getNode("seperator");

             if (sep)
                Menu[h].submenu[i] = strdup("----------");

             else if (call && strcmp(call->getStringValue(), ""))
                 Menu[h].submenu[i] = strdup(name->getStringValue());

             else
                 Menu[h].submenu[i] = strdup("not specified");

             Menu[h].cb[i] = NULL;
             for (unsigned int j=0; __fg_gui_fn[j].fn; j++)
                 if (call &&
                     !strcmp(call->getStringValue(), __fg_gui_fn[j].name) )
                 {
                     Menu[h].cb[i] = __fg_gui_fn[j].fn;
                     break;
                 }
         }

         SGPropertyNode *name = submenu[h]->getNode("name");

         Menu[h].name = strdup(name->getStringValue());
         mainMenuBar->add_submenu(Menu[h].name, Menu[h].submenu, Menu[h].cb);

    }

    mainMenuBar->close();
}


// FIXME: Has to be called from somewhere
//        or better yet, turn the menu into a class of its own
void destroyMenu(void) {
    for(unsigned int i=0; i < Menu_size; i++) {

        free(Menu[i].name);

                                // FIXME: don't use strdup/free
        for(unsigned int j=0; Menu[i].submenu[j] != NULL; j++)
           free(Menu[i].submenu[j]);
    }
}




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

    initDialog();

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
  
    if (!fgHasNode("/sim/startup/mouse-pointer")) {
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

    initMenu();
    
    // Set up menu bar toggle
    gui_menu_on = ~0;

    if (!strcmp(fgGetString("/sim/flight-model"), "ada")) {
        guiToggleMenu(); // Menu off by default
    }
}
