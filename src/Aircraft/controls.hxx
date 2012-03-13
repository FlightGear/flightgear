// controls.hxx -- defines a standard interface to all flight sim controls
//
// Written by Curtis Olson, started May 1997.
//
// Copyright (C) 1997  Curtis L. Olson  - http://www.flightgear.org/~curt
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


#ifndef _CONTROLS_HXX
#define _CONTROLS_HXX

#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/props/tiedpropertylist.hxx>

// Define a structure containing the control parameters

class FGControls : public SGSubsystem
{

public:

    enum {
        ALL_ENGINES = -1,
        MAX_ENGINES = 12
    };

    enum {
        ALL_WHEELS = -1,
        MAX_WHEELS = 3
    };

    enum {
        ALL_TANKS = -1,
        MAX_TANKS = 8
    };

    enum {
        ALL_BOOSTPUMPS = -1,
        MAX_BOOSTPUMPS = 2
    };

    enum {
        ALL_HYD_SYSTEMS = -1,
        MAX_HYD_SYSTEMS = 4
    };

    enum {
        ALL_PACKS = -1,
        MAX_PACKS = 4
    };

    enum {
        ALL_LIGHTS = -1,
        MAX_LIGHTS = 4
    };

    enum {
        ALL_STATIONS = -1,
        MAX_STATIONS = 12
    };

    enum {
        ALL_AUTOPILOTS = -1,
        MAX_AUTOPILOTS = 3
    };

    enum {
        ALL_EJECTION_SEATS = -1,
        MAX_EJECTION_SEATS = 10
    };
   
    enum {
        SEAT_SAFED = -1,
        SEAT_ARMED = 0,
        SEAT_FAIL = 1
    };
   
    enum { 
        CMD_SEL_NORM = -1,
        CMD_SEL_AFT = 0,
        CMD_SEL_SOLO = 1
    };
    
private:
    // controls/flight/
    double aileron;
    double aileron_trim;
    double elevator;
    double elevator_trim;
    double rudder;
    double rudder_trim;
    double flaps;
    double slats;
    bool BLC;          // Boundary Layer Control
    double spoilers;
    double speedbrake;
    double wing_sweep;
    bool wing_fold;
    bool drag_chute;

    // controls/engines/
    bool throttle_idle;

    // controls/engines/engine[n]/
    double throttle[MAX_ENGINES];
    bool starter[MAX_ENGINES];
    bool fuel_pump[MAX_ENGINES];
    bool fire_switch[MAX_ENGINES];
    bool fire_bottle_discharge[MAX_ENGINES];
    bool cutoff[MAX_ENGINES];
    double mixture[MAX_ENGINES];
    double prop_advance[MAX_ENGINES];
    int magnetos[MAX_ENGINES];
    int feed_tank[MAX_ENGINES];
    bool nitrous_injection[MAX_ENGINES];  // War Emergency Power
    double cowl_flaps_norm[MAX_ENGINES];
    bool feather[MAX_ENGINES];
    int ignition[MAX_ENGINES];
    bool augmentation[MAX_ENGINES];
    bool reverser[MAX_ENGINES];
    bool water_injection[MAX_ENGINES];
    double condition[MAX_ENGINES];           // turboprop speed select

    // controls/fuel/
    bool dump_valve;

    // controls/fuel/tank[n]/
    bool fuel_selector[MAX_TANKS];
    int to_engine[MAX_TANKS];
    int to_tank[MAX_TANKS];
    
    // controls/fuel/tank[n]/pump[p]/
    bool boost_pump[MAX_TANKS * MAX_BOOSTPUMPS];

    // controls/gear/
    double brake_left;
    double brake_right;
    double copilot_brake_left;
    double copilot_brake_right;
    double brake_parking;
    double steering;
    bool nose_wheel_steering;
    bool gear_down;
    bool antiskid;
    bool tailhook;
    bool launchbar;
    bool catapult_launch_cmd;
    bool tailwheel_lock;

    // controls/gear/wheel[n]/
    bool alternate_extension[MAX_WHEELS];

    // controls/anti-ice/
    bool wing_heat;
    bool pitot_heat;
    int wiper;
    bool window_heat;

    // controls/anti-ice/engine[n]/
    bool carb_heat[MAX_ENGINES];
    bool inlet_heat[MAX_ENGINES];
    
