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

#include <math.h>		// rint()
#include <stdio.h>
#include <stdlib.h>		// atof(), atoi()
#include <string.h>		// strcmp()

#include STL_STRING

#include <plib/ul.h>

#include <simgear/math/sg_random.h>
#include <simgear/misc/sgstream.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/route/route.hxx>
#include <simgear/route/waypoint.hxx>

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
    FG_OPTIONS_ERROR = 2,
    FG_OPTIONS_VERBOSE_HELP = 3,
    FG_OPTIONS_SHOW_AIRCRAFT = 4
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
				// Position (deliberately out of range)
    fgSetDouble("/position/longitude-deg", 9999.0);
    fgSetDouble("/position/latitude-deg", 9999.0);
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
    fgSetBool("/sim/sound/audible", true);
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
    fgSetBool("/sim/startup/fullscreen", false);
    fgSetBool("/sim/rendering/shading", true);
    fgSetBool("/sim/rendering/skyblend", true);
    fgSetBool("/sim/rendering/textures", true);
    fgSetBool("/sim/rendering/wireframe", false);
    fgSetInt("/sim/startup/xsize", 800);
    fgSetInt("/sim/startup/ysize", 600);
    fgSetInt("/sim/rendering/bits-per-pixel", 16);
    fgSetString("/sim/view-mode", "pilot");
    fgSetDouble("/sim/current-view/heading-offset-deg", 0);
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


