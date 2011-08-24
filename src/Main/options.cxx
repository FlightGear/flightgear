// options.cxx -- class to handle command line options
//
// Written by Curtis Olson, started April 1998.
//
// Copyright (C) 1998  Curtis L. Olson  - http://www.flightgear.org/~curt
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
#include <simgear/structure/exception.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/timing/sg_time.hxx>
#include <simgear/misc/sg_dir.hxx>

#include <math.h>		// rint()
#include <stdio.h>
#include <stdlib.h>		// atof(), atoi()
#include <string.h>		// strcmp()
#include <algorithm>

#include <iostream>
#include <string>

#include <simgear/math/sg_random.h>
#include <simgear/props/props_io.hxx>
#include <simgear/misc/sgstream.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/scene/material/mat.hxx>
#include <simgear/sound/soundmgr_openal.hxx>
#include <simgear/misc/strutils.hxx>
#include <Autopilot/route_mgr.hxx>
#include <GUI/gui.h>

#include "globals.hxx"
#include "fg_init.hxx"
#include "fg_props.hxx"
#include "options.hxx"
#include "util.hxx"
#include "viewmgr.hxx"
#include <Main/viewer.hxx>
#include <Environment/presets.hxx>

#include <osg/Version>

using std::string;
using std::sort;
using std::cout;
using std::cerr;
using std::endl;

#if defined( HAVE_VERSION_H ) && HAVE_VERSION_H
#  include <Include/version.h>
#  include <simgear/version.h>
#else
#  include <Include/no_version.h>
#endif

#define NEW_DEFAULT_MODEL_HZ 120

enum
{
    FG_OPTIONS_OK = 0,
    FG_OPTIONS_HELP = 1,
    FG_OPTIONS_ERROR = 2,
    FG_OPTIONS_EXIT = 3,
    FG_OPTIONS_VERBOSE_HELP = 4,
    FG_OPTIONS_SHOW_AIRCRAFT = 5,
    FG_OPTIONS_SHOW_SOUND_DEVICES = 6
};

static double
atof( const string& str )
{
    return ::atof( str.c_str() );
}

static int
atoi( const string& str )
{
    return ::atoi( str.c_str() );
}

static int fgSetupProxy( const char *arg );

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
    const char *envp = ::getenv( "FG_SCENERY" );

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
    fgSetDouble("/orientation/heading-deg", 9999.0);
    fgSetDouble("/orientation/roll-deg", 0.0);
    fgSetDouble("/orientation/pitch-deg", 0.424);

				// Velocities
    fgSetDouble("/velocities/uBody-fps", 0.0);
    fgSetDouble("/velocities/vBody-fps", 0.0);
    fgSetDouble("/velocities/wBody-fps", 0.0);
    fgSetDouble("/velocities/speed-north-fps", 0.0);
    fgSetDouble("/velocities/speed-east-fps", 0.0);
    fgSetDouble("/velocities/speed-down-fps", 0.0);
    fgSetDouble("/velocities/airspeed-kt", 0.0);
    fgSetDouble("/velocities/mach", 0.0);

                                // Presets
    fgSetDouble("/sim/presets/longitude-deg", 9999.0);
    fgSetDouble("/sim/presets/latitude-deg", 9999.0);
    fgSetDouble("/sim/presets/altitude-ft", -9999.0);

    fgSetDouble("/sim/presets/heading-deg", 9999.0);
    fgSetDouble("/sim/presets/roll-deg", 0.0);
    fgSetDouble("/sim/presets/pitch-deg", 0.424);

    fgSetString("/sim/presets/speed-set", "knots");
    fgSetDouble("/sim/presets/airspeed-kt", 0.0);
    fgSetDouble("/sim/presets/mach", 0.0);
    fgSetDouble("/sim/presets/uBody-fps", 0.0);
    fgSetDouble("/sim/presets/vBody-fps", 0.0);
    fgSetDouble("/sim/presets/wBody-fps", 0.0);
    fgSetDouble("/sim/presets/speed-north-fps", 0.0);
    fgSetDouble("/sim/presets/speed-east-fps", 0.0);
    fgSetDouble("/sim/presets/speed-down-fps", 0.0);

    fgSetBool("/sim/presets/onground", true);
    fgSetBool("/sim/presets/trim", false);

				// Miscellaneous
    fgSetBool("/sim/startup/game-mode", false);
    fgSetBool("/sim/startup/splash-screen", true);
    fgSetBool("/sim/startup/intro-music", true);
    // we want mouse-pointer to have an undefined value if nothing is
    // specified so we can do the right thing for voodoo-1/2 cards.
    // fgSetString("/sim/startup/mouse-pointer", "disabled");
    fgSetString("/sim/control-mode", "joystick");
    fgSetBool("/sim/auto-coordination", false);
#if defined(WIN32)
    fgSetString("/sim/startup/browser-app", "webrun.bat");
#elif defined(__APPLE__)
    fgSetString("/sim/startup/browser-app", "open");
#elif defined(sgi)
    fgSetString("/sim/startup/browser-app", "launchWebJumper");
#else
    envp = ::getenv( "WEBBROWSER" );
    if (!envp) envp = "netscape";
    fgSetString("/sim/startup/browser-app", envp);
#endif
    fgSetString("/sim/logging/priority", "alert");

				// Features
    fgSetBool("/sim/hud/color/antialiased", false);
    fgSetBool("/sim/hud/enable3d[1]", true);
    fgSetBool("/sim/hud/visibility[1]", false);
    fgSetBool("/sim/panel/visibility", true);
    fgSetBool("/sim/sound/enabled", true);
    fgSetBool("/sim/sound/working", true);

				// Flight Model options
    fgSetString("/sim/flight-model", "jsb");
    fgSetString("/sim/aero", "c172");
    fgSetInt("/sim/model-hz", NEW_DEFAULT_MODEL_HZ);
    fgSetInt("/sim/speed-up", 1);

				// Rendering options
    fgSetString("/sim/rendering/fog", "nicest");
    fgSetBool("/environment/clouds/status", true);
    fgSetBool("/sim/startup/fullscreen", false);
    fgSetBool("/sim/rendering/shading", true);
    fgSetBool("/sim/rendering/skyblend", true);
    fgSetBool("/sim/rendering/textures", true);
    fgTie( "/sim/rendering/filtering", SGGetTextureFilter, SGSetTextureFilter, false);
    fgSetInt("/sim/rendering/filtering", 1);
    fgSetBool("/sim/rendering/wireframe", false);
    fgSetBool("/sim/rendering/horizon-effect", false);
    fgSetBool("/sim/rendering/enhanced-lighting", false);
    fgSetBool("/sim/rendering/distance-attenuation", false);
    fgSetBool("/sim/rendering/specular-highlight", true);
    fgSetInt("/sim/startup/xsize", 800);
    fgSetInt("/sim/startup/ysize", 600);
    fgSetInt("/sim/rendering/bits-per-pixel", 16);
    fgSetString("/sim/view-mode", "pilot");
    fgSetDouble("/sim/current-view/heading-offset-deg", 0);

				// HUD options
    fgSetString("/sim/startup/units", "feet");
    fgSetString("/sim/hud/frame-stat-type", "tris");

				// Time options
    fgSetInt("/sim/startup/time-offset", 0);
    fgSetString("/sim/startup/time-offset-type", "system-offset");
    fgSetLong("/sim/time/cur-time-override", 0);

                                // Freeze options
    fgSetBool("/sim/freeze/master", false);
    fgSetBool("/sim/freeze/position", false);
    fgSetBool("/sim/freeze/clock", false);
    fgSetBool("/sim/freeze/fuel", false);

    fgSetString("/sim/multiplay/callsign", "callsign");
    fgSetString("/sim/multiplay/rxhost", "");
    fgSetString("/sim/multiplay/txhost", "");
    fgSetInt("/sim/multiplay/rxport", 0);
    fgSetInt("/sim/multiplay/txport", 0);
    
    fgSetString("/sim/version/flightgear", FLIGHTGEAR_VERSION);
    fgSetString("/sim/version/simgear", SG_STRINGIZE(SIMGEAR_VERSION));
    fgSetString("/sim/version/openscenegraph", osgGetVersion());
    fgSetString("/sim/version/revision", REVISION);
    fgSetInt("/sim/version/build-number", HUDSON_BUILD_NUMBER);
    fgSetString("/sim/version/build-id", HUDSON_BUILD_ID);
    if( (envp = ::getenv( "http_proxy" )) != NULL )
      fgSetupProxy( envp );
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

    SG_LOG( SG_GENERAL, SG_INFO, " parse_time() = " << sign * result );

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

    fgSetDouble("/sim/view[0]/config/default-field-of-view-deg", fov);

    // printf("parse_fov(): result = %.4f\n", fov);

    return fov;
}


