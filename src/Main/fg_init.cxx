// fg_init.cxx -- Flight Gear top level initialization routines
//
// Written by Curtis Olson, started August 1997.
//
// Copyright (C) 1997  Curtis L. Olson  - http://www.flightgear.org/~curt
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
// $Id$


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>             // strcmp()

#ifdef _WIN32
#  include <io.h>               // isatty()
#  define isatty _isatty
#endif

#include <simgear/compiler.h>

#include <string>
#include <boost/algorithm/string/compare.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/structure/event_mgr.hxx>
#include <simgear/structure/SGPerfMon.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/sg_dir.hxx>
#include <simgear/misc/sgstream.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/props/props_io.hxx>

#include <simgear/misc/interpolator.hxx>
#include <simgear/scene/material/matlib.hxx>
#include <simgear/scene/model/particles.hxx>

#include <Aircraft/controls.hxx>
#include <Aircraft/replay.hxx>
#include <Airports/runways.hxx>
#include <Airports/simple.hxx>
#include <Airports/dynamics.hxx>

#include <AIModel/AIManager.hxx>

#include <ATCDCL/ATISmgr.hxx>
#include <ATC/atc_mgr.hxx>

#include <Autopilot/route_mgr.hxx>
#include <Autopilot/autopilotgroup.hxx>

#include <Cockpit/panel.hxx>
#include <Cockpit/panel_io.hxx>

#include <Canvas/canvas_mgr.hxx>
#include <Canvas/gui_mgr.hxx>
#include <GUI/new_gui.hxx>
#include <Input/input.hxx>
#include <Instrumentation/instrument_mgr.hxx>
#include <Model/acmodel.hxx>
#include <Model/modelmgr.hxx>
#include <AIModel/submodel.hxx>
#include <AIModel/AIManager.hxx>
#include <Navaids/navdb.hxx>
#include <Navaids/navlist.hxx>
#include <Scenery/scenery.hxx>
#include <Scenery/tilemgr.hxx>
#include <Scripting/NasalSys.hxx>
#include <Sound/voice.hxx>
#include <Sound/soundmanager.hxx>
#include <Systems/system_mgr.hxx>
#include <Time/light.hxx>
#include <Traffic/TrafficMgr.hxx>
#include <MultiPlayer/multiplaymgr.hxx>
#include <FDM/fdm_shell.hxx>
#include <Environment/ephemeris.hxx>
#include <Environment/environment_mgr.hxx>
#include <Viewer/renderer.hxx>
#include <Viewer/viewmgr.hxx>
#include <Navaids/NavDataCache.hxx>
#include <Instrumentation/HUD/HUD.hxx>
#include <Cockpit/cockpitDisplayManager.hxx>
#include <Network/HTTPClient.hxx>

#include "fg_init.hxx"
#include "fg_io.hxx"
#include "fg_commands.hxx"
#include "fg_props.hxx"
#include "options.hxx"
#include "globals.hxx"
#include "logger.hxx"
#include "main.hxx"
#include "positioninit.hxx"

using std::string;
using std::endl;
using std::cerr;
using std::cout;
using namespace boost::algorithm;


// Return the current base package version
string fgBasePackageVersion() {
    SGPath base_path( globals->get_fg_root() );
    base_path.append("version");

    sg_gzifstream in( base_path.str() );
    if ( !in.is_open() ) {
        SGPath old_path( globals->get_fg_root() );
        old_path.append( "Thanks" );
        sg_gzifstream old( old_path.str() );
        if ( !old.is_open() ) {
            return "[none]";
        } else {
            return "[old version]";
        }
    }

    string version;
    in >> version;

    return version;
}


