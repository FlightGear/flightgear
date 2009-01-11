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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *
 ****************************************************************************
 *
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#define BOGUS 0xFFFF

#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <fstream>


#include <string>
#include <vector>
#include <algorithm>

#include <simgear/compiler.h>
#include <simgear/sg_inlines.h>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/props/props.hxx>
#include <simgear/route/waypoint.hxx>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/xml/easyxml.hxx>

#include <AIModel/AIFlightPlan.hxx>
#include <AIModel/AIManager.hxx>
#include <AIModel/AIAircraft.hxx>
#include <Airports/simple.hxx>
#include <Main/fg_init.hxx>   // That's pretty ugly, but I need fgFindAirportID


#include "SchedFlight.hxx"
#include "TrafficMgr.hxx"

using std::sort;

/******************************************************************************
 * the FGAISchedule class contains data members and code to maintain a
 * schedule of Flights for an articically controlled aircraft. 
 *****************************************************************************/
FGAISchedule::FGAISchedule()
{
  firstRun     = true;
  AIManagerRef = 0;

  heavy = false;
  lat = 0;
  lon = 0;
  radius = 0;
  groundOffset = 0;
  distanceToUser = 0;
  //score = 0;
}

/*
FGAISchedule::FGAISchedule(string    mdl, 
			   string    liv, 
			   string    reg, 
			   bool      hvy, 
			   string act, 
			   string arln, 
			   string mclass, 
			   string fltpe,
			   double rad,
			   double grnd,
			   int    scre,
			   FGScheduledFlightVec flt)*/
FGAISchedule::FGAISchedule(string model, 
                           string lvry,
                           string port, 
                           string reg, 
                           string flightId,
                           bool   hvy, 
                           string act, 
                           string arln, 
                           string mclass, 
                           string fltpe, 
                           double rad, 
                           double grnd)
{
  modelPath        = model; 
  livery           = lvry; 
  homePort         = port;
  registration     = reg;
  flightIdentifier = flightId;
  acType           = act;
  airline          = arln;
  m_class          = mclass;
  flightType       = fltpe;
  lat              = 0;
  lon              = 0;
  radius           = rad;
  groundOffset     = grnd;
  distanceToUser   = 0;
  heavy            = hvy;
  /*for (FGScheduledFlightVecIterator i = flt.begin();
       i != flt.end();
       i++)
    flights.push_back(new FGScheduledFlight((*(*i))));*/
  AIManagerRef     = 0;
  //score    = scre;
  firstRun         = true;
}

FGAISchedule::FGAISchedule(const FGAISchedule &other)
{
  modelPath          = other.modelPath;
  homePort           = other.homePort;
  livery             = other.livery;
  registration       = other.registration;
  heavy              = other.heavy;
  flightIdentifier   = other.flightIdentifier;
  flights            = other.flights;
  lat                = other.lat;
  lon                = other.lon;
  AIManagerRef       = other.AIManagerRef;
  acType             = other.acType;
  airline            = other.airline;
  m_class            = other.m_class;
  firstRun           = other.firstRun;
  radius             = other.radius;
  groundOffset       = other.groundOffset;
  flightType         = other.flightType;
  //score            = other.score;
  distanceToUser     = other.distanceToUser;
  currentDestination = other.currentDestination;
  firstRun           = other.firstRun;
}


FGAISchedule::~FGAISchedule()
{
/*  for (FGScheduledFlightVecIterator flt = flights.begin(); flt != flights.end(); flt++)
    {
      delete (*flt);
    }
  flights.clear();*/
} 

bool FGAISchedule::init()
{
  //tm targetTimeDate;
  //SGTime* currTimeDate = globals->get_time_params();

  //tm *temp = currTimeDate->getGmt();
  //char buffer[512];
  //sgTimeFormatTime(&targetTimeDate, buffer);
  //cout << "Scheduled Time " << buffer << endl; 
  //cout << "Time :" << time(NULL) << " SGTime : " << sgTimeGetGMT(temp) << endl;
  /*for (FGScheduledFlightVecIterator i = flights.begin(); 
       i != flights.end(); 
       i++)
    {
      //i->adjustTime(now);
      if (!((*i)->initializeAirports()))
	return false;
    } */
  //sort(flights.begin(), flights.end());
  // Since time isn't initialized yet when this function is called,
  // Find the closest possible airport.
  // This should give a reasonable initialization order. 
  //setClosestDistanceToUser();
  return true;
}