// Parse I/O channel option
//
// Format is "--protocol=medium,direction,hz,medium_options,..."
//
//   protocol = { native, nmea, garmin, AV400, AV400Sim, fgfs, rul, pve, etc. }
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
    // This check is neccessary to prevent fgviewer from segfaulting when given
    // weird options. (It doesn't run the full initailization)
    if(!globals->get_channel_options_list())
    {
        SG_LOG(SG_GENERAL, SG_ALERT, "Option " << type << "=" << channel_str
                                     << " ignored.");
        return false;
    }
    SG_LOG(SG_GENERAL, SG_INFO, "Channel string = " << channel_str );
    globals->get_channel_options_list()->push_back( type + "," + channel_str );
    return true;
}

static int
fgOptLanguage( const char *arg )
{
    globals->set_locale( fgInitLocale( arg ) );
    return FG_OPTIONS_OK;
}

static void
clearLocation ()
{
    fgSetString("/sim/presets/airport-id", "");
    fgSetString("/sim/presets/vor-id", "");
    fgSetString("/sim/presets/ndb-id", "");
    fgSetString("/sim/presets/carrier", "");
    fgSetString("/sim/presets/parkpos", "");
    fgSetString("/sim/presets/fix", "");
}

static int
fgOptVOR( const char * arg )
{
    clearLocation();
    fgSetString("/sim/presets/vor-id", arg);
    return FG_OPTIONS_OK;
}

static int
fgOptNDB( const char * arg )
{
    clearLocation();
    fgSetString("/sim/presets/ndb-id", arg);
    return FG_OPTIONS_OK;
}

static int
fgOptCarrier( const char * arg )
{
    clearLocation();
    fgSetString("/sim/presets/carrier", arg);
    return FG_OPTIONS_OK;
}

static int
fgOptParkpos( const char * arg )
{
    fgSetString("/sim/presets/parkpos", arg);
    return FG_OPTIONS_OK;
}

static int
fgOptFIX( const char * arg )
{
    clearLocation();
    fgSetString("/sim/presets/fix", arg);
    return FG_OPTIONS_OK;
}

static int
fgOptLon( const char *arg )
{
    clearLocation();
    fgSetDouble("/sim/presets/longitude-deg", parse_degree( arg ));
    fgSetDouble("/position/longitude-deg", parse_degree( arg ));
    return FG_OPTIONS_OK;
}

static int
fgOptLat( const char *arg )
{
    clearLocation();
    fgSetDouble("/sim/presets/latitude-deg", parse_degree( arg ));
    fgSetDouble("/position/latitude-deg", parse_degree( arg ));
    return FG_OPTIONS_OK;
}

static int
fgOptAltitude( const char *arg )
{
    fgSetBool("/sim/presets/onground", false);
    if ( !strcmp(fgGetString("/sim/startup/units"), "feet") )
        fgSetDouble("/sim/presets/altitude-ft", atof( arg ));
    else
        fgSetDouble("/sim/presets/altitude-ft",
		    atof( arg ) * SG_METER_TO_FEET);
    return FG_OPTIONS_OK;
}

static int
fgOptUBody( const char *arg )
{
    fgSetString("/sim/presets/speed-set", "UVW");
    if ( !strcmp(fgGetString("/sim/startup/units"), "feet") )
	fgSetDouble("/sim/presets/uBody-fps", atof( arg ));
    else
	fgSetDouble("/sim/presets/uBody-fps",
                    atof( arg ) * SG_METER_TO_FEET);
    return FG_OPTIONS_OK;
}

static int
fgOptVBody( const char *arg )
{
    fgSetString("/sim/presets/speed-set", "UVW");
    if ( !strcmp(fgGetString("/sim/startup/units"), "feet") )
	fgSetDouble("/sim/presets/vBody-fps", atof( arg ));
    else
	fgSetDouble("/sim/presets/vBody-fps",
			    atof( arg ) * SG_METER_TO_FEET);
    return FG_OPTIONS_OK;
}

static int
fgOptWBody( const char *arg )
{
    fgSetString("/sim/presets/speed-set", "UVW");
    if ( !strcmp(fgGetString("/sim/startup/units"), "feet") )
	fgSetDouble("/sim/presets/wBody-fps", atof(arg));
    else
	fgSetDouble("/sim/presets/wBody-fps",
			    atof(arg) * SG_METER_TO_FEET);
    return FG_OPTIONS_OK;
}

static int
fgOptVNorth( const char *arg )
{
    fgSetString("/sim/presets/speed-set", "NED");
    if ( !strcmp(fgGetString("/sim/startup/units"), "feet") )
	fgSetDouble("/sim/presets/speed-north-fps", atof( arg ));
    else
	fgSetDouble("/sim/presets/speed-north-fps",
			    atof( arg ) * SG_METER_TO_FEET);
    return FG_OPTIONS_OK;
}

static int
fgOptVEast( const char *arg )
{
    fgSetString("/sim/presets/speed-set", "NED");
    if ( !strcmp(fgGetString("/sim/startup/units"), "feet") )
	fgSetDouble("/sim/presets/speed-east-fps", atof(arg));
    else
	fgSetDouble("/sim/presets/speed-east-fps",
		    atof(arg) * SG_METER_TO_FEET);
    return FG_OPTIONS_OK;
}

static int
fgOptVDown( const char *arg )
{
    fgSetString("/sim/presets/speed-set", "NED");
    if ( !strcmp(fgGetString("/sim/startup/units"), "feet") )
	fgSetDouble("/sim/presets/speed-down-fps", atof(arg));
    else
	fgSetDouble("/sim/presets/speed-down-fps",
			    atof(arg) * SG_METER_TO_FEET);
    return FG_OPTIONS_OK;
}

static int
fgOptVc( const char *arg )
{
    // fgSetString("/sim/presets/speed-set", "knots");
    // fgSetDouble("/velocities/airspeed-kt", atof(arg.substr(5)));
    fgSetString("/sim/presets/speed-set", "knots");
    fgSetDouble("/sim/presets/airspeed-kt", atof(arg));
    return FG_OPTIONS_OK;
}

static int
fgOptMach( const char *arg )
{
    fgSetString("/sim/presets/speed-set", "mach");
    fgSetDouble("/sim/presets/mach", atof(arg));
    return FG_OPTIONS_OK;
}

static int
fgOptRoc( const char *arg )
{
    fgSetDouble("/velocities/vertical-speed-fps", atof(arg)/60);
    return FG_OPTIONS_OK;
}

static int
fgOptFgRoot( const char *arg )
{
    // this option is dealt with by fgInitFGRoot
    return FG_OPTIONS_OK;
}

