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

// work around a stdc++ lib bug in some versions of linux, but doesn't
// seem to hurt to have this here for all versions of Linux.
#ifdef linux
#  define _G_NO_EXTERN_TEMPLATES
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
#include <simgear/sound/soundmgr_openal.hxx>

#include <Aircraft/controls.hxx>
#include <Aircraft/replay.hxx>
#include <Airports/apt_loader.hxx>
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
#include <GUI/new_gui.hxx>
#include <Input/input.hxx>
#include <Instrumentation/instrument_mgr.hxx>
#include <Model/acmodel.hxx>
#include <Model/modelmgr.hxx>
#include <AIModel/submodel.hxx>
#include <AIModel/AIManager.hxx>
#include <Navaids/navdb.hxx>
#include <Navaids/navlist.hxx>
#include <Navaids/fix.hxx>
#include <Navaids/fixlist.hxx>
#include <Navaids/airways.hxx>
#include <Scenery/scenery.hxx>
#include <Scenery/tilemgr.hxx>
#include <Scripting/NasalSys.hxx>
#include <Sound/voice.hxx>
#include <Systems/system_mgr.hxx>
#include <Time/light.hxx>
#include <Traffic/TrafficMgr.hxx>
#include <MultiPlayer/multiplaymgr.hxx>
#include <FDM/fdm_shell.hxx>
#include <Environment/ephemeris.hxx>
#include <Environment/environment_mgr.hxx>
#include <Viewer/renderer.hxx>
#include <Viewer/viewmgr.hxx>

#include "fg_init.hxx"
#include "fg_io.hxx"
#include "fg_commands.hxx"
#include "fg_props.hxx"
#include "options.hxx"
#include "globals.hxx"
#include "logger.hxx"
#include "main.hxx"


using std::string;
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
  
    flightgear::Options::sharedInstance()->init(argc, argv, dataPath);
  
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

    // Scan user config files and command line for a specified aircraft.
    flightgear::Options::sharedInstance()->initAircraft();

    FindAndCacheAircraft f(globals->get_props());
    if (!f.loadAircraft()) {
      return false;
    }

    // parse options after loading aircraft to ensure any user
    // overrides of defaults are honored.
    flightgear::Options::sharedInstance()->processOptions();
      
    return true;
}

// Set current tower position lon/lat given an airport id
static bool fgSetTowerPosFromAirportID( const string& id) {
    const FGAirport *a = fgFindAirportID( id);
    if (a) {
        SGGeod tower = a->getTowerLocation();
        fgSetDouble("/sim/tower/longitude-deg",  tower.getLongitudeDeg());
        fgSetDouble("/sim/tower/latitude-deg",  tower.getLatitudeDeg());
        fgSetDouble("/sim/tower/altitude-ft", tower.getElevationFt());
        return true;
    } else {
        return false;
    }

}

struct FGTowerLocationListener : SGPropertyChangeListener {
    void valueChanged(SGPropertyNode* node) {
        string id(node->getStringValue());
        if (fgGetBool("/sim/tower/auto-position",true))
        {
            // enforce using closest airport when auto-positioning is enabled 
            const char* closest_airport = fgGetString("/sim/airport/closest-airport-id", "");
            if (closest_airport && (id != closest_airport))
            {
                id = closest_airport;
                node->setStringValue(id);
            }
        }
        fgSetTowerPosFromAirportID(id);
    }
};

struct FGClosestTowerLocationListener : SGPropertyChangeListener
{
    void valueChanged(SGPropertyNode* )
    {
        // closest airport has changed
        if (fgGetBool("/sim/tower/auto-position",true))
        {
            // update tower position
            const char* id = fgGetString("/sim/airport/closest-airport-id", "");
            if (id && *id!=0)
                fgSetString("/sim/tower/airport-id", id);
        }
    }
};

void fgInitTowerLocationListener() {
    fgGetNode("/sim/tower/airport-id",  true)
        ->addChangeListener( new FGTowerLocationListener(), true );
    FGClosestTowerLocationListener* ntcl = new FGClosestTowerLocationListener();
    fgGetNode("/sim/airport/closest-airport-id", true)
        ->addChangeListener(ntcl , true );
    fgGetNode("/sim/tower/auto-position", true)
           ->addChangeListener(ntcl, true );
}

