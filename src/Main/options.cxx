// options.cxx -- class to handle command line options
//
// Written by Curtis Olson, started April 1998.
//
// Copyright (C) 1998  Curtis L. Olson  - curt@me.umn.edu
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
// $Id$


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#if defined(FX) && defined(XMESA)
bool global_fullscreen = true;
#endif

#include <simgear/compiler.h>

#include <math.h>            // rint()
#include <stdio.h>
#include <stdlib.h>          // atof(), atoi()
#include <string.h>

#include STL_STRING

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/fgstream.hxx>
#include <simgear/misc/props.hxx>
#include <simgear/timing/sg_time.hxx>

#include <Include/general.hxx>
#include <Airports/simple.hxx>
#include <Cockpit/cockpit.hxx>
#include <FDM/flight.hxx>
#include <FDM/UIUCModel/uiuc_aircraftdir.h>
#ifdef FG_NETWORK_OLK
#  include <NetworkOLK/network.h>
#endif

#include "fg_init.hxx"
#include "globals.hxx"
#include "options.hxx"

FG_USING_STD(string);
FG_USING_NAMESPACE(std);

// from main.cxx
extern void fgReshape( int width, int height );

inline double
atof( const string& str )
{

#ifdef __MWERKS__ 
    // -dw- if ::atof is called, then we get an infinite loop
    return std::atof( str.c_str() );
#else
    return ::atof( str.c_str() );
#endif
}

inline int
atoi( const string& str )
{
#ifdef __MWERKS__ 
    // -dw- if ::atoi is called, then we get an infinite loop
    return std::atoi( str.c_str() );
#else
    return ::atoi( str.c_str() );
#endif
}


// Defined the shared options class here
// FGOptions current_options;

#define PROP(name) (globals->get_props()->getValue(name, true))


// Constructor
FGOptions::FGOptions() 
{
    char* envp = ::getenv( "FG_ROOT" );

    if ( envp != NULL ) {
	// fg_root could be anywhere, so default to environmental
	// variable $FG_ROOT if it is set.
        globals->set_fg_root(envp);
    } else {
	// Otherwise, default to a random compiled-in location if
	// $FG_ROOT is not set.  This can still be overridden from the
	// command line or a config file.

#if defined( WIN32 )
	globals->set_fg_root("\\FlightGear");
#elif defined( macintosh )
	globals->set_fg_root("");
#else
	globals->set_fg_root(PKGLIBDIR);
#endif
    }

    // set a possibly independent location for scenery data
    envp = ::getenv( "FG_SCENERY" );

    if ( envp != NULL ) {
	// fg_root could be anywhere, so default to environmental
	// variable $FG_ROOT if it is set.
        globals->set_fg_scenery(envp);
    } else {
	// Otherwise, default to Scenery being in $FG_ROOT/Scenery
	globals->set_fg_scenery("");
    }
}

void
FGOptions::init ()
{
				// These are all deprecated, and will
				// be removed gradually.
  airport_id = PROP("/sim/startup/airport-id");
  lon = PROP("/position/longitude");
  lat = PROP("/position/latitude");
  altitude = PROP("/position/altitude");
  heading = PROP("/orientation/heading");
  roll = PROP("/orientation/roll");
  pitch = PROP("/orientation/pitch");
  speedset = PROP("/sim/startup/speed-set");
  uBody = PROP("/velocities/uBody");
  vBody = PROP("/velocities/vBody");
  wBody = PROP("/velocities/wBody");
  vNorth = PROP("/velocities/speed-north");
  vEast = PROP("/velocities/speed-east");
  vDown = PROP("/velocities/speed-down");
  vkcas = PROP("/velocities/knots");
  mach = PROP("/velocities/mach");
  game_mode = PROP("/sim/startup/game-mode");
  splash_screen = PROP("/sim/startup/splash-screen");
  intro_music = PROP("/sim/startup/intro-music");
  mouse_pointer = PROP("/sim/startup/mouse-pointer");
  control_mode = PROP("/sim/control-mode");
  auto_coordination = PROP("/sim/auto-coordination");
  hud_status = PROP("/sim/hud/visibility");
  panel_status = PROP("/sim/panel/visibility");
  sound = PROP("/sim/sound");
  anti_alias_hud = PROP("/sim/hud/antialiased");
  flight_model = PROP("/sim/flight-model");
  aircraft = PROP("/sim/aircraft");
  model_hz = PROP("/sim/model-hz");
  speed_up = PROP("/sim/speed-up");
  trim = PROP("/sim/startup/trim");
  fog = PROP("/sim/rendering/fog");
  clouds = PROP("/environment/clouds/enabled");
  clouds_asl = PROP("/environments/clouds/altitude");
  fullscreen = PROP("/sim/startup/fullscreen");
  shading = PROP("/sim/rendering/shading");
  skyblend = PROP("/sim/rendering/skyblend");
  textures = PROP("/sim/rendering/textures");
  wireframe = PROP("/sim/rendering/wireframe");
  xsize = PROP("/sim/startup/xsize");
  ysize = PROP("/sim/startup/ysize");
  bpp = PROP("/sim/rendering/bits-per-pixel");
  view_mode = PROP("/sim/view-mode");
  default_view_offset = PROP("/sim/startup/view-offset");
  visibility = PROP("/environment/visibility");
  units = PROP("/sim/startup/units");
  tris_or_culled = PROP("/sim/hud/frame-stat-type");
  time_offset = PROP("/sim/startup/time-offset");
  time_offset_type = PROP("/sim/startup/time-offset-type");
  network_olk = PROP("/sim/networking/network-olk");
  net_id = PROP("/sim/networking/call-sign");
}


/**
 * Set a few fail-safe default property values.
 *
 * These should all be set in $FG_ROOT/preferences.xml, but just
 * in case, we provide some initial sane values here. This method 
 * should be invoked *before* reading any init files.
 */
