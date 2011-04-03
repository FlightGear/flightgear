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

#if defined( unix ) || defined( __CYGWIN__ )
#  include <unistd.h>           // for gethostname()
#endif
#ifdef _WIN32
#  include <direct.h>           // for getcwd()
#  define getcwd _getcwd
#  include <io.h>               // isatty()
#  define isatty _isatty
#  include "winsock2.h"		// for gethostname()
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
#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/sg_dir.hxx>
#include <simgear/misc/sgstream.hxx>

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

#include <ATCDCL/ATCmgr.hxx>
#include <ATCDCL/commlist.hxx>
#include <ATC/atis_mgr.hxx>
#include <ATC/atc_mgr.hxx>

#include <Autopilot/route_mgr.hxx>
#include <Autopilot/autopilotgroup.hxx>

#include <Cockpit/panel.hxx>
#include <Cockpit/panel_io.hxx>

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

#include <Environment/environment_mgr.hxx>

#include "fg_init.hxx"
#include "fg_io.hxx"
#include "fg_commands.hxx"
#include "fg_props.hxx"
#include "options.hxx"
#include "globals.hxx"
#include "logger.hxx"
#include "renderer.hxx"
#include "viewmgr.hxx"
#include "main.hxx"


#ifdef __APPLE__
#  include <CoreFoundation/CoreFoundation.h>
#endif

using std::string;
using namespace boost::algorithm;

extern const char *default_root;


// Scan the command line options for the specified option and return
// the value.
static string fgScanForOption( const string& option, int argc, char **argv ) {
    int i = 1;

    if (hostname == NULL)
    {
        char _hostname[256];
        if( gethostname(_hostname, 256) >= 0 ) {
            hostname = strdup(_hostname);
            free_hostname = true;
        }
    }

    SG_LOG(SG_GENERAL, SG_INFO, "Scanning command line for: " << option );

    int len = option.length();

    while ( i < argc ) {
	SG_LOG( SG_GENERAL, SG_DEBUG, "argv[" << i << "] = " << argv[i] );

	string arg = argv[i];
	if ( arg.find( option ) == 0 ) {
	    return arg.substr( len );
	}

	i++;
    }

    return "";
}


// Scan the user config files for the specified option and return
// the value.
static string fgScanForOption( const string& option, const string& path ) {
    sg_gzifstream in( path );
    if ( !in.is_open() ) {
        return "";
    }

    SG_LOG( SG_GENERAL, SG_INFO, "Scanning " << path << " for: " << option );

    int len = option.length();

    in >> skipcomment;
    while ( ! in.eof() ) {
	string line;
	getline( in, line, '\n' );

        // catch extraneous (DOS) line ending character
        if ( line[line.length() - 1] < 32 ) {
            line = line.substr( 0, line.length()-1 );
        }

	if ( line.find( option ) == 0 ) {
	    return line.substr( len );
	}

	in >> skipcomment;
    }

    return "";
}

// Scan the user config files for the specified option and return
// the value.
static string fgScanForOption( const string& option ) {
    string arg("");

#if defined( unix ) || defined( __CYGWIN__ ) || defined(_MSC_VER)
    // Next check home directory for .fgfsrc.hostname file
    if ( arg.empty() ) {
        if ( homedir != NULL && hostname != NULL && strlen(hostname) > 0) {
            SGPath config( homedir );
            config.append( ".fgfsrc" );
            config.concat( "." );
            config.concat( hostname );
            arg = fgScanForOption( option, config.str() );
        }
    }
#endif

    // Next check home directory for .fgfsrc file
    if ( arg.empty() ) {
        if ( homedir != NULL ) {
            SGPath config( homedir );
            config.append( ".fgfsrc" );
            arg = fgScanForOption( option, config.str() );
        }
    }

    if ( arg.empty() ) {
        // Check for $fg_root/system.fgfsrc
        SGPath config( globals->get_fg_root() );
        config.append( "system.fgfsrc" );
        arg = fgScanForOption( option, config.str() );
    }

    return arg;
}


// Read in configuration (files and command line options) but only set
// fg_root and aircraft_paths, which are needed *before* do_options() is called
// in fgInitConfig

