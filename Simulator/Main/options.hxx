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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <Include/compiler.h>

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <GL/glut.h>
#include <XGL/xgl.h>

#if defined(FX) && defined(XMESA)
extern bool global_fullscreen;
#endif

#include STL_STRING
#include <vector>

FG_USING_STD(vector);
FG_USING_STD(string);

typedef vector < string > str_container;
typedef str_container::iterator str_iterator;
typedef str_container::const_iterator const_str_iterator;


class fgOPTIONS {

public:

    enum
    {
	FG_OPTIONS_OK = 0,
	FG_OPTIONS_HELP = 1,
	FG_OPTIONS_ERROR = 2
    };

    enum
    {
	FG_UNITS_FEET = 0,
	FG_UNITS_METERS = 1
    };

    enum fgFogKind
    {
	FG_FOG_DISABLED = 0,
	FG_FOG_FASTEST  = 1,
	FG_FOG_NICEST   = 2
    };

    enum
    {
	FG_RADIUS_MIN = 1,
	FG_RADIUS_MAX = 4
    };

private:

    // The flight gear "root" directory
    string fg_root;

    // Starting position and orientation
    string airport_id;  // ID of initial starting airport
    double lon;         // starting longitude in degrees (west = -)
    double lat;         // starting latitude in degrees (south = -)
    double altitude;    // starting altitude in meters
    double heading;     // heading (yaw) angle in degress (Psi)
    double roll;        // roll angle in degrees (Phi)
    double pitch;       // pitch angle in degrees (Theta)

    // Miscellaneous
    bool game_mode;     // Game mode enabled/disabled
    bool splash_screen; // show splash screen
    bool intro_music;   // play introductory music
    int mouse_pointer;  // show mouse pointer
    bool pause;         // pause intially enabled/disabled

    // Features
    bool hud_status;    // HUD on/off
    bool panel_status;  // Panel on/off
    bool sound;         // play sound effects

    // Flight Model options
    int flight_model;   // Flight Model:  FG_SLEW, FG_LARCSIM, etc.

    // Rendering options
    fgFogKind fog;      // Fog nicest/fastest/disabled
    double fov;         // Field of View
    bool fullscreen;    // Full screen mode enabled/disabled
    int shading;        // shading method, 0 = Flat, 1 = Smooth
    bool skyblend;      // Blend sky to haze (using polygons) or just clear
    bool textures;      // Textures enabled/disabled
    bool wireframe;     // Wireframe mode enabled/disabled
    int xsize, ysize;   // window size derived from geometry string

    // Scenery options
    int tile_radius;   // Square radius of rendered tiles (around center 
                       // square.)
    int tile_diameter; // Diameter of rendered tiles.  for instance
                       // if tile_diameter = 3 then a 3 x 3 grid of tiles will 
                       // be drawn.  Increase this to see terrain that is 
                       // further away.

    // HUD options
    int units;         // feet or meters
    int tris_or_culled;

    // Time options
    int time_offset;   // Offset true time by this many seconds

    // Serial Ports, we currently support up to four channels
    // fgSerialPortKind port_a_kind;  // Port a kind
    // fgSerialPortKind port_b_kind;  // Port b kind
    // fgSerialPortKind port_c_kind;  // Port c kind
    // fgSerialPortKind port_d_kind;  // Port d kind

    // Serial port configuration strings
    str_container port_options_list;
    
public:

    fgOPTIONS();
    ~fgOPTIONS();

    // Parse a single option
    int parse_option( const string& arg );

    // Parse the command line options
    int parse_command_line( int argc, char **argv );

    // Parse the command line options
    int parse_config_file( const string& path );

    // Print usage message
    void usage ( void );

    // Query functions
    inline string get_fg_root() const { return fg_root; }
    inline string get_airport_id() const { return airport_id; }
    inline double get_lon() const { return lon; }
    inline double get_lat() const { return lat; }
    inline double get_altitude() const { return altitude; }
    inline double get_heading() const { return heading; }
    inline double get_roll() const { return roll; }
    inline double get_pitch() const { return pitch; }
    inline bool get_game_mode() const { return game_mode; }
    inline bool get_splash_screen() const { return splash_screen; }
    inline bool get_intro_music() const { return intro_music; }
    inline int get_mouse_pointer() const { return mouse_pointer; }
    inline bool get_pause() const { return pause; }
    inline bool get_hud_status() const { return hud_status; }
    inline bool get_panel_status() const { return panel_status; }
    inline bool get_sound() const { return sound; }
    inline int get_flight_model() const { return flight_model; }
    inline bool fog_enabled() const { return fog != FG_FOG_DISABLED; }
    inline fgFogKind get_fog() const { return fog; }
    inline double get_fov() const { return fov; }
    inline bool get_fullscreen() const { return fullscreen; }
    inline int get_shading() const { return shading; }
    inline bool get_skyblend() const { return skyblend; }
    inline bool get_textures() const { return textures; }
    inline bool get_wireframe() const { return wireframe; }
    inline int get_xsize() const { return xsize; }
    inline int get_ysize() const { return ysize; }
    inline int get_tile_radius() const { return tile_radius; }
    inline int get_tile_diameter() const { return tile_diameter; }

