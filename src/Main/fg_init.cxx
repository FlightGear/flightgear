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

#include <simgear/compiler.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>             // strcmp()

#if defined(SG_WINDOWS)
#  include <io.h>               // isatty()
#  include <process.h>          // _getpid()
#  include <Windows.h>
#  define isatty _isatty
#else
// for open() and options
#  include <sys/types.h>        
#  include <sys/stat.h>
#  include <fcntl.h>
#endif

#include <string>

#include <boost/algorithm/string/compare.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <osgViewer/Viewer>

#include <simgear/canvas/Canvas.hxx>
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
#include <simgear/scene/tsync/terrasync.hxx>

#include <simgear/scene/model/modellib.hxx>
#include <simgear/scene/material/matlib.hxx>
#include <simgear/scene/material/Effect.hxx>
#include <simgear/scene/model/particles.hxx>
#include <simgear/scene/tsync/terrasync.hxx>

#include <Aircraft/controls.hxx>
#include <Aircraft/replay.hxx>
#include <Aircraft/FlightHistory.hxx>
#include <Airports/runways.hxx>
#include <Airports/airport.hxx>
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
#include <Canvas/FGCanvasSystemAdapter.hxx>
#include <GUI/new_gui.hxx>
#include <GUI/MessageBox.hxx>
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
#include <Time/TimeManager.hxx>

#include <Traffic/TrafficMgr.hxx>
#include <MultiPlayer/multiplaymgr.hxx>
#include <FDM/fdm_shell.hxx>
#include <Environment/ephemeris.hxx>
#include <Environment/environment_mgr.hxx>
#include <Viewer/renderer.hxx>
#include <Viewer/viewmgr.hxx>
#include <Viewer/FGEventHandler.hxx>
#include <Navaids/NavDataCache.hxx>
#include <Instrumentation/HUD/HUD.hxx>
#include <Cockpit/cockpitDisplayManager.hxx>
#include <Network/HTTPClient.hxx>
#include <Network/fgcom.hxx>

#include <Viewer/CameraGroup.hxx>

#include "fg_init.hxx"
#include "fg_io.hxx"
#include "fg_commands.hxx"
#include "fg_props.hxx"
#include "FGInterpolator.hxx"
#include "options.hxx"
#include "globals.hxx"
#include "logger.hxx"
#include "main.hxx"
#include "positioninit.hxx"
#include "util.hxx"
#include "AircraftDirVisitorBase.hxx"

#if defined(SG_MAC)
#include <GUI/CocoaHelpers.h> // for Mac impl of platformDefaultDataPath()
#endif

//#define NEW_RESET 1

using std::string;
using std::endl;
using std::cerr;
using std::cout;
using namespace boost::algorithm;

extern osg::ref_ptr<osgViewer::Viewer> viewer;

// Return the current base package version
string fgBasePackageVersion() {
    SGPath base_path( globals->get_fg_root() );
    base_path.append("version");
    if (!base_path.exists()) {
        return string();
    }
    
    sg_gzifstream in( base_path.str() );
    if (!in.is_open()) {
        return string();
    }

    string version;
    in >> version;

    return version;
}

class FindAndCacheAircraft : public AircraftDirVistorBase
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
        flightgear::fatalMessageBox("No aircraft", "No aircraft was specified");
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
            flightgear::fatalMessageBox("Error reading aircraft",
                                        "An error occured reading the requested aircraft (" + aircraft + ")",
                                        e.getFormattedMessage());
          return false;
        }
        
        return true;
      } else {
        SG_LOG(SG_GENERAL, SG_ALERT, "aircraft '" << _searchAircraft << 
               "' not found in specified dir:" << aircraftDir);
          flightgear::fatalMessageBox("Aircraft not found",
                                      "The requested aircraft '" + aircraft + "' could not be found in the specified location.",
                                      aircraftDir);
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
  
        visitAircraftPaths();
    }
    
    if (_foundPath.str().empty()) {
      SG_LOG(SG_GENERAL, SG_ALERT, "Cannot find specified aircraft: " << aircraft );
        flightgear::fatalMessageBox("Aircraft not found",
                                    "The requested aircraft '" + aircraft + "' could not be found in any of the search paths");

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
        flightgear::fatalMessageBox("Error reading aircraft",
                                    "An error occured reading the requested aircraft (" + aircraft + ")",
                                    e.getFormattedMessage());
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
  
  virtual VisitResult visit(const SGPath& p)
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
        return VISIT_DONE;
    }

    return VISIT_CONTINUE;
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

#elif defined(SG_MAC)

// platformDefaultDataPath defined in GUI/CocoaHelpers.h

#else
static SGPath platformDefaultDataPath()
{
  return SGPath::home() / ".fgfs";
}
#endif