bool fgInitFGRoot ( int argc, char **argv ) {
    string root;

    // First parse command line options looking for --fg-root=, this
    // will override anything specified in a config file
    root = fgScanForOption( "--fg-root=", argc, argv);

    // Check in one of the user configuration files.
    if (root.empty() )
        root = fgScanForOption( "--fg-root=" );
    
    // Next check if fg-root is set as an env variable
    if ( root.empty() ) {
        char *envp = ::getenv( "FG_ROOT" );
        if ( envp != NULL ) {
            root = envp;
        }
    }

    // Otherwise, default to a random compiled-in location if we can't
    // find fg-root any other way.
    if ( root.empty() ) {
#if defined( __CYGWIN__ )
        root = "../data";
#elif defined( _WIN32 )
        root = "..\\data";
#elif defined(__APPLE__) 
        /*
        The following code looks for the base package inside the application 
        bundle, in the standard Contents/Resources location. 
        */
        CFURLRef resourcesUrl = CFBundleCopyResourcesDirectoryURL(CFBundleGetMainBundle());

        // look for a 'data' subdir
        CFURLRef dataDir = CFURLCreateCopyAppendingPathComponent(NULL, resourcesUrl, CFSTR("data"), true);

        // now convert down to a path, and the a c-string
        CFStringRef path = CFURLCopyFileSystemPath(dataDir, kCFURLPOSIXPathStyle);
        root = CFStringGetCStringPtr(path, CFStringGetSystemEncoding());

        CFRelease(resourcesUrl);
        CFRelease(dataDir);
        CFRelease(path);
#else
        root = PKGLIBDIR;
#endif
    }

    SG_LOG(SG_INPUT, SG_INFO, "fg_root = " << root );
    globals->set_fg_root(root);
    
    return true;
}


// Read in configuration (files and command line options) but only set
// aircraft
bool fgInitFGAircraft ( int argc, char **argv ) {
    
    string aircraftDir = fgScanForOption("--fg-aircraft=", argc, argv);
    if (aircraftDir.empty()) {
      aircraftDir =  fgScanForOption("--fg-aircraft="); 
    }

    const char* envp = ::getenv("FG_AIRCRAFT");
    if (aircraftDir.empty() && envp) {
      globals->append_aircraft_paths(envp);
    }
    
    if (!aircraftDir.empty()) {
      globals->append_aircraft_paths(aircraftDir);
    }
    
    string aircraft;

    // First parse command line options looking for --aircraft=, this
    // will override anything specified in a config file
    aircraft = fgScanForOption( "--aircraft=", argc, argv );
    if ( aircraft.empty() ) {
        // check synonym option
        aircraft = fgScanForOption( "--vehicle=", argc, argv );
    }

    // Check in one of the user configuration files.
    if ( aircraft.empty() ) {
        aircraft = fgScanForOption( "--aircraft=" );
    }
    if ( aircraft.empty() ) {
        aircraft = fgScanForOption( "--vehicle=" );
    }

    // if an aircraft was specified, set the property name
    if ( !aircraft.empty() ) {
        SG_LOG(SG_INPUT, SG_INFO, "aircraft = " << aircraft );
        fgSetString("/sim/aircraft", aircraft.c_str() );
    } else {
        SG_LOG(SG_INPUT, SG_INFO, "No user specified aircraft, using default" );
    }

    return true;
}


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


