// bfi.cxx - Big Friendly Interface implementation
//
// Written by David Megginson, started February, 2000.
//
// Copyright (C) 2000  David Megginson - david@megginson.com
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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#if defined( FG_HAVE_NATIVE_SGI_COMPILERS )
#  include <iostream.h>
#else
#  include <iostream>
#endif

#include <simgear/constants.h>
#include <simgear/math/fg_types.hxx>

#include <Aircraft/aircraft.hxx>
#include <Controls/controls.hxx>
#include <Autopilot/autopilot.hxx>
#include <Time/fg_time.hxx>
#include <Time/light.hxx>
#include <Cockpit/radiostack.hxx>
#ifndef FG_OLD_WEATHER
#  include <WeatherCM/FGLocalWeatherDatabase.h>
#else
#  include <Weather/weather.hxx>
#endif

#include "options.hxx"
#include "save.hxx"
#include "fg_init.hxx"

FG_USING_NAMESPACE(std);

				// FIXME: these are not part of the
				// published interface!!!
extern fgAPDataPtr APDataGlobal;
extern void fgAPAltitudeSet (double new_altitude);
extern void fgAPHeadingSet (double new_heading);


#include "bfi.hxx"



////////////////////////////////////////////////////////////////////////
// Static variables.
////////////////////////////////////////////////////////////////////////

bool FGBFI::_needReinit = false;



////////////////////////////////////////////////////////////////////////
// Local functions
////////////////////////////////////////////////////////////////////////


/**
 * Reinitialize FGFS if required.
 *
 * Some changes (especially those in aircraft position) require that
 * FGFS be reinitialized afterwards.  Rather than reinitialize after
 * every change, the setter methods simply set a flag so that there
 * can be a single reinit at the end of the frame.
 */
void
FGBFI::update ()
{
  if (_needReinit) {
    reinit();
  }
}


/**
 * Reinitialize FGFS to use the new BFI settings.
 */
void
FGBFI::reinit ()
{
				// Save the state of everything
				// that's going to get clobbered
				// when we reinit the subsystems.

				// TODO: add more AP stuff
  double elevator = getElevator();
  double aileron = getAileron();
  double rudder = getRudder();
  double throttle = getThrottle();
  double elevator_trim = getElevatorTrim();
  double flaps = getFlaps();
  double brake = getBrake();
  bool apHeadingLock = getAPHeadingLock();
  double apHeading = getAPHeading();
  bool apAltitudeLock = getAPAltitudeLock();
  double apAltitude = getAPAltitude();
  const string &targetAirport = getTargetAirport();
  bool gpsLock = getGPSLock();
  double gpsLatitude = getGPSTargetLatitude();
  double gpsLongitude = getGPSTargetLongitude();

  fgReInitSubsystems();
  // solarSystemRebuild();
  cur_light_params.Update();

				// Restore all of the old states.
  setElevator(elevator);
  setAileron(aileron);
  setRudder(rudder);
  setThrottle(throttle);
  setElevatorTrim(elevator_trim);
  setFlaps(flaps);
  setBrake(brake);
  setAPHeadingLock(apHeadingLock);
  setAPHeading(apHeading);
  setAPAltitudeLock(apAltitudeLock);
  setAPAltitude(apAltitude);
  setTargetAirport(targetAirport);
  setGPSLock(gpsLock);
  setGPSTargetLatitude(gpsLatitude);
  setGPSTargetLongitude(gpsLongitude);

  _needReinit = false;
}



////////////////////////////////////////////////////////////////////////
// Simulation.
////////////////////////////////////////////////////////////////////////


/**
 * Return the flight model as an integer.
 *
 * TODO: use a string instead.
 */
int
FGBFI::getFlightModel ()
{
  return current_options.get_flight_model();
}


/**
 * Set the flight model as an integer.
 *
 * TODO: use a string instead.
 */
void
FGBFI::setFlightModel (int model)
{
  current_options.set_flight_model(model);
  needReinit();
}


/**
 * Return the current Zulu time.
 */
time_t
FGBFI::getTimeGMT ()
{
				// FIXME: inefficient
  return mktime(FGTime::cur_time_params->getGmt());
}


/**
 * Set the current Zulu time.
 */
void
FGBFI::setTimeGMT (time_t time)
{
				// FIXME: need to update lighting
				// and solar system
  current_options.set_time_offset(time);
  current_options.set_time_offset_type(fgOPTIONS::FG_TIME_GMT_ABSOLUTE);
  FGTime::cur_time_params->init( cur_fdm_state->get_Longitude(),
				 cur_fdm_state->get_Latitude() );
  FGTime::cur_time_params->update( cur_fdm_state->get_Longitude(),
				   cur_fdm_state->get_Latitude(),
				   cur_fdm_state->get_Altitude()
				   * FEET_TO_METER );
  needReinit();
}


