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

#include <simgear/compiler.h>

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <GL/glut.h>
#include <simgear/xgl/xgl.h>

#if defined(FX) && defined(XMESA)
extern bool global_fullscreen;
#endif

#include <simgear/math/sg_types.hxx>
#include <simgear/timing/sg_time.hxx>

#include <FDM/flight.hxx>

#include "globals.hxx"

#include STL_STRING
#include <vector>

FG_USING_STD(vector);
FG_USING_STD(string);

#define NEW_DEFAULT_MODEL_HZ 120


class FGOptions {

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

    enum fgTimingOffsetType {
	FG_TIME_SYS_OFFSET   = 0,
	FG_TIME_GMT_OFFSET   = 1,
	FG_TIME_LAT_OFFSET   = 2,
	FG_TIME_SYS_ABSOLUTE = 3,
	FG_TIME_GMT_ABSOLUTE = 4,
	FG_TIME_LAT_ABSOLUTE = 5
    };
    
    enum fgSpeedSet {
	FG_VC    = 1,
	FG_MACH  = 2,
	FG_VTUVW  = 3,
	FG_VTNED = 4
    };

private:

    // The flight gear "root" directory
//     string fg_root;

    // The scenery "root" directory
//     string fg_scenery;

    // Starting position and orientation

    // These are now all SGValue pointers, but they won't stay
    // that way forever -- it's just to ease the transition to the
    // property manager.  Gradually, references to the methods that
    // use these variables will be culled out, and the variables
    // and methods will be removed.

    SGValue * airport_id;	// ID of initial starting airport
    SGValue * lon;		// starting longitude in degrees (west = -)
    SGValue * lat;		// starting latitude in degrees (south = -)
    SGValue * altitude;		// starting altitude in meters
    SGValue * heading;		// heading (yaw) angle in degress (Psi)
    SGValue * roll;		// roll angle in degrees (Phi)
    SGValue * pitch;		// pitch angle in degrees (Theta)
    SGValue * speedset;		// which speed does the user want
    SGValue * uBody;		// Body axis X velocity (U)
    SGValue * vBody;		// Body axis Y velocity (V)
    SGValue * wBody;		// Body axis Z velocity (W)
    SGValue * vNorth;		// North component of vt
    SGValue * vEast;		// East component of vt
    SGValue * vDown;		// Down component of vt
    SGValue * vkcas;		// Calibrated airspeed, knots
    SGValue * mach;		// Mach number

    // Miscellaneous
    SGValue * game_mode;	// Game mode enabled/disabled
    SGValue * splash_screen;	// show splash screen
    SGValue * intro_music;	// play introductory music
    SGValue * mouse_pointer;	// show mouse pointer
    SGValue * control_mode;	// primary control mode
    SGValue * auto_coordination; // enable auto coordination

    // Features
    SGValue * hud_status;	// HUD on/off
    SGValue * panel_status;	// Panel on/off
    SGValue * sound;		// play sound effects
    SGValue * anti_alias_hud;

    // Flight Model options
    SGValue * flight_model;	// Core flight model code
    SGValue * aircraft;		// Aircraft to model
    SGValue * model_hz;		// number of FDM iterations per second
    SGValue * speed_up;		// Sim mechanics run this much faster than 
				// normal speed
    SGValue * trim;		// use the FDM trimming routine during init
				// <0 --notrim set, 0 no trim, >0 trim
				// default behavior is to enable trimming for 
				// jsbsim and disable for all other fdm's

    // Rendering options
    SGValue * fog;		// Fog nicest/fastest/disabled
    SGValue * clouds;		// Enable clouds
    SGValue * clouds_asl;	// Cloud layer height above sea level
    SGValue * fullscreen;	// Full screen mode enabled/disabled
    SGValue * shading;		// shading method, 0 = Flat, 1 = Smooth
    SGValue * skyblend;		// Blend sky to haze (using polygons) or 
				// just clear
    SGValue * textures;		// Textures enabled/disabled
    SGValue * wireframe;	// Wireframe mode enabled/disabled
    SGValue * xsize;		// window size derived from geometry string
    SGValue * ysize;
    SGValue * bpp;		// bits per pixel
    SGValue * view_mode;	// view mode
    SGValue * default_view_offset; // default forward view offset (for use by
				// multi-display configuration
    SGValue * visibility;	// visibilty in meters

    // HUD options
    SGValue * units;         // feet or meters
    SGValue * tris_or_culled;

    // Time options
    SGValue * time_offset;	// Use this value to change time.
    SGValue * time_offset_type; // Will be set to one of the
				// FG_TIME_* enums, to deterine how
				// time_offset should be used 

    // Serial port configuration strings
//     string_list channel_options_list;

    // Network options
    SGValue * network_olk;
    SGValue * net_id;

public:

    FGOptions();
    ~FGOptions();

    void init ();

    void set_default_props ();

    // Parse a single option
    int parse_option( const string& arg );