// Initialize the localization
SGPropertyNode *fgInitLocale(const char *language) {
   SGPropertyNode *c_node = NULL, *d_node = NULL;
   SGPropertyNode *intl = fgGetNode("/sim/intl");

   SG_LOG(SG_GENERAL, SG_INFO, "Selecting language: " << language );

   // localization not defined
   if (!intl)
      return NULL;

   //
   // Select the proper language from the list
   //
   vector<SGPropertyNode_ptr> locale = intl->getChildren("locale");
   for (unsigned int i = 0; i < locale.size(); i++) {

      vector<SGPropertyNode_ptr> lang = locale[i]->getChildren("lang");
      for (unsigned int j = 0; j < lang.size(); j++) {

         if (!strcmp(lang[j]->getStringValue(), language)) {
            c_node = locale[i];
            break;
         }
      }
   }


   // Get the defaults
   d_node = intl->getChild("locale");
   if (!c_node)
      c_node = d_node;

   // Check for localized font
   SGPropertyNode *font_n = c_node->getNode("font", true);
   if ( !strcmp(font_n->getStringValue(), "") )
      font_n->setStringValue(d_node->getStringValue("font", "typewriter.txf"));


   //
   // Load the default strings
   //
   SGPath d_path( globals->get_fg_root() );

   const char *d_path_str = d_node->getStringValue("strings");
   if (!d_path_str) {
      SG_LOG(SG_GENERAL, SG_ALERT, "No path in " << d_node->getPath() << "/strings.");
      return NULL;
   }

   d_path.append(d_path_str);
   SG_LOG(SG_GENERAL, SG_INFO, "Reading localized strings from " << d_path.str());

   SGPropertyNode *strings = c_node->getNode("strings");
   try {
      readProperties(d_path.str(), strings);
   } catch (const sg_exception &) {
      SG_LOG(SG_GENERAL, SG_ALERT, "Unable to read the localized strings");
      return NULL;
   }

   //
   // Load the language specific strings
   //
   if (c_node != d_node) {
      SGPath c_path( globals->get_fg_root() );

      const char *c_path_str = c_node->getStringValue("strings");
      if (!c_path_str) {
         SG_LOG(SG_GENERAL, SG_ALERT, "No path in " << c_node->getPath() << "/strings");
         return NULL;
      }

      c_path.append(c_path_str);
      SG_LOG(SG_GENERAL, SG_INFO, "Reading localized strings from " << c_path.str());

      try {
         readProperties(c_path.str(), strings);
      } catch (const sg_exception &) {
         SG_LOG(SG_GENERAL, SG_ALERT,
                 "Unable to read the localized strings from " << c_path.str());
         return NULL;
      }
   }

   return c_node;
}



// Initialize the localization routines
bool fgDetectLanguage() {
    const char *language = ::getenv("LANG");

    if (language == NULL) {
        SG_LOG(SG_GENERAL, SG_INFO, "Unable to detect the language" );
        language = "C";
    }

    SGPropertyNode *locale = fgInitLocale(language);
    if (!locale) {
       SG_LOG(SG_GENERAL, SG_ALERT,
              "No internationalization settings specified in preferences.xml" );

       return false;
    }

    globals->set_locale( locale );

    return true;
}