template <class T>
bool fgFindAircraftInDir(const SGPath& dirPath, T* obj, bool (T::*pred)(const SGPath& p))
{
  if (!dirPath.exists()) {
    SG_LOG(SG_GENERAL, SG_WARN, "fgFindAircraftInDir: no such path:" << dirPath.str());
    return false;
  }
    
  bool recurse = true;
  simgear::Dir dir(dirPath);
  simgear::PathList setFiles(dir.children(simgear::Dir::TYPE_FILE, "-set.xml"));
  simgear::PathList::iterator p;
  for (p = setFiles.begin(); p != setFiles.end(); ++p) {
    // check file name ends with -set.xml
    
    // if we found a -set.xml at this level, don't recurse any deeper
    recurse = false;
    
    bool done = (obj->*pred)(*p);
    if (done) {
      return true;
    }
  } // of -set.xml iteration
  
  if (!recurse) {
    return false;
  }
  
  simgear::PathList subdirs(dir.children(simgear::Dir::TYPE_DIR | simgear::Dir::NO_DOT_OR_DOTDOT));
  for (p = subdirs.begin(); p != subdirs.end(); ++p) {
    if (p->file() == "CVS") {
      continue;
    }
    
    if (fgFindAircraftInDir(*p, obj, pred)) {
      return true;
    }
  } // of subdirs iteration
  
  return false;
}

template <class T>
void fgFindAircraft(T* obj, bool (T::*pred)(const SGPath& p))
{
  const string_list& paths(globals->get_aircraft_paths());
  string_list::const_iterator it = paths.begin();
  for (; it != paths.end(); ++it) {
    bool done = fgFindAircraftInDir(SGPath(*it), obj, pred);
    if (done) {
      return;
    }
  } // of aircraft paths iteration
  
  // if we reach this point, search the default location (always last)
  SGPath rootAircraft(globals->get_fg_root());
  rootAircraft.append("Aircraft");
  fgFindAircraftInDir(rootAircraft, obj, pred);
}

class FindAndCacheAircraft
{
public:
  FindAndCacheAircraft(SGPropertyNode* autoSave)
  {
    _cache = autoSave->getNode("sim/startup/path-cache", true);
  }
  
  bool loadAircraft()
  {
    std::string aircraft = fgGetString( "/sim/aircraft", "");
    if (aircraft.empty()) {
      SG_LOG(SG_GENERAL, SG_ALERT, "no aircraft specified");
      return false;
    }
    
    _searchAircraft = aircraft + "-set.xml";
    std::string aircraftDir = fgGetString("/sim/aircraft-dir", "");
    if (!aircraftDir.empty()) {
      // aircraft-dir was set, skip any searching at all, if it's valid
      simgear::Dir acPath(aircraftDir);
      SGPath setFile = acPath.file(_searchAircraft);
      if (setFile.exists()) {
        SG_LOG(SG_GENERAL, SG_INFO, "found aircraft in dir: " << aircraftDir );
        
        try {
          readProperties(setFile.str(), globals->get_props());
        } catch ( const sg_exception &e ) {
          SG_LOG(SG_INPUT, SG_ALERT, "Error reading aircraft: " << e.getFormattedMessage());
          return false;
        }
        
        return true;
      } else {
        SG_LOG(SG_GENERAL, SG_ALERT, "aircraft '" << _searchAircraft << 
               "' not found in specified dir:" << aircraftDir);
        return false;
      }
    }
    
    if (!checkCache()) {
      // prepare cache for re-scan
      SGPropertyNode *n = _cache->getNode("fg-root", true);
      n->setStringValue(globals->get_fg_root().c_str());
      n->setAttribute(SGPropertyNode::USERARCHIVE, true);
      n = _cache->getNode("fg-aircraft", true);
      n->setStringValue(getAircraftPaths().c_str());
      n->setAttribute(SGPropertyNode::USERARCHIVE, true);
      _cache->removeChildren("aircraft");
  
      fgFindAircraft(this, &FindAndCacheAircraft::checkAircraft);
    }
    
    if (_foundPath.str().empty()) {
      SG_LOG(SG_GENERAL, SG_ALERT, "Cannot find specified aircraft: " << aircraft );
      return false;
    }
    
    SG_LOG(SG_GENERAL, SG_INFO, "Loading aircraft -set file from:" << _foundPath.str());
    fgSetString( "/sim/aircraft-dir", _foundPath.dir().c_str());
    if (!_foundPath.exists()) {
      SG_LOG(SG_GENERAL, SG_ALERT, "Unable to find -set file:" << _foundPath.str());
      return false;
    }
    
    try {
      readProperties(_foundPath.str(), globals->get_props());
    } catch ( const sg_exception &e ) {
      SG_LOG(SG_INPUT, SG_ALERT, "Error reading aircraft: " << e.getFormattedMessage());
      return false;
    }
    
    return true;
  }
  
private:
  SGPath getAircraftPaths() {
    string_list pathList = globals->get_aircraft_paths();
    SGPath aircraftPaths;
    string_list::const_iterator it = pathList.begin();
    if (it != pathList.end()) {
        aircraftPaths.set(*it);
        it++;
    }
    for (; it != pathList.end(); ++it) {
        aircraftPaths.add(*it);
    }
    return aircraftPaths;
  }
  
