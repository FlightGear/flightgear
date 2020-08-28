// MouseCursor.cxx - abstract inteface for  mouse cursor control

// Copyright (C) 2013 James Turner <zakalawe@mac.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "MouseCursor.hxx"

#include <cstring>

#include <osgViewer/GraphicsWindow>
#include <osgViewer/Viewer>

#include <simgear/debug/logstream.hxx>
#include <simgear/simgear_config.h>
#include <simgear/structure/commands.hxx>

#ifdef SG_MAC
#include "CocoaMouseCursor.hxx"
#endif

#ifdef SG_WINDOWS
#include "WindowsMouseCursor.hxx"
#endif

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Viewer/renderer.hxx>
#include <Main/fg_os.hxx> // for fgWarpMouse

namespace
{

/**
 * @brief when no native cursor implementation is available, use the osgViewer support. This
 * has several limitations but is better than nothing
 */
class StockOSGCursor : public FGMouseCursor
{
public:
    StockOSGCursor() :
        mCursorObscured(false),
        mCursorVisible(true),
        mCursor(osgViewer::GraphicsWindow::InheritCursor)
    {
        mActualCursor = mCursor;

        globals->get_renderer()->getViewerBase()->getWindows(mWindows);
    }

    virtual void setCursor(Cursor aCursor)
    {
        mCursor = translateCursor(aCursor);
        updateCursor();
    }

    virtual void setCursorVisible(bool aVis)
    {
        if (mCursorObscured == aVis) {
            return;
        }

        mCursorVisible = aVis;
        updateCursor();
    }

    virtual void hideCursorUntilMouseMove()
    {
        if (mCursorObscured) {
            return;
        }

        mCursorObscured = true;
        updateCursor();
    }

    virtual void mouseMoved()
    {
        if (mCursorObscured) {
            mCursorObscured = false;
            updateCursor();
        }
    }
private:
    osgViewer::GraphicsWindow::MouseCursor translateCursor(Cursor aCursor)
    {
        switch (aCursor) {
		case CURSOR_ARROW: return osgViewer::GraphicsWindow::RightArrowCursor;
        case CURSOR_HAND: return osgViewer::GraphicsWindow::HandCursor;
        case CURSOR_CLOSED_HAND: return osgViewer::GraphicsWindow::HandCursor;
        case CURSOR_CROSSHAIR: return osgViewer::GraphicsWindow::CrosshairCursor;
        case CURSOR_IBEAM: return osgViewer::GraphicsWindow::TextCursor;
        case CURSOR_LEFT_RIGHT: return osgViewer::GraphicsWindow::LeftRightCursor;
        case CURSOR_UP_DOWN: return osgViewer::GraphicsWindow::UpDownCursor;
        default:
			return osgViewer::GraphicsWindow::RightArrowCursor;
        }
    }

    void updateCursor()
    {
        osgViewer::GraphicsWindow::MouseCursor cur = osgViewer::GraphicsWindow::InheritCursor;
        if (mCursorObscured || !mCursorVisible) {
            cur = osgViewer::GraphicsWindow::NoCursor;
        } else {
            cur = mCursor;
        }

        if (cur == mActualCursor) {
            return;
        }

        for (auto gw : mWindows) {
            gw->setCursor(cur);
        }

        mActualCursor = cur;
    }

    bool mCursorObscured;
    bool mCursorVisible;
    osgViewer::GraphicsWindow::MouseCursor mCursor, mActualCursor;
    std::vector<osgViewer::GraphicsWindow*> mWindows;
};

} // of anonymous namespace

static FGMouseCursor* static_instance = NULL;

FGMouseCursor::FGMouseCursor() :
    mAutoHideTimeMsec(10000)
{
}

FGMouseCursor* FGMouseCursor::instance()
{
    if (static_instance == NULL) {
    #ifdef SG_MAC
        if (true) {
            static_instance = new CocoaMouseCursor;
        }
    #endif
	#ifdef SG_WINDOWS
	// set osgViewer cursor inherit, otherwise it will interefere
		std::vector<osgViewer::GraphicsWindow*> gws;
		globals->get_renderer()->getViewerBase()->getWindows(gws);
		for (auto gw : gws) {
            gw->setCursor(osgViewer::GraphicsWindow::InheritCursor);
        }

        // Windows native curosr disabled while interaction with OSG
        // is resolved - right now NCHITs (non-client-area hits)
        // overwire the InheritCursor value above, and hence our cursor
        // get stuck.
	// and create our real implementation
	//	static_instance = new WindowsMouseCursor;
	#endif

        // X11

        if (static_instance == NULL) {
            static_instance = new StockOSGCursor;
        }

        // initialise mouse-hide delay from global properties

        globals->get_commands()->addCommand("set-cursor", static_instance, &FGMouseCursor::setCursorCommand);
    }

    return static_instance;
}

void FGMouseCursor::setAutoHideTimeMsec(unsigned int aMsec)
{
    mAutoHideTimeMsec = aMsec;
}


bool FGMouseCursor::setCursorCommand(const SGPropertyNode* arg, SGPropertyNode*)
{
    // JMT 2013 - I would prefer this was a seperate 'warp' command, but
    // historically set-cursor has done both.
    if (arg->hasValue("x") || arg->hasValue("y")) {
        SGPropertyNode *mx = fgGetNode("/devices/status/mice/mouse/x", true);
        SGPropertyNode *my = fgGetNode("/devices/status/mice/mouse/y", true);
        int x = arg->getIntValue("x", mx->getIntValue());
        int y = arg->getIntValue("y", my->getIntValue());
        fgWarpMouse(x, y);
        mx->setIntValue(x);
        my->setIntValue(y);
    }


    Cursor c = cursorFromString(arg->getStringValue("cursor"));
    setCursor(c);
    return true;
}

typedef struct {
    const char * name;
    FGMouseCursor::Cursor cursor;
} MouseCursorMap;

const MouseCursorMap mouse_cursor_map[] = {
    { "inherit", FGMouseCursor::CURSOR_ARROW },
    { "crosshair", FGMouseCursor::CURSOR_CROSSHAIR },
    { "left-right", FGMouseCursor::CURSOR_LEFT_RIGHT },
    { "hand", FGMouseCursor::CURSOR_HAND },
    { "closed-hand", FGMouseCursor::CURSOR_CLOSED_HAND },
    { "text", FGMouseCursor::CURSOR_IBEAM },

// aliases
    { "drag-horizontal", FGMouseCursor::CURSOR_LEFT_RIGHT },
    { "drag-vertical", FGMouseCursor::CURSOR_UP_DOWN },
    { 0, FGMouseCursor::CURSOR_ARROW }
};

FGMouseCursor::Cursor FGMouseCursor::cursorFromString(const char* cursor_name)
{
    for (unsigned int k = 0; mouse_cursor_map[k].name != 0; k++) {
        if (!strcmp(mouse_cursor_map[k].name, cursor_name)) {
            return mouse_cursor_map[k].cursor;
        }
    }

    SG_LOG(SG_GENERAL, SG_WARN, "unknown cursor:" << cursor_name);
    return CURSOR_ARROW;
}
