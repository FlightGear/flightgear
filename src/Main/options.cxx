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
#include "views.hxx"

FG_USING_STD(string);
FG_USING_NAMESPACE(std);

// from GLUTmain.cxx
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
fgOPTIONS current_options;


// Constructor
fgOPTIONS::fgOPTIONS() :
    // starting longitude in degrees (west = -)
    // starting latitude in degrees (south = -)

    // Default initial position is Globe, AZ (P13)
    lon(-110.6642444),
    lat(  33.3528917),

    // North of the city of Globe
    // lon(-110.7),
    // lat(  33.4),

    // North of the city of Globe
    // lon(-110.742578),
    // lat(  33.507122),

    // Near where I used to live in Globe, AZ
    // lon(-110.766000),
    // lat(  33.377778),

    // 10125 Jewell St. NE
    // lon(-93.15),
    // lat( 45.15),

    // Near KHSP (Hot Springs, VA)
    // lon(-79.8338964 + 0.01),
    // lat( 37.9514564 + 0.008),

    // (SEZ) SEDONA airport
    // lon(-111.7884614 + 0.01),
    // lat(  34.8486289 - 0.015),

    // Jim Brennon's Kingmont Observatory
    // lon(-121.1131667),
    // lat(  38.8293917),

    // Huaras, Peru (S09d 31.871'  W077d 31.498')
    // lon(-77.5249667),
    // lat( -9.5311833),
 
    // Eclipse Watching w73.5 n10 (approx) 18:00 UT
    // lon(-73.5),
    // lat( 10.0),

    // Timms Hill (WI)
    // lon(-90.1953055556),
    // lat( 45.4511388889),

    // starting altitude in meters (this will be reset to ground level
    // if it is lower than the terrain
    altitude(-9999.0),

    // Initial Orientation
    heading(270.0),      // heading (yaw) angle in degress (Psi)
    roll(0.0),           // roll angle in degrees (Phi)
    pitch(0.424),        // pitch angle in degrees (Theta)

    // Initialize current options velocities to 0.0
    uBody(0.0), vBody(0.0), wBody(0.0), vkcas(0.0), mach(0.0),

    // Miscellaneous
    game_mode(0),
    splash_screen(1),
    intro_music(1),
    mouse_pointer(0),
    control_mode(FG_JOYSTICK),
    auto_coordination(FG_AUTO_COORD_NOT_SPECIFIED),

    // Features
    hud_status(0),
    panel_status(1),
    sound(1),
    anti_alias_hud(0),

    // Flight Model options
    flight_model( FGInterface::FG_LARCSIM ),
    aircraft( "c172" ),
    model_hz( NEW_DEFAULT_MODEL_HZ ),
    speed_up( 1 ),
    trim(0),

    // Rendering options
    fog(FG_FOG_NICEST),  // nicest
    clouds(false),
    clouds_asl(5000*FEET_TO_METER),
    fov(55.0),
    fullscreen(0),
    shading(1),
    skyblend(1),
    textures(1),
    wireframe(0),
    xsize(800),
    ysize(600),
    bpp(16),
    view_mode(FG_VIEW_PILOT),

    // Scenery options
    tile_diameter(5),

    // HUD options
    units(FG_UNITS_FEET),
    tris_or_culled(0),
	
    // Time options
    time_offset(0),

    network_olk(false)
{
    // set initial values/defaults
    time_offset_type = FG_TIME_SYS_OFFSET;
    char* envp = ::getenv( "FG_ROOT" );

    if ( envp != NULL ) {
	// fg_root could be anywhere, so default to environmental
	// variable $FG_ROOT if it is set.
	fg_root = envp;
    } else {
	// Otherwise, default to a random compiled-in location if
	// $FG_ROOT is not set.  This can still be overridden from the
	// command line or a config file.

#if defined( WIN32 )
	fg_root = "\\FlightGear";
#elif defined( macintosh )
	fg_root = "";
#else
	fg_root = PKGLIBDIR;
#endif
    }

    // set a possibly independent location for scenery data
    envp = ::getenv( "FG_SCENERY" );

    if ( envp != NULL ) {
	// fg_root could be anywhere, so default to environmental
	// variable $FG_ROOT if it is set.
	fg_scenery = envp;
    } else {
	// Otherwise, default to Scenery being in $FG_ROOT/Scenery
	fg_scenery = "";
    }

    airport_id = "KPAO";	// default airport id
    net_id = "Johnney";		// default pilot's name

    // initialize port config string list
    channel_options_list.clear();
}

