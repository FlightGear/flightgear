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
// (Log is kept at end of this file)


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <math.h>            // rint()
#include <stdio.h>
#include <stdlib.h>          // atof(), atoi()
#include <string.h>
#include <string>

#include <Debug/logstream.hxx>
#include <FDM/flight.hxx>
#include <Include/fg_constants.h>
#include <Main/options.hxx>
#include <Misc/fgstream.hxx>

#include "fg_serial.hxx"


inline double
atof( const string& str )
{
    return ::atof( str.c_str() );
}

inline int
atoi( const string& str )
{
    return ::atoi( str.c_str() );
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

    // Miscellaneous
    game_mode(0),
    splash_screen(1),
    intro_music(1),
    mouse_pointer(0),
    pause(0),

    // Features
    hud_status(1),
    panel_status(0),
    sound(1),

    // Flight Model options
    flight_model(FGInterface::FG_LARCSIM),

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

    // Scenery options
    tile_diameter(5),

    // HUD options
    units(FG_UNITS_FEET),
    tris_or_culled(0),
	
    // Time options
    time_offset(0)

{
    // set initial values/defaults
    char* envp = ::getenv( "FG_ROOT" );

    if ( envp != NULL ) {
	// fg_root could be anywhere, so default to environmental
	// variable $FG_ROOT if it is set.
	fg_root = envp;
    } else {
	// Otherwise, default to a random compiled in location if
	// $FG_ROOT is not set.  This can still be overridden from the
	// command line or a config file.

#if defined(WIN32)
	fg_root = "\\FlightGear";
#else
	fg_root = "/usr/local/lib/FlightGear";
#endif
    }

    airport_id = "";  // default airport id

    // initialize port config string list
    port_options_list.erase ( port_options_list.begin(), 
			      port_options_list.end() );
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


// parse degree in the form of [+/-]hhh:mm:ss
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
//  format = {nmea, fgfs}
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
    } else if ( arg == "--disable-hud" ) {
	hud_status = false;	
    } else if ( arg == "--enable-hud" ) {
	hud_status = true;	
    } else if ( arg == "--disable-panel" ) {
	panel_status = false;
    } else if ( arg == "--enable-panel" ) {
	panel_status = true;
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
	string geometry = arg.substr( 11 );
	if ( geometry == "640x480" ) {
	    xsize = 640;
	    ysize = 480;
	} else if ( geometry == "800x600" ) {
	    xsize = 800;
	    ysize = 600;
	} else if ( geometry == "1024x768" ) {
	    xsize = 1024;
	    ysize = 768;
	} else {
	    FG_LOG( FG_GENERAL, FG_ALERT, "Unknown geometry: " << geometry );
	    exit(-1);
	}
    } else if ( arg == "--units-feet" ) {
	units = FG_UNITS_FEET;	
    } else if ( arg == "--units-meters" ) {
	units = FG_UNITS_METERS;	
    } else if ( arg.find( "--tile-radius=" ) != string::npos ) {
	tile_radius = parse_tile_radius( arg.substr(14) );
	tile_diameter = tile_radius * 2 + 1;
    } else if ( arg.find( "--time-offset=" ) != string::npos ) {
	time_offset = parse_time_offset( (arg.substr(14)) );
    } else if ( arg == "--hud-tris" ) {
	tris_or_culled = 0;	
    } else if ( arg == "--hud-culled" ) {
	tris_or_culled = 1;
    } else if ( arg.find( "--serial=" ) != string::npos ) {
	parse_serial( arg.substr(9) );
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
    if ( !in )
	return(FG_OPTIONS_ERROR);

    FG_LOG( FG_GENERAL, FG_INFO, "Processing config file: " << path );

    in >> skipcomment;
    while ( !in.eof() )
    {
	string line;
	getline( in, line );

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
    printf("Usage: fg [ options ... ]\n");
    printf("\n");

    printf("General Options:\n");
    printf("\t--help -h:  print usage\n");
    printf("\t--fg-root=path:  specify the root path for all the data files\n");
    printf("\t--disable-game-mode:  disable full-screen game mode\n");
    printf("\t--enable-game-mode:  enable full-screen game mode\n");
    printf("\t--disable-splash-screen:  disable splash screen\n");
    printf("\t--enable-splash-screen:  enable splash screen\n");
    printf("\t--disable-intro-music:  disable introduction music\n");
    printf("\t--enable-intro-music:  enable introduction music\n");
    printf("\t--disable-mouse-pointer:  disable extra mouse pointer\n");
    printf("\t--enable-mouse-pointer:  enable extra mouse pointer (i.e. for\n");
    printf("\t\tfull screen voodoo/voodoo-II based cards.)\n");
    printf("\t--disable-pause:  start out in an active state\n");
    printf("\t--enable-pause:  start out in a paused state\n");
    printf("\n");

    printf("Features:\n");
    printf("\t--disable-hud:  disable heads up display\n");
    printf("\t--enable-hud:  enable heads up display\n");
    printf("\t--disable-panel:  disable instrument panel\n");
    printf("\t--enable-panel:  enable instrumetn panel\n");
    printf("\t--disable-sound:  disable sound effects\n");
    printf("\t--enable-sound:  enable sound effects\n");
    printf("\n");
 
    printf("Flight Model:\n");
    printf("\t--fdm=abcd:  one of slew, jsb, larcsim, or external\n");
    printf("\n");

    printf("Initial Position and Orientation:\n");
    printf("\t--airport-id=ABCD:  specify starting postion by airport id\n");
    printf("\t--lon=degrees:  starting longitude in degrees (west = -)\n");
    printf("\t--lat=degrees:  starting latitude in degrees (south = -)\n");
    printf("\t--altitude=feet:  starting altitude in feet\n");
    printf("\t\t(unless --units-meters specified\n");
    printf("\t--heading=degrees:  heading (yaw) angle in degress (Psi)\n");
    printf("\t--roll=degrees:  roll angle in degrees (Phi)\n");
    printf("\t--pitch=degrees:  pitch angle in degrees (Theta)\n");
    printf("\n");

    printf("Rendering Options:\n");
    printf("\t--fog-disable:  disable fog/haze\n");
    printf("\t--fog-fastest:  enable fastest fog/haze\n");
    printf("\t--fog-nicest:  enable nicest fog/haze\n");
    printf("\t--fov=xx.x:  specify initial field of view angle in degrees\n");
    printf("\t--disable-fullscreen:  disable fullscreen mode\n");
    printf("\t--enable-fullscreen:  enable fullscreen mode\n");
    printf("\t--shading-flat:  enable flat shading\n");
    printf("\t--shading-smooth:  enable smooth shading\n");
    printf("\t--disable-skyblend:  disable sky blending\n");
    printf("\t--enable-skyblend:  enable sky blending\n");
    printf("\t--disable-textures:  disable textures\n");
    printf("\t--enable-textures:  enable textures\n");
    printf("\t--disable-wireframe:  disable wireframe drawing mode\n");
    printf("\t--enable-wireframe:  enable wireframe drawing mode\n");
    printf("\t--geometry=WWWxHHH:  window geometry: 640x480, 800x600, etc.\n");
    printf("\n");

    printf("Scenery Options:\n");
    printf("\t--tile-radius=n:  specify tile radius, must be 1 - 4\n");
    printf("\n");

    printf("Hud Options:\n");
    printf("\t--units-feet:  Hud displays units in feet\n");
    printf("\t--units-meters:  Hud displays units in meters\n");
    printf("\t--hud-tris:  Hud displays number of triangles rendered\n");
    printf("\t--hud-culled:  Hud displays percentage of triangles culled\n");
    printf("\n");
	
    printf("Time Options:\n");
    printf("\t--time-offset=[+-]hh:mm:ss:  offset local time by this amount\n");
}


// Destructor
fgOPTIONS::~fgOPTIONS( void ) {
}


// $Log$
// Revision 1.39  1999/02/05 21:29:12  curt
// Modifications to incorporate Jon S. Berndts flight model code.
//
// Revision 1.38  1999/02/01 21:33:35  curt
// Renamed FlightGear/Simulator/Flight to FlightGear/Simulator/FDM since
// Jon accepted my offer to do this and thought it was a good idea.
//
// Revision 1.37  1999/01/19 20:57:05  curt
// MacOS portability changes contributed by "Robert Puyol" <puyol@abvent.fr>
//
// Revision 1.36  1999/01/07 20:25:10  curt
// Updated struct fgGENERAL to class FGGeneral.
//
// Revision 1.35  1998/12/06 14:52:57  curt
// Fixed a problem with the initial starting altitude.  "v->abs_view_pos" wasn't
// being calculated correctly at the beginning causing the first terrain
// intersection to fail, returning a ground altitude of zero, causing the plane
// to free fall for one frame, until the ground altitude was corrected, but now
// being under the ground we got a big bounce and the plane always ended up
// upside down.
//
// Revision 1.34  1998/12/05 15:54:22  curt
// Renamed class fgFLIGHT to class FGState as per request by JSB.
//
// Revision 1.33  1998/12/04 01:30:44  curt
// Added support for the External flight model.
//
// Revision 1.32  1998/11/25 01:34:00  curt
// Support for an arbitrary number of serial ports.
//
// Revision 1.31  1998/11/23 21:49:04  curt
// Borland portability tweaks.
//
// Revision 1.30  1998/11/16 14:00:02  curt
// Added pow() macro bug work around.
// Added support for starting FGFS at various resolutions.
// Added some initial serial port support.
// Specify default log levels in main().
//
// Revision 1.29  1998/11/06 21:18:12  curt
// Converted to new logstream debugging facility.  This allows release
// builds with no messages at all (and no performance impact) by using
// the -DFG_NDEBUG flag.
//
// Revision 1.28  1998/11/06 14:47:03  curt
// Changes to track Bernie's updates to fgstream.
//
// Revision 1.27  1998/11/02 23:04:04  curt
// HUD units now display in feet by default with meters being a command line
// option.
//
// Revision 1.26  1998/10/17 01:34:24  curt
// C++ ifying ...
//
// Revision 1.25  1998/09/15 02:09:27  curt
// Include/fg_callback.hxx
//   Moved code inline to stop g++ 2.7 from complaining.
//
// Simulator/Time/event.[ch]xx
//   Changed return type of fgEVENT::printStat().  void caused g++ 2.7 to
//   complain bitterly.
//
// Minor bugfix and changes.
//
// Simulator/Main/GLUTmain.cxx
//   Added missing type to idle_state definition - eliminates a warning.
//
// Simulator/Main/fg_init.cxx
//   Changes to airport lookup.
//
// Simulator/Main/options.cxx
//   Uses fg_gzifstream when loading config file.
//
// Revision 1.24  1998/09/08 15:04:33  curt
// Optimizations by Norman Vine.
//
// Revision 1.23  1998/08/27 17:02:07  curt
// Contributions from Bernie Bright <bbright@c031.aone.net.au>
// - use strings for fg_root and airport_id and added methods to return
//   them as strings,
// - inlined all access methods,
// - made the parsing functions private methods,
// - deleted some unused functions.
// - propogated some of these changes out a bit further.
//
// Revision 1.22  1998/08/24 20:11:13  curt
// Added i/I to toggle full vs. minimal HUD.
// Added a --hud-tris vs --hud-culled option.
// Moved options accessor funtions to options.hxx.
//
// Revision 1.21  1998/08/20 15:10:34  curt
// Added GameGLUT support.
//
// Revision 1.20  1998/07/30 23:48:28  curt
// Output position & orientation when pausing.
// Eliminated libtool use.
// Added options to specify initial position and orientation.
// Changed default fov to 55 degrees.
// Added command line option to start in paused or unpaused state.
//
// Revision 1.19  1998/07/27 18:41:25  curt
// Added a pause command "p"
// Fixed some initialization order problems between pui and glut.
// Added an --enable/disable-sound option.
//
// Revision 1.18  1998/07/22 01:27:03  curt
// Strip out \r when parsing config file in case we are on a windoze system.
//
// Revision 1.17  1998/07/16 17:33:38  curt
// "H" / "h" now control hud brightness as well with off being one of the
//   states.
// Better checking for xmesa/fx 3dfx fullscreen/window support for deciding
//   whether or not to build in the feature.
// Translucent menu support.
// HAVE_AUDIO_SUPPORT -> ENABLE_AUDIO_SUPPORT
// Use fork() / wait() for playing mp3 init music in background under unix.
// Changed default tile diameter to 5.
//
// Revision 1.16  1998/07/13 21:01:39  curt
// Wrote access functions for current fgOPTIONS.
//
// Revision 1.15  1998/07/06 21:34:19  curt
// Added an enable/disable splash screen option.
// Added an enable/disable intro music option.
// Added an enable/disable instrument panel option.
// Added an enable/disable mouse pointer option.
// Added using namespace std for compilers that support this.
//
// Revision 1.14  1998/07/04 00:52:26  curt
// Add my own version of gluLookAt() (which is nearly identical to the
// Mesa/glu version.)  But, by calculating the Model View matrix our selves
// we can save this matrix without having to read it back in from the video
// card.  This hopefully allows us to save a few cpu cycles when rendering
// out the fragments because we can just use glLoadMatrixd() with the
// precalculated matrix for each tile rather than doing a push(), translate(),
// pop() for every fragment.
//
// Panel status defaults to off for now until it gets a bit more developed.
//
// Extract OpenGL driver info on initialization.
//
// Revision 1.13  1998/06/27 16:54:34  curt
// Replaced "extern displayInstruments" with a entry in fgOPTIONS.
// Don't change the view port when displaying the panel.
//
// Revision 1.12  1998/06/17 21:35:13  curt
// Refined conditional audio support compilation.
// Moved texture parameter setup calls to ../Scenery/materials.cxx
// #include <string.h> before various STL includes.
// Make HUD default state be enabled.
//
// Revision 1.11  1998/06/13 00:40:33  curt
// Tweaked fog command line options.
//
// Revision 1.10  1998/05/16 13:08:36  curt
// C++ - ified views.[ch]xx
// Shuffled some additional view parameters into the fgVIEW class.
// Changed tile-radius to tile-diameter because it is a much better
//   name.
// Added a WORLD_TO_EYE transformation to views.cxx.  This allows us
//  to transform world space to eye space for view frustum culling.
//
// Revision 1.9  1998/05/13 18:29:59  curt
// Added a keyboard binding to dynamically adjust field of view.
// Added a command line option to specify fov.
// Adjusted terrain color.
// Root path info moved to fgOPTIONS.
// Added ability to parse options out of a config file.
//
// Revision 1.8  1998/05/07 23:14:16  curt
// Added "D" key binding to set autopilot heading.
// Made frame rate calculation average out over last 10 frames.
// Borland C++ floating point exception workaround.
// Added a --tile-radius=n option.
//
// Revision 1.7  1998/05/06 03:16:25  curt
// Added an averaged global frame rate counter.
// Added an option to control tile radius.
//
// Revision 1.6  1998/05/03 00:47:32  curt
// Added an option to enable/disable full-screen mode.
//
// Revision 1.5  1998/04/30 12:34:19  curt
// Added command line rendering options:
//   enable/disable fog/haze
//   specify smooth/flat shading
//   disable sky blending and just use a solid color
//   enable wireframe drawing mode
//
// Revision 1.4  1998/04/28 01:20:22  curt
// Type-ified fgTIME and fgVIEW.
// Added a command line option to disable textures.
//
// Revision 1.3  1998/04/26 05:01:19  curt
// Added an rint() / HAVE_RINT check.
//
// Revision 1.2  1998/04/25 15:11:12  curt
// Added an command line option to set starting position based on airport ID.
//
// Revision 1.1  1998/04/24 00:49:21  curt
// Wrapped "#include <config.h>" in "#ifdef HAVE_CONFIG_H"
// Trying out some different option parsing code.
// Some code reorganization.
//
//
