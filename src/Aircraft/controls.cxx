// controls.cxx -- defines a standard interface to all flight sim controls
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <Main/fg_props.hxx>
#include <simgear/sg_inlines.h>
#include "controls.hxx"

////////////////////////////////////////////////////////////////////////
// Implementation of FGControls.
////////////////////////////////////////////////////////////////////////

// Constructor
FGControls::FGControls() :
    aileron( 0.0 ),
    aileron_trim( 0.0 ),
    elevator( 0.0 ),
    elevator_trim( 0.0 ),
    rudder( 0.0 ),
    rudder_trim( 0.0 ),
    flaps( 0.0 ),
    slats( 0.0 ),
    BLC( false ),
    spoilers( 0.0 ),
    speedbrake( 0.0 ),
    wing_sweep( 0.0 ),
    wing_fold( false ),
    drag_chute( false ),
    throttle_idle( true ),
    dump_valve( false ),
    brake_left( 0.0 ),
    brake_right( 0.0 ),
    copilot_brake_left( 0.0 ),
    copilot_brake_right( 0.0 ),
    brake_parking( 0.0 ),
    steering( 0.0 ),
    nose_wheel_steering( true ),
    gear_down( true ),
    antiskid( true ),
    tailhook( false ),
    launchbar( false ),
    catapult_launch_cmd( false ),
    tailwheel_lock( true ),
    wing_heat( false ),
    pitot_heat( true ),
    wiper( 0 ),
    window_heat( false ),
    battery_switch( true ),
    external_power( false ),
    APU_generator( false ),
    APU_bleed( false ),
    mode( 0 ),
    dump( false ),
    outflow_valve( 0.0 ),
    taxi_light( false ),
    logo_lights( false ),
    nav_lights( false ),
    beacon( false ),
    strobe( false ),
    panel_norm( 0.0 ),
    instruments_norm( 0.0 ),
    dome_norm( 0.0 ),
    master_arm( false ),
    station_select( 1 ),
    release_ALL( false ),
    vertical_adjust( 0.0 ),
    fore_aft_adjust( 0.0 ),
    off_start_run( 0 ),
    APU_fire_switch( false ),
    autothrottle_arm( false ),
    autothrottle_engage( false ),
    heading_select( 0.0 ),
    altitude_select( 50000.0 ),
    bank_angle_select( 30.0 ),
    vertical_speed_select( 0.0 ),
    speed_select( 0.0 ),
    mach_select( 0.0 ),
    vertical_mode( 0 ),
    lateral_mode( 0 )
{
}


void FGControls::reset_all()
{
    set_aileron( 0.0 );
    set_aileron_trim( 0.0 );
    set_elevator( 0.0 );
    set_elevator_trim( 0.0 );
    set_rudder( 0.0 );
    set_rudder_trim( 0.0 );
    BLC = false;
    set_spoilers( 0.0 );
    set_speedbrake( 0.0 );
    set_wing_sweep( 0.0 );
    wing_fold = false;
    drag_chute = false;
    set_throttle( ALL_ENGINES, 0.0 );
    set_starter( ALL_ENGINES, false );
    set_magnetos( ALL_ENGINES, 0 );
    set_fuel_pump( ALL_ENGINES, false );
    set_fire_switch( ALL_ENGINES, false );
    set_fire_bottle_discharge( ALL_ENGINES, false );
    set_cutoff( ALL_ENGINES, true );
    set_nitrous_injection( ALL_ENGINES, false );
    set_cowl_flaps_norm( ALL_ENGINES, 1.0 );
    set_feather( ALL_ENGINES, false );
    set_ignition( ALL_ENGINES, false );
    set_augmentation( ALL_ENGINES, false );
    set_reverser( ALL_ENGINES, false );
    set_water_injection( ALL_ENGINES, false );
    set_condition( ALL_ENGINES, 1.0 );
    throttle_idle = true;
    set_fuel_selector( ALL_TANKS, true );
    dump_valve = false;
    steering =  0.0;
    nose_wheel_steering = true;
    gear_down = true;
    tailhook = false;
    launchbar = false;
    catapult_launch_cmd = false;
    tailwheel_lock = true;
    set_carb_heat( ALL_ENGINES, false );
    set_inlet_heat( ALL_ENGINES, false );
    wing_heat = false;
    pitot_heat = true;
    wiper = 0;
    window_heat = false;
    set_engine_pump( ALL_HYD_SYSTEMS, true );
    set_electric_pump( ALL_HYD_SYSTEMS, true );
    landing_lights = false;
    turn_off_lights = false;
    master_arm = false;
    set_ejection_seat( ALL_EJECTION_SEATS, false );
    set_eseat_status( ALL_EJECTION_SEATS, SEAT_SAFED );
    set_cmd_selector_valve( CMD_SEL_NORM );
    APU_fire_switch = false;
    autothrottle_arm = false;
    autothrottle_engage = false;
    set_autopilot_engage( ALL_AUTOPILOTS, false );
}


// Destructor
FGControls::~FGControls() {
}


void
FGControls::init ()
{
    throttle_idle = true;
    for ( int engine = 0; engine < MAX_ENGINES; engine++ ) {
        throttle[engine] = 0.0;
        mixture[engine] = 1.0;
        fuel_pump[engine] = false;
        prop_advance[engine] = 1.0;
        magnetos[engine] = 0;
        feed_tank[engine] = -1; // set to -1 to turn off all tanks 0 feeds all engines from center body tank
        starter[engine] = false;
        feather[engine] = false;
        ignition[engine] = false;
        fire_switch[engine] = false;
        fire_bottle_discharge[engine] = false;
        cutoff[engine] = true;
        augmentation[engine] = false;
        reverser[engine] = false;
        water_injection[engine] = false;
        nitrous_injection[engine] = false;
        cowl_flaps_norm[engine] = 0.0;
        condition[engine] = 1.0;
        carb_heat[engine] = false;
        inlet_heat[engine] = false;
        generator_breaker[engine] = false;
        bus_tie[engine] = false;
        engine_bleed[engine] = false;
    }

    for ( int tank = 0; tank < MAX_TANKS; tank++ ) {
        fuel_selector[tank] = false;
        to_engine[tank] = 0;
        to_tank[tank] = 0;
    }

    for( int pump = 0; pump < MAX_TANKS * MAX_BOOSTPUMPS; pump++ ) {
        boost_pump[pump] = false;
    }

    brake_left = brake_right
        = copilot_brake_left = copilot_brake_right
        = brake_parking = 0.0;
    for ( int wheel = 0; wheel < MAX_WHEELS; wheel++ ) {
        alternate_extension[wheel] = false;
    }

    auto_coordination = fgGetNode("/controls/flight/auto-coordination", true);
    auto_coordination_factor = fgGetNode("/controls/flight/auto-coordination-factor", false );
    if( NULL == auto_coordination_factor ) {
      auto_coordination_factor = fgGetNode("/controls/flight/auto-coordination-factor", true );
      auto_coordination_factor->setDoubleValue( 0.5 );
    }
}

static inline void _SetRoot( simgear::TiedPropertyList & tiedProperties, const char * root, int index = 0 )
{
    tiedProperties.setRoot( fgGetNode( root, index, true ) );
}

