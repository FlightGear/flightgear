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
#include <Viewer/CameraGroup.hxx>
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

// Operation for querying OpenGL parameters. This must be done in a
// valid OpenGL context, potentially in another thread.

struct GeneralInitOperation : public GraphicsContextOperation
{
    GeneralInitOperation()
        : GraphicsContextOperation(std::string("General init"))
    {
    }
    void run(osg::GraphicsContext* gc)
    {
        SGPropertyNode* simRendering = fgGetNode("/sim/rendering");

        simRendering->setStringValue("gl-vendor", (char*) glGetString(GL_VENDOR));
        SG_LOG( SG_GENERAL, SG_INFO, glGetString(GL_VENDOR));

        simRendering->setStringValue("gl-renderer", (char*) glGetString(GL_RENDERER));
        SG_LOG( SG_GENERAL, SG_INFO, glGetString(GL_RENDERER));

        simRendering->setStringValue("gl-version", (char*) glGetString(GL_VERSION));
        SG_LOG( SG_GENERAL, SG_INFO, glGetString(GL_VERSION));

        // Old hardware without support for OpenGL 2.0 does not support GLSL and
        // glGetString returns NULL for GL_SHADING_LANGUAGE_VERSION.
        //
        // See http://flightgear.org/forums/viewtopic.php?f=17&t=19670&start=15#p181945
        const char* glsl_version = (const char*) glGetString(GL_SHADING_LANGUAGE_VERSION);
        if( !glsl_version )
          glsl_version = "UNSUPPORTED";
        simRendering->setStringValue("gl-shading-language-version", glsl_version);
        SG_LOG( SG_GENERAL, SG_INFO, glsl_version);

        GLint tmp;
        glGetIntegerv( GL_MAX_TEXTURE_SIZE, &tmp );
        simRendering->setIntValue("max-texture-size", tmp);

        glGetIntegerv( GL_DEPTH_BITS, &tmp );
        simRendering->setIntValue("depth-buffer-bits", tmp);
    }
};

osg::ref_ptr<GUIInitOperation> initOp;

}

/** Initializes GUI.
 * Returns true when done, false when still busy (call again). */
bool guiInit()
{
    static osg::ref_ptr<GeneralInitOperation> genOp;

    if (!genOp.valid())
    {
        // Pick some window on which to do queries.
        // XXX Perhaps all this graphics initialization code should be
        // moved to renderer.cxx?
        genOp = new GeneralInitOperation;
        osg::Camera* guiCamera = getGUICamera(CameraGroup::getDefault());
        WindowSystemAdapter* wsa = WindowSystemAdapter::getWSA();
        osg::GraphicsContext* gc = 0;
        if (guiCamera)
            gc = guiCamera->getGraphicsContext();
        if (gc) {
            gc->add(genOp.get());
            initOp = new GUIInitOperation;
            gc->add(initOp.get());
        } else {
            wsa->windows[0]->gc->add(genOp.get());
        }
        return false; // not ready yet
    }
    else
    {
        if (!genOp->isFinished())
            return false;
        if (!initOp.valid())
            return true;
        if (!initOp->isFinished())
            return false;
        genOp = 0;
        initOp = 0;
        // we're done
        return true;
    }
}
