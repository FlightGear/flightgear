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

#include <simgear/compiler.h>
#include <simgear/debug/logstream.hxx>
#include <Main/fg_props.hxx>

#include "controls.hxx"


static const int MAX_NAME_LEN = 128;


////////////////////////////////////////////////////////////////////////
// Inline utility methods.
////////////////////////////////////////////////////////////////////////

static inline void
CLAMP(double *x, double min, double max )
{
  if ( *x < min ) { *x = min; }
  if ( *x > max ) { *x = max; }
}

static inline void
CLAMP(int *i, int min, int max )
{
  if ( *i < min ) { *i = min; }
  if ( *i > max ) { *i = max; }
}


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

    auto_coordination = fgGetNode("/sim/auto-coordination", true);
}


void
FGControls::bind ()
{
  int index, i;

  // flight controls
  fgTie("/controls/flight/aileron", this,
	&FGControls::get_aileron, &FGControls::set_aileron);
  fgSetArchivable("/controls/flight/aileron");

  fgTie("/controls/flight/aileron-trim", this,
       &FGControls::get_aileron_trim, &FGControls::set_aileron_trim);
  fgSetArchivable("/controls/flight/aileron-trim");

  fgTie("/controls/flight/elevator", this,
       &FGControls::get_elevator, &FGControls::set_elevator);
  fgSetArchivable("/controls/flight/elevator");

  fgTie("/controls/flight/elevator-trim", this,
       &FGControls::get_elevator_trim, &FGControls::set_elevator_trim);
  fgSetArchivable("/controls/flight/elevator-trim");

  fgTie("/controls/flight/rudder", this,
       &FGControls::get_rudder, &FGControls::set_rudder);
  fgSetArchivable("/controls/flight/rudder");

  fgTie("/controls/flight/rudder-trim", this,
       &FGControls::get_rudder_trim, &FGControls::set_rudder_trim);
  fgSetArchivable("/controls/flight/rudder-trim");

  fgTie("/controls/flight/flaps", this,
       &FGControls::get_flaps, &FGControls::set_flaps);
  fgSetArchivable("/controls/flight/flaps");

  fgTie("/controls/flight/slats", this,
       &FGControls::get_slats, &FGControls::set_slats);
  fgSetArchivable("/controls/flight/slats");

  fgTie("/controls/flight/BLC", this,
       &FGControls::get_BLC, &FGControls::set_BLC);
  fgSetArchivable("/controls/flight/BLC");

  fgTie("/controls/flight/spoilers", this,
       &FGControls::get_spoilers, &FGControls::set_spoilers);
  fgSetArchivable("/controls/flight/spoilers");

  fgTie("/controls/flight/speedbrake", this,
       &FGControls::get_speedbrake, &FGControls::set_speedbrake);
  fgSetArchivable("/controls/flight/speedbrake");

  fgTie("/controls/flight/wing-sweep", this,
       &FGControls::get_wing_sweep, &FGControls::set_wing_sweep);
  fgSetArchivable("/controls/flight/wing-sweep");

  fgTie("/controls/flight/wing-fold", this,
       &FGControls::get_wing_fold, &FGControls::set_wing_fold);
  fgSetArchivable("/controls/flight/wing-fold");

  fgTie("/controls/flight/drag-chute", this,
       &FGControls::get_drag_chute, &FGControls::set_drag_chute);
  fgSetArchivable("/controls/flight/drag-chute");

  // engines
  fgTie("/controls/engines/throttle_idle", this,
       &FGControls::get_throttle_idle, &FGControls::set_throttle_idle);
  fgSetArchivable("/controls/engines/throttle_idle");

  for (index = 0; index < MAX_ENGINES; index++) {
    char name[MAX_NAME_LEN];
    snprintf(name, MAX_NAME_LEN,
             "/controls/engines/engine[%d]/throttle", index);
    fgTie(name, this, index,
	  &FGControls::get_throttle, &FGControls::set_throttle);
    fgSetArchivable(name);

    snprintf(name, MAX_NAME_LEN, "/controls/engines/engine[%d]/starter", index);
    fgTie(name, this, index,
	 &FGControls::get_starter, &FGControls::set_starter);
    fgSetArchivable(name);

    snprintf(name, MAX_NAME_LEN,
             "/controls/engines/engine[%d]/fuel-pump", index);
    fgTie(name, this, index,
	 &FGControls::get_fuel_pump, &FGControls::set_fuel_pump);
    fgSetArchivable(name);

    snprintf(name, MAX_NAME_LEN,
             "/controls/engines/engine[%d]/fire-switch", index);
    fgTie(name, this, index,
	 &FGControls::get_fire_switch, &FGControls::set_fire_switch);
    fgSetArchivable(name);

    snprintf(name, MAX_NAME_LEN, 
       "/controls/engines/engine[%d]/fire-bottle-discharge", index);
    fgTie(name, this, index,
	 &FGControls::get_fire_bottle_discharge,
         &FGControls::set_fire_bottle_discharge);
    fgSetArchivable(name);

    snprintf(name, MAX_NAME_LEN, "/controls/engines/engine[%d]/cutoff", index);
    fgTie(name, this, index,
	 &FGControls::get_cutoff, &FGControls::set_cutoff);
    fgSetArchivable(name);

    snprintf(name, MAX_NAME_LEN, "/controls/engines/engine[%d]/mixture", index);
    fgTie(name, this, index,
	 &FGControls::get_mixture, &FGControls::set_mixture);
    fgSetArchivable(name);

    snprintf(name, MAX_NAME_LEN, 
       "/controls/engines/engine[%d]/propeller-pitch", index);
    fgTie(name, this, index,
	 &FGControls::get_prop_advance, 
         &FGControls::set_prop_advance);
    fgSetArchivable(name);

    snprintf(name, MAX_NAME_LEN,
             "/controls/engines/engine[%d]/magnetos", index);
    fgTie(name, this, index,
	 &FGControls::get_magnetos, &FGControls::set_magnetos);
    fgSetArchivable(name);
    
   snprintf(name, MAX_NAME_LEN,
       "/controls/engines/engine[%d]/feed_tank", index);
    fgTie(name, this, index,
	 &FGControls::get_feed_tank, &FGControls::set_feed_tank);
    fgSetArchivable(name);


    snprintf(name, MAX_NAME_LEN, "/controls/engines/engine[%d]/WEP", index);
    fgTie(name, this, index,
	 &FGControls::get_nitrous_injection,
         &FGControls::set_nitrous_injection);
    fgSetArchivable(name);

    snprintf(name, MAX_NAME_LEN, 
       "/controls/engines/engine[%d]/cowl-flaps-norm", index);
    fgTie(name, this, index,
	 &FGControls::get_cowl_flaps_norm, 
         &FGControls::set_cowl_flaps_norm);
    fgSetArchivable(name);

    snprintf(name, MAX_NAME_LEN,
             "/controls/engines/engine[%d]/propeller-feather", index);
    fgTie(name, this, index,
	 &FGControls::get_feather, &FGControls::set_feather);
    fgSetArchivable(name);

    snprintf(name, MAX_NAME_LEN,
             "/controls/engines/engine[%d]/ignition", index);
    fgTie(name, this, index,
	 &FGControls::get_ignition, &FGControls::set_ignition);
    fgSetArchivable(name);

    snprintf(name, MAX_NAME_LEN,
             "/controls/engines/engine[%d]/augmentation", index);
    fgTie(name, this, index,
	 &FGControls::get_augmentation, 
         &FGControls::set_augmentation);
    fgSetArchivable(name);

    snprintf(name, MAX_NAME_LEN,
             "/controls/engines/engine[%d]/reverser", index);
    fgTie(name, this, index,
	 &FGControls::get_reverser, &FGControls::set_reverser);
    fgSetArchivable(name);

    snprintf(name, MAX_NAME_LEN, 
       "/controls/engines/engine[%d]/water-injection", index);
    fgTie(name, this, index,
	 &FGControls::get_water_injection,
         &FGControls::set_water_injection);
    fgSetArchivable(name);

    snprintf(name, MAX_NAME_LEN,
             "/controls/engines/engine[%d]/condition", index);
    fgTie(name, this, index,
	 &FGControls::get_condition, &FGControls::set_condition);
    fgSetArchivable(name);
  }

  // fuel
  fgTie("/controls/fuel/dump-valve", this,
       &FGControls::get_dump_valve, &FGControls::set_dump_valve);
  fgSetArchivable("/controls/fuel/dump-valve");

  for (index = 0; index < MAX_TANKS; index++) {
    char name[MAX_NAME_LEN];
    snprintf(name, MAX_NAME_LEN,
             "/controls/fuel/tank[%d]/fuel_selector", index);
    fgTie(name, this, index,
	  &FGControls::get_fuel_selector, 
          &FGControls::set_fuel_selector);
    fgSetArchivable(name);  

    snprintf(name, MAX_NAME_LEN, "/controls/fuel/tank[%d]/to_engine", index);
    fgTie(name, this, index,
	  &FGControls::get_to_engine, &FGControls::set_to_engine);
    fgSetArchivable(name);  

    snprintf(name, MAX_NAME_LEN, "/controls/fuel/tank[%d]/to_tank", index);
    fgTie(name, this, index,
	  &FGControls::get_to_tank, &FGControls::set_to_tank);
    fgSetArchivable(name);  

    for (i = 0; i < MAX_BOOSTPUMPS; i++) {
      char name[MAX_NAME_LEN];
      snprintf(name, MAX_NAME_LEN, 
         "/controls/fuel/tank[%d]/boost-pump[%d]", index, i);
      fgTie(name, this, index * 2 + i,
	    &FGControls::get_boost_pump, 
            &FGControls::set_boost_pump);
      fgSetArchivable(name);  
    }
  }

  // gear
  fgTie("/controls/gear/brake-left", this,
	&FGControls::get_brake_left, 
        &FGControls::set_brake_left);
  fgSetArchivable("/controls/gear/brake-left");

  fgTie("/controls/gear/brake-right", this,
	&FGControls::get_brake_right, 
        &FGControls::set_brake_right);
  fgSetArchivable("/controls/gear/brake-right");

  fgTie("/controls/gear/copilot-brake-left", this,
	&FGControls::get_copilot_brake_left, 
        &FGControls::set_copilot_brake_left);
  fgSetArchivable("/controls/gear/copilot-brake-left");

  fgTie("/controls/gear/copilot-brake-right", this,
	&FGControls::get_copilot_brake_right, 
        &FGControls::set_copilot_brake_right);
  fgSetArchivable("/controls/gear/copilot-brake-right");

  fgTie("/controls/gear/brake-parking", this,
	&FGControls::get_brake_parking, 
        &FGControls::set_brake_parking);
  fgSetArchivable("/controls/gear/brake-parking");

  fgTie("/controls/gear/steering", this,
	&FGControls::get_steering, &FGControls::set_steering);
  fgSetArchivable("/controls/gear/steering");

  fgTie("/controls/gear/nose-wheel-steering", this,
	&FGControls::get_nose_wheel_steering,
        &FGControls::set_nose_wheel_steering);
  fgSetArchivable("/controls/gear/nose-wheel-steering");

  fgTie("/controls/gear/gear-down", this,
	&FGControls::get_gear_down, &FGControls::set_gear_down);
  fgSetArchivable("/controls/gear/gear-down");

  fgTie("/controls/gear/antiskid", this,
	&FGControls::get_antiskid, &FGControls::set_antiskid);
  fgSetArchivable("/controls/gear/antiskid");

  fgTie("/controls/gear/tailhook", this,
	&FGControls::get_tailhook, &FGControls::set_tailhook);
  fgSetArchivable("/controls/gear/tailhook");

  fgTie("/controls/gear/launchbar", this,
	&FGControls::get_launchbar, &FGControls::set_launchbar);
  fgSetArchivable("/controls/gear/launchbar");

  fgTie("/controls/gear/catapult-launch-cmd", this,
	&FGControls::get_catapult_launch_cmd, &FGControls::set_catapult_launch_cmd);
  fgSetArchivable("/controls/gear/catapult-launch-cmd");

  fgTie("/controls/gear/tailwheel-lock", this,
	&FGControls::get_tailwheel_lock, 
        &FGControls::set_tailwheel_lock);
  fgSetArchivable("/controls/gear/tailwheel-lock");

  for (index = 0; index < MAX_WHEELS; index++) {
      char name[MAX_NAME_LEN];
      snprintf(name, MAX_NAME_LEN,
               "/controls/gear/wheel[%d]/alternate-extension", index);
      fgTie(name, this, index,
            &FGControls::get_alternate_extension, 
            &FGControls::set_alternate_extension);
      fgSetArchivable(name);
  }

  // anti-ice
  fgTie("/controls/anti-ice/wing-heat", this,
	&FGControls::get_wing_heat, &FGControls::set_wing_heat);
  fgSetArchivable("/controls/anti-ice/wing-heat");

  fgTie("/controls/anti-ice/pitot-heat", this,
	&FGControls::get_pitot_heat, &FGControls::set_pitot_heat);
  fgSetArchivable("/controls/anti-ice/pitot-heat");

  fgTie("/controls/anti-ice/wiper", this,
	&FGControls::get_wiper, &FGControls::set_wiper);
  fgSetArchivable("/controls/anti-ice/wiper");

  fgTie("/controls/anti-ice/window-heat", this,
	&FGControls::get_window_heat, &FGControls::set_window_heat);
  fgSetArchivable("/controls/anti-ice/window-heat");

  for (index = 0; index < MAX_ENGINES; index++) {
      char name[MAX_NAME_LEN];
      snprintf(name, MAX_NAME_LEN,
               "/controls/anti-ice/engine[%d]/carb-heat", index);  
      fgTie(name, this, index,
	&FGControls::get_carb_heat, &FGControls::set_carb_heat);
      fgSetArchivable(name);

      snprintf(name, MAX_NAME_LEN,
               "/controls/anti-ice/engine[%d]/inlet-heat", index);  
      fgTie(name, this, index,
	&FGControls::get_inlet_heat, &FGControls::set_inlet_heat);
      fgSetArchivable(name);
  }

  // hydraulics
  for (index = 0; index < MAX_HYD_SYSTEMS; index++) {
      char name[MAX_NAME_LEN];
      snprintf(name, MAX_NAME_LEN, 
         "/controls/hydraulic/system[%d]/engine-pump", index);  
      fgTie(name, this, index,
	&FGControls::get_engine_pump, &FGControls::set_engine_pump);
      fgSetArchivable(name);

      snprintf(name, MAX_NAME_LEN, 
         "/controls/hydraulic/system[%d]/electric-pump", index);  
      fgTie(name, this, index,
	&FGControls::get_electric_pump, 
        &FGControls::set_electric_pump);
      fgSetArchivable(name);
  }  

  // electric
  fgTie("/controls/electric/battery-switch", this,
	&FGControls::get_battery_switch, 
        &FGControls::set_battery_switch);
  fgSetArchivable("/controls/electric/battery-switch");
  
  fgTie("/controls/electric/external-power", this,
	&FGControls::get_external_power, 
        &FGControls::set_external_power);
  fgSetArchivable("/controls/electric/external-power");

  fgTie("/controls/electric/APU-generator", this,
	&FGControls::get_APU_generator, 
        &FGControls::set_APU_generator);
  fgSetArchivable("/controls/electric/APU-generator");

  for (index = 0; index < MAX_ENGINES; index++) {
      char name[MAX_NAME_LEN];
      snprintf(name, MAX_NAME_LEN, 
         "/controls/electric/engine[%d]/generator", index);  
      fgTie(name, this, index,
	&FGControls::get_generator_breaker, 
        &FGControls::set_generator_breaker);
      fgSetArchivable(name);

      snprintf(name, MAX_NAME_LEN,
               "/controls/electric/engine[%d]/bus-tie", index);  
      fgTie(name, this, index,
	&FGControls::get_bus_tie, 
        &FGControls::set_bus_tie);
      fgSetArchivable(name);
  }  

  // pneumatic
  fgTie("/controls/pneumatic/APU-bleed", this,
	&FGControls::get_APU_bleed, 
        &FGControls::set_APU_bleed);
  fgSetArchivable("/controls/pneumatic/APU-bleed");

  for (index = 0; index < MAX_ENGINES; index++) {
      char name[MAX_NAME_LEN];
      snprintf(name, MAX_NAME_LEN, 
         "/controls/pneumatic/engine[%d]/bleed", index);  
      fgTie(name, this, index,
	&FGControls::get_engine_bleed, 
        &FGControls::set_engine_bleed);
      fgSetArchivable(name);
  }

  // pressurization
  fgTie("/controls/pressurization/mode", this,
	&FGControls::get_mode, &FGControls::set_mode);
  fgSetArchivable("/controls/pressurization/mode");

  fgTie("/controls/pressurization/dump", this,
	&FGControls::get_dump, &FGControls::set_dump);
  fgSetArchivable("/controls/pressurization/dump");

  fgTie("/controls/pressurization/outflow-valve", this,
	&FGControls::get_outflow_valve, 
        &FGControls::set_outflow_valve);
  fgSetArchivable("/controls/pressurization/outflow-valve");

  for (index = 0; index < MAX_PACKS; index++) {
      char name[MAX_NAME_LEN];
      snprintf(name, MAX_NAME_LEN,
               "/controls/pressurization/pack[%d]/pack-on", index);  
      fgTie(name, this, index,
	&FGControls::get_pack_on, &FGControls::set_pack_on);
      fgSetArchivable(name);
  }
 
  // lights
  fgTie("/controls/lighting/landing-lights", this,
	&FGControls::get_landing_lights, 
        &FGControls::set_landing_lights);
  fgSetArchivable("/controls/lighting/landing-lights");  

  fgTie("/controls/lighting/turn-off-lights", this,
	&FGControls::get_turn_off_lights,
        &FGControls::set_turn_off_lights);
  fgSetArchivable("/controls/lighting/turn-off-lights");
  
  fgTie("/controls/lighting/taxi-light", this,
	&FGControls::get_taxi_light, &FGControls::set_taxi_light);
  fgSetArchivable("/controls/lighting/taxi-light");
  
  fgTie("/controls/lighting/logo-lights", this,
	&FGControls::get_logo_lights, &FGControls::set_logo_lights);
  fgSetArchivable("/controls/lighting/logo-lights");
  
  fgTie("/controls/lighting/nav-lights", this,
	&FGControls::get_nav_lights, &FGControls::set_nav_lights);
  fgSetArchivable("/controls/lighting/nav-lights");  

  fgTie("/controls/lighting/beacon", this,
	&FGControls::get_beacon, &FGControls::set_beacon);
  fgSetArchivable("/controls/lighting/beacon");
  
  fgTie("/controls/lighting/strobe", this,
	&FGControls::get_strobe, &FGControls::set_strobe);
  fgSetArchivable("/controls/lighting/strobe");  

  fgTie("/controls/lighting/panel-norm", this,
	&FGControls::get_panel_norm, &FGControls::set_panel_norm);
  fgSetArchivable("/controls/lighting/panel-norm");
  
  fgTie("/controls/lighting/instruments-norm", this,
	&FGControls::get_instruments_norm, 
        &FGControls::set_instruments_norm);
  fgSetArchivable("/controls/lighting/instruments-norm");  

  fgTie("/controls/lighting/dome-norm", this,
	&FGControls::get_dome_norm, &FGControls::set_dome_norm);
  fgSetArchivable("/controls/lighting/dome-norm"); 
 
  // armament
  fgTie("/controls/armament/master-arm", this,
	&FGControls::get_master_arm, &FGControls::set_master_arm);
  fgSetArchivable("/controls/armament/master-arm");  

  fgTie("/controls/armament/station-select", this,
	&FGControls::get_station_select, 
        &FGControls::set_station_select);
  fgSetArchivable("/controls/armament/station-select");  

  fgTie("/controls/armament/release-all", this,
	&FGControls::get_release_ALL, 
        &FGControls::set_release_ALL);
  fgSetArchivable("/controls/armament/release-all");  

  for (index = 0; index < MAX_STATIONS; index++) {
      char name[MAX_NAME_LEN];
      snprintf(name, MAX_NAME_LEN,
               "/controls/armament/station[%d]/stick-size", index);  
      fgTie(name, this, index,
	&FGControls::get_stick_size, &FGControls::set_stick_size);
      fgSetArchivable(name);

      snprintf(name, MAX_NAME_LEN, 
          "/controls/armament/station[%d]/release-stick", index);  
      fgTie(name, this, index,
	&FGControls::get_release_stick, &FGControls::set_release_stick);
      fgSetArchivable(name);

      snprintf(name, MAX_NAME_LEN,
               "/controls/armament/station[%d]/release-all", index);  
      fgTie(name, this, index,
	&FGControls::get_release_all, &FGControls::set_release_all);
      fgSetArchivable(name);

      snprintf(name, MAX_NAME_LEN,
               "/controls/armament/station[%d]/jettison-all", index);  
      fgTie(name, this, index,
	&FGControls::get_jettison_all, &FGControls::set_jettison_all);
      fgSetArchivable(name);
  }

  // seat
  fgTie("/controls/seat/vertical-adjust", this,
	&FGControls::get_vertical_adjust, 
        &FGControls::set_vertical_adjust);
  fgSetArchivable("/controls/seat/vertical-adjust");

  fgTie("/controls/seat/fore-aft-adjust", this,
	&FGControls::get_fore_aft_adjust, 
        &FGControls::set_fore_aft_adjust);
  fgSetArchivable("/controls/seat/fore-aft-adjust");
  
  for (index = 0; index < MAX_EJECTION_SEATS; index++) {
      char name[MAX_NAME_LEN];
      snprintf(name, MAX_NAME_LEN,
	       "/controls/seat/eject[%d]/initiate", index);
      fgTie(name, this, index,
	   &FGControls::get_ejection_seat, 
	   &FGControls::set_ejection_seat);
      fgSetArchivable(name);

      snprintf(name, MAX_NAME_LEN,
	       "/controls/seat/eject[%d]/status", index);

      fgTie(name, this, index,
           &FGControls::get_eseat_status,
	   &FGControls::set_eseat_status);

      fgSetArchivable(name);
  }
  
  fgTie("/controls/seat/cmd_selector_valve", this,
        &FGControls::get_cmd_selector_valve,
	&FGControls::set_cmd_selector_valve);
  fgSetArchivable("/controls/seat/eject/cmd_selector_valve");


  // APU
  fgTie("/controls/APU/off-start-run", this,
	&FGControls::get_off_start_run, 
        &FGControls::set_off_start_run);
  fgSetArchivable("/controls/APU/off-start-run");

  fgTie("/controls/APU/fire-switch", this,
	&FGControls::get_APU_fire_switch, 
        &FGControls::set_APU_fire_switch);
  fgSetArchivable("/controls/APU/fire-switch");

  // autoflight
  for (index = 0; index < MAX_AUTOPILOTS; index++) {
      char name[MAX_NAME_LEN];
      snprintf(name, MAX_NAME_LEN, 
         "/controls/autoflight/autopilot[%d]/engage", index);  
      fgTie(name, this, index,
	&FGControls::get_autopilot_engage, 
        &FGControls::set_autopilot_engage);
      fgSetArchivable(name);
  }
 
  fgTie("/controls/autoflight/autothrottle-arm", this,
	&FGControls::get_autothrottle_arm, 
        &FGControls::set_autothrottle_arm);
  fgSetArchivable("/controls/autoflight/autothrottle-arm");

  fgTie("/controls/autoflight/autothrottle-engage", this,
	&FGControls::get_autothrottle_engage, 
        &FGControls::set_autothrottle_engage);
  fgSetArchivable("/controls/autoflight/autothrottle-engage");

  fgTie("/controls/autoflight/heading-select", this,
	&FGControls::get_heading_select, 
        &FGControls::set_heading_select);
  fgSetArchivable("/controls/autoflight/heading-select");

  fgTie("/controls/autoflight/altitude-select", this,
	&FGControls::get_altitude_select, 
        &FGControls::set_altitude_select);
  fgSetArchivable("/controls/autoflight/altitude-select");

  fgTie("/controls/autoflight/bank-angle-select", this,
	&FGControls::get_bank_angle_select, 
        &FGControls::set_bank_angle_select);
  fgSetArchivable("/controls/autoflight/bank-angle-select");

  fgTie("/controls/autoflight/vertical-speed-select", this,
	&FGControls::get_vertical_speed_select, 
        &FGControls::set_vertical_speed_select);
  fgSetArchivable("/controls/autoflight/vertical-speed-select");

  fgTie("/controls/autoflight/speed-select", this,
	&FGControls::get_speed_select, 
        &FGControls::set_speed_select);
  fgSetArchivable("/controls/autoflight/speed-select");

  fgTie("/controls/autoflight/mach-select", this,
	&FGControls::get_mach_select, 
        &FGControls::set_mach_select);
  fgSetArchivable("/controls/autoflight/mach-select");

  fgTie("/controls/autoflight/vertical-mode", this,
	&FGControls::get_vertical_mode, 
        &FGControls::set_vertical_mode);
  fgSetArchivable("/controls/autoflight/vertical-mode");

  fgTie("/controls/autoflight/lateral-mode", this,
	&FGControls::get_lateral_mode, 
        &FGControls::set_lateral_mode);
  fgSetArchivable("/controls/autoflight/lateral-mode");

}

