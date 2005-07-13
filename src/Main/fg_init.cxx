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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
//
// $Id$


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

// For BC 5.01 this must be included before OpenGL includes.
#ifdef SG_MATH_EXCEPTION_CLASH
#  include <math.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>             // strcmp()


#if defined( unix ) || defined( __CYGWIN__ )
#  include <unistd.h>           // for gethostname()
#endif

// work around a stdc++ lib bug in some versions of linux, but doesn't
// seem to hurt to have this here for all versions of Linux.
#ifdef linux
#  define _G_NO_EXTERN_TEMPLATES
#endif

#include <simgear/compiler.h>

#include STL_STRING

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/structure/event_mgr.hxx>
#include <simgear/math/point3d.hxx>
#include <simgear/math/polar3d.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/interpolator.hxx>
#include <simgear/scene/material/matlib.hxx>
#include <simgear/timing/sg_time.hxx>
#include <simgear/timing/lowleveltime.h>

#include <Aircraft/aircraft.hxx>
#include <FDM/UIUCModel/uiuc_aircraftdir.h>
#include <Airports/apt_loader.hxx>
#include <Airports/runways.hxx>
#include <Airports/simple.hxx>
#include <AIModel/AIManager.hxx>
#include <ATC/ATCdisplay.hxx>
#include <ATC/ATCmgr.hxx>
#include <ATC/AIMgr.hxx>
#include <Autopilot/auto_gui.hxx>
#include <Autopilot/route_mgr.hxx>
#include <Autopilot/xmlauto.hxx>
#include <Cockpit/cockpit.hxx>
#include <Cockpit/panel.hxx>
#include <Cockpit/panel_io.hxx>
#ifdef ENABLE_SP_FMDS
#include <FDM/SP/ADA.hxx>
#include <FDM/SP/ACMS.hxx>
#endif
#include <FDM/Balloon.h>
#include <FDM/ExternalNet/ExternalNet.hxx>
#include <FDM/ExternalPipe/ExternalPipe.hxx>
#include <FDM/JSBSim/JSBSim.hxx>
#include <FDM/LaRCsim/LaRCsim.hxx>
#include <FDM/MagicCarpet.hxx>
#include <FDM/UFO.hxx>
#include <FDM/NullFDM.hxx>
#include <FDM/YASim/YASim.hxx>
#include <GUI/new_gui.hxx>
#include <Include/general.hxx>
#include <Input/input.hxx>
#include <Instrumentation/instrument_mgr.hxx>
#include <Model/acmodel.hxx>
#include <AIModel/submodel.hxx>
#include <AIModel/AIManager.hxx>
#include <Navaids/navdb.hxx>
#include <Navaids/navlist.hxx>
#include <Replay/replay.hxx>
#include <Scenery/scenery.hxx>
#include <Scenery/tilemgr.hxx>
#include <Scripting/NasalSys.hxx>
#include <Sound/fg_fx.hxx>
#include <Sound/beacon.hxx>
#include <Sound/morse.hxx>
#include <Systems/system_mgr.hxx>
#include <Time/light.hxx>
#include <Time/moonpos.hxx>
#include <Time/sunpos.hxx>
#include <Time/sunsolver.hxx>
#include <Time/tmp.hxx>
#include <Traffic/TrafficMgr.hxx>

#ifdef FG_MPLAYER_AS
#include <MultiPlayer/multiplaytxmgr.hxx>
#include <MultiPlayer/multiplayrxmgr.hxx>
#endif

#include <Environment/environment_mgr.hxx>

#include "fg_init.hxx"
#include "fg_io.hxx"
#include "fg_commands.hxx"
#include "fg_props.hxx"
#include "options.hxx"
#include "globals.hxx"
#include "logger.hxx"
#include "viewmgr.hxx"

#if defined(FX) && defined(XMESA)
#include <GL/xmesa.h>
#endif

SG_USING_STD(string);


class Sound;
extern const char *default_root;
float init_volume;