bool fgInitHome()
{
  SGPath dataPath = SGPath::fromEnv("FG_HOME", platformDefaultDataPath());
  globals->set_fg_home(dataPath.c_str());
    
    simgear::Dir fgHome(dataPath);
    if (!fgHome.exists()) {
        fgHome.create(0755);
    }
    
    if (!fgHome.exists()) {
        flightgear::fatalMessageBox("Problem setting up user data",
                                    "Unable to create the user-data storage folder at: '"
                                    + dataPath.str() + "'");
        return false;
    }
    
    if (fgGetBool("/sim/fghome-readonly", false)) {
        // user / config forced us into readonly mode, fine
        SG_LOG(SG_GENERAL, SG_INFO, "Running with FG_HOME readonly");
        return true;
    }
    
// write our PID, and check writeability
    SGPath pidPath(dataPath, "fgfs.pid");
    if (pidPath.exists()) {
        SG_LOG(SG_GENERAL, SG_INFO, "flightgear instance already running, switching to FG_HOME read-only.");
        // set a marker property so terrasync/navcache don't try to write
        // from secondary instances
        fgSetBool("/sim/fghome-readonly", true);
        return true;
    }
    
    char buf[16];
    bool result = false;
#if defined(SG_WINDOWS)
    size_t len = snprintf(buf, 16, "%d", _getpid());

    HANDLE f = CreateFileA(pidPath.c_str(), GENERIC_READ | GENERIC_WRITE, 
						   FILE_SHARE_READ, /* sharing */
                           NULL, /* security attributes */
                           CREATE_NEW, /* error if already exists */
                           FILE_FLAG_DELETE_ON_CLOSE,
						   NULL /* template */);
    
    result = (f != INVALID_HANDLE_VALUE);
    if (result) {
		DWORD written;
        WriteFile(f, buf, len, &written, NULL /* overlapped */);
    }
#else
    // POSIX, do open+unlink trick to the file is deleted on exit, even if we
    // crash or exit(-1)
    ssize_t len = snprintf(buf, 16, "%d", getpid());
    int fd = ::open(pidPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_EXCL, 0644);
    if (fd >= 0) {
        result = ::write(fd, buf, len) == len;
        if( ::unlink(pidPath.c_str()) != 0 ) // delete file when app quits
          result = false;
    }
#endif

    fgSetBool("/sim/fghome-readonly", false);

    if (!result) {
        flightgear::fatalMessageBox("File permissions problem",
                                    "Can't write to user-data storage folder, check file permissions and FG_HOME.",
                                    "User-data at:" + dataPath.str());
    }
    return result;
}