static void fgApplyStartOffset(const SGGeod& aStartPos, double aHeading, double aTargetHeading = HUGE_VAL)
{
  SGGeod startPos(aStartPos);
  if (aTargetHeading == HUGE_VAL) {
    aTargetHeading = aHeading;
  }
  
  if ( fabs( fgGetDouble("/sim/presets/offset-distance-nm") ) > SG_EPSILON ) {
    double offsetDistance = fgGetDouble("/sim/presets/offset-distance-nm");
    offsetDistance *= SG_NM_TO_METER;
    double offsetAzimuth = aHeading;
    if ( fabs(fgGetDouble("/sim/presets/offset-azimuth-deg")) > SG_EPSILON ) {
      offsetAzimuth = fgGetDouble("/sim/presets/offset-azimuth-deg");
      aHeading = aTargetHeading;
    }

    SGGeod offset;
    double az2; // dummy
    SGGeodesy::direct(startPos, offsetAzimuth + 180, offsetDistance, offset, az2);
    startPos = offset;
  }

  // presets
  fgSetDouble("/sim/presets/longitude-deg", startPos.getLongitudeDeg() );
  fgSetDouble("/sim/presets/latitude-deg", startPos.getLatitudeDeg() );
  fgSetDouble("/sim/presets/heading-deg", aHeading );

  // other code depends on the actual values being set ...
  fgSetDouble("/position/longitude-deg",  startPos.getLongitudeDeg() );
  fgSetDouble("/position/latitude-deg",  startPos.getLatitudeDeg() );
  fgSetDouble("/orientation/heading-deg", aHeading );
}

// Set current_options lon/lat given an airport id and heading (degrees)
bool fgSetPosFromAirportIDandHdg( const string& id, double tgt_hdg ) {
    if ( id.empty() )
        return false;

    // set initial position from runway and heading
    SG_LOG( SG_GENERAL, SG_INFO,
            "Attempting to set starting position from airport code "
            << id << " heading " << tgt_hdg );

    const FGAirport* apt = fgFindAirportID(id);
    if (!apt) return false;
    FGRunway* r = apt->findBestRunwayForHeading(tgt_hdg);
    fgSetString("/sim/atc/runway", r->ident().c_str());

    SGGeod startPos = r->pointOnCenterline(fgGetDouble("/sim/airport/runways/start-offset-m", 5.0));
	  fgApplyStartOffset(startPos, r->headingDeg(), tgt_hdg);
    return true;
}

// Set current_options lon/lat given an airport id and parkig position name
static bool fgSetPosFromAirportIDandParkpos( const string& id, const string& parkpos ) {
    if ( id.empty() )
        return false;

    // can't see an easy way around this const_cast at the moment
    FGAirport* apt = const_cast<FGAirport*>(fgFindAirportID(id));
    if (!apt) {
        SG_LOG( SG_GENERAL, SG_ALERT, "Failed to find airport " << id );
        return false;
    }
    FGAirportDynamics* dcs = apt->getDynamics();
    if (!dcs) {
        SG_LOG( SG_GENERAL, SG_ALERT,
                "Airport " << id << "does not appear to have parking information available");
        return false;
    }
    
    int park_index = dcs->getNrOfParkings() - 1;
    bool succes;
    double radius = fgGetDouble("/sim/dimensions/radius-m");
    if ((parkpos == string("AVAILABLE")) && (radius > 0)) {
        double lat, lon, heading;
        string fltType;
        string acOperator;
        SGPath acData;
        try {
            acData = fgGetString("/sim/fg-home");
            acData.append("aircraft-data");
            string acfile = fgGetString("/sim/aircraft") + string(".xml");
            acData.append(acfile);
            SGPropertyNode root;
            readProperties(acData.str(), &root);
            SGPropertyNode * node = root.getNode("sim");
            fltType    = node->getStringValue("aircraft-class", "NONE"     );
            acOperator = node->getStringValue("aircraft-operator", "NONE"     );
        } catch (const sg_exception &) {
            SG_LOG(SG_GENERAL, SG_INFO,
                "Could not load aircraft aircrat type and operator information from: " << acData.str() << ". Using defaults");

       // cout << path.str() << endl;
        }
        if (fltType.empty() || fltType == "NONE") {
            SG_LOG(SG_GENERAL, SG_INFO,
                "Aircraft type information not found in: " << acData.str() << ". Using default value");
                fltType = fgGetString("/sim/aircraft-class"   );
        }
        if (acOperator.empty() || fltType == "NONE") {
            SG_LOG(SG_GENERAL, SG_INFO,
                "Aircraft operator information not found in: " << acData.str() << ". Using default value");
                acOperator = fgGetString("/sim/aircraft-operator"   );
        }

        cerr << "Running aircraft " << fltType << " of livery " << acOperator << endl;
        string acType; // Currently not used by findAvailable parking, so safe to leave empty. 
        succes = dcs->getAvailableParking(&lat, &lon, &heading, &park_index, radius, fltType, acType, acOperator);
        if (succes) {
            fgGetString("/sim/presets/parkpos");
            fgSetString("/sim/presets/parkpos", dcs->getParking(park_index)->getName());
        } else {
            SG_LOG( SG_GENERAL, SG_ALERT,
                    "Failed to find a suitable parking at airport " << id );
            return false;
        }
    } else {
        //cerr << "We shouldn't get here when AVAILABLE" << endl;
        while (park_index >= 0 && dcs->getParkingName(park_index) != parkpos) park_index--;
        if (park_index < 0) {
            SG_LOG( SG_GENERAL, SG_ALERT,
                    "Failed to find parking position " << parkpos <<
                    " at airport " << id );
            return false;
        }
    }
    FGParking* parking = dcs->getParking(park_index);
    parking->setAvailable(false);
    fgApplyStartOffset(
      SGGeod::fromDeg(parking->getLongitude(), parking->getLatitude()),
      parking->getHeading());
    return true;
}


