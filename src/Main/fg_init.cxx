// fg_init.cxx -- Flight Gear top level initialization routines
//
// Written by Curtis Olson, started August 1997.
//
// Copyright (C) 1997  Curtis L. Olson  - curt@infoplane.com
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

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <GL/glut.h>

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
#include <simgear/misc/exception.hxx>

#include STL_STRING

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/math/point3d.hxx>
#include <simgear/math/polar3d.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/sky/clouds3d/SkySceneLoader.hpp>
#include <simgear/sky/clouds3d/SkyUtil.hpp>
#include <simgear/timing/sg_time.hxx>
#include <simgear/timing/lowleveltime.h>

#include <Aircraft/aircraft.hxx>
#include <FDM/UIUCModel/uiuc_aircraftdir.h>
#include <Airports/runways.hxx>
#include <Airports/simple.hxx>
#include <ATC/ATCdisplay.hxx>
#include <ATC/ATCmgr.hxx>
#include <ATC/atislist.hxx>
#include <ATC/towerlist.hxx>
#include <ATC/approachlist.hxx>
#include <ATC/AIMgr.hxx>
#include <Autopilot/auto_gui.hxx>
#include <Autopilot/newauto.hxx>
#include <Cockpit/cockpit.hxx>
#include <Cockpit/radiostack.hxx>
#include <Cockpit/steam.hxx>
#include <Cockpit/panel.hxx>
#include <Cockpit/panel_io.hxx>
#include <FDM/ADA.hxx>
#include <FDM/Balloon.h>
#include <FDM/ExternalNet/ExternalNet.hxx>
#include <FDM/JSBSim/JSBSim.hxx>
#include <FDM/LaRCsim.hxx>
#include <FDM/MagicCarpet.hxx>
#include <FDM/UFO.hxx>
#include <FDM/NullFDM.hxx>
#include <FDM/YASim/YASim.hxx>
#include <Include/general.hxx>
#include <Input/input.hxx>
#include <Instrumentation/instrument_mgr.hxx>
// #include <Joystick/joystick.hxx>
#include <Objects/matlib.hxx>
#include <Model/acmodel.hxx>
#include <Navaids/fixlist.hxx>
#include <Navaids/ilslist.hxx>
#include <Navaids/mkrbeacons.hxx>
#include <Navaids/navlist.hxx>
#include <Scenery/scenery.hxx>
#include <Scenery/tilemgr.hxx>
#include <Sound/fg_fx.hxx>
#include <Sound/soundmgr.hxx>
#include <Systems/system_mgr.hxx>
#include <Time/FGEventMgr.hxx>
#include <Time/light.hxx>
#include <Time/sunpos.hxx>
#include <Time/moonpos.hxx>
#include <Time/tmp.hxx>

#ifdef FG_WEATHERCM
#  include <WeatherCM/FGLocalWeatherDatabase.h>
#else
#  include <Environment/environment_mgr.hxx>
#endif

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

extern const char *default_root;

SkySceneLoader *sgCloud3d;

// Read in configuration (file and command line) and just set fg_root
bool fgInitFGRoot ( int argc, char **argv ) {
    string root;
    char* envp;

    // First parse command line options looking for fg-root, this will
    // override anything specified in a config file
    root = fgScanForRoot(argc, argv);

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
            root = fgScanForRoot(config.str());
        }
    }