void FGControls::unbind ()
{
  int index, i;
  //Tie control properties.
  fgUntie("/controls/flight/aileron");
  fgUntie("/controls/flight/aileron-trim");
  fgUntie("/controls/flight/elevator");
  fgUntie("/controls/flight/elevator-trim");
  fgUntie("/controls/flight/rudder");
  fgUntie("/controls/flight/rudder-trim");
  fgUntie("/controls/flight/flaps");
  fgUntie("/controls/flight/slats");
  fgUntie("/controls/flight/BLC");  
  fgUntie("/controls/flight/spoilers");  
  fgUntie("/controls/flight/speedbrake");  
  fgUntie("/controls/flight/wing-sweep");  
  fgUntie("/controls/flight/wing-fold");  
  fgUntie("/controls/flight/drag-chute");
  for (index = 0; index < MAX_ENGINES; index++) {
    char name[MAX_NAME_LEN];
    snprintf(name, MAX_NAME_LEN,
             "/controls/engines/engine[%d]/throttle", index);
    fgUntie(name);
    snprintf(name, MAX_NAME_LEN,
             "/controls/engines/engine[%d]/feed_tank", index);
    fgUntie(name);
    snprintf(name, MAX_NAME_LEN,
             "/controls/engines/engine[%d]/starter", index);
    fgUntie(name);
    snprintf(name, MAX_NAME_LEN,
             "/controls/engines/engine[%d]/fuel-pump", index);
    fgUntie(name);
    snprintf(name, MAX_NAME_LEN,
             "/controls/engines/engine[%d]/fire-switch", index);
    fgUntie(name);
    snprintf(name, MAX_NAME_LEN, 
             "/controls/engines/engine[%d]/fire-bottle-discharge", index);
    fgUntie(name);
    snprintf(name, MAX_NAME_LEN,
             "/controls/engines/engine[%d]/throttle_idle", index);
    fgUntie(name);
    snprintf(name, MAX_NAME_LEN, "/controls/engines/engine[%d]/cutoff", index);
    fgUntie(name);
    snprintf(name, MAX_NAME_LEN, "/controls/engines/engine[%d]/mixture", index);
    fgUntie(name);
    snprintf(name, MAX_NAME_LEN, 
             "/controls/engines/engine[%d]/propeller-pitch", index);
    fgUntie(name);
    snprintf(name, MAX_NAME_LEN,
             "/controls/engines/engine[%d]/magnetos", index);
    fgUntie(name);
    snprintf(name, MAX_NAME_LEN, "/controls/engines/engine[%d]/WEP", index);
    fgUntie(name);
    snprintf(name, MAX_NAME_LEN,
             "/controls/engines/engine[%d]/cowl-flaps-norm", index);
    fgUntie(name);
    snprintf(name, MAX_NAME_LEN,
             "/controls/engines/engine[%d]/propeller-feather", index);
    fgUntie(name);
    snprintf(name, MAX_NAME_LEN,
             "/controls/engines/engine[%d]/ignition", index);
    fgUntie(name);
    snprintf(name, MAX_NAME_LEN,
             "/controls/engines/engine[%d]/augmentation", index);
    fgUntie(name);
    snprintf(name, MAX_NAME_LEN,
             "/controls/engines/engine[%d]/reverser", index);
    fgUntie(name);
    snprintf(name, MAX_NAME_LEN,
             "/controls/engines/engine[%d]/water-injection", index);
    fgUntie(name);
    snprintf(name, MAX_NAME_LEN,
             "/controls/engines/engine[%d]/condition", index);
    fgUntie(name);
  }
  fgUntie("/controls/fuel/dump-valve");
  for (index = 0; index < MAX_TANKS; index++) {
    char name[MAX_NAME_LEN];
    snprintf(name, MAX_NAME_LEN,
             "/controls/fuel/tank[%d]/fuel_selector", index);
    fgUntie(name);
    snprintf(name, MAX_NAME_LEN, "/controls/fuel/tank[%d]/to_engine", index);
    fgUntie(name);
    snprintf(name, MAX_NAME_LEN, "/controls/fuel/tank[%d]/to_tank", index);
    fgUntie(name);
    for (i = 0; index < MAX_BOOSTPUMPS; i++) {
      snprintf(name, MAX_NAME_LEN,
               "/controls/fuel/tank[%d]/boost-pump[%d]", index, i);
      fgUntie(name);
    }
  }
  fgUntie("/controls/gear/brake-left");
  fgUntie("/controls/gear/brake-right");
  fgUntie("/controls/gear/brake-parking");
  fgUntie("/controls/gear/steering");
  fgUntie("/controls/gear/nose-wheel-steering");
  fgUntie("/controls/gear/gear-down");
  fgUntie("/controls/gear/antiskid");
  fgUntie("/controls/gear/tailhook");
  fgUntie("/controls/gear/launchbar");
  fgUntie("/controls/gear/catapult-launch-cmd");
  fgUntie("/controls/gear/tailwheel-lock");
  for (index = 0; index < MAX_WHEELS; index++) {
    char name[MAX_NAME_LEN];
    snprintf(name, MAX_NAME_LEN, 
       "/controls/gear/wheel[%d]/alternate-extension", index);
    fgUntie(name);
  }
  fgUntie("/controls/anti-ice/wing-heat");
  fgUntie("/controls/anti-ice/pitot-heat");
  fgUntie("/controls/anti-ice/wiper");
  fgUntie("/controls/anti-ice/window-heat");
  for (index = 0; index < MAX_ENGINES; index++) {
    char name[MAX_NAME_LEN];
    snprintf(name, MAX_NAME_LEN,
             "/controls/anti-ice/engine[%d]/carb-heat", index);
    fgUntie(name);
    snprintf(name, MAX_NAME_LEN,
             "/controls/anti-ice/engine[%d]/inlet-heat", index);
    fgUntie(name);
  }
  for (index = 0; index < MAX_HYD_SYSTEMS; index++) {
    char name[MAX_NAME_LEN];
    snprintf(name, MAX_NAME_LEN, 
       "/controls/hydraulic/system[%d]/engine-pump", index);
    fgUntie(name);
    snprintf(name, MAX_NAME_LEN, 
       "/controls/hydraulic/system[%d]/electric-pump", index);
    fgUntie(name);
  }
  fgUntie("/controls/electric/battery-switch");
  fgUntie("/controls/electric/external-power");
  fgUntie("/controls/electric/APU-generator");    
  for (index = 0; index < MAX_ENGINES; index++) {
    char name[MAX_NAME_LEN];
     snprintf(name, MAX_NAME_LEN, 
       "/controls/electric/engine[%d]/generator", index);
    fgUntie(name);
    snprintf(name, MAX_NAME_LEN, 
       "/controls/electric/engine[%d]/bus-tie", index);
    fgUntie(name);
  }
  fgUntie("/controls/pneumatic/APU-bleed");
  for (index = 0; index < MAX_ENGINES; index++) {
    char name[MAX_NAME_LEN];
     snprintf(name, MAX_NAME_LEN, 
       "/controls/pneumatic/engine[%d]/bleed", index);
    fgUntie(name);
  }
  fgUntie("/controls/pressurization/mode");
  fgUntie("/controls/pressurization/dump");
  for (index = 0; index < MAX_PACKS; index++) {
    char name[MAX_NAME_LEN];
    snprintf(name, MAX_NAME_LEN, 
       "/controls/pressurization/pack[%d]/pack-on", index);
    fgUntie(name);
  }
  fgUntie("/controls/lighting/landing-lights");  
  fgUntie("/controls/lighting/turn-off-lights");  
  fgUntie("/controls/lighting/taxi-light");  
  fgUntie("/controls/lighting/logo-lights");  
  fgUntie("/controls/lighting/nav-lights");  
  fgUntie("/controls/lighting/beacon");  
  fgUntie("/controls/lighting/strobe");  
  fgUntie("/controls/lighting/panel-norm");  
  fgUntie("/controls/lighting/instruments-norm");  
  fgUntie("/controls/lighting/dome-norm");

  fgUntie("/controls/armament/master-arm");  
  fgUntie("/controls/armament/station-select");  
  fgUntie("/controls/armament/release-all");  
  for (index = 0; index < MAX_STATIONS; index++) {
    char name[MAX_NAME_LEN];
    snprintf(name, MAX_NAME_LEN, 
       "/controls/armament/station[%d]/stick-size", index);
    fgUntie(name);
    snprintf(name, MAX_NAME_LEN, 
       "/controls/armament/station[%d]/release-stick", index);
    fgUntie(name);
    snprintf(name, MAX_NAME_LEN, 
       "/controls/armament/station[%d]/release-all", index);
    fgUntie(name);
    snprintf(name, MAX_NAME_LEN, 
       "/controls/armament/station[%d]/jettison-all", index);
    fgUntie(name);
  }

  fgUntie("/controls/seat/vertical-adjust");  
  fgUntie("/controls/seat/fore-aft-adjust");  
  for (index = 0; index < MAX_EJECTION_SEATS; index++) {
    char name[MAX_NAME_LEN];
    snprintf(name, MAX_NAME_LEN,
       "/controls/seat/eject[%d]/initiate", index);
    fgUntie(name);
    snprintf(name, MAX_NAME_LEN,
       "/controls/seat/eject[%d]/status", index);
    fgUntie(name);
  }
  fgUntie("/controls/seat/cmd_selector_valve");
  
  fgUntie("/controls/APU/off-start-run");  
  fgUntie("/controls/APU/fire-switch");  
  for (index = 0; index < MAX_AUTOPILOTS; index++) {
    char name[MAX_NAME_LEN];
    snprintf(name, MAX_NAME_LEN,
       "/controls/autoflight/autopilot[%d]/engage", index);
    fgUntie(name);
  }
  fgUntie("/controls/autoflight/autothrottle-arm");  
  fgUntie("/controls/autoflight/autothrottle-engage");  
  fgUntie("/controls/autoflight/heading-select");  
  fgUntie("/controls/autoflight/altitude-select");  
  fgUntie("/controls/autoflight/bank-angle-select");  
  fgUntie("/controls/autoflight/vertical-speed-select");  
  fgUntie("/controls/autoflight/speed-select");  
  fgUntie("/controls/autoflight/mach-select");  
  fgUntie("/controls/autoflight/vertical-mode");  
  fgUntie("/controls/autoflight/lateral-mode");  
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
  CLAMP( &aileron, -1.0, 1.0 );
			
  // check for autocoordination
  if ( auto_coordination->getBoolValue() ) {
    set_rudder( aileron / 2.0 );
  }
}