// Set current_options lon/lat given an airport id and runway number
static bool fgSetPosFromAirportIDandRwy( const string& id, const string& rwy, bool rwy_req ) {
    if ( id.empty() )
        return false;

    // set initial position from airport and runway number
    SG_LOG( SG_GENERAL, SG_INFO,
            "Attempting to set starting position for "
            << id << ":" << rwy );

    const FGAirport* apt = fgFindAirportID(id);
    if (!apt) {
      SG_LOG( SG_GENERAL, SG_ALERT, "Failed to find airport:" << id);
      return false;
    }
    
    if (!apt->hasRunwayWithIdent(rwy)) {
      SG_LOG( SG_GENERAL, rwy_req ? SG_ALERT : SG_INFO,
                "Failed to find runway " << rwy <<
                " at airport " << id << ". Using default runway." );
      return false;
    }
    
    FGRunway* r(apt->getRunwayByIdent(rwy));
    fgSetString("/sim/atc/runway", r->ident().c_str());
    SGGeod startPos = r->pointOnCenterline( fgGetDouble("/sim/airport/runways/start-offset-m", 5.0));
	  fgApplyStartOffset(startPos, r->headingDeg());
    return true;
}


static void fgSetDistOrAltFromGlideSlope() {
    // cout << "fgSetDistOrAltFromGlideSlope()" << endl;
    string apt_id = fgGetString("/sim/presets/airport-id");
    double gs = fgGetDouble("/sim/presets/glideslope-deg")
        * SG_DEGREES_TO_RADIANS ;
    double od = fgGetDouble("/sim/presets/offset-distance-nm");
    double alt = fgGetDouble("/sim/presets/altitude-ft");

    double apt_elev = 0.0;
    if ( ! apt_id.empty() ) {
        apt_elev = fgGetAirportElev( apt_id );
        if ( apt_elev < -9990.0 ) {
            apt_elev = 0.0;
        }
    } else {
        apt_elev = 0.0;
    }

    if( fabs(gs) > 0.01 && fabs(od) > 0.1 && alt < -9990 ) {
        // set altitude from glideslope and offset-distance
        od *= SG_NM_TO_METER * SG_METER_TO_FEET;
        alt = fabs(od*tan(gs)) + apt_elev;
        fgSetDouble("/sim/presets/altitude-ft", alt);
        fgSetBool("/sim/presets/onground", false);
        SG_LOG( SG_GENERAL, SG_INFO, "Calculated altitude as: "
                << alt  << " ft" );
    } else if( fabs(gs) > 0.01 && alt > 0 && fabs(od) < 0.1) {
        // set offset-distance from glideslope and altitude
        od  = (alt - apt_elev) / tan(gs);
        od *= -1*SG_FEET_TO_METER * SG_METER_TO_NM;
        fgSetDouble("/sim/presets/offset-distance-nm", od);
        fgSetBool("/sim/presets/onground", false);
        SG_LOG( SG_GENERAL, SG_INFO, "Calculated offset distance as: " 
                << od  << " nm" );
    } else if( fabs(gs) > 0.01 ) {
        SG_LOG( SG_GENERAL, SG_ALERT,
                "Glideslope given but not altitude or offset-distance." );
        SG_LOG( SG_GENERAL, SG_ALERT, "Resetting glideslope to zero" );
        fgSetDouble("/sim/presets/glideslope-deg", 0);
        fgSetBool("/sim/presets/onground", true);
    }
}