#endif

    // Next check home directory for .fgfsrc file
    if ( root.empty() ) {
        envp = ::getenv( "HOME" );
        if ( envp != NULL ) {
            SGPath config( envp );
            config.append( ".fgfsrc" );
            root = fgScanForRoot(config.str());
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
#elif defined( macintosh )
        root = "";
#else
        root = PKGLIBDIR;
#endif
    }

    SG_LOG(SG_INPUT, SG_INFO, "fg_root = " << root );
    globals->set_fg_root(root);

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
       cerr << "No internationalization settings specified in preferences.xml"
            << endl;

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


// Read in configuration (file and command line)
bool fgInitConfig ( int argc, char **argv ) {

                                // First, set some sane default values
    fgSetDefaults();

    // Read global preferences from $FG_ROOT/preferences.xml
    SGPath props_path(globals->get_fg_root());
    props_path.append("preferences.xml");
    SG_LOG(SG_INPUT, SG_INFO, "Reading global preferences");
    readProperties(props_path.str(), globals->get_props());
    SG_LOG(SG_INPUT, SG_INFO, "Finished Reading global preferences");

    // Detect the required language as early as possible
    if (fgDetectLanguage() != true)
       return false;

    // Read the default aircraft config file.
    do_options(argc, argv);     // preparse options for default aircraft
    string aircraft = fgGetString("/sim/aircraft", "");
    if (aircraft.size() > 0) {
      SGPath aircraft_path(globals->get_fg_root());
      aircraft_path.append("Aircraft");
      aircraft_path.append(aircraft);
      aircraft_path.concat("-set.xml");
      SG_LOG(SG_INPUT, SG_INFO, "Reading default aircraft: " << aircraft
             << " from " << aircraft_path.str());
      try {
        readProperties(aircraft_path.str(), globals->get_props());
      } catch (const sg_exception &e) {
        string message = "Error reading default aircraft: ";
        message += e.getFormattedMessage();
        SG_LOG(SG_INPUT, SG_ALERT, message);
        exit(2);
      }
    } else {
      SG_LOG(SG_INPUT, SG_ALERT, "No default aircraft specified");
    }

    do_options(argc, argv);

    return true;
}


// find basic airport location info from airport database
bool fgFindAirportID( const string& id, FGAirport *a ) {
    if ( id.length() ) {
        SGPath path( globals->get_fg_root() );
        path.append( "Airports" );
        path.append( "simple.mk4" );
        FGAirports airports( path.c_str() );

        SG_LOG( SG_GENERAL, SG_INFO, "Searching for airport code = " << id );

        if ( ! airports.search( id, a ) ) {
            SG_LOG( SG_GENERAL, SG_ALERT,
                    "Failed to find " << id << " in " << path.str() );
            return false;
        }
    } else {
        return false;
    }

    SG_LOG( SG_GENERAL, SG_INFO,
            "Position for " << id << " is ("
            << a->longitude << ", "
            << a->latitude << ")" );

    return true;
}


// Set current_options lon/lat given an airport id
bool fgSetPosFromAirportID( const string& id ) {
    FGAirport a;
    // double lon, lat;

    SG_LOG( SG_GENERAL, SG_INFO,
            "Attempting to set starting position from airport code " << id );

    if ( fgFindAirportID( id, &a ) ) {
        fgSetDouble("/position/longitude-deg",  a.longitude );
        fgSetDouble("/position/latitude-deg",  a.latitude );
        SG_LOG( SG_GENERAL, SG_INFO,
                "Position for " << id << " is ("
                << a.longitude << ", "
                << a.latitude << ")" );

        return true;
    } else {
        return false;
    }

}



// Set current tower position lon/lat given an airport id
bool fgSetTowerPosFromAirportID( const string& id, double hdg ) {
    FGAirport a;
    // tower height hard coded for now...
    float towerheight=50.0f;

    // make a little off the heading for 1 runway airports...
    float fudge_lon = fabs(sin(hdg)) * .003f;
    float fudge_lat = .003f - fudge_lon;

    if ( fgFindAirportID( id, &a ) ) {
        fgSetDouble("/sim/tower/longitude-deg",  a.longitude + fudge_lon);
        fgSetDouble("/sim/tower/latitude-deg",  a.latitude + fudge_lat);
        fgSetDouble("/sim/tower/altitude-ft", a.elevation + towerheight);
        return true;
    } else {
        return false;
    }

}


// Set current_options lon/lat given an airport id and heading (degrees)
bool fgSetPosFromAirportIDandHdg( const string& id, double tgt_hdg ) {
    FGRunway r;
    FGRunway found_r;
    double found_dir = 0.0;

    if ( id.length() ) {
        // set initial position from runway and heading

        SGPath path( globals->get_fg_root() );
        path.append( "Airports" );
        path.append( "runways.mk4" );
        FGRunways runways( path.c_str() );

        SG_LOG( SG_GENERAL, SG_INFO,
                "Attempting to set starting position from runway code "
                << id << " heading " << tgt_hdg );

        // SGPath inpath( globals->get_fg_root() );
        // inpath.append( "Airports" );
        // inpath.append( "apt_simple" );
        // airports.load( inpath.c_str() );

        // SGPath outpath( globals->get_fg_root() );
        // outpath.append( "Airports" );
        // outpath.append( "simple.gdbm" );
        // airports.dump_gdbm( outpath.c_str() );

        if ( ! runways.search( id, &r ) ) {
            SG_LOG( SG_GENERAL, SG_ALERT,
                    "Failed to find " << id << " in database." );
            return false;
        }

        double diff;
        double min_diff = 360.0;

        while ( r.id == id ) {
            // forward direction
            diff = tgt_hdg - r.heading;
            while ( diff < -180.0 ) { diff += 360.0; }
            while ( diff >  180.0 ) { diff -= 360.0; }
            diff = fabs(diff);
            SG_LOG( SG_GENERAL, SG_INFO,
                    "Runway " << r.rwy_no << " heading = " << r.heading <<
                    " diff = " << diff );
            if ( diff < min_diff ) {
                min_diff = diff;
                found_r = r;
                found_dir = 0;
            }

            // reverse direction
            diff = tgt_hdg - r.heading - 180.0;
            while ( diff < -180.0 ) { diff += 360.0; }
            while ( diff >  180.0 ) { diff -= 360.0; }
            diff = fabs(diff);
            SG_LOG( SG_GENERAL, SG_INFO,
                    "Runway -" << r.rwy_no << " heading = " <<
                    r.heading + 180.0 <<
                    " diff = " << diff );
            if ( diff < min_diff ) {
                min_diff = diff;
                found_r = r;
                found_dir = 180.0;
            }

            runways.next( &r );
        }

        SG_LOG( SG_GENERAL, SG_INFO, "closest runway = " << found_r.rwy_no
                << " + " << found_dir );

    } else {
        return false;
    }

    double heading = found_r.heading + found_dir;
    while ( heading >= 360.0 ) { heading -= 360.0; }

    double lat2, lon2, az2;
    double azimuth = found_r.heading + found_dir + 180.0;
    while ( azimuth >= 360.0 ) { azimuth -= 360.0; }

    SG_LOG( SG_GENERAL, SG_INFO,
            "runway =  " << found_r.lon << ", " << found_r.lat
            << " length = " << found_r.length * SG_FEET_TO_METER * 0.5 
            << " heading = " << azimuth );
    
    geo_direct_wgs_84 ( 0, found_r.lat, found_r.lon, 
                        azimuth, found_r.length * SG_FEET_TO_METER * 0.5 - 5.0,
                        &lat2, &lon2, &az2 );

    if ( fabs( fgGetDouble("/sim/startup/offset-distance") ) > SG_EPSILON ) {
        double olat, olon;
        double odist = fgGetDouble("/sim/startup/offset-distance");
        odist *= SG_NM_TO_METER;
        double oaz = azimuth;
        if ( fabs(fgGetDouble("/sim/startup/offset-azimuth")) > SG_EPSILON ) {
            oaz = fgGetDouble("/sim/startup/offset-azimuth") + 180;
        }
        while ( oaz >= 360.0 ) { oaz -= 360.0; }
        geo_direct_wgs_84 ( 0, lat2, lon2, oaz, odist, &olat, &olon, &az2 );
        lat2=olat;
        lon2=olon;
    }
        
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


void fgSetPosFromGlideSlope(void) {
    double gs = fgGetDouble("/velocities/glideslope");
    double od = fgGetDouble("/sim/startup/offset-distance");
    double alt = fgGetDouble("/position/altitude-ft");
    
    //if glideslope and offset-distance are set and altitude is
    //not, calculate the initial altitude
    if( fabs(gs) > 0.01 && fabs(od) > 0.1 && alt < -9990 ) {
        od *= SG_NM_TO_METER * SG_METER_TO_FEET;
        alt = fabs(od*tan(gs));
        fgSetDouble("/position/altitude-ft",alt);
        fgSetBool("/sim/startup/onground", false);
        SG_LOG(SG_GENERAL,SG_INFO, "Calculated altitude as: " << alt  << " ft");
    } else if( fabs(gs) > 0.01 && alt > 0 && fabs(od) < 0.1) {
        od  = alt/tan(gs);
        od *= -1*SG_FEET_TO_METER * SG_METER_TO_NM;
        fgSetDouble("/sim/startup/offset-distance",od);
        SG_LOG(SG_GENERAL,SG_INFO, "Calculated offset distance as: " 
                                       << od  << " nm");
    } else if( fabs(gs) > 0.01 ) {
        SG_LOG(SG_GENERAL,SG_ALERT, "Glideslope given but not altitude" 
                                  << " or offset-distance.  Resetting"
                                  << " glideslope to zero" );
        fgSetDouble("/velocities/glideslope",0);                                  
    }                              
                                      
}                       

// General house keeping initializations
bool fgInitGeneral( void ) {
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
        } else if ( model == "ada" ) {
            cur_fdm_state = new FGADA( dt );
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
    cout << name << endl;
    for(i=0; i<4; i++) {
        cout <<"  "<<mat[i][0]<<" "<<mat[i][1]<<" "<<mat[i][2]<<" "<<mat[i][3]<<endl;
    }
    cout << endl;
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

    // Handle potential user specified time offsets
    time_t cur_time = t->get_cur_time();
    time_t currGMT = sgTimeGetGMT( gmtime(&cur_time) );
    time_t systemLocalTime = sgTimeGetGMT( localtime(&cur_time) );
    time_t aircraftLocalTime = 
        sgTimeGetGMT( fgLocaltime(&cur_time, t->get_zonename() ) );

    // Okay, we now have six possible scenarios
    int offset = fgGetInt("/sim/startup/time-offset");
    const string &offset_type = fgGetString("/sim/startup/time-offset-type");
    if (offset_type == "system-offset") {
        globals->set_warp( offset );
    } else if (offset_type == "gmt-offset") {
        globals->set_warp( offset - (currGMT - systemLocalTime) );
    } else if (offset_type == "latitude-offset") {
        globals->set_warp( offset - (aircraftLocalTime - systemLocalTime) );
    } else if (offset_type == "system") {
        globals->set_warp( offset - cur_time );
    } else if (offset_type == "gmt") {
        globals->set_warp( offset - currGMT );
    } else if (offset_type == "latitude") {
        globals->set_warp( offset - (aircraftLocalTime - systemLocalTime) - 
                           cur_time ); 
    } else {
        SG_LOG( SG_GENERAL, SG_ALERT,
                "FG_TIME::Unsupported offset type " << offset_type );
        exit( -1 );
    }

    SG_LOG( SG_GENERAL, SG_INFO, "After time init, warp = " 
            << globals->get_warp() );

    globals->set_warp_delta( 0 );

    t->update( 0.0, 0.0,
               cur_time_override->getLongValue(),
               globals->get_warp() );

    return t;
}


// This is the top level init routine which calls all the other
// initialization routines.  If you are adding a subsystem to flight
// gear, its initialization call should located in this routine.
// Returns non-zero if a problem encountered.
bool fgInitSubsystems( void ) {
    static const SGPropertyNode *longitude
        = fgGetNode("/position/longitude-deg");
    static const SGPropertyNode *latitude
        = fgGetNode("/position/latitude-deg");
    static const SGPropertyNode *altitude
        = fgGetNode("/position/altitude-ft");

    fgLIGHT *l = &cur_light_params;

    SG_LOG( SG_GENERAL, SG_INFO, "Initialize Subsystems");
    SG_LOG( SG_GENERAL, SG_INFO, "========== ==========");


    ////////////////////////////////////////////////////////////////////
    // Initialize the material property subsystem.
    ////////////////////////////////////////////////////////////////////

    SGPath mpath( globals->get_fg_root() );
    mpath.append( "materials.xml" );
    if ( ! material_lib.load( mpath.str() ) ) {
        SG_LOG( SG_GENERAL, SG_ALERT, "Error loading material lib!" );
        exit(-1);
    }

    ////////////////////////////////////////////////////////////////////
    // Initialize the event manager subsystem.
    ////////////////////////////////////////////////////////////////////

    global_events.init();

    // Output event stats every 60 seconds
    global_events.Register( "FGEventMgr::print_stats()",
                            &global_events, &FGEventMgr::print_stats,
                            60000 );


    ////////////////////////////////////////////////////////////////////
    // Initialize the scenery management subsystem.
    ////////////////////////////////////////////////////////////////////

    if ( global_tile_mgr.init() ) {
        // Load the local scenery data
        double visibility_meters = fgGetDouble("/environment/visibility-m");
                
        global_tile_mgr.update( longitude->getDoubleValue(),
                                latitude->getDoubleValue(),
                                visibility_meters );
    } else {
        SG_LOG( SG_GENERAL, SG_ALERT, "Error in Tile Manager initialization!" );
        exit(-1);
    }

    // cause refresh of viewer scenery timestamps every 15 seconds...
    global_events.Register( "FGTileMgr::refresh_view_timestamps()",
                            &global_tile_mgr, &FGTileMgr::refresh_view_timestamps,
                            15000 );

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
    // Initialize the view manager subsystem.
    ////////////////////////////////////////////////////////////////////

    fgInitView();


    ////////////////////////////////////////////////////////////////////
    // Initialize the lighting subsystem.
    ////////////////////////////////////////////////////////////////////

    // fgUpdateSunPos() needs a few position and view parameters set
    // so it can calculate local relative sun angle and a few other
    // things for correctly orienting the sky.
    fgUpdateSunPos();
    fgUpdateMoonPos();
    global_events.Register( "fgUpdateSunPos()", &fgUpdateSunPos,
                            60000);
    global_events.Register( "fgUpdateMoonPos()", &fgUpdateMoonPos,
                            60000);

    // Initialize Lighting interpolation tables
    l->Init();

    // force one lighting update to make it right to start with...
    l->Update();
    // update the lighting parameters (based on sun angle)
    global_events.Register( "fgLight::Update()",
                            &cur_light_params, &fgLIGHT::Update,
                            30000 );


    ////////////////////////////////////////////////////////////////////
    // Initialize the logger.
    ////////////////////////////////////////////////////////////////////
    
    globals->get_subsystem_mgr()->add(FGSubsystemMgr::GENERAL,
                                      "logger",
                                      new FGLogger);


    ////////////////////////////////////////////////////////////////////
    // Initialize the local time subsystem.
    ////////////////////////////////////////////////////////////////////

    // update the current timezone each 30 minutes
    global_events.Register( "fgUpdateLocalTime()", &fgUpdateLocalTime,
                            30*60*1000 );


    ////////////////////////////////////////////////////////////////////
    // Initialize the weather subsystem.
    ////////////////////////////////////////////////////////////////////

    // Initialize the weather modeling subsystem
#ifdef FG_WEATHERCM
    // Initialize the WeatherDatabase
    SG_LOG(SG_GENERAL, SG_INFO, "Creating LocalWeatherDatabase");
    sgVec3 position;
    sgSetVec3( position, current_aircraft.fdm_state->get_Latitude(),
               current_aircraft.fdm_state->get_Longitude(),
               current_aircraft.fdm_state->get_Altitude() * SG_FEET_TO_METER );
    double init_vis = fgGetDouble("/environment/visibility-m");

    FGLocalWeatherDatabase::DatabaseWorkingType working_type;

    if (!strcmp(fgGetString("/environment/weather/working-type"), "internet"))
    {
      working_type = FGLocalWeatherDatabase::use_internet;
    } else {
      working_type = FGLocalWeatherDatabase::default_mode;
    }
    
    if ( init_vis > 0 ) {
      FGLocalWeatherDatabase::theFGLocalWeatherDatabase = 
        new FGLocalWeatherDatabase( position,
                                    globals->get_fg_root(),
                                    working_type,
                                    init_vis );
    } else {
      FGLocalWeatherDatabase::theFGLocalWeatherDatabase = 
        new FGLocalWeatherDatabase( position,
                                    globals->get_fg_root(),
                                    working_type );
    }

    // cout << theFGLocalWeatherDatabase << endl;
    // cout << "visibility = " 
    //      << theFGLocalWeatherDatabase->getWeatherVisibility() << endl;

    WeatherDatabase = FGLocalWeatherDatabase::theFGLocalWeatherDatabase;

    // register the periodic update of the weather
    global_events.Register( "weather update", &fgUpdateWeatherDatabase,
                            30000);
#else
    globals->get_environment_mgr()->init();
    globals->get_environment_mgr()->bind();
#endif

    ////////////////////////////////////////////////////////////////////
    // Initialize the 3D cloud subsystem.
    ////////////////////////////////////////////////////////////////////
    if ( fgGetBool("/sim/rendering/clouds3d") ) {
        SGPath cloud_path(globals->get_fg_root());
        cloud_path.append("large.sky");
        SG_LOG(SG_GENERAL, SG_INFO, "Loading CLOUDS3d from: " << cloud_path.c_str());
        if ( !sgCloud3d->Load( cloud_path.str(),
                               latitude->getDoubleValue(),
                               longitude->getDoubleValue()) )
        {
            fgSetBool("/sim/rendering/clouds3d", false);
            SG_LOG(SG_GENERAL, SG_INFO, "CLOUDS3d FAILED: ");
        }
        SG_LOG(SG_GENERAL, SG_INFO, "CLOUDS3d Loaded: ");
    }

    ////////////////////////////////////////////////////////////////////
    // Initialize vor/ndb/ils/fix list management and query systems
    ////////////////////////////////////////////////////////////////////

    SG_LOG(SG_GENERAL, SG_INFO, "Loading Navaids");

    SG_LOG(SG_GENERAL, SG_INFO, "  VOR/NDB");
    current_navlist = new FGNavList;
    SGPath p_nav( globals->get_fg_root() );
    p_nav.append( "Navaids/default.nav" );
    current_navlist->init( p_nav );

    SG_LOG(SG_GENERAL, SG_INFO, "  ILS and Marker Beacons");
    current_beacons = new FGMarkerBeacons;
    current_beacons->init();
    current_ilslist = new FGILSList;
    SGPath p_ils( globals->get_fg_root() );
    p_ils.append( "Navaids/default.ils" );
    current_ilslist->init( p_ils );

    SG_LOG(SG_GENERAL, SG_INFO, "  Fixes");
    current_fixlist = new FGFixList;
    SGPath p_fix( globals->get_fg_root() );
    p_fix.append( "Navaids/default.fix" );
    current_fixlist->init( p_fix );

    ////////////////////////////////////////////////////////////////////
    // Initialize ATC list management and query systems
    ////////////////////////////////////////////////////////////////////

    SG_LOG(SG_GENERAL, SG_INFO, "  ATIS");
    current_atislist = new FGATISList;
    SGPath p_atis( globals->get_fg_root() );
    p_atis.append( "ATC/default.atis" );
    current_atislist->init( p_atis );

    SG_LOG(SG_GENERAL, SG_INFO, "  Tower");
    current_towerlist = new FGTowerList;
    SGPath p_tower( globals->get_fg_root() );
    p_tower.append( "ATC/default.tower" );
    current_towerlist->init( p_tower );

    SG_LOG(SG_GENERAL, SG_INFO, "  Approach");
    current_approachlist = new FGApproachList;
    SGPath p_approach( globals->get_fg_root() );
    p_approach.append( "ATC/default.approach" );
    current_approachlist->init( p_approach );

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

    if (fgGetBool("/sim/ai-traffic/enabled")) {
        SG_LOG(SG_GENERAL, SG_INFO, "  AI Manager");
        globals->set_AI_mgr(new FGAIMgr);
        globals->get_AI_mgr()->init();
    }

    ////////////////////////////////////////////////////////////////////
    // Initialize the built-in commands.
    ////////////////////////////////////////////////////////////////////
    fgInitCommands();


#ifdef ENABLE_AUDIO_SUPPORT
    ////////////////////////////////////////////////////////////////////
    // Initialize the sound subsystem.
    ////////////////////////////////////////////////////////////////////

    globals->set_soundmgr(new FGSoundMgr);
    globals->get_soundmgr()->init();
    globals->get_soundmgr()->bind();


    ////////////////////////////////////////////////////////////////////
    // Initialize the sound-effects subsystem.
    ////////////////////////////////////////////////////////////////////

    globals->get_subsystem_mgr()->add(FGSubsystemMgr::GENERAL,
                                      "fx",
                                      new FGFX);
    

#endif

    globals->get_subsystem_mgr()->add(FGSubsystemMgr::GENERAL,
                                      "instrumentation",
                                      new FGInstrumentMgr);
    globals->get_subsystem_mgr()->add(FGSubsystemMgr::GENERAL,
                                      "systems",
                                      new FGSystemMgr);

    ////////////////////////////////////////////////////////////////////
    // Initialize the radio stack subsystem.
    ////////////////////////////////////////////////////////////////////

                                // A textbook example of how FGSubsystem
                                // should work...
    current_radiostack = new FGRadioStack;
    current_radiostack->init();
    current_radiostack->bind();


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

    globals->set_autopilot(new FGAutopilot);
    globals->get_autopilot()->init();
    globals->get_autopilot()->bind();

                                // FIXME: these should go in the
                                // GUI initialization code, not here.
    fgAPAdjustInit();
    NewTgtAirportInit();
    NewHeadingInit();
    NewAltitudeInit();

    ////////////////////////////////////////////////////////////////////
    // Initialize I/O subsystem.
    ////////////////////////////////////////////////////////////////////

    globals->get_io()->init();
    globals->get_io()->bind();

    // Initialize the 2D panel.
    string panel_path = fgGetString("/sim/panel/path",
                                    "Panels/Default/default.xml");
    current_panel = fgReadPanel(panel_path);
    if (current_panel == 0) {
        SG_LOG( SG_INPUT, SG_ALERT, 
                "Error reading new panel from " << panel_path );
    } else {
        SG_LOG( SG_INPUT, SG_INFO, "Loaded new panel from " << panel_path );
        current_panel->init();
        current_panel->bind();
    }

    
    ////////////////////////////////////////////////////////////////////
    // Initialize the default (kludged) properties.
    ////////////////////////////////////////////////////////////////////

    fgInitProps();


    ////////////////////////////////////////////////////////////////////
    // Initialize the controls subsystem.
    ////////////////////////////////////////////////////////////////////

    globals->get_controls()->init();
    globals->get_controls()->bind();


    ////////////////////////////////////////////////////////////////////
    // Initialize the steam subsystem.
    ////////////////////////////////////////////////////////////////////

    globals->get_steam()->init();
    globals->get_steam()->bind();


    ////////////////////////////////////////////////////////////////////
    // Initialize the input subsystem.
    ////////////////////////////////////////////////////////////////////

    globals->get_subsystem_mgr()->add(FGSubsystemMgr::GENERAL,
                                      "input",
                                      new FGInput);


    ////////////////////////////////////////////////////////////////////
    // Bind and initialize subsystems.
    ////////////////////////////////////////////////////////////////////

    globals->get_subsystem_mgr()->bind();
    globals->get_subsystem_mgr()->init();


    ////////////////////////////////////////////////////////////////////////
    // End of subsystem initialization.
    ////////////////////////////////////////////////////////////////////

    SG_LOG( SG_GENERAL, SG_INFO, endl);

                                // Save the initial state for future
                                // reference.
    globals->saveInitialState();

    return true;
}


void fgReInitSubsystems( void )
{
    static const SGPropertyNode *longitude
        = fgGetNode("/position/longitude-deg");
    static const SGPropertyNode *latitude
        = fgGetNode("/position/latitude-deg");
    static const SGPropertyNode *altitude
        = fgGetNode("/position/altitude-ft");
    static const SGPropertyNode *master_freeze
        = fgGetNode("/sim/freeze/master");

    SG_LOG( SG_GENERAL, SG_INFO,
            "fgReInitSubsystems(): /position/altitude = "
            << altitude->getDoubleValue() );

    bool freeze = master_freeze->getBoolValue();
    if ( !freeze ) {
        fgSetBool("/sim/freeze/master", true);
    }
    
    // Initialize the Scenery Management subsystem
    // FIXME, what really needs to get initialized here, at the time
    // this was commented out, scenery.init() was a noop
    // scenery.init();

    fgInitFDM();
    
    // allocates structures so must happen before any of the flight
    // model or control parameters are set
    fgAircraftInit();   // In the future this might not be the case.

    // copy viewer settings into current-view path
    globals->get_viewmgr()->copyToCurrent();

    fgInitView();

    globals->get_controls()->reset_all();
    globals->get_autopilot()->reset();

    fgUpdateSunPos();
    fgUpdateMoonPos();
    cur_light_params.Update();
    fgUpdateLocalTime();

    if ( !freeze ) {
        fgSetBool("/sim/freeze/master", false);
    }
}

