// subsystemFactory.cxx - factory for subsystems
//
// Copyright (C) 2012 James Turner  zakalawe@mac.com
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

#ifdef HAVE_CONFIG_H
    #include "config.h"
#endif

#include "subsystemFactory.hxx"

#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/structure/commands.hxx>
#include <simgear/structure/event_mgr.hxx>

#include <Main/globals.hxx>
#include <Sound/soundmanager.hxx>

// subsystem includes
#include <Aircraft/controls.hxx>
#include <Aircraft/FlightHistory.hxx>
#include <Main/fg_props.hxx>
#include <Main/FGInterpolator.hxx>
#include <Main/fg_io.hxx>
#include <FDM/fdm_shell.hxx>
#include <Environment/environment_mgr.hxx>
#include <Environment/ephemeris.hxx>
#include <Instrumentation/instrument_mgr.hxx>
#include <Instrumentation/HUD/HUD.hxx>
#include <Systems/system_mgr.hxx>
#include <Autopilot/route_mgr.hxx>
#include <Autopilot/autopilotgroup.hxx>
#include <Traffic/TrafficMgr.hxx>
#include <Network/fgcom.hxx>
#include <Network/HTTPClient.hxx>
#include <Cockpit/cockpitDisplayManager.hxx>
#include <GUI/new_gui.hxx>
#include <Main/logger.hxx>
#include <ATC/atc_mgr.hxx>
#include <AIModel/AIManager.hxx>
#include <MultiPlayer/multiplaymgr.hxx>
#if defined(ENABLE_SWIFT)
#  include <Network/Swift/swift_connection.hxx>
#endif
#include <AIModel/submodel.hxx>
#include <Aircraft/controls.hxx>
#include <Input/input.hxx>
#include <Aircraft/replay.hxx>
#include <Sound/voice.hxx>
#include <Canvas/canvas_mgr.hxx>
#include <Canvas/gui_mgr.hxx>
#include <Time/light.hxx>
#include <Viewer/viewmgr.hxx>
#include <Model/acmodel.hxx>
#include <Model/modelmgr.hxx>

using std::vector;

