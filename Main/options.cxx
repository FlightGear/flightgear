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
#include <stdlib.h>          // atof()
#include <string.h>

#include <Debug/fg_debug.h>

#include "options.hxx"


// Defined the shared options class here
fgOPTIONS current_options;


// Constructor
fgOPTIONS::fgOPTIONS( void ) {
    // set initial values/defaults

    strcpy(airport_id, "");

    // Features
    hud_status = 0;

    // Rendering options
    fog = 1;
    fullscreen = 0;
    shading = 1;
    skyblend = 1;
    textures = 1;
    wireframe = 0;

    // Time options
    time_offset = 0;
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


// Parse the command line options
int fgOPTIONS::parse( int argc, char **argv ) {
    int i = 1;

    fgPrintf(FG_GENERAL, FG_INFO, "Processing arguments\n");

    while ( i < argc ) {
	fgPrintf(FG_GENERAL, FG_INFO, "argv[%d] = %s\n", i, argv[i]);

	// General Options
	if ( (strcmp(argv[i], "--help") == 0) ||
	     (strcmp(argv[i], "-h") == 0) ) {
	    // help/usage request
	    return(FG_OPTIONS_HELP);
	} else if ( strcmp(argv[i], "--disable-hud") == 0 ) {
	    hud_status = 0;	
	} else if ( strcmp(argv[i], "--enable-hud") == 0 ) {
	    hud_status = 1;	
	} else if ( strncmp(argv[i], "--airport-id=", 13) == 0 ) {
	    argv[i] += 13;
	    strncpy(airport_id, argv[i], 4);
	} else if ( strcmp(argv[i], "--disable-fog") == 0 ) {
	    fog = 0;	
	} else if ( strcmp(argv[i], "--enable-fog") == 0 ) {
	    fog = 1;	
	} else if ( strcmp(argv[i], "--disable-fullscreen") == 0 ) {
	    fullscreen = 0;	
	} else if ( strcmp(argv[i], "--enable-fullscreen") == 0 ) {
	    fullscreen = 1;	
	} else if ( strcmp(argv[i], "--shading-flat") == 0 ) {
	    shading = 0;	
	} else if ( strcmp(argv[i], "--shading-smooth") == 0 ) {
	    shading = 1;	
	} else if ( strcmp(argv[i], "--disable-skyblend") == 0 ) {
	    skyblend = 0;	
	} else if ( strcmp(argv[i], "--enable-skyblend") == 0 ) {
	    skyblend = 1;	
	} else if ( strcmp(argv[i], "--disable-textures") == 0 ) {
	    textures = 0;	
	} else if ( strcmp(argv[i], "--enable-textures") == 0 ) {
	    textures = 1;	
	} else if ( strcmp(argv[i], "--disable-wireframe") == 0 ) {
	    wireframe = 0;	
	} else if ( strcmp(argv[i], "--enable-wireframe") == 0 ) {
	    wireframe = 1;	
	} else if ( strncmp(argv[i], "--time-offset=", 14) == 0 ) {
	    time_offset = parse_time_offset(argv[i]);
	} else {
	    return(FG_OPTIONS_ERROR);
	}

	i++;
    }
    
    return(FG_OPTIONS_OK);
}


// Print usage message
void fgOPTIONS::usage ( void ) {
    printf("Usage: fg [ options ... ]\n");
    printf("\n");

    printf("General Options:\n");
    printf("\t--help -h:  print usage\n");
    printf("\n");

    printf("Features:\n");
    printf("\t--disable-hud:  disable heads up display\n");
    printf("\t--enable-hud:  enable heads up display\n");
    printf("\n");
 
    printf("Initial Position:\n");
    printf("\t--airport-id=ABCD:  specify starting postion by airport id\n");
    printf("\n");

    printf("Rendering Options:\n");
    printf("\t--disable-fog:  disable fog/haze\n");
    printf("\t--enable-fog:  enable fog/haze\n");
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

    printf("Time Options:\n");
    printf("\t--time-offset=[+-]hh:mm:ss:  offset local time by this amount\n");
}


// Destructor
fgOPTIONS::~fgOPTIONS( void ) {
}


// $Log$
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
