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

#include <Include/fg_types.hxx>

#include STL_STRING
#include <vector>

FG_USING_STD(vector);
FG_USING_STD(string);

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

    enum
    {
      FG_TIME_SYS_OFFSET   = 1,
      FG_TIME_GMT_OFFSET   = 2,
      FG_TIME_LAT_OFFSET   = 3,
      FG_TIME_SYS_ABSOLUTE = 4,
      FG_TIME_GMT_ABSOLUTE = 5,
      FG_TIME_LAT_ABSOLUTE = 6
    };

    enum fgControlMode
    {
	FG_JOYSTICK = 0,
	FG_KEYBOARD = 1,
	FG_MOUSE = 2
    };

    enum fgViewMode
    {
	FG_VIEW_PILOT = 0,	// Pilot perspective
	FG_VIEW_FOLLOW  = 1,	// Following in the "foot steps" so to speak
	FG_VIEW_CHASE = 2,	// Chase
	FG_VIEW_CIRCLING = 3,	// Circling
	FG_VIEW_SATELLITE = 4,	// From high above
	FG_VIEW_ANCHOR = 5,	// Drop an anchor and watch from there
	FG_VIEW_TOWER = 6,	// From nearest tower?
	FG_VIEW_SPOTTER = 7	// Fron a ground spotter
    };

    enum fgAutoCoordMode
    {
	FG_AUTO_COORD_NOT_SPECIFIED = 0,
	FG_AUTO_COORD_DISABLED = 1,
	FG_AUTO_COORD_ENABLED = 2
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
    double uBody;       // Body axis X velocity (U)
    double vBody;       // Body axis Y velocity (V)
    double wBody;       // Body axis Z velocity (W)

    // Miscellaneous
    bool game_mode;     // Game mode enabled/disabled
    bool splash_screen; // show splash screen
    bool intro_music;   // play introductory music
    int mouse_pointer;  // show mouse pointer
    bool pause;         // pause intially enabled/disabled
    fgControlMode control_mode; // primary control mode
    fgAutoCoordMode auto_coordination;	// enable auto coordination

    // Features
    bool hud_status;    // HUD on/off
    bool panel_status;  // Panel on/off
    bool sound;         // play sound effects

    // Flight Model options
    int flight_model;   // Flight Model:  FG_SLEW, FG_LARCSIM, etc.
    int model_hz;       // number of FDM iterations per second
    int speed_up;       // Sim mechanics run this much faster than normal speed

    // Rendering options
    fgFogKind fog;      // Fog nicest/fastest/disabled
    bool clouds;        // Enable clouds
    double clouds_asl;  // Cloud layer height above sea level
    double fov;         // Field of View
    bool fullscreen;    // Full screen mode enabled/disabled
    int shading;        // shading method, 0 = Flat, 1 = Smooth
    bool skyblend;      // Blend sky to haze (using polygons) or just clear
    bool textures;      // Textures enabled/disabled
    bool wireframe;     // Wireframe mode enabled/disabled
    int xsize, ysize;   // window size derived from geometry string
    fgViewMode view_mode; // view mode

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
    int time_offset;		// Use this value to change time.
    int time_offset_type;	// Will be set to one of the FG_TIME_* enums,
				// To deterine how time_offset should be used 

    // Serial port configuration strings
    string_list channel_options_list;

    // Network options
    string net_id;
    
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
    inline double get_uBody() const {return uBody;}
    inline double get_vBody() const {return vBody;}
    inline double get_wBody() const {return wBody;}
    inline bool get_game_mode() const { return game_mode; }
    inline bool get_splash_screen() const { return splash_screen; }
    inline bool get_intro_music() const { return intro_music; }
    inline int get_mouse_pointer() const { return mouse_pointer; }
    inline bool get_pause() const { return pause; }
    inline fgControlMode get_control_mode() const { return control_mode; }
    inline void set_control_mode( fgControlMode mode ) { control_mode = mode; }
    inline fgAutoCoordMode get_auto_coordination() const { 
	return auto_coordination;
    }
    inline void set_auto_coordination(fgAutoCoordMode m) { 
	auto_coordination = m;
    }
    inline bool get_hud_status() const { return hud_status; }
    inline bool get_panel_status() const { return panel_status; }
    inline bool get_sound() const { return sound; }
    inline int get_flight_model() const { return flight_model; }
    inline int get_model_hz() const { return model_hz; }
    inline int get_speed_up() const { return speed_up; }
    inline void set_speed_up( int speed ) { speed_up = speed; }
    inline bool fog_enabled() const { return fog != FG_FOG_DISABLED; }
    inline fgFogKind get_fog() const { return fog; }
    inline bool get_clouds() const { return clouds; }
    inline double get_clouds_asl() const { return clouds_asl; }
    inline double get_fov() const { return fov; }
    inline bool get_fullscreen() const { return fullscreen; }
    inline int get_shading() const { return shading; }
    inline bool get_skyblend() const { return skyblend; }
    inline bool get_textures() const { return textures; }
    inline bool get_wireframe() const { return wireframe; }
    inline int get_xsize() const { return xsize; }
    inline int get_ysize() const { return ysize; }
    inline fgViewMode get_view_mode() const { return view_mode; }
    inline int get_tile_radius() const { return tile_radius; }
    inline int get_tile_diameter() const { return tile_diameter; }

    inline int get_units() const { return units; }
    inline int get_tris_or_culled() const { return tris_or_culled; }

    inline int get_time_offset() const { return time_offset; }
    inline int get_time_offset_type() const { return time_offset_type; };

    inline string_list get_channel_options_list() const { 
	return channel_options_list;
    }
    inline string get_net_id() const { return net_id; }

    // Update functions
    inline void set_airport_id( const string id ) { airport_id = id; }
    inline void set_hud_status( bool status ) { hud_status = status; }
    inline void set_clouds( bool value ) { clouds = value; }
    inline void set_clouds_asl( double value ) { clouds_asl = value; }
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
    void toggle_panel();
    inline void set_xsize( int x ) { xsize = x; }
    inline void set_ysize( int y ) { ysize = y; }
    inline void cycle_view_mode() { 
	if ( view_mode == FG_VIEW_PILOT ) {
	    view_mode = FG_VIEW_FOLLOW;
	} else if ( view_mode == FG_VIEW_FOLLOW ) {
	    view_mode = FG_VIEW_PILOT;
	}
    }

    inline void set_net_id( const string id ) { net_id = id; }

private:

    void parse_control( const string& mode );
    double parse_time( const string& time_str );
    long int parse_date( const string& date_str );
    double parse_degree( const string& degree_str );
    int parse_time_offset( const string& time_str );
    int parse_tile_radius( const string& arg );
    int parse_fdm( const string& fm );
    double parse_fov( const string& arg );
    bool parse_channel( const string& type, const string& channel_str );
};


extern fgOPTIONS current_options;


#endif /* _OPTIONS_HXX */