bool FGAISchedule::update(time_t now)
{ 
  FGAirport *dep;
  FGAirport *arr;
  double angle;

  FGAIManager *aimgr;
  string airport;
  
  double courseToUser,   courseToDest;
  double distanceToDest;
  double speed;

  time_t 
    totalTimeEnroute, 
    elapsedTimeEnroute,
    remainingTimeEnroute, deptime = 0;
  double
    userLatitude,
    userLongitude;

  if (fgGetBool("/sim/traffic-manager/enabled") == false)
    return true;
  
  aimgr = (FGAIManager *) globals-> get_subsystem("ai_model");  
    // Out-of-work aircraft seeks employment. Willing to work irregular hours ...
    //cerr << "About to find a flight " << endl;
    if (flights.empty()) {
        //execute this loop at least once. 
        SG_LOG(SG_GENERAL, SG_DEBUG, "Scheduling for : " << modelPath << " " <<  registration << " " << homePort);
        FGScheduledFlight *flight = 0;
         do {
            flight = findAvailableFlight(currentDestination, flightIdentifier);
            if (flight) {
                currentDestination = flight->getArrivalAirport()->getId();
                time_t arr, dep;
                dep = flight->getDepartureTime();
                arr = flight->getArrivalTime();
                string depT = asctime(gmtime(&dep));
                string arrT = asctime(gmtime(&arr));

                depT = depT.substr(0,24);
                arrT = arrT.substr(0,24);
                SG_LOG(SG_GENERAL, SG_DEBUG, "  " << flight->getCallSign() << ":" 
                                         << "  " << flight->getDepartureAirport()->getId() << ":"
                                         << "  " << depT << ":"
                                         << " \"" << flight->getArrivalAirport()->getId() << "\"" << ":"
                                         << "  " << arrT << ":");
            flights.push_back(flight);
            }
        } while ((currentDestination != homePort) && (flight != 0));
        SG_LOG(SG_GENERAL, SG_DEBUG, " Done");
    }
    //cerr << " Done " << endl;
   // No flights available for this aircraft
  if (flights.size() == 0) {
      return false;
  }
  // Sort all the scheduled flights according to scheduled departure time.
  // Because this is done at every update, we only need to check the status
  // of the first listed flight. 
  //sort(flights.begin(), flights.end(), compareScheduledFlights);
 if (firstRun) {
     if (fgGetBool("/sim/traffic-manager/instantaneous-action") == true) {
         deptime = now + rand() % 300; // Wait up to 5 minutes until traffic starts moving to prevent too many aircraft 
                                   // from cluttering the gate areas.
         cerr << "Scheduling " << registration << " for instantaneous action flight " << endl;
     }
     firstRun = false;
  }
  if (!deptime)
    deptime = (*flights.begin())->getDepartureTime();
  FGScheduledFlightVecIterator i = flights.begin();
  SG_LOG (SG_GENERAL, SG_DEBUG,"Traffic Manager: Processing registration " << registration << " with callsign " << (*i)->getCallSign());
  if (AIManagerRef)
    {
      // Check if this aircraft has been released. 
      FGTrafficManager *tmgr = (FGTrafficManager *) globals->get_subsystem("Traffic Manager");
      if (tmgr->isReleased(AIManagerRef))
	AIManagerRef = 0;
    }

  if (!AIManagerRef)
    {
      userLatitude  = fgGetDouble("/position/latitude-deg");
      userLongitude = fgGetDouble("/position/longitude-deg");

      //cerr << "Estimated minimum distance to user: " << distanceToUser << endl;
      // This flight entry is entirely in the past, do we need to 
      // push it forward in time to the next scheduled departure. 
      if (((*i)->getDepartureTime() < now) && ((*i)->getArrivalTime() < now))
	{
          SG_LOG (SG_GENERAL, SG_DEBUG, "Traffic Manager:      Flight is in the Past");
          //cerr << modelPath << " " <<  registration << ": Flights from the past belong to the past :-)" << endl;
          //exit(1);
          // Don't just update: check whether we need to load a new leg. etc.
          // This update occurs for distant aircraft, so we can update the current leg
          // and detach it from the current list of aircraft. 
	  (*i)->update();
          i = flights.erase(i);
	  return true;
	}

      // Departure time in the past and arrival time in the future.
      // This flight is in progress, so we need to calculate it's
      // approximate position and -if in range- create an AIAircraft
      // object for it. 
      //if ((i->getDepartureTime() < now) && (i->getArrivalTime() > now))
      
      // Part of this flight is in the future.
      if ((*i)->getArrivalTime() > now)
	{
          
	  dep = (*i)->getDepartureAirport();
	  arr = (*i)->getArrivalAirport  ();
	  if (!(dep && arr))
	    return false;
	  
          SGVec3d a = SGVec3d::fromGeoc(SGGeoc::fromDegM(dep->getLongitude(),
                                                dep->getLatitude(), 1));
          SGVec3d b = SGVec3d::fromGeoc(SGGeoc::fromDegM(arr->getLongitude(),
                                                arr->getLatitude(), 1));
          SGVec3d _cross = cross(b, a);
	  
	  angle = sgACos(dot(a, b));
	  
	  // Okay, at this point we have the angle between departure and 
	  // arrival airport, in degrees. From here we can interpolate the
	  // position of the aircraft by calculating the ratio between 
	  // total time enroute and elapsed time enroute. 
 
	  totalTimeEnroute     = (*i)->getArrivalTime() - (*i)->getDepartureTime();
	  if (now > (*i)->getDepartureTime())
	    {
	      //err << "Lat = " << lat << ", lon = " << lon << endl;
	      //cerr << "Time diff: " << now-i->getDepartureTime() << endl;
	      elapsedTimeEnroute   = now - (*i)->getDepartureTime();
	      remainingTimeEnroute = (*i)->getArrivalTime()   - now;  
              SG_LOG (SG_GENERAL, SG_DEBUG, "Traffic Manager:      Flight is in progress.");
	    }
	  else
	    {
	      lat = dep->getLatitude();
	      lon = dep->getLongitude();
	      elapsedTimeEnroute = 0;
	      remainingTimeEnroute = totalTimeEnroute;
              SG_LOG (SG_GENERAL, SG_DEBUG, "Traffic Manager:      Flight is pending.");
	    }
	  	  
	  angle *= ( (double) elapsedTimeEnroute/ (double) totalTimeEnroute);
	  
	  
	  //cout << "a = " << a[0] << " " << a[1] << " " << a[2] 
	  //     << "b = " << b[0] << " " << b[1] << " " << b[2] << endl;  
          sgdMat4 matrix;
	  sgdMakeRotMat4(matrix, angle, _cross.sg()); 
          SGVec3d newPos(0, 0, 0);
	  for(int j = 0; j < 3; j++)
	    {
	      for (int k = 0; k<3; k++)
		{
		  newPos[j] += matrix[j][k]*a[k];
		}
	    }
	  
	  if (now > (*i)->getDepartureTime())
	    {
              SGGeoc geoc = SGGeoc::fromCart(newPos);
	      lat = geoc.getLatitudeDeg();
	      lon = geoc.getLongitudeDeg(); 
	    }
	  else
	    {
	      lat = dep->getLatitude();
	      lon = dep->getLongitude();
	    }
	  
	  
	  SGWayPoint current  (lon,
			       lat,
			       (*i)->getCruiseAlt(), 
			       SGWayPoint::SPHERICAL);
	  SGWayPoint user (   userLongitude,
			      userLatitude,
			      (*i)->getCruiseAlt(), 
			      SGWayPoint::SPHERICAL);
	  SGWayPoint dest (   arr->getLongitude(),
			      arr->getLatitude(),
			      (*i)->getCruiseAlt(), 
			      SGWayPoint::SPHERICAL);
	  // We really only need distance to user
	  // and course to destination 
	  user.CourseAndDistance(current, &courseToUser, &distanceToUser);
	  dest.CourseAndDistance(current, &courseToDest, &distanceToDest);
	  speed =  (distanceToDest*SG_METER_TO_NM) / 
	    ((double) remainingTimeEnroute/3600.0);
	  

	  // If distance between user and simulated aircaft is less
	  // then 500nm, create this flight. At jet speeds 500 nm is roughly
	  // one hour flight time, so that would be a good approximate point
	  // to start a more detailed simulation of this aircraft.
	  SG_LOG (SG_GENERAL, SG_DEBUG, "Traffic manager: " << registration << " is scheduled for a flight from " 
	     << dep->getId() << " to " << arr->getId() << ". Current distance to user: " 
             << distanceToUser*SG_METER_TO_NM);
	  if ((distanceToUser*SG_METER_TO_NM) < TRAFFICTOAIDISTTOSTART)
	    {
	      string flightPlanName = dep->getId() + string("-") + arr->getId() + 
		string(".xml");
              SG_LOG (SG_GENERAL, SG_DEBUG, "Traffic manager: Creating AIModel");
	      //int alt;
	      //if  ((i->getDepartureTime() < now))
	      //{
	      //	  alt = i->getCruiseAlt() *100;
	      //	}
	      //else
	      //{
	      //	  alt = dep->_elevation+19;
	      //	}

	      // Only allow traffic to be created when the model path (or the AI version of mp) exists
	      SGPath mp(globals->get_fg_root());
	      SGPath mp_ai = mp;

	      mp.append(modelPath);
	      mp_ai.append("AI");
	      mp_ai.append(modelPath);

	      if (mp.exists() || mp_ai.exists())
	      {
		  FGAIAircraft *aircraft = new FGAIAircraft(this);
		  aircraft->setPerformance(m_class); //"jet_transport";
		  aircraft->setCompany(airline); //i->getAirline();
		  aircraft->setAcType(acType); //i->getAcType();
		  aircraft->setPath(modelPath.c_str());
		  //aircraft->setFlightPlan(flightPlanName);
		  aircraft->setLatitude(lat);
		  aircraft->setLongitude(lon);
		  aircraft->setAltitude((*i)->getCruiseAlt()*100); // convert from FL to feet
		  aircraft->setSpeed(speed);
		  aircraft->setBank(0);
		  aircraft->SetFlightPlan(new FGAIFlightPlan(aircraft, flightPlanName, courseToDest, deptime, 
							     dep, arr,true, radius, 
							     (*i)->getCruiseAlt()*100, 
							     lat, lon, speed, flightType, acType, 
							     airline));
		  aimgr->attach(aircraft);
		  
		  
		  AIManagerRef = aircraft->getID();
		  //cerr << "Class: " << m_class << ". acType: " << acType << ". Airline: " << airline << ". Speed = " << speed << ". From " << dep->getId() << " to " << arr->getId() << ". Time Fraction = " << (remainingTimeEnroute/(double) totalTimeEnroute) << endl;
		  //cerr << "Latitude : " << lat << ". Longitude : " << lon << endl;
		  //cerr << "Dep      : " << dep->getLatitude()<< ", "<< dep->getLongitude() << endl;
		  //cerr << "Arr      : " << arr->getLatitude()<< ", "<< arr->getLongitude() << endl;
		  //cerr << "Time remaining = " << (remainingTimeEnroute/3600.0) << endl;
		  //cerr << "Total time     = " << (totalTimeEnroute/3600.0) << endl;
		  //cerr << "Distance remaining = " << distanceToDest*SG_METER_TO_NM << endl;
		  }
	      else
		{
		  SG_LOG(SG_INPUT, SG_WARN, "TrafficManager: Could not load model " << mp.str());
		}
	    }
	  return true;
    }

      // Both departure and arrival time are in the future, so this
      // the aircraft is parked at the departure airport.
      // Currently this status is mostly ignored, but in future
      // versions, code should go here that -if within user range-
      // positions these aircraft at parking locations at the airport.
      if (((*i)->getDepartureTime() > now) && ((*i)->getArrivalTime() > now))
	{ 
	  dep = (*i)->getDepartureAirport();
	  return true;
	} 
    }
  //cerr << "Traffic schedule got to beyond last clause" << endl;
    // EMH: prevent a warning, should this be 'true' instead?
    // DT: YES. Originally, this code couldn't be reached, but
    // when the "if(!(AIManagerManager))" clause is false we
    // fall through right to the end. This is a valid flow.
    // the actual value is pretty innocent, only it triggers
    // warning in TrafficManager::update().
    // (which was added as a sanity check for myself in the first place. :-)
    return true;
}