void
FGControls::move_aileron (double amt)
{
  aileron += amt;
  CLAMP( &aileron, -1.0, 1.0 );
			
  // check for autocoordination
  if ( auto_coordination->getBoolValue() ) {
    set_rudder( aileron / 2.0 );
  }
}

void
FGControls::set_aileron_trim( double pos )
{
    aileron_trim = pos;
    CLAMP( &aileron_trim, -1.0, 1.0 );
}

void
FGControls::move_aileron_trim( double amt )
{
    aileron_trim += amt;
    CLAMP( &aileron_trim, -1.0, 1.0 );
}

void
FGControls::set_elevator( double pos )
{
    elevator = pos;
    CLAMP( &elevator, -1.0, 1.0 );
}

void
FGControls::move_elevator( double amt )
{
    elevator += amt;
    CLAMP( &elevator, -1.0, 1.0 );
}

void
FGControls::set_elevator_trim( double pos )
{
    elevator_trim = pos;
    CLAMP( &elevator_trim, -1.0, 1.0 );
}

void
FGControls::move_elevator_trim( double amt )
{
    elevator_trim += amt;
    CLAMP( &elevator_trim, -1.0, 1.0 );
}

void
FGControls::set_rudder( double pos )
{
    rudder = pos;
    CLAMP( &rudder, -1.0, 1.0 );
}

