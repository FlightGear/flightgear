// dynamics.cxx - Code to manage the higher order airport ground activities
// Written by Durk Talsma, started December 2004.
//
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
#  include <config.h>
#endif

#include <algorithm>
#include <string>
#include <vector>

#include <boost/foreach.hpp>

#include <simgear/compiler.h>

#include <Environment/environment_mgr.hxx>
#include <Environment/environment.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/debug/logstream.hxx>
#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <Airports/runways.hxx>
#include <ATCDCL/ATCutils.hxx>

#include "simple.hxx"
#include "dynamics.hxx"

using std::string;
using std::vector;
using std::sort;
using std::random_shuffle;

FGAirportDynamics::FGAirportDynamics(FGAirport * ap):
    _ap(ap), rwyPrefs(ap),
    startupController    (this),
    towerController      (this),
    approachController   (this),
    atisSequenceIndex(-1),
    atisSequenceTimeStamp(0.0)

{
    lastUpdate = 0;
}

// Destructor
FGAirportDynamics::~FGAirportDynamics()
{
}


// Initialization required after XMLRead
void FGAirportDynamics::init()
{
    // This may seem a bit weird to first randomly shuffle the parkings
    // and then sort them again. However, parkings are sorted here by ascending 
    // radius. Since many parkings have similar radii, with each radius class they will
    // still be allocated relatively systematically. Randomizing prior to sorting will
    // prevent any initial orderings to be destroyed, leading (hopefully) to a more 
    // naturalistic gate assignment. 
    random_shuffle(parkings.begin(), parkings.end());
    sort(parkings.begin(), parkings.end());
    // add the gate positions to the ground network. 
    groundNetwork.setParent(_ap);
    groundNetwork.addNodes(&parkings);
    groundNetwork.init();
    groundNetwork.setTowerController(&towerController);
    
}

int FGAirportDynamics::innerGetAvailableParking(double radius, const string & flType,
                                           const string & acType,
                                           const string & airline,
                                           bool skipEmptyAirlineCode)
{
  BOOST_FOREACH(FGParking* i, parkings) {
    // Taken by another aircraft, or no airline codes
    if (!i->isAvailable()) {
      continue;
    }
    
    if (skipEmptyAirlineCode && i->getCodes().empty()) {
      continue;
    }
    
    // check airline codes match
    if (!airline.empty() && !i->getCodes().empty()) {
      if (i->getCodes().find(airline, 0) == string::npos) {
        continue;
      }
    }
    
    // Type doesn't match
    if (i->getType() != flType) {
      continue;
    }
    // too small
    if (i->getRadius() < radius) {
      continue;
    }
    
    i->setAvailable(false);
    return i->getIndex();
  }
  
  return -1;
}

int FGAirportDynamics::getAvailableParking(double radius, const string & flType,
                                            const string & acType,
                                            const string & airline)
{
    if (parkings.empty()) {
      return -1;
    }
  
  // most exact seach - airline codes must be present and match
  int result = innerGetAvailableParking(radius, flType, acType, airline, true);
  if (result >= 0) {
    return result;
  }
  
  // more tolerant - gates with empty airline codes are permitted
  result = innerGetAvailableParking(radius, flType, acType, airline, false);
  if (result >= 0) {
    return result;
  }

  // fallback - ignore the airline code entirely
  return innerGetAvailableParking(radius, flType, acType, string(), false);
}

FGParking *FGAirportDynamics::getParking(int id)
{
  BOOST_FOREACH(FGParking* i, parkings) {
    if (id == i->getIndex()) {
      return i;
    }
  }
  
  return NULL;
}

string FGAirportDynamics::getParkingName(int id)
{
  FGParking* p = getParking(id);
  if (p) {
    return p->getName();
  }
  
  return string();
}

int FGAirportDynamics::findParkingByName(const std::string& name) const
{
  BOOST_FOREACH(FGParking* i, parkings) {
    if (name == i->getName()) {
      return i->getIndex();
    }
  }

  return -1;
}

void FGAirportDynamics::releaseParking(int id)
{
    if (id >= 0) {
      FGParking* parking = getParking(id);
      if (parking) {
        parking->setAvailable(true);
      }
    }
}

void FGAirportDynamics::setRwyUse(const FGRunwayPreference & ref)
{
    rwyPrefs = ref;
}