    // controls/hydraulic/system[n]/
    bool engine_pump[MAX_HYD_SYSTEMS];
    bool electric_pump[MAX_HYD_SYSTEMS];

    // controls/electric/
    bool battery_switch;
    bool external_power;
    bool APU_generator;

    // controls/electric/engine[n]/
    bool generator_breaker[MAX_ENGINES];
    bool bus_tie[MAX_ENGINES];

    // controls/pneumatic/
    bool APU_bleed;

    // controls/pneumatic/engine[n]/
    bool engine_bleed[MAX_ENGINES];
    
    // controls/pressurization/
    int mode;
    bool dump;
    double outflow_valve;

    // controls/pressurization/pack[n]/
    bool pack_on[MAX_PACKS];

    // controls/lighting/
    bool landing_lights;
    bool turn_off_lights;
    bool taxi_light;
    bool logo_lights;
    bool nav_lights;
    bool beacon;
    bool strobe;
    double panel_norm;
    double instruments_norm;
    double dome_norm;

    // controls/armament/
    bool master_arm;
    int station_select;
    bool release_ALL;

    // controls/armament/station[n]/
    int stick_size[MAX_STATIONS];
    bool release_stick[MAX_STATIONS];
    bool release_all[MAX_STATIONS];
    bool jettison_all[MAX_STATIONS];

    // controls/seat/
    double vertical_adjust;
    double fore_aft_adjust;
    bool eject[MAX_EJECTION_SEATS];
    int eseat_status[MAX_EJECTION_SEATS];
    int cmd_selector_valve;

    // controls/APU/
    int off_start_run;
    bool APU_fire_switch;

    // controls/autoflight/autopilot[n]/
    bool autopilot_engage[MAX_AUTOPILOTS];

    // controls/autoflight/
    bool autothrottle_arm;
    bool autothrottle_engage;
    double heading_select;
    double altitude_select;
    double bank_angle_select;
    double vertical_speed_select;
    double speed_select;
    double mach_select; 
    int vertical_mode;
    int lateral_mode;
     

    SGPropertyNode_ptr auto_coordination;
    SGPropertyNode_ptr auto_coordination_factor;
    simgear::TiedPropertyList _tiedProperties;
public:

    FGControls();
    ~FGControls();

    // Implementation of SGSubsystem.
    void init ();
    void bind ();
    void unbind ();
    void update (double dt);

    // Reset function
    void reset_all(void);
        
    // Query functions
    // controls/flight/
    inline double get_aileron() const { return aileron; }
    inline double get_aileron_trim() const { return aileron_trim; }
    inline double get_elevator() const { return elevator; }
    inline double get_elevator_trim() const { return elevator_trim; }
    inline double get_rudder() const { return rudder; }
    inline double get_rudder_trim() const { return rudder_trim; }
    inline double get_flaps() const { return flaps; }
    inline double get_slats() const { return slats; }
    inline bool get_BLC() const { return BLC; }  
    inline double get_spoilers() const { return spoilers; }
    inline double get_speedbrake() const { return speedbrake; }
    inline double get_wing_sweep() const { return wing_sweep; }
    inline bool get_wing_fold() const { return wing_fold; }
    inline bool get_drag_chute() const { return drag_chute; }

    // controls/engines/
    inline bool get_throttle_idle() const { return throttle_idle; }

    // controls/engines/engine[n]/
    inline double get_throttle(int engine) const { return throttle[engine]; }
    inline bool get_starter(int engine) const { return starter[engine]; }
    inline bool get_fuel_pump(int engine) const { return fuel_pump[engine]; }
    inline bool get_fire_switch(int engine) const { return fire_switch[engine]; }
    inline bool get_fire_bottle_discharge(int engine) const {
        return fire_bottle_discharge[engine];
    }
    inline bool get_cutoff(int engine) const { return cutoff[engine]; }
    inline double get_mixture(int engine) const { return mixture[engine]; }
    inline double get_prop_advance(int engine) const {
        return prop_advance[engine];
    }
    inline int get_magnetos(int engine) const { return magnetos[engine]; }
    inline int get_feed_tank(int engine) const { return feed_tank[engine]; }
    inline bool get_nitrous_injection(int engine) const { 
        return nitrous_injection[engine];
    }
    inline double get_cowl_flaps_norm(int engine) const {
        return cowl_flaps_norm[engine];
    }
    inline bool get_feather(int engine) const { return feather[engine]; }
    inline int get_ignition(int engine) const { return ignition[engine]; }
    inline bool get_augmentation(int engine) const { return augmentation[engine]; }
    inline bool get_reverser(int engine) const { return reverser[engine]; }
    inline bool get_water_injection(int engine) const { 
        return water_injection[engine]; 
    }
    inline double get_condition(int engine) const { return condition[engine]; }