void
FGOptions::set_default_props ()
{
  SGPropertyNode * props = globals->get_props();

				// Position (Globe, AZ)
  props->setDoubleValue("/position/longitude", -110.6642444);
  props->setDoubleValue("/position/latitude", 33.3528917);
  props->setDoubleValue("/position/altitude", -9999.0);

				// Orientation
  props->setDoubleValue("/orientation/heading", 270);
  props->setDoubleValue("/orientation/roll", 0);
  props->setDoubleValue("/orientation/pitch", 0.424);

				// Velocities
  props->setStringValue("/sim/startup/speed-set", "knots");
  props->setDoubleValue("/velocities/uBody", 0.0);
  props->setDoubleValue("/velocities/vBody", 0.0);
  props->setDoubleValue("/velocities/wBody", 0.0);
  props->setDoubleValue("/velocities/speed-north", 0.0);
  props->setDoubleValue("/velocities/speed-east", 0.0);
  props->setDoubleValue("/velocities/speed-down", 0.0);
  props->setDoubleValue("/velocities/airspeed", 0.0);
  props->setDoubleValue("/velocities/mach", 0.0);

				// Miscellaneous
  props->setBoolValue("/sim/startup/game-mode", false);
  props->setBoolValue("/sim/startup/splash-screen", true);
  props->setBoolValue("/sim/startup/intro-music", true);
  props->setStringValue("/sim/startup/mouse-pointer", "disabled");
  props->setStringValue("/sim/control-mode", "joystick");
  props->setBoolValue("/sim/auto-coordination", false);

				// Features
  props->setBoolValue("/sim/hud/visibility", false);
  props->setBoolValue("/sim/panel/visibility", true);
  props->setBoolValue("/sim/sound", true);
  props->setBoolValue("/sim/hud/antialiased", false);

				// Flight Model options
  props->setStringValue("/sim/flight-model", "larcsim");
  props->setStringValue("/sim/aircraft", "c172");
  props->setIntValue("/sim/model-hz", NEW_DEFAULT_MODEL_HZ);
  props->setIntValue("/sim/speed-up", 1);
  props->setBoolValue("/sim/startup/trim", false);

				// Rendering options
  props->setStringValue("/sim/rendering/fog", "nicest");
  props->setBoolValue("/environment/clouds/enabled", true);
  props->setDoubleValue("/environment/clouds/altitude", 5000);
  props->setBoolValue("/sim/startup/fullscreen", false);
  props->setBoolValue("/sim/rendering/shading", true);
  props->setBoolValue("/sim/rendering/skyblend", true);
  props->setBoolValue("/sim/rendering/textures", true);
  props->setBoolValue("/sim/rendering/wireframe", false);
  props->setIntValue("/sim/startup/xsize", 800);
  props->setIntValue("/sim/startup/ysize", 600);
  props->setIntValue("/sim/rendering/bits-per-pixel", 16);
  props->setIntValue("/sim/view-mode", (int)FG_VIEW_PILOT);
  props->setDoubleValue("/sim/startup/view-offset", 0);
  props->setDoubleValue("/environment/visibility", 20000);

				// HUD options
  props->setStringValue("/sim/startup/units", "feet");
  props->setStringValue("/sim/hud/frame-stat-type", "tris");
	
				// Time options
  props->setIntValue("/sim/startup/time-offset", 0);

  props->setBoolValue("/sim/networking/network-olk", false);
  props->setStringValue("/sim/networking/call-sign", "Johnny");
}

void 
FGOptions::toggle_panel() {
    SGPropertyNode * props = globals->get_props();
    bool freeze = globals->get_freeze();

    if( !freeze )
        globals->set_freeze(true);
    
    if(props->getBoolValue("/sim/panel/visibility"))
        props->setBoolValue("/sim/panel/visibility", false);
    else
      props->setBoolValue("/sim/panel/visibility", true);

    fgReshape(props->getIntValue("/sim/startup/xsize"),
	      props->getIntValue("/sim/startup/ysize"));

    if( !freeze )
        globals->set_freeze( false );
}

double
FGOptions::parse_time(const string& time_in) {
    char *time_str, num[256];
    double hours, minutes, seconds;
    double result = 0.0;
    int sign = 1;
    int i;

    time_str = (char *)time_in.c_str();

    // printf("parse_time(): %s\n", time_str);

    // check for sign
    if ( strlen(time_str) ) {
	if ( time_str[0] == '+' ) {
	    sign = 1;
	    time_str++;
	} else if ( time_str[0] == '-' ) {
	    sign = -1;
	    time_str++;
	}
    }
    // printf("sign = %d\n", sign);

    // get hours
    if ( strlen(time_str) ) {
	i = 0;
	while ( (time_str[0] != ':') && (time_str[0] != '\0') ) {
	    num[i] = time_str[0];
	    time_str++;
	    i++;
	}
	if ( time_str[0] == ':' ) {
	    time_str++;
	}
	num[i] = '\0';
	hours = atof(num);
	// printf("hours = %.2lf\n", hours);

	result += hours;
    }

    // get minutes
    if ( strlen(time_str) ) {
	i = 0;
	while ( (time_str[0] != ':') && (time_str[0] != '\0') ) {
	    num[i] = time_str[0];
	    time_str++;
	    i++;
	}
	if ( time_str[0] == ':' ) {
	    time_str++;
	}
	num[i] = '\0';
	minutes = atof(num);
	// printf("minutes = %.2lf\n", minutes);

	result += minutes / 60.0;
    }

    // get seconds
    if ( strlen(time_str) ) {
	i = 0;
	while ( (time_str[0] != ':') && (time_str[0] != '\0') ) {
	    num[i] = time_str[0];
	    time_str++;
	    i++;
	}
	num[i] = '\0';
	seconds = atof(num);
	// printf("seconds = %.2lf\n", seconds);

	result += seconds / 3600.0;
    }

    return(sign * result);
}


