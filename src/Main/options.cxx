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

#include <simgear/compiler.h>
#include <simgear/misc/exception.hxx>

#include <math.h>            // rint()
#include <stdio.h>
#include <stdlib.h>          // atof(), atoi()
#include <string.h>

#include STL_STRING

#include <simgear/misc/sgstream.hxx>

// #include <Include/general.hxx>
// #include <Airports/simple.hxx>
// #include <Cockpit/cockpit.hxx>
// #include <FDM/flight.hxx>
// #include <FDM/UIUCModel/uiuc_aircraftdir.h>
#ifdef FG_NETWORK_OLK
#  include <NetworkOLK/network.h>
#endif

#include <GUI/gui.h>

#include "globals.hxx"
#include "fg_init.hxx"
#include "fg_props.hxx"
#include "options.hxx"
#include "viewmgr.hxx"

SG_USING_STD(string);
SG_USING_NAMESPACE(std);


#define NEW_DEFAULT_MODEL_HZ 120

enum
{
    FG_OPTIONS_OK = 0,
    FG_OPTIONS_HELP = 1,
    FG_OPTIONS_ERROR = 2
};

static double
atof( const string& str )
{

#ifdef __MWERKS__ 
    // -dw- if ::atof is called, then we get an infinite loop
    return std::atof( str.c_str() );
#else
    return ::atof( str.c_str() );
#endif
}

static int
atoi( const string& str )
{
#ifdef __MWERKS__ 
    // -dw- if ::atoi is called, then we get an infinite loop
    return std::atoi( str.c_str() );
#else
    return ::atoi( str.c_str() );
#endif
}


/**
 * Set a few fail-safe default property values.
 *
 * These should all be set in $FG_ROOT/preferences.xml, but just
 * in case, we provide some initial sane values here. This method 
 * should be invoked *before* reading any init files.
 */
void
fgSetDefaults ()
{
    // set a possibly independent location for scenery data
    char *envp = ::getenv( "FG_SCENERY" );

    if ( envp != NULL ) {
	// fg_root could be anywhere, so default to environmental
	// variable $FG_ROOT if it is set.
        globals->set_fg_scenery(envp);
    } else {
	// Otherwise, default to Scenery being in $FG_ROOT/Scenery
	globals->set_fg_scenery("");
    }
				// Position (Globe, AZ)
    fgSetDouble("/position/longitude-deg", -110.6642444);
    fgSetDouble("/position/latitude-deg", 33.3528917);
    fgSetDouble("/position/altitude-ft", -9999.0);

				// Orientation
    fgSetDouble("/orientation/heading-deg", 270);
    fgSetDouble("/orientation/roll-deg", 0);
    fgSetDouble("/orientation/pitch-deg", 0.424);

				// Velocities
    fgSetString("/sim/startup/speed-set", "knots");
    fgSetDouble("/velocities/uBody-fps", 0.0);
    fgSetDouble("/velocities/vBody-fps", 0.0);
    fgSetDouble("/velocities/wBody-fps", 0.0);
    fgSetDouble("/velocities/speed-north-fps", 0.0);
    fgSetDouble("/velocities/speed-east-fps", 0.0);
    fgSetDouble("/velocities/speed-down-fps", 0.0);
    fgSetDouble("/velocities/airspeed-kt", 0.0);
    fgSetDouble("/velocities/mach", 0.0);

				// Miscellaneous
    fgSetBool("/sim/startup/game-mode", false);
    fgSetBool("/sim/startup/splash-screen", true);
    fgSetBool("/sim/startup/intro-music", true);
    // we want mouse-pointer to have an undefined value if nothing is
    // specified so we can do the right thing for voodoo-1/2 cards.
    // fgSetString("/sim/startup/mouse-pointer", "disabled");
    fgSetString("/sim/control-mode", "joystick");
    fgSetBool("/sim/auto-coordination", false);
#if !defined(WIN32)
    fgSetString("/sim/startup/browser-app", "netscape");
#else
    fgSetString("/sim/startup/browser-app", "webrun.bat");
#endif
				// Features
    fgSetBool("/sim/hud/visibility", false);
    fgSetBool("/sim/panel/visibility", true);
    fgSetBool("/sim/sound", true);
    fgSetBool("/sim/hud/antialiased", false);

				// Flight Model options
    fgSetString("/sim/flight-model", "jsb");
    fgSetString("/sim/aero", "c172");
    fgSetInt("/sim/model-hz", NEW_DEFAULT_MODEL_HZ);
    fgSetInt("/sim/speed-up", 1);
    fgSetBool("/sim/startup/trim", false);
    fgSetBool("/sim/startup/onground", true);

				// Rendering options
    fgSetString("/sim/rendering/fog", "nicest");
    fgSetBool("/environment/clouds/status", true);
    fgSetDouble("/environment/clouds/altitude-ft", 5000);
    fgSetBool("/sim/startup/fullscreen", false);
    fgSetBool("/sim/rendering/shading", true);
    fgSetBool("/sim/rendering/skyblend", true);
    fgSetBool("/sim/rendering/textures", true);
    fgSetBool("/sim/rendering/wireframe", false);
    fgSetInt("/sim/startup/xsize", 800);
    fgSetInt("/sim/startup/ysize", 600);
    fgSetInt("/sim/rendering/bits-per-pixel", 16);
    fgSetString("/sim/view-mode", "pilot");
    fgSetDouble("/sim/view/offset-deg", 0);
    fgSetDouble("/environment/visibility-m", 20000);

				// HUD options
    fgSetString("/sim/startup/units", "feet");
    fgSetString("/sim/hud/frame-stat-type", "tris");
	
				// Time options
    fgSetInt("/sim/startup/time-offset", 0);
    fgSetString("/sim/startup/time-offset-type", "system-offset");
    fgSetLong("/sim/time/cur-time-override", 0);

    fgSetBool("/sim/networking/network-olk", false);
    fgSetString("/sim/networking/call-sign", "Johnny");

                                // Freeze options
    fgSetBool("/sim/freeze/master", false);
    fgSetBool("/sim/freeze/position", false);
    fgSetBool("/sim/freeze/clock", false);
    fgSetBool("/sim/freeze/fuel", false);
}