/**
 * Return true if the HUD is visible.
 */
bool
FGBFI::getHUDVisible ()
{
  return current_options.get_hud_status();
}


/**
 * Ensure that the HUD is visible or hidden.
 */
void
FGBFI::setHUDVisible (bool visible)
{
  current_options.set_hud_status(visible);
}


/**
 * Return true if the 2D panel is visible.
 */
bool
FGBFI::getPanelVisible ()
{
  return current_options.get_panel_status();
}


/**
 * Ensure that the 2D panel is visible or hidden.
 */
void
FGBFI::setPanelVisible (bool visible)
{
  if (current_options.get_panel_status() != visible) {
    current_options.toggle_panel();
  }
}



////////////////////////////////////////////////////////////////////////
// Position
////////////////////////////////////////////////////////////////////////


/**
 * Return the current latitude in degrees (negative for south).
 */
double
FGBFI::getLatitude ()
{
  return current_aircraft.fdm_state->get_Latitude() * RAD_TO_DEG;
}


/**
 * Set the current latitude in degrees (negative for south).
 */
void
FGBFI::setLatitude (double latitude)
{
  current_options.set_lat(latitude);
  needReinit();
}


/**
 * Return the current longitude in degrees (negative for west).
 */
double
FGBFI::getLongitude ()
{
  return current_aircraft.fdm_state->get_Longitude() * RAD_TO_DEG;
}


/**
 * Set the current longitude in degrees (negative for west).
 */
void
FGBFI::setLongitude (double longitude)
{
  current_options.set_lon(longitude);
  needReinit();
}


/**
 * Return the current altitude in feet.
 */
double
FGBFI::getAltitude ()
{
  return current_aircraft.fdm_state->get_Altitude();
}


/**
 * Set the current altitude in feet.
 */
void
FGBFI::setAltitude (double altitude)
{
  current_options.set_altitude(altitude * FEET_TO_METER);
  needReinit();
}



////////////////////////////////////////////////////////////////////////
// Attitude
////////////////////////////////////////////////////////////////////////


/**
 * Return the current heading in degrees.
 */
double
FGBFI::getHeading ()
{
  return current_aircraft.fdm_state->get_Psi() * RAD_TO_DEG;
}


/**
 * Set the current heading in degrees.
 */
void
FGBFI::setHeading (double heading)
{
  current_options.set_heading(heading);
  needReinit();
}


/**
 * Return the current pitch in degrees.
 */
double
FGBFI::getPitch ()
{
  return current_aircraft.fdm_state->get_Theta() * RAD_TO_DEG;
}


/**
 * Set the current pitch in degrees.
 */
void
FGBFI::setPitch (double pitch)
{

  current_options.set_pitch(pitch);
  needReinit();
}


/**
 * Return the current roll in degrees.
 */
double
FGBFI::getRoll ()
{
  return current_aircraft.fdm_state->get_Phi() * RAD_TO_DEG;
}


/**
 * Set the current roll in degrees.
 */
void
FGBFI::setRoll (double roll)
{
  current_options.set_roll(roll);
  needReinit();
}



////////////////////////////////////////////////////////////////////////
// Velocities
////////////////////////////////////////////////////////////////////////


/**
 * Return the current airspeed in knots.
 */
double
FGBFI::getAirspeed ()
{
				// FIXME: should we add speed-up?
  return current_aircraft.fdm_state->get_V_calibrated_kts();
}


/**
 * Return the current sideslip (FIXME: units unknown).
 */
double
FGBFI::getSideSlip ()
{
  return current_aircraft.fdm_state->get_Beta();
}


/**
 * Return the current climb rate in feet/second (FIXME: verify).
 */
double
FGBFI::getVerticalSpeed ()
{
				// What about meters?
  return current_aircraft.fdm_state->get_Climb_Rate() * 60.0;
}


/**
 * Get the current north velocity (units??).
 */
double
FGBFI::getSpeedNorth ()
{
  return current_aircraft.fdm_state->get_V_north();
}


/**
 * Set the current north velocity (units??).
 */
void
FGBFI::setSpeedNorth (double speed)
{
  current_options.set_uBody(speed);
  needReinit();
}


/**
 * Get the current east velocity (units??).
 */
double
FGBFI::getSpeedEast ()
{
  return current_aircraft.fdm_state->get_V_east();
}


/**
 * Set the current east velocity (units??).
 */
void
FGBFI::setSpeedEast (double speed)
{
  current_options.set_vBody(speed);
  needReinit();
}


/**
 * Get the current down velocity (units??).
 */
double
FGBFI::getSpeedDown ()
{
  return current_aircraft.fdm_state->get_V_down();
}


/**
 * Set the current down velocity (units??).
 */