namespace flightgear
{
  
SGSubsystem* createSubsystemByName(const std::string& name)
{
#define MAKE_SUB(cl, n) \
    if (name == n) return new cl;

    MAKE_SUB(FGSoundManager, "sound");
    MAKE_SUB(FGInterpolator, "prop-interpolator")
    MAKE_SUB(FGProperties, "properties");
    MAKE_SUB(FGHTTPClient, "http");
    MAKE_SUB(FDMShell, "flight");
    MAKE_SUB(FGEnvironmentMgr, "environment");
    MAKE_SUB(Ephemeris, "ephemeris");
    MAKE_SUB(FGSystemMgr, "systems");
    MAKE_SUB(FGInstrumentMgr, "instrumentation");
    MAKE_SUB(HUD, "hud");
    MAKE_SUB(flightgear::CockpitDisplayManager, "cockpit-displays");
    MAKE_SUB(FGRouteMgr, "route-manager");
    MAKE_SUB(FGIO, "io");
    MAKE_SUB(FGLogger, "logger");
    MAKE_SUB(NewGUI, "gui");
    MAKE_SUB(CanvasMgr, "Canvas");
    MAKE_SUB(GUIMgr, "CanvasGUI");
    MAKE_SUB(FGATCManager, "ATC");
    MAKE_SUB(FGMultiplayMgr, "mp");
    MAKE_SUB(FGAIManager, "ai-model");
    MAKE_SUB(FGSubmodelMgr, "submodel-mgr");
    MAKE_SUB(FGTrafficManager, "traffic-manager");
    MAKE_SUB(FGControls, "controls");
    MAKE_SUB(FGInput, "input");
    MAKE_SUB(FGReplay, "replay");
    MAKE_SUB(FGFlightHistory, "history");
#ifdef ENABLE_AUDIO_SUPPORT
    MAKE_SUB(FGVoiceMgr, "voice");
#endif
#ifdef ENABLE_IAX
    MAKE_SUB(FGCom, "fgcom");
#endif
#ifdef ENABLE_SWIFT
    MAKE_SUB(SwiftConnection, "swift");
#endif
    MAKE_SUB(FGLight, "tides");
    MAKE_SUB(FGLight, "lighting");
    MAKE_SUB(FGAircraftModel, "aircraft-model");
    MAKE_SUB(FGModelMgr, "model-manager");
    MAKE_SUB(FGViewMgr, "view-manager");
#undef MAKE_SUB

    throw sg_range_exception("unknown subsystem:" + name);
}

SGSubsystemMgr::GroupType mapGroupNameToType(const std::string& s)
{
    if (s == "init")        return SGSubsystemMgr::INIT;
    if (s == "general")     return SGSubsystemMgr::GENERAL;
    if (s == "fdm")         return SGSubsystemMgr::FDM;
    if (s == "post-fdm")    return SGSubsystemMgr::POST_FDM;
    if (s == "display")     return SGSubsystemMgr::DISPLAY;
    if (s == "sound")       return SGSubsystemMgr::SOUND;
    
    SG_LOG(SG_GENERAL, SG_ALERT, "unrecognized subsystem group:" << s);
    return SGSubsystemMgr::GENERAL;
}

static SGSubsystem* getSubsystem(const SGPropertyNode* arg, bool create)
{
  std::string subsystem(arg->getStringValue("subsystem"));
  std::string name = arg->getStringValue("name");

  if (name.empty()) {
    // default name is simply the subsytem's name
    name = subsystem;
  }

  SGSubsystem* sys = globals->get_subsystem_mgr()->get_subsystem(name);
  if (!create)
    return sys;

  if( subsystem.empty() ) {
    SG_LOG( SG_GENERAL,
            SG_ALERT,
            "do_add_subsystem: no subsystem/name supplied" );
    return 0;
  }

  if (sys) {
    SG_LOG( SG_GENERAL,
            SG_ALERT,
            "do_add_subsystem: duplicate subsystem name:" << name );
    return 0;
  }

  std::string groupname = arg->getStringValue("group");
  SGSubsystemMgr::GroupType group = SGSubsystemMgr::GENERAL;
  if (!groupname.empty()) {
    group = mapGroupNameToType(groupname);
  }

  try {
    sys = createSubsystemByName(subsystem);
  } catch (sg_exception& e) {
    SG_LOG( SG_GENERAL,
            SG_ALERT,
            "subsystem creation failed:" << name
                                  << ":" << e.getFormattedMessage() );
    return 0;
  }

  bool doInit = arg->getBoolValue("do-bind-init", false);
  if (doInit) {
    sys->bind();
    sys->init();
  }

  double minTime = arg->getDoubleValue("min-time-sec", 0.0);
  globals->get_subsystem_mgr()
         ->add(name.c_str(), sys, group, minTime);

  return sys;
}

static bool
do_check_subsystem_running(const SGPropertyNode * arg, SGPropertyNode * root)
{
  return getSubsystem(arg, false) != 0;
}

static bool
do_add_subsystem (const SGPropertyNode * arg, SGPropertyNode * root)
{
  return getSubsystem(arg, true) != 0;
}

static bool do_remove_subsystem(const SGPropertyNode * arg, SGPropertyNode * root)
{
  std::string name = arg->getStringValue("subsystem");

  SGSubsystem* instance = globals->get_subsystem_mgr()->get_subsystem(name);
  if (!instance) {
    SG_LOG(SG_GENERAL, SG_ALERT, "do_remove_subsystem: unknown subsytem:" << name);
    return false;
  }

  // is it safe to always call these? let's assume so!
  instance->shutdown();
  instance->unbind();

  // unplug from the manager (this also deletes the instance!)
  globals->get_subsystem_mgr()->remove(name.c_str());

  return true;
}
  
/**
 * Built-in command: reinitialize one or more subsystems.
 *
 * subsystem[*]: the name(s) of the subsystem(s) to reinitialize; if
 * none is specified, reinitialize all of them.
 */
static bool
do_reinit (const SGPropertyNode * arg, SGPropertyNode * root)
{
    bool result = true;

    vector<SGPropertyNode_ptr> subsystems = arg->getChildren("subsystem");
    if (subsystems.empty()) {
        globals->get_subsystem_mgr()->reinit();
    } else {
        for ( unsigned int i = 0; i < subsystems.size(); i++ ) {
            const char * name = subsystems[i]->getStringValue();
            SGSubsystem * subsystem = globals->get_subsystem(name);
            if (subsystem == 0) {
                result = false;
                SG_LOG( SG_GENERAL, SG_ALERT,
                        "Subsystem " << name << " not found" );
            } else {
                subsystem->reinit();
            }
        }
    }

    globals->get_event_mgr()->reinit();

    return result;
}

/**
 * Built-in command: suspend one or more subsystems.
 *
 * subsystem[*] - the name(s) of the subsystem(s) to suspend.
 */
static bool
do_suspend (const SGPropertyNode * arg, SGPropertyNode * root)
{
    bool result = true;

    vector<SGPropertyNode_ptr> subsystems = arg->getChildren("subsystem");
    for ( unsigned int i = 0; i < subsystems.size(); i++ ) {
        const char * name = subsystems[i]->getStringValue();
        SGSubsystem * subsystem = globals->get_subsystem(name);
        if (subsystem == 0) {
            result = false;
            SG_LOG(SG_GENERAL, SG_ALERT, "Subsystem " << name << " not found");
        } else {
            subsystem->suspend();
        }
    }
    return result;
}

/**
 * Built-in command: suspend one or more subsystems.
 *
 * subsystem[*] - the name(s) of the subsystem(s) to suspend.
 */
static bool
do_resume (const SGPropertyNode * arg, SGPropertyNode * root)
{
    bool result = true;

    vector<SGPropertyNode_ptr> subsystems = arg->getChildren("subsystem");
    for ( unsigned int i = 0; i < subsystems.size(); i++ ) {
        const char * name = subsystems[i]->getStringValue();
        SGSubsystem * subsystem = globals->get_subsystem(name);
        if (subsystem == 0) {
            result = false;
            SG_LOG(SG_GENERAL, SG_ALERT, "Subsystem " << name << " not found");
        } else {
            subsystem->resume();
        }
    }
    return result;
}

static struct {
  const char * name;
  SGCommandMgr::command_t command;
} built_ins [] = {
    { "add-subsystem", do_add_subsystem },
    { "remove-subsystem", do_remove_subsystem },
    { "subsystem-running", do_check_subsystem_running },
    { "reinit", do_reinit },
    { "suspend", do_suspend },
    { "resume", do_resume },
    { 0, 0 }			// zero-terminated
};

void registerSubsystemCommands(SGCommandMgr* cmdMgr)
{
    for (int i = 0; built_ins[i].name != 0; i++) {
      cmdMgr->addCommand(built_ins[i].name, built_ins[i].command);
    }
}

} // of namepace flightgear
