//
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

#include <Debug/fg_debug.h>
#include <Include/fg_constants.h>
#include <Include/fg_zlib.h>

#include "options.hxx"


// Defined the shared options class here
fgOPTIONS current_options;


// Constructor
fgOPTIONS::fgOPTIONS( void ) {
    // set initial values/defaults

    if ( getenv("FG_ROOT") != NULL ) {
	// fg_root could be anywhere, so default to environmental
	// variable $FG_ROOT if it is set.

	strcpy(fg_root, getenv("FG_ROOT"));
    } else {
	// Otherwise, default to a random compiled in location if
	// $FG_ROOT is not set.  This can still be overridden from the
	// command line or a config file.

#if defined(WIN32)
	strcpy(fg_root, "\\FlightGear");
#else
	strcpy(fg_root, "/usr/local/lib/FlightGear");
#endif
    }

    // default airport id
    strcpy(airport_id, "");

    // Miscellaneous
    splash_screen = 1;
    intro_music = 1;
    mouse_pointer = 0;

    // Features
    hud_status = 1;
    panel_status = 0;

    // Rendering options
    fog = 2;    // nicest
    fov = 65.0;
    fullscreen = 0;
    shading = 1;
    skyblend = 1;
    textures = 1;
    wireframe = 0;

    // Scenery options
    tile_diameter = 5;

    // Time options
    time_offset = 0;
}


// Parse an int out of a --foo-bar=n type option 
static int parse_int(char *arg) {
    int result;

    // advance past the '='
    while ( (arg[0] != '=') && (arg[0] != '\0') ) {
	arg++;
    }

    if ( arg[0] == '=' ) {
	arg++;
    }

    // printf("parse_int(): arg = %s\n", arg);

    result = atoi(arg);

    // printf("parse_int(): result = %d\n", result);

    return(result);
}


// Parse an int out of a --foo-bar=n type option 
static double parse_double(char *arg) {
    double result;

    // advance past the '='
    while ( (arg[0] != '=') && (arg[0] != '\0') ) {
	arg++;
    }

    if ( arg[0] == '=' ) {
	arg++;
    }

    printf("parse_double(): arg = %s\n", arg);

    result = atof(arg);

    printf("parse_double(): result = %.4f\n", result);

    return(result);
}


// parse time string in the form of [+-]hh:mm:ss, returns the value in seconds
static double parse_time(char *time_str) {
    char num[256];
    double hours, minutes, seconds;
    double result = 0.0;
    int sign = 1;
    int i;

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

	result += hours * 3600.0;
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

	result += minutes * 60.0;
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

	result += seconds;
    }

    return(sign * result);
}


// parse time offset command line option
static int parse_time_offset(char *time_str) {
    int result;

    time_str += 14;

    // printf("time offset = %s\n", time_str);

#ifdef HAVE_RINT
    result = (int)rint(parse_time(time_str));
#else
    result = (int)parse_time(time_str);
#endif

    printf("parse_time_offset(): %d\n", result);

    return( result );
}


// Parse --tile-diameter=n type option 

#define FG_RADIUS_MIN 1
#define FG_RADIUS_MAX 4

static int parse_tile_radius(char *arg) {
    int radius, tmp;

    radius = parse_int(arg);

    if ( radius < FG_RADIUS_MIN ) { radius = FG_RADIUS_MIN; }
    if ( radius > FG_RADIUS_MAX ) { radius = FG_RADIUS_MAX; }

    // printf("parse_tile_radius(): radius = %d\n", radius);

    return(radius);
}


// Parse --fov=x.xx type option 
static double parse_fov(char *arg) {
    double fov;

    fov = parse_double(arg);

    if ( fov < FG_FOV_MIN ) { fov = FG_FOV_MIN; }
    if ( fov > FG_FOV_MAX ) { fov = FG_FOV_MAX; }

    printf("parse_fov(): result = %.4f\n", fov);

    return(fov);
}


