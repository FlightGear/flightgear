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

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <simgear/misc/props.hxx>
#include <simgear/misc/props_io.hxx>
#include <simgear/misc/exception.hxx>
#include <simgear/misc/sg_path.hxx>

#include <plib/pu.h>

#include <Include/general.hxx>
#include <Main/globals.hxx>
#include <Main/fg_props.hxx>

#include "gui.h"
#include "gui_local.hxx"
#include "apt_dlg.hxx"
#include "net_dlg.hxx"


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

             int pos = option.size()-i-1;
             if (sep)
                Menu[h].submenu[pos] = strdup("----------");

             else if (call && strcmp(call->getStringValue(), ""))
                 Menu[h].submenu[pos] = strdup(name->getStringValue());

             else
                 Menu[h].submenu[pos] = strdup("not specified");

             Menu[h].cb[pos] = NULL;
             for (unsigned int j=0; __fg_gui_fn[j].fn; j++)
                 if (call &&
                     !strcmp(call->getStringValue(), __fg_gui_fn[j].name) )
                 {
                     Menu[h].cb[pos] = __fg_gui_fn[j].fn;
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
    fntpath.append(fgGetString("/sim/font", "typewriter.txf"));
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