bool FGAISchedule::next()
{
  FGScheduledFlightVecIterator i = flights.begin();
  (*i)->release();
  //FIXME: remove first entry, 
  // load new flights until back at home airport
  // Lock loaded flights
  //sort(flights.begin(), flights.end(), compareScheduledFlights);
  // until that time
  i = flights.erase(i);
  //cerr << "Next: scheduling for : " << modelPath << " " <<  registration << endl;
  FGScheduledFlight *flight = findAvailableFlight(currentDestination, flightIdentifier);
  if (flight) {
      currentDestination = flight->getArrivalAirport()->getId();
      time_t arr, dep;
      dep = flight->getDepartureTime();
      arr = flight->getArrivalTime();
      string depT = asctime(gmtime(&dep));
      string arrT = asctime(gmtime(&arr));

      depT = depT.substr(0,24);
      arrT = arrT.substr(0,24);
      //cerr << "  " << flight->getCallSign() << ":" 
      //     << "  " << flight->getDepartureAirport()->getId() << ":"
      //     << "  " << depT << ":"
      //     << " \"" << flight->getArrivalAirport()->getId() << "\"" << ":"
      //     << "  " << arrT << ":" << endl;

       flights.push_back(flight);
       return true;
  } else {
       return false;
  }
  //cerr << "FGAISchedule :: next needs updating" << endl;
  //exit(1);
}

