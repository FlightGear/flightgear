// bfi.hxx - Big Flat Interface
//
// Written by David Megginson, started February, 2000.
//
// Copyright (C) 2000  David Megginson - david@megginson.com
//
// THIS INTERFACE IS DEPRECATED; USE THE PROPERTY MANAGER INSTEAD.
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

#include <simgear/compiler.h>

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

				// Initialize before first use.
  static void init ();

				// Reinit if necessary.
  static void update ();

				// Simulation
  static int getFlightModel ();
  static void setFlightModel (int flightModel);

  static string getAircraft ();
  static void setAircraft (string aircraft);

  static string getAircraftDir ();
  static void setAircraftDir (string aircraftDir);

//   static time_t getTimeGMT ();
//   static void setTimeGMT (time_t time);
  static string getDateString ();// ISO 8601 subset
  static void setDateString (string time_string); // ISO 8601 subset

				// deprecated
  static string getGMTString ();

  static bool getHUDVisible ();
  static void setHUDVisible (bool hudVisible);

				// Position
  static double getLatitude ();	// degrees
  static void setLatitude (double latitude); // degrees

  static double getLongitude (); // degrees
  static void setLongitude (double longitude); // degrees

  static double getAltitude ();	// feet
  static void setAltitude (double altitude); // feet

  static double getAGL ();	// feet


				// Attitude
  static double getHeading ();	  // degrees
  static void setHeading (double heading); // degrees

  static double getHeadingMag (); // degrees

  static double getPitch ();	// degrees
  static void setPitch (double pitch); // degrees

  static double getRoll ();	// degrees
  static void setRoll (double roll); // degrees

				// Engine
  static double getRPM ();	// revolutions/minute
  static void setRPM ( double rpm ); // revolutions/minute

  static double getEGT ();	// [unit??]
  static double getCHT ();	// [unit??]
  static double getMP ();	// [unit??]

				// Velocities
  static double getAirspeed ();	// knots
  static void setAirspeed (double speed); // knots

  static double getSideSlip ();	// [unit??]

  static double getVerticalSpeed (); // feet/second

  static double getSpeedNorth (); // feet/second

  static double getSpeedEast (); // feet/second

  static double getSpeedDown (); // feet/second

//   static void setSpeedNorth (double speed);
//   static void setSpeedEast (double speed);
//   static void setSpeedDown (double speed);


#if 0
				// Controls
  static double getThrottle ();	// 0.0:1.0
  static void setThrottle (double throttle); // 0.0:1.0

  static double getMixture ();	// 0.0:1.0
  static void setMixture (double mixture); // 0.0:1.0

  static double getPropAdvance (); // 0.0:1.0
  static void setPropAdvance (double pitch); // 0.0:1.0

  static double getFlaps ();	// 0.0:1.0
  static void setFlaps (double flaps); // 0.0:1.0

  static double getAileron ();	// -1.0:1.0
  static void setAileron (double aileron); // -1.0:1.0

  static double getRudder ();	// -1.0:1.0
  static void setRudder (double rudder); // -1.0:1.0

  static double getElevator ();	// -1.0:1.0
  static void setElevator (double elevator); // -1.0:1.0

  static double getElevatorTrim (); // -1.0:1.0
  static void setElevatorTrim (double trim); // -1.0:1.0

  static double getBrakes ();	// 0.0:1.0
  static void setBrakes (double brake);	// 0.0:1.0

  static double getLeftBrake (); // 0.0:1.0
  static void setLeftBrake (double brake); // 0.0:1.0

  static double getRightBrake (); // 0.0:1.0
  static void setRightBrake (double brake); // 0.0:1.0

  static double getCenterBrake (); // 0.0:1.0
  static void setCenterBrake (double brake); // 0.0:1.0

#endif

				// Autopilot
  static bool getAPAltitudeLock ();
  static void setAPAltitudeLock (bool lock);

  static double getAPAltitude (); // feet
  static void setAPAltitude (double altitude); // feet

  static bool getAPHeadingLock ();
  static void setAPHeadingLock (bool lock);

  static double getAPHeading (); // degrees
  static void setAPHeading (double heading); // degrees

  static double getAPHeadingMag (); // degrees
  static void setAPHeadingMag (double heading);	// degrees

  static bool getAPNAV1Lock ();
  static void setAPNAV1Lock (bool lock);


				// GPS
  static string getTargetAirport ();
  static void setTargetAirport (string targetAirport);

  static bool getGPSLock ();
  static void setGPSLock (bool lock);

  static double getGPSTargetLatitude (); // degrees

  static double getGPSTargetLongitude (); // degrees


				// Weather
  static double getVisibility ();// meters
  static void setVisibility (double visiblity);	// meters
  static double getWindNorth (); // feet/second
  static void setWindNorth (double speed); // feet/second
  static double getWindEast ();	// feet/second
  static void setWindEast (double speed); // feet/second
  static double getWindDown ();	// feet/second
  static void setWindDown (double speed); // feet/second

				// View
  static void setViewAxisLong (double axis);// -1.0:1.0
  static void setViewAxisLat (double axis); // -1.0:1.0


                                // Time (this varies with time) huh, huh
  static double getMagVar ();	// degrees
  static double getMagDip ();	// degrees


private:
				// Will cause a linking error if invoked.
  FGBFI ();

};

// end of bfi.hxx
