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

#include <Include/compiler.h>

#include <math.h>            // rint()
#include <stdio.h>
#include <stdlib.h>          // atof(), atoi()
#include <string.h>

#include STL_STRING

#include <Include/fg_constants.h>
#include <Include/general.hxx>
#include <Cockpit/cockpit.hxx>
#include <Debug/logstream.hxx>
#include <FDM/flight.hxx>
#include <Misc/fgstream.hxx>
#ifdef FG_NETWORK_OLK
#  include <Network/network.h>
#endif
#include <Time/fg_time.hxx>

#include "options.hxx"
#include "fg_serial.hxx"

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
    uBody(0.0), vBody(0.0), wBody(0.0),

    // Miscellaneous
    game_mode(0),
    splash_screen(1),
    intro_music(1),
    mouse_pointer(0),
    pause(0),
    control_mode(FG_JOYSTICK),

    // Features
    hud_status(1),
    panel_status(0),
    sound(1),

    // Flight Model options
    flight_model( FGInterface::FG_LARCSIM ),
    model_hz( NEW_DEFAULT_MODEL_HZ ),
    speed_up( 1 ),

    // Rendering options
    fog(FG_FOG_NICEST),  // nicest
    fov(55.0),
    fullscreen(0),
    shading(1),
    skyblend(1),
    textures(1),
    wireframe(0),
    xsize(640),
    ysize(480),
    view_mode(FG_VIEW_FIRST_PERSON),

    // Scenery options
    tile_diameter(5),

    // HUD options
    units(FG_UNITS_FEET),
    tris_or_culled(0),
	
    // Time options
    time_offset(0)
{
    // set initial values/defaults
    time_offset_type=FG_TIME_SYS_OFFSET;
    char* envp = ::getenv( "FG_ROOT" );

    if ( envp != NULL ) {
	// fg_root could be anywhere, so default to environmental
	// variable $FG_ROOT if it is set.
	fg_root = envp;
    } else {
	// Otherwise, default to a random compiled in location if
	// $FG_ROOT is not set.  This can still be overridden from the
	// command line or a config file.

#if defined( WIN32 )
	fg_root = "\\FlightGear";
#elif defined( MACOS )
	fg_root = "";
#else
	fg_root = PKGLIBDIR;
#endif
    }

    airport_id = "";		// default airport id
    net_id = "Johnney";		// default pilot's name

    // initialize port config string list
    port_options_list.erase ( port_options_list.begin(), 
			      port_options_list.end() );
}