FGScheduledFlight* FGAISchedule::findAvailableFlight (const string &currentDestination,
                                                      const string &req)
{
    time_t now = time(NULL) + fgGetLong("/sim/time/warp");

    FGTrafficManager *tmgr = (FGTrafficManager *) globals->get_subsystem("Traffic Manager");
    FGScheduledFlightVecIterator fltBegin, fltEnd;
    fltBegin = tmgr->getFirstFlight(req);
    fltEnd   = tmgr->getLastFlight(req);


     //cerr << "Finding available flight " << endl;
     // For Now:
     // Traverse every registered flight
     if (fltBegin == fltEnd) {
          //cerr << "No Flights Scheduled for " << req << endl;
     }
     int counter = 0;
     for (FGScheduledFlightVecIterator i = fltBegin; i != fltEnd; i++) {
          (*i)->adjustTime(now);
           //sort(fltBegin, fltEnd, compareScheduledFlights);
           //cerr << counter++ << endl;
     }
     sort(fltBegin, fltEnd, compareScheduledFlights);
     for (FGScheduledFlightVecIterator i = fltBegin; i != fltEnd; i++) {
          //bool valid = true;
          counter++;
          if (!(*i)->isAvailable()) {
               //cerr << (*i)->getCallSign() << "is no longer available" << endl;
               continue;
          }
          if (!((*i)->getRequirement() == req)) {
               continue;
          }
          if (!(((*i)->getArrivalAirport()) && ((*i)->getDepartureAirport()))) {
              continue;
          }
          if (!(currentDestination.empty())) {
              if (currentDestination != (*i)->getDepartureAirport()->getId()) {
                   //cerr << (*i)->getCallSign() << "Doesn't match destination" << endl;
                   //cerr << "Current Destination " << currentDestination << "Doesnt match flight's " <<
                   //          (*i)->getArrivalAirport()->getId() << endl;
                   continue;
              }
          }
          //TODO: check time
          // So, if we actually get here, we have a winner
          //cerr << "found flight: " << req << " : " << currentDestination << " : " <<       
          //         (*i)->getArrivalAirport()->getId() << endl;
          (*i)->lock();
          return (*i);
     }
     // matches req?
     // if currentDestination has a value, does it match departure of next flight?
     // is departure time later than planned arrival?
     // is departure port valid?
     // is arrival port valid?
     //cerr << "Ack no flight found: " << endl;
     return 0;
}