// parse a time string ([+/-]%f[:%f[:%f]]) into hours
static double
parse_time(const string& time_in) {
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


// parse a date string (yyyy:mm:dd:hh:mm:ss) into a time_t (seconds)
static long int 
parse_date( const string& date)
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


// parse angle in the form of [+/-]ddd:mm:ss into degrees
static double
parse_degree( const string& degree_str) {
    double result = parse_time( degree_str );

    // printf("Degree = %.4f\n", result);

    return(result);
}


// parse time offset string into seconds
static int
parse_time_offset( const string& time_str) {
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


// Parse --fov=x.xx type option 
static double
parse_fov( const string& arg ) {
    double fov = atof(arg);

    if ( fov < FG_FOV_MIN ) { fov = FG_FOV_MIN; }
    if ( fov > FG_FOV_MAX ) { fov = FG_FOV_MAX; }

    fgSetDouble("/sim/field-of-view", fov);

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

static bool 
add_channel( const string& type, const string& channel_str ) {
    // cout << "Channel string = " << channel_str << endl;

    globals->get_channel_options_list()->push_back( type + "," + channel_str );

    return true;
}


// Parse --wp=ID[@alt]
static bool 
parse_wp( const string& arg ) {
    string id, alt_str;
    double alt = 0.0;

    unsigned int pos = arg.find( "@" );
    if ( pos != string::npos ) {
	id = arg.substr( 0, pos );
	alt_str = arg.substr( pos + 1 );
	// cout << "id str = " << id << "  alt str = " << alt_str << endl;
	alt = atof( alt_str.c_str() );
	if ( fgGetString("/sim/startup/units") == "feet" ) {
	    alt *= SG_FEET_TO_METER;
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
static bool 
parse_flightplan(const string& arg)
{
    sg_gzifstream in(arg.c_str());
    if ( !in.is_open() ) {
	return false;
    }
    while ( true ) {
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

	if ( in.eof() ) {
	    break;
	}
	parse_wp(line);
    }

    return true;
}


// Parse a single option
static int 
parse_option (const string& arg) 
{
    // General Options
    if ( (arg == "--help") || (arg == "-h") ) {
	// help/usage request
	return(FG_OPTIONS_HELP);
    } else if ( arg == "--disable-game-mode") {
	fgSetBool("/sim/startup/game-mode", false);
    } else if ( arg == "--enable-game-mode" ) {
	fgSetBool("/sim/startup/game-mode", true);
    } else if ( arg == "--disable-splash-screen" ) {
	fgSetBool("/sim/startup/splash-screen", false); 
    } else if ( arg == "--enable-splash-screen" ) {
        fgSetBool("/sim/startup/splash-screen", true);
    } else if ( arg == "--disable-intro-music" ) {
	fgSetBool("/sim/startup/intro-music", false);
    } else if ( arg == "--enable-intro-music" ) {
	fgSetBool("/sim/startup/intro-music", true);
    } else if ( arg == "--disable-mouse-pointer" ) {
	fgSetString("/sim/startup/mouse-pointer", "disabled");
    } else if ( arg == "--enable-mouse-pointer" ) {
	fgSetString("/sim/startup/mouse-pointer", "enabled");
    } else if ( arg == "--disable-freeze" ) {
        fgSetBool("/sim/freeze/master", false);
    } else if ( arg == "--enable-freeze" ) {
        fgSetBool("/sim/freeze/master", true);
    } else if ( arg == "--disable-fuel-freeze" ) {
        fgSetBool("/sim/freeze/fuel", false);
    } else if ( arg == "--enable-fuel-freeze" ) {
        fgSetBool("/sim/freeze/fuel", true);
    } else if ( arg == "--disable-clock-freeze" ) {
        fgSetBool("/sim/freeze/clock", false);
    } else if ( arg == "--enable-clock-freeze" ) {
        fgSetBool("/sim/freeze/clock", true);
    } else if ( arg == "--disable-anti-alias-hud" ) {
	fgSetBool("/sim/hud/antialiased", false);
    } else if ( arg == "--enable-anti-alias-hud" ) {
        fgSetBool("/sim/hud/antialiased", true);
    } else if ( arg.find( "--control=") == 0 ) {
        fgSetString("/sim/control-mode", arg.substr(10));
    } else if ( arg == "--disable-auto-coordination" ) {
        fgSetBool("/sim/auto-coordination", false);
    } else if ( arg == "--enable-auto-coordination" ) {
	fgSetBool("/sim/auto-coordination", true);
    } else if ( arg.find( "--browser-app=") == 0 ) {
	fgSetString("/sim/startup/browser-app", arg.substr(14));
    } else if ( arg == "--disable-hud" ) {
	fgSetBool("/sim/hud/visibility", false);
    } else if ( arg == "--enable-hud" ) {
	fgSetBool("/sim/hud/visibility", true);
    } else if ( arg == "--disable-panel" ) {
	fgSetBool("/sim/panel/visibility", false);
    } else if ( arg == "--enable-panel" ) {
	fgSetBool("/sim/panel/visibility", true);
    } else if ( arg == "--disable-sound" ) {
	fgSetBool("/sim/sound", false);
    } else if ( arg == "--enable-sound" ) {
	fgSetBool("/sim/sound", true);
    } else if ( arg.find( "--airport-id=") == 0 ) {
				// NB: changed property name!!!
	fgSetString("/sim/startup/airport-id", arg.substr(13));
    } else if ( arg.find( "--offset-distance=") == 0 ) {
	fgSetDouble("/sim/startup/offset-distance", atof(arg.substr(18)));
    } else if ( arg.find( "--offset-azimuth=") == 0 ) {
	fgSetDouble("/sim/startup/offset-azimuth", atof(arg.substr(17))); 
    } else if ( arg.find( "--lon=" ) == 0 ) {
	fgSetDouble("/position/longitude-deg",
			      parse_degree(arg.substr(6)));
	fgSetString("/sim/startup/airport-id", "");
    } else if ( arg.find( "--lat=" ) == 0 ) {
	fgSetDouble("/position/latitude-deg",
			      parse_degree(arg.substr(6)));
	fgSetString("/sim/startup/airport-id", "");
    } else if ( arg.find( "--altitude=" ) == 0 ) {
	fgSetBool("/sim/startup/onground", false);
	if ( fgGetString("/sim/startup/units") == "feet" )
	    fgSetDouble("/position/altitude-ft", atof(arg.substr(11)));
	else
	    fgSetDouble("/position/altitude-ft",
			atof(arg.substr(11)) * SG_METER_TO_FEET);
    } else if ( arg.find( "--uBody=" ) == 0 ) {
        fgSetString("/sim/startup/speed-set", "UVW");
	if ( fgGetString("/sim/startup/units") == "feet" )
	  fgSetDouble("/velocities/uBody-fps", atof(arg.substr(8)));
	else
	  fgSetDouble("/velocities/uBody-fps",
			       atof(arg.substr(8)) * SG_METER_TO_FEET);
    } else if ( arg.find( "--vBody=" ) == 0 ) {
        fgSetString("/sim/startup/speed-set", "UVW");
	if ( fgGetString("/sim/startup/units") == "feet" )
	  fgSetDouble("/velocities/vBody-fps", atof(arg.substr(8)));
	else
	  fgSetDouble("/velocities/vBody-fps",
			       atof(arg.substr(8)) * SG_METER_TO_FEET);
    } else if ( arg.find( "--wBody=" ) == 0 ) {
        fgSetString("/sim/startup/speed-set", "UVW");
	if ( fgGetString("/sim/startup/units") == "feet" )
	  fgSetDouble("/velocities/wBody-fps", atof(arg.substr(8)));
	else
	  fgSetDouble("/velocities/wBody-fps",
			       atof(arg.substr(8)) * SG_METER_TO_FEET);
    } else if ( arg.find( "--vNorth=" ) == 0 ) {
        fgSetString("/sim/startup/speed-set", "NED");
	if ( fgGetString("/sim/startup/units") == "feet" )
	  fgSetDouble("/velocities/speed-north-fps", atof(arg.substr(9)));
	else
	  fgSetDouble("/velocities/speed-north-fps",
			       atof(arg.substr(9)) * SG_METER_TO_FEET);
    } else if ( arg.find( "--vEast=" ) == 0 ) {
        fgSetString("/sim/startup/speed-set", "NED");
	if ( fgGetString("/sim/startup/units") == "feet" )
	  fgSetDouble("/velocities/speed-east-fps", atof(arg.substr(8)));
	else
	  fgSetDouble("/velocities/speed-east-fps",
		      atof(arg.substr(8)) * SG_METER_TO_FEET);
    } else if ( arg.find( "--vDown=" ) == 0 ) {
        fgSetString("/sim/startup/speed-set", "NED");
	if ( fgGetString("/sim/startup/units") == "feet" )
	  fgSetDouble("/velocities/speed-down-fps", atof(arg.substr(8)));
	else
	  fgSetDouble("/velocities/speed-down-fps",
			       atof(arg.substr(8)) * SG_METER_TO_FEET);
    } else if ( arg.find( "--vc=" ) == 0) {
        fgSetString("/sim/startup/speed-set", "knots");
	fgSetDouble("/velocities/airspeed-kt", atof(arg.substr(5)));
    } else if ( arg.find( "--mach=" ) == 0) {
        fgSetString("/sim/startup/speed-set", "mach");
	fgSetDouble("/velocities/mach", atof(arg.substr(7)));
    } else if ( arg.find( "--heading=" ) == 0 ) {
	fgSetDouble("/orientation/heading-deg", atof(arg.substr(10)));
    } else if ( arg.find( "--roll=" ) == 0 ) {
	fgSetDouble("/orientation/roll-deg", atof(arg.substr(7)));
    } else if ( arg.find( "--pitch=" ) == 0 ) {
	fgSetDouble("/orientation/pitch-deg", atof(arg.substr(8)));
    } else if ( arg.find( "--glideslope=" ) == 0 ) {
	fgSetDouble("/velocities/glideslope", atof(arg.substr(13))
                                          *SG_DEGREES_TO_RADIANS);
    }  else if ( arg.find( "--roc=" ) == 0 ) {
	fgSetDouble("/velocities/vertical-speed-fps", atof(arg.substr(6))/60);
    } else if ( arg.find( "--fg-root=" ) == 0 ) {
	globals->set_fg_root(arg.substr( 10 ));
    } else if ( arg.find( "--fg-scenery=" ) == 0 ) {
        globals->set_fg_scenery(arg.substr( 13 ));
    } else if ( arg.find( "--fdm=" ) == 0 ) {
	fgSetString("/sim/flight-model", arg.substr(6));
    } else if ( arg.find( "--aero=" ) == 0 ) {
	fgSetString("/sim/aero", arg.substr(7));
    } else if ( arg.find( "--aircraft-dir=" ) == 0 ) {
        fgSetString("/sim/aircraft-dir", arg.substr(15));
    } else if ( arg.find( "--model-hz=" ) == 0 ) {
	fgSetInt("/sim/model-hz", atoi(arg.substr(11)));
    } else if ( arg.find( "--speed=" ) == 0 ) {
	fgSetInt("/sim/speed-up", atoi(arg.substr(8)));
    } else if ( arg.find( "--trim") == 0) {
        fgSetBool("/sim/startup/trim", true);
    } else if ( arg.find( "--notrim") == 0) {
        fgSetBool("/sim/startup/trim", false);
    } else if ( arg.find( "--on-ground") == 0) {
        fgSetBool("/sim/startup/onground", true);
    } else if ( arg.find( "--in-air") == 0) {
        fgSetBool("/sim/startup/onground", false);
    } else if ( arg == "--fog-disable" ) {
	fgSetString("/sim/rendering/fog", "disabled");
    } else if ( arg == "--fog-fastest" ) {
	fgSetString("/sim/rendering/fog", "fastest");
    } else if ( arg == "--fog-nicest" ) {
	fgSetString("/sim/fog", "nicest");
    } else if ( arg == "--disable-clouds" ) {
        fgSetBool("/environment/clouds/status", false);
    } else if ( arg == "--enable-clouds" ) {
        fgSetBool("/environment/clouds/status", true);
    } else if ( arg.find( "--clouds-asl=" ) == 0 ) {
				// FIXME: check units
        if ( fgGetString("/sim/startup/units") == "feet" )
	  fgSetDouble("/environment/clouds/altitude-ft",
				atof(arg.substr(13)));
	else
	  fgSetDouble("/environment/clouds/altitude-ft",
				atof(arg.substr(13)) * SG_METER_TO_FEET);
    } else if ( arg.find( "--fov=" ) == 0 ) {
	parse_fov( arg.substr(6) );
    } else if ( arg == "--disable-fullscreen" ) {
        fgSetBool("/sim/startup/fullscreen", false);
    } else if ( arg== "--enable-fullscreen") {
        fgSetBool("/sim/startup/fullscreen", true);
    } else if ( arg == "--shading-flat") {
	fgSetBool("/sim/rendering/shading", false);
    } else if ( arg == "--shading-smooth") {
	fgSetBool("/sim/rendering/shading", true);
    } else if ( arg == "--disable-skyblend") {
	fgSetBool("/sim/rendering/skyblend", false);
    } else if ( arg== "--enable-skyblend" ) {
	fgSetBool("/sim/rendering/skyblend", true);
    } else if ( arg == "--disable-textures" ) {
        fgSetBool("/sim/rendering/textures", false);
    } else if ( arg == "--enable-textures" ) {
        fgSetBool("/sim/rendering/textures", true);
    } else if ( arg == "--disable-wireframe" ) {
        fgSetBool("/sim/rendering/wireframe", false);
    } else if ( arg == "--enable-wireframe" ) {
        fgSetBool("/sim/rendering/wireframe", true);
    } else if ( arg.find( "--geometry=" ) == 0 ) {
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
  	    SG_LOG( SG_GENERAL, SG_ALERT, "Unknown geometry: " << geometry );
 	    SG_LOG( SG_GENERAL, SG_ALERT,
 		    "Setting geometry to " << xsize << 'x' << ysize << '\n');
  	} else {
	  SG_LOG( SG_GENERAL, SG_INFO,
		  "Setting geometry to " << xsize << 'x' << ysize << '\n');
	  fgSetInt("/sim/startup/xsize", xsize);
	  fgSetInt("/sim/startup/ysize", ysize);
	}
    } else if ( arg.find( "--bpp=" ) == 0 ) {
	string bits_per_pix = arg.substr( 6 );
	if ( bits_per_pix == "16" ) {
	    fgSetInt("/sim/rendering/bits-per-pixel", 16);
	} else if ( bits_per_pix == "24" ) {
	    fgSetInt("/sim/rendering/bits-per-pixel", 24);
	} else if ( bits_per_pix == "32" ) {
	    fgSetInt("/sim/rendering/bits-per-pixel", 32);
	} else {
	  SG_LOG(SG_GENERAL, SG_ALERT, "Unsupported bpp " << bits_per_pix);
	}
    } else if ( arg == "--units-feet" ) {
	fgSetString("/sim/startup/units", "feet");
    } else if ( arg == "--units-meters" ) {
	fgSetString("/sim/startup/units", "meters");
    } else if ( arg.find( "--time-offset" ) == 0 ) {
        fgSetInt("/sim/startup/time-offset",
                 parse_time_offset( (arg.substr(14)) ));
	fgSetString("/sim/startup/time-offset-type", "system-offset");
    } else if ( arg.find( "--time-match-real") == 0 ) {
        fgSetString("/sim/startup/time-offset-type", "system-offset");
    } else if ( arg.find( "--time-match-local") == 0 ) {
        fgSetString("/sim/startup/time-offset-type", "latitude-offset");
    } else if ( arg.find( "--start-date-sys=") == 0 ) {
        fgSetInt("/sim/startup/time-offset", parse_date((arg.substr(17))));
	fgSetString("/sim/startup/time-offset-type", "system");
    } else if ( arg.find( "--start-date-lat=") == 0 ) {
        fgSetInt("/sim/startup/time-offset", parse_date((arg.substr(17))));
	fgSetString("/sim/startup/time-offset-type", "latitude");
    } else if ( arg.find( "--start-date-gmt=") == 0 ) {
        fgSetInt("/sim/startup/time-offset", parse_date((arg.substr(17))));
	fgSetString("/sim/startup/time-offset-type", "gmt");
    } else if ( arg == "--hud-tris" ) {
        fgSetString("/sim/hud/frame-stat-type", "tris");
    } else if ( arg == "--hud-culled" ) {
        fgSetString("/sim/hud/frame-stat-type", "culled");
    } else if ( arg.find( "--atc610x" ) == 0 ) {
	add_channel( "atc610x", "dummy" );
    } else if ( arg.find( "--atlas=" ) == 0 ) {
	add_channel( "atlas", arg.substr(8) );
    } else if ( arg.find( "--httpd=" ) == 0 ) {
	add_channel( "httpd", arg.substr(8) );
#ifdef FG_JPEG_SERVER
    } else if ( arg.find( "--jpg-httpd=" ) == 0 ) {
	add_channel( "jpg-httpd", arg.substr(12) );
#endif
    } else if ( arg.find( "--native=" ) == 0 ) {
	add_channel( "native", arg.substr(9) );
    } else if ( arg.find( "--native-ctrls=" ) == 0 ) {
	add_channel( "native_ctrls", arg.substr(15) );
    } else if ( arg.find( "--native-fdm=" ) == 0 ) {
	add_channel( "native_fdm", arg.substr(13) );
    } else if ( arg.find( "--opengc=" ) == 0 ) {
	// char stop;
	// cout << "Adding channel for OpenGC Display" << endl; cin >> stop;
	add_channel( "opengc", arg.substr(9) );
    } else if ( arg.find( "--garmin=" ) == 0 ) {
	add_channel( "garmin", arg.substr(9) );
    } else if ( arg.find( "--nmea=" ) == 0 ) {
	add_channel( "nmea", arg.substr(7) );
    } else if ( arg.find( "--props=" ) == 0 ) {
	add_channel( "props", arg.substr(8) );
    } else if ( arg.find( "--pve=" ) == 0 ) {
	add_channel( "pve", arg.substr(6) );
    } else if ( arg.find( "--ray=" ) == 0 ) {
	add_channel( "ray", arg.substr(6) );
    } else if ( arg.find( "--rul=" ) == 0 ) {
	add_channel( "rul", arg.substr(6) );
    } else if ( arg.find( "--joyclient=" ) == 0 ) {
	add_channel( "joyclient", arg.substr(12) );
#ifdef FG_NETWORK_OLK
    } else if ( arg == "--disable-network-olk" ) {
        fgSetBool("/sim/networking/olk", false);
    } else if ( arg== "--enable-network-olk") {
        fgSetBool("/sim/networking/olk", true);
    } else if ( arg == "--net-hud" ) {
        fgSetBool("/sim/hud/net-display", true);
	net_hud_display = 1;	// FIXME
    } else if ( arg.find( "--net-id=") == 0 ) {
        fgSetString("sim/networking/call-sign", arg.substr(9));
#endif
    } else if ( arg.find( "--prop:" ) == 0 ) {
        string assign = arg.substr(7);
	unsigned int pos = assign.find('=');
	if ( pos == arg.npos || pos == 0 ) {
	    SG_LOG( SG_GENERAL, SG_ALERT, "Bad property assignment: " << arg );
	    return FG_OPTIONS_ERROR;
	}
	string name = assign.substr(0, pos);
	string value = assign.substr(pos + 1);
	fgSetString(name.c_str(), value);
	// SG_LOG(SG_GENERAL, SG_INFO, "Setting default value of property "
	//        << name << " to \"" << value << '"');
    } else if ( arg.find("--trace-read=") == 0) {
        string name = arg.substr(13);
	SG_LOG(SG_GENERAL, SG_INFO, "Tracing reads for property " << name);
	fgGetNode(name, true)->setAttribute(SGPropertyNode::TRACE_READ, true);
    } else if ( arg.find("--trace-write=") == 0) {
        string name = arg.substr(14);
	SG_LOG(SG_GENERAL, SG_INFO, "Tracing writes for property " << name);
	fgGetNode(name, true)->setAttribute(SGPropertyNode::TRACE_WRITE, true);
    } else if ( arg.find( "--view-offset=" ) == 0 ) {
        // $$$ begin - added VS Renganathan, 14 Oct 2K
        // for multi-window outside window imagery
	string woffset = arg.substr( 14 );
	double default_view_offset = 0.0;
	if ( woffset == "LEFT" ) {
	       default_view_offset = SGD_PI * 0.25;
	} else if ( woffset == "RIGHT" ) {
	    default_view_offset = SGD_PI * 1.75;
	} else if ( woffset == "CENTER" ) {
	    default_view_offset = 0.00;
	} else {
	    default_view_offset = atof( woffset.c_str() ) * SGD_DEGREES_TO_RADIANS;
	}
	FGViewerRPH *pilot_view =
	    (FGViewerRPH *)globals->get_viewmgr()->get_view( 0 );
	pilot_view->set_view_offset( default_view_offset );
	pilot_view->set_goal_view_offset( default_view_offset );
	fgSetDouble("/sim/view/offset-deg", default_view_offset);
    // $$$ end - added VS Renganathan, 14 Oct 2K
    } else if ( arg.find( "--visibility=" ) == 0 ) {
	fgSetDouble("/environment/visibility-m", atof(arg.substr(13)));
    } else if ( arg.find( "--visibility-miles=" ) == 0 ) {
        double visibility = atof(arg.substr(19)) * 5280.0 * SG_FEET_TO_METER;
	fgSetDouble("/environment/visibility-m", visibility);
    } else if ( arg.find( "--wind=" ) == 0 ) {
        string val = arg.substr(7);
	unsigned int pos = val.find('@');
	if ( pos == string::npos ) {
	  SG_LOG( SG_GENERAL, SG_ALERT, "bad wind value " << val );
	  return FG_OPTIONS_ERROR;
	}
	double dir = atof(val.substr(0,pos).c_str());
	double speed = atof(val.substr(pos+1).c_str());
	SG_LOG(SG_GENERAL, SG_INFO, "WIND: " << dir << '@' << 
	       speed << " knots" << endl);
	fgSetDouble("/environment/wind-from-heading-deg", dir);
	fgSetDouble("/environment/wind-speed-knots", speed);

        // convert to fps
	speed *= SG_NM_TO_METER * SG_METER_TO_FEET * (1.0/3600);
	// dir += 180;
	if (dir >= 360)
	  dir -= 360;
	dir *= SGD_DEGREES_TO_RADIANS;
	fgSetDouble("/environment/wind-north-fps",
		    speed * cos(dir));
	fgSetDouble("/environment/wind-east-fps",
		    speed * sin(dir));
    } else if ( arg.find( "--wp=" ) == 0 ) {
	parse_wp( arg.substr( 5 ) );
    } else if ( arg.find( "--flight-plan=") == 0) {
	parse_flightplan ( arg.substr (14) );
    } else if ( arg.find( "--config=" ) == 0 ) {
        string file = arg.substr(9);
	try {
	  readProperties(file, globals->get_props());
	} catch (const sg_exception &e) {
	  string message = "Error loading config file: ";
	  message += e.getFormattedMessage();
	  SG_LOG(SG_INPUT, SG_ALERT, message);
	  exit(2);
	}
    } else if ( arg.find( "--aircraft=" ) == 0 ) {
        // read in the top level aircraft definition file
        SGPath apath( globals->get_fg_root() );
        apath.append( "Aircraft" );
        apath.append( arg.substr(11) );
        apath.concat( "-set.xml" );
        try {
            readProperties( apath.str(), globals->get_props() );
        } catch (const sg_exception &e) {
            string message = "Error loading aircraft file: ";
            message += e.getFormattedMessage();
            SG_LOG(SG_INPUT, SG_ALERT, message);
            exit(2);
        }
    } else {
	SG_LOG( SG_GENERAL, SG_ALERT, "Unknown option '" << arg << "'" );
	return FG_OPTIONS_ERROR;
    }
    
    return FG_OPTIONS_OK;
}


// Scan the command line options for an fg_root definition and set
// just that.
string
fgScanForRoot (int argc, char **argv) 
{
    int i = 1;

    SG_LOG(SG_GENERAL, SG_INFO, "Scanning for root: command line");

    while ( i < argc ) {
	SG_LOG( SG_GENERAL, SG_DEBUG, "argv[" << i << "] = " << argv[i] );

	string arg = argv[i];
	if ( arg.find( "--fg-root=" ) == 0 ) {
	    return arg.substr( 10 );
	}

	i++;
    }

    return "";
}


// Scan the config file for an fg_root definition and set just that.
string
fgScanForRoot (const string& path)
{
    sg_gzifstream in( path );
    if ( !in.is_open() )
      return "";

    SG_LOG( SG_GENERAL, SG_INFO, "Scanning for root: " << path );

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

	if ( line.find( "--fg-root=" ) == 0 ) {
	    return line.substr( 10 );
	}

	in >> skipcomment;
    }

    return "";
}


// Parse the command line options
void
fgParseArgs (int argc, char **argv)
{
    bool in_options = true;

    SG_LOG(SG_GENERAL, SG_INFO, "Processing command line arguments");

    for (int i = 1; i < argc; i++) {
        string arg = argv[i];

	if (in_options && (arg.find('-') == 0)) {
	  if (arg == "--") {
	    in_options = false;
	  } else {
	    int result = parse_option(arg);
	    if ( (result == FG_OPTIONS_HELP) ||
		 (result == FG_OPTIONS_ERROR) ) {
	      fgUsage();
	      exit(-1);
	    }
	  }
	} else {
	  in_options = false;
	  SG_LOG(SG_GENERAL, SG_INFO,
		 "Reading command-line property file " << arg);
	  readProperties(arg, globals->get_props());
	}
    }

    SG_LOG(SG_GENERAL, SG_INFO, "Finished command line arguments");
}


// Parse config file options
void
fgParseOptions (const string& path) {
    sg_gzifstream in( path );
    if ( !in.is_open() ) {
        return;
    }

    SG_LOG( SG_GENERAL, SG_INFO, "Processing config file: " << path );

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

	if ( parse_option( line ) == FG_OPTIONS_ERROR ) {
	    SG_LOG( SG_GENERAL, SG_ALERT, 
		    "Config file parse error: " << path << " '" 
		    << line << "'" );
	    fgUsage();
	    exit(-1);
	}
	in >> skipcomment;
    }
}


// Print usage message
void 
fgUsage ()
{
    cout << "Usage: fgfs [ option ... ]" << endl
         << endl

         << "General Options:" << endl
         << "    --help, -h                    Print usage" << endl
         << "    --fg-root=path                Specify the root data path" << endl
         << "    --fg-scenery=path             Specify the base scenery path;" << endl
	 << "                                  Defaults to $FG_ROOT/Scenery" << endl
         << "    --disable-game-mode           Disable full-screen game mode" << endl
         << "    --enable-game-mode            Enable full-screen game mode" << endl
         << "    --disable-splash-screen       Disable splash screen" << endl
         << "    --enable-splash-screen        Enable splash screen" << endl
         << "    --disable-intro-music         Disable introduction music" << endl
         << "    --enable-intro-music          Enable introduction music" << endl
         << "    --disable-mouse-pointer       Disable extra mouse pointer" << endl
         << "    --enable-mouse-pointer        Enable extra mouse pointer (i.e. for full-" << endl
         << "                                  screen Voodoo based cards)" << endl
         << "    --disable-freeze              Start in a running state" << endl
         << "    --enable-freeze               Start in a frozen state" << endl
         << "    --disable-fuel-freeze         Fuel is consumed normally" << endl
         << "    --enable-fuel-freeze          Fuel tank quantity forced to remain constant" << endl
         << "    --disable-clock-freeze        Clock advances normally" << endl
         << "    --enable-clock-freeze         Do not advance clock" << endl
         << "    --control=mode                Primary control mode (joystick, keyboard," << endl
         << "                                  mouse)" << endl
         << "    --enable-auto-coordination    Enable auto coordination" << endl
         << "    --disable-auto-coordination   Disable auto coordination" << endl
         << "    --browser-app=path            Specify path to your web browser" << endl
         << "    --prop:name=value             Set property <name> to <value>" << endl
         << "    --config=path                 Load additional properties from path" << endl
         << "    --units-feet                  Use feet for distances" << endl
         << "    --units-meters                Use meters for distances" << endl
         << endl

         << "Features:" << endl
         << "    --disable-hud                 Disable Heads Up Display (HUD)" << endl
         << "    --enable-hud                  Enable Heads Up Display (HUD)" << endl
         << "    --disable-panel               Disable instrument panel" << endl
         << "    --enable-panel                Enable instrument panel" << endl
         << "    --disable-sound               Disable sound effects" << endl
         << "    --enable-sound                Enable sound effects" << endl
         << "    --disable-anti-alias-hud      Disable anti-aliased HUD" << endl
         << "    --enable-anti-alias-hud       Enable anti-aliased HUD" << endl
         << endl

         << "Aircraft:" <<endl
         << "    --aircraft=name               Select an aircraft profile as defined by a" << endl
         << "                                  top level <name>-set.xml" << endl
         << endl

         << "Flight Model:" << endl
         << "    --fdm=name                    Select the core flight dynamics model" << endl
         << "                                  Can be one of jsb, larcsim, yasim, magic," << endl
         << "                                  balloon, ada, external, or null" << endl
         << "    --aero=name                   Select aircraft aerodynamics model to load" << endl
         << "    --model-hz=n                  Run the FDM this rate (iterations per" << endl
         << "                                  second)" << endl
         << "    --speed=n                     Run the FDM 'n' times faster than real time" << endl
         << "    --notrim                      Do NOT attempt to trim the model (only with" << endl
         << "                                  fdm=jsbsim)" << endl
         << "    --on-ground                   Start at ground level (default)" << endl
         << "    --in-air                      Start in air (implied when using --altitude)" << endl
         << "    --wind=DIR@SPEED              Specify wind coming from DIR (degrees) at" << endl
         << "                                  SPEED (knots)" << endl
         << endl

         << "Aircraft model directory (UIUC FDM ONLY):" << endl
         << "    --aircraft-dir=path           Aircraft directory relative to the path of" << endl
         << "                                  the executable" << endl
         << endl

         << "Initial Position and Orientation:" << endl
         << "    --airport-id=ID               Specify starting position by airport ID" << endl
         << "    --offset-distance=nm          Specify distance to threshold" << endl 
         << "    --offset-azimuth=degrees      Specify heading to threshold" << endl    
         << "    --lon=degrees                 Starting longitude (west = -)" << endl
         << "    --lat=degrees                 Starting latitude (south = -)" << endl
         << "    --altitude=value              Starting altitude (in feet unless" << endl
         << "                                  --units-meters specified)" << endl
         << "    --heading=degrees             Specify heading (yaw) angle (Psi)" << endl
         << "    --roll=degrees                Specify roll angle (Phi)" << endl
         << "    --pitch=degrees               Specify pitch angle (Theta)" << endl
         << "    --uBody=units_per_sec         Specify velocity along the body X axis" << endl
         << "                                  (in feet unless --units-meters specified)" << endl
         << "    --vBody=units_per_sec         Specify velocity along the body Y axis" << endl
         << "                                  (in feet unless --units-meters specified)" << endl
         << "    --wBody=units_per_sec         Specify velocity along the body Z axis" << endl
         << "                                  (in feet unless --units-meters specified)" << endl
         << "    --vc=knots                    Specify initial airspeed" << endl
         << "    --mach=num                    Specify initial mach number" << endl
         << "    --glideslope=degreees         Specify flight path angle (can be positive)" << endl
         << "    --roc=fpm                     Specify initial climb rate (can be negative)" << endl
         << endl

         << "Rendering Options:" << endl
         << "    --bpp=depth                   Specify the bits per pixel" << endl
         << "    --fog-disable                 Disable fog/haze" << endl
         << "    --fog-fastest                 Enable fastest fog/haze" << endl
         << "    --fog-nicest                  Enable nicest fog/haze" << endl
         << "    --enable-clouds               Enable cloud layers" << endl
         << "    --disable-clouds              Disable cloud layers" << endl
         << "    --clouds-asl=altitude         Specify altitude of cloud layer above sea" << endl
         << "                                  level" << endl
         << "    --fov=degrees                 Specify field of view angle" << endl
         << "    --disable-fullscreen          Disable fullscreen mode" << endl
         << "    --enable-fullscreen           Enable fullscreen mode" << endl
         << "    --shading-flat                Enable flat shading" << endl
         << "    --shading-smooth              Enable smooth shading" << endl
         << "    --disable-skyblend            Disable sky blending" << endl
         << "    --enable-skyblend             Enable sky blending" << endl
         << "    --disable-textures            Disable textures" << endl
         << "    --enable-textures             Enable textures" << endl
         << "    --disable-wireframe           Disable wireframe drawing mode" << endl
         << "    --enable-wireframe            Enable wireframe drawing mode" << endl
         << "    --geometry=WxH                Specify window geometry (640x480, etc)" << endl
         << "    --view-offset=value           Specify the default forward view direction" << endl
         << "                                  as an offset from straight ahead.  Allowable" << endl
         << "                                  values are LEFT, RIGHT, CENTER, or a specific" << endl
         << "                                  number in degrees" << endl
         << "    --visibility=meters           Specify initial visibility" << endl
         << "    --visibility-miles=miles      Specify initial visibility in miles" << endl
         << endl

         << "Hud Options:" << endl
         << "    --hud-tris                    Hud displays number of triangles rendered" << endl
         << "    --hud-culled                  Hud displays percentage of triangles culled" << endl
         << endl
	
         << "Time Options:" << endl
         << "    --time-offset=[+-]hh:mm:ss    Add this time offset" << endl
         << "    --time-match-real             Synchronize time with real-world time" << endl
         << "    --time-match-local            Synchronize time with local real-world time" << endl
         << "    --start-date-sys=yyyy:mm:dd:hh:mm:ss" << endl
         << "                                  Specify a starting date/time with respect to" << endl
         << "                                  system time" << endl
         << "    --start-date-gmt=yyyy:mm:dd:hh:mm:ss" << endl
         << "                                  Specify a starting date/time with respect to" << endl
         << "                                  Greenwich Mean Time" << endl
         << "    --start-date-lat=yyyy:mm:dd:hh:mm:ss" << endl
         << "                                  Specify a starting date/time with respect to" << endl
         << "                                  Local Aircraft Time" << endl
         << endl

         << "Network Options:" << endl
         << "    --httpd=port                  Enable http server on the specified port" << endl
#ifdef FG_JPEG_SERVER
         << "    --jpg-httpd=port              Enable screen shot http server on the" << endl
         << "                                  specified port" << endl
#endif
#ifdef FG_NETWORK_OLK
         << "    --disable-network-olk         Disable Multipilot mode (default)" << endl
         << "    --enable-network-olk          Enable Multipilot mode" << endl
         << "    --net-hud                     Hud displays network info" << endl
         << "    --net-id=name                 Specify your own callsign" << endl
#endif
         << endl

         << "Route/Way Point Options:" << endl
         << "    --wp=ID[@alt]                 Specify a waypoint for the GC autopilot;" << endl
         << "                                  multiple instances can be used to create a" << endl
         << "                                  route" << endl
         << "    --flight-plan=file            Read all waypoints from a file" << endl
         << endl

         << "IO Options:" << endl
         << "    --gamin=params                Open connection using the Garmin GPS protocol" << endl
         << "    --joyclient=params            Open connection to an Agwagon joystick" << endl
         << "    --native-ctrls=params         Open connection using the FG Native Controls" << endl
         << "                                  protocol" << endl
         << "    --native-fdm=params           Open connection using the FG Native FDM" << endl
         << "                                  protocol" << endl
         << "    --native=params               Open connection using the FG Native protocol" << endl
         << "    --nmea=params                 Open connection using the NMEA protocol" << endl
         << "    --opengc=params               Open connection using the OpenGC protocol" << endl
         << "    --props=params                Open connection using the interactive" << endl
         << "                                  property manager" << endl
         << "    --pve=params                  Open connection using the PVE protocol" << endl
         << "    --ray=params                  Open connection using the RayWoodworth" << endl
         << "                                  motion chair protocol" << endl
         << "    --rul=params                  Open connection using the RUL protocol" << endl
         << endl
         << "    --atc610x                     Enable atc610x interface." << endl
         << endl

         << "Debugging Options:" << endl
         << "    --trace-read=property         Trace the reads for a property; multiple" << endl
         << "                                  instances allowed." << endl
         << "    --trace-write=property        Trace the writes for a property; multiple" << endl
         << "                                  instances allowed." << endl
         << endl;
}