  bool checkCache()
  {
    if (globals->get_fg_root() != _cache->getStringValue("fg-root", "")) {
      return false; // cache mismatch
    }

    if (getAircraftPaths().str() != _cache->getStringValue("fg-aircraft", "")) {
      return false; // cache mismatch
    }
    
    vector<SGPropertyNode_ptr> cache = _cache->getChildren("aircraft");
    for (unsigned int i = 0; i < cache.size(); i++) {
      const char *name = cache[i]->getStringValue("file", "");
      if (!boost::equals(_searchAircraft, name, is_iequal())) {
        continue;
      }
      
      SGPath xml(cache[i]->getStringValue("path", ""));
      xml.append(name);
      if (xml.exists()) {
        _foundPath = xml;
        return true;
      } 
      
      return false;
    } // of aircraft in cache iteration
    
    return false;
  }
  
  bool checkAircraft(const SGPath& p)
  {
    // create cache node
    int i = 0;
    while (1) {
        if (!_cache->getChild("aircraft", i++, false))
            break;
    }
    
    SGPropertyNode *n, *entry = _cache->getChild("aircraft", --i, true);

    std::string fileName(p.file());
    n = entry->getNode("file", true);
    n->setStringValue(fileName);
    n->setAttribute(SGPropertyNode::USERARCHIVE, true);

    n = entry->getNode("path", true);
    n->setStringValue(p.dir());
    n->setAttribute(SGPropertyNode::USERARCHIVE, true);

    if ( boost::equals(fileName, _searchAircraft.c_str(), is_iequal()) ) {
        _foundPath = p;
        return true;
    }

    return false;
  }
  
  std::string _searchAircraft;
  SGPath _foundPath;
  SGPropertyNode* _cache;
};

#ifdef _WIN32
static SGPath platformDefaultDataPath()
{
  char *envp = ::getenv( "APPDATA" );
  SGPath config( envp );
  config.append( "flightgear.org" );
  return config;
}
#elif __APPLE__

#include <CoreServices/CoreServices.h>

static SGPath platformDefaultDataPath()
{
  FSRef ref;
  OSErr err = FSFindFolder(kUserDomain, kApplicationSupportFolderType, false, &ref);
  if (err) {
    return SGPath();
  }
  
  unsigned char path[1024];
  if (FSRefMakePath(&ref, path, 1024) != noErr) {
    return SGPath();
  }
  
  SGPath appData;
  appData.set((const char*) path);
  appData.append("FlightGear");
  return appData;
}
#else
static SGPath platformDefaultDataPath()
{
  SGPath config( homedir );
  config.append( ".fgfs" );
  return config;
}
#endif

