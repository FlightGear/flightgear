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
#include <simgear/debug/logstream.hxx>
#include <simgear/ephemeris/ephemeris.hxx>
#include <simgear/math/fg_types.hxx>
#include <simgear/misc/props.hxx>
#include <simgear/timing/sg_time.hxx>

#include <Aircraft/aircraft.hxx>
#include <FDM/UIUCModel/uiuc_aircraftdir.h>
#include <Controls/controls.hxx>
#include <Autopilot/newauto.hxx>
#include <Scenery/scenery.hxx>
#include <Time/light.hxx>
#include <Time/event.hxx>
#include <Time/sunpos.hxx>
#include <Time/tmp.hxx>
#include <Cockpit/radiostack.hxx>
#ifndef FG_OLD_WEATHER
#  include <WeatherCM/FGLocalWeatherDatabase.h>
#else
#  include <Weather/weather.hxx>
#endif

#include "globals.hxx"
#include "options.hxx"
#include "save.hxx"
#include "fg_init.hxx"

FG_USING_NAMESPACE(std);


#include "bfi.hxx"



////////////////////////////////////////////////////////////////////////
// Static variables.
////////////////////////////////////////////////////////////////////////

bool FGBFI::_needReinit = false;



////////////////////////////////////////////////////////////////////////
// Local functions
////////////////////////////////////////////////////////////////////////


/**
 * Initialize the BFI by binding its functions to properties.
 *
 * TODO: perhaps these should migrate into the individual modules
 * (i.e. they should register themselves).
 */