void
FGControls::move_rudder( double amt )
{
    rudder += amt;
    CLAMP( &rudder, -1.0, 1.0 );
}

void
FGControls::set_rudder_trim( double pos )
{
    rudder_trim = pos;
    CLAMP( &rudder_trim, -1.0, 1.0 );
}

void
FGControls::move_rudder_trim( double amt )
{
    rudder_trim += amt;
    CLAMP( &rudder_trim, -1.0, 1.0 );
}

void
FGControls::set_flaps( double pos )
{
    flaps = pos;
    CLAMP( &flaps, 0.0, 1.0 );
}

void
FGControls::move_flaps( double amt )
{
    flaps += amt;
    CLAMP( &flaps, 0.0, 1.0 );
}

void
FGControls::set_slats( double pos )
{
    slats = pos;
    CLAMP( &slats, 0.0, 1.0 );
}

void
FGControls::move_slats( double amt )
{
    slats += amt;
    CLAMP( &slats, 0.0, 1.0 );
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
    CLAMP( &spoilers, 0.0, 1.0 );
}

void
FGControls::move_spoilers( double amt )
{
    spoilers += amt;
    CLAMP( &spoilers, 0.0, 1.0 );
}

void
FGControls::set_speedbrake( double pos )
{
    speedbrake = pos;
    CLAMP( &speedbrake, 0.0, 1.0 );
}