static int
fgOptFgScenery( const char *arg )
{
    globals->set_fg_scenery(arg);
    return FG_OPTIONS_OK;
}

static int
fgOptFgAircraft(const char* arg)
{
  // this option is dealt with by fgInitFGAircraft
  return FG_OPTIONS_OK;
}

static int
fgOptFov( const char *arg )
{
    parse_fov( arg );
    return FG_OPTIONS_OK;
}

static int
fgOptGeometry( const char *arg )
{
    bool geometry_ok = true;
    int xsize = 0, ysize = 0;
    string geometry = arg;
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
    return FG_OPTIONS_OK;
}

static int
fgOptBpp( const char *arg )
{
    string bits_per_pix = arg;
    if ( bits_per_pix == "16" ) {
	fgSetInt("/sim/rendering/bits-per-pixel", 16);
    } else if ( bits_per_pix == "24" ) {
	fgSetInt("/sim/rendering/bits-per-pixel", 24);
    } else if ( bits_per_pix == "32" ) {
	fgSetInt("/sim/rendering/bits-per-pixel", 32);
    } else {
	SG_LOG(SG_GENERAL, SG_ALERT, "Unsupported bpp " << bits_per_pix);
    }
    return FG_OPTIONS_OK;
}

static int
fgOptTimeOffset( const char *arg )
{
    fgSetInt("/sim/startup/time-offset",
                parse_time_offset( arg ));
    fgSetString("/sim/startup/time-offset-type", "system-offset");
    return FG_OPTIONS_OK;
}

static int
fgOptStartDateSys( const char *arg )
{
    fgSetInt("/sim/startup/time-offset", parse_date( arg ) );
    fgSetString("/sim/startup/time-offset-type", "system");
    return FG_OPTIONS_OK;
}

static int
fgOptStartDateLat( const char *arg )
{
    fgSetInt("/sim/startup/time-offset", parse_date( arg ) );
    fgSetString("/sim/startup/time-offset-type", "latitude");
    return FG_OPTIONS_OK;
}

static int
fgOptStartDateGmt( const char *arg )
{
    fgSetInt("/sim/startup/time-offset", parse_date( arg ) );
    fgSetString("/sim/startup/time-offset-type", "gmt");
    return FG_OPTIONS_OK;
}

static int
fgSetupProxy( const char *arg )
{
    string options = simgear::strutils::strip( arg );
    string host, port, auth;
    string::size_type pos;

    // this is NURLP - NURLP is not an url parser
    if( simgear::strutils::starts_with( options, "http://" ) )
        options = options.substr( 7 );
    if( simgear::strutils::ends_with( options, "/" ) )
        options = options.substr( 0, options.length() - 1 );

    host = port = auth = "";
    if ((pos = options.find("@")) != string::npos)
        auth = options.substr(0, pos++);
    else
        pos = 0;

    host = options.substr(pos, options.size());
    if ((pos = host.find(":")) != string::npos) {
        port = host.substr(++pos, host.size());
        host.erase(--pos, host.size());
    }

    fgSetString("/sim/presets/proxy/host", host.c_str());
    fgSetString("/sim/presets/proxy/port", port.c_str());
    fgSetString("/sim/presets/proxy/authentication", auth.c_str());

    return FG_OPTIONS_OK;
}

static int
fgOptTraceRead( const char *arg )
{
    string name = arg;
    SG_LOG(SG_GENERAL, SG_INFO, "Tracing reads for property " << name);
    fgGetNode(name.c_str(), true)
	->setAttribute(SGPropertyNode::TRACE_READ, true);
    return FG_OPTIONS_OK;
}

static int
fgOptLogLevel( const char *arg )
{
    fgSetString("/sim/logging/classes", "all");
    fgSetString("/sim/logging/priority", arg);

    string priority = arg;
    logbuf::set_log_classes(SG_ALL);
    if (priority == "bulk") {
      logbuf::set_log_priority(SG_BULK);
    } else if (priority == "debug") {
      logbuf::set_log_priority(SG_DEBUG);
    } else if (priority == "info") {
      logbuf::set_log_priority(SG_INFO);
    } else if (priority == "warn") {
      logbuf::set_log_priority(SG_WARN);
    } else if (priority == "alert") {
      logbuf::set_log_priority(SG_ALERT);
    } else {
      SG_LOG(SG_GENERAL, SG_WARN, "Unknown logging priority " << priority);
    }
    SG_LOG(SG_GENERAL, SG_DEBUG, "Logging priority is " << priority);

    return FG_OPTIONS_OK;
}


static int
fgOptTraceWrite( const char *arg )
{
    string name = arg;
    SG_LOG(SG_GENERAL, SG_INFO, "Tracing writes for property " << name);
    fgGetNode(name.c_str(), true)
	->setAttribute(SGPropertyNode::TRACE_WRITE, true);
    return FG_OPTIONS_OK;
}

static int
fgOptViewOffset( const char *arg )
{
    // $$$ begin - added VS Renganathan, 14 Oct 2K
    // for multi-window outside window imagery
    string woffset = arg;
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
    return FG_OPTIONS_OK;
}

static int
fgOptVisibilityMeters( const char *arg )
{
    Environment::Presets::VisibilitySingleton::instance()->preset( atof( arg ) );
    return FG_OPTIONS_OK;
}

static int
fgOptVisibilityMiles( const char *arg )
{
    Environment::Presets::VisibilitySingleton::instance()->preset( atof( arg ) * 5280.0 * SG_FEET_TO_METER );
    return FG_OPTIONS_OK;
}

static int
fgOptRandomWind( const char *arg )
{
    double min_hdg = sg_random() * 360.0;
    double max_hdg = min_hdg + (20 - sqrt(sg_random() * 400));
    double speed = sg_random() * sg_random() * 40;
    double gust = speed + (10 - sqrt(sg_random() * 100));
    Environment::Presets::WindSingleton::instance()->preset(min_hdg, max_hdg, speed, gust);
    return FG_OPTIONS_OK;
}

static int
fgOptWind( const char *arg )
{
    double min_hdg = 0.0, max_hdg = 0.0, speed = 0.0, gust = 0.0;
    if (!parse_wind( arg, &min_hdg, &max_hdg, &speed, &gust)) {
	SG_LOG( SG_GENERAL, SG_ALERT, "bad wind value " << arg );
	return FG_OPTIONS_ERROR;
    }
    Environment::Presets::WindSingleton::instance()->preset(min_hdg, max_hdg, speed, gust);
    return FG_OPTIONS_OK;
}

static int
fgOptTurbulence( const char *arg )
{
    Environment::Presets::TurbulenceSingleton::instance()->preset( atof(arg) );
    return FG_OPTIONS_OK;
}

static int
fgOptCeiling( const char *arg )
{
    double elevation, thickness;
    string spec = arg;
    string::size_type pos = spec.find(':');
    if (pos == string::npos) {
        elevation = atof(spec.c_str());
        thickness = 2000;
    } else {
        elevation = atof(spec.substr(0, pos).c_str());
        thickness = atof(spec.substr(pos + 1).c_str());
    }
    Environment::Presets::CeilingSingleton::instance()->preset( elevation, thickness );
    return FG_OPTIONS_OK;
}

static int
fgOptWp( const char *arg )
{
    string_list *waypoints = globals->get_initial_waypoints();
    if (!waypoints) {
        waypoints = new string_list;
        globals->set_initial_waypoints(waypoints);
    }
    waypoints->push_back(arg);
    return FG_OPTIONS_OK;
}

static int
fgOptConfig( const char *arg )
{
    string file = arg;
    try {
	readProperties(file, globals->get_props());
    } catch (const sg_exception &e) {
	string message = "Error loading config file: ";
	message += e.getFormattedMessage() + e.getOrigin();
	SG_LOG(SG_INPUT, SG_ALERT, message);
	exit(2);
    }
    return FG_OPTIONS_OK;
}