void 
fgOPTIONS::toggle_panel() {
    
    FGTime *t = FGTime::cur_time_params;
    
    int toggle_pause = t->getPause();
    
    if( !toggle_pause )
        t->togglePauseMode();
    
    if( panel_status ) {
	panel_status = false;
    } else {
	panel_status = true;
    }
    if ( panel_status ) {
	if( FGPanel::OurPanel == 0)
	    new FGPanel;
	fov *= 0.4232;
    } else {
	fov *= (1.0 / 0.4232);
    }
    fgReshape( xsize, ysize);
    
    if( !toggle_pause )
        t->togglePauseMode();
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
    time_t theTime = FGTime::cur_time_params->get_gmt(gmt.tm_year,
                                                      gmt.tm_mon,
						      gmt.tm_mday,
						      gmt.tm_hour,
						      gmt.tm_min,
						      gmt.tm_sec);
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
    // printf("fdm = %s\n", fm);

    if ( fm == "slew" ) {
	return FGInterface::FG_SLEW;
    } else if ( fm == "jsb" ) {
	return FGInterface::FG_JSBSIM;
    } else if ( (fm == "larcsim") || (fm == "LaRCsim") ) {
	return FGInterface::FG_LARCSIM;
    } else if ( fm == "external" ) {
	return FGInterface::FG_EXTERNAL;
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


// Parse serial port option --serial=/dev/ttyS1,nmea,4800,out
//
// Format is "--serial=device,format,baud,direction" where
// 
//  device = OS device name to be open()'ed
//  format = {nmea, garmin,fgfs,rul,pve}
//  baud = {300, 1200, 2400, ..., 230400}
//  direction = {in, out, bi}

bool 
fgOPTIONS::parse_serial( const string& serial_str ) {
    string::size_type pos;

    // cout << "Serial string = " << serial_str << endl;

    // a flailing attempt to see if the port config string has a
    // chance at being valid
    pos = serial_str.find(",");
    if ( pos == string::npos ) {
	FG_LOG( FG_GENERAL, FG_ALERT, 
		"Malformed serial port configure string" );
	return false;
    }
    
    port_options_list.push_back( serial_str );

    return true;
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
    } else if ( arg == "--disable-pause" ) {
	pause = false;	
    } else if ( arg == "--enable-pause" ) {
	pause = true;	
    } else if ( arg.find( "--control=") != string::npos ) {
	parse_control( arg.substr(10) );
    } else if ( arg == "--disable-hud" ) {
	hud_status = false;	
    } else if ( arg == "--enable-hud" ) {
	hud_status = true;	
    } else if ( arg == "--disable-panel" ) {
	panel_status = false;
    } else if ( arg == "--enable-panel" ) {
	panel_status = true;
	fov *= 0.4232;
    } else if ( arg == "--disable-sound" ) {
	sound = false;
    } else if ( arg == "--enable-sound" ) {
	sound = true;
    } else if ( arg.find( "--airport-id=") != string::npos ) {
	airport_id = arg.substr( 13 );
    } else if ( arg.find( "--lon=" ) != string::npos ) {
	lon = parse_degree( arg.substr(6) );
    } else if ( arg.find( "--lat=" ) != string::npos ) {
	lat = parse_degree( arg.substr(6) );
    } else if ( arg.find( "--altitude=" ) != string::npos ) {
	if ( units == FG_UNITS_FEET ) {
	    altitude = atof( arg.substr(11) ) * FEET_TO_METER;
	} else {
	    altitude = atof( arg.substr(11) );
	}
    } else if ( arg.find( "--uBody=" ) != string::npos ) {
	if ( units == FG_UNITS_FEET ) {
	    uBody = atof( arg.substr(8) ) * FEET_TO_METER;
	} else {
	    uBody = atof( arg.substr(8) );
	}
    } else if ( arg.find( "--vBody=" ) != string::npos ) {
	if ( units == FG_UNITS_FEET ) {
	    vBody = atof( arg.substr(8) ) * FEET_TO_METER;
	} else {
	    vBody = atof( arg.substr(8) );
	}
    } else if ( arg.find( "--wBody=" ) != string::npos ) {
	if ( units == FG_UNITS_FEET ) {
	    wBody = atof( arg.substr(8) ) * FEET_TO_METER;
	} else {
	    wBody = atof( arg.substr(8) );
	}
    } else if ( arg.find( "--heading=" ) != string::npos ) {
	heading = atof( arg.substr(10) );
    } else if ( arg.find( "--roll=" ) != string::npos ) {
	roll = atof( arg.substr(7) );
    } else if ( arg.find( "--pitch=" ) != string::npos ) {
	pitch = atof( arg.substr(8) );
    } else if ( arg.find( "--fg-root=" ) != string::npos ) {
	fg_root = arg.substr( 10 );
    } else if ( arg.find( "--fdm=" ) != string::npos ) {
	flight_model = parse_fdm( arg.substr(6) );
    } else if ( arg.find( "--model-hz=" ) != string::npos ) {
	model_hz = atoi( arg.substr(11) );
    } else if ( arg.find( "--speed=" ) != string::npos ) {
	speed_up = atoi( arg.substr(8) );
    } else if ( arg == "--fog-disable" ) {
	fog = FG_FOG_DISABLED;	
    } else if ( arg == "--fog-fastest" ) {
	fog = FG_FOG_FASTEST;	
    } else if ( arg == "--fog-nicest" ) {
	fog = FG_FOG_NICEST;	
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
    } else if ( arg == "--units-feet" ) {
	units = FG_UNITS_FEET;	
    } else if ( arg == "--units-meters" ) {
	units = FG_UNITS_METERS;	
    } else if ( arg.find( "--tile-radius=" ) != string::npos ) {
	tile_radius = parse_tile_radius( arg.substr(14) );
	tile_diameter = tile_radius * 2 + 1;
    } else if ( arg.find( "--time-offset-sys=" ) != string::npos ) {
	time_offset = parse_time_offset( (arg.substr(18)) );
	time_offset_type = FG_TIME_SYS_OFFSET;
    } else if ( arg.find( "--time-offset-lat=") != string::npos ) {
        time_offset = parse_time_offset(arg.substr(18));
	time_offset_type = FG_TIME_LAT_OFFSET;
    } else if ( arg.find( "--time-offset-gmt=") != string::npos ) {
        time_offset = parse_time_offset(arg.substr(18));
	time_offset_type = FG_TIME_GMT_OFFSET;
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
    } else if ( arg.find( "--serial=" ) != string::npos ) {
	parse_serial( arg.substr(9) );
#ifdef FG_NETWORK_OLK
    } else if ( arg == "--net-hud" ) {
	net_hud_display = 1;	
    } else if ( arg.find( "--net-id=") != string::npos ) {
	net_id = arg.substr( 9 );
#endif
    } else {
	FG_LOG( FG_GENERAL, FG_ALERT, "Unknown option '" << arg << "'" );
	return FG_OPTIONS_ERROR;
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
    
    return(FG_OPTIONS_OK);
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
    cout << "\t--disable-pause:  start out in an active state" << endl;
    cout << "\t--enable-pause:  start out in a paused state" << endl;
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
    cout << endl;
 
    cout << "Flight Model:" << endl;
    cout << "\t--fdm=abcd:  one of slew, jsb, larcsim, or external" << endl;
    cout << "\t--model-hz=n:  run the FDM this rate (iterations per second)" 
	 << endl;
    cout << "\t--speed=n:  run the FDM this much faster than real time" << endl;
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
    cout << endl;

    cout << "Rendering Options:" << endl;
    cout << "\t--fog-disable:  disable fog/haze" << endl;
    cout << "\t--fog-fastest:  enable fastest fog/haze" << endl;
    cout << "\t--fog-nicest:  enable nicest fog/haze" << endl;
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
    cout << "\t--time-offset-sys=[+-]hh:mm:ss: add this time offset to" << endl
	 << "\t\tyour system time" << endl;
    cout << "\t--time-offset-gmt:[+-]hh:mm:ss: add this time offset to" << endl
	 << "\t\tGreenwich Mean Time (GMT)" << endl;
    cout << "\t--time-offset-lat:[+-]hh:mm:ss: add this time offset to" << endl
	 << "\t\tLocal Aircraft Time (LAT)" << endl;   
    cout << "\t--start-date-sys=yyyy:mm:dd:hh:mm:ss: specify a starting" << endl
	 << "\t\tdate/time. Uses your system time " << endl;
    cout << "\t--start-date-gmt=yyyy:mm:dd:hh:mm:ss: specify a starting" << endl
	 << "\t\tdate/time. Uses Greenwich Mean Time" << endl;
    cout << "\t--start-date-lat=yyyy:mm:dd:hh:mm:ss: specify a starting" << endl
	 << "\t\tdate/time. Uses Local Aircraft Time" << endl;
#ifdef FG_NETWORK_OLK
    cout << "" << endl;

    cout << "Network Options:" << endl;
    cout << "\t--net-hud:  Hud displays network info" << endl;
    cout << "\t--net-id=name:  specify your own callsign" << endl;
#endif
}


// Destructor
fgOPTIONS::~fgOPTIONS( void ) {
}