void 
fgOPTIONS::toggle_panel() {
    
    bool freeze = globals->get_freeze();

    if( !freeze )
        globals->set_freeze(true);
    
    if( panel_status ) {
	panel_status = false;
	if ( current_panel != NULL )
	  current_panel->setVisibility(false);
    } else {
	panel_status = true;
	if ( current_panel != NULL )
	  current_panel->setVisibility(true);
    }

    // new rule .. "fov" shouldn't get messed with like this.
    /* if ( panel_status ) {
	fov *= 0.4232;
    } else {
	fov *= (1.0 / 0.4232);
    } */

    // fgReshape( xsize, ysize);
    fgReshape( current_view.get_winWidth(), current_view.get_winHeight() );

    if( !freeze )
        globals->set_freeze( false );
}

double
fgOPTIONS::parse_time(const string& time_in) {
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


long int fgOPTIONS::parse_date( const string& date)
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


// parse degree in the form of [+/-]hhh:mm:ss
void fgOPTIONS::parse_control( const string& mode ) {
    if ( mode == "joystick" ) {
	control_mode = FG_JOYSTICK;
    } else if ( mode == "mouse" ) {
	control_mode = FG_MOUSE;
    } else {
	control_mode = FG_KEYBOARD;
    }
}


/// parse degree in the form of [+/-]hhh:mm:ss
double
fgOPTIONS::parse_degree( const string& degree_str) {
    double result = parse_time( degree_str );

    // printf("Degree = %.4f\n", result);

    return(result);
}


// parse time offset command line option
int
fgOPTIONS::parse_time_offset( const string& time_str) {
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


// Parse --tile-diameter=n type option 

int
fgOPTIONS::parse_tile_radius( const string& arg ) {
    int radius = atoi( arg );

    if ( radius < FG_RADIUS_MIN ) { radius = FG_RADIUS_MIN; }
    if ( radius > FG_RADIUS_MAX ) { radius = FG_RADIUS_MAX; }

    // printf("parse_tile_radius(): radius = %d\n", radius);

    return(radius);
}


// Parse --fdm=abcdefg type option 
int
fgOPTIONS::parse_fdm( const string& fm ) {
    // cout << "fdm = " << fm << endl;

    if ( fm == "balloon" ) {
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
fgOPTIONS::parse_fov( const string& arg ) {
    double fov = atof(arg);

    if ( fov < FG_FOV_MIN ) { fov = FG_FOV_MIN; }
    if ( fov > FG_FOV_MAX ) { fov = FG_FOV_MAX; }

    // printf("parse_fov(): result = %.4f\n", fov);

    return(fov);
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
fgOPTIONS::parse_channel( const string& type, const string& channel_str ) {
    // cout << "Channel string = " << channel_str << endl;

    channel_options_list.push_back( type + "," + channel_str );

    return true;
}


// Parse --wp=ID[,alt]
bool fgOPTIONS::parse_wp( const string& arg ) {
    string id, alt_str;
    double alt = 0.0;

    int pos = arg.find( "@" );
    if ( pos != string::npos ) {
	id = arg.substr( 0, pos );
	alt_str = arg.substr( pos + 1 );
	// cout << "id str = " << id << "  alt str = " << alt_str << endl;
	alt = atof( alt_str.c_str() );
	if ( units == FG_UNITS_FEET ) {
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


// Parse a single option
int fgOPTIONS::parse_option( const string& arg ) {
    // General Options
    if ( (arg == "--help") || (arg == "-h") ) {
	// help/usage request
	return(FG_OPTIONS_HELP);
    } else if ( arg == "--disable-game-mode") {
	game_mode = false;
    } else if ( arg == "--enable-game-mode" ) {
	game_mode = true;
    } else if ( arg == "--disable-splash-screen" ) {
	splash_screen = false;
    } else if ( arg == "--enable-splash-screen" ) {
	splash_screen = true;
    } else if ( arg == "--disable-intro-music" ) {
	intro_music = false;
    } else if ( arg == "--enable-intro-music" ) {
	intro_music = true;
    } else if ( arg == "--disable-mouse-pointer" ) {
	mouse_pointer = 1;
    } else if ( arg == "--enable-mouse-pointer" ) {
	mouse_pointer = 2;
    } else if ( arg == "--disable-freeze" ) {
	globals->set_freeze( false );
    } else if ( arg == "--enable-freeze" ) {
	globals->set_freeze( true );
    } else if ( arg == "--disable-anti-alias-hud" ) {
	anti_alias_hud = false;	
    } else if ( arg == "--enable-anti-alias-hud" ) {
	anti_alias_hud = true;	
    } else if ( arg.find( "--control=") != string::npos ) {
	parse_control( arg.substr(10) );
    } else if ( arg == "--disable-auto-coordination" ) {
	auto_coordination = FG_AUTO_COORD_DISABLED;	
    } else if ( arg == "--enable-auto-coordination" ) {
	auto_coordination = FG_AUTO_COORD_ENABLED;	
    } else if ( arg == "--disable-hud" ) {
	hud_status = false;	
    } else if ( arg == "--enable-hud" ) {
	hud_status = true;	
    } else if ( arg == "--disable-panel" ) {
	panel_status = false;
	if ( current_panel != NULL )
	  current_panel->setVisibility(false);
    } else if ( arg == "--enable-panel" ) {
	panel_status = true;
	if ( current_panel != NULL )
	    current_panel->setVisibility(true);
	// fov *= 0.4232; /* NO!!! */
    } else if ( arg == "--disable-sound" ) {
	sound = false;
    } else if ( arg == "--enable-sound" ) {
	sound = true;
    } else if ( arg.find( "--airport-id=") != string::npos ) {
	airport_id = arg.substr( 13 );
	current_properties.setStringValue("/position/airport-id", airport_id);
    } else if ( arg.find( "--lon=" ) != string::npos ) {
	lon = parse_degree( arg.substr(6) );
	airport_id = "";
	current_properties.setDoubleValue("/position/longitude", lon);
	current_properties.setStringValue("/position/airport-id", airport_id);
    } else if ( arg.find( "--lat=" ) != string::npos ) {
	lat = parse_degree( arg.substr(6) );
	airport_id = "";
	current_properties.setDoubleValue("/position/latitude", lat);
	current_properties.setStringValue("/position/airport-id", airport_id);
    } else if ( arg.find( "--altitude=" ) != string::npos ) {
	if ( units == FG_UNITS_FEET ) {
	    altitude = atof( arg.substr(11) ) * FEET_TO_METER;
	} else {
	    altitude = atof( arg.substr(11) );
	}
	current_properties.setDoubleValue("/position/altitude", altitude);
    } else if ( arg.find( "--uBody=" ) != string::npos ) {
	vkcas=mach=-1;
	if ( units == FG_UNITS_FEET ) {
	    uBody = atof( arg.substr(8) );
	} else {
	    uBody = atof( arg.substr(8) ) * FEET_TO_METER;
	}
	current_properties.setDoubleValue("/velocities/speed-north", uBody);
    } else if ( arg.find( "--vBody=" ) != string::npos ) {
	vkcas=mach=-1;
	if ( units == FG_UNITS_FEET ) {
	    vBody = atof( arg.substr(8) );
	} else {
	    vBody = atof( arg.substr(8) ) * FEET_TO_METER;
	}
	current_properties.setDoubleValue("/velocities/speed-east", vBody);
    } else if ( arg.find( "--wBody=" ) != string::npos ) {
	vkcas=mach=-1;
	if ( units == FG_UNITS_FEET ) {
	    wBody = atof( arg.substr(8) );
	} else {
	    wBody = atof( arg.substr(8) ) * FEET_TO_METER;
	}
	current_properties.setDoubleValue("/velocities/speed-down", wBody);
    } else if ( arg.find( "--vc=" ) != string::npos) {
	mach=-1;
	vkcas=atof( arg.substr(5) );
	cout << "Got vc: " << vkcas << endl;
    } else if ( arg.find( "--mach=" ) != string::npos) {
	vkcas=-1;
	mach=atof( arg.substr(7) );
    } else if ( arg.find( "--heading=" ) != string::npos ) {
	heading = atof( arg.substr(10) );
	current_properties.setDoubleValue("/orientation/heading", heading);
    } else if ( arg.find( "--roll=" ) != string::npos ) {
	roll = atof( arg.substr(7) );
	current_properties.setDoubleValue("/orientation/roll", roll);
    } else if ( arg.find( "--pitch=" ) != string::npos ) {
	pitch = atof( arg.substr(8) );
	current_properties.setDoubleValue("/orientation/pitch", pitch);
    } else if ( arg.find( "--fg-root=" ) != string::npos ) {
	fg_root = arg.substr( 10 );
    } else if ( arg.find( "--fg-scenery=" ) != string::npos ) {
	fg_scenery = arg.substr( 13 );
    } else if ( arg.find( "--fdm=" ) != string::npos ) {
	flight_model = parse_fdm( arg.substr(6) );
	current_properties.setIntValue("/sim/flight-model", flight_model);
    if((flight_model == FGInterface::FG_JSBSIM) && (get_trim_mode() == 0)) {
	set_trim_mode(1);
    } else {
	set_trim_mode(0);
    }        
    } else if ( arg.find( "--aircraft=" ) != string::npos ) {
	aircraft = arg.substr(11);
	current_properties.setStringValue("/sim/aircraft", aircraft);
    } else if ( arg.find( "--aircraft-dir=" ) != string::npos ) {
	aircraft_dir =  arg.substr(15); //  (UIUC)
    } else if ( arg.find( "--model-hz=" ) != string::npos ) {
	model_hz = atoi( arg.substr(11) );
    } else if ( arg.find( "--speed=" ) != string::npos ) {
	speed_up = atoi( arg.substr(8) );
    } else if ( arg.find( "--notrim") != string::npos) {
	trim=-1;
    } else if ( arg == "--fog-disable" ) {
	fog = FG_FOG_DISABLED;	
    } else if ( arg == "--fog-fastest" ) {
	fog = FG_FOG_FASTEST;	
    } else if ( arg == "--fog-nicest" ) {
	fog = FG_FOG_NICEST;	
    } else if ( arg == "--disable-clouds" ) {
	clouds = false;	
    } else if ( arg == "--enable-clouds" ) {
	clouds = true;	
    } else if ( arg.find( "--clouds-asl=" ) != string::npos ) {
	if ( units == FG_UNITS_FEET ) {
	    clouds_asl = atof( arg.substr(13) ) * FEET_TO_METER;
	} else {
	    clouds_asl = atof( arg.substr(13) );
	}
    } else if ( arg.find( "--fov=" ) != string::npos ) {
	fov = parse_fov( arg.substr(6) );
    } else if ( arg == "--disable-fullscreen" ) {
	fullscreen = false;	
    } else if ( arg== "--enable-fullscreen") {
	fullscreen = true;	
    } else if ( arg == "--shading-flat") {
	shading = 0;	
    } else if ( arg == "--shading-smooth") {
	shading = 1;	
    } else if ( arg == "--disable-skyblend") {
	skyblend = false;	
    } else if ( arg== "--enable-skyblend" ) {
	skyblend = true;	
    } else if ( arg == "--disable-textures" ) {
	textures = false;	
    } else if ( arg == "--enable-textures" ) {
	textures = true;
    } else if ( arg == "--disable-wireframe" ) {
	wireframe = false;	
    } else if ( arg == "--enable-wireframe" ) {
	wireframe = true;
    } else if ( arg.find( "--geometry=" ) != string::npos ) {
	bool geometry_ok = true;
	string geometry = arg.substr( 11 );
	string::size_type i = geometry.find('x');

 	if (i != string::npos) {
	    xsize = atoi(geometry.substr(0, i));
	    ysize = atoi(geometry.substr(i+1));
	    // cout << "Geometry is " << xsize << 'x' << ysize << '\n';
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
  	}
    } else if ( arg.find( "--bpp=" ) != string::npos ) {
	string bits_per_pix = arg.substr( 6 );
	if ( bits_per_pix == "16" ) {
	    bpp = 16;
	} else if ( bits_per_pix == "24" ) {
	    bpp = 24;
	} else if ( bits_per_pix == "32" ) {
	    bpp = 32;
	}
    } else if ( arg == "--units-feet" ) {
	units = FG_UNITS_FEET;	
    } else if ( arg == "--units-meters" ) {
	units = FG_UNITS_METERS;	
    } else if ( arg.find( "--tile-radius=" ) != string::npos ) {
	tile_radius = parse_tile_radius( arg.substr(14) );
	tile_diameter = tile_radius * 2 + 1;
    } else if ( arg.find( "--time-offset" ) != string::npos ) {
	time_offset = parse_time_offset( (arg.substr(14)) );
	//time_offset_type = FG_TIME_SYS_OFFSET;
    } else if ( arg.find( "--time-match-real") != string::npos ) {
      //time_offset = parse_time_offset(arg.substr(18));
	time_offset_type = FG_TIME_SYS_OFFSET;
    } else if ( arg.find( "--time-match-local") != string::npos ) {
      //time_offset = parse_time_offset(arg.substr(18));
	time_offset_type = FG_TIME_LAT_OFFSET;
    } else if ( arg.find( "--start-date-sys=") != string::npos ) {
        time_offset = parse_date( (arg.substr(17)) );
	time_offset_type = FG_TIME_SYS_ABSOLUTE;
    } else if ( arg.find( "--start-date-lat=") != string::npos ) {
        time_offset = parse_date( (arg.substr(17)) );
	time_offset_type = FG_TIME_LAT_ABSOLUTE;
    } else if ( arg.find( "--start-date-gmt=") != string::npos ) {
        time_offset = parse_date( (arg.substr(17)) );
	time_offset_type = FG_TIME_GMT_ABSOLUTE;

    } else if ( arg == "--hud-tris" ) {
	tris_or_culled = 0;	
    } else if ( arg == "--hud-culled" ) {
	tris_or_culled = 1;
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
	network_olk = false;	
    } else if ( arg== "--enable-network-olk") {
	network_olk = true;	
    } else if ( arg == "--net-hud" ) {
	net_hud_display = 1;	
    } else if ( arg.find( "--net-id=") != string::npos ) {
	net_id = arg.substr( 9 );
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
	current_properties.setStringValue(name.c_str(), value);
	FG_LOG(FG_GENERAL, FG_INFO, "Setting default value of property "
	       << name << " to \"" << value << '"');
    } else if ( arg.find( "--wp=" ) != string::npos ) {
	parse_wp( arg.substr( 5 ) );
    } else {
	FG_LOG( FG_GENERAL, FG_ALERT, "Unknown option '" << arg << "'" );
	return FG_OPTIONS_ERROR;
    }
    
    return FG_OPTIONS_OK;
}


// Scan the command line options for an fg_root definition and set
// just that.
int fgOPTIONS::scan_command_line_for_root( int argc, char **argv ) {
    int i = 1;
    int result;

    FG_LOG(FG_GENERAL, FG_INFO, "Processing command line arguments");

    while ( i < argc ) {
	FG_LOG( FG_GENERAL, FG_DEBUG, "argv[" << i << "] = " << argv[i] );

	string arg = argv[i];
	if ( arg.find( "--fg-root=" ) != string::npos ) {
	    fg_root = arg.substr( 10 );
	}

	i++;
    }
    
    return FG_OPTIONS_OK;
}


// Scan the config file for an fg_root definition and set just that.
int fgOPTIONS::scan_config_file_for_root( const string& path ) {
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

	if ( line.find( "--fg-root=" ) != string::npos ) {
	    fg_root = line.substr( 10 );
	}

	in >> skipcomment;
    }

    return FG_OPTIONS_OK;
}


// Parse the command line options
int fgOPTIONS::parse_command_line( int argc, char **argv ) {
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
int fgOPTIONS::parse_config_file( const string& path ) {
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
void fgOPTIONS::usage ( void ) {
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
    cout << "\t\tcan be one of jsb, larcsim, magic, or external" << endl;
    cout << "\t--aircraft=abcd:  aircraft model to load" << endl;
    cout << "\t--model-hz=n:  run the FDM this rate (iterations per second)" 
	 << endl;
    cout << "\t--speed=n:  run the FDM this much faster than real time" << endl;
    cout << "\t--notrim:  Do NOT attempt to trim the model when initializing JSBsim" << endl;
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
}


// Destructor
fgOPTIONS::~fgOPTIONS( void ) {
}
