/**************************************************************************
 * gui_funcs.cxx
 *
 * Based on gui.cxx and renamed on 2002/08/13 by Erik Hofman.
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

#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif

#include <simgear/compiler.h>

#include <fstream>
#include <string>
#include <cstring>
#include <sstream>

#include <stdlib.h>

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/screen/screen-dump.hxx>
#include <simgear/structure/commands.hxx>
#include <simgear/structure/event_mgr.hxx>
#include <simgear/props/props_io.hxx>

#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <Main/fg_os.hxx>
#include <Viewer/renderer.hxx>
#include <Viewer/viewmgr.hxx>
#include <Viewer/WindowSystemAdapter.hxx>
#include <Viewer/CameraGroup.hxx>
#include <GUI/new_gui.hxx>


#ifdef _WIN32
#  include <shellapi.h>
#endif

#if defined(SG_MAC)
# include <GUI/CocoaHelpers.h> // for cocoaOpenUrl
#endif

#include "gui.h"

using std::string;

const __fg_gui_fn_t __fg_gui_fn[] = {
        {"dumpSnapShot", fgDumpSnapShotWrapper},
        // Help
        {"helpCb", helpCb},

        // Structure termination
        {"", NULL}
};


/* ================ General Purpose Functions ================ */

// General Purpose Message Box. Makes sure no more than 5 different
// messages are displayed at the same time, and none of them are
// duplicates. (5 is a *lot*, but this will hardly ever be reached
// and we don't want to miss any, either.)
void mkDialog (const char *txt)
{
    auto gui = globals->get_subsystem<NewGUI>();
    if (!gui)
        return;
    SGPropertyNode *master = gui->getDialogProperties("message");
    if (!master)
        return;

    const int maxdialogs = 5;
    string name;
    SGPropertyNode *msg = fgGetNode("/sim/gui/dialogs", true);
    int i;
    for (i = 0; i < maxdialogs; i++) {
        std::ostringstream s;
        s << "message-" << i;
        name = s.str();

        if (!msg->getNode(name.c_str(), false))
            break;

        if (!strcmp(txt, msg->getNode(name.c_str())->getStringValue("message").c_str())) {
            SG_LOG(SG_GENERAL, SG_WARN, "mkDialog(): duplicate of message " << txt);
            return;
        }
    }
    if (i == maxdialogs)
        return;
    msg = msg->getNode(name.c_str(), true);
    msg->setStringValue("message", txt);
    msg = msg->getNode("dialog", true);
    copyProperties(master, msg);
    msg->setStringValue("name", name.c_str());
    gui->newDialog(msg);
    gui->showDialog(name.c_str());
}

// Message Box to report an error.
void guiErrorMessage (const char *txt)
{
    SG_LOG(SG_GENERAL, SG_ALERT, txt);
    mkDialog(txt);
}

// Message Box to report a throwable (usually an exception).
void guiErrorMessage (const char *txt, const sg_throwable &throwable)
{
    string msg = txt;
    msg += '\n';
    msg += throwable.getFormattedMessage();
    if (std::strlen(throwable.getOrigin()) != 0) {
        msg += "\n (reported by ";
        msg += throwable.getOrigin();
        msg += ')';
    }
    SG_LOG(SG_GENERAL, SG_ALERT, msg);
    mkDialog(msg.c_str());
}



/* -----------------------------------------------------------------------
the Gui callback functions 
____________________________________________________________________*/

void helpCb()
{
    openBrowser( "Docs/index.html" );
}