// Set current_options lon/lat given an airport id and heading (degrees)
static bool fgSetPosFromNAV( const string& id, const double& freq, FGPositioned::Type type ) {

    const nav_list_type navlist
        = globals->get_navlist()->findByIdentAndFreq( id.c_str(), freq, type );

    if (navlist.size() == 0 ) {
        SG_LOG( SG_GENERAL, SG_ALERT, "Failed to locate NAV = "
            << id << ":" << freq );
        return false;
    }

    if( navlist.size() > 1 ) {
        ostringstream buf;
        buf << "Ambigous NAV-ID: '" << id << "'. Specify id and frequency. Available stations:" << endl;
        for( nav_list_type::const_iterator it = navlist.begin(); it != navlist.end(); ++it ) {
            // NDB stored in kHz, VOR stored in MHz * 100 :-P
            double factor = (*it)->type() == FGPositioned::NDB ? 1.0 : 1/100.0;
            string unit = (*it)->type() == FGPositioned::NDB ? "kHz" : "MHz";
            buf << (*it)->ident() << " "
                << setprecision(5) << (double)((*it)->get_freq() * factor) << " "
                << (*it)->get_lat() << "/" << (*it)->get_lon()
                << endl;
        }

        SG_LOG( SG_GENERAL, SG_ALERT, buf.str() );
        return false;
    }

    FGNavRecord *nav = navlist[0];
    fgApplyStartOffset(nav->geod(), fgGetDouble("/sim/presets/heading-deg"));
    return true;
}

// Set current_options lon/lat given an aircraft carrier id
static bool fgSetPosFromCarrier( const string& carrier, const string& posid ) {

    // set initial position from runway and heading
    SGGeod geodPos;
    double heading;
    SGVec3d uvw;
    if (FGAIManager::getStartPosition(carrier, posid, geodPos, heading, uvw)) {
        double lon = geodPos.getLongitudeDeg();
        double lat = geodPos.getLatitudeDeg();
        double alt = geodPos.getElevationFt();

        SG_LOG( SG_GENERAL, SG_INFO, "Attempting to set starting position for "
                << carrier << " at lat = " << lat << ", lon = " << lon
                << ", alt = " << alt << ", heading = " << heading);

        fgSetDouble("/sim/presets/longitude-deg",  lon);
        fgSetDouble("/sim/presets/latitude-deg",  lat);
        fgSetDouble("/sim/presets/altitude-ft", alt);
        fgSetDouble("/sim/presets/heading-deg", heading);
        fgSetDouble("/position/longitude-deg",  lon);
        fgSetDouble("/position/latitude-deg",  lat);
        fgSetDouble("/position/altitude-ft", alt);
        fgSetDouble("/orientation/heading-deg", heading);

        fgSetString("/sim/presets/speed-set", "UVW");
        fgSetDouble("/velocities/uBody-fps", uvw(0));
        fgSetDouble("/velocities/vBody-fps", uvw(1));
        fgSetDouble("/velocities/wBody-fps", uvw(2));
        fgSetDouble("/sim/presets/uBody-fps", uvw(0));
        fgSetDouble("/sim/presets/vBody-fps", uvw(1));
        fgSetDouble("/sim/presets/wBody-fps", uvw(2));

        fgSetBool("/sim/presets/onground", true);

        return true;
    } else {
        SG_LOG( SG_GENERAL, SG_ALERT, "Failed to locate aircraft carrier = "
                << carrier );
        return false;
    }
}
 
// Set current_options lon/lat given an airport id and heading (degrees)
static bool fgSetPosFromFix( const string& id )
{
  FGPositioned::TypeFilter fixFilter(FGPositioned::FIX);
  FGPositioned* fix = FGPositioned::findNextWithPartialId(NULL, id, &fixFilter);
  if (!fix) {
    SG_LOG( SG_GENERAL, SG_ALERT, "Failed to locate fix = " << id );
    return false;
  }
  
  fgApplyStartOffset(fix->geod(), fgGetDouble("/sim/presets/heading-deg"));
  return true;
}

