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
#include <boost/foreach.hpp>

#include <simgear/compiler.h>
#include <simgear/sg_inlines.h>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/xml/easyxml.hxx>

#include <AIModel/AIFlightPlan.hxx>
#include <AIModel/AIManager.hxx>
#include <AIModel/AIAircraft.hxx>
#include <Airports/airport.hxx>
#include <Main/fg_init.hxx>   // That's pretty ugly, but I need fgFindAirportID


#include "SchedFlight.hxx"
#include "TrafficMgr.hxx"

using std::string;

/******************************************************************************
 * the FGAISchedule class contains data members and code to maintain a
 * schedule of Flights for an artificially controlled aircraft.
 *****************************************************************************/
FGAISchedule::FGAISchedule()
  : heavy(false),
    radius(0),
    groundOffset(0),
    distanceToUser(0),
    score(0),
    runCount(0),
    hits(0),
    lastRun(0),
    firstRun(false),
    courseToDest(0),
    initialized(false),
    valid(false),
    scheduleComplete(false)
{
}


FGAISchedule::FGAISchedule(const string& model,
                           const string& lvry,
                           const string& port,
                           const string& reg,
                           const string& flightId,
                           bool   hvy, 
                           const string& act,
                           const string& arln,
                           const string& mclass,
                           const string& fltpe,
                           double rad, 
                           double grnd)
    : heavy(hvy),
      radius(rad),
      groundOffset(grnd),
      distanceToUser(0),
      score(0),
      runCount(0),
      hits(0),
      lastRun(0),
      firstRun(true),
      courseToDest(0),
      initialized(false),
      valid(true),
      scheduleComplete(false)
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
  /*for (FGScheduledFlightVecIterator i = flt.begin();
       i != flt.end();
       i++)
    flights.push_back(new FGScheduledFlight((*(*i))));*/
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
  aiAircraft         = other.aiAircraft;
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
  courseToDest       = other.courseToDest;
  initialized        = other.initialized;
  valid              = other.valid;
  scheduleComplete   = other.scheduleComplete;
}