long int FGOptions::parse_date( const string& date)
{
    struct tm gmt;
    char * date_str, num[256];
    int i;
    // initialize to zero
    gmt.tm_sec = 0;
    gmt.tm_min = 0;
    gmt.tm_hour = 0;
    gmt.tm_mday = 0;
    gmt.tm_mon = 0;
    gmt.tm_year = 0;
    gmt.tm_isdst = 0; // ignore daylight savings time for the moment
    date_str = (char *)date.c_str();
    // get year
    if ( strlen(date_str) ) {
	i = 0;
	while ( (date_str[0] != ':') && (date_str[0] != '\0') ) {
	    num[i] = date_str[0];
	    date_str++;
	    i++;
	}
	if ( date_str[0] == ':' ) {
	    date_str++;
	}
	num[i] = '\0';
	gmt.tm_year = atoi(num) - 1900;
    }
    // get month
    if ( strlen(date_str) ) {
	i = 0;
	while ( (date_str[0] != ':') && (date_str[0] != '\0') ) {
	    num[i] = date_str[0];
	    date_str++;
	    i++;
	}
	if ( date_str[0] == ':' ) {
	    date_str++;
	}
	num[i] = '\0';
	gmt.tm_mon = atoi(num) -1;
    }
    // get day
    if ( strlen(date_str) ) {
	i = 0;
	while ( (date_str[0] != ':') && (date_str[0] != '\0') ) {
	    num[i] = date_str[0];
	    date_str++;
	    i++;
	}
	if ( date_str[0] == ':' ) {
	    date_str++;
	}
	num[i] = '\0';
	gmt.tm_mday = atoi(num);
    }
    // get hour
    if ( strlen(date_str) ) {
	i = 0;
	while ( (date_str[0] != ':') && (date_str[0] != '\0') ) {
	    num[i] = date_str[0];
	    date_str++;
	    i++;
	}
	if ( date_str[0] == ':' ) {
	    date_str++;
	}
	num[i] = '\0';
	gmt.tm_hour = atoi(num);
    }
    // get minute
    if ( strlen(date_str) ) {
	i = 0;
	while ( (date_str[0] != ':') && (date_str[0] != '\0') ) {
	    num[i] = date_str[0];
	    date_str++;
	    i++;
	}
	if ( date_str[0] == ':' ) {
	    date_str++;
	}
	num[i] = '\0';
	gmt.tm_min = atoi(num);
    }
    // get second
    if ( strlen(date_str) ) {
	i = 0;
	while ( (date_str[0] != ':') && (date_str[0] != '\0') ) {
	    num[i] = date_str[0];
	    date_str++;
	    i++;
	}
	if ( date_str[0] == ':' ) {
	    date_str++;
	}
	num[i] = '\0';
	gmt.tm_sec = atoi(num);
    }
    time_t theTime = sgTimeGetGMT( gmt.tm_year, gmt.tm_mon, gmt.tm_mday,
				   gmt.tm_hour, gmt.tm_min, gmt.tm_sec );
    //printf ("Date is %s\n", ctime(&theTime));
    //printf ("in seconds that is %d\n", theTime);
    //exit(1);
    return (theTime);
}


/// parse degree in the form of [+/-]hhh:mm:ss
double
FGOptions::parse_degree( const string& degree_str) {
    double result = parse_time( degree_str );

    // printf("Degree = %.4f\n", result);

    return(result);
}


// parse time offset command line option
int
FGOptions::parse_time_offset( const string& time_str) {
    int result;

    // printf("time offset = %s\n", time_str);

#ifdef HAVE_RINT
    result = (int)rint(parse_time(time_str) * 3600.0);
#else
    result = (int)(parse_time(time_str) * 3600.0);
#endif

    // printf("parse_time_offset(): %d\n", result);

    return( result );
}


// Parse --fdm=abcdefg type option 
int
FGOptions::parse_fdm( const string& fm ) const {
    // cout << "fdm = " << fm << endl;

    if ( fm == "ada" ) {
	return FGInterface::FG_ADA;
    } else if ( fm == "balloon" ) {
	return FGInterface::FG_BALLOONSIM;
    } else if ( fm == "external" ) {
	return FGInterface::FG_EXTERNAL;
    } else if ( fm == "jsb" ) {
	return FGInterface::FG_JSBSIM;
    } else if ( (fm == "larcsim") || (fm == "LaRCsim") ) {
	return FGInterface::FG_LARCSIM;
    } else if ( fm == "magic" ) {
	return FGInterface::FG_MAGICCARPET;
    } else {
	FG_LOG( FG_GENERAL, FG_ALERT, "Unknown fdm = " << fm );
	exit(-1);
    }

    // we'll never get here, but it makes the compiler happy.
    return -1;
}


// Parse --fov=x.xx type option 
double
FGOptions::parse_fov( const string& arg ) {
    double fov = atof(arg);

    if ( fov < FG_FOV_MIN ) { fov = FG_FOV_MIN; }
    if ( fov > FG_FOV_MAX ) { fov = FG_FOV_MAX; }

    globals->get_props()->setDoubleValue("/sim/field-of-view", fov);

    // printf("parse_fov(): result = %.4f\n", fov);

    return fov;
}


// Parse I/O channel option
//
// Format is "--protocol=medium,direction,hz,medium_options,..."
//
//   protocol = { native, nmea, garmin, fgfs, rul, pve, etc. }
//   medium = { serial, socket, file, etc. }
//   direction = { in, out, bi }
//   hz = number of times to process channel per second (floating
//        point values are ok.
//
// Serial example "--nmea=serial,dir,hz,device,baud" where
// 
//  device = OS device name of serial line to be open()'ed
//  baud = {300, 1200, 2400, ..., 230400}
//
// Socket exacmple "--native=socket,dir,hz,machine,port,style" where
// 
//  machine = machine name or ip address if client (leave empty if server)
//  port = port, leave empty to let system choose
//  style = tcp or udp
//
// File example "--garmin=file,dir,hz,filename" where
// 
//  filename = file system file name

bool 
FGOptions::parse_channel( const string& type, const string& channel_str ) {
    // cout << "Channel string = " << channel_str << endl;

    globals->get_channel_options_list().push_back( type + "," + channel_str );

    return true;
}


// Parse --wp=ID[,alt]
bool FGOptions::parse_wp( const string& arg ) {
    string id, alt_str;
    double alt = 0.0;

    int pos = arg.find( "@" );
    if ( pos != string::npos ) {
	id = arg.substr( 0, pos );
	alt_str = arg.substr( pos + 1 );
	// cout << "id str = " << id << "  alt str = " << alt_str << endl;
	alt = atof( alt_str.c_str() );
	if ( globals->get_props()->getStringValue("/sim/startup/units")
	     == "feet" ) {
	    alt *= FEET_TO_METER;
	}
    } else {
	id = arg;
    }

    FGAirport a;
    if ( fgFindAirportID( id, &a ) ) {
	SGWayPoint wp( a.longitude, a.latitude, alt, SGWayPoint::WGS84, id );
	globals->get_route()->add_waypoint( wp );

	return true;
    } else {
	return false;
    }
}


// Parse --flight-plan=[file]
bool FGOptions::parse_flightplan(const string& arg)
{
    fg_gzifstream infile(arg.c_str());
    if (!infile) {
	return false;
    }
    while ( true ) {
	string line;
#ifdef GETLINE_NEEDS_TERMINATOR
	getline( infile, line, '\n' );
#elif defined( macintosh )
	getline( infile, line, '\r' );
#else
	getline( infile, line );
#endif
	if ( infile.eof() ) {
	    break;
	}
	parse_wp(line);
    }

    return true;
}


