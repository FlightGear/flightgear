/******************************************************************************
 * Schedule.cxx
 * Written by Durk Talsma, started May 5, 2004.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 ****************************************************************************
 *
 *****************************************************************************/
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <fstream>


#include <string>
#include <vector>
#include <algorithm>

#include <plib/sg.h>

#include <simgear/compiler.h>
#include <simgear/math/polar3d.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/props/props.hxx>
#include <simgear/route/waypoint.hxx>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/xml/easyxml.hxx>

#include <AIModel/AIFlightPlan.hxx>
#include <AIModel/AIManager.hxx>
#include <Airports/simple.hxx>
#include <Main/fg_init.hxx>   // That's pretty ugly, but I need fgFindAirportID


#include "SchedFlight.hxx"
#include "TrafficMgr.hxx"

SG_USING_STD( sort );

/******************************************************************************
 * the FGAISchedule class contains data members and code to maintain a
 * schedule of Flights for an articically controlled aircraft. 
 *****************************************************************************/
FGAISchedule::FGAISchedule()
{
  firstRun     = true;
  AIManagerRef = 0;
}

FGAISchedule::FGAISchedule(string    mdl, 
			   string    liv, 
			   string    reg, 
			   bool      hvy, 
			   FGScheduledFlightVec flt)
{
  modelPath    = mdl; 
  livery       = liv; 
  registration = reg;
  heavy = hvy;
  for (FGScheduledFlightVecIterator i = flt.begin();
       i != flt.end();
       i++)
    flights.push_back(FGScheduledFlight((*i)));
  AIManagerRef = 0;
  firstRun = true;
}

FGAISchedule::FGAISchedule(const FGAISchedule &other)
{
  modelPath    = other.modelPath;
  livery       = other.livery;
  registration = other.registration;
  heavy        = other.heavy;
  flights      = other.flights;
  lat          = other.lat;
  lon          = other.lon;
  AIManagerRef = other.AIManagerRef;
  firstRun     = other.firstRun;
}

FGAISchedule::~FGAISchedule()
{
  
} 