// Parse a single option
int fgOPTIONS::parse_option( char *arg ) {
    // General Options
    if ( (strcmp(arg, "--help") == 0) ||
	 (strcmp(arg, "-h") == 0) ) {
	// help/usage request
	return(FG_OPTIONS_HELP);
    } else if ( strcmp(arg, "--disable-splash-screen") == 0 ) {
	splash_screen = 0;
    } else if ( strcmp(arg, "--enable-splash-screen") == 0 ) {
	splash_screen = 1;
    } else if ( strcmp(arg, "--disable-intro-music") == 0 ) {
	intro_music = 0;
    } else if ( strcmp(arg, "--enable-intro-music") == 0 ) {
	intro_music = 1;
    } else if ( strcmp(arg, "--disable-mouse-pointer") == 0 ) {
	mouse_pointer = 1;
    } else if ( strcmp(arg, "--enable-mouse-pointer") == 0 ) {
	mouse_pointer = 2;
    } else if ( strcmp(arg, "--disable-hud") == 0 ) {
	hud_status = 0;	
    } else if ( strcmp(arg, "--enable-hud") == 0 ) {
	hud_status = 1;	
    } else if ( strcmp(arg, "--disable-panel") == 0 ) {
	panel_status = 0;
    } else if ( strcmp(arg, "--enable-panel") == 0 ) {
	panel_status = 1;
    } else if ( strcmp(arg, "--disable-sound") == 0 ) {
	sound = 0;
    } else if ( strcmp(arg, "--enable-sound") == 0 ) {
	sound = 1;
    } else if ( strncmp(arg, "--airport-id=", 13) == 0 ) {
	arg += 13;
	strncpy(airport_id, arg, 4);
    } else if ( strncmp(arg, "--fg-root=", 10) == 0 ) {
	arg += 10;
	strcpy(fg_root, arg);
    } else if ( strcmp(arg, "--fog-disable") == 0 ) {
	fog = 0;	
    } else if ( strcmp(arg, "--fog-fastest") == 0 ) {
	fog = 1;	
    } else if ( strcmp(arg, "--fog-nicest") == 0 ) {
	fog = 2;	
    } else if ( strncmp(arg, "--fov=", 6) == 0 ) {
	fov = parse_fov(arg);
    } else if ( strcmp(arg, "--disable-fullscreen") == 0 ) {
	fullscreen = 0;	
    } else if ( strcmp(arg, "--enable-fullscreen") == 0 ) {
	fullscreen = 1;	
    } else if ( strcmp(arg, "--shading-flat") == 0 ) {
	shading = 0;	
    } else if ( strcmp(arg, "--shading-smooth") == 0 ) {
	shading = 1;	
    } else if ( strcmp(arg, "--disable-skyblend") == 0 ) {
	skyblend = 0;	
    } else if ( strcmp(arg, "--enable-skyblend") == 0 ) {
	skyblend = 1;	
    } else if ( strcmp(arg, "--disable-textures") == 0 ) {
	textures = 0;	
    } else if ( strcmp(arg, "--enable-textures") == 0 ) {
	textures = 1;	
    } else if ( strcmp(arg, "--disable-wireframe") == 0 ) {
	wireframe = 0;	
    } else if ( strcmp(arg, "--enable-wireframe") == 0 ) {
	wireframe = 1;	
    } else if ( strncmp(arg, "--tile-radius=", 14) == 0 ) {
	tile_radius = parse_tile_radius(arg);
	tile_diameter = tile_radius * 2 + 1;
    } else if ( strncmp(arg, "--time-offset=", 14) == 0 ) {
	time_offset = parse_time_offset(arg);
    } else {
	return(FG_OPTIONS_ERROR);
    }
    
    return(FG_OPTIONS_OK);
}


// Parse the command line options
int fgOPTIONS::parse_command_line( int argc, char **argv ) {
    int i = 1;
    int result;

    fgPrintf(FG_GENERAL, FG_INFO, "Processing command line arguments\n");

    while ( i < argc ) {
	fgPrintf(FG_GENERAL, FG_DEBUG, "argv[%d] = %s\n", i, argv[i]);

	result = parse_option(argv[i]);
	if ( (result == FG_OPTIONS_HELP) || (result == FG_OPTIONS_ERROR) ) {
	    return(result);
	}

	i++;
    }
    
    return(FG_OPTIONS_OK);
}