void
FGControls::bind ()
{
    int index, i;

    // flight controls
    _SetRoot( _tiedProperties, "/controls/flight" );

    _tiedProperties.Tie( "aileron", this, &FGControls::get_aileron, &FGControls::set_aileron )
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    _tiedProperties.Tie( "aileron-trim", this, &FGControls::get_aileron_trim, &FGControls::set_aileron_trim )
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    _tiedProperties.Tie( "elevator", this, &FGControls::get_elevator, &FGControls::set_elevator )
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    _tiedProperties.Tie( "elevator-trim", this, &FGControls::get_elevator_trim, &FGControls::set_elevator_trim )
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    _tiedProperties.Tie( "rudder", this, &FGControls::get_rudder, &FGControls::set_rudder )
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    _tiedProperties.Tie( "rudder-trim", this, &FGControls::get_rudder_trim, &FGControls::set_rudder_trim )
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    _tiedProperties.Tie( "flaps", this, &FGControls::get_flaps, &FGControls::set_flaps )
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    _tiedProperties.Tie( "slats", this, &FGControls::get_slats, &FGControls::set_slats )
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    _tiedProperties.Tie( "BLC", this, &FGControls::get_BLC, &FGControls::set_BLC )
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    _tiedProperties.Tie( "spoilers", this, &FGControls::get_spoilers, &FGControls::set_spoilers )
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    _tiedProperties.Tie( "speedbrake", this, &FGControls::get_speedbrake, &FGControls::set_speedbrake )
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    _tiedProperties.Tie( "wing-sweep", this, &FGControls::get_wing_sweep, &FGControls::set_wing_sweep )
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    _tiedProperties.Tie( "wing-fold", this, &FGControls::get_wing_fold, &FGControls::set_wing_fold )
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    _tiedProperties.Tie( "drag-chute", this, &FGControls::get_drag_chute, &FGControls::set_drag_chute )
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    // engines
    _tiedProperties.setRoot( fgGetNode("/controls/engines", true ) );

    _tiedProperties.Tie( "throttle_idle", this, &FGControls::get_throttle_idle, &FGControls::set_throttle_idle )
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    for (index = 0; index < MAX_ENGINES; index++) {
        _SetRoot(_tiedProperties, "/controls/engines/engine", index );

        _tiedProperties.Tie( "throttle", this, index, &FGControls::get_throttle, &FGControls::set_throttle )
            ->setAttribute( SGPropertyNode::ARCHIVE, true );

        _tiedProperties.Tie( "starter", this, index, &FGControls::get_starter, &FGControls::set_starter )
            ->setAttribute( SGPropertyNode::ARCHIVE, true );

        _tiedProperties.Tie( "fuel-pump", this, index, &FGControls::get_fuel_pump, &FGControls::set_fuel_pump )
            ->setAttribute( SGPropertyNode::ARCHIVE, true );

        _tiedProperties.Tie( "fire-switch", this, index, &FGControls::get_fire_switch, &FGControls::set_fire_switch )
            ->setAttribute( SGPropertyNode::ARCHIVE, true );

        _tiedProperties.Tie( "fire-bottle-discharge", this, index, &FGControls::get_fire_bottle_discharge, &FGControls::set_fire_bottle_discharge )
            ->setAttribute( SGPropertyNode::ARCHIVE, true );

        _tiedProperties.Tie( "cutoff", this, index, &FGControls::get_cutoff, &FGControls::set_cutoff )
            ->setAttribute( SGPropertyNode::ARCHIVE, true );

        _tiedProperties.Tie( "mixture", this, index, &FGControls::get_mixture, &FGControls::set_mixture )
            ->setAttribute( SGPropertyNode::ARCHIVE, true );

        _tiedProperties.Tie( "propeller-pitch", this, index, &FGControls::get_prop_advance, &FGControls::set_prop_advance )
            ->setAttribute( SGPropertyNode::ARCHIVE, true );

        _tiedProperties.Tie( "magnetos", this, index, &FGControls::get_magnetos, &FGControls::set_magnetos )
            ->setAttribute( SGPropertyNode::ARCHIVE, true );

        _tiedProperties.Tie( "feed_tank", this, index, &FGControls::get_feed_tank, &FGControls::set_feed_tank )
            ->setAttribute( SGPropertyNode::ARCHIVE, true );

        _tiedProperties.Tie( "WEP", this, index, &FGControls::get_nitrous_injection, &FGControls::set_nitrous_injection )
            ->setAttribute( SGPropertyNode::ARCHIVE, true );

        _tiedProperties.Tie( "cowl-flaps-norm", this, index, &FGControls::get_cowl_flaps_norm, &FGControls::set_cowl_flaps_norm )
            ->setAttribute( SGPropertyNode::ARCHIVE, true );

        _tiedProperties.Tie( "propeller-feather", this, index, &FGControls::get_feather, &FGControls::set_feather )
            ->setAttribute( SGPropertyNode::ARCHIVE, true );

        _tiedProperties.Tie( "ignition", this, index, &FGControls::get_ignition, &FGControls::set_ignition )
            ->setAttribute( SGPropertyNode::ARCHIVE, true );

        _tiedProperties.Tie( "augmentation", this, index, &FGControls::get_augmentation, &FGControls::set_augmentation )
            ->setAttribute( SGPropertyNode::ARCHIVE, true );

        _tiedProperties.Tie( "reverser", this, index, &FGControls::get_reverser, &FGControls::set_reverser )
            ->setAttribute( SGPropertyNode::ARCHIVE, true );

        _tiedProperties.Tie( "water-injection", this, index, &FGControls::get_water_injection, &FGControls::set_water_injection )
            ->setAttribute( SGPropertyNode::ARCHIVE, true );

        _tiedProperties.Tie( "condition", this, index, &FGControls::get_condition, &FGControls::set_condition )
            ->setAttribute( SGPropertyNode::ARCHIVE, true );
    }

    // fuel
    _SetRoot( _tiedProperties, "/controls/fuel" );

    _tiedProperties.Tie( "dump-valve", this, &FGControls::get_dump_valve, &FGControls::set_dump_valve)
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    for (index = 0; index < MAX_TANKS; index++) {
        _SetRoot( _tiedProperties, "/controls/fuel/tank", index );

        _tiedProperties.Tie( "fuel_selector", this, index, &FGControls::get_fuel_selector, &FGControls::set_fuel_selector)
            ->setAttribute( SGPropertyNode::ARCHIVE, true );

        _tiedProperties.Tie( "to_engine", this, index, &FGControls::get_to_engine, &FGControls::set_to_engine)
            ->setAttribute( SGPropertyNode::ARCHIVE, true );

        _tiedProperties.Tie( "to_tank", this, index, &FGControls::get_to_tank, &FGControls::set_to_tank)
            ->setAttribute( SGPropertyNode::ARCHIVE, true );

        for (i = 0; i < MAX_BOOSTPUMPS; i++) {
            _tiedProperties.Tie( "boost-pump", i,
                this, index * 2 + i, &FGControls::get_boost_pump, &FGControls::set_boost_pump)
                ->setAttribute( SGPropertyNode::ARCHIVE, true );
        }
    }

    // gear
    _SetRoot( _tiedProperties, "/controls/gear" );

    _tiedProperties.Tie( "brake-left", this, &FGControls::get_brake_left, &FGControls::set_brake_left)
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    _tiedProperties.Tie( "brake-right", this, &FGControls::get_brake_right, &FGControls::set_brake_right)
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    _tiedProperties.Tie( "copilot-brake-left", this, &FGControls::get_copilot_brake_left, &FGControls::set_copilot_brake_left)
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    _tiedProperties.Tie( "copilot-brake-right", this, &FGControls::get_copilot_brake_right, &FGControls::set_copilot_brake_right)
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    _tiedProperties.Tie( "brake-parking", this, &FGControls::get_brake_parking, &FGControls::set_brake_parking)
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    _tiedProperties.Tie( "steering", this, &FGControls::get_steering, &FGControls::set_steering)
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    _tiedProperties.Tie( "nose-wheel-steering", this, &FGControls::get_nose_wheel_steering, &FGControls::set_nose_wheel_steering)
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    _tiedProperties.Tie( "gear-down", this, &FGControls::get_gear_down, &FGControls::set_gear_down)
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    _tiedProperties.Tie( "antiskid", this, &FGControls::get_antiskid, &FGControls::set_antiskid)
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    _tiedProperties.Tie( "tailhook", this, &FGControls::get_tailhook, &FGControls::set_tailhook)
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    _tiedProperties.Tie( "launchbar", this, &FGControls::get_launchbar, &FGControls::set_launchbar)
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    _tiedProperties.Tie( "catapult-launch-cmd", this, &FGControls::get_catapult_launch_cmd, &FGControls::set_catapult_launch_cmd)
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    _tiedProperties.Tie( "tailwheel-lock", this, &FGControls::get_tailwheel_lock, &FGControls::set_tailwheel_lock)
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    for (index = 0; index < MAX_WHEELS; index++) {
        _SetRoot( _tiedProperties, "/controls/gear/wheel", index );
        _tiedProperties.Tie( "alternate-extension", this, index, &FGControls::get_alternate_extension, &FGControls::set_alternate_extension)
            ->setAttribute( SGPropertyNode::ARCHIVE, true );
    }

    // anti-ice
    _SetRoot( _tiedProperties, "/controls/anti-ice" );

    _tiedProperties.Tie( "wing-heat", this, &FGControls::get_wing_heat, &FGControls::set_wing_heat)
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    _tiedProperties.Tie( "pitot-heat", this, &FGControls::get_pitot_heat, &FGControls::set_pitot_heat)
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    _tiedProperties.Tie( "wiper", this, &FGControls::get_wiper, &FGControls::set_wiper)
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    _tiedProperties.Tie( "window-heat", this, &FGControls::get_window_heat, &FGControls::set_window_heat)
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    for (index = 0; index < MAX_ENGINES; index++) {
        _SetRoot( _tiedProperties, "/controls/anti-ice/engine", index );

        _tiedProperties.Tie( "carb-heat", this, index, &FGControls::get_carb_heat, &FGControls::set_carb_heat)
            ->setAttribute( SGPropertyNode::ARCHIVE, true );

        _tiedProperties.Tie( "inlet-heat", this, index, &FGControls::get_inlet_heat, &FGControls::set_inlet_heat)
            ->setAttribute( SGPropertyNode::ARCHIVE, true );
    }

    // hydraulics
    for (index = 0; index < MAX_HYD_SYSTEMS; index++) {
        _SetRoot( _tiedProperties, "/controls/hydraulic/system", index );

        _tiedProperties.Tie( "engine-pump", this, index, &FGControls::get_engine_pump, &FGControls::set_engine_pump)
            ->setAttribute( SGPropertyNode::ARCHIVE, true );

        _tiedProperties.Tie( "electric-pump", this, index, &FGControls::get_electric_pump, &FGControls::set_electric_pump)
            ->setAttribute( SGPropertyNode::ARCHIVE, true );
    }  

    // electric
    _SetRoot( _tiedProperties, "/controls/electric" );

    _tiedProperties.Tie( "battery-switch", this, &FGControls::get_battery_switch, &FGControls::set_battery_switch)
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    _tiedProperties.Tie( "external-power", this, &FGControls::get_external_power, &FGControls::set_external_power)
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    _tiedProperties.Tie( "APU-generator", this, &FGControls::get_APU_generator, &FGControls::set_APU_generator)
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    for (index = 0; index < MAX_ENGINES; index++) {
        _SetRoot( _tiedProperties, "/controls/electric/engine", index );

        _tiedProperties.Tie( "generator", this, index, &FGControls::get_generator_breaker, &FGControls::set_generator_breaker)
            ->setAttribute( SGPropertyNode::ARCHIVE, true );

        _tiedProperties.Tie( "bus-tie", this, index, &FGControls::get_bus_tie, &FGControls::set_bus_tie)
            ->setAttribute( SGPropertyNode::ARCHIVE, true );
    }  

    // pneumatic
    _SetRoot( _tiedProperties, "/controls/pneumatic" );

    _tiedProperties.Tie( "APU-bleed", this, &FGControls::get_APU_bleed, &FGControls::set_APU_bleed)
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    for (index = 0; index < MAX_ENGINES; index++) {
        _SetRoot( _tiedProperties, "/controls/pneumatic/engine", index );

        _tiedProperties.Tie( "bleed", this, index, &FGControls::get_engine_bleed, &FGControls::set_engine_bleed)
            ->setAttribute( SGPropertyNode::ARCHIVE, true );
    }

    // pressurization
    _SetRoot( _tiedProperties, "/controls/pressurization" );

    _tiedProperties.Tie( "mode", this, &FGControls::get_mode, &FGControls::set_mode)
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    _tiedProperties.Tie( "dump", this, &FGControls::get_dump, &FGControls::set_dump)
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    _tiedProperties.Tie( "outflow-valve", this, &FGControls::get_outflow_valve, &FGControls::set_outflow_valve)
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    for (index = 0; index < MAX_PACKS; index++) {
        _SetRoot( _tiedProperties, "/controls/pressurization/pack", index );

        _tiedProperties.Tie( "pack-on", this, index, &FGControls::get_pack_on, &FGControls::set_pack_on)
            ->setAttribute( SGPropertyNode::ARCHIVE, true );
    }

    // lights
    _SetRoot( _tiedProperties, "/controls/lighting" );

    _tiedProperties.Tie( "landing-lights", this, &FGControls::get_landing_lights, &FGControls::set_landing_lights)
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    _tiedProperties.Tie( "turn-off-lights", this, &FGControls::get_turn_off_lights, &FGControls::set_turn_off_lights)
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    _tiedProperties.Tie( "taxi-light", this, &FGControls::get_taxi_light, &FGControls::set_taxi_light)
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    _tiedProperties.Tie( "logo-lights", this, &FGControls::get_logo_lights, &FGControls::set_logo_lights)
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    _tiedProperties.Tie( "nav-lights", this, &FGControls::get_nav_lights, &FGControls::set_nav_lights)
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    _tiedProperties.Tie( "beacon", this, &FGControls::get_beacon, &FGControls::set_beacon)
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    _tiedProperties.Tie( "strobe", this, &FGControls::get_strobe, &FGControls::set_strobe)
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    _tiedProperties.Tie( "panel-norm", this, &FGControls::get_panel_norm, &FGControls::set_panel_norm)
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    _tiedProperties.Tie( "instruments-norm", this, &FGControls::get_instruments_norm, &FGControls::set_instruments_norm)
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    _tiedProperties.Tie( "dome-norm", this, &FGControls::get_dome_norm, &FGControls::set_dome_norm)
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    // armament
    _SetRoot( _tiedProperties, "/controls/armament" );

    _tiedProperties.Tie( "master-arm", this, &FGControls::get_master_arm, &FGControls::set_master_arm)
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    _tiedProperties.Tie( "station-select", this, &FGControls::get_station_select, &FGControls::set_station_select)
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    _tiedProperties.Tie( "release-all", this, &FGControls::get_release_ALL, &FGControls::set_release_ALL)
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    for (index = 0; index < MAX_STATIONS; index++) {
        _SetRoot( _tiedProperties, "/controls/armament/station", index );

        _tiedProperties.Tie( "stick-size", this, index, &FGControls::get_stick_size, &FGControls::set_stick_size)
            ->setAttribute( SGPropertyNode::ARCHIVE, true );

        _tiedProperties.Tie( "release-stick", this, index, &FGControls::get_release_stick, &FGControls::set_release_stick)
            ->setAttribute( SGPropertyNode::ARCHIVE, true );

        _tiedProperties.Tie( "release-all", this, index, &FGControls::get_release_all, &FGControls::set_release_all)
            ->setAttribute( SGPropertyNode::ARCHIVE, true );

        _tiedProperties.Tie( "jettison-all", this, index, &FGControls::get_jettison_all, &FGControls::set_jettison_all)
            ->setAttribute( SGPropertyNode::ARCHIVE, true );
    }

    // seat
    _SetRoot( _tiedProperties, "/controls/seat" );

    _tiedProperties.Tie( "vertical-adjust", this, &FGControls::get_vertical_adjust, &FGControls::set_vertical_adjust)
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    _tiedProperties.Tie( "fore-aft-adjust", this, &FGControls::get_fore_aft_adjust, &FGControls::set_fore_aft_adjust)
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    _tiedProperties.Tie( "cmd_selector_valve", this, &FGControls::get_cmd_selector_valve, &FGControls::set_cmd_selector_valve)
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    for (index = 0; index < MAX_EJECTION_SEATS; index++) {
        _SetRoot( _tiedProperties, "/controls/seat/eject", index );

        _tiedProperties.Tie( "initiate", this, index, &FGControls::get_ejection_seat, &FGControls::set_ejection_seat)
            ->setAttribute( SGPropertyNode::ARCHIVE, true );

        _tiedProperties.Tie( "status", this, index, &FGControls::get_eseat_status, &FGControls::set_eseat_status)
            ->setAttribute( SGPropertyNode::ARCHIVE, true );
    }

    // APU
    _SetRoot( _tiedProperties, "/controls/APU" );

    _tiedProperties.Tie( "off-start-run", this, &FGControls::get_off_start_run, &FGControls::set_off_start_run)
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    _tiedProperties.Tie( "fire-switch", this, &FGControls::get_APU_fire_switch, &FGControls::set_APU_fire_switch)
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    // autoflight
    for (index = 0; index < MAX_AUTOPILOTS; index++) {

        _SetRoot( _tiedProperties, "/controls/autoflight/autopilot", index );

        _tiedProperties.Tie( "engage", this, index, &FGControls::get_autopilot_engage, &FGControls::set_autopilot_engage)
            ->setAttribute( SGPropertyNode::ARCHIVE, true );
    }

    _SetRoot( _tiedProperties, "/controls/autoflight/" );

    _tiedProperties.Tie( "autothrottle-arm", this, &FGControls::get_autothrottle_arm, &FGControls::set_autothrottle_arm)
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    _tiedProperties.Tie( "autothrottle-engage", this, &FGControls::get_autothrottle_engage, &FGControls::set_autothrottle_engage)
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    _tiedProperties.Tie( "heading-select", this, &FGControls::get_heading_select, &FGControls::set_heading_select)
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    _tiedProperties.Tie( "altitude-select", this, &FGControls::get_altitude_select, &FGControls::set_altitude_select)
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    _tiedProperties.Tie( "bank-angle-select", this, &FGControls::get_bank_angle_select, &FGControls::set_bank_angle_select)
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    _tiedProperties.Tie( "vertical-speed-select", this, &FGControls::get_vertical_speed_select, &FGControls::set_vertical_speed_select)
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    _tiedProperties.Tie( "speed-select", this, &FGControls::get_speed_select, &FGControls::set_speed_select)
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    _tiedProperties.Tie( "mach-select", this, &FGControls::get_mach_select, &FGControls::set_mach_select)
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    _tiedProperties.Tie( "vertical-mode", this, &FGControls::get_vertical_mode, &FGControls::set_vertical_mode)
        ->setAttribute( SGPropertyNode::ARCHIVE, true );

    _tiedProperties.Tie( "lateral-mode", this, &FGControls::get_lateral_mode, &FGControls::set_lateral_mode)
        ->setAttribute( SGPropertyNode::ARCHIVE, true );
}