void
FGControls::move_speedbrake( double amt )
{
    speedbrake += amt;
    CLAMP( &speedbrake, 0.0, 1.0 );
}

void
FGControls::set_wing_sweep( double pos )
{
    wing_sweep = pos;
    CLAMP( &wing_sweep, 0.0, 1.0 );
}

void
FGControls::move_wing_sweep( double amt )
{
    wing_sweep += amt;
    CLAMP( &wing_sweep, 0.0, 1.0 );
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
      CLAMP( &throttle[i], 0.0, 1.0 );
    }
  } else {
    if ( (engine >= 0) && (engine < MAX_ENGINES) ) {
      throttle[engine] = pos;
      CLAMP( &throttle[engine], 0.0, 1.0 );
    }
  }
}

void
FGControls::move_throttle( int engine, double amt )
{
    if ( engine == ALL_ENGINES ) {
	for ( int i = 0; i < MAX_ENGINES; i++ ) {
	    throttle[i] += amt;
	    CLAMP( &throttle[i], 0.0, 1.0 );
	}
    } else {
	if ( (engine >= 0) && (engine < MAX_ENGINES) ) {
	    throttle[engine] += amt;
	    CLAMP( &throttle[engine], 0.0, 1.0 );
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
	    CLAMP( &feed_tank[i], -1, 4 );
	}
    } else {
	if ( (engine >= 0) && (engine < MAX_ENGINES) ) {
	    feed_tank[engine] = tank;
	    CLAMP( &feed_tank[engine], -1, 4 );
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
	    CLAMP( &mixture[i], 0.0, 1.0 );
	}
    } else {
	if ( (engine >= 0) && (engine < MAX_ENGINES) ) {
	    mixture[engine] = pos;
	    CLAMP( &mixture[engine], 0.0, 1.0 );
	}
    }
}