// Read in configuration (file and command line)
bool fgInitConfig ( int argc, char **argv )
{
    SGPath dataPath = platformDefaultDataPath();
    
    const char *fg_home = getenv("FG_HOME");
    if (fg_home)
      dataPath = fg_home;
      
    globals->set_fg_home(dataPath.c_str());
    
    simgear::Dir exportDir(simgear::Dir(dataPath).file("Export"));
    if (!exportDir.exists()) {
      exportDir.create(0777);
    }
    
    // Set /sim/fg-home and don't allow malign code to override it until
    // Nasal security is set up.  Use FG_HOME if necessary.
    SGPropertyNode *home = fgGetNode("/sim", true);
    home->removeChild("fg-home", 0, false);
    home = home->getChild("fg-home", 0, true);
    home->setStringValue(dataPath.c_str());
    home->setAttribute(SGPropertyNode::WRITE, false);
  
    flightgear::Options* options = flightgear::Options::sharedInstance();
    options->init(argc, argv, dataPath);
    bool loadDefaults = flightgear::Options::sharedInstance()->shouldLoadDefaultConfig();
    if (loadDefaults) {
      // Read global preferences from $FG_ROOT/preferences.xml
      SG_LOG(SG_INPUT, SG_INFO, "Reading global preferences");
      fgLoadProps("preferences.xml", globals->get_props());
      SG_LOG(SG_INPUT, SG_INFO, "Finished Reading global preferences");

      // do not load user settings when reset to default is requested
      if (flightgear::Options::sharedInstance()->isOptionSet("restore-defaults"))
      {
          SG_LOG(SG_ALL, SG_ALERT, "Ignoring user settings. Restoring defaults.");
      }
      else
      {
          globals->loadUserSettings(dataPath);
      }
    } else {
      SG_LOG(SG_GENERAL, SG_INFO, "not reading default configuration files");
    }// of no-default-config selected
  
    // Scan user config files and command line for a specified aircraft.
    options->initAircraft();

    FindAndCacheAircraft f(globals->get_props());
    if (!f.loadAircraft()) {
      return false;
    }

    // parse options after loading aircraft to ensure any user
    // overrides of defaults are honored.
    options->processOptions();
      
    return true;
}



/**
 * Initialize vor/ndb/ils/fix list management and query systems (as
 * well as simple airport db list)
 * This is called multiple times in the case of a cache rebuild,
 * to allow length caching to take place in the background, without
 * blocking the main/UI thread.
 */
bool
fgInitNav ()
{
  flightgear::NavDataCache* cache = flightgear::NavDataCache::instance();
  static bool doingRebuild = false;
  if (doingRebuild || cache->isRebuildRequired()) {
    doingRebuild = true;
    bool finished = cache->rebuild();
    if (!finished) {
      // sleep to give the rebuild thread more time
      SGTimeStamp::sleepForMSec(50);
      return false;
    }
  }
  
  FGTACANList *channellist = new FGTACANList;
  globals->set_channellist( channellist );
  
  SGPath path(globals->get_fg_root());
  path.append( "Navaids/TACAN_freq.dat" );
  flightgear::loadTacan(path, channellist);
  
  return true;
}

// General house keeping initializations
bool fgInitGeneral() {
    string root;

    SG_LOG( SG_GENERAL, SG_INFO, "General Initialization" );
    SG_LOG( SG_GENERAL, SG_INFO, "======= ==============" );

    root = globals->get_fg_root();
    if ( ! root.length() ) {
        // No root path set? Then bail ...
        SG_LOG( SG_GENERAL, SG_ALERT,
                "Cannot continue without a path to the base package "
                << "being defined." );
        exit(-1);
    }
    SG_LOG( SG_GENERAL, SG_INFO, "FG_ROOT = " << '"' << root << '"' << endl );

    // Note: browser command is hard-coded for Mac/Windows, so this only affects other platforms
    globals->set_browser(fgGetString("/sim/startup/browser-app", WEB_BROWSER));
    fgSetString("/sim/startup/browser-app", globals->get_browser());

    simgear::Dir cwd(simgear::Dir::current());
    SGPropertyNode *curr = fgGetNode("/sim", true);
    curr->removeChild("fg-current", 0, false);
    curr = curr->getChild("fg-current", 0, true);
    curr->setStringValue(cwd.path().str());
    curr->setAttribute(SGPropertyNode::WRITE, false);

    fgSetBool("/sim/startup/stdout-to-terminal", isatty(1) != 0 );
    fgSetBool("/sim/startup/stderr-to-terminal", isatty(2) != 0 );
    return true;
}