void FGControls::unbind ()
{
    _tiedProperties.Untie();
} 


void
FGControls::update (double dt)
{
}



////////////////////////////////////////////////////////////////////////
// Setters and adjusters.
////////////////////////////////////////////////////////////////////////

void
FGControls::set_aileron (double pos)
{
    aileron = pos;
    SG_CLAMP_RANGE<double>( aileron, -1.0, 1.0 );
    do_autocoordination();
}

void
FGControls::move_aileron (double amt)
{
    aileron += amt;
    SG_CLAMP_RANGE<double>( aileron, -1.0, 1.0 );
    do_autocoordination();
}

void
FGControls::set_aileron_trim( double pos )
{
    aileron_trim = pos;
    SG_CLAMP_RANGE<double>( aileron_trim, -1.0, 1.0 );
}

void
FGControls::move_aileron_trim( double amt )
{
    aileron_trim += amt;
    SG_CLAMP_RANGE<double>( aileron_trim, -1.0, 1.0 );
}

void
FGControls::set_elevator( double pos )
{
    elevator = pos;
    SG_CLAMP_RANGE<double>( elevator, -1.0, 1.0 );
}

void
FGControls::move_elevator( double amt )
{
    elevator += amt;
    SG_CLAMP_RANGE<double>( elevator, -1.0, 1.0 );
}