// Parse a single option
int FGOptions::parse_option( const string& arg ) {
    SGPropertyNode * props = globals->get_props();
    // General Options
    if ( (arg == "--help") || (arg == "-h") ) {
	// help/usage request
	return(FG_OPTIONS_HELP);
    } else if ( arg == "--disable-game-mode") {
	props->setBoolValue("/sim/startup/game-mode", false);
    } else if ( arg == "--enable-game-mode" ) {
	props->setBoolValue("/sim/startup/game-mode", true);
    } else if ( arg == "--disable-splash-screen" ) {
	props->setBoolValue("/sim/startup/splash-screen", false); 
    } else if ( arg == "--enable-splash-screen" ) {
        props->setBoolValue("/sim/startup/splash-screen", true);
    } else if ( arg == "--disable-intro-music" ) {
	props->setBoolValue("/sim/startup/intro-music", false);
    } else if ( arg == "--enable-intro-music" ) {
	props->setBoolValue("/sim/startup/intro-music", true);
    } else if ( arg == "--disable-mouse-pointer" ) {
	props->setStringValue("/sim/startup/mouse-pointer", "disabled");
    } else if ( arg == "--enable-mouse-pointer" ) {
	props->setStringValue("/sim/startup/mouse-pointer", "enabled");
    } else if ( arg == "--disable-freeze" ) {
	props->setBoolValue("/sim/freeze", false);
    } else if ( arg == "--enable-freeze" ) {
	props->setBoolValue("/sim/freeze", true);
    } else if ( arg == "--disable-anti-alias-hud" ) {
	props->setBoolValue("/sim/hud/antialiased", false);
    } else if ( arg == "--enable-anti-alias-hud" ) {
        props->setBoolValue("/sim/hud/antialiased", true);
    } else if ( arg.find( "--control=") != string::npos ) {
        props->setStringValue("/sim/control-mode", arg.substr(10));
    } else if ( arg == "--disable-auto-coordination" ) {
        props->setBoolValue("/sim/auto-coordination", false);
    } else if ( arg == "--enable-auto-coordination" ) {
	props->setBoolValue("/sim/auto-coordination", true);
    } else if ( arg == "--disable-hud" ) {
	props->setBoolValue("/sim/hud/visibility", false);
    } else if ( arg == "--enable-hud" ) {
	props->setBoolValue("/sim/hud/visibility", true);
    } else if ( arg == "--disable-panel" ) {
	props->setBoolValue("/sim/panel/visibility", false);
    } else if ( arg == "--enable-panel" ) {
	props->setBoolValue("/sim/panel/visibility", true);
    } else if ( arg == "--disable-sound" ) {
	props->setBoolValue("/sim/sound", false);
    } else if ( arg == "--enable-sound" ) {
	props->setBoolValue("/sim/sound", true);
    } else if ( arg.find( "--airport-id=") != string::npos ) {
				// NB: changed property name!!!
	props->setStringValue("/sim/startup/airport-id", arg.substr(13));
    } else if ( arg.find( "--lon=" ) != string::npos ) {
	props->setDoubleValue("/position/longitude",
			      parse_degree(arg.substr(6)));
	props->setStringValue("/position/airport-id", "");
    } else if ( arg.find( "--lat=" ) != string::npos ) {
	props->setDoubleValue("/position/latitude",
			      parse_degree(arg.substr(6)));
	props->setStringValue("/position/airport-id", "");
    } else if ( arg.find( "--altitude=" ) != string::npos ) {
	if ( props->getStringValue("/sim/startup/units") == "feet" )
	    props->setDoubleValue(atof(arg.substr(11)));
	else
	    props->setDoubleValue(atof(arg.substr(11)) * METER_TO_FEET);
    } else if ( arg.find( "--uBody=" ) != string::npos ) {
        props->setStringValue("/sim/startup/speed-set", "UVW");
				// FIXME: the units are totally confused here
	if ( props->getStringValue("/sim/startup/units") == "feet" )
	  props->setDoubleValue("/velocities/uBody", atof(arg.substr(8)));
	else
	  props->setDoubleValue("/velocities/uBody",
			       atof(arg.substr(8)) * FEET_TO_METER);
    } else if ( arg.find( "--vBody=" ) != string::npos ) {
        props->setStringValue("/sim/startup/speed-set", "UVW");
				// FIXME: the units are totally confused here
	if ( props->getStringValue("/sim/startup/units") == "feet" )
	  props->setDoubleValue("/velocities/vBody", atof(arg.substr(8)));
	else
	  props->setDoubleValue("/velocities/vBody",
			       atof(arg.substr(8)) * FEET_TO_METER);
    } else if ( arg.find( "--wBody=" ) != string::npos ) {
        props->setStringValue("/sim/startup/speed-set", "UVW");
				// FIXME: the units are totally confused here
	if ( props->getStringValue("/sim/startup/units") == "feet" )
	  props->setDoubleValue("/velocities/wBody", atof(arg.substr(8)));
	else
	  props->setDoubleValue("/velocities/wBody",
			       atof(arg.substr(8)) * FEET_TO_METER);
    } else if ( arg.find( "--vNorth=" ) != string::npos ) {
        props->setStringValue("/sim/startup/speed-set", "NED");
				// FIXME: the units are totally confused here
	if ( props->getStringValue("/sim/startup/units") == "feet" )
	  props->setDoubleValue("/velocities/speed-north", atof(arg.substr(8)));
	else
	  props->setDoubleValue("/velocities/speed-north",
			       atof(arg.substr(8)) * FEET_TO_METER);
    } else if ( arg.find( "--vEast=" ) != string::npos ) {
        props->setStringValue("/sim/startup/speed-set", "NED");
				// FIXME: the units are totally confused here
	if ( props->getStringValue("/sim/startup/units") == "feet" )
	  props->setDoubleValue("/velocities/speed-east", atof(arg.substr(8)));
	else
	  props->setDoubleValue("/velocities/speed-east",
			       atof(arg.substr(8)) * FEET_TO_METER);
    } else if ( arg.find( "--vDown=" ) != string::npos ) {
        props->setStringValue("/sim/startup/speed-set", "NED");
				// FIXME: the units are totally confused here
	if ( props->getStringValue("/sim/startup/units") == "feet" )
	  props->setDoubleValue("/velocities/speed-down", atof(arg.substr(8)));
	else
	  props->setDoubleValue("/velocities/speed-down",
			       atof(arg.substr(8)) * FEET_TO_METER);
    } else if ( arg.find( "--vc=" ) != string::npos) {
        props->setStringValue("/sim/startup/speed-set", "knots");
	props->setDoubleValue("/velocities/airspeed", atof(arg.substr(5)));
    } else if ( arg.find( "--mach=" ) != string::npos) {
        props->setStringValue("/sim/startup/speed-set", "mach");
	props->setDoubleValue("/velocities/mach", atof(arg.substr(7)));
    } else if ( arg.find( "--heading=" ) != string::npos ) {
	props->setDoubleValue("/orientation/heading", atof(arg.substr(10)));
    } else if ( arg.find( "--roll=" ) != string::npos ) {
	props->setDoubleValue("/orientation/roll", atof(arg.substr(7)));
    } else if ( arg.find( "--pitch=" ) != string::npos ) {
	props->setDoubleValue("/orientation/pitch", atof(arg.substr(8)));
    } else if ( arg.find( "--fg-root=" ) != string::npos ) {
	globals->set_fg_root(arg.substr( 10 ));
    } else if ( arg.find( "--fg-scenery=" ) != string::npos ) {
        globals->set_fg_scenery(arg.substr( 13 ));
    } else if ( arg.find( "--fdm=" ) != string::npos ) {
	props->setStringValue("/sim/flight-model", arg.substr(6));
				// FIXME: reimplement this logic, somehow
//     if((flight_model == FGInterface::FG_JSBSIM) && (get_trim_mode() == 0)) {
//         props->
// 	set_trim_mode(1);
//     } else {
// 	set_trim_mode(0);
//     }        
    } else if ( arg.find( "--aircraft=" ) != string::npos ) {
	props->setStringValue("/sim/aircraft", arg.substr(11));
    } else if ( arg.find( "--aircraft-dir=" ) != string::npos ) {
        props->setStringValue("/sim/aircraft-dir", arg.substr(15));
    } else if ( arg.find( "--model-hz=" ) != string::npos ) {
	props->setIntValue("/sim/model-hz", atoi(arg.substr(11)));
    } else if ( arg.find( "--speed=" ) != string::npos ) {
	props->setIntValue("/sim/speed-up", atoi(arg.substr(8)));
    } else if ( arg.find( "--notrim") != string::npos) {
        props->setIntValue("/sim/startup/trim", -1);
    } else if ( arg == "--fog-disable" ) {
	props->setStringValue("/sim/rendering/fog", "disabled");
    } else if ( arg == "--fog-fastest" ) {
	props->setStringValue("/sim/rendering/fog", "fastest");
    } else if ( arg == "--fog-nicest" ) {
	props->setStringValue("/sim/fog", "nicest");
    } else if ( arg == "--disable-clouds" ) {
        props->setBoolValue("/environment/clouds/enabled", false);
    } else if ( arg == "--enable-clouds" ) {
        props->setBoolValue("/environment/clouds/enabled", true);
    } else if ( arg.find( "--clouds-asl=" ) != string::npos ) {
				// FIXME: check units
        if ( props->getStringValue("/sim/startup/units") == "feet" )
	  props->setDoubleValue("/environment/clouds/altitude",
				atof(arg.substr(13)) * FEET_TO_METER);
	else
	  props->setDoubleValue("/environment/clouds/altitude",
				atof(arg.substr(13)));
    } else if ( arg.find( "--fov=" ) != string::npos ) {
	parse_fov( arg.substr(6) );
    } else if ( arg == "--disable-fullscreen" ) {
        props->setBoolValue("/sim/startup/fullscreen", false);
    } else if ( arg== "--enable-fullscreen") {
        props->setBoolValue("/sim/startup/fullscreen", true);
    } else if ( arg == "--shading-flat") {
	props->setBoolValue("/sim/rendering/shading", false);
    } else if ( arg == "--shading-smooth") {
	props->setBoolValue("/sim/rendering/shading", true);
    } else if ( arg == "--disable-skyblend") {
	props->setBoolValue("/sim/rendering/skyblend", false);
    } else if ( arg== "--enable-skyblend" ) {
	props->setBoolValue("/sim/rendering/skyblend", true);
    } else if ( arg == "--disable-textures" ) {
        props->setBoolValue("/sim/rendering/textures", false);
    } else if ( arg == "--enable-textures" ) {
        props->setBoolValue("/sim/rendering/textures", true);
    } else if ( arg == "--disable-wireframe" ) {
        props->setBoolValue("/sim/rendering/wireframe", false);
    } else if ( arg == "--enable-wireframe" ) {
        props->setBoolValue("/sim/rendering/wireframe", true);
    } else if ( arg.find( "--geometry=" ) != string::npos ) {
	bool geometry_ok = true;
	int xsize = 0, ysize = 0;
	string geometry = arg.substr( 11 );
	string::size_type i = geometry.find('x');

 	if (i != string::npos) {
	    xsize = atoi(geometry.substr(0, i));
	    ysize = atoi(geometry.substr(i+1));
  	} else {
	    geometry_ok = false;
 	}

 	if ( xsize <= 0 || ysize <= 0 ) {
	    xsize = 640;
	    ysize = 480;
	    geometry_ok = false;
 	}

 	if ( !geometry_ok ) {
  	    FG_LOG( FG_GENERAL, FG_ALERT, "Unknown geometry: " << geometry );
 	    FG_LOG( FG_GENERAL, FG_ALERT,
 		    "Setting geometry to " << xsize << 'x' << ysize << '\n');
  	} else {
	  FG_LOG( FG_GENERAL, FG_INFO,
		  "Setting geometry to " << xsize << 'x' << ysize << '\n');
	  props->setIntValue("/sim/startup/xsize", xsize);
	  props->setIntValue("/sim/startup/ysize", ysize);
	}
    } else if ( arg.find( "--bpp=" ) != string::npos ) {
	string bits_per_pix = arg.substr( 6 );
	if ( bits_per_pix == "16" ) {
	    props->setIntValue("/sim/rendering/bits-per-pixel", 16);
	} else if ( bits_per_pix == "24" ) {
	    props->setIntValue("/sim/rendering/bits-per-pixel", 24);
	} else if ( bits_per_pix == "32" ) {
	    props->setIntValue("/sim/rendering/bits-per-pixel", 32);
	} else {
	  FG_LOG(FG_GENERAL, FG_ALERT, "Unsupported bpp " << bits_per_pix);
	}
    } else if ( arg == "--units-feet" ) {
	props->setStringValue("/sim/startup/units", "feet");
    } else if ( arg == "--units-meters" ) {
	props->setStringValue("/sim/startup/units", "meters");
    } else if ( arg.find( "--time-offset" ) != string::npos ) {
        props->setIntValue("/sim/startup/time-offset",
			   parse_time_offset( (arg.substr(14)) ));
    } else if ( arg.find( "--time-match-real") != string::npos ) {
        props->setStringValue("/sim/startup/time-offset_type",
			      "system-offset");
    } else if ( arg.find( "--time-match-local") != string::npos ) {
        props->setStringValue("/sim/startup/time-offset_type",
			      "latitude-offset");
    } else if ( arg.find( "--start-date-sys=") != string::npos ) {
        props->setIntValue("/sim/startup/time-offset",
			   parse_date((arg.substr(17))));
	props->setStringValue("/sim/startup/time-offset-type", "system");
    } else if ( arg.find( "--start-date-lat=") != string::npos ) {
        props->setIntValue("/sim/startup/time-offset",
			   parse_date((arg.substr(17))));
	props->setStringValue("/sim/startup/time-offset-type",
			   "latitude");
    } else if ( arg.find( "--start-date-gmt=") != string::npos ) {
        props->setIntValue("/sim/startup/time-offset",
			   parse_date((arg.substr(17))));
	props->setStringValue("/sim/startup/time-offset-type", "gmt");
    } else if ( arg == "--hud-tris" ) {
        props->setStringValue("/sim/hud/frame-stat-type", "tris");
    } else if ( arg == "--hud-culled" ) {
        props->setStringValue("/sim/hud/frame-stat-type", "culled");
    } else if ( arg.find( "--native=" ) != string::npos ) {
	parse_channel( "native", arg.substr(9) );
    } else if ( arg.find( "--garmin=" ) != string::npos ) {
	parse_channel( "garmin", arg.substr(9) );
    } else if ( arg.find( "--nmea=" ) != string::npos ) {
	parse_channel( "nmea", arg.substr(7) );
    } else if ( arg.find( "--props=" ) != string::npos ) {
	parse_channel( "props", arg.substr(8) );
    } else if ( arg.find( "--pve=" ) != string::npos ) {
	parse_channel( "pve", arg.substr(6) );
    } else if ( arg.find( "--ray=" ) != string::npos ) {
	parse_channel( "ray", arg.substr(6) );
    } else if ( arg.find( "--rul=" ) != string::npos ) {
	parse_channel( "rul", arg.substr(6) );
    } else if ( arg.find( "--joyclient=" ) != string::npos ) {
	parse_channel( "joyclient", arg.substr(12) );
#ifdef FG_NETWORK_OLK
    } else if ( arg == "--disable-network-olk" ) {
        props->setBoolValue("/sim/networking/olk", false);
    } else if ( arg== "--enable-network-olk") {
        props->setBoolValue("/sim/networking/olk", true);
    } else if ( arg == "--net-hud" ) {
        props->setBoolValue("/sim/hud/net-display", true);
	net_hud_display = 1;	// FIXME
    } else if ( arg.find( "--net-id=") != string::npos ) {
        props->setStringValue("sim/networking/call-sign", arg.substr(9));
#endif
    } else if ( arg.find( "--prop:" ) == 0 ) {
        string assign = arg.substr(7);
	int pos = assign.find('=');
	if (pos == arg.npos || pos == 0) {
	    FG_LOG(FG_GENERAL, FG_ALERT, "Bad property assignment: " << arg);
	    return FG_OPTIONS_ERROR;
	}
	string name = assign.substr(0, pos);
	string value = assign.substr(pos + 1);
	props->setStringValue(name.c_str(), value);
	FG_LOG(FG_GENERAL, FG_INFO, "Setting default value of property "
	       << name << " to \"" << value << '"');
    // $$$ begin - added VS Renganathan, 14 Oct 2K
    // for multi-window outside window imagery
    } else if ( arg.find( "--view-offset=" ) != string::npos ) {
	string woffset = arg.substr( 14 );
	double default_view_offset = 0.0;
	if ( woffset == "LEFT" ) {
	       default_view_offset = FG_PI * 0.25;
	} else if ( woffset == "RIGHT" ) {
	    default_view_offset = FG_PI * 1.75;
	} else if ( woffset == "CENTER" ) {
	    default_view_offset = 0.00;
	} else {
	    default_view_offset = atof( woffset.c_str() ) * DEG_TO_RAD;
	}
	FGViewerRPH *pilot_view =
	    (FGViewerRPH *)globals->get_viewmgr()->get_view( 0 );
	pilot_view->set_view_offset( default_view_offset );
	pilot_view->set_goal_view_offset( default_view_offset );
	props->setDoubleValue("/sim/startup/view-offset", default_view_offset);
    // $$$ end - added VS Renganathan, 14 Oct 2K
    } else if ( arg.find( "--visibility=" ) != string::npos ) {
	props->setDoubleValue("/environment/visibility", atof(arg.substr(13)));
    } else if ( arg.find( "--visibility-miles=" ) != string::npos ) {
        double visibility = atof(arg.substr(19)) * 5280.0 * FEET_TO_METER;
	props->setDoubleValue("/environment/visibility", visibility);
    } else if ( arg.find( "--wind=" ) == 0 ) {
        string val = arg.substr(7);
	int pos = val.find('@');
	if (pos == string::npos) {
	  FG_LOG(FG_GENERAL, FG_ALERT, "bad wind value " << val);
	  return FG_OPTIONS_ERROR;
	}
	double dir = atof(val.substr(0,pos).c_str());
	double speed = atof(val.substr(pos+1).c_str());
	FG_LOG(FG_GENERAL, FG_INFO, "WIND: " << dir << '@' << 
	       speed << " knots" << endl);
				// convert to fps
	speed *= NM_TO_METER * METER_TO_FEET * (1.0/3600);
	dir += 180;
	if (dir >= 360)
	  dir -= 360;
	dir *= DEG_TO_RAD;
	props->setDoubleValue("/environment/wind-north",
					     speed * cos(dir));
	props->setDoubleValue("/environment/wind-east",
					     speed * sin(dir));
    } else if ( arg.find( "--wp=" ) != string::npos ) {
	parse_wp( arg.substr( 5 ) );
    } else if ( arg.find( "--flight-plan=") != string::npos) {
	parse_flightplan ( arg.substr (14) );
    } else {
	FG_LOG( FG_GENERAL, FG_ALERT, "Unknown option '" << arg << "'" );
	return FG_OPTIONS_ERROR;
    }
    
    return FG_OPTIONS_OK;
}


