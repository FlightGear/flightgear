//
// options.hxx -- class to handle command line options
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


#ifndef _OPTIONS_HXX
#define _OPTIONS_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#define FG_OPTIONS_OK 0
#define FG_OPTIONS_HELP 1
#define FG_OPTIONS_ERROR 2


class fgOPTIONS {

    // The flight gear "root" directory
    char fg_root[256];

    // Starting position and orientation
    char airport_id[5]; // ID of initial starting airport
    double lon;         // starting longitude in degrees (west = -)
    double lat;         // starting latitude in degrees (south = -)
    double altitude;    // starting altitude in meters
    double heading;     // heading (yaw) angle in degress (Psi)
    double roll;        // roll angle in degrees (Phi)
    double pitch;       // pitch angle in degrees (Theta)

    // Miscellaneous
    int splash_screen; // show splash screen
    int intro_music;   // play introductory music
    int mouse_pointer; // show mouse pointer
    int pause;         // pause intially enabled/disabled

    // Features
    int hud_status;    // HUD on/off
    int panel_status;  // Panel on/off
    int sound;         // play sound effects

    // Flight Model options
    int flight_model;  // Flight Model:  FG_SLEW, FG_LARCSIM, etc.

    // Rendering options
    int fog;           // Fog enabled/disabled
    double fov;        // Field of View
    int fullscreen;    // Full screen mode enabled/disabled
    int shading;       // shading method, 0 = Flat, 1 = Smooth
    int skyblend;      // Blend sky to haze (using polygons) or just clear
    int textures;      // Textures enabled/disabled
    int wireframe;     // Wireframe mode enabled/disabled

    // Scenery options
    int tile_radius;   // Square radius of rendered tiles (around center 
                       // square.)
    int tile_diameter; // Diameter of rendered tiles.  for instance
                       // if tile_diameter = 3 then a 3 x 3 grid of tiles will 
                       // be drawn.  Increase this to see terrain that is 
                       // further away.

    // Time options
    int time_offset;   // Offset true time by this many seconds

public:

    // Constructor
    fgOPTIONS( void );

    // Parse a single option
    int parse_option( char *arg );

    // Parse the command line options
    int parse_command_line( int argc, char **argv );

    // Parse the command line options
    int parse_config_file( char *path );

    // Print usage message
    void usage ( void );

    // Query functions
    void get_fg_root(char *root);
    void get_airport_id(char *id);
    double get_lon( void );
    double get_lat( void );
    double get_altitude( void );
    double get_heading( void );
    double get_roll( void );
    double get_pitch( void );
    int get_splash_screen( void );
    int get_intro_music( void );
    int get_mouse_pointer( void );
    int get_pause( void );
    int get_hud_status( void );
    int get_panel_status( void );
    int get_sound( void );
    int get_flight_model( void );
    int get_fog( void );
    double get_fov( void );
    int get_fullscreen( void );
    int get_shading( void );
    int get_skyblend( void );
    int get_textures( void );
    int get_wireframe( void );
    int get_tile_radius( void );
    int get_tile_diameter( void );
    int get_time_offset( void );

    // Update functions
    void set_hud_status( int status );
    void set_fov( double amount );

    // Destructor
    ~fgOPTIONS( void );

};


extern fgOPTIONS current_options;


#endif /* _OPTIONS_HXX */


// $Log$
// Revision 1.13  1998/07/30 23:48:29  curt
// Output position & orientation when pausing.
// Eliminated libtool use.
// Added options to specify initial position and orientation.
// Changed default fov to 55 degrees.
// Added command line option to start in paused or unpaused state.
//
// Revision 1.12  1998/07/27 18:41:26  curt
// Added a pause command "p"
// Fixed some initialization order problems between pui and glut.
// Added an --enable/disable-sound option.
//
// Revision 1.11  1998/07/13 21:01:39  curt
// Wrote access functions for current fgOPTIONS.
//
// Revision 1.10  1998/07/06 21:34:20  curt
// Added an enable/disable splash screen option.
// Added an enable/disable intro music option.
// Added an enable/disable instrument panel option.
// Added an enable/disable mouse pointer option.
// Added using namespace std for compilers that support this.
//
// Revision 1.9  1998/06/27 16:54:34  curt
// Replaced "extern displayInstruments" with a entry in fgOPTIONS.
// Don't change the view port when displaying the panel.
//
// Revision 1.8  1998/05/16 13:08:36  curt
// C++ - ified views.[ch]xx
// Shuffled some additional view parameters into the fgVIEW class.
// Changed tile-radius to tile-diameter because it is a much better
//   name.
// Added a WORLD_TO_EYE transformation to views.cxx.  This allows us
//  to transform world space to eye space for view frustum culling.
//
// Revision 1.7  1998/05/13 18:29:59  curt
// Added a keyboard binding to dynamically adjust field of view.
// Added a command line option to specify fov.
// Adjusted terrain color.
// Root path info moved to fgOPTIONS.
// Added ability to parse options out of a config file.
//
// Revision 1.6  1998/05/06 03:16:26  curt
// Added an averaged global frame rate counter.
// Added an option to control tile radius.
//
// Revision 1.5  1998/05/03 00:47:32  curt
// Added an option to enable/disable full-screen mode.
//
// Revision 1.4  1998/04/30 12:34:19  curt
// Added command line rendering options:
//   enable/disable fog/haze
//   specify smooth/flat shading
//   disable sky blending and just use a solid color
//   enable wireframe drawing mode
//
// Revision 1.3  1998/04/28 01:20:23  curt
// Type-ified fgTIME and fgVIEW.
// Added a command line option to disable textures.
//
// Revision 1.2  1998/04/25 15:11:13  curt
// Added an command line option to set starting position based on airport ID.
//
// Revision 1.1  1998/04/24 00:49:21  curt
// Wrapped "#include <config.h>" in "#ifdef HAVE_CONFIG_H"
// Trying out some different option parsing code.
// Some code reorganization.
//
//
