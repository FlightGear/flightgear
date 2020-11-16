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


#include <config.h>

#include <simgear/compiler.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>             // strcmp()

#if defined(SG_WINDOWS)
#define _WINSOCKAPI_
#  include <io.h>               // isatty()
#  include <process.h>          // _getpid()
#  include <Windows.h>
#  define isatty _isatty
#else
// for open() and options
#  include <sys/types.h>        
#  include <sys/stat.h>
#  include <fcntl.h>
#  include <sys/file.h>
#endif

#include <string>

#include <osgViewer/Viewer>

#include <simgear/canvas/Canvas.hxx>
#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/structure/event_mgr.hxx>
#include <simgear/structure/SGPerfMon.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/sg_dir.hxx>
#include <simgear/io/iostreams/sgstream.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/embedded_resources/EmbeddedResourceManager.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/scene/tsync/terrasync.hxx>

#include <simgear/scene/material/Effect.hxx>
#include <simgear/scene/material/matlib.hxx>
#include <simgear/scene/model/modellib.hxx>
#include <simgear/scene/model/particles.hxx>
#include <simgear/scene/tgdb/TreeBin.hxx>
#include <simgear/scene/tgdb/userdata.hxx>
#include <simgear/scene/tsync/terrasync.hxx>

#include <simgear/package/Root.hxx>
#include <simgear/package/Package.hxx>
#include <simgear/package/Install.hxx>
#include <simgear/package/Catalog.hxx>

#include <Add-ons/AddonManager.hxx>

#include <Aircraft/controls.hxx>
#include <Aircraft/replay.hxx>
#include <Aircraft/FlightHistory.hxx>
#include <Aircraft/initialstate.hxx>

#include <Airports/runways.hxx>
#include <Airports/airport.hxx>
#include <Airports/dynamics.hxx>
#include <Airports/airportdynamicsmanager.hxx>

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
#include <AIModel/performancedb.hxx>
#include <Main/locale.hxx>
#include <Navaids/navdb.hxx>
#include <Navaids/navlist.hxx>
#include <Scenery/scenery.hxx>
#include <Scenery/SceneryPager.hxx>
#include <Scripting/NasalSys.hxx>
#include <Sound/voice.hxx>
#include <Sound/soundmanager.hxx>
#include <Systems/system_mgr.hxx>
#include <Time/tide.hxx>
#include <Time/light.hxx>
#include <Time/TimeManager.hxx>

#include <Traffic/TrafficMgr.hxx>
#include <MultiPlayer/multiplaymgr.hxx>
#if defined(ENABLE_SWIFT)
#include <Network/Swift/swift_connection.hxx>
#endif
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
#include <Network/DNSClient.hxx>
#include <Network/fgcom.hxx>
#include <Network/http/httpd.hxx>
#include <Viewer/splash.hxx>
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
#include <Main/sentryIntegration.hxx>

#if defined(SG_MAC)
#include <GUI/CocoaHelpers.h> // for Mac impl of platformDefaultDataPath()
#endif

using std::string;
using std::endl;
using std::cerr;
using std::cout;

using namespace simgear::pkg;

extern osg::ref_ptr<osgViewer::Viewer> viewer;

// Return the current base package version
string fgBasePackageVersion(const SGPath& base_path) {
    SGPath p(base_path);
    p.append("version");
    if (!p.exists()) {
        return string();
    }
    
    sg_gzifstream in( p );
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
  
  /**
   * @brief haveExplicitAircraft - check if the combination of /sim/aircraft
   * and /sim/aircraft-dir defines an explicit -set.xml. We need to detect
   * this case to short-circuit package detection
   * @return
   */
  bool haveExplicitAircraft() const
  {
      const std::string aircraftDir = fgGetString("/sim/aircraft-dir", "");
      if (aircraftDir.empty()) {
          return false;
      }

      const std::string aircraft = fgGetString( "/sim/aircraft", "");
      SGPath setFile = SGPath::fromUtf8(aircraftDir) / (aircraft + "-set.xml");
      return setFile.exists();
  }

  bool loadAircraft()
  {
    std::string aircraft = fgGetString( "/sim/aircraft", "");
    if (aircraft.empty()) {
      flightgear::fatalMessageBoxWithoutExit("No aircraft",
                                             "No aircraft was specified");
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
          readProperties(setFile, globals->get_props());
        } catch ( const sg_exception &e ) {
          SG_LOG(SG_INPUT, SG_ALERT,
                 "Error reading aircraft: " << e.getFormattedMessage());
          flightgear::fatalMessageBoxWithoutExit(
            "Error reading aircraft",
            "An error occured reading the requested aircraft (" + aircraft + ")",
            e.getFormattedMessage());
          return false;
        }

        checkAircraftMinVersion();

          // apply state after the -set.xml, but before any options are are set
          flightgear::applyInitialState();
        return true;
      } else {
        SG_LOG(SG_GENERAL, SG_ALERT, "aircraft '" << _searchAircraft << 
               "' not found in specified dir:" << aircraftDir);
        flightgear::fatalMessageBoxWithoutExit(
          "Aircraft not found",
          "The requested aircraft (" + aircraft + ") could not be found "
          "in the specified location.",
          aircraftDir);
        return false;
      }
    }
    
    if (!checkCache()) {
      // prepare cache for re-scan
      SGPropertyNode *n = _cache->getNode("fg-root", true);
      n->setStringValue(globals->get_fg_root().utf8Str());
      n->setAttribute(SGPropertyNode::USERARCHIVE, true);
      n = _cache->getNode("fg-aircraft", true);
      n->setStringValue(getAircraftPaths().c_str());
      n->setAttribute(SGPropertyNode::USERARCHIVE, true);
      _cache->removeChildren("aircraft");
  
        visitAircraftPaths();
    }
    
    if (_foundPath.isNull()) {
      SG_LOG(SG_GENERAL, SG_ALERT,
             "Cannot find the specified aircraft: '" << aircraft << "'");
      flightgear::fatalMessageBoxWithoutExit(
        "Aircraft not found",
        "The requested aircraft (" + aircraft + ") could not be found "
        "in any of the search paths.");
      return false;
    }
    
    SG_LOG(SG_GENERAL, SG_INFO, "Loading aircraft -set file from:" << _foundPath);
    fgSetString( "/sim/aircraft-dir", _foundPath.dir().c_str());
    if (!_foundPath.exists()) {
      SG_LOG(SG_GENERAL, SG_ALERT, "Unable to find -set file:" << _foundPath);
      return false;
    }
    
    try {
      readProperties(_foundPath, globals->get_props());
    } catch ( const sg_exception &e ) {
      SG_LOG(SG_INPUT, SG_ALERT,
             "Error reading aircraft: " << e.getFormattedMessage());
      flightgear::fatalMessageBoxWithoutExit(
        "Error reading aircraft",
        "An error occured reading the requested aircraft (" + aircraft + ")",
        e.getFormattedMessage());
      return false;
    }

      // apply state after the -set.xml, but before any options are are set
      flightgear::applyInitialState();

      checkAircraftMinVersion();

    return true;
  }
  