// This is the top level init routine which calls all the other
// initialization routines.  If you are adding a subsystem to flight
// gear, its initialization call should located in this routine.
// Returns non-zero if a problem encountered.
void fgCreateSubsystems() {

    SG_LOG( SG_GENERAL, SG_INFO, "Creating Subsystems");
    SG_LOG( SG_GENERAL, SG_INFO, "========== ==========");

    ////////////////////////////////////////////////////////////////////
    // Initialize the sound subsystem.
    ////////////////////////////////////////////////////////////////////
    // Sound manager uses an own subsystem group "SOUND" which is the last
    // to be updated in every loop.
    // Sound manager is updated last so it can use the CPU while the GPU
    // is processing the scenery (doubled the frame-rate for me) -EMH-
    globals->add_subsystem("sound", new FGSoundManager, SGSubsystemMgr::SOUND);

    ////////////////////////////////////////////////////////////////////
    // Initialize the event manager subsystem.
    ////////////////////////////////////////////////////////////////////

    globals->get_event_mgr()->init();
    globals->get_event_mgr()->setRealtimeProperty(fgGetNode("/sim/time/delta-realtime-sec", true));

    ////////////////////////////////////////////////////////////////////
    // Initialize the property interpolator subsystem. Put into the INIT
    // group because the "nasal" subsystem may need it at GENERAL take-down.
    ////////////////////////////////////////////////////////////////////
    globals->add_subsystem("interpolator", new SGInterpolator, SGSubsystemMgr::INIT);


    ////////////////////////////////////////////////////////////////////
    // Add the FlightGear property utilities.
    ////////////////////////////////////////////////////////////////////
    globals->add_subsystem("properties", new FGProperties);


    ////////////////////////////////////////////////////////////////////
    // Add the performance monitoring system.
    ////////////////////////////////////////////////////////////////////
    globals->add_subsystem("performance-mon",
            new SGPerformanceMonitor(globals->get_subsystem_mgr(),
                                     fgGetNode("/sim/performance-monitor", true)));

    ////////////////////////////////////////////////////////////////////
    // Initialize the material property subsystem.
    ////////////////////////////////////////////////////////////////////

    SGPath mpath( globals->get_fg_root() );
    mpath.append( fgGetString("/sim/rendering/materials-file") );
    if ( ! globals->get_matlib()->load(globals->get_fg_root(), mpath.str(),
            globals->get_props()) ) {
        SG_LOG( SG_GENERAL, SG_ALERT,
                "Error loading materials file " << mpath.str() );
        exit(-1);
    }

    globals->add_subsystem( "http", new FGHTTPClient );
    
    ////////////////////////////////////////////////////////////////////
    // Initialize the scenery management subsystem.
    ////////////////////////////////////////////////////////////////////

    globals->get_scenery()->get_scene_graph()
        ->addChild(simgear::Particles::getCommonRoot());
    simgear::GlobalParticleCallback::setSwitch(fgGetNode("/sim/rendering/particles", true));

    ////////////////////////////////////////////////////////////////////
    // Initialize the flight model subsystem.
    ////////////////////////////////////////////////////////////////////

    globals->add_subsystem("flight", new FDMShell, SGSubsystemMgr::FDM);

    ////////////////////////////////////////////////////////////////////
    // Initialize the weather subsystem.
    ////////////////////////////////////////////////////////////////////

    // Initialize the weather modeling subsystem
    globals->add_subsystem("environment", new FGEnvironmentMgr);
    globals->add_subsystem("ephemeris", new Ephemeris);
    
    ////////////////////////////////////////////////////////////////////
    // Initialize the aircraft systems and instrumentation (before the
    // autopilot.)
    ////////////////////////////////////////////////////////////////////

    globals->add_subsystem("systems", new FGSystemMgr, SGSubsystemMgr::FDM);
    globals->add_subsystem("instrumentation", new FGInstrumentMgr, SGSubsystemMgr::FDM);
    globals->add_subsystem("hud", new HUD, SGSubsystemMgr::DISPLAY);
    globals->add_subsystem("cockpit-displays", new flightgear::CockpitDisplayManager, SGSubsystemMgr::DISPLAY);
  
    ////////////////////////////////////////////////////////////////////
    // Initialize the XML Autopilot subsystem.
    ////////////////////////////////////////////////////////////////////

    globals->add_subsystem( "xml-autopilot", FGXMLAutopilotGroup::createInstance("autopilot"), SGSubsystemMgr::FDM );
    globals->add_subsystem( "xml-proprules", FGXMLAutopilotGroup::createInstance("property-rule"), SGSubsystemMgr::GENERAL );
    globals->add_subsystem( "route-manager", new FGRouteMgr );

    ////////////////////////////////////////////////////////////////////
    // Initialize the Input-Output subsystem
    ////////////////////////////////////////////////////////////////////
    globals->add_subsystem( "io", new FGIO );
  
    ////////////////////////////////////////////////////////////////////
    // Create and register the logger.
    ////////////////////////////////////////////////////////////////////
    
    globals->add_subsystem("logger", new FGLogger);

    ////////////////////////////////////////////////////////////////////
    // Create and register the XML GUI.
    ////////////////////////////////////////////////////////////////////

    globals->add_subsystem("gui", new NewGUI, SGSubsystemMgr::INIT);

    //////////////////////////////////////////////////////////////////////
    // Initialize the 2D cloud subsystem.
    ////////////////////////////////////////////////////////////////////
    fgGetBool("/sim/rendering/bump-mapping", false);

    ////////////////////////////////////////////////////////////////////
    // Initialize the canvas 2d drawing subsystem.
    ////////////////////////////////////////////////////////////////////
    globals->add_subsystem("Canvas", new CanvasMgr, SGSubsystemMgr::DISPLAY);
    globals->add_subsystem("CanvasGUI", new GUIMgr, SGSubsystemMgr::DISPLAY);

    ////////////////////////////////////////////////////////////////////
    // Initialise the ATIS Manager
    // Note that this is old stuff, but is necessary for the
    // current ATIS implementation. Therefore, leave it in here
    // until the ATIS system is ported over to make use of the ATIS 
    // sub system infrastructure.
    ////////////////////////////////////////////////////////////////////

    globals->add_subsystem("ATIS", new FGATISMgr, SGSubsystemMgr::INIT, 0.4);

    ////////////////////////////////////////////////////////////////////
   // Initialize the ATC subsystem
    ////////////////////////////////////////////////////////////////////
    globals->add_subsystem("ATC", new FGATCManager, SGSubsystemMgr::POST_FDM);

    ////////////////////////////////////////////////////////////////////
    // Initialize multiplayer subsystem
    ////////////////////////////////////////////////////////////////////

    globals->add_subsystem("mp", new FGMultiplayMgr, SGSubsystemMgr::POST_FDM);

    ////////////////////////////////////////////////////////////////////
    // Initialise the AI Model Manager
    ////////////////////////////////////////////////////////////////////
    SG_LOG(SG_GENERAL, SG_INFO, "  AI Model Manager");
    globals->add_subsystem("ai-model", new FGAIManager, SGSubsystemMgr::POST_FDM);
    globals->add_subsystem("submodel-mgr", new FGSubmodelMgr, SGSubsystemMgr::POST_FDM);


    // It's probably a good idea to initialize the top level traffic manager
    // After the AI and ATC systems have been initialized properly.
    // AI Traffic manager
    globals->add_subsystem("traffic-manager", new FGTrafficManager, SGSubsystemMgr::POST_FDM);

    ////////////////////////////////////////////////////////////////////
    // Add a new 2D panel.
    ////////////////////////////////////////////////////////////////////

    fgSetArchivable("/sim/panel/visibility");
    fgSetArchivable("/sim/panel/x-offset");
    fgSetArchivable("/sim/panel/y-offset");
    fgSetArchivable("/sim/panel/jitter");
  
    ////////////////////////////////////////////////////////////////////
    // Initialize the controls subsystem.
    ////////////////////////////////////////////////////////////////////
    
    globals->add_subsystem("controls", new FGControls, SGSubsystemMgr::GENERAL);

    ////////////////////////////////////////////////////////////////////
    // Initialize the input subsystem.
    ////////////////////////////////////////////////////////////////////

    globals->add_subsystem("input", new FGInput, SGSubsystemMgr::GENERAL);


    ////////////////////////////////////////////////////////////////////
    // Initialize the replay subsystem
    ////////////////////////////////////////////////////////////////////
    globals->add_subsystem("replay", new FGReplay);

#ifdef ENABLE_AUDIO_SUPPORT
    ////////////////////////////////////////////////////////////////////
    // Initialize the sound-effects subsystem.
    ////////////////////////////////////////////////////////////////////
    globals->add_subsystem("voice", new FGVoiceMgr, SGSubsystemMgr::DISPLAY);
#endif

    ////////////////////////////////////////////////////////////////////
    // Initialize the lighting subsystem.
    ////////////////////////////////////////////////////////////////////

    globals->add_subsystem("lighting", new FGLight, SGSubsystemMgr::DISPLAY);
    
    // ordering here is important : Nasal (via events), then models, then views
    globals->add_subsystem("events", globals->get_event_mgr(), SGSubsystemMgr::DISPLAY);

    globals->add_subsystem("aircraft-model", new FGAircraftModel, SGSubsystemMgr::DISPLAY);
    globals->add_subsystem("model-manager", new FGModelMgr, SGSubsystemMgr::DISPLAY);

    globals->add_subsystem("view-manager", new FGViewMgr, SGSubsystemMgr::DISPLAY);

    globals->add_subsystem("tile-manager", globals->get_tile_mgr(), 
      SGSubsystemMgr::DISPLAY);
}