/**
 * Initialize vor/ndb/ils/fix list management and query systems (as
 * well as simple airport db list)
 */
bool
fgInitNav ()
{
    SG_LOG(SG_GENERAL, SG_INFO, "Loading Airport Database ...");

    SGPath aptdb( globals->get_fg_root() );
    aptdb.append( "Airports/apt.dat" );

    SGPath p_metar( globals->get_fg_root() );
    p_metar.append( "Airports/metar.dat" );

    fgAirportDBLoad( aptdb.str(), p_metar.str() );
    
    FGNavList *navlist = new FGNavList;
    FGNavList *loclist = new FGNavList;
    FGNavList *gslist = new FGNavList;
    FGNavList *dmelist = new FGNavList;
    FGNavList *tacanlist = new FGNavList;
    FGNavList *carrierlist = new FGNavList;
    FGTACANList *channellist = new FGTACANList;

    globals->set_navlist( navlist );
    globals->set_loclist( loclist );
    globals->set_gslist( gslist );
    globals->set_dmelist( dmelist );
    globals->set_tacanlist( tacanlist );
    globals->set_carrierlist( carrierlist );
    globals->set_channellist( channellist );

    if ( !fgNavDBInit(navlist, loclist, gslist, dmelist, tacanlist, carrierlist, channellist) ) {
        SG_LOG( SG_GENERAL, SG_ALERT,
                "Problems loading one or more navigational database" );
    }
    
    SG_LOG(SG_GENERAL, SG_INFO, "  Fixes");
    SGPath p_fix( globals->get_fg_root() );
    p_fix.append( "Navaids/fix.dat" );
    FGFixList fixlist;
    fixlist.init( p_fix );  // adds fixes to the DB in positioned.cxx

    SG_LOG(SG_GENERAL, SG_INFO, "  Airways");
    flightgear::Airway::load();
    
    return true;
}