static bool
parse_colon (const string &s, double * val1, double * val2)
{
    string::size_type pos = s.find(':');
    if (pos == string::npos) {
        *val2 = atof(s);
        return false;
    } else {
        *val1 = atof(s.substr(0, pos).c_str());
        *val2 = atof(s.substr(pos+1).c_str());
        return true;
    }
}


static int
fgOptFailure( const char * arg )
{
    string a = arg;
    if (a == "pitot") {
        fgSetBool("/systems/pitot/serviceable", false);
    } else if (a == "static") {
        fgSetBool("/systems/static/serviceable", false);
    } else if (a == "vacuum") {
        fgSetBool("/systems/vacuum/serviceable", false);
    } else if (a == "electrical") {
        fgSetBool("/systems/electrical/serviceable", false);
    } else {
        SG_LOG(SG_INPUT, SG_ALERT, "Unknown failure mode: " << a);
        return FG_OPTIONS_ERROR;
    }

    return FG_OPTIONS_OK;
}


static int
fgOptNAV1( const char * arg )
{
    double radial, freq;
    if (parse_colon(arg, &radial, &freq))
        fgSetDouble("/instrumentation/nav[0]/radials/selected-deg", radial);
    fgSetDouble("/instrumentation/nav[0]/frequencies/selected-mhz", freq);
    return FG_OPTIONS_OK;
}

static int
fgOptNAV2( const char * arg )
{
    double radial, freq;
    if (parse_colon(arg, &radial, &freq))
        fgSetDouble("/instrumentation/nav[1]/radials/selected-deg", radial);
    fgSetDouble("/instrumentation/nav[1]/frequencies/selected-mhz", freq);
    return FG_OPTIONS_OK;
}

static int
fgOptADF1( const char * arg )
{
    double rot, freq;
    if (parse_colon(arg, &rot, &freq))
        fgSetDouble("/instrumentation/adf[0]/rotation-deg", rot);
    fgSetDouble("/instrumentation/adf[0]/frequencies/selected-khz", freq);
    return FG_OPTIONS_OK;
}

static int
fgOptADF2( const char * arg )
{
    double rot, freq;
    if (parse_colon(arg, &rot, &freq))
        fgSetDouble("/instrumentation/adf[1]/rotation-deg", rot);
    fgSetDouble("/instrumentation/adf[1]/frequencies/selected-khz", freq);
    return FG_OPTIONS_OK;
}

static int
fgOptDME( const char *arg )
{
    string opt = arg;
    if (opt == "nav1") {
        fgSetInt("/instrumentation/dme/switch-position", 1);
        fgSetString("/instrumentation/dme/frequencies/source",
                    "/instrumentation/nav[0]/frequencies/selected-mhz");
    } else if (opt == "nav2") {
        fgSetInt("/instrumentation/dme/switch-position", 3);
        fgSetString("/instrumentation/dme/frequencies/source",
                    "/instrumentation/nav[1]/frequencies/selected-mhz");
    } else {
        double frequency = atof(arg);
        if (frequency==0.0)
        {
            SG_LOG(SG_INPUT, SG_ALERT, "Invalid DME frequency: '" << arg << "'.");
            return FG_OPTIONS_ERROR;
        }
        fgSetInt("/instrumentation/dme/switch-position", 2);
        fgSetString("/instrumentation/dme/frequencies/source",
                    "/instrumentation/dme/frequencies/selected-mhz");
        fgSetDouble("/instrumentation/dme/frequencies/selected-mhz", frequency);
    }
    return FG_OPTIONS_OK;
}

static int
fgOptLivery( const char *arg )
{
    string opt = arg;
    string livery_path = "livery/" + opt;
    fgSetString("/sim/model/texture-path", livery_path.c_str() );
    return FG_OPTIONS_OK;
}

static int
fgOptScenario( const char *arg )
{
    SGPropertyNode_ptr ai_node = fgGetNode( "/sim/ai", true );
    vector<SGPropertyNode_ptr> scenarii = ai_node->getChildren( "scenario" );
    int index = -1;
    for ( size_t i = 0; i < scenarii.size(); ++i ) {
        int ind = scenarii[i]->getIndex();
        if ( index < ind ) {
            index = ind;
        }
    }
    SGPropertyNode_ptr scenario = ai_node->getNode( "scenario", index + 1, true );
    scenario->setStringValue( arg );
    ai_node->setBoolValue( "enabled", true );
    return FG_OPTIONS_OK;
}

static int
fgOptNoScenarios( const char *arg )
{
    SGPropertyNode_ptr ai_node = fgGetNode( "/sim/ai", true );
    ai_node->removeChildren("scenario",false);
    ai_node->setBoolValue( "enabled", false );
    return FG_OPTIONS_OK;
}

static int
fgOptRunway( const char *arg )
{
    fgSetString("/sim/presets/runway", arg );
    fgSetBool("/sim/presets/runway-requested", true );
    return FG_OPTIONS_OK;
}

static int
fgOptParking( const char *arg )
{
    cerr << "Processing argument " << arg << endl;
    fgSetString("/sim/presets/parking", arg );
    fgSetBool  ("/sim/presets/parking-requested", true );
    return FG_OPTIONS_OK;
}

static int
fgOptVersion( const char *arg )
{
    cerr << "FlightGear version: " << FLIGHTGEAR_VERSION << endl;
    cerr << "Revision: " << REVISION << endl;
    cerr << "Build-Id: " << HUDSON_BUILD_ID << endl;
    cerr << "FG_ROOT=" << globals->get_fg_root() << endl;
    cerr << "FG_HOME=" << fgGetString("/sim/fg-home") << endl;
    cerr << "FG_SCENERY=";

    int didsome = 0;
    string_list scn = globals->get_fg_scenery();
    for (string_list::const_iterator it = scn.begin(); it != scn.end(); it++)
    {
        if (didsome) cerr << ":";
        didsome++;
        cerr << *it;
    }
    cerr << endl;
    cerr << "SimGear version: " << SG_STRINGIZE(SIMGEAR_VERSION) << endl;
    cerr << "PLIB version: " << PLIB_VERSION << endl;
    return FG_OPTIONS_EXIT;
}

static int
fgOptFpe(const char* arg)
{
    // Actually handled in bootstrap.cxx
    return FG_OPTIONS_OK;
}

static int
fgOptFgviewer(const char* arg)
{
    // Actually handled in bootstrap.cxx
    return FG_OPTIONS_OK;
}

static int
fgOptCallSign(const char * arg)
{
    int i;
    char callsign[11];
    strncpy(callsign,arg,10);
    callsign[10]=0;
    for (i=0;callsign[i];i++)
    {
        char c = callsign[i];
        if (c >= 'A' && c <= 'Z') continue;
        if (c >= 'a' && c <= 'z') continue;
        if (c >= '0' && c <= '9') continue;
        if (c == '-' || c == '_') continue;
        // convert any other illegal characters
        callsign[i]='-';
    }
    fgSetString("sim/multiplay/callsign", callsign );
    return FG_OPTIONS_OK;
}


static map<string,size_t> fgOptionMap;

/*
   option       has_param type        property         b_param s_param  func

where:
 option    : name of the option
 has_param : option is --name=value if true or --name if false
 type      : OPTION_BOOL    - property is a boolean
             OPTION_STRING  - property is a string
             OPTION_DOUBLE  - property is a double
             OPTION_INT     - property is an integer
             OPTION_CHANNEL - name of option is the name of a channel
             OPTION_FUNC    - the option trigger a function
 b_param   : if type==OPTION_BOOL,
             value set to the property (has_param is false for boolean)
 s_param   : if type==OPTION_STRING,
             value set to the property if has_param is false
 func      : function called if type==OPTION_FUNC. if has_param is true,
             the value is passed to the function as a string, otherwise,
             s_param is passed.

    For OPTION_DOUBLE and OPTION_INT, the parameter value is converted into a
    double or an integer and set to the property.

    For OPTION_CHANNEL, add_channel is called with the parameter value as the
    argument.
*/