void fgPostInitSubsystems()
{
    SGTimeStamp st;
    st.stamp();
  
    ////////////////////////////////////////////////////////////////////////
    // Initialize the Nasal interpreter.
    // Do this last, so that the loaded scripts see initialized state
    ////////////////////////////////////////////////////////////////////////
    FGNasalSys* nasal = new FGNasalSys();
    globals->add_subsystem("nasal", nasal, SGSubsystemMgr::INIT);
    nasal->init();
    SG_LOG(SG_GENERAL, SG_INFO, "Nasal init took:" << st.elapsedMSec());
  
    // initialize methods that depend on other subsystems.
    st.stamp();
    globals->get_subsystem_mgr()->postinit();
    SG_LOG(SG_GENERAL, SG_INFO, "Subsystems postinit took:" << st.elapsedMSec());
  
    ////////////////////////////////////////////////////////////////////////
    // End of subsystem initialization.
    ////////////////////////////////////////////////////////////////////

    fgSetBool("/sim/crashed", false);
    fgSetBool("/sim/initialized", true);

    SG_LOG( SG_GENERAL, SG_INFO, endl);

                                // Save the initial state for future
                                // reference.
    globals->saveInitialState();
}

// Reset: this is what the 'reset' command (and hence, GUI) is attached to
void fgReInitSubsystems()
{
    SGPropertyNode *master_freeze = fgGetNode("/sim/freeze/master");

    SG_LOG( SG_GENERAL, SG_INFO, "fgReInitSubsystems()");

// setup state to begin re-init
    bool freeze = master_freeze->getBoolValue();
    if ( !freeze ) {
        master_freeze->setBoolValue(true);
    }
    
    fgSetBool("/sim/signals/reinit", true);
    fgSetBool("/sim/crashed", false);

// do actual re-init steps
    globals->get_subsystem("flight")->unbind();
    
  // reset control state, before restoring initial state; -set or config files
  // may specify values for flaps, trim tabs, magnetos, etc
    globals->get_controls()->reset_all();
        
    globals->restoreInitialState();

    // update our position based on current presets
    flightgear::initPosition();
    
    // Force reupdating the positions of the ai 3d models. They are used for
    // initializing ground level for the FDM.
    globals->get_subsystem("ai-model")->reinit();

    // Initialize the FDM
    globals->get_subsystem("flight")->reinit();

    // reset replay buffers
    globals->get_subsystem("replay")->reinit();
    
    // reload offsets from config defaults
    globals->get_viewmgr()->reinit();

    globals->get_subsystem("time")->reinit();

    // need to bind FDMshell again, since we manually unbound it above...
    globals->get_subsystem("flight")->bind();

    // need to reset aircraft (systems/instruments) so they can adapt to current environment
    globals->get_subsystem("systems")->reinit();
    globals->get_subsystem("instrumentation")->reinit();

    globals->get_subsystem("ATIS")->reinit();

// setup state to end re-init
    fgSetBool("/sim/signals/reinit", false);
    if ( !freeze ) {
        master_freeze->setBoolValue(false);
    }
    fgSetBool("/sim/sceneryloaded",false);
}