void
FGControls::set_elevator_trim( double pos )
{
    elevator_trim = pos;
    SG_CLAMP_RANGE<double>( elevator_trim, -1.0, 1.0 );
}

void
FGControls::move_elevator_trim( double amt )
{
    elevator_trim += amt;
    SG_CLAMP_RANGE<double>( elevator_trim, -1.0, 1.0 );
}

void
FGControls::set_rudder( double pos )
{
    rudder = pos;
    SG_CLAMP_RANGE<double>( rudder, -1.0, 1.0 );
}

void
FGControls::move_rudder( double amt )
{
    rudder += amt;
    SG_CLAMP_RANGE<double>( rudder, -1.0, 1.0 );
}

void
FGControls::set_rudder_trim( double pos )
{
    rudder_trim = pos;
    SG_CLAMP_RANGE<double>( rudder_trim, -1.0, 1.0 );
}

void
FGControls::move_rudder_trim( double amt )
{
    rudder_trim += amt;
    SG_CLAMP_RANGE<double>( rudder_trim, -1.0, 1.0 );
}

void
FGControls::set_flaps( double pos )
{
    flaps = pos;
    SG_CLAMP_RANGE<double>( flaps, 0.0, 1.0 );
}

void
FGControls::move_flaps( double amt )
{
    flaps += amt;
    SG_CLAMP_RANGE<double>( flaps, 0.0, 1.0 );
}

void
FGControls::set_slats( double pos )
{
    slats = pos;
    SG_CLAMP_RANGE<double>( slats, 0.0, 1.0 );
}

void
FGControls::move_slats( double amt )
{
    slats += amt;
    SG_CLAMP_RANGE<double>( slats, 0.0, 1.0 );
}