    // controls/fuel/
    inline bool get_dump_valve() const { return dump_valve; }

    // controls/fuel/tank[n]/
    inline bool get_fuel_selector(int tank) const {
        return fuel_selector[tank];
    }
    inline int get_to_engine(int tank) const { return to_engine[tank]; }
    inline int get_to_tank(int tank) const { return to_tank[tank]; }

    // controls/fuel/tank[n]/pump[p]/
    inline bool get_boost_pump(int index) const {
        return boost_pump[index];
    }

    // controls/gear/
    inline double get_brake_left() const { return brake_left; }
    inline double get_brake_right() const { return brake_right; }
    inline double get_copilot_brake_left() const { return copilot_brake_left; }
    inline double get_copilot_brake_right() const { return copilot_brake_right; }
    inline double get_brake_parking() const { return brake_parking; }
    inline double get_steering() const { return steering; }
    inline bool get_nose_wheel_steering() const { return nose_wheel_steering; }
    inline bool get_gear_down() const { return gear_down; }
    inline bool get_antiskid() const { return antiskid; }
    inline bool get_tailhook() const { return tailhook; }
    inline bool get_launchbar() const { return launchbar; }
    inline bool get_catapult_launch_cmd() const { return catapult_launch_cmd; }
    inline bool get_tailwheel_lock() const { return tailwheel_lock; }

    // controls/gear/wheel[n]/
    inline bool get_alternate_extension(int wheel) const {
        return alternate_extension[wheel];
    }

    // controls/anti-ice/
    inline bool get_wing_heat() const { return wing_heat; }
    inline bool get_pitot_heat() const { return pitot_heat; }
    inline int get_wiper() const { return wiper; }
    inline bool get_window_heat() const { return window_heat; }

    // controls/anti-ice/engine[n]/
    inline bool get_carb_heat(int engine) const { return carb_heat[engine]; }
    inline bool get_inlet_heat(int engine) const { return inlet_heat[engine]; }

    // controls/hydraulic/system[n]/
    inline bool get_engine_pump(int system) const { return engine_pump[system]; }
    inline bool get_electric_pump(int system) const { return electric_pump[system]; }

    // controls/electric/
    inline bool get_battery_switch() const { return battery_switch; }
    inline bool get_external_power() const { return external_power; }
    inline bool get_APU_generator() const { return APU_generator; }

    // controls/electric/engine[n]/
    inline bool get_generator_breaker(int engine) const { 
        return generator_breaker[engine];
    }
    inline bool get_bus_tie(int engine) const { return bus_tie[engine]; }

    // controls/pneumatic/
    inline bool get_APU_bleed() const { return APU_bleed; }

    // controls/pneumatic/engine[n]/
    inline bool get_engine_bleed(int engine) const { return engine_bleed[engine]; }
    
    // controls/pressurization/
    inline int get_mode() const { return mode; }
    inline double get_outflow_valve() const { return outflow_valve; }
    inline bool get_dump() const { return dump; }

    // controls/pressurization/pack[n]/
    inline bool get_pack_on(int pack) const { return pack_on[pack]; }

    // controls/lighting/
    inline bool get_landing_lights() const { return landing_lights; }
    inline bool get_turn_off_lights() const { return  turn_off_lights; }
    inline bool get_taxi_light() const { return taxi_light; }
    inline bool get_logo_lights() const { return logo_lights; }
    inline bool get_nav_lights() const { return nav_lights; }
    inline bool get_beacon() const { return beacon; }
    inline bool get_strobe() const { return strobe; }
    inline double get_panel_norm() const { return panel_norm; }
    inline double get_instruments_norm() const { return instruments_norm; }
    inline double get_dome_norm() const { return dome_norm; }

    // controls/armament/
    inline bool get_master_arm() const { return master_arm; }
    inline int get_station_select() const { return station_select; }
    inline bool get_release_ALL() const { return release_ALL; }

