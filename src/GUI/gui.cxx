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

#include <string>

#include <simgear/structure/exception.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>

#include <plib/pu.h>

#include <Main/main.hxx>
#include <Main/globals.hxx>
#include <Main/locale.hxx>
#include <Main/fg_props.hxx>
#include <Viewer/WindowSystemAdapter.hxx>
#include <GUI/new_gui.hxx>
#include <GUI/FGFontCache.hxx>

#include "gui.h"
#include "layout.hxx"

#include <osg/GraphicsContext>

using namespace flightgear;

puFont guiFnt = 0;


/* -------------------------------------------------------------------------
init the gui
_____________________________________________________________________*/

namespace
{
class GUIInitOperation : public GraphicsContextOperation
{
public:
    GUIInitOperation() : GraphicsContextOperation(std::string("GUI init"))
    {
    }
    void run(osg::GraphicsContext* gc)
    {
        WindowSystemAdapter* wsa = WindowSystemAdapter::getWSA();
        wsa->puInitialize();
        puSetDefaultStyle         ( PUSTYLE_SMALL_SHADED ); //PUSTYLE_DEFAULT
        puSetDefaultColourScheme  (0.8, 0.8, 0.9, 1);

        FGFontCache *fc = globals->get_fontcache();
        fc->initializeFonts();
        puFont *GuiFont
            = fc->get(globals->get_locale()->getDefaultFont("typewriter.txf"),
                      15);
        puSetDefaultFonts(*GuiFont, *GuiFont);
        guiFnt = puGetDefaultLabelFont();

        LayoutWidget::setDefaultFont(GuiFont, 15);
  
        if (!fgHasNode("/sim/startup/mouse-pointer")) {
            // no preference specified for mouse pointer
        } else if ( !fgGetBool("/sim/startup/mouse-pointer") ) {
            // don't show pointer
        } else {
            // force showing pointer
            puShowCursor();
        }
    }
};

osg::ref_ptr<GUIInitOperation> initOp;
}

void guiStartInit(osg::GraphicsContext* gc)
{
    if (gc) {
        initOp = new GUIInitOperation;
        gc->add(initOp.get());
    }
}

bool guiFinishInit()
{
    if (!initOp.valid())
        return true;
    if (!initOp->isFinished())
        return false;
    initOp = 0;
    return true;
}

