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
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/xml/easyxml.hxx>

#include <AIModel/AIFlightPlan.hxx>
#include <AIModel/AIManager.hxx>
#include <AIModel/AIAircraft.hxx>
#include <Airports/simple.hxx>
#include <Main/fg_init.hxx>   // That's pretty ugly, but I need fgFindAirportID


#include "SchedFlight.hxx"
#include "TrafficMgr.hxx"

/******************************************************************************
 * the FGAISchedule class contains data members and code to maintain a
 * schedule of Flights for an artificially controlled aircraft.
 *****************************************************************************/
FGAISchedule::FGAISchedule()
{
  firstRun     = true;
  AIManagerRef = 0;

  heavy = false;
  radius = 0;
  groundOffset = 0;
  distanceToUser = 0;
  valid = true;
  lastRun = 0;
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
  radius           = rad;
  groundOffset     = grnd;
  distanceToUser   = 0;
  heavy            = hvy;
  /*for (FGScheduledFlightVecIterator i = flt.begin();
       i != flt.end();
       i++)
    flights.push_back(new FGScheduledFlight((*(*i))));*/
  AIManagerRef     = 0;
  score    =         0;
  firstRun         = true;
  runCount         = 0;
  hits             = 0;
  lastRun          = 0;
  initialized      = false;
  valid            = true;
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
  AIManagerRef       = other.AIManagerRef;
  acType             = other.acType;
  airline            = other.airline;
  m_class            = other.m_class;
  firstRun           = other.firstRun;
  radius             = other.radius;
  groundOffset       = other.groundOffset;
  flightType         = other.flightType;
  score              = other.score;
  distanceToUser     = other.distanceToUser;
  currentDestination = other.currentDestination;
  firstRun           = other.firstRun;
  runCount           = other.runCount;
  hits               = other.hits;
  lastRun            = other.lastRun;
  initialized        = other.initialized;
  valid              = other.valid;
}