// Parse the command line options
int fgOPTIONS::parse_config_file( char *path ) {
    char fgpath[256], line[256];
    fgFile f;
    int len, result;

    strcpy(fgpath, path);
    strcat(fgpath, ".gz");

    // first try "path.gz"
    if ( (f = fgopen(fgpath, "rb")) == NULL ) {
	// next try "path"
        if ( (f = fgopen(path, "rb")) == NULL ) {
	    return(FG_OPTIONS_ERROR);
	}
    }

    fgPrintf(FG_GENERAL, FG_INFO, "Processing config file: %s\n", path);

    while ( fggets(f, line, 250) != NULL ) {
	// strip trailing newline if it exists
	len = strlen(line);
	if ( line[len-1] == '\n' ) {
	    line[len-1] = '\0';
	}

        // strip dos ^M if it exists
	len = strlen(line);
	if ( line[len-1] == '\r' ) {
	    line[len-1] = '\0';
	}

	result = parse_option(line);
	if ( result == FG_OPTIONS_ERROR ) {
	    fgPrintf( FG_GENERAL, FG_EXIT, 
		      "Config file parse error: %s '%s'\n", path, line );
	}
    }

    fgclose(f);
    return(FG_OPTIONS_OK);
}


// Print usage message
void fgOPTIONS::usage ( void ) {
    printf("Usage: fg [ options ... ]\n");
    printf("\n");

    printf("General Options:\n");
    printf("\t--help -h:  print usage\n");
    printf("\t--fg-root=path:  specify the root path for all the data files\n");
    printf("\t--disable-splash-screen:  disable splash screen\n");
    printf("\t--enable-splash-screen:  enable splash screen\n");
    printf("\t--disable-intro-music:  disable introduction music\n");
    printf("\t--enable-intro-music:  enable introduction music\n");
    printf("\t--disable-mouse-pointer:  disable extra mouse pointer\n");
    printf("\t--enable-mouse-pointer:  enable extra mouse pointer (i.e. for\n");
    printf("\t\tfull screen voodoo/voodoo-II based cards.\n");
    printf("\n");

    printf("Features:\n");
    printf("\t--disable-hud:  disable heads up display\n");
    printf("\t--enable-hud:  enable heads up display\n");
    printf("\t--disable-panel:  disable instrument panel\n");
    printf("\t--enable-panel:  enable instrumetn panel\n");
    printf("\t--disable-sound:  disable sound effects\n");
    printf("\t--enable-sound:  enable sound effects\n");
    printf("\n");
 
    printf("Initial Position:\n");
    printf("\t--airport-id=ABCD:  specify starting postion by airport id\n");
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
    printf("\n");

    printf("Scenery Options:\n");
    printf("\t--tile-radius=n:  specify tile radius, must be 1 - 4\n");
    printf("\n");

    printf("Time Options:\n");
    printf("\t--time-offset=[+-]hh:mm:ss:  offset local time by this amount\n");
}


// Query functions
void fgOPTIONS::get_fg_root(char *root) { strcpy(root, fg_root); }
void fgOPTIONS::get_airport_id(char *id) { strcpy(id, airport_id); }
int fgOPTIONS::get_splash_screen( void ) { return(splash_screen); }
int fgOPTIONS::get_intro_music( void ) { return(intro_music); }
int fgOPTIONS::get_mouse_pointer( void ) { return(mouse_pointer); }
int fgOPTIONS::get_hud_status( void ) { return(hud_status); }
int fgOPTIONS::get_panel_status( void ) { return(panel_status); }
int fgOPTIONS::get_sound( void ) { return(sound); }
int fgOPTIONS::get_fog( void ) { return(fog); }
double fgOPTIONS::get_fov( void ) { return(fov); }
int fgOPTIONS::get_fullscreen( void ) { return(fullscreen); }
int fgOPTIONS::get_shading( void ) { return(shading); }
int fgOPTIONS::get_skyblend( void ) { return(skyblend); }
int fgOPTIONS::get_textures( void ) { return(textures); }
int fgOPTIONS::get_wireframe( void ) { return(wireframe); }
int fgOPTIONS::get_tile_radius( void ) { return(tile_radius); }
int fgOPTIONS::get_tile_diameter( void ) { return(tile_diameter); }
int fgOPTIONS::get_time_offset( void ) { return(time_offset); }


// Update functions
void fgOPTIONS::set_hud_status( int status ) { hud_status = status; }
void fgOPTIONS::set_fov( double amount ) { fov = amount; }

// Destructor
fgOPTIONS::~fgOPTIONS( void ) {
}


// $Log$
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
