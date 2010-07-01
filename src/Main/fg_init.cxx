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
#include <simgear/misc/interpolator.hxx>
#include <simgear/scene/material/matlib.hxx>
#include <simgear/scene/model/particles.hxx>
#include <simgear/sound/soundmgr_openal.hxx>
#include <simgear/timing/sg_time.hxx>
#include <simgear/timing/lowleveltime.h>

#include <Aircraft/controls.hxx>
#include <Aircraft/replay.hxx>
#include <Airports/apt_loader.hxx>
#include <Airports/runways.hxx>
#include <Airports/simple.hxx>
#include <Airports/dynamics.hxx>

#include <AIModel/AIManager.hxx>

#if ENABLE_ATCDCL
#   include <ATCDCL/ATCmgr.hxx>
#   include <ATCDCL/AIMgr.hxx>
#   include "ATCDCL/commlist.hxx"
#else
#   include "ATC/atcutils.hxx"
#endif

#include <Autopilot/route_mgr.hxx>
#include <Autopilot/autopilotgroup.hxx>

#include <Cockpit/cockpit.hxx>
#include <Cockpit/panel.hxx>
#include <Cockpit/panel_io.hxx>

#include <GUI/new_gui.hxx>
#include <Include/general.hxx>
#include <Input/input.hxx>
#include <Instrumentation/instrument_mgr.hxx>
#include <Model/acmodel.hxx>
#include <AIModel/submodel.hxx>
#include <AIModel/AIManager.hxx>
#include <Navaids/navdb.hxx>
#include <Navaids/navlist.hxx>
#include <Navaids/fix.hxx>
#include <Navaids/fixlist.hxx>
#include <Scenery/scenery.hxx>
#include <Scenery/tilemgr.hxx>
#include <Scripting/NasalSys.hxx>
#include <Sound/voice.hxx>
#include <Systems/system_mgr.hxx>
#include <Time/light.hxx>
#include <Time/sunsolver.hxx>
#include <Time/tmp.hxx>
#include <Traffic/TrafficMgr.hxx>
#include <MultiPlayer/multiplaymgr.hxx>
#include <FDM/fdm_shell.hxx>

#include <Environment/environment_mgr.hxx>
#include <Environment/ridge_lift.hxx>

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
        gethostname(_hostname, 256);
        hostname = strdup(_hostname);
        free_hostname = true;
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