// Scan the command line options for an fg_root definition and set
// just that.
int FGOptions::scan_command_line_for_root( int argc, char **argv ) {
    int i = 1;

    FG_LOG(FG_GENERAL, FG_INFO, "Processing command line arguments");

    while ( i < argc ) {
	FG_LOG( FG_GENERAL, FG_DEBUG, "argv[" << i << "] = " << argv[i] );

	string arg = argv[i];
	if ( arg.find( "--fg-root=" ) != string::npos ) {
	    globals->set_fg_root(arg.substr( 10 ));
	}

	i++;
    }
    
    return FG_OPTIONS_OK;
}


// Scan the config file for an fg_root definition and set just that.
int FGOptions::scan_config_file_for_root( const string& path ) {
    fg_gzifstream in( path );
    if ( !in.is_open() )
	return(FG_OPTIONS_ERROR);

    FG_LOG( FG_GENERAL, FG_INFO, "Scanning for root: " << path );

    in >> skipcomment;
#ifndef __MWERKS__
    while ( ! in.eof() ) {
#else
    char c = '\0';
    while ( in.get(c) && c != '\0' ) {
	in.putback(c);
#endif
	string line;

#ifdef GETLINE_NEEDS_TERMINATOR
        getline( in, line, '\n' );
#elif defined( macintosh )
	getline( in, line, '\r' );
#else
        getline( in, line );
#endif

	if ( line.find( "--fg-root=" ) != string::npos ) {
	    globals->set_fg_root(line.substr( 10 ));
	}

	in >> skipcomment;
    }

    return FG_OPTIONS_OK;
}


// Parse the command line options
int FGOptions::parse_command_line( int argc, char **argv ) {
    int i = 1;
    int result;

    FG_LOG(FG_GENERAL, FG_INFO, "Processing command line arguments");

    while ( i < argc ) {
	FG_LOG( FG_GENERAL, FG_DEBUG, "argv[" << i << "] = " << argv[i] );

	result = parse_option(argv[i]);
	if ( (result == FG_OPTIONS_HELP) || (result == FG_OPTIONS_ERROR) ) {
	    return(result);
	}

	i++;
    }
    
    return FG_OPTIONS_OK;
}


// Parse config file options
int FGOptions::parse_config_file( const string& path ) {
    fg_gzifstream in( path );
    if ( !in.is_open() )
	return(FG_OPTIONS_ERROR);

    FG_LOG( FG_GENERAL, FG_INFO, "Processing config file: " << path );

    in >> skipcomment;
#ifndef __MWERKS__
    while ( ! in.eof() ) {
#else
    char c = '\0';
    while ( in.get(c) && c != '\0' ) {
	in.putback(c);
#endif
	string line;

#ifdef GETLINE_NEEDS_TERMINATOR
        getline( in, line, '\n' );
#elif defined( macintosh )
	getline( in, line, '\r' );
#else
        getline( in, line );
#endif

	if ( parse_option( line ) == FG_OPTIONS_ERROR ) {
	    FG_LOG( FG_GENERAL, FG_ALERT, 
		    "Config file parse error: " << path << " '" 
		    << line << "'" );
	    exit(-1);
	}
	in >> skipcomment;
    }

    return FG_OPTIONS_OK;
}


// Print usage message
void FGOptions::usage ( void ) {
    cout << "Usage: fg [ options ... ]" << endl;
    cout << endl;

    cout << "General Options:" << endl;
    cout << "\t--help -h:  print usage" << endl;
    cout << "\t--fg-root=path:  specify the root path for all the data files"
	 << endl;
    cout << "\t--fg-scenery=path:  specify the base path for all the scenery"
	 << " data." << endl
	 << "\t\tdefaults to $FG_ROOT/Scenery" << endl;
    cout << "\t--disable-game-mode:  disable full-screen game mode" << endl;
    cout << "\t--enable-game-mode:  enable full-screen game mode" << endl;
    cout << "\t--disable-splash-screen:  disable splash screen" << endl;
    cout << "\t--enable-splash-screen:  enable splash screen" << endl;
    cout << "\t--disable-intro-music:  disable introduction music" << endl;
    cout << "\t--enable-intro-music:  enable introduction music" << endl;
    cout << "\t--disable-mouse-pointer:  disable extra mouse pointer" << endl;
    cout << "\t--enable-mouse-pointer:  enable extra mouse pointer (i.e. for"
	 << endl;
    cout << "\t\tfull screen voodoo/voodoo-II based cards.)" << endl;
    cout << "\t--disable-freeze:  start out in an running state" << endl;
    cout << "\t--enable-freeze:  start out in a frozen state" << endl;
    cout << "\t--control=mode:  primary control mode " 
	 << "(joystick, keyboard, mouse)" << endl;
    cout << endl;

    cout << "Features:" << endl;
    cout << "\t--disable-hud:  disable heads up display" << endl;
    cout << "\t--enable-hud:  enable heads up display" << endl;
    cout << "\t--disable-panel:  disable instrument panel" << endl;
    cout << "\t--enable-panel:  enable instrumetn panel" << endl;
    cout << "\t--disable-sound:  disable sound effects" << endl;
    cout << "\t--enable-sound:  enable sound effects" << endl;
    cout << "\t--disable-anti-alias-hud:  disable anti aliased hud" << endl;
    cout << "\t--enable-anti-alias-hud:  enable anti aliased hud" << endl;
    cout << endl;
 
    cout << "Flight Model:" << endl;
    cout << "\t--fdm=abcd:  selects the core flight model code." << endl;
    cout << "\t\tcan be one of jsb, larcsim, magic, external, balloon, or ada"
	 << endl;
    cout << "\t--aircraft=abcd:  aircraft model to load" << endl;
    cout << "\t--model-hz=n:  run the FDM this rate (iterations per second)" 
	 << endl;
    cout << "\t--speed=n:  run the FDM this much faster than real time" << endl;
    cout << "\t--notrim:  Do NOT attempt to trim the model when initializing JSBsim" << endl;
    cout << "\t--wind=DIR@SPEED: specify wind coming from DIR (degrees) at SPEED (knots)" << endl;
    cout << endl;

    //(UIUC)
    cout <<"Aircraft model directory:" << endl;
    cout <<"\t--aircraft-dir=<path> path is relative to the path of the executable" << endl;
    cout << endl;

    cout << "Initial Position and Orientation:" << endl;
    cout << "\t--airport-id=ABCD:  specify starting postion by airport id" 
	 << endl;
    cout << "\t--lon=degrees:  starting longitude in degrees (west = -)" 
	 << endl;
    cout << "\t--lat=degrees:  starting latitude in degrees (south = -)"
	 << endl;
    cout << "\t--altitude=feet:  starting altitude in feet" << endl;
    cout << "\t\t(unless --units-meters specified" << endl;
    cout << "\t--heading=degrees:  heading (yaw) angle in degress (Psi)"
	 << endl;
    cout << "\t--roll=degrees:  roll angle in degrees (Phi)" << endl;
    cout << "\t--pitch=degrees:  pitch angle in degrees (Theta)" << endl;
    cout << "\t--uBody=feet per second:  velocity along the body X axis"
	 << endl;
    cout << "\t--vBody=feet per second:  velocity along the body Y axis"
	 << endl;
    cout << "\t--wBody=feet per second:  velocity along the body Z axis"
	 << endl;
    cout << "\t\t(unless --units-meters specified" << endl;
    cout << "\t--vc= initial airspeed in knots (--fdm=jsb only)" << endl;
    cout << "\t--mach= initial mach number (--fdm=jsb only)" << endl;
    cout << endl;

    cout << "Rendering Options:" << endl;
    cout << "\t--fog-disable:  disable fog/haze" << endl;
    cout << "\t--fog-fastest:  enable fastest fog/haze" << endl;
    cout << "\t--fog-nicest:  enable nicest fog/haze" << endl;
    cout << "\t--enable-clouds:  enable demo cloud layer" << endl;
    cout << "\t--disable-clouds:  disable demo cloud layer" << endl;
    cout << "\t--clouds-asl=xxx:  specify altitude of cloud layer above sea level" << endl;
    cout << "\t--fov=xx.x:  specify initial field of view angle in degrees"
	 << endl;
    cout << "\t--disable-fullscreen:  disable fullscreen mode" << endl;
    cout << "\t--enable-fullscreen:  enable fullscreen mode" << endl;
    cout << "\t--shading-flat:  enable flat shading" << endl;
    cout << "\t--shading-smooth:  enable smooth shading" << endl;
    cout << "\t--disable-skyblend:  disable sky blending" << endl;
    cout << "\t--enable-skyblend:  enable sky blending" << endl;
    cout << "\t--disable-textures:  disable textures" << endl;
    cout << "\t--enable-textures:  enable textures" << endl;
    cout << "\t--disable-wireframe:  disable wireframe drawing mode" << endl;
    cout << "\t--enable-wireframe:  enable wireframe drawing mode" << endl;
    cout << "\t--geometry=WWWxHHH:  window geometry: 640x480, 800x600, etc."
	 << endl;
    cout << "\t--view-offset=xxx:  set the default forward view direction"
	 << endl;
    cout << "\t\tas an offset from straight ahead.  Allowable values are"
	 << endl;
    cout << "\t\tLEFT, RIGHT, CENTER, or a specific number of degrees" << endl;
    cout << "\t--visibility=xxx:  specify initial visibility in meters" << endl;
    cout << "\t--visibility-miles=xxx:  specify initial visibility in miles"
	 << endl;
    cout << endl;

    cout << "Scenery Options:" << endl;
    cout << "\t--tile-radius=n:  specify tile radius, must be 1 - 4" << endl;
    cout << endl;

    cout << "Hud Options:" << endl;
    cout << "\t--units-feet:  Hud displays units in feet" << endl;
    cout << "\t--units-meters:  Hud displays units in meters" << endl;
    cout << "\t--hud-tris:  Hud displays number of triangles rendered" << endl;
    cout << "\t--hud-culled:  Hud displays percentage of triangles culled"
	 << endl;
    cout << endl;
	
    cout << "Time Options:" << endl;
    cout << "\t--time-offset=[+-]hh:mm:ss: add this time offset" << endl;
    cout << "\t--time-match-real: Synchronize real-world and FlightGear" << endl
	 << "\t\ttime. Can be used in combination with --time-offset." << endl;
    cout << "\t--time-match-local:Synchronize local real-world and " << endl
	 << "\t\tFlightGear time" << endl;   
    cout << "\t--start-date-sys=yyyy:mm:dd:hh:mm:ss: specify a starting" << endl
	 << "\t\tdate/time. Uses your system time " << endl;
    cout << "\t--start-date-gmt=yyyy:mm:dd:hh:mm:ss: specify a starting" << endl
	 << "\t\tdate/time. Uses Greenwich Mean Time" << endl;
    cout << "\t--start-date-lat=yyyy:mm:dd:hh:mm:ss: specify a starting" << endl
	 << "\t\tdate/time. Uses Local Aircraft Time" << endl;
#ifdef FG_NETWORK_OLK
    cout << endl;

    cout << "Network Options:" << endl;
    cout << "\t--enable-network-olk:  enable Multipilot mode" << endl;
    cout << "\t--disable-network-olk:  disable Multipilot mode (default)" << endl;
    cout << "\t--net-hud:  Hud displays network info" << endl;
    cout << "\t--net-id=name:  specify your own callsign" << endl;
#endif

    cout << endl;
    cout << "Route/Way Point Options:" << endl;
    cout << "\t--wp=ID[@alt]:  specify a waypoint for the GC autopilot" << endl;
    cout << "\t\tYou can specify multiple waypoints (a route) with multiple"
	 << endl;
    cout << "\t\tinstances of --wp=" << endl;
    cout << "\t--flight-plan=[file]: Read all waypoints from [file]" <<endl;
}


// Destructor
FGOptions::~FGOptions( void ) {
}


//
// Temporary methods...
//

string
FGOptions::get_fg_root() const
{
  return globals->get_fg_root();
}

string
FGOptions::get_fg_scenery () const
{
  return globals->get_fg_scenery();
}

string_list
FGOptions::get_channel_options_list () const
{
  return globals->get_channel_options_list();
}