enum OptionType { OPTION_BOOL, OPTION_STRING, OPTION_DOUBLE, OPTION_INT, OPTION_CHANNEL, OPTION_FUNC };
struct OptionDesc {
    const char *option;
    bool has_param;
    enum OptionType type;
    const char *property;
    bool b_param;
    const char *s_param;
    int (*func)( const char * );
    } fgOptionArray[] = {

    {"language",                     true,  OPTION_FUNC,   "", false, "", fgOptLanguage },
    {"disable-game-mode",            false, OPTION_BOOL,   "/sim/startup/game-mode", false, "", 0 },
    {"enable-game-mode",             false, OPTION_BOOL,   "/sim/startup/game-mode", true, "", 0 },
    {"disable-splash-screen",        false, OPTION_BOOL,   "/sim/startup/splash-screen", false, "", 0 },
    {"enable-splash-screen",         false, OPTION_BOOL,   "/sim/startup/splash-screen", true, "", 0 },
    {"disable-intro-music",          false, OPTION_BOOL,   "/sim/startup/intro-music", false, "", 0 },
    {"enable-intro-music",           false, OPTION_BOOL,   "/sim/startup/intro-music", true, "", 0 },
    {"disable-mouse-pointer",        false, OPTION_STRING, "/sim/startup/mouse-pointer", false, "disabled", 0 },
    {"enable-mouse-pointer",         false, OPTION_STRING, "/sim/startup/mouse-pointer", false, "enabled", 0 },
    {"disable-random-objects",       false, OPTION_BOOL,   "/sim/rendering/random-objects", false, "", 0 },
    {"enable-random-objects",        false, OPTION_BOOL,   "/sim/rendering/random-objects", true, "", 0 },
    {"disable-real-weather-fetch",   false, OPTION_BOOL,   "/environment/realwx/enabled", false, "", 0 },
    {"enable-real-weather-fetch",    false, OPTION_BOOL,   "/environment/realwx/enabled", true,  "", 0 },
    {"metar",                        true,  OPTION_STRING, "/environment/metar/data", false, "", 0 },
    {"disable-ai-models",            false, OPTION_BOOL,   "/sim/ai/enabled", false, "", 0 },
    {"enable-ai-models",             false, OPTION_BOOL,   "/sim/ai/enabled", true, "", 0 },
    {"disable-ai-traffic",           false, OPTION_BOOL,   "/sim/traffic-manager/enabled", false, "", 0 },
    {"enable-ai-traffic",            false, OPTION_BOOL,   "/sim/traffic-manager/enabled", true,  "", 0 },
    {"disable-freeze",               false, OPTION_BOOL,   "/sim/freeze/master", false, "", 0 },
    {"enable-freeze",                false, OPTION_BOOL,   "/sim/freeze/master", true, "", 0 },
    {"disable-fuel-freeze",          false, OPTION_BOOL,   "/sim/freeze/fuel", false, "", 0 },
    {"enable-fuel-freeze",           false, OPTION_BOOL,   "/sim/freeze/fuel", true, "", 0 },
    {"disable-clock-freeze",         false, OPTION_BOOL,   "/sim/freeze/clock", false, "", 0 },
    {"enable-clock-freeze",          false, OPTION_BOOL,   "/sim/freeze/clock", true, "", 0 },
    {"disable-hud-3d",               false, OPTION_BOOL,   "/sim/hud/enable3d[1]", false, "", 0 },
    {"enable-hud-3d",                false, OPTION_BOOL,   "/sim/hud/enable3d[1]", true, "", 0 },
    {"disable-anti-alias-hud",       false, OPTION_BOOL,   "/sim/hud/color/antialiased", false, "", 0 },
    {"enable-anti-alias-hud",        false, OPTION_BOOL,   "/sim/hud/color/antialiased", true, "", 0 },
    {"control",                      true,  OPTION_STRING, "/sim/control-mode", false, "", 0 },
    {"disable-auto-coordination",    false, OPTION_BOOL,   "/sim/auto-coordination", false, "", 0 },
    {"enable-auto-coordination",     false, OPTION_BOOL,   "/sim/auto-coordination", true, "", 0 },
    {"browser-app",                  true,  OPTION_STRING, "/sim/startup/browser-app", false, "", 0 },
    {"disable-hud",                  false, OPTION_BOOL,   "/sim/hud/visibility[1]", false, "", 0 },
    {"enable-hud",                   false, OPTION_BOOL,   "/sim/hud/visibility[1]", true, "", 0 },
    {"disable-panel",                false, OPTION_BOOL,   "/sim/panel/visibility", false, "", 0 },
    {"enable-panel",                 false, OPTION_BOOL,   "/sim/panel/visibility", true, "", 0 },
    {"disable-sound",                false, OPTION_BOOL,   "/sim/sound/working", false, "", 0 },
    {"enable-sound",                 false, OPTION_BOOL,   "/sim/sound/working", true, "", 0 },
    {"sound-device",                 true,  OPTION_STRING, "/sim/sound/device-name", false, "", 0 },
    {"airport",                      true,  OPTION_STRING, "/sim/presets/airport-id", false, "", 0 },
    {"runway",                       true,  OPTION_FUNC,   "", false, "", fgOptRunway },
    {"vor",                          true,  OPTION_FUNC,   "", false, "", fgOptVOR },
    {"vor-frequency",                true,  OPTION_DOUBLE, "/sim/presets/vor-freq", false, "", fgOptVOR },
    {"ndb",                          true,  OPTION_FUNC,   "", false, "", fgOptNDB },
    {"ndb-frequency",                true,  OPTION_DOUBLE, "/sim/presets/ndb-freq", false, "", fgOptVOR },
    {"carrier",                      true,  OPTION_FUNC,   "", false, "", fgOptCarrier },
    {"parkpos",                      true,  OPTION_FUNC,   "", false, "", fgOptParkpos },
    {"fix",                          true,  OPTION_FUNC,   "", false, "", fgOptFIX },
    {"offset-distance",              true,  OPTION_DOUBLE, "/sim/presets/offset-distance-nm", false, "", 0 },
    {"offset-azimuth",               true,  OPTION_DOUBLE, "/sim/presets/offset-azimuth-deg", false, "", 0 },
    {"lon",                          true,  OPTION_FUNC,   "", false, "", fgOptLon },
    {"lat",                          true,  OPTION_FUNC,   "", false, "", fgOptLat },
    {"altitude",                     true,  OPTION_FUNC,   "", false, "", fgOptAltitude },
    {"uBody",                        true,  OPTION_FUNC,   "", false, "", fgOptUBody },
    {"vBody",                        true,  OPTION_FUNC,   "", false, "", fgOptVBody },
    {"wBody",                        true,  OPTION_FUNC,   "", false, "", fgOptWBody },
    {"vNorth",                       true,  OPTION_FUNC,   "", false, "", fgOptVNorth },
    {"vEast",                        true,  OPTION_FUNC,   "", false, "", fgOptVEast },
    {"vDown",                        true,  OPTION_FUNC,   "", false, "", fgOptVDown },
    {"vc",                           true,  OPTION_FUNC,   "", false, "", fgOptVc },
    {"mach",                         true,  OPTION_FUNC,   "", false, "", fgOptMach },
    {"heading",                      true,  OPTION_DOUBLE, "/sim/presets/heading-deg", false, "", 0 },
    {"roll",                         true,  OPTION_DOUBLE, "/sim/presets/roll-deg", false, "", 0 },
    {"pitch",                        true,  OPTION_DOUBLE, "/sim/presets/pitch-deg", false, "", 0 },
    {"glideslope",                   true,  OPTION_DOUBLE, "/sim/presets/glideslope-deg", false, "", 0 },
    {"roc",                          true,  OPTION_FUNC,   "", false, "", fgOptRoc },
    {"fg-root",                      true,  OPTION_FUNC,   "", false, "", fgOptFgRoot },
    {"fg-scenery",                   true,  OPTION_FUNC,   "", false, "", fgOptFgScenery },
    {"fg-aircraft",                  true,  OPTION_FUNC,   "", false, "", fgOptFgAircraft },
    {"fdm",                          true,  OPTION_STRING, "/sim/flight-model", false, "", 0 },
    {"aero",                         true,  OPTION_STRING, "/sim/aero", false, "", 0 },
    {"aircraft-dir",                 true,  OPTION_STRING, "/sim/aircraft-dir", false, "", 0 },
    {"model-hz",                     true,  OPTION_INT,    "/sim/model-hz", false, "", 0 },
    {"speed",                        true,  OPTION_INT,    "/sim/speed-up", false, "", 0 },
    {"trim",                         false, OPTION_BOOL,   "/sim/presets/trim", true, "", 0 },
    {"notrim",                       false, OPTION_BOOL,   "/sim/presets/trim", false, "", 0 },
    {"on-ground",                    false, OPTION_BOOL,   "/sim/presets/onground", true, "", 0 },
    {"in-air",                       false, OPTION_BOOL,   "/sim/presets/onground", false, "", 0 },
    {"fog-disable",                  false, OPTION_STRING, "/sim/rendering/fog", false, "disabled", 0 },
    {"fog-fastest",                  false, OPTION_STRING, "/sim/rendering/fog", false, "fastest", 0 },
    {"fog-nicest",                   false, OPTION_STRING, "/sim/rendering/fog", false, "nicest", 0 },
    {"disable-horizon-effect",       false, OPTION_BOOL,   "/sim/rendering/horizon-effect", false, "", 0 },
    {"enable-horizon-effect",        false, OPTION_BOOL,   "/sim/rendering/horizon-effect", true, "", 0 },
    {"disable-enhanced-lighting",    false, OPTION_BOOL,   "/sim/rendering/enhanced-lighting", false, "", 0 },
    {"enable-enhanced-lighting",     false, OPTION_BOOL,   "/sim/rendering/enhanced-lighting", true, "", 0 },
    {"disable-distance-attenuation", false, OPTION_BOOL,   "/sim/rendering/distance-attenuation", false, "", 0 },
    {"enable-distance-attenuation",  false, OPTION_BOOL,   "/sim/rendering/distance-attenuation", true, "", 0 },
    {"disable-specular-highlight",   false, OPTION_BOOL,   "/sim/rendering/specular-highlight", false, "", 0 },
    {"enable-specular-highlight",    false, OPTION_BOOL,   "/sim/rendering/specular-highlight", true, "", 0 },
    {"disable-clouds",               false, OPTION_BOOL,   "/environment/clouds/status", false, "", 0 },
    {"enable-clouds",                false, OPTION_BOOL,   "/environment/clouds/status", true, "", 0 },
    {"disable-clouds3d",             false, OPTION_BOOL,   "/sim/rendering/clouds3d-enable", false, "", 0 },
    {"enable-clouds3d",              false, OPTION_BOOL,   "/sim/rendering/clouds3d-enable", true, "", 0 },
    {"fov",                          true,  OPTION_FUNC,   "", false, "", fgOptFov },
    {"aspect-ratio-multiplier",      true,  OPTION_DOUBLE, "/sim/current-view/aspect-ratio-multiplier", false, "", 0 },
    {"disable-fullscreen",           false, OPTION_BOOL,   "/sim/startup/fullscreen", false, "", 0 },
    {"enable-fullscreen",            false, OPTION_BOOL,   "/sim/startup/fullscreen", true, "", 0 },
    {"disable-save-on-exit",         false, OPTION_BOOL,   "/sim/startup/save-on-exit", false, "", 0 },
    {"enable-save-on-exit",          false, OPTION_BOOL,   "/sim/startup/save-on-exit", true, "", 0 },
    {"shading-flat",                 false, OPTION_BOOL,   "/sim/rendering/shading", false, "", 0 },
    {"shading-smooth",               false, OPTION_BOOL,   "/sim/rendering/shading", true, "", 0 },
    {"disable-skyblend",             false, OPTION_BOOL,   "/sim/rendering/skyblend", false, "", 0 },
    {"enable-skyblend",              false, OPTION_BOOL,   "/sim/rendering/skyblend", true, "", 0 },
    {"disable-textures",             false, OPTION_BOOL,   "/sim/rendering/textures", false, "", 0 },
    {"enable-textures",              false, OPTION_BOOL,   "/sim/rendering/textures", true, "", 0 },
    {"texture-filtering",            false, OPTION_INT,    "/sim/rendering/filtering", 1, "", 0 },
    {"disable-wireframe",            false, OPTION_BOOL,   "/sim/rendering/wireframe", false, "", 0 },
    {"enable-wireframe",             false, OPTION_BOOL,   "/sim/rendering/wireframe", true, "", 0 },
    {"disable-terrasync",            false, OPTION_BOOL,   "/sim/terrasync/enabled", false, "", 0 },
    {"enable-terrasync",             false, OPTION_BOOL,   "/sim/terrasync/enabled", true, "", 0 },
    {"terrasync-dir",                true,  OPTION_STRING, "/sim/terrasync/scenery-dir", false, "", 0 },
    {"geometry",                     true,  OPTION_FUNC,   "", false, "", fgOptGeometry },
    {"bpp",                          true,  OPTION_FUNC,   "", false, "", fgOptBpp },
    {"units-feet",                   false, OPTION_STRING, "/sim/startup/units", false, "feet", 0 },
    {"units-meters",                 false, OPTION_STRING, "/sim/startup/units", false, "meters", 0 },
    {"timeofday",                    true,  OPTION_STRING, "/sim/startup/time-offset-type", false, "noon", 0 },
    {"season",                       true,  OPTION_STRING, "/sim/startup/season", false, "summer", 0 },
    {"time-offset",                  true,  OPTION_FUNC,   "", false, "", fgOptTimeOffset },
    {"time-match-real",              false, OPTION_STRING, "/sim/startup/time-offset-type", false, "system-offset", 0 },
    {"time-match-local",             false, OPTION_STRING, "/sim/startup/time-offset-type", false, "latitude-offset", 0 },
    {"start-date-sys",               true,  OPTION_FUNC,   "", false, "", fgOptStartDateSys },
    {"start-date-lat",               true,  OPTION_FUNC,   "", false, "", fgOptStartDateLat },
    {"start-date-gmt",               true,  OPTION_FUNC,   "", false, "", fgOptStartDateGmt },
    {"hud-tris",                     false, OPTION_STRING, "/sim/hud/frame-stat-type", false, "tris", 0 },
    {"hud-culled",                   false, OPTION_STRING, "/sim/hud/frame-stat-type", false, "culled", 0 },
    {"atcsim",                       true,  OPTION_CHANNEL, "", false, "dummy", 0 },
    {"atlas",                        true,  OPTION_CHANNEL, "", false, "", 0 },
    {"httpd",                        true,  OPTION_CHANNEL, "", false, "", 0 },
#ifdef FG_JPEG_SERVER
    {"jpg-httpd",                    true,  OPTION_CHANNEL, "", false, "", 0 },
#endif
    {"native",                       true,  OPTION_CHANNEL, "", false, "", 0 },
    {"native-ctrls",                 true,  OPTION_CHANNEL, "", false, "", 0 },
    {"native-fdm",                   true,  OPTION_CHANNEL, "", false, "", 0 },
    {"native-gui",                   true,  OPTION_CHANNEL, "", false, "", 0 },
    {"opengc",                       true,  OPTION_CHANNEL, "", false, "", 0 },
    {"AV400",                        true,  OPTION_CHANNEL, "", false, "", 0 },
    {"AV400Sim",                     true,  OPTION_CHANNEL, "", false, "", 0 },
    {"AV400WSimA",                   true,  OPTION_CHANNEL, "", false, "", 0 },
    {"AV400WSimB",                   true,  OPTION_CHANNEL, "", false, "", 0 },
    {"garmin",                       true,  OPTION_CHANNEL, "", false, "", 0 },
    {"nmea",                         true,  OPTION_CHANNEL, "", false, "", 0 },
    {"generic",                      true,  OPTION_CHANNEL, "", false, "", 0 },
    {"props",                        true,  OPTION_CHANNEL, "", false, "", 0 },
    {"telnet",                       true,  OPTION_CHANNEL, "", false, "", 0 },
    {"pve",                          true,  OPTION_CHANNEL, "", false, "", 0 },
    {"ray",                          true,  OPTION_CHANNEL, "", false, "", 0 },
    {"rul",                          true,  OPTION_CHANNEL, "", false, "", 0 },
    {"joyclient",                    true,  OPTION_CHANNEL, "", false, "", 0 },
    {"jsclient",                     true,  OPTION_CHANNEL, "", false, "", 0 },
    {"proxy",                        true,  OPTION_FUNC,    "", false, "", fgSetupProxy },
    {"callsign",                     true,  OPTION_FUNC,    "", false, "", fgOptCallSign},
    {"multiplay",                    true,  OPTION_CHANNEL, "", false, "", 0 },
#ifdef FG_HAVE_HLA
    {"hla",                          true,  OPTION_CHANNEL, "", false, "", 0 },
#endif
    {"trace-read",                   true,  OPTION_FUNC,   "", false, "", fgOptTraceRead },
    {"trace-write",                  true,  OPTION_FUNC,   "", false, "", fgOptTraceWrite },
    {"log-level",                    true,  OPTION_FUNC,   "", false, "", fgOptLogLevel },
    {"view-offset",                  true,  OPTION_FUNC,   "", false, "", fgOptViewOffset },
    {"visibility",                   true,  OPTION_FUNC,   "", false, "", fgOptVisibilityMeters },
    {"visibility-miles",             true,  OPTION_FUNC,   "", false, "", fgOptVisibilityMiles },
    {"random-wind",                  false, OPTION_FUNC,   "", false, "", fgOptRandomWind },
    {"wind",                         true,  OPTION_FUNC,   "", false, "", fgOptWind },
    {"turbulence",                   true,  OPTION_FUNC,   "", false, "", fgOptTurbulence },
    {"ceiling",                      true,  OPTION_FUNC,   "", false, "", fgOptCeiling },
    {"wp",                           true,  OPTION_FUNC,   "", false, "", fgOptWp },
    {"flight-plan",                  true,  OPTION_STRING,   "/autopilot/route-manager/file-path", false, "", NULL },
    {"config",                       true,  OPTION_FUNC,   "", false, "", fgOptConfig },
    {"aircraft",                     true,  OPTION_STRING, "/sim/aircraft", false, "", 0 },
    {"vehicle",                      true,  OPTION_STRING, "/sim/aircraft", false, "", 0 },
    {"failure",                      true,  OPTION_FUNC,   "", false, "", fgOptFailure },
    {"com1",                         true,  OPTION_DOUBLE, "/instrumentation/comm[0]/frequencies/selected-mhz", false, "", 0 },
    {"com2",                         true,  OPTION_DOUBLE, "/instrumentation/comm[1]/frequencies/selected-mhz", false, "", 0 },
    {"nav1",                         true,  OPTION_FUNC,   "", false, "", fgOptNAV1 },
    {"nav2",                         true,  OPTION_FUNC,   "", false, "", fgOptNAV2 },
    {"adf", /*legacy*/               true,  OPTION_FUNC,   "", false, "", fgOptADF1 },
    {"adf1",                         true,  OPTION_FUNC,   "", false, "", fgOptADF1 },
    {"adf2",                         true,  OPTION_FUNC,   "", false, "", fgOptADF2 },
    {"dme",                          true,  OPTION_FUNC,   "", false, "", fgOptDME },
    {"min-status",                   true,  OPTION_STRING,  "/sim/aircraft-min-status", false, "all", 0 },
    {"livery",                       true,  OPTION_FUNC,   "", false, "", fgOptLivery },
    {"ai-scenario",                  true,  OPTION_FUNC,   "", false, "", fgOptScenario },
    {"disable-ai-scenarios",         false, OPTION_FUNC,   "", false, "", fgOptNoScenarios},
    {"parking-id",                   true,  OPTION_FUNC,   "", false, "", fgOptParking  },
    {"version",                      false, OPTION_FUNC,   "", false, "", fgOptVersion },
    {"enable-fpe",                   false, OPTION_FUNC,   "", false, "", fgOptFpe},
    {"fgviewer",                     false, OPTION_FUNC,   "", false, "", fgOptFgviewer},
    {0}
};


