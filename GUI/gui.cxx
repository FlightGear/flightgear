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
 * (Log is kept at end of this file)
 **************************************************************************/


#include <stdlib.h>
#include <string.h>

#include <Include/general.h>
#include <Main/options.hxx>

#include "gui.h"


puMenuBar    *mainMenuBar;
puButton     *hideMenuButton;
puDialogBox  *dialogBox;
puText       *dialogBoxMessage;
puOneShot    *dialogBoxOkButton;
puText       *timerText;

/* --------------------------------------------------------------------
       Mouse stuff 
  ---------------------------------------------------------------------*/

void guiMotionFunc ( int x, int y )
{
  puMouse ( x, y ) ;
  glutPostRedisplay () ;
}


void guiMouseFunc(int button, int updown, int x, int y)
{
    puMouse (button, updown, x,y);
    glutPostRedisplay ();
}

/* -----------------------------------------------------------------------
  the Gui callback functions 
  ____________________________________________________________________*/

void hideMenuCb (puObject *cb)
{
  if (cb -> getValue () )
    {
      mainMenuBar -> reveal();
      printf("Showing Menu");
      hideMenuButton -> setLegend ("Hide Menu");
    }
  else
    {
      mainMenuBar -> hide  ();
      printf("Hiding Menu");
      hideMenuButton -> setLegend ("Show Menu");
    }
}

 void goAwayCb (puObject *)
{
  delete dialogBox;
  dialogBox = NULL;
}

void mkDialog (char *txt)
{
  dialogBox = new puDialogBox (150, 50);
  {
    new puFrame (0,0,400, 100);
    dialogBoxMessage =   new puText         (10, 70);
    dialogBoxMessage ->  setLabel           (txt);
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

/* -----------------------------------------------------------------------
   The menu stuff 
   ---------------------------------------------------------------------*/
char *fileSubmenu        [] = { "Exit", "Close", "---------", "Print", "---------", "Save", "New", NULL };
char *editSubmenu        [] = { "Edit text", NULL };
char *viewSubmenu        [] = { "Cockpit View > ", "View >","------------", "View options...", NULL };
char *aircraftSubmenu    [] = { "Autopilot ...", "Engine ...", "Navigation", "Communication", NULL};
char *environmentSubmenu [] = { "Time & Date...", "Terrain ...", "Weather", NULL};
char *optionsSubmenu     [] = { "Preferences", "Realism & Reliablity...", NULL};
char *helpSubmenu        [] = { "About...", "Help", NULL };

puCallback fileSubmenuCb        [] = { notCb, notCb, NULL, notCb, NULL, notCb, notCb, NULL};
puCallback editSubmenuCb        [] = { notCb, NULL };
puCallback viewSubmenuCb        [] = { notCb, notCb, NULL, notCb, NULL };
puCallback aircraftSubmenuCb    [] = { notCb, notCb, notCb,notCb, NULL };
puCallback environmentSubmenuCb [] = { notCb, notCb, notCb, NULL };
puCallback optionsSubmenuCb     [] = { notCb, notCb, NULL};
puCallback helpSubmenuCb        [] = { notCb, notCb, NULL };

 

/* -------------------------------------------------------------------------
   init the gui
   _____________________________________________________________________*/



void guiInit()
{
    fgGENERAL *g;
    char *mesa_win_state;

    g = &general;

    // Initialize PUI
    puInit();

    if ( current_options.get_mouse_pointer() == 0 ) {
	// no preference specified for mouse pointer, attempt to autodetect...
	// Determine if we need to render the cursor, or if the windowing
	// system will do it.  First test if we are rendering with glide.
	if ( strstr ( g->glRenderer, "Glide" ) ) {
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
    puSetDefaultColourScheme  (0.2, 0.4, 0.8, 0.5);
      
    /* OK the rest is largerly put in here to mimick Steve Baker's
       "complex" example It should change in future versions */
      
    // timerText = new puText (300, 10);
    // timerText -> setColour (PUCOL_LABEL, 1.0, 1.0, 1.0);

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