private:
    std::string getAircraftPaths()
    {
        return SGPath::join(globals->get_aircraft_paths(), ";");
    }
  
  bool checkCache()
  {
    if (globals->get_fg_root().utf8Str() != _cache->getStringValue("fg-root", "")) {
      return false; // cache mismatch
    }

    if (getAircraftPaths() != _cache->getStringValue("fg-aircraft", "")) {
      return false; // cache mismatch
    }
    
    vector<SGPropertyNode_ptr> cache = _cache->getChildren("aircraft");
    for (unsigned int i = 0; i < cache.size(); i++) {
      const char *name = cache[i]->getStringValue("file", "");
      if (!simgear::strutils::iequals(_searchAircraft, name)) {
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
      SGPath realPath = p.realpath();
    // create cache node
    int i = 0;
    while (1) {
        if (!_cache->getChild("aircraft", i++, false))
            break;
    }
    
    SGPropertyNode *n, *entry = _cache->getChild("aircraft", --i, true);

    std::string fileName(realPath.file());
    n = entry->getNode("file", true);
    n->setStringValue(fileName);
    n->setAttribute(SGPropertyNode::USERARCHIVE, true);

    n = entry->getNode("path", true);
    n->setStringValue(realPath.dir());
    n->setAttribute(SGPropertyNode::USERARCHIVE, true);

    if (simgear::strutils::iequals(fileName, _searchAircraft)) {
        _foundPath = realPath;
        return VISIT_DONE;
    }

    return VISIT_CONTINUE;
  }

  bool checkAircraftMinVersion()
  {
      SGPropertyNode* minVersionNode = globals->get_props()->getNode("/sim/minimum-fg-version");
      if (minVersionNode) {
          std::string minVersion = minVersionNode->getStringValue();
          const int c = simgear::strutils::compare_versions(FLIGHTGEAR_VERSION, minVersion, 2);
          if (c < 0) {
              SG_LOG(SG_AIRCRAFT, SG_DEV_ALERT, "Aircraft minimum version (" << minVersion <<
                     ") is higher than FG version:" << FLIGHTGEAR_VERSION);
              flightgear::modalMessageBox("Aircraft requires newer version of FlightGear",
                                          "The selected aircraft requires FlightGear version " + minVersion
                                          + " to work correctly. Some features may not work as expected, or the aircraft may not load at all.");
              return false;
          }
      } else {
          SG_LOG(SG_AIRCRAFT, SG_DEV_ALERT, "Aircraft does not specify a minimum FG version: please add one at /sim/minimum-fg-version");
      }
      
      auto compatNodes = globals->get_props()->getNode("/sim")->getChildren("compatible-fg-version");
      if (!compatNodes.empty()) {
          bool showCompatWarning = true;
          
          // if we have at least one compatibility node, then it needs to match
          for (const auto& cn : compatNodes) {
              const auto v = cn->getStringValue();
              if (simgear::strutils::compareVersionToWildcard(FLIGHTGEAR_VERSION, v)) {
                  showCompatWarning = false;
                  break;
              }
          }
          
          if (showCompatWarning) {
              flightgear::modalMessageBox("Aircraft not compatible with this version",
              "The selected aircraft has not been checked for compatability with this version of FlightGear (" FLIGHTGEAR_VERSION  "). "
                                          "Some aircraft features might not work, or might be displayed incorrectly.");
          }
      }
      
      return true;
  }
  
  std::string _searchAircraft;
  SGPath _foundPath;
  SGPropertyNode* _cache;
};

#ifdef _WIN32
static SGPath platformDefaultDataPath()
{
  SGPath appDataPath = SGPath::fromEnv("APPDATA");

  if (appDataPath.isNull()) {
    flightgear::fatalMessageBoxThenExit(
      "FlightGear", "Unable to get the value of APPDATA.",
      "FlightGear is unable to retrieve the value of the APPDATA environment "
      "variable. This is quite unexpected on Windows platforms, and FlightGear "
      "can't continue its execution without this value, sorry.");
  }

  return appDataPath / "flightgear.org";
}

#elif defined(SG_MAC)

// platformDefaultDataPath defined in GUI/CocoaHelpers.h

#else
static SGPath platformDefaultDataPath()
{
  return SGPath::home() / ".fgfs";
}
#endif

#if defined(SG_WINDOWS)
static HANDLE static_fgHomeWriteMutex = nullptr;
#endif

SGPath fgHomePath()
{
    return SGPath::fromEnv("FG_HOME", platformDefaultDataPath());
}

InitHomeResult fgInitHome()
{
  SGPath dataPath = fgHomePath();
  globals->set_fg_home(dataPath);
    
    simgear::Dir fgHome(dataPath);
    if (!fgHome.exists()) {
        fgHome.create(0755);
    }
    
    if (!fgHome.exists()) {
        flightgear::fatalMessageBoxWithoutExit(
          "Problem setting up user data",
          "Unable to create the user-data storage folder at '" +
          dataPath.utf8Str() + "'.");
        return InitHomeAbort;
    }
    
    if (fgGetBool("/sim/fghome-readonly", false)) {
        // user / config forced us into readonly mode, fine
        SG_LOG(SG_GENERAL, SG_INFO, "Running with FG_HOME readonly");
        return InitHomeExplicitReadOnly;
    }
    
    InitHomeResult result = InitHomeOkay;
#if defined(SG_WINDOWS)
	// don't use a PID file on Windows, because deleting on close is
	// unreliable and causes false-positives. Instead, use a named
	// mutex.

	static_fgHomeWriteMutex = CreateMutexA(nullptr, FALSE, "org.flightgear.fgfs.primary");
	if (static_fgHomeWriteMutex == nullptr) {
		printf("CreateMutex error: %d\n", GetLastError());
		SG_LOG(SG_GENERAL, SG_POPUP, "Failed to create mutex for multi-app protection");
        return InitHomeAbort;
	} else if (GetLastError() == ERROR_ALREADY_EXISTS) {
		SG_LOG(SG_GENERAL, SG_ALERT, "flightgear instance already running, switching to FG_HOME read-only.");
		fgSetBool("/sim/fghome-readonly", true);
        return InitHomeReadOnly;
	} else {
		SG_LOG(SG_GENERAL, SG_INFO, "Created multi-app mutex, we are in writeable mode");
        result = InitHomeOkay;
	}
#else
// write our PID, and check writeability
    SGPath pidPath(dataPath, "fgfs_lock.pid");
    std::string ps = pidPath.utf8Str();

    if (pidPath.exists()) {
        int fd = ::open(ps.c_str(), O_RDONLY, 0644);
        if (fd < 0) {
            SG_LOG(SG_GENERAL, SG_ALERT, "failed to open local file:" << pidPath
                   << "\n\tdue to:" << simgear::strutils::error_string(errno));
            return InitHomeAbort;
        }
        
        int err = ::flock(fd, LOCK_EX | LOCK_NB);
        if (err < 0) {
            if ( errno ==  EWOULDBLOCK) {
                SG_LOG(SG_GENERAL, SG_ALERT, "flightgear instance already running, switching to FG_HOME read-only. ");
                SG_LOG(SG_GENERAL, SG_ALERT, "Couldn't flock() file at:" << pidPath);

                // set a marker property so terrasync/navcache don't try to write
                // from secondary instances
                fgSetBool("/sim/fghome-readonly", true);
                return InitHomeReadOnly;
            } else {
                SG_LOG(SG_GENERAL, SG_ALERT, "failed to lock file:" << pidPath
                       << "\n\tdue to:" << simgear::strutils::error_string(errno));
                return InitHomeAbort;
            }
        }
        
       // we locked it!
        result = InitHomeOkay;
    } else {
        char buf[16];
       std::string ps = pidPath.utf8Str();

       ssize_t len = snprintf(buf, 16, "%d\n", getpid());
       int fd = ::open(ps.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            SG_LOG(SG_GENERAL, SG_ALERT, "failed to open local file:" << pidPath
               << "\n\tdue to:" << simgear::strutils::error_string(errno));
            return InitHomeAbort;
        }
            
        int err = write(fd, buf, len);
        if (err < 0) {
            SG_LOG(SG_GENERAL, SG_ALERT, "failed to write to lock file:" << pidPath
            << "\n\tdue to:" << simgear::strutils::error_string(errno));
            return InitHomeAbort;
        }

        err = flock(fd, LOCK_EX);
        if (err != 0) {
            SG_LOG(SG_GENERAL, SG_ALERT, "failed to lock file:" << pidPath
            << "\n\tdue to:" << simgear::strutils::error_string(errno));
            return InitHomeAbort;
        }
        
        result = InitHomeOkay;
    }
#endif
    fgSetBool("/sim/fghome-readonly", false);
    return result;
}

void fgShutdownHome()
{
#if defined(SG_WINDOWS)
	if (static_fgHomeWriteMutex) {
		CloseHandle(static_fgHomeWriteMutex);
	}
#else
    if (fgGetBool("/sim/fghome-readonly") == false) {
        SGPath pidPath = globals->get_fg_home() / "fgfs_lock.pid";
        pidPath.remove();
    }
#endif
}

void fgDeleteLockFile()
{
#if defined(SG_WINDOWS)
    // there's no file here, so we can't actually delete anything
#else
    SGPath pidPath = globals->get_fg_home() / "fgfs_lock.pid";
    pidPath.remove();
#endif
}

static void createBaseStorageDirForAddons(const SGPath& exportDir)
{
    SGPath addonStorageBasePath = exportDir / "Addons";
    if (addonStorageBasePath.exists()) {
      if (!addonStorageBasePath.isDir()) {
        throw sg_error(
          "Unable to create add-on storage base directory, because the entry "
          "already exists but is not a directory: '" +
          addonStorageBasePath.utf8Str() + "'");
      }
    } else {
      simgear::Dir(addonStorageBasePath).create(0777); // respect user's umask
    }
}

struct SimLogFileLine : SGPropertyChangeListener
{
    SimLogFileLine() {
        fgAddChangeListener(this, "/sim/log-file-line");
    }
    virtual void valueChanged(SGPropertyNode* node) {
        bool    fileLine = node->getBoolValue();
        sglog().setFileLine(fileLine);
    }
};

// Read in configuration (file and command line)
int fgInitConfig ( int argc, char **argv, bool reinit )
{
    SGPath dataPath = globals->get_fg_home();
    
    simgear::Dir exportDir(simgear::Dir(dataPath).file("Export"));
    if (!exportDir.exists()) {
      exportDir.create(0755);
    }

    // Reserve a directory where add-ons can write. There will be a subdir for
    // each add-on, see Addon::createStorageDir() and Addon::getStoragePath().
    createBaseStorageDirForAddons(exportDir.path());

    // Set /sim/fg-home.  Use FG_HOME if necessary.
    // deliberately not a tied property, for fgValidatePath security
    // write-protect to avoid accidents
    SGPropertyNode *home = fgGetNode("/sim", true);
    home->removeChild("fg-home", 0);
    home = home->getChild("fg-home", 0, true);
    home->setStringValue(dataPath.utf8Str());
    home->setAttribute(SGPropertyNode::WRITE, false);
  
    fgSetDefaults();
    flightgear::Options* options = flightgear::Options::sharedInstance();
    if (!reinit) {
        auto result = options->init(argc, argv, dataPath);
        if (result != flightgear::FG_OPTIONS_OK) {
            return result;
        }
    }

    // establish default for developer-mode based upon compiled build types
    bool developerMode = true;
    if (!strcmp(FG_BUILD_TYPE, "Release")) {
        developerMode = false;
    }

    // allow command line to override
    if (options->isOptionSet("developer")) {
        string s = options->valueForOption("developer", "yes");
        developerMode = simgear::strutils::to_bool(s);
    }

    auto node = fgGetNode("/sim/developer-mode", true);
    // ensure this value survives reset
    node->setAttribute(SGPropertyNode::PRESERVE, true);
    node->setBoolValue(developerMode);
    sglog().setDeveloperMode(developerMode);
    
    static SimLogFileLine   simLogFileLine;

    // Read global defaults from $FG_ROOT/defaults
    SG_LOG(SG_GENERAL, SG_DEBUG, "Reading global defaults");
    SGPath defaultsXML = globals->get_fg_root() / "defaults.xml";
    if (!defaultsXML.exists()) {
        flightgear::fatalMessageBoxThenExit(
            "Missing file",
            "Couldn't load an essential simulator data file.",
            defaultsXML.utf8Str());
    }

    if(!fgLoadProps("defaults.xml", globals->get_props()))
    {
        flightgear::fatalMessageBoxThenExit(
            "Corrupted file",
            "Couldn't load an essential simulator data file as it is corrupted.",
            defaultsXML.utf8Str());
    }
    SG_LOG(SG_GENERAL, SG_DEBUG, "Finished Reading global defaults");

    // do not load user settings when reset to default is requested, or if
    // told to explicitly ignore
    if (options->isOptionSet("restore-defaults") || options->isOptionSet("ignore-autosave"))
    {
        SG_LOG(SG_GENERAL, SG_ALERT, "Ignoring user settings. Restoring defaults.");
    } else {
        globals->loadUserSettings(dataPath);
    }

    return flightgear::FG_OPTIONS_OK;
}

static void initAircraftDirsNasalSecurity()
{
    // deliberately not a tied property, for fgValidatePath security
    // write-protect to avoid accidents
    SGPropertyNode* sim = fgGetNode("/sim", true);
    sim->removeChildren("fg-aircraft");

    int index = 0;
    const PathList& aircraft_paths = globals->get_aircraft_paths();
    for (PathList::const_iterator it = aircraft_paths.begin();
         it != aircraft_paths.end(); ++it, ++index )
    {
        SGPropertyNode* n = sim->getChild("fg-aircraft", index, true);
        n->setStringValue(it->utf8Str());
        n->setAttribute(SGPropertyNode::WRITE, false);
    }
}

void fgInitAircraftPaths(bool reinit)
{
  if (!globals->packageRoot()) {
      fgInitPackageRoot();
  }

  SGSharedPtr<Root> pkgRoot(globals->packageRoot());
  SGPropertyNode* aircraftProp = fgGetNode("/sim/aircraft", true);
  aircraftProp->setAttribute(SGPropertyNode::PRESERVE, true);

  if (!reinit) {
      flightgear::Options::sharedInstance()->initPaths();
  }
}

int fgInitAircraft(bool reinit)
{
    if (!reinit) {
        auto r = flightgear::Options::sharedInstance()->initAircraft();
        if (r == flightgear::FG_OPTIONS_SHOW_AIRCRAFT)
            return r;
    }
    
    FindAndCacheAircraft f(globals->get_props());
    const bool haveExplicit = f.haveExplicitAircraft();

    SGSharedPtr<Root> pkgRoot(globals->packageRoot());
    SGPropertyNode* aircraftProp = fgGetNode("/sim/aircraft", true);
    string aircraftId(aircraftProp->getStringValue());

    flightgear::addSentryTag("aircraft", aircraftId);
        
    PackageRef acftPackage;
    if (!haveExplicit) {
        acftPackage = pkgRoot->getPackageById(aircraftId);
    }

    if (acftPackage) {
        if (acftPackage->isInstalled()) {
            SG_LOG(SG_GENERAL, SG_INFO, "Loading aircraft from package:" << acftPackage->qualifiedId());

            // set catalog path so intra-package dependencies within the catalog
            // are resolved correctly.
            globals->set_catalog_aircraft_path(acftPackage->catalog()->installRoot());

            // set aircraft-dir to short circuit the search process
            InstallRef acftInstall = acftPackage->install();
            fgSetString("/sim/aircraft-dir", acftInstall->path().utf8Str());

            // overwrite the fully qualified ID with the aircraft one, so the
            // code in FindAndCacheAircraft works as normal
            // note since we may be using a variant, we can't use the package ID
            size_t lastDot = aircraftId.rfind('.');
            if (lastDot != std::string::npos) {
                aircraftId = aircraftId.substr(lastDot + 1);
            }
            aircraftProp->setStringValue(aircraftId);

            // run the traditional-code path below
        } else {
#if 0
            // naturally the better option would be to on-demand install it!
            flightgear::fatalMessageBoxWithoutExit(
                "Aircraft not installed",
                "Requested aircraft is not currently installed.",
                aircraftId);

            return flightgear::FG_OPTIONS_ERROR;
#endif
            // fall back the default aircraft instead
        }
    }

    initAircraftDirsNasalSecurity();

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

    if (!cache) {
        cache = flightgear::NavDataCache::createInstance();
        doingRebuild = cache->isRebuildRequired();
    }

    static const char* splashIdentsByRebuildPhase[] = {
        "loading-nav-dat",
        "navdata-reading-apt-dat-files",
        "navdata-loading-airports",
        "navdata-navaids",
        "navdata-fixes",
        "navdata-pois"
    };

    if (doingRebuild) {
        flightgear::NavDataCache::RebuildPhase phase;
        phase = cache->rebuild();
        if (phase != flightgear::NavDataCache::REBUILD_DONE) {
            // update the splash text based on percentage, phase

            fgSplashProgress(splashIdentsByRebuildPhase[phase],
                             cache->rebuildPhaseCompletionPercentage());

            // sleep to give the rebuild thread more time
            SGTimeStamp::sleepForMSec(50);
            return false;
        }
    }

  FGTACANList *channellist = new FGTACANList;
  globals->set_channellist( channellist );
  
  SGPath path(globals->get_fg_root());
  path.append( "Navaids/TACAN_freq.dat" );
  flightgear::NavLoader().loadTacan(path, channellist);
  
  return true;
}

// General house keeping initializations
bool fgInitGeneral() {

    SG_LOG( SG_GENERAL, SG_INFO, "General Initialization" );
    SG_LOG( SG_GENERAL, SG_INFO, "======= ==============" );

    if ( globals->get_fg_root().isNull() ) {
        // No root path set? Then bail ...
        SG_LOG( SG_GENERAL, SG_ALERT,
                "Cannot continue without a path to the base package "
                << "being defined." );
        return false;
    }
    SG_LOG( SG_GENERAL, SG_INFO, "FG_ROOT = " << '"' << globals->get_fg_root() << '"' << endl );

    // Note: browser command is hard-coded for Mac/Windows, so this only affects other platforms
    globals->set_browser(fgGetString("/sim/startup/browser-app", WEB_BROWSER));
    fgSetString("/sim/startup/browser-app", globals->get_browser());

    simgear::Dir cwd(simgear::Dir::current());
    SGPropertyNode *curr = fgGetNode("/sim", true);
    curr->removeChild("fg-current", 0);
    curr = curr->getChild("fg-current", 0, true);
    curr->setStringValue(cwd.path().utf8Str());
    curr->setAttribute(SGPropertyNode::WRITE, false);

    fgSetBool("/sim/startup/stdout-to-terminal", isatty(1) != 0 );
    fgSetBool("/sim/startup/stderr-to-terminal", isatty(2) != 0 );

    sgUserDataInit( globals->get_props() );

    return true;
}

// Write various configuraton values out to the logs
void fgOutputSettings()
{    
    SG_LOG( SG_GENERAL, SG_INFO, "Configuration State" );
    SG_LOG( SG_GENERAL, SG_INFO, "============= =====" );
    
    SG_LOG( SG_GENERAL, SG_INFO, "aircraft-dir = " << '"' << fgGetString("/sim/aircraft-dir") << '"' );
    SG_LOG( SG_GENERAL, SG_INFO, "fghome-dir = " << '"' << globals->get_fg_home() << '"');
    SG_LOG( SG_GENERAL, SG_INFO, "download-dir = " << '"' << fgGetString("/sim/paths/download-dir") << '"' );
    SG_LOG( SG_GENERAL, SG_INFO, "terrasync-dir = " << '"' << fgGetString("/sim/terrasync/scenery-dir") << '"' );

    SG_LOG( SG_GENERAL, SG_INFO, "aircraft-search-paths = \n\t" << SGPath::join(globals->get_aircraft_paths(), "\n\t") );
    SG_LOG( SG_GENERAL, SG_INFO, "scenery-search-paths = \n\t" << SGPath::join(globals->get_fg_scenery(), "\n\t") );
}

// This is the top level init routine which calls all the other
// initialization routines.  If you are adding a subsystem to flight
// gear, its initialization call should located in this routine.
// Returns non-zero if a problem encountered.
void fgCreateSubsystems(bool duringReset) {

    SG_LOG( SG_GENERAL, SG_INFO, "Creating Subsystems");
    SG_LOG( SG_GENERAL, SG_INFO, "======== ==========");

    ////////////////////////////////////////////////////////////////////
    // Initialize the sound subsystem.
    ////////////////////////////////////////////////////////////////////
    // Sound manager uses an own subsystem group "SOUND" which is the last
    // to be updated in every loop.
    // Sound manager is updated last so it can use the CPU while the GPU
    // is processing the scenery (doubled the frame-rate for me) -EMH-
    globals->add_new_subsystem<FGSoundManager>(SGSubsystemMgr::SOUND);

    ////////////////////////////////////////////////////////////////////
    // Initialize the event manager subsystem.
    ////////////////////////////////////////////////////////////////////

    globals->get_event_mgr()->init();
    globals->get_event_mgr()->setRealtimeProperty(fgGetNode("/sim/time/delta-realtime-sec", true));

    ////////////////////////////////////////////////////////////////////
    // Initialize the property interpolator subsystem. Put into the INIT
    // group because the "nasal" subsystem may need it at GENERAL take-down.
    ////////////////////////////////////////////////////////////////////
    globals->add_subsystem("prop-interpolator", new FGInterpolator, SGSubsystemMgr::INIT);


    ////////////////////////////////////////////////////////////////////
    // Add the FlightGear property utilities.
    ////////////////////////////////////////////////////////////////////
    globals->add_subsystem("properties", new FGProperties);


    ////////////////////////////////////////////////////////////////////
    // Add the FlightGear property utilities.
    ////////////////////////////////////////////////////////////////////
    globals->add_new_subsystem<flightgear::AirportDynamicsManager>();

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
    if ( ! globals->get_matlib()->load(globals->get_fg_root(), mpath, globals->get_props()) ) {
       throw sg_io_exception("Error loading materials file", mpath);
    }

    // may exist already due to GUI startup
    if (!globals->get_subsystem<FGHTTPClient>()) {
        globals->add_new_subsystem<FGHTTPClient>();
    }
    globals->add_new_subsystem<FGDNSClient>();

    ////////////////////////////////////////////////////////////////////
    // Initialize the flight model subsystem.
    ////////////////////////////////////////////////////////////////////

    globals->add_subsystem("flight", new FDMShell, SGSubsystemMgr::FDM);

    ////////////////////////////////////////////////////////////////////
    // Initialize the weather subsystem.
    ////////////////////////////////////////////////////////////////////

    // Initialize the weather modeling subsystem
    globals->add_subsystem("environment", new FGEnvironmentMgr);
    globals->add_new_subsystem<Ephemeris>();
    
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
    globals->add_new_subsystem<FGRouteMgr>();

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
   // Initialize the ATC subsystem
    ////////////////////////////////////////////////////////////////////

    globals->add_new_subsystem<PerformanceDB>(SGSubsystemMgr::POST_FDM);
    globals->add_subsystem("ATC", new FGATCManager, SGSubsystemMgr::POST_FDM);

    ////////////////////////////////////////////////////////////////////
    // Initialize multiplayer subsystem
    ////////////////////////////////////////////////////////////////////

    globals->add_subsystem("mp", new FGMultiplayMgr, SGSubsystemMgr::POST_FDM);

#ifdef ENABLE_SWIFT
    ////////////////////////////////////////////////////////////////////
    // Initialize Swift subsystem
    ////////////////////////////////////////////////////////////////////

    globals->add_subsystem("swift", new SwiftConnection, SGSubsystemMgr::POST_FDM);
#endif

    ////////////////////////////////////////////////////////////////////
    // Initialise the AI Model Manager
    ////////////////////////////////////////////////////////////////////

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
    
    globals->add_new_subsystem<FGControls>(SGSubsystemMgr::GENERAL);

    ////////////////////////////////////////////////////////////////////
    // Initialize the input subsystem.
    ////////////////////////////////////////////////////////////////////

    globals->add_new_subsystem<FGInput>(SGSubsystemMgr::GENERAL);


    ////////////////////////////////////////////////////////////////////
    // Initialize the replay subsystem
    ////////////////////////////////////////////////////////////////////
    globals->add_new_subsystem<FGReplay>(SGSubsystemMgr::GENERAL);
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
    // very important this goes in the SOUND group, since IAXClient
    // depends on OpenAL, which is shutdown when the SOUND group
    // shutdown.
    // Sentry: FLIGHTGEAR-66

    ////////////////////////////////////////////////////////////////////
    globals->add_new_subsystem<FGCom>(SGSubsystemMgr::SOUND);
#endif

    {
      SGSubsystem * httpd = flightgear::http::FGHttpd::createInstance( fgGetNode(flightgear::http::PROPERTY_ROOT) );
      if( NULL != httpd ) 
        globals->add_subsystem("httpd", httpd  );
    }

    ////////////////////////////////////////////////////////////////////
    // Initialize the lighting subsystem.
    ////////////////////////////////////////////////////////////////////

    // ordering here is important : Nasal (via events), then models, then views
    if (!duringReset) {
        globals->add_subsystem("lighting", new FGLight, SGSubsystemMgr::DISPLAY);
        globals->add_subsystem("events", globals->get_event_mgr(), SGSubsystemMgr::DISPLAY);
        globals->add_subsystem("tides", new FGTide );
    }

    globals->add_new_subsystem<FGAircraftModel>(SGSubsystemMgr::DISPLAY);
    globals->add_new_subsystem<FGModelMgr>(SGSubsystemMgr::DISPLAY);

    globals->add_new_subsystem<FGViewMgr>(SGSubsystemMgr::DISPLAY);
}

void fgPostInitSubsystems()
{
    SGTimeStamp st;
    st.stamp();
  
    ////////////////////////////////////////////////////////////////////////
    // Initialize the Nasal interpreter.
    // Do this last, so that the loaded scripts see initialized state
    ////////////////////////////////////////////////////////////////////////
    globals->add_new_subsystem<FGNasalSys>(SGSubsystemMgr::INIT);

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
}

// re-position is a simplified version of the traditional (legacy)
// reset procedure. We only need to poke systems which will be upset by
// a sudden change in aircraft position. Since this potentially includes
// Nasal, we trigger the 'reinit' signal.
void fgStartReposition()
{
  SGPropertyNode *master_freeze = fgGetNode("/sim/freeze/master");
  SG_LOG( SG_GENERAL, SG_INFO, "fgStartReposition()");
  
  flightgear::addSentryBreadcrumb("start reposition", "info");

  // ensure we are frozen
  bool freeze = master_freeze->getBoolValue();
  if ( !freeze ) {
    master_freeze->setBoolValue(true);
  }
  
  // set this signal so Nasal scripts can take action.
  fgSetBool("/sim/signals/reinit", true);
  fgSetBool("/sim/crashed", false);
  
  FDMShell* fdm = globals->get_subsystem<FDMShell>();
  fdm->unbind();
  
  // update our position based on current presets
  // this will mark position as needed finalized which we'll do in the
  // main-loop
  flightgear::initPosition();
  
  auto terraSync = globals->get_subsystem<simgear::SGTerraSync>();
  if (terraSync) {
    terraSync->reposition();
  }
  
  // Initialize the FDM
  globals->get_subsystem<FDMShell>()->reinit();
  
  // reset replay buffers
  globals->get_subsystem<FGReplay>()->reinit();
  
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
  
    // needed for parking assignment to work after reposition
    auto atcManager = globals->get_subsystem<FGATCManager>();
    if (atcManager) {
        atcManager->reposition();
    }

  // need to bind FDMshell again
  fdm->bind();

  // need to reset aircraft (systems/instruments/autopilot)
  // so they can adapt to current environment
  globals->get_subsystem("systems")->reinit();
  globals->get_subsystem("instrumentation")->reinit();
  globals->get_subsystem("xml-autopilot")->reinit();
    
  // setup state to end re-init
  fgSetBool("/sim/signals/reinit", false);
  if ( !freeze ) {
    master_freeze->setBoolValue(false);
  }
  fgSetBool("/sim/sceneryloaded",false);
  flightgear::addSentryBreadcrumb("end of reposition", "info");
}

void fgStartNewReset()
{
    // save user settings now, so that USERARCIVE-d values changes since the
    // last init are recorded and hence re-loaded when we fgInitConfig down
    // later in this function. Otherwise all such settings are lost.
    globals->saveUserSettings();

    SGPropertyNode_ptr preserved(new SGPropertyNode);
  
    if (!copyPropertiesWithAttribute(globals->get_props(), preserved, SGPropertyNode::PRESERVE))
        SG_LOG(SG_GENERAL, SG_ALERT, "Error saving preserved state");
    
    fgSetBool("/sim/signals/reinit", true);
    fgSetBool("/sim/freeze/master", true);
    
    SGSubsystemMgr* subsystemManger = globals->get_subsystem_mgr();
    // Nasal is added in fgPostInit, ensure it's already shutdown
    // before other subsystems, so Nasal listeners don't fire during shutdown
    subsystemManger->remove("nasal");
    
    subsystemManger->shutdown();
    subsystemManger->unbind();
    
    // remove most subsystems, with a few exceptions.
    for (int g=0; g<SGSubsystemMgr::MAX_GROUPS; ++g) {
        SGSubsystemGroup* grp = subsystemManger->get_group(static_cast<SGSubsystemMgr::GroupType>(g));
        for (auto nm : grp->member_names()) {
            if ((nm == "time") || (nm == "terrasync") || (nm == "events")
                || (nm == "lighting") || (nm == FGScenery::staticSubsystemClassId()))
            {
                continue;
            }
            
            try {
                subsystemManger->remove(nm.c_str());
            } catch (std::exception& e) {
                SG_LOG(SG_GENERAL, SG_INFO, "caught " << e.what() << " << shutting down:" << nm);
            } catch (...) {
                SG_LOG(SG_GENERAL, SG_INFO, "caught generic exception shutting down:" << nm);
            }
            
            // don't delete here, dropping the ref should be sufficient
        }
    } // of top-level groups iteration

    // drop any references to AI objects with TACAN
    flightgear::NavDataCache::instance()->clearDynamicPositioneds();

    FGRenderer* render = globals->get_renderer();
    // needed or we crash in multi-threaded OSG mode
    render->getViewerBase()->stopThreading();
    
    // order is important here since tile-manager shutdown needs to
    // access the scenery object
    subsystemManger->remove(FGScenery::staticSubsystemClassId());

    FGScenery::getPagerSingleton()->clearRequests();
    flightgear::CameraGroup::setDefault(NULL);
    
    // don't cancel the pager until after shutdown, since AIModels (and
    // potentially others) can queue delete requests on the pager.
    render->getView()->getDatabasePager()->cancel();
    render->getView()->getDatabasePager()->clear();
    
    osgDB::Registry::instance()->clearObjectCache();
    // Pager requests depend on this, so don't clear it until now
    sgUserDataInit( NULL );

    // preserve the event handler; re-creating it would entail fixing the
    // idle handler
    osg::ref_ptr<flightgear::FGEventHandler> eventHandler = render->getEventHandler();
    // tell the event handler to drop properties, etc
    eventHandler->clear();

    globals->set_renderer(NULL);
    globals->set_matlib(NULL);

    flightgear::unregisterMainLoopProperties();
    FGReplayData::resetStatisticsProperties();

    simgear::clearSharedTreeGeometry();
    simgear::clearEffectCache();
    simgear::SGModelLib::resetPropertyRoot();
    simgear::ParticlesGlobalManager::clear();
    simgear::UniformFactory::instance()->reset();    

    flightgear::addons::AddonManager::reset();

    globals->resetPropertyRoot();
    // otherwise channels are duplicated
    globals->get_channel_options_list()->clear();

    // IMPORTANT
    // this is the low-water mark of the reset process.
    // Subsystems (except the special ones), properties, OSG nodes, Effects
    // should all be gone at this, except for special things, such as the
    // splash node.
    // From here onwards we're recreating early parts of main/init, before
    // we restart the main loop.
    // This is the place to check that instances of classes have all be
    // cleaned up correctly. (Also, the OSG threads are all paused, we're back
    // in single threaded mode)
    /////////////////////

    flightgear::addons::AddonManager::createInstance();

    fgInitConfig(0, NULL, true);
    fgInitGeneral(); // all of this?
    
    // set out new property root on the command manager
    SGCommandMgr::instance()->setImplicitRoot(globals->get_props());
    
    flightgear::Options::sharedInstance()->processOptions();

    // Rebuild the lists of allowed paths for cases where a path comes from an
    // untrusted source, such as the global property tree (this uses $FG_HOME
    // and other paths set by Options::processOptions()).
    fgInitAllowedPaths();

    const auto& resMgr = simgear::EmbeddedResourceManager::instance();
    // The language was (re)set in processOptions()
    const string locale = globals->get_locale()->getPreferredLanguage();
    resMgr->selectLocale(locale);
    SG_LOG(SG_GENERAL, SG_INFO,
           "EmbeddedResourceManager: selected locale '" << locale << "'");

    // PRESERVED properties over-write state from options, intentionally
    if ( copyProperties(preserved, globals->get_props()) ) {
        SG_LOG( SG_GENERAL, SG_INFO, "Preserved state restored successfully" );
    } else {
        SG_LOG( SG_GENERAL, SG_INFO,
               "Some errors restoring preserved state (read-only props?)" );
    }

    fgGetNode("/sim")->removeChild("aircraft-dir");
    fgInitAircraftPaths(true);
    fgInitAircraft(true);
    
    render = new FGRenderer;
    render->setEventHandler(eventHandler);
    eventHandler->reset();
    globals->set_renderer(render);
    render->init();
    render->setView(viewer.get());

    sgUserDataInit( globals->get_props() );

    viewer->getDatabasePager()->setUpThreads(20, 1);
    
    // must do this before preinit for Rembrandthe
    flightgear::CameraGroup::buildDefaultGroup(viewer.get());
    render->preinit();
    viewer->startThreading();
    
    fgOSResetProperties();

// init some things manually
// which do not follow the regular init pattern
    
    globals->get_event_mgr()->init();
    globals->get_event_mgr()->setRealtimeProperty(fgGetNode("/sim/time/delta-realtime-sec", true));
    
    globals->set_matlib( new SGMaterialLib );
    
// terra-sync needs the property tree root, pass it back in
    auto terra_sync = subsystemManger->get_subsystem<simgear::SGTerraSync>();
    if (terra_sync) {
        terra_sync->setRoot(globals->get_props());
    }
    
    fgSetBool("/sim/signals/reinit", false);
    fgSetBool("/sim/freeze/master", false);
    fgSetBool("/sim/sceneryloaded",false);

    flightgear::addSentryBreadcrumb("end of reset", "info");
}

void fgInitPackageRoot()
{
    if (globals->packageRoot()) {
        return;
    }

    SGPath packageAircraftDir = flightgear::Options::sharedInstance()->valueForOption("download-dir");
    if (packageAircraftDir.isNull()) {
        packageAircraftDir = flightgear::defaultDownloadDir();
    }

    packageAircraftDir.append("Aircraft");

    SG_LOG(SG_GENERAL, SG_INFO, "init package root at:" << packageAircraftDir);


    SGSharedPtr<Root> pkgRoot(new Root(packageAircraftDir, FLIGHTGEAR_VERSION));
    // set the http client later (too early in startup right now)
    globals->setPackageRoot(pkgRoot);

}

int fgUninstall()
{
    SGPath dataPath = SGPath::fromEnv("FG_HOME", platformDefaultDataPath());
    simgear::Dir fgHome(dataPath);
    if (fgHome.exists()) {
        if (!fgHome.remove(true /* recursive */)) {
            fprintf(stderr, "Errors occurred trying to remove FG_HOME");
            return EXIT_FAILURE;
        }
    }

    if (fgHome.exists()) {
        fprintf(stderr, "unable to remove FG_HOME");
        return EXIT_FAILURE;
    }

#if defined(SG_WINDOWS)
    SGPath p = flightgear::defaultDownloadDir();
    // we don't want to remove the whole dir, let's nuke specific
    // subdirs which are (hopefully) safe

    SGPath terrasyncPath = p / "TerraSync";
    if (terrasyncPath.exists()) {
        simgear::Dir dir(terrasyncPath);
        if (!dir.remove(true /*recursive*/)) {
            fprintf(stderr, "Errors occurred trying to remove Documents/FlightGear/TerraSync");
        }
    }

    SGPath packagesPath = p / "Aircraft";
    if (packagesPath.exists()) {
        simgear::Dir dir(packagesPath);
        if (!dir.remove(true /*recursive*/)) {
            fprintf(stderr, "Errors occurred trying to remove Documents/FlightGear/Aircraft");
        }
    }
#endif
    return EXIT_SUCCESS;
}