    inline int get_units() const { return units; }
    inline int get_tris_or_culled() const { return tris_or_culled; }

    inline int get_time_offset() const { return time_offset; }

    inline str_container get_port_options_list() const { 
	return port_options_list;
    }

    // Update functions
    inline void set_hud_status( bool status ) { hud_status = status; }
    inline void set_fov( double amount ) { fov = amount; }
    inline void set_textures( bool status ) { textures = status; }
    inline void cycle_fog( void ) { 
	if ( fog == FG_FOG_DISABLED ) {
	    fog = FG_FOG_FASTEST;
	} else if ( fog == FG_FOG_FASTEST ) {
	    fog = FG_FOG_NICEST;
	    xglHint ( GL_FOG_HINT, GL_NICEST );
	} else if ( fog == FG_FOG_NICEST ) {
	    fog = FG_FOG_DISABLED;
	    xglHint ( GL_FOG_HINT, GL_FASTEST );
	}
    }
    inline void set_xsize( int x ) { xsize= x; }
    inline void set_ysize( int y ) { xsize= y; }

private:

    double parse_time( const string& time_str );
    double parse_degree( const string& degree_str );
    int parse_time_offset( const string& time_str );
    int parse_tile_radius( const string& arg );
    int parse_fdm( const string& fm );
    double parse_fov( const string& arg );
    bool parse_serial( const string& serial_str );
};


extern fgOPTIONS current_options;


#endif /* _OPTIONS_HXX */


// $Log$
// Revision 1.1  1999/04/05 21:32:46  curt
// Initial revision
//
// Revision 1.30  1999/03/11 23:09:51  curt
// When "Help" is selected from the menu check to see if netscape is running.
// If so, command it to go to the flight gear user guide url.  Otherwise
// start a new version of netscape with this url.
//
// Revision 1.29  1999/03/02 01:03:19  curt
// Tweaks for building with native SGI compilers.
//
// Revision 1.28  1999/02/26 22:09:52  curt
// Added initial support for native SGI compilers.
//
// Revision 1.27  1999/02/05 21:29:13  curt
// Modifications to incorporate Jon S. Berndts flight model code.
//
// Revision 1.26  1999/02/02 20:13:37  curt
// MSVC++ portability changes by Bernie Bright:
//
// Lib/Serial/serial.[ch]xx: Initial Windows support - incomplete.
// Simulator/Astro/stars.cxx: typo? included <stdio> instead of <cstdio>
// Simulator/Cockpit/hud.cxx: Added Standard headers
// Simulator/Cockpit/panel.cxx: Redefinition of default parameter
// Simulator/Flight/flight.cxx: Replaced cout with FG_LOG.  Deleted <stdio.h>
// Simulator/Main/fg_init.cxx:
// Simulator/Main/GLUTmain.cxx:
// Simulator/Main/options.hxx: Shuffled <fg_serial.hxx> dependency
// Simulator/Objects/material.hxx:
// Simulator/Time/timestamp.hxx: VC++ friend kludge
// Simulator/Scenery/tile.[ch]xx: Fixed using std::X declarations
// Simulator/Main/views.hxx: Added a constant
//
// Revision 1.25  1999/01/19 20:57:06  curt
// MacOS portability changes contributed by "Robert Puyol" <puyol@abvent.fr>
//
// Revision 1.24  1998/11/25 01:34:01  curt
// Support for an arbitrary number of serial ports.
//
// Revision 1.23  1998/11/23 21:49:05  curt
// Borland portability tweaks.
//
// Revision 1.22  1998/11/20 01:02:38  curt
// Try to detect Mesa/Glide/Voodoo and chose the appropriate resolution.
//
// Revision 1.21  1998/11/16 14:00:04  curt
// Added pow() macro bug work around.
// Added support for starting FGFS at various resolutions.
// Added some initial serial port support.
// Specify default log levels in main().
//
// Revision 1.20  1998/11/02 23:04:05  curt
// HUD units now display in feet by default with meters being a command line
// option.
//
// Revision 1.19  1998/10/25 14:08:49  curt
// Turned "struct fgCONTROLS" into a class, with inlined accessor functions.
//
// Revision 1.18  1998/09/17 18:35:31  curt
// Added F8 to toggle fog and F9 to toggle texturing.
//
// Revision 1.17  1998/09/08 21:40:10  curt
// Fixes by Charlie Hotchkiss.
//
// Revision 1.16  1998/08/27 17:02:08  curt
// Contributions from Bernie Bright <bbright@c031.aone.net.au>
// - use strings for fg_root and airport_id and added methods to return
//   them as strings,
// - inlined all access methods,
// - made the parsing functions private methods,
// - deleted some unused functions.
// - propogated some of these changes out a bit further.
//
// Revision 1.15  1998/08/24 20:11:15  curt
// Added i/I to toggle full vs. minimal HUD.
// Added a --hud-tris vs --hud-culled option.
// Moved options accessor funtions to options.hxx.
//
// Revision 1.14  1998/08/20 15:10:35  curt
// Added GameGLUT support.
//
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