static bool
parse_wind (const string &wind, double * min_hdg, double * max_hdg,
	    double * speed, double * gust)
{
  string::size_type pos = wind.find('@');
  if (pos == string::npos)
    return false;
  string dir = wind.substr(0, pos);
  string spd = wind.substr(pos+1);
  pos = dir.find(':');
  if (pos == string::npos) {
    *min_hdg = *max_hdg = atof(dir.c_str());
  } else {
    *min_hdg = atof(dir.substr(0,pos).c_str());
    *max_hdg = atof(dir.substr(pos+1).c_str());
  }
  pos = spd.find(':');
  if (pos == string::npos) {
    *speed = *gust = atof(spd.c_str());
  } else {
    *speed = atof(spd.substr(0,pos).c_str());
    *gust = atof(spd.substr(pos+1).c_str());
  }
  return true;
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

    fgSetDouble("/sim/current-view/field-of-view", fov);

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


static void
setup_wind (double min_hdg, double max_hdg, double speed, double gust)
{
  fgSetDouble("/environment/wind-from-heading-deg", min_hdg);
  fgSetDouble("/environment/params/min-wind-from-heading-deg", min_hdg);
  fgSetDouble("/environment/params/max-wind-from-heading-deg", max_hdg);
  fgSetDouble("/environment/wind-speed-kt", speed);
  fgSetDouble("/environment/params/base-wind-speed-kt", speed);
  fgSetDouble("/environment/params/gust-wind-speed-kt", gust);

  SG_LOG(SG_GENERAL, SG_INFO, "WIND: " << min_hdg << '@' << 
	 speed << " knots" << endl);

#ifdef FG_WEATHERCM
  // convert to fps
  speed *= SG_NM_TO_METER * SG_METER_TO_FEET * (1.0/3600);
  while (min_hdg > 360)
    min_hdg -= 360;
  while (min_hdg <= 0)
    min_hdg += 360;
  min_hdg *= SGD_DEGREES_TO_RADIANS;
  fgSetDouble("/environment/wind-from-north-fps", speed * cos(dir));
  fgSetDouble("/environment/wind-from-east-fps", speed * sin(dir));
#endif // FG_WEATHERCM
}


// Parse --wp=ID[@alt]
static bool 
parse_wp( const string& arg ) {
    string id, alt_str;
    double alt = 0.0;

    string::size_type pos = arg.find( "@" );
    if ( pos != string::npos ) {
	id = arg.substr( 0, pos );
	alt_str = arg.substr( pos + 1 );
	// cout << "id str = " << id << "  alt str = " << alt_str << endl;
	alt = atof( alt_str.c_str() );
	if ( !strcmp(fgGetString("/sim/startup/units"), "feet") ) {
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
    } else if ( (arg == "--verbose") || (arg == "-v") ) {
        // verbose help/usage request
        return(FG_OPTIONS_VERBOSE_HELP);
    } else if ( arg.find( "--language=") == 0 ) {
        globals->set_locale( fgInitLocale( arg.substr( 11 ).c_str() ) );
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
    } else if ( arg == "--disable-random-objects" ) {
        fgSetBool("/sim/rendering/random-objects", false);
    } else if ( arg == "--enable-random-objects" ) {
        fgSetBool("/sim/rendering/random-objects", true);
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
        fgSetString("/sim/control-mode", arg.substr(10).c_str());
    } else if ( arg == "--disable-auto-coordination" ) {
        fgSetBool("/sim/auto-coordination", false);
    } else if ( arg == "--enable-auto-coordination" ) {
	fgSetBool("/sim/auto-coordination", true);
    } else if ( arg.find( "--browser-app=") == 0 ) {
	fgSetString("/sim/startup/browser-app", arg.substr(14).c_str());
    } else if ( arg == "--disable-hud" ) {
	fgSetBool("/sim/hud/visibility", false);
    } else if ( arg == "--enable-hud" ) {
	fgSetBool("/sim/hud/visibility", true);
    } else if ( arg == "--disable-panel" ) {
	fgSetBool("/sim/panel/visibility", false);
    } else if ( arg == "--enable-panel" ) {
	fgSetBool("/sim/panel/visibility", true);
    } else if ( arg == "--disable-sound" ) {
	fgSetBool("/sim/sound/audible", false);
    } else if ( arg == "--enable-sound" ) {
	fgSetBool("/sim/sound/audible", true);
    } else if ( arg.find( "--airport-id=") == 0 ) {
				// NB: changed property name!!!
	fgSetString("/sim/startup/airport-id", arg.substr(13).c_str());
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
	if ( !strcmp(fgGetString("/sim/startup/units"), "feet") )
	    fgSetDouble("/position/altitude-ft", atof(arg.substr(11)));
	else
	    fgSetDouble("/position/altitude-ft",
			atof(arg.substr(11)) * SG_METER_TO_FEET);
    } else if ( arg.find( "--uBody=" ) == 0 ) {
        fgSetString("/sim/startup/speed-set", "UVW");
	if ( !strcmp(fgGetString("/sim/startup/units"), "feet") )
	  fgSetDouble("/velocities/uBody-fps", atof(arg.substr(8)));
	else
	  fgSetDouble("/velocities/uBody-fps",
			       atof(arg.substr(8)) * SG_METER_TO_FEET);
    } else if ( arg.find( "--vBody=" ) == 0 ) {
        fgSetString("/sim/startup/speed-set", "UVW");
	if ( !strcmp(fgGetString("/sim/startup/units"), "feet") )
	  fgSetDouble("/velocities/vBody-fps", atof(arg.substr(8)));
	else
	  fgSetDouble("/velocities/vBody-fps",
			       atof(arg.substr(8)) * SG_METER_TO_FEET);
    } else if ( arg.find( "--wBody=" ) == 0 ) {
        fgSetString("/sim/startup/speed-set", "UVW");
	if ( !strcmp(fgGetString("/sim/startup/units"), "feet") )
	  fgSetDouble("/velocities/wBody-fps", atof(arg.substr(8)));
	else
	  fgSetDouble("/velocities/wBody-fps",
			       atof(arg.substr(8)) * SG_METER_TO_FEET);
    } else if ( arg.find( "--vNorth=" ) == 0 ) {
        fgSetString("/sim/startup/speed-set", "NED");
	if ( !strcmp(fgGetString("/sim/startup/units"), "feet") )
	  fgSetDouble("/velocities/speed-north-fps", atof(arg.substr(9)));
	else
	  fgSetDouble("/velocities/speed-north-fps",
			       atof(arg.substr(9)) * SG_METER_TO_FEET);
    } else if ( arg.find( "--vEast=" ) == 0 ) {
        fgSetString("/sim/startup/speed-set", "NED");
	if ( !strcmp(fgGetString("/sim/startup/units"), "feet") )
	  fgSetDouble("/velocities/speed-east-fps", atof(arg.substr(8)));
	else
	  fgSetDouble("/velocities/speed-east-fps",
		      atof(arg.substr(8)) * SG_METER_TO_FEET);
    } else if ( arg.find( "--vDown=" ) == 0 ) {
        fgSetString("/sim/startup/speed-set", "NED");
	if ( !strcmp(fgGetString("/sim/startup/units"), "feet") )
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
	fgSetString("/sim/flight-model", arg.substr(6).c_str());
    } else if ( arg.find( "--aero=" ) == 0 ) {
	fgSetString("/sim/aero", arg.substr(7).c_str());
    } else if ( arg.find( "--aircraft-dir=" ) == 0 ) {
        fgSetString("/sim/aircraft-dir", arg.substr(15).c_str());
    } else if ( arg.find( "--show-aircraft") == 0) {
        return(FG_OPTIONS_SHOW_AIRCRAFT);
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
    } else if ( arg == "--disable-clouds3d" ) {
        fgSetBool("/sim/rendering/clouds3d", false);
    } else if ( arg == "--enable-clouds3d" ) {
        fgSetBool("/sim/rendering/clouds3d", true);
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
    } else if ( arg.find( "--telnet=" ) == 0 ) {
	add_channel( "telnet", arg.substr(9) );
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
        fgSetString("sim/networking/call-sign", arg.substr(9).c_str());
#endif
    } else if ( arg.find( "--prop:" ) == 0 ) {
        string assign = arg.substr(7);
	string::size_type pos = assign.find('=');
	if ( pos == arg.npos || pos == 0 ) {
	    SG_LOG( SG_GENERAL, SG_ALERT, "Bad property assignment: " << arg );
	    return FG_OPTIONS_ERROR;
	}
	string name = assign.substr(0, pos);
	string value = assign.substr(pos + 1);
	fgSetString(name.c_str(), value.c_str());
	// SG_LOG(SG_GENERAL, SG_INFO, "Setting default value of property "
	//        << name << " to \"" << value << '"');
    } else if ( arg.find("--trace-read=") == 0) {
        string name = arg.substr(13);
	SG_LOG(SG_GENERAL, SG_INFO, "Tracing reads for property " << name);
	fgGetNode(name.c_str(), true)
	  ->setAttribute(SGPropertyNode::TRACE_READ, true);
    } else if ( arg.find("--trace-write=") == 0) {
        string name = arg.substr(14);
	SG_LOG(SG_GENERAL, SG_INFO, "Tracing writes for property " << name);
	fgGetNode(name.c_str(), true)
	  ->setAttribute(SGPropertyNode::TRACE_WRITE, true);
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
	/* apparently not used (CLO, 11 Jun 2002) 
           FGViewer *pilot_view =
	      (FGViewer *)globals->get_viewmgr()->get_view( 0 ); */
        // this will work without calls to the viewer...
	fgSetDouble( "/sim/current-view/heading-offset-deg",
                     default_view_offset  * SGD_RADIANS_TO_DEGREES );
    // $$$ end - added VS Renganathan, 14 Oct 2K
    } else if ( arg.find( "--visibility=" ) == 0 ) {
	fgSetDouble("/environment/visibility-m", atof(arg.substr(13)));
    } else if ( arg.find( "--visibility-miles=" ) == 0 ) {
        double visibility = atof(arg.substr(19)) * 5280.0 * SG_FEET_TO_METER;
	fgSetDouble("/environment/visibility-m", visibility);
    } else if ( arg.find( "--random-wind" ) == 0 ) {
        double min_hdg = sg_random() * 360.0;
        double max_hdg = min_hdg + (20 - sqrt(sg_random() * 400));
	double speed = 40 - sqrt(sg_random() * 1600.0);
	double gust = speed + (10 - sqrt(sg_random() * 100));
	setup_wind(min_hdg, max_hdg, speed, gust);
    } else if ( arg.find( "--wind=" ) == 0 ) {
        double min_hdg, max_hdg, speed, gust;
        if (!parse_wind(arg.substr(7), &min_hdg, &max_hdg, &speed, &gust)) {
	  SG_LOG( SG_GENERAL, SG_ALERT, "bad wind value " << arg.substr(7) );
	  return FG_OPTIONS_ERROR;
	}
	setup_wind(min_hdg, max_hdg, speed, gust);
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
        fgSetString("/sim/aircraft", arg.substr(11).c_str());
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
    bool verbose = false;
    bool help = false;

    SG_LOG(SG_GENERAL, SG_INFO, "Processing command line arguments");

    for (int i = 1; i < argc; i++) {
        string arg = argv[i];

	if (in_options && (arg.find('-') == 0)) {
	  if (arg == "--") {
	    in_options = false;
	  } else {
	    int result = parse_option(arg);
	    if ((result == FG_OPTIONS_HELP) || (result == FG_OPTIONS_ERROR))
              help = true;

            else if (result == FG_OPTIONS_VERBOSE_HELP)
              verbose = true;

            else if (result == FG_OPTIONS_SHOW_AIRCRAFT) {
               fgShowAircraft();
               exit(0);
            }
	  }
	} else {
	  in_options = false;
	  SG_LOG(SG_GENERAL, SG_INFO,
		 "Reading command-line property file " << arg);
	  readProperties(arg, globals->get_props());
	}
    }

    if (help) {
       fgUsage(verbose);
       exit(0);
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
            cerr << endl << "Config file parse error: " << path << " '" 
		    << line << "'" << endl;
	    fgUsage();
	    exit(-1);
	}
	in >> skipcomment;
    }
}


// Print usage message
void 
fgUsage (bool verbose)
{
    SGPropertyNode *locale = globals->get_locale();

    SGPropertyNode options_root;
    SGPath opath( globals->get_fg_root() );
    opath.append( "options.xml" );

    cout << "" << endl;

    try {
        readProperties(opath.c_str(), &options_root);
    } catch (const sg_exception &ex) {
        cout << "Unable to read the help file." << endl;
        cout << "Make sure the file options.xml is located in the FlightGear base directory," << endl;
        cout << "and the location of the base directory is specified by setting $FG_ROOT or" << endl;
        cout << "by adding --fg-root=path as a program argument." << endl;
        
        exit(-1);
    }

    SGPropertyNode *options = options_root.getNode("options");
    if (!options) {
        SG_LOG( SG_GENERAL, SG_ALERT,
                "Error reading options.xml: <options> directive not found." );
        exit(-1);
    }

    SGPropertyNode *usage = locale->getNode(options->getStringValue("usage"));
    if (usage) {
        cout << "Usage: " << usage->getStringValue() << endl;
    }

    vector<SGPropertyNode_ptr>section = options->getChildren("section");
    for (unsigned int j = 0; j < section.size(); j++) {
        string msg = "";

        vector<SGPropertyNode_ptr>option = section[j]->getChildren("option");
        for (unsigned int k = 0; k < option.size(); k++) {

            SGPropertyNode *name = option[k]->getNode("name");
            SGPropertyNode *short_name = option[k]->getNode("short");
            SGPropertyNode *key = option[k]->getNode("key");
            SGPropertyNode *arg = option[k]->getNode("arg");
            bool brief = option[k]->getNode("brief");

            if ((brief || verbose) && name) {
                string tmp = name->getStringValue();

                if (key){
                    tmp.append(":");
                    tmp.append(key->getStringValue());
                }
                if (arg) {
                    tmp.append("=");
                    tmp.append(arg->getStringValue());
                }
                if (short_name) {
                    tmp.append(", -");
                    tmp.append(short_name->getStringValue());
                }

                char cstr[96];
                if (tmp.size() <= 25) {
                    snprintf(cstr, 96, "   --%-27s", tmp.c_str());
                } else {
                    snprintf(cstr, 96, "\n   --%s\n%32c", tmp.c_str(), ' ');
                }

                // There may be more than one <description> tag assosiated
                // with one option

                msg += cstr;
                vector<SGPropertyNode_ptr>desc =
                                          option[k]->getChildren("description");

                if (desc.size() > 0) {
                   for ( unsigned int l = 0; l < desc.size(); l++) {

                      // There may be more than one translation line.

                      string t = desc[l]->getStringValue();
                      SGPropertyNode *n = locale->getNode("strings");
                      vector<SGPropertyNode_ptr>trans_desc =
                               n->getChildren(t.substr(8).c_str());

                      for ( unsigned int m = 0; m < trans_desc.size(); m++ ) {
                         string t_str = trans_desc[m]->getStringValue();

                         if ((m > 0) || ((l > 0) && m == 0)) {
                            snprintf(cstr, 96, "%32c", ' ');
                            msg += cstr;

                         }

                         // If the string is too large to fit on the screen,
                         // then split it up in several pieces.

                         while ( t_str.size() > 47 ) {

                            unsigned int m = t_str.rfind(' ', 47);
                            msg += t_str.substr(0, m);
                            snprintf(cstr, 96, "\n%32c", ' ');
                            msg += cstr;

                            t_str.erase(t_str.begin(), t_str.begin() + m + 1);
                        }
                        msg += t_str + '\n';
                     }
                  }
               }
            }
        }

        SGPropertyNode *name =
                            locale->getNode(section[j]->getStringValue("name"));

        if (!msg.empty() && name) {
           cout << endl << name->getStringValue() << ":" << endl;
           cout << msg;
           msg.erase();
        }
    }

    if ( !verbose ) {
        cout << endl;
        cout << "For a complete list of options use --help --verbose" << endl;
    }
}

// Show available aircraft types
void fgShowAircraft(void) {
   SGPath path( globals->get_fg_root() );
   path.append("Aircraft");

   ulDirEnt* dire;
   ulDir *dirp;

   dirp = ulOpenDir(path.c_str());
   if (dirp == NULL) {
      cerr << "Unable to open aircraft directory." << endl;
      exit(-1);
   }

   cout << "Available aircraft:" << endl;
   while ((dire = ulReadDir(dirp)) != NULL) {
      char *ptr;

      if ((ptr = strstr(dire->d_name, "-set.xml")) && ptr[8] == '\0' ) {
          SGPath afile = path;
          afile.append(dire->d_name);

          *ptr = '\0';

          SGPropertyNode root;
          try {
             readProperties(afile.str(), &root);
          } catch (...) {
             continue;
          }

          SGPropertyNode *desc, *node = root.getNode("sim");
          if (node)
             desc = node->getNode("description");

          char cstr[96];
          if (strlen(dire->d_name) <= 27)
             snprintf(cstr, 96, "   %-27s  %s", dire->d_name,
                      (desc) ? desc->getStringValue() : "" );

          else
             snprintf(cstr, 96, "   %-27s\n%32c%s", dire->d_name, ' ',
                      (desc) ? desc->getStringValue() : "" );

          cout << cstr << endl;
      }
   }

   ulCloseDir(dirp);
}