      // controls/armament/station[n]/
    inline int get_stick_size(int station) const { return stick_size[station]; }
    inline bool get_release_stick(int station) const { return release_stick[station]; }
    inline bool get_release_all(int station) const { return release_all[station]; }
    inline bool get_jettison_all(int station) const { return jettison_all[station]; }

    // controls/seat/
    inline double get_vertical_adjust() const { return vertical_adjust; }
    inline double get_fore_aft_adjust() const { return fore_aft_adjust; }
    inline bool get_ejection_seat( int which_seat ) const {
        return eject[which_seat];
    }
    inline int get_eseat_status( int which_seat ) const {
        return eseat_status[which_seat];
    }
    inline int get_cmd_selector_valve() const { return cmd_selector_valve; }
    

    // controls/APU/
    inline int get_off_start_run() const { return off_start_run; }
    inline bool get_APU_fire_switch() const { return APU_fire_switch; }

    // controls/autoflight/
    inline bool get_autothrottle_arm() const { return autothrottle_arm; }
    inline bool get_autothrottle_engage() const { return autothrottle_engage; }
    inline double get_heading_select() const { return heading_select; }
    inline double get_altitude_select() const { return altitude_select; }
    inline double get_bank_angle_select() const { return bank_angle_select; }
    inline double get_vertical_speed_select() const { 
        return vertical_speed_select; 
    }
    inline double get_speed_select() const { return speed_select; }
    inline double get_mach_select() const { return mach_select; } 
    inline int get_vertical_mode() const { return vertical_mode; }
    inline int get_lateral_mode() const { return lateral_mode; }

    // controls/autoflight/autopilot[n]/
    inline bool get_autopilot_engage(int ap) const { 
        return autopilot_engage[ap];
    }
     

    // Update functions
    // controls/flight/
    void set_aileron( double pos );
    void move_aileron( double amt );
    void set_aileron_trim( double pos );
    void move_aileron_trim( double amt );
    void set_elevator( double pos );
    void move_elevator( double amt );
    void set_elevator_trim( double pos );
    void move_elevator_trim( double amt );
    void set_rudder( double pos );
    void move_rudder( double amt );
    void set_rudder_trim( double pos );
    void move_rudder_trim( double amt );
    void set_flaps( double pos );
    void move_flaps( double amt );
    void set_slats( double pos );
    void move_slats( double amt );
    void set_BLC( bool val ); 
    void set_spoilers( double pos );
    void move_spoilers( double amt );
    void set_speedbrake( double pos );
    void move_speedbrake( double amt );
    void set_wing_sweep( double pos );
    void move_wing_sweep( double amt );
    void set_wing_fold( bool val );   
    void set_drag_chute( bool val );

    // controls/engines/
    void set_throttle_idle( bool val );

    // controls/engines/engine[n]/
    void set_throttle( int engine, double pos );
    void move_throttle( int engine, double amt );
    void set_starter( int engine, bool flag );
    void set_fuel_pump( int engine, bool val );
    void set_fire_switch( int engine, bool val );
    void set_fire_bottle_discharge( int engine, bool val );
    void set_cutoff( int engine, bool val );
    void set_mixture( int engine, double pos );
    void move_mixture( int engine, double amt );
    void set_prop_advance( int engine, double pos );
    void move_prop_advance( int engine, double amt );
    void set_magnetos( int engine, int pos );
    void move_magnetos( int engine, int amt );
    void set_feed_tank( int engine, int tank );
    void set_nitrous_injection( int engine, bool val );
    void set_cowl_flaps_norm( int engine, double pos );
    void move_cowl_flaps_norm( int engine, double amt );
    void set_feather( int engine, bool val );
    void set_ignition( int engine, int val );
    void set_augmentation( int engine, bool val );
    void set_reverser( int engine, bool val );
    void set_water_injection( int engine, bool val );
    void set_condition( int engine, double val );    

    // controls/fuel
    void set_dump_valve( bool val );

    // controls/fuel/tank[n]/
    void set_fuel_selector( int tank, bool pos );
    void set_to_engine( int tank, int engine );
    void set_to_tank( int tank, int dest_tank );

    // controls/fuel/tank[n]/pump[p]
    void set_boost_pump( int index, bool val );