    // Scan the command line options for an fg_root definition and set
    // just that.
    int scan_command_line_for_root( int argc, char **argv );

    // Scan the config file for an fg_root definition and set just
    // that.
    int scan_config_file_for_root( const string& path );

    // Parse the command line options
    int parse_command_line( int argc, char **argv );

    // Parse the command line options
    int parse_config_file( const string& path );

    // Print usage message
    void usage ( void );

    // Query functions
    string get_fg_root() const;
    string get_fg_scenery() const;
    inline string get_airport_id() const {
      return airport_id->getStringValue();
    }
    inline double get_lon() const { return lon->getDoubleValue(); }
    inline double get_lat() const { return lat->getDoubleValue(); }
    inline double get_altitude() const { return altitude->getDoubleValue(); }
    inline double get_heading() const { return heading->getDoubleValue(); }
    inline double get_roll() const { return roll->getDoubleValue(); }
    inline double get_pitch() const { return pitch->getDoubleValue(); }
    inline fgSpeedSet get_speedset() const { 
      const string &s = speedset->getStringValue();
      if (s == "UVW" || s == "uvw")
	return FG_VTUVW;
      else if (s == "NED" || s == "ned")
	return FG_VTNED;
      else if (s == "knots" || s == "KNOTS")
	return FG_VC;
      else if (s == "mach" || s == "MACH")
	return FG_MACH;
      else
	return FG_VC;
    }
    inline double get_uBody() const {return uBody->getDoubleValue();}
    inline double get_vBody() const {return vBody->getDoubleValue();}
    inline double get_wBody() const {return wBody->getDoubleValue();}
    inline double get_vNorth() const {return vNorth->getDoubleValue();}
    inline double get_vEast() const {return vEast->getDoubleValue();}
    inline double get_vDown() const {return vDown->getDoubleValue();}
    inline double get_vc() const {return vkcas->getDoubleValue();}
    inline double get_mach() const {return mach->getDoubleValue();}
	
    inline bool get_game_mode() const { return game_mode->getBoolValue(); }
    inline bool get_splash_screen() const {
      return splash_screen->getBoolValue();
    }
    inline bool get_intro_music() const {
      return intro_music->getBoolValue();
    }
    inline int get_mouse_pointer() const {
      return mouse_pointer->getBoolValue();
    }
    inline bool get_anti_alias_hud() const {
      return anti_alias_hud->getBoolValue();
    }
    inline fgControlMode get_control_mode() const {
      const string &s = control_mode->getStringValue();
      if (s == "joystick")
	return FG_JOYSTICK;
      else if (s == "keyboard")
	return FG_KEYBOARD;
      else if (s == "mouse")
	return FG_MOUSE;
      else
	return FG_JOYSTICK;
    }
    inline void set_control_mode( fgControlMode mode ) {
      if(mode == FG_JOYSTICK)
	control_mode->setStringValue("joystick");
      else if (mode == FG_KEYBOARD)
	control_mode->setStringValue("keyboard");
      else if (mode == FG_MOUSE)
	control_mode->setStringValue("mouse");
      else
	control_mode->setStringValue("joystick");
    }
    inline fgAutoCoordMode get_auto_coordination() const { 
        if (auto_coordination->getBoolValue())
	  return FG_AUTO_COORD_ENABLED;
	else
	  return FG_AUTO_COORD_DISABLED;
    }
    inline void set_auto_coordination(fgAutoCoordMode m) { 
        if (m == FG_AUTO_COORD_ENABLED)
	  auto_coordination->setBoolValue(true);
	else if (m == FG_AUTO_COORD_DISABLED)
	  auto_coordination->setBoolValue(false);
    }
    inline bool get_hud_status() const { return hud_status->getBoolValue(); }
    inline bool get_panel_status() const {
      return panel_status->getBoolValue();
    }
    inline bool get_sound() const { return sound->getBoolValue(); }
    inline int get_flight_model() const {
      return parse_fdm(flight_model->getStringValue());
    }
    inline string get_aircraft() const { return aircraft->getStringValue(); }
    inline int get_model_hz() const { return model_hz->getIntValue(); }
    inline int get_speed_up() const { return speed_up->getIntValue(); }
    inline void set_speed_up( int speed ) { speed_up->setIntValue(speed); }
    inline int get_trim_mode(void) { return trim->getIntValue(); }
	
