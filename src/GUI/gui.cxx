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

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <simgear/structure/exception.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>

#include <plib/pu.h>

#include <Include/general.hxx>
#include <Main/main.hxx>
#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <GUI/new_gui.hxx>

#include "gui.h"
#include "gui_local.hxx"
#include "preset_dlg.hxx"
#include "layout.hxx"


puFont guiFnt = 0;


/* -------------------------------------------------------------------------
init the gui
_____________________________________________________________________*/


void guiInit()
{
    char *mesa_win_state;

    // Initialize PUI
#ifndef PU_USE_NONE
    puInit();
#endif
    puSetDefaultStyle         ( PUSTYLE_SMALL_SHADED ); //PUSTYLE_DEFAULT
    puSetDefaultColourScheme  (0.8, 0.8, 0.9, 1);

    FGFontCache *fc = globals->get_fontcache();
    puFont *GuiFont = fc->get(globals->get_locale()->getStringValue("font", "typewriter.txf"), 15);
    puSetDefaultFonts(*GuiFont, *GuiFont);
    guiFnt = puGetDefaultLabelFont();

    LayoutWidget::setDefaultFont(GuiFont, 15);
  
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
    fgPresetInit();
}
