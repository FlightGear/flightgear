// save.cxx -- class to save and restore a flight.
//
// Written by Curtis Olson, started November 1999.
//
// Copyright (C) 1999  David Megginson - david@megginson.com
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


/*
CHANGES:
- time is now working
- autopilot is working (even with GPS)

TODO:
- use a separate options object so that we can roll back on error
- use proper FGFS logging
- add view direction, and other stuff
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <iostream>

#include <simgear/fg_types.hxx>
#include <simgear/constants.h>

#include <Aircraft/aircraft.hxx>
#include <Controls/controls.hxx>
#include <Autopilot/autopilot.hxx>
#include <Time/fg_time.hxx>
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

#define SAVE(name, value) { output << name << ": " << value << endl; }

/**
 * Save the current state of the simulator to a stream.
 */
bool
fgSaveFlight (ostream &output)
{
  char tb[100];
  const FGInterface * f = current_aircraft.fdm_state;
  struct tm *t = FGTime::cur_time_params->getGmt();

  output << "#!fgfs" << endl;

  //
  // Simulation
  //
  SAVE("flight-model", current_options.get_flight_model());
  SAVE("time", mktime(t));
  SAVE("hud", current_options.get_hud_status());
  SAVE("panel", current_options.get_panel_status());

  // 
  // Location
  //
  SAVE("latitude", (f->get_Latitude() * RAD_TO_DEG));
  SAVE("longitude", (f->get_Longitude() * RAD_TO_DEG));
  SAVE("altitude", f->get_Altitude());

  //
  // Orientation
  //
  SAVE("heading", (f->get_Psi() * RAD_TO_DEG));
  SAVE("pitch", (f->get_Theta() * RAD_TO_DEG));
  SAVE("roll", (f->get_Phi() * RAD_TO_DEG));

  //
  // Velocities
  //
  SAVE("speed-north", f->get_V_north());
  SAVE("speed-east", f->get_V_east());
  SAVE("speed-down", f->get_V_down());

  //
  // Primary controls
  //
  SAVE("elevator", controls.get_elevator());
  SAVE("aileron", controls.get_aileron());
  SAVE("rudder", controls.get_rudder());
				// FIXME: save each throttle separately
  SAVE("throttle", controls.get_throttle(0));

  //
  // Secondary controls
  //
  SAVE("elevator-trim", controls.get_elevator_trim());
  SAVE("flaps", controls.get_flaps());
				// FIXME: save each brake separately
  SAVE("brake", controls.get_brake(0));

  //
  // Navigation.
  //
  if (current_options.get_airport_id().length() > 0) {
    SAVE("target-airport", current_options.get_airport_id());
  }
  SAVE("autopilot-altitude-lock", fgAPAltitudeEnabled());
  SAVE("autopilot-altitude", fgAPget_TargetAltitude() * METER_TO_FEET);
  SAVE("autopilot-heading-lock", fgAPHeadingEnabled());
  SAVE("autopilot-heading", fgAPget_TargetHeading());
  SAVE("autopilot-gps-lock", fgAPWayPointEnabled());
  SAVE("autopilot-gps-lat", fgAPget_TargetLatitude());
  SAVE("autopilot-gps-lon", fgAPget_TargetLongitude());

  //
  // Environment.
  //
#ifndef FG_OLD_WEATHER
  SAVE("visibility", WeatherDatabase->getWeatherVisibility());
#else
  SAVE("visibility", current_weather.get_visibility());
#endif

  return true;
}


/**
 * Restore the current state of the simulator from a stream.
 */