    inline bool fog_enabled() const { 
      return fog->getStringValue() != "disabled";
    }
    inline fgFogKind get_fog() const {
      const string &s = fog->getStringValue();
      if (s == "disabled")
	return FG_FOG_DISABLED;
      else if (s == "fastest")
	return FG_FOG_FASTEST;
      else if (s == "nicest")
	return FG_FOG_NICEST;
      else
	return FG_FOG_DISABLED;
    }
    inline bool get_clouds() const { return clouds->getBoolValue(); }
    inline double get_clouds_asl() const {
      return clouds_asl->getDoubleValue() * FEET_TO_METER;
    }
    inline bool get_fullscreen() const { return fullscreen->getBoolValue(); }
    inline int get_shading() const { return shading->getIntValue(); }
    inline bool get_skyblend() const { return skyblend->getBoolValue(); }
    inline bool get_textures() const { return textures->getBoolValue(); }
    inline bool get_wireframe() const { return wireframe->getBoolValue(); }
    inline int get_xsize() const { return xsize->getIntValue(); }
    inline int get_ysize() const { return ysize->getIntValue(); }
    inline int get_bpp() const { return bpp->getIntValue(); }
    inline fgViewMode get_view_mode() const {
      return (fgViewMode)(view_mode->getIntValue()); // FIXME!!
    }
    inline double get_default_view_offset() const {
	return default_view_offset->getDoubleValue();;
    }
    inline double get_default_visibility() const {
      return visibility->getDoubleValue();
    }

    inline int get_units() const { 
      if (units->getStringValue() == "meters")
	return FG_UNITS_METERS;
      else
	return FG_UNITS_FEET;
    }
    inline int get_tris_or_culled() const { 
      if (tris_or_culled->getStringValue() == "tris")
	return 1;		// FIXME: check this!!!
      else
	return 2;
    }

    inline int get_time_offset() const { return time_offset->getIntValue(); }
    inline fgTimingOffsetType  get_time_offset_type() const {
      const string &s = time_offset_type->getStringValue();
      if (s == "system-offset")
	return FG_TIME_SYS_OFFSET;
      else if (s == "gmt-offset")
	return FG_TIME_GMT_OFFSET;
      else if (s == "latitude-offset")
	return FG_TIME_LAT_OFFSET;
      else if (s == "system")
	return FG_TIME_SYS_ABSOLUTE;
      else if (s == "gmt")
        return FG_TIME_GMT_ABSOLUTE;
      else if (s == "latitude")
	return FG_TIME_LAT_ABSOLUTE;
      else
	return FG_TIME_SYS_OFFSET;
    };

    string_list get_channel_options_list () const;

    inline bool get_network_olk() const { return network_olk->getBoolValue(); }
    inline string get_net_id() const { return net_id->getStringValue(); }

    // Update functions
    inline void set_airport_id( const string id ) {
      airport_id->setStringValue(id);
    }
    inline void set_lon (double value) { lon->setDoubleValue(value); }
    inline void set_lat (double value) { lat->setDoubleValue(value); }
    inline void set_altitude (double value) {
      altitude->setDoubleValue(value);
    }
    inline void set_heading (double value) { heading->setDoubleValue(value); }
    inline void set_roll (double value) { roll->setDoubleValue(value); }
    inline void set_pitch (double value) { pitch->setDoubleValue(value); }
    inline void set_anti_alias_hud (bool value) {
      anti_alias_hud->setBoolValue(value);
    }
    inline void set_hud_status( bool status ) {
      hud_status->setBoolValue(status);
    }
    inline void set_flight_model (int value) {
      if (value == FGInterface::FG_ADA)
	flight_model->setStringValue("ada");
      else if (value == FGInterface::FG_BALLOONSIM)
	flight_model->setStringValue("balloon");
      else if (value == FGInterface::FG_EXTERNAL)
	flight_model->setStringValue("external");
      else if (value == FGInterface::FG_JSBSIM)
	flight_model->setStringValue("jsb");
      else if (value == FGInterface::FG_LARCSIM)
	flight_model->setStringValue("larcsim");
      else if (value == FGInterface::FG_MAGICCARPET)
	flight_model->setStringValue("magic");
      else
	flight_model->setStringValue("larcsim");
    }
    inline void set_aircraft (const string &ac) {
      aircraft->setStringValue(ac);
    }
    inline void set_textures( bool status ) { textures->setBoolValue(status); }
    inline void cycle_fog( void ) { 
        const string &s = fog->getStringValue();
	if ( s == "disabled" ) {
	    fog->setStringValue("fastest");
	} else if ( s == "fastest" ) {
	    fog->setStringValue("nicest");
	    glHint ( GL_FOG_HINT, GL_NICEST );
	} else if ( s == "nicest" ) {
	    fog->setStringValue("disabled");
	    glHint ( GL_FOG_HINT, GL_FASTEST );
	}
    }
    void toggle_panel();
    inline void set_xsize( int x ) { xsize->setIntValue(x); }
    inline void set_ysize( int y ) { ysize->setIntValue(y); }

    inline void set_net_id( const string id ) { net_id->setStringValue(id); }

private:

    void parse_control( const string& mode );
    double parse_time( const string& time_str );
    long int parse_date( const string& date_str );
    double parse_degree( const string& degree_str );
    int parse_time_offset( const string& time_str );
    int parse_fdm( const string& fm ) const;
    double parse_fov( const string& arg );
    bool parse_channel( const string& type, const string& channel_str );
    bool parse_wp( const string& arg );
    bool parse_flightplan(const string& arg);
};


#endif /* _OPTIONS_HXX */