// Scan the command line options for the specified option and return
// the value.
static string fgScanForOption( const string& option, int argc, char **argv ) {
    int i = 1;

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
#ifndef __MWERKS__
    while ( ! in.eof() ) {
#else
    char c = '\0';
    while ( in.get(c) && c != '\0' ) {
	in.putback(c);
#endif
	string line;

#if defined( macintosh )
        getline( in, line, '\r' );
#else
	getline( in, line, '\n' );
#endif

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


// Read in configuration (files and command line options) but only set
// fg_root
bool fgInitFGRoot ( int argc, char **argv ) {
    string root;
    char* envp;

    // First parse command line options looking for --fg-root=, this
    // will override anything specified in a config file
    root = fgScanForOption( "--fg-root=", argc, argv);

#if defined( unix ) || defined( __CYGWIN__ )
    // Next check home directory for .fgfsrc.hostname file
    if ( root.empty() ) {
        envp = ::getenv( "HOME" );
        if ( envp != NULL ) {
            SGPath config( envp );
            config.append( ".fgfsrc" );
            char name[256];
            gethostname( name, 256 );
            config.concat( "." );
            config.concat( name );
            root = fgScanForOption( "--fg-root=", config.str() );
        }
    }
#endif

    // Next check home directory for .fgfsrc file
    if ( root.empty() ) {
        envp = ::getenv( "HOME" );
        if ( envp != NULL ) {
            SGPath config( envp );
            config.append( ".fgfsrc" );
            root = fgScanForOption( "--fg-root=", config.str() );
        }
    }
    
    // Next check if fg-root is set as an env variable
    if ( root.empty() ) {
        envp = ::getenv( "FG_ROOT" );
        if ( envp != NULL ) {
            root = envp;
        }
    }

    // Otherwise, default to a random compiled-in location if we can't
    // find fg-root any other way.
    if ( root.empty() ) {
#if defined( __CYGWIN__ )
        root = "/FlightGear";
#elif defined( WIN32 )
        root = "\\FlightGear";
#elif defined(OSX_BUNDLE) 
        /* the following code looks for the base package directly inside
            the application bundle. This can be changed fairly easily by
            fiddling with the code below. And yes, I know it's ugly and verbose.
        */
        CFBundleRef appBundle = CFBundleGetMainBundle();
        CFURLRef appUrl = CFBundleCopyBundleURL(appBundle);
        CFRelease(appBundle);

        // look for a 'data' subdir directly inside the bundle : is there
        // a better place? maybe in Resources? I don't know ...
        CFURLRef dataDir = CFURLCreateCopyAppendingPathComponent(NULL, appUrl, CFSTR("data"), true);

        // now convert down to a path, and the a c-string
        CFStringRef path = CFURLCopyFileSystemPath(dataDir, kCFURLPOSIXPathStyle);
        root = CFStringGetCStringPtr(path, CFStringGetSystemEncoding());

        // tidy up.
        CFRelease(appBundle);
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
    char* envp;

    // First parse command line options looking for --aircraft=, this
    // will override anything specified in a config file
    aircraft = fgScanForOption( "--aircraft=", argc, argv );

#if defined( unix ) || defined( __CYGWIN__ )
    // Next check home directory for .fgfsrc.hostname file
    if ( aircraft.empty() ) {
        envp = ::getenv( "HOME" );
        if ( envp != NULL ) {
            SGPath config( envp );
            config.append( ".fgfsrc" );
            char name[256];
            gethostname( name, 256 );
            config.concat( "." );
            config.concat( name );
            aircraft = fgScanForOption( "--aircraft=", config.str() );
        }
    }
#endif

    // Next check home directory for .fgfsrc file
    if ( aircraft.empty() ) {
        envp = ::getenv( "HOME" );
        if ( envp != NULL ) {
            SGPath config( envp );
            config.append( ".fgfsrc" );
            aircraft = fgScanForOption( "--aircraft=", config.str() );
        }
    }

    if ( aircraft.empty() ) {
        // Check for $fg_root/system.fgfsrc
        SGPath sysconf( globals->get_fg_root() );
        sysconf.append( "system.fgfsrc" );
        aircraft = fgScanForOption( "--aircraft=", sysconf.str() );
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
      SG_LOG(SG_GENERAL, SG_ALERT, "Incorrect path in configuration file.");
      return NULL;
   }

   d_path.append(d_path_str);
   SG_LOG(SG_GENERAL, SG_INFO, "Reading localized strings from "
                                  << d_path.str());

   SGPropertyNode *strings = c_node->getNode("strings");
   try {
      readProperties(d_path.str(), strings);
   } catch (const sg_exception &e) {
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
         SG_LOG(SG_GENERAL, SG_ALERT, "Incorrect path in configuration file.");
         return NULL;
      }

      c_path.append(c_path_str);
      SG_LOG(SG_GENERAL, SG_INFO, "Reading localized strings from "
                                     << c_path.str());

      try {
         readProperties(c_path.str(), strings);
      } catch (const sg_exception &e) {
         SG_LOG(SG_GENERAL, SG_ALERT, "Unable to read the localized strings");
         return NULL;
      }
   }

   return c_node;
}



// Initialize the localization routines
bool fgDetectLanguage() {
    char *language = ::getenv("LANG");

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
    char name[256];
    // Check for $fg_root/system.fgfsrc.hostname
    gethostname( name, 256 );
    config.concat( "." );
    config.concat( name );
    fgParseOptions(config.str());
#endif

    // Check for ~/.fgfsrc
    char* envp = ::getenv( "HOME" );
    if ( envp != NULL ) {
        config.set( envp );
        config.append( ".fgfsrc" );
        fgParseOptions(config.str());
    }

#if defined( unix ) || defined( __CYGWIN__ )
    // Check for ~/.fgfsrc.hostname
    gethostname( name, 256 );
    config.concat( "." );
    config.concat( name );
    fgParseOptions(config.str());
#endif

    // Parse remaining command line options
    // These will override anything specified in a config file
    fgParseArgs(argc, argv);
}


static string fgFindAircraftPath( const SGPath &path, const string &aircraft ) {
    ulDirEnt* dire;
    ulDir *dirp = ulOpenDir(path.str().c_str());
    if (dirp == NULL) {
        cerr << "Unable to open aircraft directory '" << path.str() << '\'' << endl;
        exit(-1);
    }

    while ((dire = ulReadDir(dirp)) != NULL) {
        if (dire->d_isdir) {
            if ( strcmp("CVS", dire->d_name) && strcmp(".", dire->d_name)
                 && strcmp("..", dire->d_name) )
            {
                SGPath next = path;
                next.append(dire->d_name);

                string result = fgFindAircraftPath( next, aircraft );
                if ( ! result.empty() ) {
                    return result;
                }
            }
        } else if ( !strcmp(dire->d_name, aircraft.c_str()) ) {
            return path.str();
        }
    }

    ulCloseDir(dirp);

    return "";
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

    // Scan user config files and command line for a specified aircraft.
    fgInitFGAircraft(argc, argv);

    string aircraft = fgGetString( "/sim/aircraft", "" );
    if ( aircraft.size() > 0 ) {
        SGPath aircraft_search( globals->get_fg_root() );
        aircraft_search.append( "Aircraft" );

        string aircraft_set = aircraft + "-set.xml";

        string result = fgFindAircraftPath( aircraft_search, aircraft_set );
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

    // parse options after loading aircraft to ensure any user
    // overrides of defaults are honored.
    do_options(argc, argv);

    return true;
}


// find basic airport location info from airport database
bool fgFindAirportID( const string& id, FGAirport *a ) {
    FGAirport result;
    if ( id.length() ) {
        SG_LOG( SG_GENERAL, SG_INFO, "Searching for airport code = " << id );

        result = globals->get_airports()->search( id );

        if ( result.getId().empty() ) {
            SG_LOG( SG_GENERAL, SG_ALERT,
                    "Failed to find " << id << " in basic.dat.gz" );
            return false;
        }
    } else {
        return false;
    }

    *a = result;

    SG_LOG( SG_GENERAL, SG_INFO,
            "Position for " << id << " is ("
            << a->getLongitude() << ", "
            << a->getLatitude() << ")" );

    return true;
}


// get airport elevation
static double fgGetAirportElev( const string& id ) {
    FGAirport a;
    // double lon, lat;

    SG_LOG( SG_GENERAL, SG_INFO,
            "Finding elevation for airport: " << id );

    if ( fgFindAirportID( id, &a ) ) {
        return a.getElevation();
    } else {
        return -9999.0;
    }
}


#if 0 
  // 
  // This function is never used, but maybe useful in the future ???
  //

// Preset lon/lat given an airport id
static bool fgSetPosFromAirportID( const string& id ) {
    FGAirport a;
    // double lon, lat;

    SG_LOG( SG_GENERAL, SG_INFO,
            "Attempting to set starting position from airport code " << id );

    if ( fgFindAirportID( id, &a ) ) {
        // presets
        fgSetDouble("/sim/presets/longitude-deg", a.longitude );
        fgSetDouble("/sim/presets/latitude-deg", a.latitude );

        // other code depends on the actual postition being set so set
        // that as well
        fgSetDouble("/position/longitude-deg", a.longitude );
        fgSetDouble("/position/latitude-deg", a.latitude );

        SG_LOG( SG_GENERAL, SG_INFO,
                "Position for " << id << " is (" << a.longitude
                << ", " << a.latitude << ")" );

        return true;
    } else {
        return false;
    }
}
#endif


// Set current tower position lon/lat given an airport id
static bool fgSetTowerPosFromAirportID( const string& id, double hdg ) {
    FGAirport a;
    // tower height hard coded for now...
    float towerheight=50.0f;

    // make a little off the heading for 1 runway airports...
    float fudge_lon = fabs(sin(hdg)) * .003f;
    float fudge_lat = .003f - fudge_lon;

    if ( fgFindAirportID( id, &a ) ) {
        fgSetDouble("/sim/tower/longitude-deg",  a.getLongitude() + fudge_lon);
        fgSetDouble("/sim/tower/latitude-deg",  a.getLatitude() + fudge_lat);
        fgSetDouble("/sim/tower/altitude-ft", a.getElevation() + towerheight);
        return true;
    } else {
        return false;
    }

}


// Set current_options lon/lat given an airport id and heading (degrees)
static bool fgSetPosFromAirportIDandHdg( const string& id, double tgt_hdg ) {
    FGRunway r;

    if ( id.length() ) {
        // set initial position from runway and heading

        SG_LOG( SG_GENERAL, SG_INFO,
                "Attempting to set starting position from airport code "
                << id << " heading " << tgt_hdg );
		
        if ( ! globals->get_runways()->search( id, (int)tgt_hdg, &r ) ) {
            SG_LOG( SG_GENERAL, SG_ALERT,
                    "Failed to find a good runway for " << id << '\n' );
            return false;
        }	
    } else {
        return false;
    }

    double lat2, lon2, az2;
    double heading = r._heading;
    double azimuth = heading + 180.0;
    while ( azimuth >= 360.0 ) { azimuth -= 360.0; }

    SG_LOG( SG_GENERAL, SG_INFO,
            "runway =  " << r._lon << ", " << r._lat
            << " length = " << r._length * SG_FEET_TO_METER 
            << " heading = " << azimuth );
	    
    geo_direct_wgs_84 ( 0, r._lat, r._lon, azimuth, 
                        r._length * SG_FEET_TO_METER * 0.5 - 5.0,
                        &lat2, &lon2, &az2 );

    if ( fabs( fgGetDouble("/sim/presets/offset-distance") ) > SG_EPSILON ) {
        double olat, olon;
        double odist = fgGetDouble("/sim/presets/offset-distance");
        odist *= SG_NM_TO_METER;
        double oaz = azimuth;
        if ( fabs(fgGetDouble("/sim/presets/offset-azimuth")) > SG_EPSILON ) {
            oaz = fgGetDouble("/sim/presets/offset-azimuth") + 180;
            heading = tgt_hdg;
        }
        while ( oaz >= 360.0 ) { oaz -= 360.0; }
        geo_direct_wgs_84 ( 0, lat2, lon2, oaz, odist, &olat, &olon, &az2 );
        lat2=olat;
        lon2=olon;
    }

    // presets
    fgSetDouble("/sim/presets/longitude-deg",  lon2 );
    fgSetDouble("/sim/presets/latitude-deg",  lat2 );
    fgSetDouble("/sim/presets/heading-deg", heading );

    // other code depends on the actual values being set ...
    fgSetDouble("/position/longitude-deg",  lon2 );
    fgSetDouble("/position/latitude-deg",  lat2 );
    fgSetDouble("/orientation/heading-deg", heading );

    SG_LOG( SG_GENERAL, SG_INFO,
            "Position for " << id << " is ("
            << lon2 << ", "
            << lat2 << ") new heading is "
            << heading );
	    
    return true;
}


// Set current_options lon/lat given an airport id and runway number
static bool fgSetPosFromAirportIDandRwy( const string& id, const string& rwy ) {
    FGRunway r;

    if ( id.length() ) {
        // set initial position from airport and runway number

        SG_LOG( SG_GENERAL, SG_INFO,
                "Attempting to set starting position for "
                << id << ":" << rwy );

        if ( ! globals->get_runways()->search( id, rwy, &r ) ) {
            SG_LOG( SG_GENERAL, SG_ALERT,
                    "Failed to find runway " << rwy << 
                    " at airport " << id );
            return false;
        }
    } else {
        return false;
    }

    double lat2, lon2, az2;
    double heading = r._heading;
    double azimuth = heading + 180.0;
    while ( azimuth >= 360.0 ) { azimuth -= 360.0; }
    
    SG_LOG( SG_GENERAL, SG_INFO,
    "runway =  " << r._lon << ", " << r._lat
    << " length = " << r._length * SG_FEET_TO_METER 
    << " heading = " << azimuth );
    
    geo_direct_wgs_84 ( 0, r._lat, r._lon, 
    azimuth,
    r._length * SG_FEET_TO_METER * 0.5 - 5.0,
    &lat2, &lon2, &az2 );
    
    if ( fabs( fgGetDouble("/sim/presets/offset-distance") ) > SG_EPSILON )
    {
	double olat, olon;
	double odist = fgGetDouble("/sim/presets/offset-distance");
	odist *= SG_NM_TO_METER;
	double oaz = azimuth;
	if ( fabs(fgGetDouble("/sim/presets/offset-azimuth")) > SG_EPSILON )
	{
	    oaz = fgGetDouble("/sim/presets/offset-azimuth") + 180;
            heading = fgGetDouble("/sim/presets/heading-deg");
	}
	while ( oaz >= 360.0 ) { oaz -= 360.0; }
	geo_direct_wgs_84 ( 0, lat2, lon2, oaz, odist, &olat, &olon, &az2 );
	lat2=olat;
	lon2=olon;
    }
    
    // presets
    fgSetDouble("/sim/presets/longitude-deg",  lon2 );
    fgSetDouble("/sim/presets/latitude-deg",  lat2 );
    fgSetDouble("/sim/presets/heading-deg", heading );
    
    // other code depends on the actual values being set ...
    fgSetDouble("/position/longitude-deg",  lon2 );
    fgSetDouble("/position/latitude-deg",  lat2 );
    fgSetDouble("/orientation/heading-deg", heading );
    
    SG_LOG( SG_GENERAL, SG_INFO,
    "Position for " << id << " is ("
    << lon2 << ", "
    << lat2 << ") new heading is "
    << heading );
    
    return true;
}


static void fgSetDistOrAltFromGlideSlope() {
    // cout << "fgSetDistOrAltFromGlideSlope()" << endl;
    string apt_id = fgGetString("/sim/presets/airport-id");
    double gs = fgGetDouble("/sim/presets/glideslope-deg")
        * SG_DEGREES_TO_RADIANS ;
    double od = fgGetDouble("/sim/presets/offset-distance");
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
        fgSetDouble("/sim/presets/offset-distance", od);
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

    // set initial position from runway and heading
    if ( nav != NULL ) {
        SG_LOG( SG_GENERAL, SG_INFO, "Attempting to set starting position for "
                << id << ":" << freq );

        double lon = nav->get_lon();
        double lat = nav->get_lat();

        if ( fabs( fgGetDouble("/sim/presets/offset-distance") ) > SG_EPSILON )
        {
            double odist = fgGetDouble("/sim/presets/offset-distance")
                * SG_NM_TO_METER;
            double oaz = fabs(fgGetDouble("/sim/presets/offset-azimuth"))
                + 180.0;
            while ( oaz >= 360.0 ) { oaz -= 360.0; }
            double olat, olon, az2;
            geo_direct_wgs_84 ( 0, lat, lon, oaz, odist, &olat, &olon, &az2 );

            lat = olat;
            lon = olon;
        }

        // presets
        fgSetDouble("/sim/presets/longitude-deg",  lon );
        fgSetDouble("/sim/presets/latitude-deg",  lat );

        // other code depends on the actual values being set ...
        fgSetDouble("/position/longitude-deg",  lon );
        fgSetDouble("/position/latitude-deg",  lat );
        fgSetDouble("/orientation/heading-deg", 
                    fgGetDouble("/sim/presets/heading-deg") );

        SG_LOG( SG_GENERAL, SG_INFO,
                "Position for " << id << ":" << freq << " is ("
                << lon << ", "<< lat << ")" );

        return true;
    } else {
        SG_LOG( SG_GENERAL, SG_ALERT, "Failed to locate NAV = "
                << id << ":" << freq );
        return false;
    }
}

// Set current_options lon/lat given an aircraft carrier id
static bool fgSetPosFromCarrier( const string& carrier, const string& posid ) {

    // set initial position from runway and heading
    Point3D geodPos;
    double heading;
    sgdVec3 uvw;
    if (FGAIManager::getStartPosition(carrier, posid, geodPos, heading, uvw)) {
        double lon = geodPos.lon() * SGD_RADIANS_TO_DEGREES;
        double lat = geodPos.lat() * SGD_RADIANS_TO_DEGREES;
        double alt = geodPos.elev() * SG_METER_TO_FEET;

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
        fgSetDouble("/velocities/uBody-fps", uvw[0]);
        fgSetDouble("/velocities/vBody-fps", uvw[1]);
        fgSetDouble("/velocities/wBody-fps", uvw[2]);
        fgSetDouble("/sim/presets/uBody-fps", uvw[0]);
        fgSetDouble("/sim/presets/vBody-fps", uvw[1]);
        fgSetDouble("/sim/presets/wBody-fps", uvw[2]);

        fgSetBool("/sim/presets/onground", true);

        return true;
    } else {
        SG_LOG( SG_GENERAL, SG_ALERT, "Failed to locate aircraft carroer = "
                << carrier );
        return false;
    }
}
 
// Set current_options lon/lat given an airport id and heading (degrees)
static bool fgSetPosFromFix( const string& id ) {
    FGFix fix;

    // set initial position from runway and heading
    if ( globals->get_fixlist()->query( id.c_str(), &fix ) ) {
        SG_LOG( SG_GENERAL, SG_INFO, "Attempting to set starting position for "
                << id );

        double lon = fix.get_lon();
        double lat = fix.get_lat();

        if ( fabs( fgGetDouble("/sim/presets/offset-distance") ) > SG_EPSILON )
        {
            double odist = fgGetDouble("/sim/presets/offset-distance")
                * SG_NM_TO_METER;
            double oaz = fabs(fgGetDouble("/sim/presets/offset-azimuth"))
                + 180.0;
            while ( oaz >= 360.0 ) { oaz -= 360.0; }
            double olat, olon, az2;
            geo_direct_wgs_84 ( 0, lat, lon, oaz, odist, &olat, &olon, &az2 );

            lat = olat;
            lon = olon;
        }

        // presets
        fgSetDouble("/sim/presets/longitude-deg",  lon );
        fgSetDouble("/sim/presets/latitude-deg",  lat );

        // other code depends on the actual values being set ...
        fgSetDouble("/position/longitude-deg",  lon );
        fgSetDouble("/position/latitude-deg",  lat );
        fgSetDouble("/orientation/heading-deg", 
                    fgGetDouble("/sim/presets/heading-deg") );

        SG_LOG( SG_GENERAL, SG_INFO,
                "Position for " << id << " is ("
                << lon << ", "<< lat << ")" );

        return true;
    } else {
        SG_LOG( SG_GENERAL, SG_ALERT, "Failed to locate NAV = "
                << id );
        return false;
    }
}
 
static void parseWaypoints() {
    string_list *waypoints = globals->get_initial_waypoints();
    if (waypoints) {
	vector<string>::iterator i;
	for (i = waypoints->begin(); 
	     i  != waypoints->end();
	     i++)
        {
            NewWaypoint(*i);
        }
	// Now were done using the way points we can deallocate the
	// memory they used
	while (waypoints->begin() != waypoints->end()) {
            waypoints->pop_back();
        }
	delete waypoints;
	globals->set_initial_waypoints(0);
    }
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

    FGAirportList *airports = new FGAirportList();
    globals->set_airports( airports );
    FGRunwayList *runways = new FGRunwayList();
    globals->set_runways( runways );

    fgAirportDBLoad( airports, runways, aptdb.str(), p_metar.str() );

    FGNavList *navlist = new FGNavList;
    FGNavList *loclist = new FGNavList;
    FGNavList *gslist = new FGNavList;
    FGNavList *dmelist = new FGNavList;
    FGNavList *mkrlist = new FGNavList;

    globals->set_navlist( navlist );
    globals->set_loclist( loclist );
    globals->set_gslist( gslist );
    globals->set_dmelist( dmelist );
    globals->set_mkrlist( mkrlist );

    if ( !fgNavDBInit(airports, navlist, loclist, gslist, dmelist, mkrlist) ) {
        SG_LOG( SG_GENERAL, SG_ALERT,
                "Problems loading one or more navigational database" );
    }

    if ( fgGetBool("/sim/navdb/localizers/auto-align", true) ) {
        // align all the localizers with their corresponding runways
        // since data sources are good for cockpit navigation
        // purposes, but not always to the error tolerances needed to
        // exactly place these items.
        double threshold
            = fgGetDouble( "/sim/navdb/localizers/auto-align-threshold-deg",
                           5.0 );
        fgNavDBAlignLOCwithRunway( runways, loclist, threshold );
    }

    SG_LOG(SG_GENERAL, SG_INFO, "  Fixes");
    SGPath p_fix( globals->get_fg_root() );
    p_fix.append( "Navaids/fix.dat" );
    FGFixList *fixlist = new FGFixList;
    fixlist->init( p_fix );
    globals->set_fixlist( fixlist );

    return true;
}


// Set the initial position based on presets (or defaults)
bool fgInitPosition() {
    // cout << "fgInitPosition()" << endl;
    double gs = fgGetDouble("/sim/presets/glideslope-deg")
        * SG_DEGREES_TO_RADIANS ;
    double od = fgGetDouble("/sim/presets/offset-distance");
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
    double hdg = fgGetDouble("/sim/presets/heading-deg");
    string vor = fgGetString("/sim/presets/vor-id");
    double vor_freq = fgGetDouble("/sim/presets/vor-freq");
    string ndb = fgGetString("/sim/presets/ndb-id");
    double ndb_freq = fgGetDouble("/sim/presets/ndb-freq");
    string carrier = fgGetString("/sim/presets/carrier");
    string parkpos = fgGetString("/sim/presets/parkpos");
    string fix = fgGetString("/sim/presets/fix");

    if ( !set_pos && !apt.empty() && !rwy_no.empty() ) {
        // An airport + runway is requested
        if ( fgSetPosFromAirportIDandRwy( apt, rwy_no ) ) {
            // set tower position (a little off the heading for single
            // runway airports)
            fgSetTowerPosFromAirportID( apt, hdg );
            set_pos = true;
        }
    }

    if ( !set_pos && !apt.empty() ) {
        // An airport is requested (find runway closest to hdg)
        if ( fgSetPosFromAirportIDandHdg( apt, hdg ) ) {
            // set tower position (a little off the heading for single
            // runway airports)
            fgSetTowerPosFromAirportID( apt, hdg );
            set_pos = true;
        }
    }

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
    fgSetDouble( "/orientation/heading-deg",
                 fgGetDouble("/sim/presets/heading-deg") );

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

#if defined(FX) && defined(XMESA)
    char *mesa_win_state;
#endif

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

#if defined(FX) && defined(XMESA)
    // initialize full screen flag
    globals->set_fullscreen(false);
    if ( strstr ( general.get_glRenderer(), "Glide" ) ) {
        // Test for the MESA_GLX_FX env variable
        if ( (mesa_win_state = getenv( "MESA_GLX_FX" )) != NULL) {
            // test if we are fullscreen mesa/glide
            if ( (mesa_win_state[0] == 'f') ||
                 (mesa_win_state[0] == 'F') ) {
                globals->set_fullscreen(true);
            }
        }
    }
#endif

    return true;
}


// Initialize the flight model subsystem.  This just creates the
// object.  The actual fdm initialization is delayed until we get a
// proper scenery elevation hit.  This is checked for in main.cxx

void fgInitFDM() {

    if ( cur_fdm_state ) {
        delete cur_fdm_state;
        cur_fdm_state = 0;
    }

    double dt = 1.0 / fgGetInt("/sim/model-hz");
    aircraft_dir = fgGetString("/sim/aircraft-dir");
    const string &model = fgGetString("/sim/flight-model");

    try {
        if ( model == "larcsim" ) {
            cur_fdm_state = new FGLaRCsim( dt );
        } else if ( model == "jsb" ) {
            cur_fdm_state = new FGJSBsim( dt );
#ifdef ENABLE_SP_FMDS
        } else if ( model == "ada" ) {
            cur_fdm_state = new FGADA( dt );
        } else if ( model == "acms" ) {
            cur_fdm_state = new FGACMS( dt );
#endif
        } else if ( model == "balloon" ) {
            cur_fdm_state = new FGBalloonSim( dt );
        } else if ( model == "magic" ) {
            cur_fdm_state = new FGMagicCarpet( dt );
        } else if ( model == "ufo" ) {
            cur_fdm_state = new FGUFO( dt );
        } else if ( model == "external" ) {
            // external is a synonym for "--fdm=null" and is
            // maintained here for backwards compatibility
            cur_fdm_state = new FGNullFDM( dt );
        } else if ( model.find("network") == 0 ) {
            string host = "localhost";
            int port1 = 5501;
            int port2 = 5502;
            int port3 = 5503;
            string net_options = model.substr(8);
            string::size_type begin, end;
            begin = 0;
            // host
            end = net_options.find( ",", begin );
            if ( end != string::npos ) {
                host = net_options.substr(begin, end - begin);
                begin = end + 1;
            }
            // port1
            end = net_options.find( ",", begin );
            if ( end != string::npos ) {
                port1 = atoi( net_options.substr(begin, end - begin).c_str() );
                begin = end + 1;
            }
            // port2
            end = net_options.find( ",", begin );
            if ( end != string::npos ) {
                port2 = atoi( net_options.substr(begin, end - begin).c_str() );
                begin = end + 1;
            }
            // port3
            end = net_options.find( ",", begin );
            if ( end != string::npos ) {
                port3 = atoi( net_options.substr(begin, end - begin).c_str() );
                begin = end + 1;
            }
            cur_fdm_state = new FGExternalNet( dt, host, port1, port2, port3 );
        } else if ( model.find("pipe") == 0 ) {
            // /* old */ string pipe_path = model.substr(5);
            // /* old */ cur_fdm_state = new FGExternalPipe( dt, pipe_path );
            string pipe_path = "";
            string pipe_protocol = "";
            string pipe_options = model.substr(5);
            string::size_type begin, end;
            begin = 0;
            // pipe file path
            end = pipe_options.find( ",", begin );
            if ( end != string::npos ) {
                pipe_path = pipe_options.substr(begin, end - begin);
                begin = end + 1;
            }
            // protocol (last option)
            pipe_protocol = pipe_options.substr(begin);
            cur_fdm_state = new FGExternalPipe( dt, pipe_path, pipe_protocol );
        } else if ( model == "null" ) {
            cur_fdm_state = new FGNullFDM( dt );
        } else if ( model == "yasim" ) {
            cur_fdm_state = new YASim( dt );
        } else {
            SG_LOG(SG_GENERAL, SG_ALERT,
                   "Unrecognized flight model '" << model
                   << "', cannot init flight dynamics model.");
            exit(-1);
        }
    } catch ( ... ) {
        SG_LOG(SG_GENERAL, SG_ALERT, "FlightGear aborting\n\n");
        exit(-1);
    }
}

static void printMat(const sgVec4 *mat, char *name="")
{
    int i;
    SG_LOG(SG_GENERAL, SG_BULK, name );
    for(i=0; i<4; i++) {
        SG_LOG(SG_GENERAL, SG_BULK, "  " << mat[i][0] << " " << mat[i][1]
                                    << " " << mat[i][2] << " " << mat[i][3] );
    }
}

// Initialize view parameters
void fgInitView() {
  // force update of model so that viewer can get some data...
  globals->get_aircraft_model()->update(0);
  // run update for current view so that data is current...
  globals->get_viewmgr()->update(0);

  printMat(globals->get_current_view()->get_VIEW(),"VIEW");
  printMat(globals->get_current_view()->get_UP(),"UP");
  // printMat(globals->get_current_view()->get_LOCAL(),"LOCAL");
  
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
    const string &offset_type = fgGetString("/sim/startup/time-offset-type");

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
    } else if ( offset_type == "gmt-offset" ) {
        warp = offset - (currGMT - systemLocalTime);
    } else if ( offset_type == "latitude-offset" ) {
        warp = offset - (aircraftLocalTime - systemLocalTime);
    } else if ( offset_type == "system" ) {
        warp = offset - cur_time;
    } else if ( offset_type == "gmt" ) {
        warp = offset - currGMT;
    } else if ( offset_type == "latitude" ) {
        warp = offset - (aircraftLocalTime - systemLocalTime) - cur_time; 
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
    globals->get_event_mgr()->setFreezeProperty(fgGetNode("/sim/freeze/clock"));

    ////////////////////////////////////////////////////////////////////
    // Initialize the property interpolator subsystem
    ////////////////////////////////////////////////////////////////////
    globals->add_subsystem("interpolator", new SGInterpolator);


    ////////////////////////////////////////////////////////////////////
    // Add the FlightGear property utilities.
    ////////////////////////////////////////////////////////////////////
    globals->add_subsystem("properties", new FGProperties);

    ////////////////////////////////////////////////////////////////////
    // Initialize the material property subsystem.
    ////////////////////////////////////////////////////////////////////

    SGPath mpath( globals->get_fg_root() );
    mpath.append( "materials.xml" );
    if ( ! globals->get_matlib()->load(globals->get_fg_root(), mpath.str()) ) {
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

    // cause refresh of viewer scenery timestamps every 15 seconds...
    globals->get_event_mgr()->addTask( "FGTileMgr::refresh_view_timestamps()",
                                       globals->get_tile_mgr(),
                                       &FGTileMgr::refresh_view_timestamps,
                                       15 );

    SG_LOG( SG_GENERAL, SG_DEBUG,
            "Current terrain elevation after tile mgr init " <<
            globals->get_scenery()->get_cur_elev() );


    ////////////////////////////////////////////////////////////////////
    // Initialize the flight model subsystem.
    ////////////////////////////////////////////////////////////////////

    fgInitFDM();
        
    // allocates structures so must happen before any of the flight
    // model or control parameters are set
    fgAircraftInit();   // In the future this might not be the case.


    ////////////////////////////////////////////////////////////////////
    // Initialize the XML Autopilot subsystem.
    ////////////////////////////////////////////////////////////////////

    globals->add_subsystem( "xml-autopilot", new FGXMLAutopilot );
    globals->add_subsystem( "route-manager", new FGRouteMgr );

  
    ////////////////////////////////////////////////////////////////////
    // Initialize the view manager subsystem.
    ////////////////////////////////////////////////////////////////////

    fgInitView();

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
    // Initialize the weather subsystem.
    ////////////////////////////////////////////////////////////////////

    // Initialize the weather modeling subsystem
    globals->add_subsystem("environment", new FGEnvironmentMgr);


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
    // Initialize the sound subsystem.
    ////////////////////////////////////////////////////////////////////

    init_volume = fgGetFloat("/sim/sound/volume");
    fgSetFloat("/sim/sound/volume", 0.0f);
    globals->set_soundmgr(new SGSoundMgr);
    globals->get_soundmgr()->init();
    globals->get_soundmgr()->bind();


    ////////////////////////////////////////////////////////////////////
    // Initialize the sound-effects subsystem.
    ////////////////////////////////////////////////////////////////////

    globals->add_subsystem("fx", new FGFX);
    
#endif

    ////////////////////////////////////////////////////////////////////
    // Initialise ATC display system
    ////////////////////////////////////////////////////////////////////

    SG_LOG(SG_GENERAL, SG_INFO, "  ATC Display");
    globals->set_ATC_display(new FGATCDisplay);
    globals->get_ATC_display()->init(); 

    ////////////////////////////////////////////////////////////////////
    // Initialise the ATC Manager 
    ////////////////////////////////////////////////////////////////////

    SG_LOG(SG_GENERAL, SG_INFO, "  ATC Manager");
    globals->set_ATC_mgr(new FGATCMgr);
    globals->get_ATC_mgr()->init(); 
    
    ////////////////////////////////////////////////////////////////////
    // Initialise the AI Manager 
    ////////////////////////////////////////////////////////////////////

    SG_LOG(SG_GENERAL, SG_INFO, "  AI Manager");
    globals->set_AI_mgr(new FGAIMgr);
    globals->get_AI_mgr()->init();

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
	     FGTrafficManager *dispatcher = 
	     (FGTrafficManager*) globals->get_subsystem("Traffic Manager");
	     SGPath path = globals->get_fg_root();
	     path.append("/Traffic/fgtraffic.xml");
     readXML(path.str(),
	*dispatcher);
	     //globals->get_subsystem("Traffic Manager")->init();

    globals->add_subsystem("instrumentation", new FGInstrumentMgr);
    globals->add_subsystem("systems", new FGSystemMgr);



    ////////////////////////////////////////////////////////////////////
    // Initialize the cockpit subsystem
    ////////////////////////////////////////////////////////////////////
    if( fgCockpitInit( &current_aircraft )) {
        // Cockpit initialized ok.
    } else {
        SG_LOG( SG_GENERAL, SG_ALERT, "Error in Cockpit initialization!" );
        exit(-1);
    }


    ////////////////////////////////////////////////////////////////////
    // Initialize the autopilot subsystem.
    ////////////////////////////////////////////////////////////////////

                                // FIXME: these should go in the
                                // GUI initialization code, not here.
    // fgAPAdjustInit();
    NewTgtAirportInit();
    NewHeadingInit();
    NewAltitudeInit();

    ////////////////////////////////////////////////////////////////////
    // Initialize I/O subsystem.
    ////////////////////////////////////////////////////////////////////

    globals->get_io()->init();
    globals->get_io()->bind();


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

#ifdef FG_MPLAYER_AS
    ////////////////////////////////////////////////////////////////////
    // Initialize multiplayer subsystem
    ////////////////////////////////////////////////////////////////////

    globals->set_multiplayer_tx_mgr(new FGMultiplayTxMgr);
    globals->get_multiplayer_tx_mgr()->init();

    globals->set_multiplayer_rx_mgr(new FGMultiplayRxMgr);
    globals->get_multiplayer_rx_mgr()->init();
#endif

    ////////////////////////////////////////////////////////////////////////
    // Initialize the Nasal interpreter.
    // Do this last, so that the loaded scripts see initialized state
    ////////////////////////////////////////////////////////////////////////
    FGNasalSys* nasal = new FGNasalSys();
    globals->add_subsystem("nasal", nasal);
    nasal->init();

    ////////////////////////////////////////////////////////////////////
    // At this point we could try and parse the waypoint options
    ///////////////////////////////////////////////////////////////////
    parseWaypoints();

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
    fgInitFDM();
    
    // allocates structures so must happen before any of the flight
    // model or control parameters are set
    fgAircraftInit();   // In the future this might not be the case.

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