void FGAISchedule::update(time_t now)
{
  FGAirport *dep;
  FGAirport *arr;
  sgdVec3 a, b, cross;
  sgdVec3 newPos;
  sgdMat4 matrix;
  double angle;

  FGAIManager *aimgr;
  string airport;
  
  double courseToUser,   courseToDest;
  double distanceToUser, distanceToDest;
  double speed;

  Point3D temp;
  time_t 
    totalTimeEnroute, 
    elapsedTimeEnroute,
    remainingTimeEnroute;
  double
    userLatitude,
    userLongitude;

  if (fgGetBool("/sim/ai/enabled") == false)
    return;
  
  aimgr = (FGAIManager *) globals-> get_subsystem("ai_model");  
  // Before the flight status of this traffic entity is updated 
  // for the first time, we need to roll back it's flight schedule so
  // so that all the flights are centered around this simulated week's time
  // table. This is to avoid the situation where the first scheduled flight is
  // in the future, causing the traffic manager to not generate traffic until
  // simulated time has caught up with the real world time at initialization.
  // This is to counter a more general initialization bug, caused by the fact
  // that warp is not yet set when the  schedule is initialized. This is
  // especially a problem when using a negative time offset.
  // i.e let's say we specify FlightGear to run with --time-offset=-24:00:00. 
  // Then the schedule will initialize using today, but we will fly yesterday.
  // Thus, it would take a whole day of simulation before the traffic manager
  // finally kicks in. 
  if (firstRun)
    {
      for (FGScheduledFlightVecIterator i = flights.begin(); 
	   i != flights.end(); 
	   i++)
	{
	  i->adjustTime(now);
	}
      firstRun = false;
    }
  
  // Sort all the scheduled flights according to scheduled departure time.
  // Because this is done at every update, we only need to check the status
  // of the first listed flight. 
  sort(flights.begin(), flights.end());
  FGScheduledFlightVecIterator i = flights.begin();
  if (!AIManagerRef)
    {
      userLatitude  = fgGetDouble("/position/latitude-deg");
      userLongitude = fgGetDouble("/position/longitude-deg");


      // This flight entry is entirely in the past, do we need to 
      // push it forward in time to the next scheduled departure. 
      if ((i->getDepartureTime() < now) && (i->getArrivalTime() < now))
	{
	  i->update();
	  return;
	}

      // Departure time in the past and arrival time in the future.
      // This flight is in progress, so we need to calculate it's
      // approximate position and -if in range- create an AIAircraft
      // object for it. 
      if ((i->getDepartureTime() < now) && (i->getArrivalTime() > now))
	{
	  dep = i->getDepartureAirport();
	  arr = i->getArrivalAirport  ();
	  
	  temp = sgPolarToCart3d(Point3D(dep->longitude * 
					 SG_DEGREES_TO_RADIANS, 
					 dep->latitude  * 
					 SG_DEGREES_TO_RADIANS, 
					 1.0));
	  a[0] = temp.x();
	  a[1] = temp.y();
	  a[2] = temp.z();
	  
	  temp = sgPolarToCart3d(Point3D(arr->longitude *
					 SG_DEGREES_TO_RADIANS,
					 arr->latitude  *
					 SG_DEGREES_TO_RADIANS, 
					 1.0));
	  b[0] = temp.x();
	  b[1] = temp.y();
	  b[2] = temp.z();
	  sgdNormaliseVec3(a);
	  sgdNormaliseVec3(b);
	  sgdVectorProductVec3(cross,b,a);
	  
	  angle = sgACos(sgdScalarProductVec3(a,b));
	  
	  // Okay, at this point we have the angle between departure and 
	  // arrival airport, in degrees. From here we can interpolate the
	  // position of the aircraft by calculating the ratio between 
	  // total time enroute and elapsed time enroute. 
	  totalTimeEnroute     = i->getArrivalTime() - i->getDepartureTime();
	  elapsedTimeEnroute   = now - i->getDepartureTime();
	  remainingTimeEnroute = i->getArrivalTime()   - now;  
	  
	  angle *= ( (double) elapsedTimeEnroute/ (double) totalTimeEnroute);
	  
	  
	  //cout << "a = " << a[0] << " " << a[1] << " " << a[2] 
	  //     << "b = " << b[0] << " " << b[1] << " " << b[2] << endl;  
	  sgdMakeRotMat4(matrix, angle, cross); 
	  for(int j = 0; j < 3; j++)
	    {
	      newPos[j] =0.0;
	      for (int k = 0; k<3; k++)
		{
		  newPos[j] += matrix[j][k]*a[k];
		}
	    }

	  temp = sgCartToPolar3d(Point3D(newPos[0], newPos[1],newPos[2]));

	  lat = temp.lat() * SG_RADIANS_TO_DEGREES;
	  lon = temp.lon() * SG_RADIANS_TO_DEGREES;
	  
	  SGWayPoint current  (lon,
			       lat,
			       i->getCruiseAlt());
	  SGWayPoint user (   userLongitude,
			      userLatitude,
			      i->getCruiseAlt());
	  SGWayPoint dest (   arr->longitude,
			      arr->latitude,
			      i->getCruiseAlt());
	  // We really only need distance to user
	  // and course to destination 
	  current.CourseAndDistance(user, &courseToUser, &distanceToUser);
	  current.CourseAndDistance(dest, &courseToDest, &distanceToDest);
	  speed =  (distanceToDest*SG_METER_TO_NM) / 
	    ((double) remainingTimeEnroute/3600.0);
	  

	  // If distance between user and simulated aircaft is less
	  // then 500nm, create this flight. At jet speeds 500 nm is roughly
	  // one hour flight time, so that would be a good approximate point
	  // to start a more detailed simulation of this aircraft.
	  //cerr << registration << " is currently enroute from " 
	  //   << dep->id << " to " << arr->id << "distance : " << distanceToUser*SG_METER_TO_NM << endl;
	  if ((distanceToUser*SG_METER_TO_NM) < 500.0)
	    {
	      string flightPlanName = dep->id + string("-") + arr->id + string(".xml");

	      FGAIFlightPlan* f;
	      // If we're less then 10 minutes behind schedule, do a normal
	      // full flight plan initialization, otherwise, do a modified 
	      // in-air initializition, at the location where this flight is
	      // according to the Traffic Manager
	      if (elapsedTimeEnroute < 600)
		f = new FGAIFlightPlan(flightPlanName);
	      else
		f = new FGAIFlightPlan(flightPlanName, 
				       lat, 
				       lon, 
				       i->getCruiseAlt() * 1000, // convert from FL to feet
				       speed, 
				       courseToDest);
	      // Fixme: A non-existent model path results in an
	      // abort, due to an unhandled exeption, in fg main loop.
	      AIManagerRef = aimgr->createAircraft("jet_transport", 
					       modelPath, f); 
	      //cerr << "Created: " << AIManagerRef << endl;
	    }
	  return;
	}
      // Both departure and arrival time are in the future, so this
      // the aircraft is parked at the departure airport.
      // Currently this status is mostly ignored, but in furture
      // versions, code should go here that -if within user range-
      // positions these aircraft at parking locations at the airport.
      if ((i->getDepartureTime() > now) && (i->getArrivalTime() > now))
	{ 
	  dep = i->getDepartureAirport();
	  return;
	} 
    }
}