void
FGControls::move_mixture( int engine, double amt )
{
    if ( engine == ALL_ENGINES ) {
	for ( int i = 0; i < MAX_ENGINES; i++ ) {
	    mixture[i] += amt;
	    CLAMP( &mixture[i], 0.0, 1.0 );
	}
    } else {
	if ( (engine >= 0) && (engine < MAX_ENGINES) ) {
	    mixture[engine] += amt;
	    CLAMP( &mixture[engine], 0.0, 1.0 );
	}
    }
}

void
FGControls::set_prop_advance( int engine, double pos )
{
    if ( engine == ALL_ENGINES ) {
	for ( int i = 0; i < MAX_ENGINES; i++ ) {
	    prop_advance[i] = pos;
	    CLAMP( &prop_advance[i], 0.0, 1.0 );
	}
    } else {
	if ( (engine >= 0) && (engine < MAX_ENGINES) ) {
	    prop_advance[engine] = pos;
	    CLAMP( &prop_advance[engine], 0.0, 1.0 );
	}
    }
}

void
FGControls::move_prop_advance( int engine, double amt )
{
    if ( engine == ALL_ENGINES ) {
	for ( int i = 0; i < MAX_ENGINES; i++ ) {
	    prop_advance[i] += amt;
	    CLAMP( &prop_advance[i], 0.0, 1.0 );
	}
    } else {
	if ( (engine >= 0) && (engine < MAX_ENGINES) ) {
	    prop_advance[engine] += amt;
	    CLAMP( &prop_advance[engine], 0.0, 1.0 );
	}
    }
}