void
FGBFI::setSpeedDown (double speed)
{
  current_options.set_wBody(speed);
  needReinit();
}



////////////////////////////////////////////////////////////////////////
// Controls
////////////////////////////////////////////////////////////////////////


/**
 * Get the throttle setting, from 0.0 (none) to 1.0 (full).
 */
double
FGBFI::getThrottle ()
{
				// FIXME: add throttle selector
  return controls.get_throttle(0);
}


/**
 * Set the throttle, from 0.0 (none) to 1.0 (full).
 */
void
FGBFI::setThrottle (double throttle)
{
				// FIXME: allow throttle selection
				// FIXME: clamp?
  controls.set_throttle(0, throttle);
}


/**
 * Get the flaps setting, from 0.0 (none) to 1.0 (full).
 */
double
FGBFI::getFlaps ()
{
  return controls.get_flaps();
}


/**
 * Set the flaps, from 0.0 (none) to 1.0 (full).
 */
void
FGBFI::setFlaps (double flaps)
{
				// FIXME: clamp?
  controls.set_flaps(flaps);
}


/**
 * Get the aileron, from -1.0 (left) to 1.0 (right).
 */
double
FGBFI::getAileron ()
{
  return controls.get_aileron();
}


/**
 * Set the aileron, from -1.0 (left) to 1.0 (right).
 */
void
FGBFI::setAileron (double aileron)
{
				// FIXME: clamp?
  controls.set_aileron(aileron);
}


/**
 * Get the rudder setting, from -1.0 (left) to 1.0 (right).
 */
double
FGBFI::getRudder ()
{
  return controls.get_rudder();
}


/**
 * Set the rudder, from -1.0 (left) to 1.0 (right).
 */
void
FGBFI::setRudder (double rudder)
{
				// FIXME: clamp?
  controls.set_rudder(rudder);
}


/**
 * Get the elevator setting, from -1.0 (down) to 1.0 (up).
 */
double
FGBFI::getElevator ()
{
  return controls.get_elevator();
}


/**
 * Set the elevator, from -1.0 (down) to 1.0 (up).
 */
void
FGBFI::setElevator (double elevator)
{
				// FIXME: clamp?
  controls.set_elevator(elevator);
}


/**
 * Get the elevator trim, from -1.0 (down) to 1.0 (up).
 */
double
FGBFI::getElevatorTrim ()
{
  return controls.get_elevator_trim();
}


/**
 * Set the elevator trim, from -1.0 (down) to 1.0 (up).
 */
void
FGBFI::setElevatorTrim (double trim)
{
				// FIXME: clamp?
  controls.set_elevator_trim(trim);
}


/**
 * Get the brake setting, from 0.0 (none) to 1.0 (full).
 */
double
FGBFI::getBrake ()
{
				// FIXME: add brake selector
  return controls.get_brake(0);
}


/**
 * Set the brake, from 0.0 (none) to 1.0 (full).
 */
void
FGBFI::setBrake (double brake)
{
				// FIXME: clamp?
				// FIXME: allow brake selection
  controls.set_brake(0, brake);
}



////////////////////////////////////////////////////////////////////////
// Autopilot
////////////////////////////////////////////////////////////////////////


/**
 * Get the autopilot altitude lock (true=on).
 */
bool
FGBFI::getAPAltitudeLock ()
{
  return fgAPAltitudeEnabled();
}


/**
 * Set the autopilot altitude lock (true=on).
 */
void
FGBFI::setAPAltitudeLock (bool lock)
{
  APDataGlobal->altitude_hold = lock;
}


/**
 * Get the autopilot target altitude in feet.
 */
double
FGBFI::getAPAltitude ()
{
  return fgAPget_TargetAltitude() * METER_TO_FEET;
}


/**
 * Set the autopilot target altitude in feet.
 */
void
FGBFI::setAPAltitude (double altitude)
{
  fgAPAltitudeSet(altitude);
}


/**
 * Get the autopilot heading lock (true=on).
 */
bool
FGBFI::getAPHeadingLock ()
{
  return fgAPHeadingEnabled();
}


/**
 * Set the autopilot heading lock (true=on).
 */
void
FGBFI::setAPHeadingLock (bool lock)
{
  APDataGlobal->heading_hold = lock;
}


/**
 * Get the autopilot target heading in degrees.
 */
double
FGBFI::getAPHeading ()
{
  return fgAPget_TargetHeading();
}


/**
 * Set the autopilot target heading in degrees.
 */
void
FGBFI::setAPHeading (double heading)
{
  fgAPHeadingSet(heading);
}



////////////////////////////////////////////////////////////////////////
// Radio navigation.
////////////////////////////////////////////////////////////////////////

double
FGBFI::getNAV1Freq ()
{
  return current_radiostack->get_nav1_freq();
}

double
FGBFI::getNAV1AltFreq ()
{
  return current_radiostack->get_nav1_alt_freq();
}