bool openBrowser(const std::string& aAddress)
{
    bool ok = true;
    string address(aAddress);
    
    // do not resolve addresses with given protocol, i.e. "http://...", "ftp://..."
    if (address.find("://")==string::npos)
    {
        // resolve local file path
        SGPath path(address);
        path = globals->resolve_maybe_aircraft_path(address);
        if (!path.isNull()) {
            address = "file://" + path.local8BitStr();
        } else {
            mkDialog ("Sorry, file not found!");
            SG_LOG(SG_GENERAL, SG_ALERT, "openBrowser: Cannot find requested file '"  
                    << address << "'.");
            return false;
        }
    }

#ifdef SG_MAC
  cocoaOpenUrl(address);
#elif defined _WIN32

    // Look for favorite browser
    char win32_name[1024];
# ifdef __CYGWIN__
    cygwin32_conv_to_full_win32_path(address.c_str(),win32_name);
# else
    strncpy(win32_name,address.c_str(), 1024);
# endif
    ShellExecuteA(NULL, "open", win32_name, NULL, NULL,
                  SW_SHOWNORMAL);
#else
    // Linux, BSD, SGI etc
    string command = globals->get_browser();
    string::size_type pos;
    if ((pos = command.find("%u", 0)) != string::npos)
        command.replace(pos, 2, address);
    else
        command += " \"" + address +"\"";

    command += " &";
    ok = (system( command.c_str() ) == 0);
#endif

    if( fgGetBool("/sim/gui/show-browser-open-hint", true) )
        mkDialog("The file is shown in your web browser window.");

    return ok;
}

void fgDumpSnapShotWrapper () {
    fgDumpSnapShot();
}

namespace
{
    using namespace flightgear;

    SGPath nextScreenshotPath(const SGPath& screenshotDir)
    {
        char filename[60];
        int count = 0;
        while (count < 100) { // 100 per second should be more than enough.
            char time_str[20];
            time_t calendar_time = time(NULL);
            struct tm *tmUTC;
            tmUTC = gmtime(&calendar_time);
            strftime(time_str, sizeof(time_str), "%Y%m%d%H%M%S", tmUTC);

            if (count)
                snprintf(filename, 32, "fgfs-%s-%d.png", time_str, count++);
            else
                snprintf(filename, 32, "fgfs-%s.png", time_str);

            SGPath p = screenshotDir / filename;
            if (!p.exists()) {
                return p;
            }
        }
        
        return SGPath();
    }
    
    class GUISnapShotOperation :
        public GraphicsContextOperation
    {
    public:

        // start new snap shot
        static bool start()
        {
            // allow only one snapshot at a time
            if (_snapShotOp.valid())
                return false;
            _snapShotOp = new GUISnapShotOperation();
            /* register with graphics context so actual snap shot is done
             * in the graphics context (thread) */
            osg::Camera* guiCamera = getGUICamera(CameraGroup::getDefault());
            WindowSystemAdapter* wsa = WindowSystemAdapter::getWSA();
            osg::GraphicsContext* gc = 0;
            if (guiCamera)
                gc = guiCamera->getGraphicsContext();
            if (gc) {
                gc->add(_snapShotOp.get());
            } else {
                wsa->windows[0]->gc->add(_snapShotOp.get());
            }
            return true;
        }

        static void cancel()
        {
            _snapShotOp = nullptr;
        }

    private:
        // constructor to be executed in main loop's thread
        GUISnapShotOperation() :
            flightgear::GraphicsContextOperation(std::string("GUI snap shot")),
            _master_freeze(fgGetNode("/sim/freeze/master", true)),
            _freeze(_master_freeze->getBoolValue()),
            _result(false),
            _mouse(fgGetMouseCursor())
        {
            if (!_freeze)
                _master_freeze->setBoolValue(true);

            fgSetMouseCursor(FGMouseCursor::CURSOR_NONE);

            SGPath dir = SGPath::fromUtf8(fgGetString("/sim/paths/screenshot-dir"));
            if (dir.isNull())
                dir = SGPath::desktop();

            if (!dir.exists() && dir.create_dir( 0755 )) {
                SG_LOG(SG_GENERAL, SG_ALERT, "Cannot create screenshot directory '"
                        << dir << "'. Trying home directory.");
                dir = globals->get_fg_home();
            }

            _path = nextScreenshotPath(dir);
            _xsize = fgGetInt("/sim/startup/xsize");
            _ysize = fgGetInt("/sim/startup/ysize");

            FGRenderer *renderer = globals->get_renderer();
            renderer->resize(_xsize, _ysize);
            globals->get_event_mgr()->addTask("SnapShotTimer",
                    [this](){ this->timerExpired(); },
                    0.1, false);
        }

        // to be executed in graphics context (maybe separate thread)
        void run(osg::GraphicsContext* gc)
        {
            std::string ps = _path.local8BitStr();
            _result = sg_glDumpWindow(ps.c_str(),
                                     _xsize,
                                     _ysize);
        }

        // timer method, to be executed in main loop's thread
        virtual void timerExpired()
        {
            if (isFinished())
            {
                globals->get_event_mgr()->removeTask("SnapShotTimer");

                fgSetString("/sim/paths/screenshot-last", _path.utf8Str());
                fgSetBool("/sim/signals/screenshot", _result);

                fgSetMouseCursor(_mouse);

                if ( !_freeze )
                    _master_freeze->setBoolValue(false);

                _snapShotOp = 0;
            }
        }
    
        static osg::ref_ptr<GUISnapShotOperation> _snapShotOp;
        SGPropertyNode_ptr _master_freeze;
        bool _freeze;
        bool _result;
        FGMouseCursor::Cursor _mouse;
        int _xsize, _ysize;
        SGPath _path;
    };

} // of anonymous namespace