void
FGControls::set_BLC( bool val )
{
    BLC = val;
}

void
FGControls::set_spoilers( double pos )
{
    spoilers = pos;
    SG_CLAMP_RANGE<double>( spoilers, 0.0, 1.0 );
}

void
FGControls::move_spoilers( double amt )
{
    spoilers += amt;
    SG_CLAMP_RANGE<double>( spoilers, 0.0, 1.0 );
}

void
FGControls::set_speedbrake( double pos )
{
    speedbrake = pos;
    SG_CLAMP_RANGE<double>( speedbrake, 0.0, 1.0 );
}

void
FGControls::move_speedbrake( double amt )
{
    speedbrake += amt;
    SG_CLAMP_RANGE<double>( speedbrake, 0.0, 1.0 );
}

void
FGControls::set_wing_sweep( double pos )
{
    wing_sweep = pos;
    SG_CLAMP_RANGE<double>( wing_sweep, 0.0, 1.0 );
}

void
FGControls::move_wing_sweep( double amt )
{
    wing_sweep += amt;
    SG_CLAMP_RANGE<double>( wing_sweep, 0.0, 1.0 );
}

void
FGControls::set_wing_fold( bool val )
{
    wing_fold = val;
}

void
FGControls::set_drag_chute( bool val )
{
    drag_chute = val;
}

void
FGControls::set_throttle_idle( bool val )
{
    throttle_idle = val;
}

void
FGControls::set_throttle( int engine, double pos )
{
    if ( engine == ALL_ENGINES ) {
        for ( int i = 0; i < MAX_ENGINES; i++ ) {
            throttle[i] = pos;
            SG_CLAMP_RANGE<double>( throttle[i], 0.0, 1.0 );
        }
    } else {
        if ( (engine >= 0) && (engine < MAX_ENGINES) ) {
            throttle[engine] = pos;
            SG_CLAMP_RANGE<double>( throttle[engine], 0.0, 1.0 );
        }
    }
}

void
FGControls::move_throttle( int engine, double amt )
{
    if ( engine == ALL_ENGINES ) {
        for ( int i = 0; i < MAX_ENGINES; i++ ) {
            throttle[i] += amt;
            SG_CLAMP_RANGE<double>( throttle[i], 0.0, 1.0 );
        }
    } else {
        if ( (engine >= 0) && (engine < MAX_ENGINES) ) {
            throttle[engine] += amt;
            SG_CLAMP_RANGE<double>( throttle[engine], 0.0, 1.0 );
        }
    }
}

void
FGControls::set_starter( int engine, bool flag )
{
    if ( engine == ALL_ENGINES ) {
        for ( int i = 0; i < MAX_ENGINES; i++ ) {
            starter[i] = flag;
        }
    } else {
        if ( (engine >= 0) && (engine < MAX_ENGINES) ) {
            starter[engine] = flag;
        }
    }
}

void
FGControls::set_fuel_pump( int engine, bool val )
{
    if ( engine == ALL_ENGINES ) {
        for ( int i = 0; i < MAX_ENGINES; i++ ) {
            fuel_pump[i] = val;
        }
    } else {
        if ( (engine >= 0) && (engine < MAX_ENGINES) ) {
            fuel_pump[engine] = val;
        }
    }
}

void
FGControls::set_fire_switch( int engine, bool val )
{
    if ( engine == ALL_ENGINES ) {
        for ( int i = 0; i < MAX_ENGINES; i++ ) {
            fire_switch[i] = val;
        }
    } else {
        if ( (engine >= 0) && (engine < MAX_ENGINES) ) {
            fire_switch[engine] = val;
        }
    }
}

void
FGControls::set_fire_bottle_discharge( int engine, bool val )
{
    if ( engine == ALL_ENGINES ) {
        for ( int i = 0; i < MAX_ENGINES; i++ ) {
            fire_bottle_discharge[i] = val;
        }
    } else {
        if ( (engine >= 0) && (engine < MAX_ENGINES) ) {
            fire_bottle_discharge[engine] = val;
        }
    }
}

void
FGControls::set_cutoff( int engine, bool val )
{
    if ( engine == ALL_ENGINES ) {
        for ( int i = 0; i < MAX_ENGINES; i++ ) {
            cutoff[i] = val;
        }
    } else {
        if ( (engine >= 0) && (engine < MAX_ENGINES) ) {
            cutoff[engine] = val;
        }
    }
}

void
FGControls::set_feed_tank( int engine, int tank )
{ 
    if ( engine == ALL_ENGINES ) {
        for ( int i = 0; i < MAX_ENGINES; i++ ) {
            feed_tank[i] = tank;
            SG_CLAMP_RANGE<int>( feed_tank[i], -1, 4 );
        }
    } else {
        if ( (engine >= 0) && (engine < MAX_ENGINES) ) {
            feed_tank[engine] = tank;
            SG_CLAMP_RANGE<int>( feed_tank[engine], -1, 4 );
        }
    } 
    //   feed_tank[engine] = engine;
}


void
FGControls::set_mixture( int engine, double pos )
{
    if ( engine == ALL_ENGINES ) {
        for ( int i = 0; i < MAX_ENGINES; i++ ) {
            mixture[i] = pos;
            SG_CLAMP_RANGE<double>( mixture[i], 0.0, 1.0 );
        }
    } else {
        if ( (engine >= 0) && (engine < MAX_ENGINES) ) {
            mixture[engine] = pos;
            SG_CLAMP_RANGE<double>( mixture[engine], 0.0, 1.0 );
        }
    }
}

void
FGControls::move_mixture( int engine, double amt )
{
    if ( engine == ALL_ENGINES ) {
        for ( int i = 0; i < MAX_ENGINES; i++ ) {
            mixture[i] += amt;
            SG_CLAMP_RANGE<double>( mixture[i], 0.0, 1.0 );
        }
    } else {
        if ( (engine >= 0) && (engine < MAX_ENGINES) ) {
            mixture[engine] += amt;
            SG_CLAMP_RANGE<double>( mixture[engine], 0.0, 1.0 );
        }
    }
}

void
FGControls::set_prop_advance( int engine, double pos )
{
    if ( engine == ALL_ENGINES ) {
        for ( int i = 0; i < MAX_ENGINES; i++ ) {
            prop_advance[i] = pos;
            SG_CLAMP_RANGE<double>( prop_advance[i], 0.0, 1.0 );
        }
    } else {
        if ( (engine >= 0) && (engine < MAX_ENGINES) ) {
            prop_advance[engine] = pos;
            SG_CLAMP_RANGE<double>( prop_advance[engine], 0.0, 1.0 );
        }
    }
}

void
FGControls::move_prop_advance( int engine, double amt )
{
    if ( engine == ALL_ENGINES ) {
        for ( int i = 0; i < MAX_ENGINES; i++ ) {
            prop_advance[i] += amt;
            SG_CLAMP_RANGE<double>( prop_advance[i], 0.0, 1.0 );
        }
    } else {
        if ( (engine >= 0) && (engine < MAX_ENGINES) ) {
            prop_advance[engine] += amt;
            SG_CLAMP_RANGE<double>( prop_advance[engine], 0.0, 1.0 );
        }
    }
}