// Set the initial position based on presets (or defaults)
bool fgInitPosition() {
    // cout << "fgInitPosition()" << endl;
    double gs = fgGetDouble("/sim/presets/glideslope-deg")
        * SG_DEGREES_TO_RADIANS ;
    double od = fgGetDouble("/sim/presets/offset-distance-nm");
    double alt = fgGetDouble("/sim/presets/altitude-ft");

    bool set_pos = false;

    // If glideslope is specified, then calculate offset-distance or
    // altitude relative to glide slope if either of those was not
    // specified.
    if ( fabs( gs ) > 0.01 ) {
        fgSetDistOrAltFromGlideSlope();
    }


    // If we have an explicit, in-range lon/lat, don't change it, just use it.
    // If not, check for an airport-id and use that.
    // If not, default to the middle of the KSFO field.
    // The default values for lon/lat are deliberately out of range
    // so that the airport-id can take effect; valid lon/lat will
    // override airport-id, however.
    double lon_deg = fgGetDouble("/sim/presets/longitude-deg");
    double lat_deg = fgGetDouble("/sim/presets/latitude-deg");
    if ( lon_deg >= -180.0 && lon_deg <= 180.0
         && lat_deg >= -90.0 && lat_deg <= 90.0 )
    {
        set_pos = true;
    }

    string apt = fgGetString("/sim/presets/airport-id");
    string rwy_no = fgGetString("/sim/presets/runway");
    bool rwy_req = fgGetBool("/sim/presets/runway-requested");
    string vor = fgGetString("/sim/presets/vor-id");
    double vor_freq = fgGetDouble("/sim/presets/vor-freq");
    string ndb = fgGetString("/sim/presets/ndb-id");
    double ndb_freq = fgGetDouble("/sim/presets/ndb-freq");
    string carrier = fgGetString("/sim/presets/carrier");
    string parkpos = fgGetString("/sim/presets/parkpos");
    string fix = fgGetString("/sim/presets/fix");
    SGPropertyNode *hdg_preset = fgGetNode("/sim/presets/heading-deg", true);
    double hdg = hdg_preset->getDoubleValue();

    // save some start parameters, so that we can later say what the
    // user really requested. TODO generalize that and move it to options.cxx
    static bool start_options_saved = false;
    if (!start_options_saved) {
        start_options_saved = true;
        SGPropertyNode *opt = fgGetNode("/sim/startup/options", true);

        opt->setDoubleValue("latitude-deg", lat_deg);
        opt->setDoubleValue("longitude-deg", lon_deg);
        opt->setDoubleValue("heading-deg", hdg);
        opt->setStringValue("airport", apt.c_str());
        opt->setStringValue("runway", rwy_no.c_str());
    }

    if (hdg > 9990.0)
        hdg = fgGetDouble("/environment/config/boundary/entry/wind-from-heading-deg", 270);

    if ( !set_pos && !apt.empty() && !parkpos.empty() ) {
        // An airport + parking position is requested
        if ( fgSetPosFromAirportIDandParkpos( apt, parkpos ) ) {
            // set tower position
            fgSetString("/sim/airport/closest-airport-id",  apt.c_str());
            fgSetString("/sim/tower/airport-id",  apt.c_str());
            set_pos = true;
        }
    }

    if ( !set_pos && !apt.empty() && !rwy_no.empty() ) {
        // An airport + runway is requested
        if ( fgSetPosFromAirportIDandRwy( apt, rwy_no, rwy_req ) ) {
            // set tower position (a little off the heading for single
            // runway airports)
            fgSetString("/sim/airport/closest-airport-id",  apt.c_str());
            fgSetString("/sim/tower/airport-id",  apt.c_str());
            set_pos = true;
        }
    }

    if ( !set_pos && !apt.empty() ) {
        // An airport is requested (find runway closest to hdg)
        if ( fgSetPosFromAirportIDandHdg( apt, hdg ) ) {
            // set tower position (a little off the heading for single
            // runway airports)
            fgSetString("/sim/airport/closest-airport-id",  apt.c_str());
            fgSetString("/sim/tower/airport-id",  apt.c_str());
            set_pos = true;
        }
    }

    if (hdg_preset->getDoubleValue() > 9990.0)
        hdg_preset->setDoubleValue(hdg);

    if ( !set_pos && !vor.empty() ) {
        // a VOR is requested
        if ( fgSetPosFromNAV( vor, vor_freq, FGPositioned::VOR ) ) {
            set_pos = true;
        }
    }

    if ( !set_pos && !ndb.empty() ) {
        // an NDB is requested
        if ( fgSetPosFromNAV( ndb, ndb_freq, FGPositioned::NDB ) ) {
            set_pos = true;
        }
    }

    if ( !set_pos && !carrier.empty() ) {
        // an aircraft carrier is requested
        if ( fgSetPosFromCarrier( carrier, parkpos ) ) {
            set_pos = true;
        }
    }

    if ( !set_pos && !fix.empty() ) {
        // a Fix is requested
        if ( fgSetPosFromFix( fix ) ) {
            set_pos = true;
        }
    }

    if ( !set_pos ) {
        // No lon/lat specified, no airport specified, default to
        // middle of KSFO field.
        fgSetDouble("/sim/presets/longitude-deg", -122.374843);
        fgSetDouble("/sim/presets/latitude-deg", 37.619002);
    }

    fgSetDouble( "/position/longitude-deg",
                 fgGetDouble("/sim/presets/longitude-deg") );
    fgSetDouble( "/position/latitude-deg",
                 fgGetDouble("/sim/presets/latitude-deg") );
    fgSetDouble( "/orientation/heading-deg", hdg_preset->getDoubleValue());

    // determine if this should be an on-ground or in-air start
    if ((fabs(gs) > 0.01 || fabs(od) > 0.1 || alt > 0.1) && carrier.empty()) {
        fgSetBool("/sim/presets/onground", false);
    } else {
        fgSetBool("/sim/presets/onground", true);
    }

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

    globals->set_browser(fgGetString("/sim/startup/browser-app", "firefox %u"));

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
bool fgInitSubsystems() {

    SG_LOG( SG_GENERAL, SG_INFO, "Initialize Subsystems");
    SG_LOG( SG_GENERAL, SG_INFO, "========== ==========");

    ////////////////////////////////////////////////////////////////////
    // Initialize the sound subsystem.
    ////////////////////////////////////////////////////////////////////
    // Sound manager uses an own subsystem group "SOUND" which is the last
    // to be updated in every loop.
    // Sound manager is updated last so it can use the CPU while the GPU
    // is processing the scenery (doubled the frame-rate for me) -EMH-
    globals->add_subsystem("sound", new SGSoundMgr, SGSubsystemMgr::SOUND);

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

    globals->add_subsystem("instrumentation", new FGInstrumentMgr, SGSubsystemMgr::FDM);
    globals->add_subsystem("systems", new FGSystemMgr, SGSubsystemMgr::FDM);

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
    globals->add_subsystem("Canvas2D", new CanvasMgr, SGSubsystemMgr::DISPLAY);

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

    globals->get_controls()->init();
    globals->get_controls()->bind();


    ////////////////////////////////////////////////////////////////////
    // Initialize the input subsystem.
    ////////////////////////////////////////////////////////////////////

    globals->add_subsystem("input", new FGInput);


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
    
    FGAircraftModel* acm = new FGAircraftModel;
    globals->set_aircraft_model(acm);
    globals->add_subsystem("aircraft-model", acm, SGSubsystemMgr::DISPLAY);

    FGModelMgr* mm = new FGModelMgr;
    globals->set_model_mgr(mm);
    globals->add_subsystem("model-manager", mm, SGSubsystemMgr::DISPLAY);

    FGViewMgr *viewmgr = new FGViewMgr;
    globals->set_viewmgr( viewmgr );
    globals->add_subsystem("view-manager", viewmgr, SGSubsystemMgr::DISPLAY);

    globals->add_subsystem("tile-manager", globals->get_tile_mgr(), 
      SGSubsystemMgr::DISPLAY);
      
    ////////////////////////////////////////////////////////////////////
    // Bind and initialize subsystems.
    ////////////////////////////////////////////////////////////////////

    globals->get_subsystem_mgr()->bind();
    globals->get_subsystem_mgr()->init();

    ////////////////////////////////////////////////////////////////////////
    // Initialize the Nasal interpreter.
    // Do this last, so that the loaded scripts see initialized state
    ////////////////////////////////////////////////////////////////////////
    FGNasalSys* nasal = new FGNasalSys();
    globals->add_subsystem("nasal", nasal, SGSubsystemMgr::INIT);
    nasal->init();

    // initialize methods that depend on other subsystems.
    globals->get_subsystem_mgr()->postinit();

    ////////////////////////////////////////////////////////////////////
    // TODO FIXME! UGLY KLUDGE!
    ////////////////////////////////////////////////////////////////////
    {
        /* Scenarios require Nasal, so FGAIManager loads the scenarios,
         * including its models such as a/c carriers, in its 'postinit',
         * which is the very last thing we do.
         * fgInitPosition is called very early in main.cxx/fgIdleFunction,
         * one of the first things we do, long before scenarios/carriers are
         * loaded. => When requested "initial preset position" relates to a
         * carrier, recalculate the 'initial' position here (how have things
         * ever worked before this hack - this init sequence has always been
         * this way...?)*/
        std::string carrier = fgGetString("/sim/presets/carrier","");
        if (carrier != "")
        {
            // clear preset location and re-trigger position setup
            fgSetDouble("/sim/presets/longitude-deg", 9999);
            fgSetDouble("/sim/presets/latitude-deg", 9999);
            fgInitPosition();
        }
    }

    ////////////////////////////////////////////////////////////////////////
    // End of subsystem initialization.
    ////////////////////////////////////////////////////////////////////

    fgSetBool("/sim/crashed", false);
    fgSetBool("/sim/initialized", true);

    SG_LOG( SG_GENERAL, SG_INFO, endl);

                                // Save the initial state for future
                                // reference.
    globals->saveInitialState();
    
    return true;
}

// Reset: this is what the 'reset' command (and hence, GUI) is attached to
void fgReInitSubsystems()
{
    static const SGPropertyNode *master_freeze
        = fgGetNode("/sim/freeze/master");

    SG_LOG( SG_GENERAL, SG_INFO, "fgReInitSubsystems()");

// setup state to begin re-init
    bool freeze = master_freeze->getBoolValue();
    if ( !freeze ) {
        fgSetBool("/sim/freeze/master", true);
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
    fgInitPosition();
    
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

// setup state to end re-init
    fgSetBool("/sim/signals/reinit", false);
    if ( !freeze ) {
        fgSetBool("/sim/freeze/master", false);
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
    cin.get();
#endif
}