void
FGControls::set_magnetos( int engine, int pos )
{
    if ( engine == ALL_ENGINES ) {
	for ( int i = 0; i < MAX_ENGINES; i++ ) {
	    magnetos[i] = pos;
	    CLAMP( &magnetos[i], 0, 3 );
	}
    } else {
	if ( (engine >= 0) && (engine < MAX_ENGINES) ) {
	    magnetos[engine] = pos;
	    CLAMP( &magnetos[engine], 0, 3 );
	}
    }
}

void
FGControls::move_magnetos( int engine, int amt )
{
    if ( engine == ALL_ENGINES ) {
	for ( int i = 0; i < MAX_ENGINES; i++ ) {
	    magnetos[i] += amt;
	    CLAMP( &magnetos[i], 0, 3 );
	}
    } else {
	if ( (engine >= 0) && (engine < MAX_ENGINES) ) {
	    magnetos[engine] += amt;
	    CLAMP( &magnetos[engine], 0, 3 );
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
	    CLAMP( &cowl_flaps_norm[i], 0.0, 1.0 );
	}
    } else {
	if ( (engine >= 0) && (engine < MAX_ENGINES) ) {
	    cowl_flaps_norm[engine] = pos;
	    CLAMP( &cowl_flaps_norm[engine], 0.0, 1.0 );
	}
    }
}