void
FGControls::set_magnetos( int engine, int pos )
{
    if ( engine == ALL_ENGINES ) {
        for ( int i = 0; i < MAX_ENGINES; i++ ) {
            magnetos[i] = pos;
            SG_CLAMP_RANGE<int>( magnetos[i], 0, 3 );
        }
    } else {
        if ( (engine >= 0) && (engine < MAX_ENGINES) ) {
            magnetos[engine] = pos;
            SG_CLAMP_RANGE<int>( magnetos[engine], 0, 3 );
        }
    }
}

void
FGControls::move_magnetos( int engine, int amt )
{
    if ( engine == ALL_ENGINES ) {
        for ( int i = 0; i < MAX_ENGINES; i++ ) {
            magnetos[i] += amt;
            SG_CLAMP_RANGE<int>( magnetos[i], 0, 3 );
        }
    } else {
        if ( (engine >= 0) && (engine < MAX_ENGINES) ) {
            magnetos[engine] += amt;
            SG_CLAMP_RANGE<int>( magnetos[engine], 0, 3 );
        }
    }
}

void
FGControls::set_nitrous_injection( int engine, bool val )
{
    if ( engine == ALL_ENGINES ) {
        for ( int i = 0; i < MAX_ENGINES; i++ ) {
            nitrous_injection[i] = val;
        }
    } else {
        if ( (engine >= 0) && (engine < MAX_ENGINES) ) {
            nitrous_injection[engine] = val;
        }
    }
}


void
FGControls::set_cowl_flaps_norm( int engine, double pos )
{
    if ( engine == ALL_ENGINES ) {
        for ( int i = 0; i < MAX_ENGINES; i++ ) {
            cowl_flaps_norm[i] = pos;
            SG_CLAMP_RANGE<double>( cowl_flaps_norm[i], 0.0, 1.0 );
        }
    } else {
        if ( (engine >= 0) && (engine < MAX_ENGINES) ) {
            cowl_flaps_norm[engine] = pos;
            SG_CLAMP_RANGE<double>( cowl_flaps_norm[engine], 0.0, 1.0 );
        }
    }
}

void
FGControls::move_cowl_flaps_norm( int engine, double amt )
{
    if ( engine == ALL_ENGINES ) {
        for ( int i = 0; i < MAX_ENGINES; i++ ) {
            cowl_flaps_norm[i] += amt;
            SG_CLAMP_RANGE<double>( cowl_flaps_norm[i], 0.0, 1.0 );
        }
    } else {
        if ( (engine >= 0) && (engine < MAX_ENGINES) ) {
            cowl_flaps_norm[engine] += amt;
            SG_CLAMP_RANGE<double>( cowl_flaps_norm[engine], 0.0, 1.0 );
        }
    }
}

void
FGControls::set_feather( int engine, bool val )
{
    if ( engine == ALL_ENGINES ) {
        for ( int i = 0; i < MAX_ENGINES; i++ ) {
            feather[i] = val;
        }
    } else {
        if ( (engine >= 0) && (engine < MAX_ENGINES) ) {
            feather[engine] = val;
        }
    }
}

void
FGControls::set_ignition( int engine, int pos )
{
    if ( engine == ALL_ENGINES ) {
        for ( int i = 0; i < MAX_ENGINES; i++ ) {
            ignition[i] = pos;
            SG_CLAMP_RANGE<int>( ignition[i], 0, 3 );
        }
    } else {
        if ( (engine >= 0) && (engine < MAX_ENGINES) ) {
            ignition[engine] = pos;
            SG_CLAMP_RANGE<int>( ignition[engine], 0, 3 );
        }
    }
}

void
FGControls::set_augmentation( int engine, bool val )
{
    if ( engine == ALL_ENGINES ) {
        for ( int i = 0; i < MAX_ENGINES; i++ ) {
            augmentation[i] = val;
        }
    } else {
        if ( (engine >= 0) && (engine < MAX_ENGINES) ) {
            augmentation[engine] = val;
        }
    }
}

void
FGControls::set_reverser( int engine, bool val )
{
    if ( engine == ALL_ENGINES ) {
        for ( int i = 0; i < MAX_ENGINES; i++ ) {
            reverser[i] = val;
        }
    } else {
        if ( (engine >= 0) && (engine < MAX_ENGINES) ) {
            reverser[engine] = val;
        }
    }
}

void
FGControls::set_water_injection( int engine, bool val )
{
    if ( engine == ALL_ENGINES ) {
        for ( int i = 0; i < MAX_ENGINES; i++ ) {
            water_injection[i] = val;
        }
    } else {
        if ( (engine >= 0) && (engine < MAX_ENGINES) ) {
            water_injection[engine] = val;
        }
    }
}

void
FGControls::set_condition( int engine, double val )
{
    if ( engine == ALL_ENGINES ) {
        for ( int i = 0; i < MAX_ENGINES; i++ ) {
            condition[i] = val;
            SG_CLAMP_RANGE<double>( condition[i], 0.0, 1.0 );
        }
    } else {
        if ( (engine >= 0) && (engine < MAX_ENGINES) ) {
            condition[engine] = val;
            SG_CLAMP_RANGE<double>( condition[engine], 0.0, 1.0 );
        }
    }
}

void
FGControls::set_dump_valve( bool val )
{
    dump_valve = val;
}


void
FGControls::set_fuel_selector( int tank, bool pos )
{
    if ( tank == ALL_TANKS ) {
        for ( int i = 0; i < MAX_TANKS; i++ ) {
            fuel_selector[i] = pos;
        }
    } else {
        if ( (tank >= 0) && (tank < MAX_TANKS) ) {
            fuel_selector[tank] = pos;
        }
    }
}

void
FGControls::set_to_engine( int tank, int engine )
{
    if ( tank == ALL_TANKS ) {
        for ( int i = 0; i < MAX_TANKS; i++ ) {
            to_engine[i] = engine;
        }
    } else {
        if ( (tank >= 0) && (tank < MAX_TANKS) ) {
            to_engine[tank] = engine;
        }
    }
}

void
FGControls::set_to_tank( int tank, int dest_tank )
{
    if ( tank == ALL_TANKS ) {
        for ( int i = 0; i < MAX_TANKS; i++ ) {
            to_tank[i] = dest_tank;
        }
    } else {
        if ( (tank >= 0) && (tank < MAX_TANKS) ) {
            to_tank[tank] = dest_tank;
        }
    }
}

void
FGControls::set_boost_pump( int index, bool val ) 
{
    if ( index == -1 ) {
        for ( int i = 0; i < (MAX_TANKS * MAX_BOOSTPUMPS); i++ ) {
            boost_pump[i] = val;
        }
    } else {
        if ( (index >= 0) && (index < (MAX_TANKS * MAX_BOOSTPUMPS)) ) {
            boost_pump[index] = val;
        }
    }
}


void
FGControls::set_brake_left( double pos )
{
    brake_left = pos;
    SG_CLAMP_RANGE<double>(brake_left, 0.0, 1.0);
}

void
FGControls::move_brake_left( double amt )
{
    brake_left += amt;
    SG_CLAMP_RANGE<double>( brake_left, 0.0, 1.0 );
}

void
FGControls::set_brake_right( double pos )
{
    brake_right = pos;
    SG_CLAMP_RANGE<double>(brake_right, 0.0, 1.0);
}

void
FGControls::move_brake_right( double amt )
{
    brake_right += amt;
    SG_CLAMP_RANGE<double>( brake_right, 0.0, 1.0 );
}

void
FGControls::set_copilot_brake_left( double pos )
{
    copilot_brake_left = pos;
    SG_CLAMP_RANGE<double>(brake_left, 0.0, 1.0);
}