// Set a property for the --prop: option. Syntax: --prop:[<type>:]<name>=<value>
// <type> can be "double" etc. but also only the first letter "d".
// Examples:  --prop:alpha=1  --prop:bool:beta=true  --prop:d:gamma=0.123
static bool
set_property(const string& arg)
{
    string::size_type pos = arg.find('=');
    if (pos == arg.npos || pos == 0 || pos + 1 == arg.size())
        return false;

    string name = arg.substr(0, pos);
    string value = arg.substr(pos + 1);
    string type;
    pos = name.find(':');

    if (pos != name.npos && pos != 0 && pos + 1 != name.size()) {
        type = name.substr(0, pos);
        name = name.substr(pos + 1);
    }
    SGPropertyNode *n = fgGetNode(name.c_str(), true);

    bool writable = n->getAttribute(SGPropertyNode::WRITE);
    if (!writable)
        n->setAttribute(SGPropertyNode::WRITE, true);

    bool ret = false;
    if (type.empty())
        ret = n->setUnspecifiedValue(value.c_str());
    else if (type == "s" || type == "string")
        ret = n->setStringValue(value.c_str());
    else if (type == "d" || type == "double")
        ret = n->setDoubleValue(strtod(value.c_str(), 0));
    else if (type == "f" || type == "float")
        ret = n->setFloatValue(atof(value.c_str()));
    else if (type == "l" || type == "long")
        ret =  n->setLongValue(strtol(value.c_str(), 0, 0));
    else if (type == "i" || type == "int")
        ret =  n->setIntValue(atoi(value.c_str()));
    else if (type == "b" || type == "bool")
        ret =  n->setBoolValue(value == "true" || atoi(value.c_str()) != 0);

    if (!writable)
        n->setAttribute(SGPropertyNode::WRITE, false);
    return ret;
}