FGAISchedule::~FGAISchedule()
{
    // remove related object from AI manager
    if (AIManagerRef)
    {
        FGAIManager* aimgr = (FGAIManager *) globals-> get_subsystem("ai-model");
        if (aimgr)
            aimgr->destroyObject(AIManagerRef);
        AIManagerRef = 0;
    }

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

bool FGAISchedule::update(time_t now, const SGVec3d& userCart)
{ 
  time_t 
    totalTimeEnroute, 
    elapsedTimeEnroute,
    //remainingTimeEnroute,
    deptime = 0;
  if (!valid) {
    return false;
  }
  scheduleFlights(now);
  if (flights.empty()) { // No flights available for this aircraft
      valid = false;
      return false;
  }
  
  // Sort all the scheduled flights according to scheduled departure time.
  // Because this is done at every update, we only need to check the status
  // of the first listed flight. 
  //sort(flights.begin(), flights.end(), compareScheduledFlights);
  
  if (firstRun) {
     if (fgGetBool("/sim/traffic-manager/instantaneous-action") == true) {
         deptime = now; // + rand() % 300; // Wait up to 5 minutes until traffic starts moving to prevent too many aircraft 
                                   // from cluttering the gate areas.
     }
     firstRun = false;
  }
  
  FGScheduledFlight* flight = flights.front();
  if (!deptime) {
    deptime = flight->getDepartureTime();
    //cerr << "Settiing departure time " << deptime << endl;
  }
    
  if (AIManagerRef) {
    // Check if this aircraft has been released. 
    FGTrafficManager *tmgr = (FGTrafficManager *) globals->get_subsystem("traffic-manager");
    if (tmgr->isReleased(AIManagerRef)) {
      AIManagerRef = 0;
    } else {
      return true; // in visual range, let the AIManager handle it
    }
  }
  
  // This flight entry is entirely in the past, do we need to 
  // push it forward in time to the next scheduled departure. 
  if (flight->getArrivalTime() < now) {
    SG_LOG (SG_GENERAL, SG_BULK, "Traffic Manager:      Flight is in the Past");
    // Don't just update: check whether we need to load a new leg. etc.
    // This update occurs for distant aircraft, so we can update the current leg
    // and detach it from the current list of aircraft. 
    flight->update();
    flights.erase(flights.begin()); // pop_front(), effectively
    return true;
  }
  
  FGAirport* dep = flight->getDepartureAirport();
  FGAirport* arr = flight->getArrivalAirport();
  if (!dep || !arr) {
    return false;
  }
    
  double speed = 450.0;
  if (dep != arr) {
    totalTimeEnroute = flight->getArrivalTime() - flight->getDepartureTime();
    if (flight->getDepartureTime() < now) {
      elapsedTimeEnroute   = now - flight->getDepartureTime();
      //remainingTimeEnroute = totalTimeEnroute - elapsedTimeEnroute;
      double x = elapsedTimeEnroute / (double) totalTimeEnroute;
      
    // current pos is based on great-circle course between departure/arrival,
    // with percentage of distance travelled, based upon percentage of time
    // enroute elapsed.
      double course, az2, distanceM;
      SGGeodesy::inverse(dep->geod(), arr->geod(), course, az2, distanceM);
      double coveredDistance = distanceM * x;
      
      SGGeodesy::direct(dep->geod(), course, coveredDistance, position, az2);
      
      SG_LOG (SG_GENERAL, SG_BULK, "Traffic Manager:      Flight is in progress, %=" << x);
      speed = ((distanceM - coveredDistance) * SG_METER_TO_NM) / 3600.0;
    } else {
    // not departed yet
      //remainingTimeEnroute = totalTimeEnroute;
      elapsedTimeEnroute = 0;
      position = dep->geod();
      SG_LOG (SG_GENERAL, SG_BULK, "Traffic Manager:      Flight is pending, departure in "
        << flight->getDepartureTime() - now << " seconds ");
    }
  } else {
    // departure / arrival coincident
    //remainingTimeEnroute = totalTimeEnroute = 0.0;
    elapsedTimeEnroute = 0;
    position = dep->geod();
  }
    
  // cartesian calculations are more numerically stable over the (potentially)
  // large distances involved here: see bug #80
  distanceToUser = dist(userCart, SGVec3d::fromGeod(position)) * SG_METER_TO_NM;

  // If distance between user and simulated aircaft is less
  // then 500nm, create this flight. At jet speeds 500 nm is roughly
  // one hour flight time, so that would be a good approximate point
  // to start a more detailed simulation of this aircraft.
  SG_LOG (SG_GENERAL, SG_BULK, "Traffic manager: " << registration << " is scheduled for a flight from " 
	     << dep->getId() << " to " << arr->getId() << ". Current distance to user: " 
             << distanceToUser);
  if (distanceToUser >= TRAFFICTOAIDISTTOSTART) {
    return true; // out of visual range, for the moment.
  }
  return createAIAircraft(flight, speed, deptime);
}

bool FGAISchedule::validModelPath(const std::string& modelPath)
{
    SGPath mp(globals->get_fg_root());
    SGPath mp_ai = mp;

    mp.append(modelPath);
    mp_ai.append("AI");
    mp_ai.append(modelPath);

    return mp.exists() || mp_ai.exists();
}

bool FGAISchedule::createAIAircraft(FGScheduledFlight* flight, double speedKnots, time_t deptime)
{
  FGAirport* dep = flight->getDepartureAirport();
  FGAirport* arr = flight->getArrivalAirport();
  string flightPlanName = dep->getId() + "-" + arr->getId() + ".xml";
  SG_LOG(SG_GENERAL, SG_INFO, "Traffic manager: Creating AIModel from:" << flightPlanName);

  // Only allow traffic to be created when the model path (or the AI version of mp) exists
  SGPath mp(globals->get_fg_root());
  SGPath mp_ai = mp;

  mp.append(modelPath);
  mp_ai.append("AI");
  mp_ai.append(modelPath);

  if (!mp.exists() && !mp_ai.exists()) {
    SG_LOG(SG_GENERAL, SG_WARN, "TrafficManager: Could not load model " << mp_ai.str());
    return true;
  }

  FGAIAircraft *aircraft = new FGAIAircraft(this);
  aircraft->setPerformance(m_class); //"jet_transport";
  aircraft->setCompany(airline); //i->getAirline();
  aircraft->setAcType(acType); //i->getAcType();
  aircraft->setPath(modelPath.c_str());
  //aircraft->setFlightPlan(flightPlanName);
  aircraft->setLatitude(position.getLatitudeDeg());
  aircraft->setLongitude(position.getLongitudeDeg());
  aircraft->setAltitude(flight->getCruiseAlt()*100); // convert from FL to feet
  aircraft->setSpeed(0);
  aircraft->setBank(0);
      
  courseToDest = SGGeodesy::courseDeg(position, arr->geod());
  FGAIFlightPlan *fp = new FGAIFlightPlan(aircraft, flightPlanName, courseToDest, deptime,
                                            dep, arr, true, radius, 
                                            flight->getCruiseAlt()*100, 
                                            position.getLatitudeDeg(), 
                                            position.getLongitudeDeg(), 
                                            speedKnots, flightType, acType, 
                                            airline);
  if (fp->isValidPlan()) {
        aircraft->SetFlightPlan(fp);
        FGAIManager* aimgr = (FGAIManager *) globals-> get_subsystem("ai-model");
        aimgr->attach(aircraft);
        AIManagerRef = aircraft->getID();
        return true;
  } else {
        delete aircraft;
        delete fp;
        //hand back the flights that had already been scheduled
        while (!flights.empty()) {
            flights.front()->release();
            flights.erase(flights.begin());
        }
        return false;
  }
}

// Create an initial heading for user controlled aircraft.
void FGAISchedule::setHeading()  
{ 
    courseToDest = SGGeodesy::courseDeg((*flights.begin())->getDepartureAirport()->geod(), (*flights.begin())->getArrivalAirport()->geod());
}

void FGAISchedule::scheduleFlights(time_t now)
{
  if (!flights.empty()) {
    return;
  }
  //string startingPort;
  string userPort = fgGetString("/sim/presets/airport-id");
  SG_LOG(SG_GENERAL, SG_BULK, "Scheduling Flights for : " << modelPath << " " <<  registration << " " << homePort);
  FGScheduledFlight *flight = NULL;
  do {
    if (currentDestination.empty()) {
        flight = findAvailableFlight(userPort, flightIdentifier, now, (now+6400));
        if (!flight)
            flight = findAvailableFlight(currentDestination, flightIdentifier);
    } else {
        flight = findAvailableFlight(currentDestination, flightIdentifier);
    }
    if (!flight) {
      break;
    }
    currentDestination = flight->getArrivalAirport()->getId();
    //cerr << "Current destination " <<  currentDestination << endl;
    if (!initialized) {
        string departurePort = flight->getDepartureAirport()->getId();
        if (userPort == departurePort) {
            lastRun = 1;
            hits++;
        } else {
            lastRun = 0;
        }
        //runCount++;
        initialized = true;
    }
  
    time_t arr, dep;
    dep = flight->getDepartureTime();
    arr = flight->getArrivalTime();
    string depT = asctime(gmtime(&dep));
    string arrT = asctime(gmtime(&arr));

    depT = depT.substr(0,24);
    arrT = arrT.substr(0,24);
    SG_LOG(SG_GENERAL, SG_BULK, "  Flight " << flight->getCallSign() << ":" 
                             << "  "        << flight->getDepartureAirport()->getId() << ":"
                             << "  "        << depT << ":"
                             << " \""       << flight->getArrivalAirport()->getId() << "\"" << ":"
                             << "  "        << arrT << ":");
  
    flights.push_back(flight);
  } while (currentDestination != homePort);
  SG_LOG(SG_GENERAL, SG_BULK, " Done ");
}

bool FGAISchedule::next()
{
  if (!flights.empty()) {
    flights.front()->release();
    flights.erase(flights.begin());
  }
  
  FGScheduledFlight *flight = findAvailableFlight(currentDestination, flightIdentifier);
  if (!flight) {
    return false;
  }
  
  currentDestination = flight->getArrivalAirport()->getId();
/*
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
*/
   flights.push_back(flight);
   return true;
}

FGScheduledFlight* FGAISchedule::findAvailableFlight (const string &currentDestination,
                                                      const string &req,
                                                     time_t min, time_t max)
{
    time_t now = time(NULL) + fgGetLong("/sim/time/warp");

    FGTrafficManager *tmgr = (FGTrafficManager *) globals->get_subsystem("traffic-manager");
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
     std::sort(fltBegin, fltEnd, compareScheduledFlights);
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
          if (flights.size()) {
            time_t arrival = flights.back()->getArrivalTime();
            int groundTime = groundTimeFromRadius();
            if ((*i)->getDepartureTime() < (arrival+(groundTime)))
                continue;
          }
          if (min != 0) {
              time_t dep = (*i)->getDepartureTime();
              if ((dep < min) || (dep > max))
                  continue;
          }

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
     return NULL;
}

int FGAISchedule::groundTimeFromRadius()
{
    if (radius < 10) 
        return 15 * 60;
    else if (radius < 15)
        return 20 * 60;
    else if (radius < 20)
        return 30 * 60;
    else if (radius < 25)
        return 50 * 60;
    else if (radius < 30)
        return 90 * 60;
    else 
        return 120 * 60;
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

void FGAISchedule::setScore   () 
{ 
    if (runCount) {
        score = ((double) hits / (double) runCount);
    } else {
        if (homePort == fgGetString("/sim/presets/airport-id")) {
            score = 0.1;
        } else {
            score = 0.0;
        }
    }
    runCount++;
}

bool compareSchedules(FGAISchedule*a, FGAISchedule*b)
{ 
  return (*a) < (*b); 
} 

bool FGAISchedule::operator< (const FGAISchedule &other) const
{ 
    //cerr << "Sorting " << registration << " and "  << other.registration << endl;
    double currentScore = score       * (1.5 - lastRun);
    double otherScore   = other.score * (1.5 - other.lastRun);
    return currentScore > otherScore;
}