// Read in configuration (file and command line)
int fgInitConfig ( int argc, char **argv, bool reinit )
{
    SGPath dataPath = globals->get_fg_home();
    
    simgear::Dir exportDir(simgear::Dir(dataPath).file("Export"));
    if (!exportDir.exists()) {
      exportDir.create(0755);
    }
    
    // Set /sim/fg-home and don't allow malign code to override it until
    // Nasal security is set up.  Use FG_HOME if necessary.
    SGPropertyNode *home = fgGetNode("/sim", true);
    home->removeChild("fg-home", 0, false);
    home = home->getChild("fg-home", 0, true);
    home->setStringValue(dataPath.c_str());
    home->setAttribute(SGPropertyNode::WRITE, false);
  
    fgSetDefaults();
    flightgear::Options* options = flightgear::Options::sharedInstance();
    if (!reinit) {
        options->init(argc, argv, dataPath);
    }
    
    bool loadDefaults = options->shouldLoadDefaultConfig();
    if (loadDefaults) {
      // Read global preferences from $FG_ROOT/preferences.xml
      SG_LOG(SG_INPUT, SG_INFO, "Reading global preferences");
      fgLoadProps("preferences.xml", globals->get_props());
      SG_LOG(SG_INPUT, SG_INFO, "Finished Reading global preferences");
        
      // do not load user settings when reset to default is requested, or if
      // told to explicitly ignore
      if (options->isOptionSet("restore-defaults") || options->isOptionSet("ignore-autosave"))
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
    
    return flightgear::FG_OPTIONS_OK;
}

int fgInitAircraft(bool reinit)
{
    // Scan user config files and command line for a specified aircraft.
    if (!reinit) {
        flightgear::Options::sharedInstance()->initAircraft();
    }
    
    FindAndCacheAircraft f(globals->get_props());
    if (!f.loadAircraft()) {
        return flightgear::FG_OPTIONS_ERROR;
    }
    
    return flightgear::FG_OPTIONS_OK;
}

/**
 * Initialize vor/ndb/ils/fix list management and query systems (as
 * well as simple airport db list)
 * This is called multiple times in the case of a cache rebuild,
 * to allow lengthy caching to take place in the background, without
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
        return false;
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

// Write various configuraton values out to the logs
void fgOutputSettings()
{    
    SG_LOG( SG_GENERAL, SG_INFO, "Configuration State" );
    SG_LOG( SG_GENERAL, SG_INFO, "======= ==============" );
    
    SG_LOG( SG_GENERAL, SG_INFO, "aircraft-dir = " << '"' << fgGetString("/sim/aircraft-dir") << '"' );
    SG_LOG( SG_GENERAL, SG_INFO, "fghome-dir = " << '"' << globals->get_fg_home() << '"');
    SG_LOG( SG_GENERAL, SG_INFO, "aircraft-dir = " << '"' << fgGetString("/sim/aircraft-dir") << '"');
    
    SG_LOG( SG_GENERAL, SG_INFO, "aircraft-search-paths = \n\t" << simgear::strutils::join(globals->get_aircraft_paths(), "\n\t") );
    SG_LOG( SG_GENERAL, SG_INFO, "scenery-search-paths = \n\t" << simgear::strutils::join(globals->get_fg_scenery(), "\n\t") );
}

// This is the top level init routine which calls all the other
// initialization routines.  If you are adding a subsystem to flight
// gear, its initialization call should located in this routine.
// Returns non-zero if a problem encountered.
void fgCreateSubsystems(bool duringReset) {

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
    FGInterpolator* interp = new FGInterpolator;
    interp->setRealtimeProperty(fgGetNode("/sim/time/delta-realtime-sec", true));
    globals->add_subsystem("prop-interpolator", interp, SGSubsystemMgr::INIT);
    SGPropertyNode::setInterpolationMgr(interp);

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
       throw sg_io_exception("Error loading materials file", mpath);
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
    simgear::canvas::Canvas::setSystemAdapter(
      simgear::canvas::SystemAdapterPtr(new canvas::FGCanvasSystemAdapter)
    );
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
    globals->add_subsystem("history", new FGFlightHistory);
    
#ifdef ENABLE_AUDIO_SUPPORT
    ////////////////////////////////////////////////////////////////////
    // Initialize the sound-effects subsystem.
    ////////////////////////////////////////////////////////////////////
    globals->add_subsystem("voice", new FGVoiceMgr, SGSubsystemMgr::DISPLAY);
#endif

#ifdef ENABLE_IAX
    ////////////////////////////////////////////////////////////////////
    // Initialize the FGCom subsystem.
    ////////////////////////////////////////////////////////////////////
    globals->add_subsystem("fgcom", new FGCom);
#endif

    ////////////////////////////////////////////////////////////////////
    // Initialize the lighting subsystem.
    ////////////////////////////////////////////////////////////////////

    // ordering here is important : Nasal (via events), then models, then views
    if (!duringReset) {
        globals->add_subsystem("lighting", new FGLight, SGSubsystemMgr::DISPLAY);
        globals->add_subsystem("events", globals->get_event_mgr(), SGSubsystemMgr::DISPLAY);
    }

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

    // Ensure IOrules and path validation are working properly by trying to
    // access a folder/file which should never be accessible.
    const char* no_access_path =
#ifdef _WIN32
      "Z:"
#endif
      "/do-not-access";

    if( fgValidatePath(no_access_path, true) )
      SG_LOG
      (
        SG_GENERAL,
        SG_ALERT,
        "Check your IOrules! (write to '" << no_access_path << "' is allowed)"
      );
    if( fgValidatePath(no_access_path, false) )
      SG_LOG
      (
        SG_GENERAL,
        SG_ALERT,
        "Check your IOrules! (read from '" << no_access_path << "' is allowed)"
      );
  
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
#ifdef NEW_RESET
    fgResetIdleState();
    return;
#endif
    
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
    // this will mark position as needed finalized which we'll do in the
    // main-loop
    flightgear::initPosition();
    
    simgear::SGTerraSync* terraSync =
        static_cast<simgear::SGTerraSync*>(globals->get_subsystem("terrasync"));
    if (terraSync) {
        terraSync->reposition();
    }
    
    // Force reupdating the positions of the ai 3d models. They are used for
    // initializing ground level for the FDM.
    globals->get_subsystem("ai-model")->reinit();

    // Initialize the FDM
    globals->get_subsystem("flight")->reinit();

    // reset replay buffers
    globals->get_subsystem("replay")->reinit();
    
    // reload offsets from config defaults
    globals->get_viewmgr()->reinit();

    // ugly: finalizePosition waits for METAR to arrive for the new airport.
    // we don't re-init the environment manager here, since historically we did
    // not, and doing so seems to have other issues. All that's needed is to
    // schedule METAR fetch immediately, so it's available for finalizePosition.
    // So we manually extract the METAR-fetching component inside the environment
    // manager, and re-init that.
    SGSubsystemGroup* envMgr = static_cast<SGSubsystemGroup*>(globals->get_subsystem("environment"));
    if (envMgr) {
      envMgr->get_subsystem("realwx")->reinit();
    }
  
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

void fgStartNewReset()
{
    SGPropertyNode_ptr preserved(new SGPropertyNode);
    
    if (!copyPropertiesWithAttribute(globals->get_props(), preserved, SGPropertyNode::PRESERVE))
        SG_LOG(SG_GENERAL, SG_ALERT, "Error saving preserved state");
    
    fgSetBool("/sim/signals/reinit", true);
    fgSetBool("/sim/freeze/master", true);
    
    SGSubsystemMgr* subsystemManger = globals->get_subsystem_mgr();
    subsystemManger->shutdown();
    subsystemManger->unbind();
    
    // remove most subsystems, with a few exceptions.
    for (int g=0; g<SGSubsystemMgr::MAX_GROUPS; ++g) {
        SGSubsystemGroup* grp = subsystemManger->get_group(static_cast<SGSubsystemMgr::GroupType>(g));
        const string_list& names(grp->member_names());
        string_list::const_iterator it;
        for (it = names.begin(); it != names.end(); ++it) {
            if ((*it == "time") || (*it == "terrasync") || (*it == "events")
                || (*it == "lighting"))
            {
                continue;
            }
            
            try {
                subsystemManger->remove(it->c_str());
            } catch (std::exception& e) {
                SG_LOG(SG_GENERAL, SG_INFO, "caught std::exception shutting down:" << *it);
            } catch (...) {
                SG_LOG(SG_GENERAL, SG_INFO, "caught generic exception shutting down:" << *it);
            }
            
            // don't delete here, dropping the ref should be sufficient
        }
    } // of top-level groups iteration
    
    // order is important here since tile-manager shutdown needs to
    // access the scenery object
    globals->set_tile_mgr(NULL);
    globals->set_scenery(NULL);
    flightgear::CameraGroup::setDefault(NULL);
    
    FGRenderer* render = globals->get_renderer();
    // don't cancel the pager until after shutdown, since AIModels (and
    // potentially others) can queue delete requests on the pager.
    render->getViewer()->getDatabasePager()->cancel();
    render->getViewer()->getDatabasePager()->clear();
    
    osgDB::Registry::instance()->clearObjectCache();
    
    // preserve the event handler; re-creating it would entail fixing the
    // idle handler
    osg::ref_ptr<flightgear::FGEventHandler> eventHandler = render->getEventHandler();
    
    globals->set_renderer(NULL);
    globals->set_matlib(NULL);
    globals->set_chatter_queue(NULL);
    
    simgear::clearEffectCache();
    simgear::SGModelLib::resetPropertyRoot();
        
    simgear::GlobalParticleCallback::setSwitch(NULL);
    
    globals->resetPropertyRoot();
    fgInitConfig(0, NULL, true);
    fgInitGeneral(); // all of this?
    
    if ( copyProperties(preserved, globals->get_props()) ) {
        SG_LOG( SG_GENERAL, SG_INFO, "Preserved state restored successfully" );
    } else {
        SG_LOG( SG_GENERAL, SG_INFO,
               "Some errors restoring preserved state (read-only props?)" );
    }

    fgGetNode("/sim")->removeChild("aircraft-dir");
    fgInitAircraft(true);
    flightgear::Options::sharedInstance()->processOptions();
    
    render = new FGRenderer;
    render->setEventHandler(eventHandler);
    globals->set_renderer(render);
    render->init();
    render->setViewer(viewer.get());
    viewer->getDatabasePager()->setUpThreads(1, 1);
    render->splashinit();
    
    flightgear::CameraGroup::buildDefaultGroup(viewer.get());

    fgOSResetProperties();

    
// init some things manually
// which do not follow the regular init pattern
    
    globals->get_event_mgr()->init();
    globals->get_event_mgr()->setRealtimeProperty(fgGetNode("/sim/time/delta-realtime-sec", true));
    
    globals->set_matlib( new SGMaterialLib );
    
// terra-sync needs the property tree root, pass it back in
    simgear::SGTerraSync* terra_sync = static_cast<simgear::SGTerraSync*>(subsystemManger->get_subsystem("terrasync"));
    terra_sync->setRoot(globals->get_props());

    fgSetBool("/sim/signals/reinit", false);
    fgSetBool("/sim/freeze/master", false);
    fgSetBool("/sim/sceneryloaded",false);
}