double
FGBFI::getNAV1SelRadial ()
{
  return current_radiostack->get_nav1_sel_radial();
}

double
FGBFI::getNAV1Radial ()
{
  return current_radiostack->get_nav1_radial();
}

double
FGBFI::getNAV2Freq ()
{
  return current_radiostack->get_nav2_freq();
}

double
FGBFI::getNAV2AltFreq ()
{
  return current_radiostack->get_nav2_alt_freq();
}

double
FGBFI::getNAV2SelRadial ()
{
  return current_radiostack->get_nav2_sel_radial();
}

double
FGBFI::getNAV2Radial ()
{
  return current_radiostack->get_nav2_radial();
}

double
FGBFI::getADFFreq ()
{
  return current_radiostack->get_adf_freq();
}

double
FGBFI::getADFAltFreq ()
{
  return current_radiostack->get_adf_alt_freq();
}

double
FGBFI::getADFRotation ()
{
  return current_radiostack->get_adf_rotation();
}

void
FGBFI::setNAV1Freq (double freq)
{
  current_radiostack->set_nav1_freq(freq);
}

void
FGBFI::setNAV1AltFreq (double freq)
{
  current_radiostack->set_nav1_alt_freq(freq);
}

void
FGBFI::setNAV1SelRadial (double radial)
{
  current_radiostack->set_nav1_sel_radial(radial);
}

void
FGBFI::setNAV2Freq (double freq)
{
  current_radiostack->set_nav2_freq(freq);
}

void
FGBFI::setNAV2AltFreq (double freq)
{
  current_radiostack->set_nav2_alt_freq(freq);
}

void
FGBFI::setNAV2SelRadial (double radial)
{
  current_radiostack->set_nav2_sel_radial(radial);
}

void
FGBFI::setADFFreq (double freq)
{
  current_radiostack->set_adf_freq(freq);
}

void
FGBFI::setADFAltFreq (double freq)
{
  current_radiostack->set_adf_alt_freq(freq);
}

void
FGBFI::setADFRotation (double rot)
{
  current_radiostack->set_adf_rotation(rot);
}



////////////////////////////////////////////////////////////////////////
// GPS
////////////////////////////////////////////////////////////////////////


/**
 * Get the autopilot GPS lock (true=on).
 */
bool
FGBFI::getGPSLock ()
{
  return fgAPWayPointEnabled();
}


/**
 * Set the autopilot GPS lock (true=on).
 */
void
FGBFI::setGPSLock (bool lock)
{
  APDataGlobal->waypoint_hold = lock;
}


/**
 * Get the GPS target airport code.
 */
const string
FGBFI::getTargetAirport ()
{
  return current_options.get_airport_id();
}


/**
 * Set the GPS target airport code.
 */
void
FGBFI::setTargetAirport (const string &airportId)
{
  current_options.set_airport_id(airportId);
}


/**
 * Get the GPS target latitude in degrees (negative for south).
 */
double
FGBFI::getGPSTargetLatitude ()
{
  return fgAPget_TargetLatitude();
}


/**
 * Set the GPS target latitude in degrees (negative for south).
 */
void
FGBFI::setGPSTargetLatitude (double latitude)
{
  APDataGlobal->TargetLatitude = latitude;
}


/**
 * Get the GPS target longitude in degrees (negative for west).
 */
double
FGBFI::getGPSTargetLongitude ()
{
  return fgAPget_TargetLongitude();
}


/**
 * Set the GPS target longitude in degrees (negative for west).
 */
void
FGBFI::setGPSTargetLongitude (double longitude)
{
  APDataGlobal->TargetLongitude = longitude;
}



////////////////////////////////////////////////////////////////////////
// Weather
////////////////////////////////////////////////////////////////////////


/**
 * Get the current visible (units??).
 */
double
FGBFI::getVisibility ()
{
#ifndef FG_OLD_WEATHER
  return WeatherDatabase->getWeatherVisibility();
#else
  return current_weather.get_visibility();
#endif
}


/**
 * Set the current visibility (units??).
 */
void
FGBFI::setVisibility (double visibility)
{
#ifndef FG_OLD_WEATHER
  WeatherDatabase->setWeatherVisibility(visibility);
#else
  current_weather.set_visibility(visibility);
#endif
}



////////////////////////////////////////////////////////////////////////
// Time
////////////////////////////////////////////////////////////////////////

/**
 * Return the magnetic variation
 */
double
FGBFI::getMagVar ()
{
  return FGTime::cur_time_params->getMagVar() * RAD_TO_DEG;
}


/**
 * Return the magnetic variation
 */
double
FGBFI::getMagDip ()
{
  return FGTime::cur_time_params->getMagDip() * RAD_TO_DEG;
}


// end of bfi.cxx