#if defined( unix ) || defined( __CYGWIN__ )
    // Next check home directory for .fgfsrc.hostname file
    if ( arg.empty() ) {
        if ( homedir != NULL ) {
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
// fg_root
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

#if defined( unix ) || defined( __CYGWIN__ )
    config.concat( "." );
    config.concat( hostname );
    fgParseOptions(config.str());
#endif

    // Check for ~/.fgfsrc
    if ( homedir != NULL ) {
        config.set( homedir );
        config.append( ".fgfsrc" );
        fgParseOptions(config.str());
    }

#if defined( unix ) || defined( __CYGWIN__ )
    // Check for ~/.fgfsrc.hostname
    config.concat( "." );
    config.concat( hostname );
    fgParseOptions(config.str());
#endif

    // Parse remaining command line options
    // These will override anything specified in a config file
    fgParseArgs(argc, argv);
}


static string fgFindAircraftPath( const SGPath &path, const string &aircraft,
                                  SGPropertyNode *cache, int depth = 0 )
{
    const int MAXDEPTH = 1;

    ulDirEnt* dire;
    ulDir *dirp = ulOpenDir(path.str().c_str());
    if (dirp == NULL) {
        cerr << "Unable to open aircraft directory '" << path.str() << '\'' << endl;
        exit(-1);
    }

    string result;
    while ((dire = ulReadDir(dirp)) != NULL) {
        if (dire->d_isdir) {
            if (depth > MAXDEPTH)
                continue;

            if (!strcmp("CVS", dire->d_name) || !strcmp(".", dire->d_name)
                    || !strcmp("..", dire->d_name) || !strcmp("AI", dire->d_name))
                continue;

            SGPath next = path;
            next.append(dire->d_name);

            result = fgFindAircraftPath( next, aircraft, cache, depth + 1 );
            if ( ! result.empty() )
                break;

        } else {
            int len = strlen(dire->d_name);
            if (len < 9 || strcmp(dire->d_name + len - 8, "-set.xml"))
                continue;

            // create cache node
            int i = 0;
            while (1)
                if (!cache->getChild("aircraft", i++, false))
                    break;

            SGPropertyNode *n, *entry = cache->getChild("aircraft", --i, true);

            n = entry->getNode("file", true);
            n->setStringValue(dire->d_name);
            n->setAttribute(SGPropertyNode::USERARCHIVE, true);

            n = entry->getNode("path", true);
            n->setStringValue(path.str().c_str());
            n->setAttribute(SGPropertyNode::USERARCHIVE, true);

            if ( boost::equals(dire->d_name, aircraft.c_str(), is_iequal()) ) {
                result = path.str();
                break;
            }
        }
    }

    ulCloseDir(dirp);
    return result;
}


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
        SG_LOG(SG_INPUT, SG_INFO, "Reading user settings from " << config.str());
        try {
            readProperties(config.str(), &autosave, SGPropertyNode::USERARCHIVE);
        } catch (...) {
            SG_LOG(SG_INPUT, SG_DEBUG, "First time reading user settings");
        }
        SG_LOG(SG_INPUT, SG_DEBUG, "Finished Reading user settings");
    }
    SGPropertyNode *cache_root = autosave.getNode("sim/startup/path-cache", true);


    // Scan user config files and command line for a specified aircraft.
    fgInitFGAircraft(argc, argv);

    string aircraft = fgGetString( "/sim/aircraft", "" );
    if ( aircraft.size() > 0 ) {
        SGPath aircraft_search( globals->get_fg_root() );
        aircraft_search.append( "Aircraft" );

        string aircraft_set = aircraft + "-set.xml";
        string result;

        // check if the *-set.xml file is already in the cache
        if (globals->get_fg_root() == cache_root->getStringValue("fg-root", "")) {
            vector<SGPropertyNode_ptr> cache = cache_root->getChildren("aircraft");
            for (unsigned int i = 0; i < cache.size(); i++) {
                const char *name = cache[i]->getStringValue("file", "");
                if (boost::equals(aircraft_set, name, is_iequal())) {
                    const char *path = cache[i]->getStringValue("path", "");
                    SGPath xml(path);
                    xml.append(name);
                    if (xml.exists())
                        result = path;
                    break;
                }
            }
        }

        if (result.empty()) {
            // prepare cache for rescan
            SGPropertyNode *n = cache_root->getNode("fg-root", true);
            n->setStringValue(globals->get_fg_root().c_str());
            n->setAttribute(SGPropertyNode::USERARCHIVE, true);
            cache_root->removeChildren("aircraft");

            result = fgFindAircraftPath( aircraft_search, aircraft_set, cache_root );
        }

        if ( !result.empty() ) {
            fgSetString( "/sim/aircraft-dir", result.c_str() );
            SGPath full_name( result );
            full_name.append( aircraft_set );

            SG_LOG(SG_INPUT, SG_INFO, "Reading default aircraft: " << aircraft
                   << " from " << full_name.str());
            try {
                readProperties( full_name.str(), globals->get_props() );
            } catch ( const sg_exception &e ) {
                string message = "Error reading default aircraft: ";
                message += e.getFormattedMessage();
                SG_LOG(SG_INPUT, SG_ALERT, message);
                exit(2);
            }
        } else {
            SG_LOG( SG_INPUT, SG_ALERT, "Cannot find specified aircraft: "
                    << aircraft );
            return false;
        }

    } else {
        SG_LOG( SG_INPUT, SG_ALERT, "No default aircraft specified" );
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
static bool fgSetPosFromAirportIDandHdg( const string& id, double tgt_hdg ) {
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
    SGPath p_awy( globals->get_fg_root() );
    p_awy.append( "Navaids/awy.dat" );
    FGAirwayNetwork *awyNet = new FGAirwayNetwork;
    //cerr << "Loading Airways" << endl;
    awyNet->load (p_awy );
    awyNet->init();
    //cerr << "initializing airways" << endl;
    globals->set_airwaynet( awyNet );

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

    fgSetBool("/sim/startup/stdout-to-terminal", isatty(1));
    fgSetBool("/sim/startup/stderr-to-terminal", isatty(2));
    return true;
}

// Initialize view parameters
void fgInitView() {
  // force update of model so that viewer can get some data...
  globals->get_aircraft_model()->update(0);
  // run update for current view so that data is current...
  globals->get_viewmgr()->update(0);
}


SGTime *fgInitTime() {
    // Initialize time
    static const SGPropertyNode *longitude
        = fgGetNode("/position/longitude-deg");
    static const SGPropertyNode *latitude
        = fgGetNode("/position/latitude-deg");
    static const SGPropertyNode *cur_time_override
        = fgGetNode("/sim/time/cur-time-override", true);

    SGPath zone( globals->get_fg_root() );
    zone.append( "Timezone" );
    SGTime *t = new SGTime( longitude->getDoubleValue()
                              * SGD_DEGREES_TO_RADIANS,
                            latitude->getDoubleValue()
                              * SGD_DEGREES_TO_RADIANS,
                            zone.str(),
                            cur_time_override->getLongValue() );

    globals->set_warp_delta( 0 );

    t->update( 0.0, 0.0,
               cur_time_override->getLongValue(),
               globals->get_warp() );

    return t;
}


// set up a time offset (aka warp) if one is specified
void fgInitTimeOffset() {
    static const SGPropertyNode *longitude
        = fgGetNode("/position/longitude-deg");
    static const SGPropertyNode *latitude
        = fgGetNode("/position/latitude-deg");
    static const SGPropertyNode *cur_time_override
        = fgGetNode("/sim/time/cur-time-override", true);

    // Handle potential user specified time offsets
    int orig_warp = globals->get_warp();
    SGTime *t = globals->get_time_params();
    time_t cur_time = t->get_cur_time();
    time_t currGMT = sgTimeGetGMT( gmtime(&cur_time) );
    time_t systemLocalTime = sgTimeGetGMT( localtime(&cur_time) );
    time_t aircraftLocalTime = 
        sgTimeGetGMT( fgLocaltime(&cur_time, t->get_zonename() ) );
    
    // Okay, we now have several possible scenarios
    int offset = fgGetInt("/sim/startup/time-offset");
    string offset_type = fgGetString("/sim/startup/time-offset-type");

    int warp = 0;
    if ( offset_type == "real" ) {
        warp = 0;
    } else if ( offset_type == "dawn" ) {
        warp = fgTimeSecondsUntilSunAngle( cur_time,
                                           longitude->getDoubleValue()
                                             * SGD_DEGREES_TO_RADIANS,
                                           latitude->getDoubleValue()
                                             * SGD_DEGREES_TO_RADIANS,
                                           90.0, true ); 
    } else if ( offset_type == "morning" ) {
        warp = fgTimeSecondsUntilSunAngle( cur_time,
                                           longitude->getDoubleValue()
                                             * SGD_DEGREES_TO_RADIANS,
                                           latitude->getDoubleValue()
                                             * SGD_DEGREES_TO_RADIANS,
                                           75.0, true ); 
    } else if ( offset_type == "noon" ) {
        warp = fgTimeSecondsUntilSunAngle( cur_time,
                                           longitude->getDoubleValue()
                                             * SGD_DEGREES_TO_RADIANS,
                                           latitude->getDoubleValue()
                                             * SGD_DEGREES_TO_RADIANS,
                                           0.0, true ); 
    } else if ( offset_type == "afternoon" ) {
        warp = fgTimeSecondsUntilSunAngle( cur_time,
                                           longitude->getDoubleValue()
                                             * SGD_DEGREES_TO_RADIANS,
                                           latitude->getDoubleValue()
                                             * SGD_DEGREES_TO_RADIANS,
                                           60.0, false ); 
     } else if ( offset_type == "dusk" ) {
        warp = fgTimeSecondsUntilSunAngle( cur_time,
                                           longitude->getDoubleValue()
                                             * SGD_DEGREES_TO_RADIANS,
                                           latitude->getDoubleValue()
                                             * SGD_DEGREES_TO_RADIANS,
                                           90.0, false ); 
     } else if ( offset_type == "evening" ) {
        warp = fgTimeSecondsUntilSunAngle( cur_time,
                                           longitude->getDoubleValue()
                                             * SGD_DEGREES_TO_RADIANS,
                                           latitude->getDoubleValue()
                                             * SGD_DEGREES_TO_RADIANS,
                                           100.0, false ); 
    } else if ( offset_type == "midnight" ) {
        warp = fgTimeSecondsUntilSunAngle( cur_time,
                                           longitude->getDoubleValue()
                                             * SGD_DEGREES_TO_RADIANS,
                                           latitude->getDoubleValue()
                                             * SGD_DEGREES_TO_RADIANS,
                                           180.0, false ); 
    } else if ( offset_type == "system-offset" ) {
        warp = offset;
	orig_warp = 0;
    } else if ( offset_type == "gmt-offset" ) {
        warp = offset - (currGMT - systemLocalTime);
	orig_warp = 0;
    } else if ( offset_type == "latitude-offset" ) {
        warp = offset - (aircraftLocalTime - systemLocalTime);
	orig_warp = 0;
    } else if ( offset_type == "system" ) {
      warp = offset - (systemLocalTime - currGMT) - cur_time;
    } else if ( offset_type == "gmt" ) {
        warp = offset - cur_time;
    } else if ( offset_type == "latitude" ) {
        warp = offset - (aircraftLocalTime - currGMT)- cur_time; 
    } else {
        SG_LOG( SG_GENERAL, SG_ALERT,
                "FG_TIME::Unsupported offset type " << offset_type );
        exit( -1 );
    }
    globals->set_warp( orig_warp + warp );
    t->update( longitude->getDoubleValue() * SGD_DEGREES_TO_RADIANS,
               latitude->getDoubleValue() * SGD_DEGREES_TO_RADIANS,
               cur_time_override->getLongValue(),
               globals->get_warp() );

    SG_LOG( SG_GENERAL, SG_INFO, "After fgInitTimeOffset(): warp = " 
            << globals->get_warp() );
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

    if ( globals->get_tile_mgr()->init() ) {
        // Load the local scenery data
        double visibility_meters = fgGetDouble("/environment/visibility-m");
                
        globals->get_tile_mgr()->update( visibility_meters );
    } else {
        SG_LOG( SG_GENERAL, SG_ALERT, "Error in Tile Manager initialization!" );
        exit(-1);
    }

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
    // Initialize the ridge lift simulation.
    ////////////////////////////////////////////////////////////////////

    // Initialize the ridgelift subsystem
    globals->add_subsystem("ridgelift", new FGRidgeLift);

    ////////////////////////////////////////////////////////////////////
    // Initialize the aircraft systems and instrumentation (before the
    // autopilot.)
    ////////////////////////////////////////////////////////////////////

    globals->add_subsystem("instrumentation", new FGInstrumentMgr, SGSubsystemMgr::FDM);
    globals->add_subsystem("systems", new FGSystemMgr, SGSubsystemMgr::FDM);

    ////////////////////////////////////////////////////////////////////
    // Initialize the XML Autopilot subsystem.
    ////////////////////////////////////////////////////////////////////

    globals->add_subsystem( "xml-autopilot", new FGXMLAutopilotGroup, SGSubsystemMgr::FDM );
    globals->add_subsystem( "route-manager", new FGRouteMgr );
    
    ////////////////////////////////////////////////////////////////////
    // Initialize the view manager subsystem.
    ////////////////////////////////////////////////////////////////////

    fgInitView();

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


    ////////////////////////////////////////////////////////////////////
    // Initialize the local time subsystem.
    ////////////////////////////////////////////////////////////////////

    // update the current timezone each 30 minutes
    globals->get_event_mgr()->addTask( "fgUpdateLocalTime()",
                                       &fgUpdateLocalTime, 30*60 );
    fgInitTimeOffset();		// the environment subsystem needs this


    ////////////////////////////////////////////////////////////////////
    // Initialize the lighting subsystem.
    ////////////////////////////////////////////////////////////////////

    globals->add_subsystem("lighting", new FGLight);

    //////////////////////////////////////////////////////////////////////
    // Initialize the 2D cloud subsystem.
    ////////////////////////////////////////////////////////////////////
    fgGetBool("/sim/rendering/bump-mapping", false);

#ifdef ENABLE_AUDIO_SUPPORT
    ////////////////////////////////////////////////////////////////////
    // Initialize the sound-effects subsystem.
    ////////////////////////////////////////////////////////////////////
    globals->add_subsystem("voice", new FGVoiceMgr);
#endif

    ////////////////////////////////////////////////////////////////////
    // Initialise the ATC Manager 
    ////////////////////////////////////////////////////////////////////

#if ENABLE_ATCDCL
    SG_LOG(SG_GENERAL, SG_INFO, "  ATC Manager");
    globals->set_ATC_mgr(new FGATCMgr);
    globals->get_ATC_mgr()->init(); 

    ////////////////////////////////////////////////////////////////////
    // Initialise the AI Manager 
    ////////////////////////////////////////////////////////////////////

    SG_LOG(SG_GENERAL, SG_INFO, "  AI Manager");
    globals->set_AI_mgr(new FGAIMgr);
    globals->get_AI_mgr()->init();
#endif
    ////////////////////////////////////////////////////////////////////
    // Initialise the AI Model Manager
    ////////////////////////////////////////////////////////////////////
    SG_LOG(SG_GENERAL, SG_INFO, "  AI Model Manager");
    globals->add_subsystem("ai_model", new FGAIManager);
    globals->add_subsystem("submodel_mgr", new FGSubmodelMgr);


    // It's probably a good idea to initialize the top level traffic manager
    // After the AI and ATC systems have been initialized properly.
    // AI Traffic manager
    globals->add_subsystem("Traffic Manager", new FGTrafficManager);


    if( fgCockpitInit()) {
        // Cockpit initialized ok.
    } else {
        SG_LOG( SG_GENERAL, SG_ALERT, "Error in Cockpit initialization!" );
        exit(-1);
    }


    ////////////////////////////////////////////////////////////////////
    // Add a new 2D panel.
    ////////////////////////////////////////////////////////////////////

    string panel_path = fgGetString("/sim/panel/path",
                                    "Panels/Default/default.xml");

    globals->set_current_panel( fgReadPanel(panel_path) );
    if (globals->get_current_panel() == 0) {
        SG_LOG( SG_INPUT, SG_ALERT,
                "Error reading new panel from " << panel_path );
    } else {
        SG_LOG( SG_INPUT, SG_INFO, "Loaded new panel from " << panel_path );
        globals->get_current_panel()->init();
        globals->get_current_panel()->bind();
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


    ////////////////////////////////////////////////////////////////////
    // Bind and initialize subsystems.
    ////////////////////////////////////////////////////////////////////

    globals->get_subsystem_mgr()->bind();
    globals->get_subsystem_mgr()->init();

    ////////////////////////////////////////////////////////////////////
    // Initialize multiplayer subsystem
    ////////////////////////////////////////////////////////////////////

    globals->set_multiplayer_mgr(new FGMultiplayMgr);
    globals->get_multiplayer_mgr()->init();

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

    SG_LOG( SG_GENERAL, SG_INFO, endl);

                                // Save the initial state for future
                                // reference.
    globals->saveInitialState();
    
    return true;
}


void fgReInitSubsystems()
{
    // static const SGPropertyNode *longitude
    //     = fgGetNode("/sim/presets/longitude-deg");
    // static const SGPropertyNode *latitude
    //     = fgGetNode("/sim/presets/latitude-deg");
    static const SGPropertyNode *altitude
        = fgGetNode("/sim/presets/altitude-ft");
    static const SGPropertyNode *master_freeze
        = fgGetNode("/sim/freeze/master");

    SG_LOG( SG_GENERAL, SG_INFO,
            "fgReInitSubsystems(): /position/altitude = "
            << altitude->getDoubleValue() );

    bool freeze = master_freeze->getBoolValue();
    if ( !freeze ) {
        fgSetBool("/sim/freeze/master", true);
    }
    fgSetBool("/sim/crashed", false);

    // Force reupdating the positions of the ai 3d models. They are used for
    // initializing ground level for the FDM.
    globals->get_subsystem("ai_model")->reinit();

    // Initialize the FDM
    globals->get_subsystem("flight")->reinit();

    // reload offsets from config defaults
    globals->get_viewmgr()->reinit();

    fgInitView();

    globals->get_controls()->reset_all();

    fgUpdateLocalTime();

    // re-init to proper time of day setting
    fgInitTimeOffset();

    if ( !freeze ) {
        fgSetBool("/sim/freeze/master", false);
    }
    fgSetBool("/sim/sceneryloaded",false);
}


void doSimulatorReset(void)  // from gui_local.cxx -- TODO merge with fgReInitSubsystems()
{
    static SGPropertyNode_ptr master_freeze = fgGetNode("/sim/freeze/master", true);

    bool freeze = master_freeze->getBoolValue();
    if (!freeze)
        master_freeze->setBoolValue(true);

    fgSetBool("/sim/signals/reinit", true);

    globals->get_subsystem("flight")->unbind();

    // in case user has changed window size as
    // restoreInitialState() overwrites these
    int xsize = fgGetInt("/sim/startup/xsize");
    int ysize = fgGetInt("/sim/startup/ysize");

    // viewports also needs to be saved/restored as
    // restoreInitialState() overwrites these
    SGPropertyNode *guiNode = new SGPropertyNode;
    SGPropertyNode *cameraNode = new SGPropertyNode;
    SGPropertyNode *cameraGroupNode = fgGetNode("/sim/rendering/camera-group");
    copyProperties(cameraGroupNode->getChild("camera"), cameraNode);
    copyProperties(cameraGroupNode->getChild("gui"), guiNode);

    globals->restoreInitialState();

    // update our position based on current presets
    fgInitPosition();

    // We don't know how to resize the window, so keep the last values
    //  for xsize and ysize, and don't use the one set initially
    fgSetInt("/sim/startup/xsize", xsize);
    fgSetInt("/sim/startup/ysize", ysize);

    copyProperties(cameraNode, cameraGroupNode->getChild("camera"));
    copyProperties(guiNode, cameraGroupNode->getChild("gui"));

    delete guiNode;
    delete cameraNode;

    SGTime *t = globals->get_time_params();
    delete t;
    t = fgInitTime();
    globals->set_time_params(t);

    fgReInitSubsystems();

    globals->get_tile_mgr()->update(fgGetDouble("/environment/visibility-m"));
    globals->get_renderer()->resize(xsize, ysize);
    fgSetBool("/sim/signals/reinit", false);

    if (!freeze)
        master_freeze->setBoolValue(false);
}