osg::ref_ptr<GUISnapShotOperation> GUISnapShotOperation::_snapShotOp;

// do a screen snap shot
bool fgDumpSnapShot ()
{
    // start snap shot operation, while needs to be executed in
    // graphics context
    return GUISnapShotOperation::start();
}

void fgCancelSnapShot()
{
    GUISnapShotOperation::cancel();
}

// do an entire scenegraph dump
void fgDumpSceneGraph()
{
    char *filename = new char [24];
    string message;
    static unsigned short count = 1;

    SGPropertyNode *master_freeze = fgGetNode("/sim/freeze/master");

    bool freeze = master_freeze->getBoolValue();
    if ( !freeze ) {
        master_freeze->setBoolValue(true);
    }

    while (count < 1000) {
        FILE *fp;
        snprintf(filename, 24, "fgfs-graph-%03d.osg", count++);
        if ( (fp = fopen(filename, "r")) == NULL )
            break;
        fclose(fp);
    }

    if ( fgDumpSceneGraphToFile(filename)) {
	message = "Entire scene graph saved to \"";
	message += filename;
	message += "\".";
    } else {
        message = "Failed to save to \"";
	message += filename;
	message += "\".";
    }

    mkDialog (message.c_str());

    delete [] filename;

    if ( !freeze ) {
        master_freeze->setBoolValue(false);
    }
}

    
// do an terrain branch dump
void fgDumpTerrainBranch()
{
    char *filename = new char [24];
    string message;
    static unsigned short count = 1;

    SGPropertyNode *master_freeze = fgGetNode("/sim/freeze/master");

    bool freeze = master_freeze->getBoolValue();
    if ( !freeze ) {
        master_freeze->setBoolValue(true);
    }

    while (count < 1000) {
        FILE *fp;
        snprintf(filename, 24, "fgfs-graph-%03d.osg", count++);
        if ( (fp = fopen(filename, "r")) == NULL )
            break;
        fclose(fp);
    }

    if ( fgDumpTerrainBranchToFile(filename)) {
	message = "Terrain graph saved to \"";
	message += filename;
	message += "\".";
    } else {
        message = "Failed to save to \"";
	message += filename;
	message += "\".";
    }

    mkDialog (message.c_str());

    delete [] filename;

    if ( !freeze ) {
        master_freeze->setBoolValue(false);
    }
}

void fgPrintVisibleSceneInfoCommand()
{
    SGPropertyNode *master_freeze = fgGetNode("/sim/freeze/master");

    bool freeze = master_freeze->getBoolValue();
    if ( !freeze ) {
        master_freeze->setBoolValue(true);
    }

    fgPrintVisibleSceneInfo(globals->get_renderer());

    if ( !freeze ) {
        master_freeze->setBoolValue(false);
    }
}

void syncPausePopupState()
{
    bool paused = fgGetBool("/sim/freeze/master", true) || fgGetBool("/sim/freeze/clock", true);
    SGPropertyNode_ptr args(new SGPropertyNode);
    args->setStringValue("id", "sim-pause");
    if (paused && fgGetBool("/sim/view-name-popup")) {
        args->setStringValue("label", "Simulation is paused");
        globals->get_commands()->execute("show-message", args, nullptr);
    } else {
        globals->get_commands()->execute("clear-message", args, nullptr);
    }
}