bool FGAirportDynamics::innerGetActiveRunway(const string & trafficType,
                                             int action, string & runway,
                                             double heading)
{
    double windSpeed;
    double windHeading;
    double maxTail;
    double maxCross;
    string name;
    string type;

    if (!rwyPrefs.available()) {
        return false;
    }

    RunwayGroup *currRunwayGroup = 0;
    int nrActiveRunways = 0;
    time_t dayStart = fgGetLong("/sim/time/utc/day-seconds");
    if ((abs((long) (dayStart - lastUpdate)) > 600)
        || trafficType != prevTrafficType) {
        landing.clear();
        takeoff.clear();
        lastUpdate = dayStart;
        prevTrafficType = trafficType;
        /*
        FGEnvironment
            stationweather =
            ((FGEnvironmentMgr *) globals->get_subsystem("environment"))
            ->getEnvironment(getLatitude(), getLongitude(),
                             getElevation());
        */
        windSpeed   = fgGetInt("/environment/metar/base-wind-speed-kt"); //stationweather.get_wind_speed_kt();
        windHeading = fgGetInt("/environment/metar/base-wind-dir-deg");
        //stationweather.get_wind_from_heading_deg();
        string scheduleName;
        //cerr << "finding active Runway for : " << _ap->getId() << endl;
        //cerr << "Wind Heading              : " << windHeading << endl;
        //cerr << "Wind Speed                : " << windSpeed << endl;

        //cerr << "Nr of seconds since day start << " << dayStart << endl;

        ScheduleTime *currSched;
        //cerr << "A"<< endl;
        currSched = rwyPrefs.getSchedule(trafficType.c_str());
        if (!(currSched))
            return false;
        //cerr << "B"<< endl;
        scheduleName = currSched->getName(dayStart);
        maxTail = currSched->getTailWind();
        maxCross = currSched->getCrossWind();
        //cerr << "Current Schedule =        : " << scheduleName << endl;
        if (scheduleName.empty())
            return false;
        //cerr << "C"<< endl;
        currRunwayGroup = rwyPrefs.getGroup(scheduleName);
        //cerr << "D"<< endl;
        if (!(currRunwayGroup))
            return false;
        nrActiveRunways = currRunwayGroup->getNrActiveRunways();

        // Keep a history of the currently active runways, to ensure
        // that an already established selection of runways will not
        // be overridden once a more preferred selection becomes 
        // available as that can lead to random runway swapping.
        if (trafficType == "com") {
            currentlyActive = &comActive;
        } else if (trafficType == "gen") {
            currentlyActive = &genActive;
        } else if (trafficType == "mil") {
            currentlyActive = &milActive;
        } else if (trafficType == "ul") {
            currentlyActive = &ulActive;
        }

        //cerr << "Durrently active selection for " << trafficType << ": ";
        for (stringVecIterator it = currentlyActive->begin();
             it != currentlyActive->end(); it++) {
             //cerr << (*it) << " ";
         }
         //cerr << endl;

        currRunwayGroup->setActive(_ap,
                                   windSpeed,
                                   windHeading,
                                   maxTail, maxCross, currentlyActive);

        // Note that I SHOULD keep multiple lists in memory, one for 
        // general aviation, one for commercial and one for military
        // traffic.
        currentlyActive->clear();
        nrActiveRunways = currRunwayGroup->getNrActiveRunways();
        //cerr << "Choosing runway for " << trafficType << endl;
        for (int i = 0; i < nrActiveRunways; i++) {
            type = "unknown";   // initialize to something other than landing or takeoff
            currRunwayGroup->getActive(i, name, type);
            if (type == "landing") {
                landing.push_back(name);
                currentlyActive->push_back(name);
                //cerr << "Landing " << name << endl; 
            }
            if (type == "takeoff") {
                takeoff.push_back(name);
                currentlyActive->push_back(name);
                //cerr << "takeoff " << name << endl;
            }
        }
        //cerr << endl;
    }

    if (action == 1)            // takeoff 
    {
        int nr = takeoff.size();
        if (nr) {
            // Note that the randomization below, is just a placeholder to choose between
            // multiple active runways for this action. This should be
            // under ATC control.
            runway = chooseRwyByHeading(takeoff, heading);
        } else {                // Fallback
            runway = chooseRunwayFallback();
        }
    }

    if (action == 2)            // landing
    {
        int nr = landing.size();
        if (nr) {
            runway = chooseRwyByHeading(landing, heading);
        } else {                //fallback
            runway = chooseRunwayFallback();
        }
    }

    return true;
}