// Parse a single option
static int
parse_option (const string& arg)
{
    if ( fgOptionMap.size() == 0 ) {
        size_t i = 0;
        OptionDesc *pt = &fgOptionArray[ 0 ];
        while ( pt->option != 0 ) {
            fgOptionMap[ pt->option ] = i;
            i += 1;
            pt += 1;
        }
    }

    // General Options
    if ( (arg == "--help") || (arg == "-h") ) {
	// help/usage request
	return(FG_OPTIONS_HELP);
    } else if ( (arg == "--verbose") || (arg == "-v") ) {
        // verbose help/usage request
        return(FG_OPTIONS_VERBOSE_HELP);
    } else if ( arg.find( "--show-aircraft") == 0) {
        return(FG_OPTIONS_SHOW_AIRCRAFT);
    } else if ( arg.find( "--show-sound-devices") == 0) {
        return(FG_OPTIONS_SHOW_SOUND_DEVICES);
    } else if ( arg.find( "--prop:" ) == 0 ) {
        if (!set_property(arg.substr(7))) {
            SG_LOG( SG_GENERAL, SG_ALERT, "Bad property assignment: " << arg );
            return FG_OPTIONS_ERROR;
        }
    } else if ( arg.find("-psn_") == 0) {
    // on Mac, when launched from the GUI, we are passed the ProcessSerialNumber
    // as an argument (and no others). Silently ignore the argument here.
        return FG_OPTIONS_OK;
    } else if ( arg.find( "--" ) == 0 ) {
        size_t pos = arg.find( '=' );
        string arg_name, arg_value;
        if ( pos == string::npos ) {
            arg_name = arg.substr( 2 );
        } else {
            arg_name = arg.substr( 2, pos - 2 );
            arg_value = arg.substr( pos + 1);
        }
        map<string,size_t>::iterator it = fgOptionMap.find( arg_name );
        if ( it != fgOptionMap.end() ) {
            OptionDesc *pt = &fgOptionArray[ it->second ];
            switch ( pt->type ) {
                case OPTION_BOOL:
                    fgSetBool( pt->property, pt->b_param );
                    break;
                case OPTION_STRING:
                    if ( pt->has_param && !arg_value.empty() ) {
                        fgSetString( pt->property, arg_value.c_str() );
                    } else if ( !pt->has_param && arg_value.empty() ) {
                        fgSetString( pt->property, pt->s_param );
                    } else if ( pt->has_param ) {
                        SG_LOG( SG_GENERAL, SG_ALERT, "Option '" << arg << "' needs a parameter" );
                        return FG_OPTIONS_ERROR;
                    } else {
                        SG_LOG( SG_GENERAL, SG_ALERT, "Option '" << arg << "' does not have a parameter" );
                        return FG_OPTIONS_ERROR;
                    }
                    break;
                case OPTION_DOUBLE:
                    if ( !arg_value.empty() ) {
                        fgSetDouble( pt->property, atof( arg_value ) );
                    } else {
                        SG_LOG( SG_GENERAL, SG_ALERT, "Option '" << arg << "' needs a parameter" );
                        return FG_OPTIONS_ERROR;
                    }
                    break;
                case OPTION_INT:
                    if ( !arg_value.empty() ) {
                        fgSetInt( pt->property, atoi( arg_value ) );
                    } else {
                        SG_LOG( SG_GENERAL, SG_ALERT, "Option '" << arg << "' needs a parameter" );
                        return FG_OPTIONS_ERROR;
                    }
                    break;
                case OPTION_CHANNEL:
                    // XXX return value of add_channel should be checked?
                    if ( pt->has_param && !arg_value.empty() ) {
                        add_channel( pt->option, arg_value );
                    } else if ( !pt->has_param && arg_value.empty() ) {
                        add_channel( pt->option, pt->s_param );
                    } else if ( pt->has_param ) {
                        SG_LOG( SG_GENERAL, SG_ALERT, "Option '" << arg << "' needs a parameter" );
                        return FG_OPTIONS_ERROR;
                    } else {
                        SG_LOG( SG_GENERAL, SG_ALERT, "Option '" << arg << "' does not have a parameter" );
                        return FG_OPTIONS_ERROR;
                    }
                    break;
                case OPTION_FUNC:
                    if ( pt->has_param && !arg_value.empty() ) {
                        return pt->func( arg_value.c_str() );
                    } else if ( !pt->has_param && arg_value.empty() ) {
                        return pt->func( pt->s_param );
                    } else if ( pt->has_param ) {
                        SG_LOG( SG_GENERAL, SG_ALERT, "Option '" << arg << "' needs a parameter" );
                        return FG_OPTIONS_ERROR;
                    } else {
                        SG_LOG( SG_GENERAL, SG_ALERT, "Option '" << arg << "' does not have a parameter" );
                        return FG_OPTIONS_ERROR;
                    }
                    break;
            }
        } else {
            SG_LOG( SG_GENERAL, SG_ALERT, "Unknown option '" << arg << "'" );
            return FG_OPTIONS_ERROR;
        }
    } else {
        SG_LOG( SG_GENERAL, SG_ALERT, "Unknown option '" << arg << "'" );
        return FG_OPTIONS_ERROR;
    }

    return FG_OPTIONS_OK;
}