void
FGControls::set_copilot_brake_right( double pos )
{
    copilot_brake_right = pos;
    SG_CLAMP_RANGE<double>(brake_right, 0.0, 1.0);
}

void
FGControls::set_brake_parking( double pos )
{
    brake_parking = pos;
    SG_CLAMP_RANGE<double>(brake_parking, 0.0, 1.0);
}

void
FGControls::set_steering( double angle )
{
    steering = angle;
    SG_CLAMP_RANGE<double>(steering, -80.0, 80.0);
}

void
FGControls::set_nose_wheel_steering( bool nws )
{
    nose_wheel_steering = nws;
}

void
FGControls::move_steering( double angle )
{
    steering += angle;
    SG_CLAMP_RANGE<double>(steering, -80.0, 80.0);
}

void
FGControls::set_gear_down( bool gear )
{
    gear_down = gear;
}

void
FGControls::set_antiskid( bool state )
{
    antiskid = state;
}

void
FGControls::set_tailhook( bool state )
{
    tailhook = state;
}

void
FGControls::set_launchbar( bool state )
{
    launchbar = state;
}

void
FGControls::set_catapult_launch_cmd( bool state )
{
    catapult_launch_cmd = state;
}

void
FGControls::set_tailwheel_lock( bool state )
{
    tailwheel_lock = state;
}


void
FGControls::set_alternate_extension( int wheel, bool val )
{
    if ( wheel == ALL_WHEELS ) {
        for ( int i = 0; i < MAX_WHEELS; i++ ) {
            alternate_extension[i] = val;
        }
    } else {
        if ( (wheel >= 0) && (wheel < MAX_WHEELS) ) {
            alternate_extension[wheel] = val;
        }
    }
}

void
FGControls::set_wing_heat( bool state )
{
    wing_heat = state;
}

void
FGControls::set_pitot_heat( bool state )
{
    pitot_heat = state;
}

void
FGControls::set_wiper( int state )
{
    wiper = state;
}

void
FGControls::set_window_heat( bool state )
{
    window_heat = state;
}

void
FGControls::set_carb_heat( int engine, bool val )
{
    if ( engine == ALL_ENGINES ) {
        for ( int i = 0; i < MAX_ENGINES; i++ ) {
            carb_heat[i] = val;
        }
    } else {
        if ( (engine >= 0) && (engine < MAX_ENGINES) ) {
            carb_heat[engine] = val;
        }
    }
}

void
FGControls::set_inlet_heat( int engine, bool val )
{
    if ( engine == ALL_ENGINES ) {
        for ( int i = 0; i < MAX_ENGINES; i++ ) {
            inlet_heat[i] = val;
        }
    } else {
        if ( (engine >= 0) && (engine < MAX_ENGINES) ) {
            inlet_heat[engine] = val;
        }
    }
}

void
FGControls::set_engine_pump( int system, bool val )
{
    if ( system == ALL_HYD_SYSTEMS ) {
        for ( int i = 0; i < MAX_HYD_SYSTEMS; i++ ) {
            engine_pump[i] = val;
        }
    } else {
        if ( (system >= 0) && (system < MAX_HYD_SYSTEMS) ) {
            engine_pump[system] = val;
        }
    }
}

void
FGControls::set_electric_pump( int system, bool val )
{
    if ( system == ALL_HYD_SYSTEMS ) {
        for ( int i = 0; i < MAX_HYD_SYSTEMS; i++ ) {
            electric_pump[i] = val;
        }
    } else {
        if ( (system >= 0) && (system < MAX_HYD_SYSTEMS) ) {
            electric_pump[system] = val;
        }
    }
}

void
FGControls::set_battery_switch( bool state )
{
    battery_switch = state;
}

void
FGControls::set_external_power( bool state )
{
    external_power = state;
}

void
FGControls::set_APU_generator( bool state )
{
    APU_generator = state;
}

void
FGControls::set_generator_breaker( int engine, bool val )
{
    if ( engine == ALL_ENGINES ) {
        for ( int i = 0; i < MAX_ENGINES; i++ ) {
            generator_breaker[i] = val;
        }
    } else {
        if ( (engine >= 0) && (engine < MAX_ENGINES) ) {
            generator_breaker[engine] = val;
        }
    }
}

void
FGControls::set_bus_tie( int engine, bool val )
{
    if ( engine == ALL_ENGINES ) {
        for ( int i = 0; i < MAX_ENGINES; i++ ) {
            bus_tie[i] = val;
        }
    } else {
        if ( (engine >= 0) && (engine < MAX_ENGINES) ) {
            bus_tie[engine] = val;
        }
    }
}

void
FGControls::set_APU_bleed( bool state )
{
    APU_bleed = state;
}

void
FGControls::set_engine_bleed( int engine, bool val )
{
    if ( engine == ALL_ENGINES ) {
        for ( int i = 0; i < MAX_ENGINES; i++ ) {
            engine_bleed[i] = val;
        }
    } else {
        if ( (engine >= 0) && (engine < MAX_ENGINES) ) {
            engine_bleed[engine] = val;
        }
    }
}

void
FGControls::set_mode( int new_mode )
{
    mode = new_mode;
}

void
FGControls::set_outflow_valve( double pos )
{
    outflow_valve = pos;
    SG_CLAMP_RANGE<double>( outflow_valve, 0.0, 1.0 );
}

void
FGControls::move_outflow_valve( double amt )
{
    outflow_valve += amt;
    SG_CLAMP_RANGE<double>( outflow_valve, 0.0, 1.0 );
}

void
FGControls::set_dump( bool state )
{
    dump = state;
}

void
FGControls::set_pack_on( int pack, bool val )
{
    if ( pack == ALL_PACKS ) {
        for ( int i = 0; i < MAX_PACKS; i++ ) {
            pack_on[i] = val;
        }
    } else {
        if ( (pack >= 0) && (pack < MAX_PACKS) ) {
            pack_on[pack] = val;
        }
    }
}

void
FGControls::set_landing_lights( bool state )
{
    landing_lights = state;
}

void
FGControls::set_turn_off_lights( bool state )
{
    turn_off_lights = state;
}

void
FGControls::set_taxi_light( bool state )
{
    taxi_light = state;
}

void
FGControls::set_logo_lights( bool state )
{
    logo_lights = state;
}

void
FGControls::set_nav_lights( bool state )
{
    nav_lights = state;
}

void
FGControls::set_beacon( bool state )
{
    beacon = state;
}

void
FGControls::set_strobe( bool state )
{
    strobe = state;
}

void
FGControls::set_panel_norm( double intensity )
{
    panel_norm = intensity;
    SG_CLAMP_RANGE<double>( panel_norm, 0.0, 1.0 );
}

void
FGControls::move_panel_norm( double amt )
{
    panel_norm += amt;
    SG_CLAMP_RANGE<double>( panel_norm, 0.0, 1.0 );
}

void
FGControls::set_instruments_norm( double intensity )
{
    instruments_norm = intensity;
    SG_CLAMP_RANGE<double>( instruments_norm, 0.0, 1.0 );
}

void
FGControls::move_instruments_norm( double amt )
{
    instruments_norm += amt;
    SG_CLAMP_RANGE<double>( instruments_norm, 0.0, 1.0 );
}

void
FGControls::set_dome_norm( double intensity )
{
    dome_norm = intensity;
    SG_CLAMP_RANGE<double>( dome_norm, 0.0, 1.0 );
}