void
FGBFI::init ()
{
  FG_LOG(FG_GENERAL, FG_INFO, "Starting BFI init");
				// Simulation
  current_properties.tieInt("/sim/flight-model",
			    getFlightModel, setFlightModel);
//   current_properties.tieString("/sim/aircraft",
// 			       getAircraft, setAircraft);
  // TODO: timeGMT
  current_properties.tieBool("/sim/hud/visibility",
			     getHUDVisible, setHUDVisible);
  current_properties.tieBool("/sim/panel/visibility",
			     getPanelVisible, setPanelVisible);

				// Position
  current_properties.tieDouble("/position/latitude",
				getLatitude, setLatitude);
  current_properties.tieDouble("/position/longitude",
			       getLongitude, setLongitude);
  current_properties.tieDouble("/position/altitude",
			       getAltitude, setAltitude);
  current_properties.tieDouble("/position/altitude-agl",
			       getAGL, 0);

				// Orientation
  current_properties.tieDouble("/orientation/heading",
			       getHeading, setHeading);
  current_properties.tieDouble("/orientation/heading-magnetic",
			       getHeadingMag, 0);
  current_properties.tieDouble("/orientation/pitch",
			       getPitch, setPitch);
  current_properties.tieDouble("/orientation/roll",
			       getRoll, setRoll);

				// Velocities
  current_properties.tieDouble("/velocities/airspeed",
			       getAirspeed, 0);
  current_properties.tieDouble("/velocities/side-slip",
			       getSideSlip, 0);
  current_properties.tieDouble("/velocities/vertical-speed",
			       getVerticalSpeed, 0);
  current_properties.tieDouble("/velocities/speed-north",
			       getSpeedNorth, setSpeedNorth);
  current_properties.tieDouble("/velocities/speed-east",
			       getSpeedEast, setSpeedEast);
  current_properties.tieDouble("/velocities/speed-down",
			       getSpeedDown, setSpeedDown);

				// Controls
  current_properties.tieDouble("/controls/throttle",
			       getThrottle, setThrottle);
  current_properties.tieDouble("/controls/flaps",
			       getFlaps, setFlaps);
  current_properties.tieBool  ("/controls/flaps/raise",
 			       0, setFlapsRaise);
  current_properties.tieBool  ("/controls/flaps/lower",
 			       0, setFlapsLower);
  current_properties.tieDouble("/controls/aileron",
			       getAileron, setAileron);
  current_properties.tieDouble("/controls/rudder",
			       getRudder, setRudder);
  current_properties.tieDouble("/controls/elevator",
			       getElevator, setElevator);
  current_properties.tieDouble("/controls/elevator-trim",
			       getElevatorTrim, setElevatorTrim);
  current_properties.tieDouble("/controls/brake",
			       getBrake, setBrake);
  current_properties.tieDouble("/controls/left-brake",
			       getLeftBrake, setLeftBrake);
  current_properties.tieDouble("/controls/right-brake",
			       getRightBrake, setRightBrake);

				// Autopilot
  current_properties.tieBool("/autopilot/locks/altitude",
			     getAPAltitudeLock, setAPAltitudeLock);
  current_properties.tieDouble("/autopilot/settings/altitude",
			       getAPAltitude, setAPAltitude);
  current_properties.tieBool("/autopilot/locks/heading",
			     getAPHeadingLock, setAPHeadingLock);
  current_properties.tieDouble("/autopilot/settings/heading-magnetic",
			       getAPHeadingMag, setAPHeadingMag);
  current_properties.tieBool("/autopilot/locks/nav1",
			     getAPNAV1Lock, setAPNAV1Lock);

				// Radio navigation
  current_properties.tieDouble("/radios/nav1/frequencies/selected",
			       getNAV1Freq, setNAV1Freq);
  current_properties.tieDouble("/radios/nav1/frequencies/standby",
			       getNAV1AltFreq, setNAV1AltFreq);
  current_properties.tieDouble("/radios/nav1/radials/actual",
			       getNAV1Radial, 0);
  current_properties.tieDouble("/radios/nav1/radials/selected",
			       getNAV1SelRadial, setNAV1SelRadial);
  current_properties.tieDouble("/radios/nav1/dme/distance",
			       getNAV1DistDME, 0);
  current_properties.tieBool("/radios/nav1/in-range",
			     getNAV1InRange, 0);
  current_properties.tieBool("/radios/nav1/dme/in-range",
			     getNAV1DMEInRange, 0);
			       
  current_properties.tieDouble("/radios/nav2/frequencies/selected",
			       getNAV2Freq, setNAV2Freq);
  current_properties.tieDouble("/radios/nav2/frequencies/standby",
			       getNAV2AltFreq, setNAV2AltFreq);
  current_properties.tieDouble("/radios/nav2/radials/actual",
			       getNAV2Radial, 0);
  current_properties.tieDouble("/radios/nav2/radials/selected",
			       getNAV2SelRadial, setNAV2SelRadial);
  current_properties.tieDouble("/radios/nav2/dme/distance",
			       getNAV2DistDME, 0);
  current_properties.tieBool("/radios/nav2/in-range",
			     getNAV2InRange, 0);
  current_properties.tieBool("/radios/nav2/dme/in-range",
			     getNAV2DMEInRange, 0);

  current_properties.tieDouble("/radios/adf/frequencies/selected",
			       getADFFreq, setADFFreq);
  current_properties.tieDouble("/radios/adf/frequencies/standby",
			       getADFAltFreq, setADFAltFreq);
  current_properties.tieDouble("/radios/adf/rotation",
			       getADFRotation, setADFRotation);

  FG_LOG(FG_GENERAL, FG_INFO, "Ending BFI init");
}


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

  cout << "BFI: start reinit\n";

  setHeading(getHeading());
  setPitch(getPitch());
  setRoll(getRoll());
  setSpeedNorth(getSpeedNorth());
  setSpeedEast(getSpeedEast());
  setSpeedDown(getSpeedDown());
  setLatitude(getLatitude());
  setLongitude(getLongitude());
  setAltitude(getAltitude());

				// TODO: add more AP stuff
  double elevator = getElevator();
  double aileron = getAileron();
  double rudder = getRudder();
  double throttle = getThrottle();
  double elevator_trim = getElevatorTrim();
  double flaps = getFlaps();
  double brake = getBrake();
  bool apHeadingLock = getAPHeadingLock();
  double apHeadingMag = getAPHeadingMag();
  bool apAltitudeLock = getAPAltitudeLock();
  double apAltitude = getAPAltitude();
  const string &targetAirport = getTargetAirport();
  bool gpsLock = getGPSLock();
  double gpsLatitude = getGPSTargetLatitude();
  double gpsLongitude = getGPSTargetLongitude();

  setTargetAirport("");
  cout << "Target airport is " << current_options.get_airport_id() << endl;

  fgReInitSubsystems();

				// FIXME: this is wrong.
				// All of these are scheduled events,
				// and it should be possible to force
				// them all to run once.
  fgUpdateSunPos();
  fgUpdateMoonPos();
  cur_light_params.Update();
  fgUpdateLocalTime();
  fgUpdateWeatherDatabase();
  fgRadioSearch();

				// Restore all of the old states.
  setElevator(elevator);
  setAileron(aileron);
  setRudder(rudder);
  setThrottle(throttle);
  setElevatorTrim(elevator_trim);
  setFlaps(flaps);
  setBrake(brake);
  setAPHeadingLock(apHeadingLock);
  setAPHeadingMag(apHeadingMag);
  setAPAltitudeLock(apAltitudeLock);
  setAPAltitude(apAltitude);
  setTargetAirport(targetAirport);
  setGPSLock(gpsLock);
  setGPSTargetLatitude(gpsLatitude);
  setGPSTargetLongitude(gpsLongitude);

  _needReinit = false;

  cout << "BFI: end reinit\n";
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
 * Return the current aircraft as a string.
 */