bool
fgLoadFlight (istream &input)
{
  FGInterface * f = current_aircraft.fdm_state;
  string text;
  double n;
  long int i;

  double elevator = controls.get_elevator();
  double aileron = controls.get_aileron();
  double rudder = controls.get_rudder();
  double throttle = controls.get_throttle(0);
  double elevator_trim = controls.get_elevator_trim();
  double flaps = controls.get_flaps();
  double brake =  controls.get_brake(FGControls::ALL_WHEELS);

  bool ap_heading_lock = false;
  double ap_heading = 0;
  bool ap_altitude_lock = false;
  double ap_altitude = 0;
  bool ap_gps_lock = false;
  double ap_gps_lat = 0;
  double ap_gps_lon = 0;

  string airport_id = current_options.get_airport_id();
  current_options.set_airport_id("");
  current_options.set_time_offset(0);
  current_options.set_time_offset_type(fgOPTIONS::FG_TIME_GMT_OFFSET);

  if (!input.good() || input.eof()) {
    cout << "Stream is no good!\n";
    return false;
  }

  input >> text;
  if (text != "#!fgfs") {
    printf("Bad save file format!\n");
    return false;
  }


  while (input.good() && !input.eof()) {
    input >> text;

    //
    // Simulation.
    //

    if (text == "flight-model:") {
      input >> i;
      cout << "flight model is " << i << endl;
      current_options.set_flight_model(i);
    }

    else if (text == "time:") {
      input >> i;
      cout << "saved time is " << i << endl;
      current_options.set_time_offset(i);
      current_options.set_time_offset_type(fgOPTIONS::FG_TIME_GMT_ABSOLUTE);
      FGTime::cur_time_params->init(*cur_fdm_state);
      FGTime::cur_time_params->update(*cur_fdm_state);
    }

    else if (text == "hud:") {
      input >> i;
      cout << "hud status is " << i << endl;
      current_options.set_hud_status(i);
    }

    else if (text == "panel:") {
      input >> i;
      cout << "panel status is " << i << endl;
      if (current_options.get_panel_status() != i) {
	current_options.toggle_panel();
      }
    }

    //
    // Location
    //
      
    else if (text == "latitude:") {
      input >> n;
      cout << "latitude is " << n << endl;
      current_options.set_lat(n);
    } else if (text == "longitude:") {
      input >> n;
      cout << "longitude is " << n << endl;
      current_options.set_lon(n);
    } else if (text == "altitude:") {
      input >> n;
      cout << "altitude is " << n << endl;
      current_options.set_altitude(n * FEET_TO_METER);
    } 

    //
    // Orientation
    //

    else if (text == "heading:") {
      input >> n;
      cout << "heading is " << n << endl;
      current_options.set_heading(n);
    } else if (text == "pitch:") {
      input >> n;
      cout << "pitch is " << n << endl;
      current_options.set_pitch(n);
    } else if (text == "roll:") {
      input >> n;
      cout << "roll is " << n << endl;
      current_options.set_roll(n);
    } 

    //
    // Velocities
    //

    else if (text == "speed-north:") {
      input >> n;
      cout << "speed north is " << n << endl;
      current_options.set_uBody(n);
    } else if (text == "speed-east:") {
      input >> n;
      cout << "speed east is " << n << endl;
      current_options.set_vBody(n);
    } else if (text == "speed-down:") {
      input >> n;
      cout << "speed down is " << n << endl;
      current_options.set_wBody(n);
    } 

    //
    // Primary controls
    //

    else if (text == "elevator:") {
      input >> elevator;
      cout << "elevator is " << elevator << endl;
    }

    else if (text == "aileron:") {
      input >> aileron;
      cout << "aileron is " << aileron << endl;
    }

    else if (text == "rudder:") {
      input >> rudder;
      cout << "rudder is " << rudder << endl;
    }

				// FIXME: assumes single engine
    else if (text == "throttle:") {
      input >> throttle;
      cout << "throttle is " << throttle << endl;
    }

    //
    // Secondary controls

    else if (text == "elevator-trim:") {
      input >> elevator_trim;
      cout << "elevator trim is " << elevator_trim << endl;
    }

    else if (text == "flaps:") {
      input >> flaps;
      cout << "flaps are " << flaps << endl;
    }

    else if (text == "brake:") {
      input >> brake;
      cout << "brake is " << brake << endl;
    }

    //
    // Navigation.
    //

    else if (text == "target-airport:") {
      input >> airport_id;
      cout << "target airport is " << airport_id << endl;
    }

    else if (text == "autopilot-altitude-lock:") {
      input >> ap_altitude_lock;
      cout << "autopilot altitude lock is " << ap_altitude_lock << endl;
    }

    else if (text == "autopilot-altitude:") {
      input >> ap_altitude;
      cout << "autopilot altitude is " << ap_altitude << endl;
    }

    else if (text == "autopilot-heading-lock:") {
      input >> ap_heading_lock;
      cout << "autopilot heading lock is " << ap_heading_lock << endl;
    }

    else if (text == "autopilot-heading:") {
      input >> ap_heading;
      cout << "autopilot heading is " << ap_heading << endl;
    }

    else if (text == "autopilot-gps-lock:") {
      input >> ap_gps_lock;
      cout << "autopilot GPS lock is " << ap_gps_lock << endl;
    }

    else if (text == "autopilot-gps-lat:") {
      input >> ap_gps_lat;
    }

    else if (text == "autopilot-gps-lon:") {
      input >> ap_gps_lon;
    }

    //
    // Environment.
    //

    else if (text == "visibility:") {
      input >> n;
      cout << "visibility is " << n << endl;
      
#ifndef FG_OLD_WEATHER
      WeatherDatabase->setWeatherVisibility(n);
#else
      current_weather.set_visibility(n);
#endif
    }

    //
    // Don't die if we don't recognize something
    //
    
    else {
      cerr << "Skipping unknown field: " << text << endl;
      input >> text;
    }
  }

  fgReInitSubsystems();

				// Set airport and controls after the
				// re-init.
  current_options.set_airport_id(airport_id);

				// The controls have to be set after
				// the reinit
  controls.set_elevator(elevator);
  controls.set_aileron(aileron);
  controls.set_rudder(rudder);
  controls.set_throttle(FGControls::ALL_ENGINES, throttle);
  controls.set_elevator_trim(elevator_trim);
  controls.set_flaps(flaps);
  controls.set_brake(FGControls::ALL_WHEELS, brake);

				// Ditto for the autopilot.
				// FIXME: shouldn't have to use
				// APDataGlobal.
  APDataGlobal->heading_hold = ap_heading_lock;
  APDataGlobal->altitude_hold = ap_altitude_lock;
  fgAPHeadingSet(ap_heading);
  fgAPAltitudeSet(ap_altitude);
				// GPS overrides heading
  APDataGlobal->waypoint_hold = ap_gps_lock;
  APDataGlobal->TargetLatitude = ap_gps_lat;
  APDataGlobal->TargetLongitude = ap_gps_lon;

  return true;
}

// end of save.cxx