// Attempt to locate and parse the various non-XML config files in order
// from least precidence to greatest precidence
static void
do_options (int argc, char ** argv)
{
    // Check for $fg_root/system.fgfsrc
    SGPath config( globals->get_fg_root() );
    config.append( "system.fgfsrc" );
    fgParseOptions(config.str());

#if defined( unix ) || defined( __CYGWIN__ ) || defined(_MSC_VER)
    if( hostname != NULL && strlen(hostname) > 0 ) {
        config.concat( "." );
        config.concat( hostname );
        fgParseOptions(config.str());
    }
#endif

    // Check for ~/.fgfsrc
    if ( homedir != NULL ) {
        config.set( homedir );
        config.append( ".fgfsrc" );
        fgParseOptions(config.str());
    }

#if defined( unix ) || defined( __CYGWIN__ ) || defined(_MSC_VER)
    if( hostname != NULL && strlen(hostname) > 0 ) {
        // Check for ~/.fgfsrc.hostname
        config.concat( "." );
        config.concat( hostname );
        fgParseOptions(config.str());
    }
#endif

    // Parse remaining command line options
    // These will override anything specified in a config file
    fgParseArgs(argc, argv);
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

// Read in configuration (file and command line)
bool fgInitConfig ( int argc, char **argv ) {

    // First, set some sane default values
    fgSetDefaults();

    // Read global preferences from $FG_ROOT/preferences.xml
    SG_LOG(SG_INPUT, SG_INFO, "Reading global preferences");
    fgLoadProps("preferences.xml", globals->get_props());
    SG_LOG(SG_INPUT, SG_INFO, "Finished Reading global preferences");

    // Detect the required language as early as possible
    if ( !fgDetectLanguage() ) {
        return false;
    }

    SGPropertyNode autosave;
#ifdef _WIN32
    char *envp = ::getenv( "APPDATA" );
    if (envp != NULL ) {
        SGPath config( envp );
        config.append( "flightgear.org" );
#else
    if ( homedir != NULL ) {
        SGPath config( homedir );
        config.append( ".fgfs" );
#endif
        const char *fg_home = getenv("FG_HOME");
        if (fg_home)
            config = fg_home;

        SGPath home_export(config.str());
        home_export.append("Export/dummy");
        home_export.create_dir(0777);

        // Set /sim/fg-home and don't allow malign code to override it until
        // Nasal security is set up.  Use FG_HOME if necessary.
        SGPropertyNode *home = fgGetNode("/sim", true);
        home->removeChild("fg-home", 0, false);
        home = home->getChild("fg-home", 0, true);
        home->setStringValue(config.c_str());
        home->setAttribute(SGPropertyNode::WRITE, false);

        config.append( "autosave.xml" );
        if (config.exists()) {
          SG_LOG(SG_INPUT, SG_INFO, "Reading user settings from " << config.str());
          try {
              readProperties(config.str(), &autosave, SGPropertyNode::USERARCHIVE);
          } catch (sg_exception& e) {
              SG_LOG(SG_INPUT, SG_WARN, "failed to read user settings:" << e.getMessage()
                << "(from " << e.getOrigin() << ")");
          }
        }
    }
    
    // Scan user config files and command line for a specified aircraft.
    fgInitFGAircraft(argc, argv);
    FindAndCacheAircraft f(&autosave);
    if (!f.loadAircraft()) {
      return false;
    }

    copyProperties(&autosave, globals->get_props());

    // parse options after loading aircraft to ensure any user
    // overrides of defaults are honored.
    do_options(argc, argv);

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
        const string id(node->getStringValue());
        fgSetTowerPosFromAirportID(id);
    }
};

void fgInitTowerLocationListener() {
    fgGetNode("/sim/tower/airport-id",  true)
        ->addChangeListener( new FGTowerLocationListener(), true );
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
                "Failed to find parking position " << parkpos <<
                " at airport " << id );
        return false;
    }
    
    int park_index = dcs->getNrOfParkings() - 1;
    while (park_index >= 0 && dcs->getParkingName(park_index) != parkpos) park_index--;
    if (park_index < 0) {
        SG_LOG( SG_GENERAL, SG_ALERT,
                "Failed to find parking position " << parkpos <<
                " at airport " << id );
        return false;
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
static bool fgSetPosFromNAV( const string& id, const double& freq ) {
    FGNavRecord *nav
        = globals->get_navlist()->findByIdentAndFreq( id.c_str(), freq );

  if (!nav) {
    SG_LOG( SG_GENERAL, SG_ALERT, "Failed to locate NAV = "
                << id << ":" << freq );
    return false;
  }
  
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

// Initialise the frequency search map BEFORE reading
// the airport database:



    current_commlist = new FGCommList;
    current_commlist->init( globals->get_fg_root() );
    fgAirportDBLoad( aptdb.str(), current_commlist, p_metar.str() );

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
            fgSetString("/sim/tower/airport-id",  apt.c_str());
            set_pos = true;
        }
    }

    if ( !set_pos && !apt.empty() && !rwy_no.empty() ) {
        // An airport + runway is requested
        if ( fgSetPosFromAirportIDandRwy( apt, rwy_no, rwy_req ) ) {
            // set tower position (a little off the heading for single
            // runway airports)
	    fgSetString("/sim/tower/airport-id",  apt.c_str());
            set_pos = true;
        }
    }

    if ( !set_pos && !apt.empty() ) {
        // An airport is requested (find runway closest to hdg)
        if ( fgSetPosFromAirportIDandHdg( apt, hdg ) ) {
            // set tower position (a little off the heading for single
            // runway airports)
	    fgSetString("/sim/tower/airport-id",  apt.c_str());
            set_pos = true;
        }
    }

    if (hdg_preset->getDoubleValue() > 9990.0)
        hdg_preset->setDoubleValue(hdg);

    if ( !set_pos && !vor.empty() ) {
        // a VOR is requested
        if ( fgSetPosFromNAV( vor, vor_freq ) ) {
            set_pos = true;
        }
    }

    if ( !set_pos && !ndb.empty() ) {
        // an NDB is requested
        if ( fgSetPosFromNAV( ndb, ndb_freq ) ) {
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

    char buf[512], *cwd = getcwd(buf, 511);
    buf[511] = '\0';
    SGPropertyNode *curr = fgGetNode("/sim", true);
    curr->removeChild("fg-current", 0, false);
    curr = curr->getChild("fg-current", 0, true);
    curr->setStringValue(cwd ? cwd : "");
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
    // static const SGPropertyNode *longitude
    //     = fgGetNode("/sim/presets/longitude-deg");
    // static const SGPropertyNode *latitude
    //     = fgGetNode("/sim/presets/latitude-deg");
    // static const SGPropertyNode *altitude
    //     = fgGetNode("/sim/presets/altitude-ft");

    SG_LOG( SG_GENERAL, SG_INFO, "Initialize Subsystems");
    SG_LOG( SG_GENERAL, SG_INFO, "========== ==========");

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
    // Initialize the material property subsystem.
    ////////////////////////////////////////////////////////////////////

    SGPath mpath( globals->get_fg_root() );
    mpath.append( "materials.xml" );
    if ( ! globals->get_matlib()->load(globals->get_fg_root(), mpath.str(),
            globals->get_props()) ) {
        SG_LOG( SG_GENERAL, SG_ALERT, "Error loading material lib!" );
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

    ////////////////////////////////////////////////////////////////////
    // Initialize the aircraft systems and instrumentation (before the
    // autopilot.)
    ////////////////////////////////////////////////////////////////////

    globals->add_subsystem("instrumentation", new FGInstrumentMgr, SGSubsystemMgr::FDM);
    globals->add_subsystem("systems", new FGSystemMgr, SGSubsystemMgr::FDM);

    ////////////////////////////////////////////////////////////////////
    // Initialize the XML Autopilot subsystem.
    ////////////////////////////////////////////////////////////////////

    globals->add_subsystem( "xml-autopilot", FGXMLAutopilotGroup::createInstance(), SGSubsystemMgr::FDM );
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
    // Initialise the ATC Manager
    // Note that this is old stuff, but might be necessesary for the 
    // current ATIS implementation. Therefore, leave it in here
    // until the ATIS system is ported over to make use of the ATIS 
    // sub system infrastructure.
    ////////////////////////////////////////////////////////////////////

    SG_LOG(SG_GENERAL, SG_INFO, "  ATC Manager");
    globals->set_ATC_mgr(new FGATCMgr);
    globals->get_ATC_mgr()->init(); 

    ////////////////////////////////////////////////////////////////////
   // Initialize the ATC subsystem
    ////////////////////////////////////////////////////////////////////
    globals->add_subsystem("ATC", new FGATCManager, SGSubsystemMgr::POST_FDM);
    ////////////////////////////////////////////////////////////////////
    // Initialise the ATIS Subsystem
    ////////////////////////////////////////////////////////////////////
    globals->add_subsystem("atis", new FGAtisManager, SGSubsystemMgr::POST_FDM);


    ////////////////////////////////////////////////////////////////////
    // Initialize multiplayer subsystem
    ////////////////////////////////////////////////////////////////////

    globals->add_subsystem("mp", new FGMultiplayMgr, SGSubsystemMgr::POST_FDM);

    ////////////////////////////////////////////////////////////////////
    // Initialise the AI Model Manager
    ////////////////////////////////////////////////////////////////////
    SG_LOG(SG_GENERAL, SG_INFO, "  AI Model Manager");
    globals->add_subsystem("ai_model", new FGAIManager, SGSubsystemMgr::POST_FDM);
    globals->add_subsystem("submodel_mgr", new FGSubmodelMgr, SGSubsystemMgr::POST_FDM);


    // It's probably a good idea to initialize the top level traffic manager
    // After the AI and ATC systems have been initialized properly.
    // AI Traffic manager
    globals->add_subsystem("Traffic Manager", new FGTrafficManager, SGSubsystemMgr::POST_FDM);

    ////////////////////////////////////////////////////////////////////
    // Add a new 2D panel.
    ////////////////////////////////////////////////////////////////////

    string panel_path(fgGetString("/sim/panel/path"));
    if (!panel_path.empty()) {
      FGPanel* p = fgReadPanel(panel_path);
      if (p) {
        globals->set_current_panel(p);
        p->init();
        p->bind();
        SG_LOG( SG_INPUT, SG_INFO, "Loaded new panel from " << panel_path );
      } else {
        SG_LOG( SG_INPUT, SG_ALERT,
                "Error reading new panel from " << panel_path );
      }
    }

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

    ////////////////////////////////////////////////////////////////////////
    // End of subsystem initialization.
    ////////////////////////////////////////////////////////////////////

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
    globals->get_subsystem("ai_model")->reinit();

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