string FGAirportDynamics::chooseRwyByHeading(stringVec rwys,
                                             double heading)
{
    double bestError = 360.0;
    double rwyHeading, headingError;
    string runway;
    for (stringVecIterator i = rwys.begin(); i != rwys.end(); i++) {
        if (!_ap->hasRunwayWithIdent(*i)) {
          SG_LOG(SG_ATC, SG_WARN, "chooseRwyByHeading: runway " << *i <<
            " not found at " << _ap->ident());
          continue;
        }
        
        FGRunway *rwy = _ap->getRunwayByIdent((*i));
        rwyHeading = rwy->headingDeg();
        headingError = fabs(heading - rwyHeading);
        if (headingError > 180)
            headingError = fabs(headingError - 360);
        if (headingError < bestError) {
            runway = (*i);
            bestError = headingError;
        }
    }
    //cerr << "Using active runway " << runway << " for heading " << heading << endl;
    return runway;
}

void FGAirportDynamics::getActiveRunway(const string & trafficType,
                                        int action, string & runway,
                                        double heading)
{
    bool ok = innerGetActiveRunway(trafficType, action, runway, heading);
    if (!ok) {
        runway = chooseRunwayFallback();
    }
}

string FGAirportDynamics::chooseRunwayFallback()
{
    FGRunway *rwy = _ap->getActiveRunwayForUsage();
    return rwy->ident();
}

void FGAirportDynamics::addParking(FGParking* park)
{
    parkings.push_back(park);
}

double FGAirportDynamics::getElevation() const
{
    return _ap->getElevation();
}

const string FGAirportDynamics::getId() const
{
    return _ap->getId();
}

// Experimental: Return a different ground frequency depending on the leg of the
// Flight. Leg should always have a minimum value of two when this function is called. 
// Note that in this scheme, the assignment of various frequencies to various ground 
// operations is completely arbitrary. As such, is a short cut I need to take now,
// so that at least I can start working on assigning different frequencies to different
// operations.

int FGAirportDynamics::getGroundFrequency(unsigned leg)
{
    //return freqGround.size() ? freqGround[0] : 0; };
    //cerr << "Getting frequency for : " << leg << endl;
    int groundFreq = 0;
    if (leg < 1) {
        SG_LOG(SG_ATC, SG_ALERT,
               "Leg value is smaller than one at " << SG_ORIGIN);
    }
    if (freqGround.size() == 0) {
        return 0;
    }

    if ((freqGround.size() < leg) && (leg > 0)) {
        groundFreq =
            (freqGround.size() <=
             (leg - 1)) ? freqGround[freqGround.size() -
                                     1] : freqGround[leg - 1];
    }
    if ((freqGround.size() >= leg) && (leg > 0)) {
        groundFreq = freqGround[leg - 1];
    }
    return groundFreq;
}

int FGAirportDynamics::getTowerFrequency(unsigned nr)
{
    int towerFreq = 0;
    if (nr < 2) {
        SG_LOG(SG_ATC, SG_ALERT,
               "Leg value is smaller than two at " << SG_ORIGIN);
    }
    if (freqTower.size() == 0) {
        return 0;
    }
    if ((freqTower.size() > nr - 1) && (nr > 1)) {
        towerFreq = freqTower[nr - 1];
    }
    if ((freqTower.size() < nr - 1) && (nr > 1)) {
        towerFreq =
            (freqTower.size() <
             (nr - 1)) ? freqTower[freqTower.size() -
                                     1] : freqTower[nr - 2];
    }
    if ((freqTower.size() >= nr - 1) && (nr > 1)) {
        towerFreq = freqTower[nr - 2];
    }
    return towerFreq;
}

const std::string FGAirportDynamics::getAtisSequence()
{
   if (atisSequenceIndex == -1) {
       updateAtisSequence(1, false);
   }
   
   return GetPhoneticLetter(atisSequenceIndex);
}

int FGAirportDynamics::updateAtisSequence(int interval, bool forceUpdate)
{
    double now = globals->get_sim_time_sec();
    if (atisSequenceIndex == -1) {
        // first computation
        atisSequenceTimeStamp = now;
        atisSequenceIndex = rand() % LTRS; // random initial sequence letters
        return atisSequenceIndex;
    }

    int steps = static_cast<int>((now - atisSequenceTimeStamp) / interval);
    atisSequenceTimeStamp += (interval * steps);
    if (forceUpdate && (steps == 0)) {
        ++steps; // a "special" ATIS update is required
    } 
    
    atisSequenceIndex = (atisSequenceIndex + steps) % LTRS;
    // return a huge value if no update occurred
    return (atisSequenceIndex + (steps ? 0 : LTRS*1000));
}