const string
FGBFI::getAircraft ()
{
  return current_options.get_aircraft();
}


/**
 * Return the current aircraft directory (UIUC) as a string.
 */
const string
FGBFI::getAircraftDir ()
{
  return aircraft_dir;
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
 * Set the current aircraft.
 */
void
FGBFI::setAircraft (const string &aircraft)
{
  current_options.set_aircraft(aircraft);
  needReinit();
}


/**
 * Set the current aircraft directory (UIUC).
 */
void
FGBFI::setAircraftDir (const string &dir)
{
  aircraft_dir = dir;
  needReinit();
}


/**
 * Return the current Zulu time.
 */
time_t
FGBFI::getTimeGMT ()
{
  return globals->get_time_params()->get_cur_time();
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
  globals->get_time_params()->update( cur_fdm_state->get_Longitude(),
				      cur_fdm_state->get_Latitude(),
				      globals->get_warp() );
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
  current_aircraft.fdm_state->set_Latitude(latitude * DEG_TO_RAD);
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
  current_aircraft.fdm_state->set_Longitude(longitude * DEG_TO_RAD);
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
 * Return the current altitude in above the terrain.
 */
double
FGBFI::getAGL ()
{
  return current_aircraft.fdm_state->get_Altitude()
	 - (scenery.cur_elev * METER_TO_FEET);
}


/**
 * Set the current altitude in feet.
 */
void
FGBFI::setAltitude (double altitude)
{
  fgFDMForceAltitude(getFlightModel(), altitude * FEET_TO_METER);
//   current_options.set_altitude(altitude * FEET_TO_METER);
//   current_aircraft.fdm_state->set_Altitude(altitude);
//   needReinit();
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
 * Return the current heading in degrees.
 */
double
FGBFI::getHeadingMag ()
{
  return current_aircraft.fdm_state->get_Psi() * RAD_TO_DEG - getMagVar();
}


/**
 * Set the current heading in degrees.
 */
void
FGBFI::setHeading (double heading)
{
  current_options.set_heading(heading);
  current_aircraft.fdm_state->set_Euler_Angles(getRoll() * DEG_TO_RAD,
					       getPitch() * DEG_TO_RAD,
					       heading * DEG_TO_RAD);
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
  current_aircraft.fdm_state->set_Euler_Angles(getRoll() * DEG_TO_RAD,
					       pitch * DEG_TO_RAD,
					       getHeading() * DEG_TO_RAD);
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
  current_aircraft.fdm_state->set_Euler_Angles(roll * DEG_TO_RAD,
					       getPitch() * DEG_TO_RAD,
					       getHeading() * DEG_TO_RAD);
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
 * Return the current climb rate in feet/minute
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
  current_aircraft.fdm_state->set_Velocities_Local(speed,
						   getSpeedEast(),
						   getSpeedDown());
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
  current_aircraft.fdm_state->set_Velocities_Local(getSpeedNorth(),
						   speed,
						   getSpeedDown());
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
  current_aircraft.fdm_state->set_Velocities_Local(getSpeedNorth(),
						   getSpeedEast(),
						   speed);
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


void
FGBFI::setFlapsRaise (bool step)
{
    if (step)
	controls.set_flaps(controls.get_flaps() - 0.26);
    printf ( "Raise: %i\n", step );
}


void
FGBFI::setFlapsLower (bool step)
{
    if (step)
	controls.set_flaps(controls.get_flaps() + 0.26);
    printf ( "Lower: %i\n", step );
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
  return controls.get_brake(2);
}


/**
 * Set the brake, from 0.0 (none) to 1.0 (full).
 */
void
FGBFI::setBrake (double brake)
{
  controls.set_brake(FGControls::ALL_WHEELS, brake);
}


/**
 * Get the left brake, from 0.0 (none) to 1.0 (full).
 */
double
FGBFI::getLeftBrake ()
{
  return controls.get_brake(0);
}


/**
 * Set the left brake, from 0.0 (none) to 1.0 (full).
 */
void
FGBFI::setLeftBrake (double brake)
{
  controls.set_brake(0, brake);
}


/**
 * Get the right brake, from 0.0 (none) to 1.0 (full).
 */
double
FGBFI::getRightBrake ()
{
  return controls.get_brake(1);
}


/**
 * Set the right brake, from 0.0 (none) to 1.0 (full).
 */
void
FGBFI::setRightBrake (double brake)
{
  controls.set_brake(1, brake);
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
    return current_autopilot->get_AltitudeEnabled();
}


/**
 * Set the autopilot altitude lock (true=on).
 */
void
FGBFI::setAPAltitudeLock (bool lock)
{
  current_autopilot->set_AltitudeMode(FGAutopilot::FG_ALTITUDE_LOCK);
  current_autopilot->set_AltitudeEnabled(lock);
}


/**
 * Get the autopilot target altitude in feet.
 */
double
FGBFI::getAPAltitude ()
{
  return current_autopilot->get_TargetAltitude() * METER_TO_FEET;
}


/**
 * Set the autopilot target altitude in feet.
 */
void
FGBFI::setAPAltitude (double altitude)
{
    current_autopilot->set_TargetAltitude( altitude );
}


/**
 * Get the autopilot heading lock (true=on).
 */
bool
FGBFI::getAPHeadingLock ()
{
    return
      (current_autopilot->get_HeadingEnabled() &&
       current_autopilot->get_HeadingMode() == FGAutopilot::FG_HEADING_LOCK);
}


/**
 * Set the autopilot heading lock (true=on).
 */
void
FGBFI::setAPHeadingLock (bool lock)
{
  if (lock) {
				// We need to do this so that
				// it's possible to lock onto a
				// heading other than the current
				// heading.
    double heading = getAPHeadingMag();
    current_autopilot->set_HeadingMode(FGAutopilot::FG_HEADING_LOCK);
    current_autopilot->set_HeadingEnabled(true);
    setAPHeadingMag(heading);
  } else if (current_autopilot->get_HeadingMode() ==
	     FGAutopilot::FG_HEADING_LOCK) {
    current_autopilot->set_HeadingEnabled(false);
  }
}


/**
 * Get the autopilot target heading in degrees.
 */
double
FGBFI::getAPHeadingMag ()
{
  return current_autopilot->get_TargetHeading() - getMagVar();
}


/**
 * Set the autopilot target heading in degrees.
 */
void
FGBFI::setAPHeadingMag (double heading)
{
  current_autopilot->set_TargetHeading( heading + getMagVar() );
}


/**
 * Return true if the autopilot is locked to NAV1.
 */
bool
FGBFI::getAPNAV1Lock ()
{
  return
    (current_autopilot->get_HeadingEnabled() &&
     current_autopilot->get_HeadingMode() == FGAutopilot::FG_HEADING_NAV1);
}


/**
 * Set the autopilot NAV1 lock.
 */
void
FGBFI::setAPNAV1Lock (bool lock)
{
  if (lock) {
    current_autopilot->set_HeadingMode(FGAutopilot::FG_HEADING_NAV1);
    current_autopilot->set_HeadingEnabled(true);
  } else if (current_autopilot->get_HeadingMode() ==
	     FGAutopilot::FG_HEADING_NAV1) {
    current_autopilot->set_HeadingEnabled(false);
  }
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
FGBFI::getNAV1Radial ()
{
  return current_radiostack->get_nav1_radial();
}

double
FGBFI::getNAV1SelRadial ()
{
  return current_radiostack->get_nav1_sel_radial();
}

double
FGBFI::getNAV1DistDME ()
{
  return current_radiostack->get_nav1_dme_dist();
}

bool
FGBFI::getNAV1InRange ()
{
  return current_radiostack->get_nav1_inrange();
}

bool
FGBFI::getNAV1DMEInRange ()
{
  return (current_radiostack->get_nav1_inrange() &&
	  current_radiostack->get_nav1_has_dme());
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
FGBFI::getNAV2Radial ()
{
  return current_radiostack->get_nav2_radial();
}

double
FGBFI::getNAV2SelRadial ()
{
  return current_radiostack->get_nav2_sel_radial();
}

double
FGBFI::getNAV2DistDME ()
{
  return current_radiostack->get_nav2_dme_dist();
}

bool
FGBFI::getNAV2InRange ()
{
  return current_radiostack->get_nav2_inrange();
}

bool
FGBFI::getNAV2DMEInRange ()
{
  return (current_radiostack->get_nav2_inrange() &&
	  current_radiostack->get_nav2_has_dme());
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
  return (current_autopilot->get_HeadingEnabled() &&
	  (current_autopilot->get_HeadingMode() ==
	   FGAutopilot::FG_HEADING_WAYPOINT ));
}


/**
 * Set the autopilot GPS lock (true=on).
 */
void
FGBFI::setGPSLock (bool lock)
{
  if (lock) {
    current_autopilot->set_HeadingMode(FGAutopilot::FG_HEADING_WAYPOINT);
    current_autopilot->set_HeadingEnabled(true);
  } else if (current_autopilot->get_HeadingMode() ==
	     FGAutopilot::FG_HEADING_WAYPOINT) {
    current_autopilot->set_HeadingEnabled(false);
  }
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
    return current_autopilot->get_TargetLatitude();
}


/**
 * Set the GPS target latitude in degrees (negative for south).
 */
void
FGBFI::setGPSTargetLatitude (double latitude)
{
  current_autopilot->set_TargetLatitude( latitude );
}


/**
 * Get the GPS target longitude in degrees (negative for west).
 */
double
FGBFI::getGPSTargetLongitude ()
{
  return current_autopilot->get_TargetLongitude();
}


/**
 * Set the GPS target longitude in degrees (negative for west).
 */
void
FGBFI::setGPSTargetLongitude (double longitude)
{
  current_autopilot->set_TargetLongitude( longitude );
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
 * Check whether clouds are enabled.
 */
bool
FGBFI::getClouds ()
{
  return current_options.get_clouds();
}


/**
 * Check the height of the clouds ASL (units?).
 */
double
FGBFI::getCloudsASL ()
{
  return current_options.get_clouds_asl();
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


/**
 * Switch clouds on or off.
 */
void
FGBFI::setClouds (bool clouds)
{
  cout << "Set clouds to " << clouds << endl;
  current_options.set_clouds(clouds);
  needReinit();
}


/**
 * Set the cloud height.
 */
void
FGBFI::setCloudsASL (double cloudsASL)
{
  current_options.set_clouds_asl(cloudsASL);
  needReinit();
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
  return cur_magvar.get_magvar() * RAD_TO_DEG;
}


/**
 * Return the magnetic variation
 */
double
FGBFI::getMagDip ()
{
  return cur_magvar.get_magdip() * RAD_TO_DEG;
}


// end of bfi.cxx