void
FGControls::move_cowl_flaps_norm( int engine, double amt )
{
    if ( engine == ALL_ENGINES ) {
	for ( int i = 0; i < MAX_ENGINES; i++ ) {
	    cowl_flaps_norm[i] += amt;
	    CLAMP( &cowl_flaps_norm[i], 0.0, 1.0 );
	}
    } else {
	if ( (engine >= 0) && (engine < MAX_ENGINES) ) {
	    cowl_flaps_norm[engine] += amt;
	    CLAMP( &cowl_flaps_norm[engine], 0.0, 1.0 );
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
	    CLAMP( &ignition[i], 0, 3 );
	}
    } else {
	if ( (engine >= 0) && (engine < MAX_ENGINES) ) {
	    ignition[engine] = pos;
	    CLAMP( &ignition[engine], 0, 3 );
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
	    CLAMP( &condition[i], 0.0, 1.0 );
	}
    } else {
	if ( (engine >= 0) && (engine < MAX_ENGINES) ) {
	    condition[engine] = val;
	    CLAMP( &condition[engine], 0.0, 1.0 );
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
    CLAMP(&brake_left, 0.0, 1.0);
}

void
FGControls::move_brake_left( double amt )
{
    brake_left += amt;
    CLAMP( &brake_left, 0.0, 1.0 );
}

void
FGControls::set_brake_right( double pos )
{
    brake_right = pos;
    CLAMP(&brake_right, 0.0, 1.0);
}

void
FGControls::move_brake_right( double amt )
{
    brake_right += amt;
    CLAMP( &brake_right, 0.0, 1.0 );
}

void
FGControls::set_copilot_brake_left( double pos )
{
    copilot_brake_left = pos;
    CLAMP(&brake_left, 0.0, 1.0);
}

void
FGControls::set_copilot_brake_right( double pos )
{
    copilot_brake_right = pos;
    CLAMP(&brake_right, 0.0, 1.0);
}

void
FGControls::set_brake_parking( double pos )
{
    brake_parking = pos;
    CLAMP(&brake_parking, 0.0, 1.0);
}

void
FGControls::set_steering( double angle )
{
    steering = angle;
    CLAMP(&steering, -80.0, 80.0);
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
    CLAMP(&steering, -80.0, 80.0);
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
  CLAMP( &outflow_valve, 0.0, 1.0 );
}

void
FGControls::move_outflow_valve( double amt )
{
  outflow_valve += amt;
  CLAMP( &outflow_valve, 0.0, 1.0 );
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
  CLAMP( &panel_norm, 0.0, 1.0 );
}

void
FGControls::move_panel_norm( double amt )
{
  panel_norm += amt;
  CLAMP( &panel_norm, 0.0, 1.0 );
}

void
FGControls::set_instruments_norm( double intensity )
{
  instruments_norm = intensity;
  CLAMP( &instruments_norm, 0.0, 1.0 );
}

void
FGControls::move_instruments_norm( double amt )
{
  instruments_norm += amt;
  CLAMP( &instruments_norm, 0.0, 1.0 );
}

void
FGControls::set_dome_norm( double intensity )
{
  dome_norm = intensity;
  CLAMP( &dome_norm, 0.0, 1.0 );
}

void
FGControls::move_dome_norm( double amt )
{
  dome_norm += amt;
  CLAMP( &dome_norm, 0.0, 1.0 );
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
  CLAMP( &station_select, 0, MAX_STATIONS );
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
            CLAMP( &stick_size[i], 1, 20 );
	}
    } else {
	if ( (station >= 0) && (station < MAX_STATIONS) ) {
	    stick_size[station] = size;
            CLAMP( &stick_size[station], 1, 20 );
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
  CLAMP( &vertical_adjust, -1.0, 1.0 );
}

void
FGControls::move_vertical_adjust( double amt )
{
  vertical_adjust += amt;
  CLAMP( &vertical_adjust, -1.0, 1.0 );
}

void
FGControls::set_fore_aft_adjust( double pos )
{
  fore_aft_adjust = pos;
  CLAMP( &fore_aft_adjust, -1.0, 1.0 );
}

void
FGControls::move_fore_aft_adjust( double amt )
{
  fore_aft_adjust += amt;
  CLAMP( &fore_aft_adjust, -1.0, 1.0 );
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
  CLAMP( &off_start_run, 0, 3 );
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
  CLAMP( &heading_select, 0.0, 360.0 );
}

void
FGControls::move_heading_select( double amt )
{
  heading_select += amt;
  CLAMP( &heading_select, 0.0, 360.0 );
}

void
FGControls::set_altitude_select( double altitude )
{
  altitude_select = altitude;
  CLAMP( &altitude_select, -1000.0, 100000.0 );
}

void
FGControls::move_altitude_select( double amt )
{
  altitude_select += amt;
  CLAMP( &altitude_select, -1000.0, 100000.0 );
}

void
FGControls::set_bank_angle_select( double angle )
{
  bank_angle_select = angle;
  CLAMP( &bank_angle_select, 10.0, 30.0 );
}

void
FGControls::move_bank_angle_select( double amt )
{
  bank_angle_select += amt;
  CLAMP( &bank_angle_select, 10.0, 30.0 );
}

void
FGControls::set_vertical_speed_select( double speed )
{
  vertical_speed_select = speed;
  CLAMP( &vertical_speed_select, -3000.0, 4000.0 );
}

void
FGControls::move_vertical_speed_select( double amt )
{
  vertical_speed_select += amt;
  CLAMP( &vertical_speed_select, -3000.0, 4000.0 );
}

void
FGControls::set_speed_select( double speed )
{
  speed_select = speed;
  CLAMP( &speed_select, 60.0, 400.0 );
}

void
FGControls::move_speed_select( double amt )
{
  speed_select += amt;
  CLAMP( &speed_select, 60.0, 400.0 );
}

void
FGControls::set_mach_select( double mach )
{
  mach_select = mach;
  CLAMP( &mach_select, 0.4, 4.0 );
}

void
FGControls::move_mach_select( double amt )
{
  mach_select += amt;
  CLAMP( &mach_select, 0.4, 4.0 );
}

void
FGControls::set_vertical_mode( int mode )
{
  vertical_mode = mode;
  CLAMP( &vertical_mode, 0, 4 );
}

void
FGControls::set_lateral_mode( int mode )
{
  lateral_mode = mode;
  CLAMP( &lateral_mode, 0, 4 );
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