FGAISchedule::~FGAISchedule()
{
    // remove related object from AI manager
    if (aiAircraft)
    {
      aiAircraft->setDie(true);
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

/**
 *  Returns true when processing is complete.
 *  Returns false when processing was aborted due to timeout, so
 *    more time required - and another call is requested (next sim iteration).
 */
bool FGAISchedule::update(time_t now, const SGVec3d& userCart)
{

  time_t totalTimeEnroute,
         elapsedTimeEnroute,
         //remainingTimeEnroute,
         deptime = 0;

  if (!valid) {
    return true; // processing complete
  }

  if (!scheduleComplete) {
      scheduleComplete = scheduleFlights(now);
  }

  if (!scheduleComplete) {
      return false; // not ready yet, continue processing in next iteration
  }

  if (flights.empty()) { // No flights available for this aircraft
      valid = false;
      return true; // processing complete
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
    //cerr << "Setting departure time " << deptime << endl;
  }
    
  if (aiAircraft) {
    if (aiAircraft->getDie()) {
      aiAircraft = NULL;
    } else {
      return true; // in visual range, let the AIManager handle it
    }
  }
  
  // This flight entry is entirely in the past, do we need to 
  // push it forward in time to the next scheduled departure. 
  if (flight->getArrivalTime() < now) {
    SG_LOG (SG_AI, SG_BULK, "Traffic Manager:      Flight is in the Past");
    // Don't just update: check whether we need to load a new leg. etc.
    // This update occurs for distant aircraft, so we can update the current leg
    // and detach it from the current list of aircraft. 
    flight->update();
    flights.erase(flights.begin()); // pop_front(), effectively
    return true; // processing complete
  }
  
  FGAirport* dep = flight->getDepartureAirport();
  FGAirport* arr = flight->getArrivalAirport();
  if (!dep || !arr) {
    return true; // processing complete
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
      
      SG_LOG (SG_AI, SG_BULK, "Traffic Manager:      Flight is in progress, %=" << x);
      speed = ((distanceM - coveredDistance) * SG_METER_TO_NM) / 3600.0;
    } else {
    // not departed yet
      //remainingTimeEnroute = totalTimeEnroute;
      elapsedTimeEnroute = 0;
      position = dep->geod();
      SG_LOG (SG_AI, SG_BULK, "Traffic Manager:      Flight is pending, departure in "
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


  // If distance between user and simulated aircraft is less
  // then 500nm, create this flight. At jet speeds 500 nm is roughly
  // one hour flight time, so that would be a good approximate point
  // to start a more detailed simulation of this aircraft.
  SG_LOG (SG_AI, SG_BULK, "Traffic manager: " << registration << " is scheduled for a flight from "
	     << dep->getId() << " to " << arr->getId() << ". Current distance to user: " 
             << distanceToUser);
  if (distanceToUser >= TRAFFICTOAIDISTTOSTART) {
    return true; // out of visual range, for the moment.
  }

  if (!createAIAircraft(flight, speed, deptime)) {
      valid = false;
  }


    return true; // processing complete
}

bool FGAISchedule::validModelPath(const std::string& modelPath)
{
    return (resolveModelPath(modelPath) != SGPath());
}

SGPath FGAISchedule::resolveModelPath(const std::string& modelPath)
{
    BOOST_FOREACH(SGPath aiPath, globals->get_data_paths("AI")) {
        aiPath.append(modelPath);
        if (aiPath.exists()) {
            return aiPath;
        }
    }
    
    // check aircraft dirs
    BOOST_FOREACH(std::string aircraftPath, globals->get_aircraft_paths()) {
        SGPath mp(aircraftPath);
        mp.append(modelPath);
        if (mp.exists()) {
            return mp;
        }
    }

    return SGPath();
}

bool FGAISchedule::createAIAircraft(FGScheduledFlight* flight, double speedKnots, time_t deptime)
{
  FGAirport* dep = flight->getDepartureAirport();
  FGAirport* arr = flight->getArrivalAirport();
  string flightPlanName = dep->getId() + "-" + arr->getId() + ".xml";
  SG_LOG(SG_AI, SG_DEBUG, "Traffic manager: Creating AIModel from:" << flightPlanName);

  aiAircraft = new FGAIAircraft(this);
  aiAircraft->setPerformance(acType, m_class); //"jet_transport";
  aiAircraft->setCompany(airline); //i->getAirline();
  aiAircraft->setAcType(acType); //i->getAcType();
  aiAircraft->setPath(modelPath.c_str());
  //aircraft->setFlightPlan(flightPlanName);
  aiAircraft->setLatitude(position.getLatitudeDeg());
  aiAircraft->setLongitude(position.getLongitudeDeg());
  aiAircraft->setAltitude(flight->getCruiseAlt()*100); // convert from FL to feet
  aiAircraft->setSpeed(0);
  aiAircraft->setBank(0);
      
  courseToDest = SGGeodesy::courseDeg(position, arr->geod());
  FGAIFlightPlan *fp = new FGAIFlightPlan(aiAircraft, flightPlanName, courseToDest, deptime,
                                            dep, arr, true, radius, 
                                            flight->getCruiseAlt()*100, 
                                            position.getLatitudeDeg(), 
                                            position.getLongitudeDeg(), 
                                            speedKnots, flightType, acType, 
                                            airline);
  if (fp->isValidPlan()) {
        aiAircraft->SetFlightPlan(fp);
        FGAIManager* aimgr = (FGAIManager *) globals-> get_subsystem("ai-model");
        aimgr->attach(aiAircraft);
        return true;
  } else {
        aiAircraft = NULL;
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

bool FGAISchedule::scheduleFlights(time_t now)
{
  //string startingPort;
  const string& userPort = fgGetString("/sim/presets/airport-id");
  SG_LOG(SG_AI, SG_BULK, "Scheduling Flights for : " << modelPath << " " <<  registration << " " << homePort);
  FGScheduledFlight *flight = NULL;
  SGTimeStamp start;
  start.stamp();

  bool first = true;
  if (currentDestination.empty())
    flight = findAvailableFlight(userPort, flightIdentifier, now, (now+6400));

  do {
    if ((!flight)||(!first)) {
        flight = findAvailableFlight(currentDestination, flightIdentifier);
    }
    if (!flight) {
      break;
    }

    first = false;
    currentDestination = flight->getArrivalAirport()->getId();
    //cerr << "Current destination " <<  currentDestination << endl;
    if (!initialized) {
        const string& departurePort = flight->getDepartureAirport()->getId();
        if (userPort == departurePort) {
            lastRun = 1;
            hits++;
        } else {
            lastRun = 0;
        }
        //runCount++;
        initialized = true;
    }
  
    if (sglog().would_log(SG_AI, SG_BULK))
    {
        time_t arr, dep;
        dep = flight->getDepartureTime();
        arr = flight->getArrivalTime();
        string depT = asctime(gmtime(&dep));
        string arrT = asctime(gmtime(&arr));
        depT = depT.substr(0,24);
        arrT = arrT.substr(0,24);
        SG_LOG(SG_AI, SG_BULK, "  Flight " << flight->getCallSign() << ":"
                                 << "  "        << flight->getDepartureAirport()->getId() << ":"
                                 << "  "        << depT << ":"
                                 << " \""       << flight->getArrivalAirport()->getId() << "\"" << ":"
                                 << "  "        << arrT << ":");
    }
  
    flights.push_back(flight);

    // continue processing until complete, or preempt after timeout
  } while ((currentDestination != homePort)&&
           (start.elapsedMSec()<3.0));

  if (flight && (currentDestination != homePort))
  {
      // processing preempted, need to continue in next iteration
      return false;
  }

  SG_LOG(SG_AI, SG_BULK, " Done ");
  return true;
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
          if (! flights.empty()) {
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