///////////////////////////////////////////////////////////////////////////////
// helper object to implement the --show-aircraft command.
// resides here so we can share the fgFindAircraftInDir template above,
// and hence ensure this command lists exectly the same aircraft as the normal
// loading path.
class ShowAircraft 
{
public:
  ShowAircraft()
  {
    _minStatus = getNumMaturity(fgGetString("/sim/aircraft-min-status", "all"));
  }
  
  
  void show(const SGPath& path)
  {
    fgFindAircraftInDir(path, this, &ShowAircraft::processAircraft);
  
    std::sort(_aircraft.begin(), _aircraft.end(), ciLessLibC());
    SG_LOG( SG_GENERAL, SG_ALERT, "" ); // To popup the console on Windows
    cout << "Available aircraft:" << endl;
    for ( unsigned int i = 0; i < _aircraft.size(); i++ ) {
        cout << _aircraft[i] << endl;
    }
  }
  
private:
  bool processAircraft(const SGPath& path)
  {
    SGPropertyNode root;
    try {
       readProperties(path.str(), &root);
    } catch (sg_exception& ) {
       return false;
    }
  
    int maturity = 0;
    string descStr("   ");
    descStr += path.file();
  // trim common suffix from file names
    int nPos = descStr.rfind("-set.xml");
    if (nPos == (int)(descStr.size() - 8)) {
      descStr.resize(nPos);
    }
    
    SGPropertyNode *node = root.getNode("sim");
    if (node) {
      SGPropertyNode* desc = node->getNode("description");
      // if a status tag is found, read it in
      if (node->hasValue("status")) {
        maturity = getNumMaturity(node->getStringValue("status"));
      }
      
      if (desc) {
        if (descStr.size() <= 27+3) {
          descStr.append(29+3-descStr.size(), ' ');
        } else {
          descStr += '\n';
          descStr.append( 32, ' ');
        }
        descStr += desc->getStringValue();
      }
    } // of have 'sim' node
    
    if (maturity < _minStatus) {
      return false;
    }

    _aircraft.push_back(descStr);
    return false;
  }


  int getNumMaturity(const char * str) 
  {
    // changes should also be reflected in $FG_ROOT/data/options.xml & 
    // $FG_ROOT/data/Translations/string-default.xml
    const char* levels[] = {"alpha","beta","early-production","production"}; 

    if (!strcmp(str, "all")) {
      return 0;
    }

    for (size_t i=0; i<(sizeof(levels)/sizeof(levels[0]));i++) 
      if (strcmp(str,levels[i])==0)
        return i;

    return 0;
  }

  // recommended in Meyers, Effective STL when internationalization and embedded
  // NULLs aren't an issue.  Much faster than the STL or Boost lex versions.
  struct ciLessLibC : public std::binary_function<string, string, bool>
  {
    bool operator()(const std::string &lhs, const std::string &rhs) const
    {
      return strcasecmp(lhs.c_str(), rhs.c_str()) < 0 ? 1 : 0;
    }
  };

  int _minStatus;
  string_list _aircraft;
};

void fgShowAircraft(const SGPath &path)
{
    ShowAircraft s;
    s.show(path);
        
#ifdef _MSC_VER
    cout << "Hit a key to continue..." << endl;
    std::cin.get();
#endif
}