    // controls/gear/
    void set_brake_left( double pos );
    void move_brake_left( double amt );
    void set_brake_right( double pos );
    void move_brake_right( double amt );
    void set_copilot_brake_left( double pos );
    void set_copilot_brake_right( double pos );
    void set_brake_parking( double pos );
    void set_steering( double pos );
    void move_steering( double amt );
    void set_nose_wheel_steering( bool nws );
    void set_gear_down( bool gear );
    void set_antiskid( bool val );
    void set_tailhook( bool val );
    void set_launchbar( bool val );
    void set_catapult_launch_cmd( bool val );
    void set_tailwheel_lock( bool val );

    // controls/gear/wheel[n]/
    void set_alternate_extension( int wheel, bool val );

    // controls/anti-ice/
    void set_wing_heat( bool val );
    void set_pitot_heat( bool val );
    void set_wiper( int speed );
    void set_window_heat( bool val );

    // controls/anti-ice/engine[n]/
    void set_carb_heat( int engine, bool val );
    void set_inlet_heat( int engine, bool val );

    // controls/hydraulic/system[n]/
    void set_engine_pump( int system, bool val );
    void set_electric_pump( int system, bool val );

    // controls/electric/
    void set_battery_switch( bool val );
    void set_external_power( bool val );
    void set_APU_generator( bool val );

    // controls/electric/engine[n]/
    void set_generator_breaker( int engine, bool val );
    void set_bus_tie( int engine, bool val );

    // controls/pneumatic/
    void set_APU_bleed( bool val );

    // controls/pneumatic/engine[n]/
    void set_engine_bleed( int engine, bool val );
    
    // controls/pressurization/
    void set_mode( int mode );
    void set_outflow_valve( double pos );
    void move_outflow_valve( double amt );
    void set_dump( bool val );

    // controls/pressurization/pack[n]/
    void set_pack_on( int pack, bool val );

    // controls/lighting/
    void set_landing_lights( bool val );
    void set_turn_off_lights( bool val );
    void set_taxi_light( bool val );
    void set_logo_lights( bool val );
    void set_nav_lights( bool val );
    void set_beacon( bool val );
    void set_strobe( bool val );
    void set_panel_norm( double intensity );
    void move_panel_norm( double amt );
    void set_instruments_norm( double intensity );
    void move_instruments_norm( double amt );
    void set_dome_norm( double intensity );
    void move_dome_norm( double amt );

    // controls/armament/
    void set_master_arm( bool val );
    void set_station_select( int station );
    void set_release_ALL( bool val );

    // controls/armament/station[n]/
    void set_stick_size( int station, int size );
    void set_release_stick( int station, bool val );
    void set_release_all( int station, bool val );
    void set_jettison_all( int station, bool val );

    // controls/seat/
    void set_vertical_adjust( double pos );
    void move_vertical_adjust( double amt );
    void set_fore_aft_adjust( double pos );
    void move_fore_aft_adjust( double amt );
    void set_ejection_seat( int which_seat, bool val );
    void set_eseat_status( int which_seat, int val );
    void set_cmd_selector_valve( int val );

    // controls/APU/
    void set_off_start_run( int pos );
    void set_APU_fire_switch( bool val );

    // controls/autoflight/
    void set_autothrottle_arm( bool val );
    void set_autothrottle_engage( bool val );
    void set_heading_select( double heading );
    void move_heading_select( double amt );
    void set_altitude_select( double altitude );
    void move_altitude_select( double amt );
    void set_bank_angle_select( double angle );
    void move_bank_angle_select( double amt );
    void set_vertical_speed_select( double vs );
    void move_vertical_speed_select( double amt );
    void set_speed_select( double speed );
    void move_speed_select( double amt );
    void set_mach_select( double mach ); 
    void move_mach_select( double amt ); 
    void set_vertical_mode( int mode );
    void set_lateral_mode( int mode );

    // controls/autoflight/autopilot[n]/
    void set_autopilot_engage( int ap, bool val );

private:
    inline void do_autocoordination() {
      // check for autocoordination
      if ( auto_coordination->getBoolValue() ) {
        double factor = auto_coordination_factor->getDoubleValue();
        if( factor > 0.0 ) set_rudder( aileron * factor );
      }
    }    
};


#endif // _CONTROLS_HXX