// Parse the command line options
void
fgParseArgs (int argc, char **argv)
{
    bool in_options = true;
    bool verbose = false;
    bool help = false;

    SG_LOG(SG_GENERAL, SG_ALERT, "Processing command line arguments");

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
              fgOptLogLevel( "alert" );
              SGPath path( globals->get_fg_root() );
              path.append("Aircraft");
              fgShowAircraft(path);
              exit(0);

            } else if (result == FG_OPTIONS_SHOW_SOUND_DEVICES) {
              SGSoundMgr smgr;

              smgr.init();
              string vendor = smgr.get_vendor();
              string renderer = smgr.get_renderer();
              cout << renderer << " provided by " << vendor << endl;
              cout << endl << "No. Device" << endl;

              vector <const char*>devices = smgr.get_available_devices();
              for (vector <const char*>::size_type i=0; i<devices.size(); i++) {
                cout << i << ".  \"" << devices[i] << "\"" << endl;
              }
              devices.clear();
              exit(0);
            }

            else if (result == FG_OPTIONS_EXIT)
               exit(0);
	  }
	} else {
	  in_options = false;
	  SG_LOG(SG_GENERAL, SG_INFO,
		 "Reading command-line property file " << arg);
	  readProperties(arg, globals->get_props());
	}
    }

    if (help) {
       fgOptLogLevel( "alert" );
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
    while ( ! in.eof() ) {
	string line;
	getline( in, line, '\n' );

        // catch extraneous (DOS) line ending character
        int i;
        for (i = line.length(); i > 0; i--)
            if (line[i - 1] > 32)
                break;
        line = line.substr( 0, i );

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

    SG_LOG( SG_GENERAL, SG_ALERT, "" ); // To popup the console on Windows
    cout << endl;

    try {
        fgLoadProps("options.xml", &options_root);
    } catch (const sg_exception &) {
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
            bool brief = option[k]->getNode("brief") != 0;

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

                if (tmp.size() <= 25) {
                    msg+= "   --";
                    msg += tmp;
                    msg.append( 27-tmp.size(), ' ');
                } else {
                    msg += "\n   --";
                    msg += tmp + '\n';
                    msg.append(32, ' ');
                }
                // There may be more than one <description> tag assosiated
                // with one option

                vector<SGPropertyNode_ptr> desc;
                desc = option[k]->getChildren("description");
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
                            msg.append( 32, ' ');
                         }

                         // If the string is too large to fit on the screen,
                         // then split it up in several pieces.

                         while ( t_str.size() > 47 ) {

                            string::size_type m = t_str.rfind(' ', 47);
                            msg += t_str.substr(0, m) + '\n';
                            msg.append( 32, ' ');

                            t_str.erase(t_str.begin(), t_str.begin() + m + 1);
                        }
                        msg += t_str + '\n';
                     }
                  }
               }
            }
        }

        SGPropertyNode *name;
        name = locale->getNode(section[j]->getStringValue("name"));

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
#ifdef _MSC_VER
    cout << "Hit a key to continue..." << endl;
    cin.get();
#endif
}
