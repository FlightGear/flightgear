// bfi.hxx - Big Flat Interface
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

#include <time.h>
#include <string>

FG_USING_NAMESPACE(std);


/**
 * Big Flat Interface
 *
 * This class implements the Facade design pattern (GOF p.185) to provide
 * a single, (deceptively) simple flat interface for the FlightGear
 * subsystems.  
 *
 * To help cut down on interdependence, subsystems should
 * use the BFI whenever possible for inter-system communication.
 *
 * TODO:
 * - add selectors to switch the current plane, throttle, brake, etc.
 * - add more autopilot settings
 */
class FGBFI
{
public:

				// Reinit if necessary.
  static void update ();

				// Simulation
  static int getFlightModel ();
  static time_t getTimeGMT ();
  static bool getHUDVisible ();
  static bool getPanelVisible ();

  static void setFlightModel (int flightModel);
  static void setTimeGMT (time_t time);
  static void setHUDVisible (bool hudVisible);
  static void setPanelVisible (bool panelVisible);


				// Position
  static double getLatitude ();
  static double getLongitude ();
  static double getAltitude ();

  static void setLatitude (double latitude);
  static void setLongitude (double longitude);
  static void setAltitude (double altitude);


				// Attitude
  static double getHeading ();
  static double getPitch ();
  static double getRoll ();

  static void setHeading (double heading);
  static void setPitch (double pitch);
  static void setRoll (double roll);


				// Velocities
  static double getAirspeed ();
  static double getSideSlip ();
  static double getVerticalSpeed ();
  static double getSpeedNorth ();
  static double getSpeedEast ();
  static double getSpeedDown ();

  static void setSpeedNorth (double speed);
  static void setSpeedEast (double speed);
  static void setSpeedDown (double speed);


				// Controls
  static double getThrottle ();
  static double getFlaps ();
  static double getAileron ();
  static double getRudder ();
  static double getElevator ();
  static double getElevatorTrim ();
  static double getBrake ();

  static void setThrottle (double throttle);
  static void setFlaps (double flaps);
  static void setAileron (double aileron);
  static void setRudder (double rudder);
  static void setElevator (double elevator);
  static void setElevatorTrim (double trim);
  static void setBrake (double brake);


				// Autopilot
  static bool getAPAltitudeLock ();
  static double getAPAltitude ();
  static bool getAPHeadingLock ();
  static double getAPHeading ();

  static void setAPAltitudeLock (bool lock);
  static void setAPAltitude (double altitude);
  static void setAPHeadingLock (bool lock);
  static void setAPHeading (double heading);


				// GPS
  static const string getTargetAirport ();
  static bool getGPSLock ();
  static double getGPSTargetLatitude ();
  static double getGPSTargetLongitude ();

  static void setTargetAirport (const string &targetAirport);
  static void setGPSLock (bool lock);
  static void setGPSTargetLatitude (double latitude);
  static void setGPSTargetLongitude (double longitude);


				// Weather
  static double getVisibility ();

  static void setVisibility (double visiblity);

                                // Time (this varies with time) huh, huh
  static double getMagVar (); 
  static double getMagDip (); 


private:
				// Will cause a linking error if invoked.
  FGBFI ();

  static void reinit ();
  static void needReinit () { _needReinit = true; }
  static bool _needReinit;
};

// end of bfi.hxx