double FGAISchedule::getSpeed()
{
  FGScheduledFlightVecIterator i = flights.begin();
 
  FGAirport* dep = (*i)->getDepartureAirport(),
   *arr = (*i)->getArrivalAirport();
  double dist = SGGeodesy::distanceNm(dep->geod(), arr->geod());
  double remainingTimeEnroute = (*i)->getArrivalTime() - (*i)->getDepartureTime();

  double speed = dist / (remainingTimeEnroute/3600.0);
  SG_CLAMP_RANGE(speed, 300.0, 500.0);
  return speed;
}
/*
bool compareSchedules(FGAISchedule*a, FGAISchedule*b)
{ 
  //return (*a) < (*b); 
} 
*/

// void FGAISchedule::setClosestDistanceToUser()
// {
  
  
//   double course;
//   double dist;

//   Point3D temp;
//   time_t 
//     totalTimeEnroute, 
//     elapsedTimeEnroute;
 
//   double userLatitude  = fgGetDouble("/position/latitude-deg");
//   double userLongitude = fgGetDouble("/position/longitude-deg");

//   FGAirport *dep;
  
// #if defined( __CYGWIN__) || defined( __MINGW32__)
//   #define HUGE HUGE_VAL
// #endif
//   distanceToUser = HUGE;
//   FGScheduledFlightVecIterator i = flights.begin();
//   while (i != flights.end())
//     {
//       dep = i->getDepartureAirport();
//       //if (!(dep))
//       //return HUGE;
      
//       SGWayPoint user (   userLongitude,
// 			  userLatitude,
// 			  i->getCruiseAlt());
//       SGWayPoint current (dep->getLongitude(),
// 			  dep->getLatitude(),
// 			  0);
//       user.CourseAndDistance(current, &course, &dist);
//       if (dist < distanceToUser)
// 	{
// 	  distanceToUser = dist;
// 	  //cerr << "Found closest distance to user for " << registration << " to be " << distanceToUser << " at airport " << dep->getId() << endl;
// 	}
//       i++;
//     }
//   //return distToUser;
// }