void
FGControls::move_dome_norm( double amt )
{
    dome_norm += amt;
    SG_CLAMP_RANGE<double>( dome_norm, 0.0, 1.0 );
}

void
FGControls::set_master_arm( bool val )
{
    master_arm = val;
}

void
FGControls::set_station_select( int station )
{
    station_select = station;
    SG_CLAMP_RANGE<int>( station_select, 0, MAX_STATIONS );
}

void
FGControls::set_release_ALL( bool val )
{
    release_ALL = val;
}

void
FGControls::set_stick_size( int station, int size )
{
    if ( station == ALL_STATIONS ) {
        for ( int i = 0; i < MAX_STATIONS; i++ ) {
            stick_size[i] = size;
            SG_CLAMP_RANGE<int>( stick_size[i], 1, 20 );
        }
    } else {
        if ( (station >= 0) && (station < MAX_STATIONS) ) {
            stick_size[station] = size;
            SG_CLAMP_RANGE<int>( stick_size[station], 1, 20 );
        }
    }
}

void
FGControls::set_release_stick( int station, bool val )
{
    if ( station == ALL_STATIONS ) {
        for ( int i = 0; i < MAX_STATIONS; i++ ) {
            release_stick[i] = val;
        }
    } else {
        if ( (station >= 0) && (station < MAX_STATIONS) ) {
            release_stick[station] = val;
        }
    }
}

void
FGControls::set_release_all( int station, bool val )
{
    if ( station == ALL_STATIONS ) {
        for ( int i = 0; i < MAX_STATIONS; i++ ) {
            release_all[i] = val;
        }
    } else {
        if ( (station >= 0) && (station < MAX_STATIONS) ) {
            release_all[station] = val;
        }
    }
}

void
FGControls::set_jettison_all( int station, bool val )
{
    if ( station == ALL_STATIONS ) {
        for ( int i = 0; i < MAX_STATIONS; i++ ) {
            jettison_all[i] = val;
        }
    } else {
        if ( (station >= 0) && (station < MAX_STATIONS) ) {
            jettison_all[station] = val;
        }
    }
}

void
FGControls::set_vertical_adjust( double pos )
{
    vertical_adjust = pos;
    SG_CLAMP_RANGE<double>( vertical_adjust, -1.0, 1.0 );
}

void
FGControls::move_vertical_adjust( double amt )
{
    vertical_adjust += amt;
    SG_CLAMP_RANGE<double>( vertical_adjust, -1.0, 1.0 );
}

void
FGControls::set_fore_aft_adjust( double pos )
{
    fore_aft_adjust = pos;
    SG_CLAMP_RANGE<double>( fore_aft_adjust, -1.0, 1.0 );
}

void
FGControls::move_fore_aft_adjust( double amt )
{
    fore_aft_adjust += amt;
    SG_CLAMP_RANGE<double>( fore_aft_adjust, -1.0, 1.0 );
}

void
FGControls::set_ejection_seat( int which_seat, bool val )
{
    if ( which_seat == ALL_EJECTION_SEATS ) {
        for ( int i = 0; i < MAX_EJECTION_SEATS; i++ ) {
            eject[i] = val;
        }
    } else {
        if ( (which_seat >= 0) && (which_seat <= MAX_EJECTION_SEATS) ) {
            if ( eseat_status[which_seat] == SEAT_SAFED ||
                eseat_status[which_seat] == SEAT_FAIL )
            {
                // we can never eject if SEAT_SAFED or SEAT_FAIL
                val = false;
            }

            eject[which_seat] = val;
        }
    }
}

void
FGControls::set_eseat_status( int which_seat, int val )
{
    if ( which_seat == ALL_EJECTION_SEATS ) {
        for ( int i = 0; i < MAX_EJECTION_SEATS; i++ ) {
            eseat_status[i] = val;
        }
    } else {
        if ( (which_seat >=0) && (which_seat <= MAX_EJECTION_SEATS) ) {
            eseat_status[which_seat] = val;
        }
    }
}

void
FGControls::set_cmd_selector_valve( int val )
{
    cmd_selector_valve = val;
}


void
FGControls::set_off_start_run( int pos )
{
    off_start_run = pos;
    SG_CLAMP_RANGE<int>( off_start_run, 0, 3 );
}

void
FGControls::set_APU_fire_switch( bool val )
{
    APU_fire_switch = val;
}

void
FGControls::set_autothrottle_arm( bool val )
{
    autothrottle_arm = val;
}

void
FGControls::set_autothrottle_engage( bool val )
{
    autothrottle_engage = val;
}

void
FGControls::set_heading_select( double heading )
{
    heading_select = heading;
    SG_CLAMP_RANGE<double>( heading_select, 0.0, 360.0 );
}

void
FGControls::move_heading_select( double amt )
{
    heading_select += amt;
    SG_CLAMP_RANGE<double>( heading_select, 0.0, 360.0 );
}

void
FGControls::set_altitude_select( double altitude )
{
    altitude_select = altitude;
    SG_CLAMP_RANGE<double>( altitude_select, -1000.0, 100000.0 );
}

void
FGControls::move_altitude_select( double amt )
{
    altitude_select += amt;
    SG_CLAMP_RANGE<double>( altitude_select, -1000.0, 100000.0 );
}

void
FGControls::set_bank_angle_select( double angle )
{
    bank_angle_select = angle;
    SG_CLAMP_RANGE<double>( bank_angle_select, 10.0, 30.0 );
}

void
FGControls::move_bank_angle_select( double amt )
{
    bank_angle_select += amt;
    SG_CLAMP_RANGE<double>( bank_angle_select, 10.0, 30.0 );
}

void
FGControls::set_vertical_speed_select( double speed )
{
    vertical_speed_select = speed;
    SG_CLAMP_RANGE<double>( vertical_speed_select, -3000.0, 4000.0 );
}

void
FGControls::move_vertical_speed_select( double amt )
{
    vertical_speed_select += amt;
    SG_CLAMP_RANGE<double>( vertical_speed_select, -3000.0, 4000.0 );
}

void
FGControls::set_speed_select( double speed )
{
    speed_select = speed;
    SG_CLAMP_RANGE<double>( speed_select, 60.0, 400.0 );
}

void
FGControls::move_speed_select( double amt )
{
    speed_select += amt;
    SG_CLAMP_RANGE<double>( speed_select, 60.0, 400.0 );
}

void
FGControls::set_mach_select( double mach )
{
    mach_select = mach;
    SG_CLAMP_RANGE<double>( mach_select, 0.4, 4.0 );
}

void
FGControls::move_mach_select( double amt )
{
    mach_select += amt;
    SG_CLAMP_RANGE<double>( mach_select, 0.4, 4.0 );
}

void
FGControls::set_vertical_mode( int mode )
{
    vertical_mode = mode;
    SG_CLAMP_RANGE<int>( vertical_mode, 0, 4 );
}

void
FGControls::set_lateral_mode( int mode )
{
    lateral_mode = mode;
    SG_CLAMP_RANGE<int>( lateral_mode, 0, 4 );
}

void
FGControls::set_autopilot_engage( int ap, bool val )
{
    if ( ap == ALL_AUTOPILOTS ) {
        for ( int i = 0; i < MAX_AUTOPILOTS; i++ ) {
            autopilot_engage[i] = val;
        }
    } else {
        if ( (ap >= 0) && (ap < MAX_AUTOPILOTS) ) {
            autopilot_engage[ap] = val;
        }
    }
}
