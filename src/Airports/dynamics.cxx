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
#include <Main/locale.hxx>
#include <Airports/runways.hxx>
#include <Airports/groundnetwork.hxx>
#include <Navaids/NavDataCache.hxx>

#include "airport.hxx"
#include "dynamics.hxx"

using std::string;
using std::vector;
using std::sort;
using std::random_shuffle;

class ParkingAssignment::ParkingAssignmentPrivate
{
public:
  ParkingAssignmentPrivate(FGParking* pk, FGAirportDynamics* dyn) :
    refCount(0),
    parking(pk),
    dynamics(dyn)
  {
    assert(pk);
    assert(dyn);
    retain(); // initial count of 1
  }
  
  ~ParkingAssignmentPrivate()
  {
    dynamics->releaseParking(parking);
  }
  
  void release()
  {
    if ((--refCount) == 0) {
      delete this;
    }
  }
  
  void retain()
  {
    ++refCount;
  }
  
  unsigned int refCount;
  FGParkingRef parking;
  FGAirportDynamicsRef dynamics;
};

ParkingAssignment::ParkingAssignment() :
  _sharedData(NULL)
{
}

ParkingAssignment::~ParkingAssignment()
{
  if (_sharedData) {
    _sharedData->release();
  }
}
  
ParkingAssignment::ParkingAssignment(FGParking* pk, FGAirportDynamics* dyn) :
  _sharedData(NULL)
{
  if (pk) {
    _sharedData = new ParkingAssignmentPrivate(pk, dyn);
  }
}

ParkingAssignment::ParkingAssignment(const ParkingAssignment& aOther) :
  _sharedData(aOther._sharedData)
{
  if (_sharedData) {
    _sharedData->retain();
  }
}

void ParkingAssignment::operator=(const ParkingAssignment& aOther)
{
  if (_sharedData == aOther._sharedData) {
    return; // self-assignment, special case
  }
  
  if (_sharedData) {
    _sharedData->release();
  }
  
  _sharedData = aOther._sharedData;
  if (_sharedData) {
    _sharedData->retain();
  }
}
  
void ParkingAssignment::release()
{
  if (_sharedData) {
    _sharedData->release();
    _sharedData = NULL;
  }
}

bool ParkingAssignment::isValid() const
{
  return (_sharedData != NULL);
}

FGParking* ParkingAssignment::parking() const
{
  return _sharedData ? _sharedData->parking.ptr() : NULL;
}

////////////////////////////////////////////////////////////////////////////////

FGAirportDynamics::FGAirportDynamics(FGAirport * ap):
    _ap(ap), rwyPrefs(ap),
    startupController    (this),
    towerController      (this),
    approachController   (this),
    atisSequenceIndex(-1),
    atisSequenceTimeStamp(0.0)

{
    lastUpdate = 0;
    groundNetwork.reset(new FGGroundNetwork);
}

// Destructor
FGAirportDynamics::~FGAirportDynamics()
{
    SG_LOG(SG_AI, SG_INFO, "destroyed dynamics for:" << _ap->ident());
}


// Initialization required after XMLRead
void FGAirportDynamics::init()
{

    groundNetwork->init(this);
    groundController.setTowerController(&towerController);
    groundController.init(this);
}

FGParking* FGAirportDynamics::innerGetAvailableParking(double radius, const string & flType,
                                           const string & airline,
                                           bool skipEmptyAirlineCode)
{
    const FGParkingList& parkings(groundNetwork->allParkings());
    FGParkingList::const_iterator it;
    for (it = parkings.begin(); it != parkings.end(); ++it) {
        FGParkingRef parking = *it;
        if (!isParkingAvailable(parking)) {
          continue;
        }

        if (skipEmptyAirlineCode && parking->getCodes().empty()) {
          continue;
        }

        if (!airline.empty() && !parking->getCodes().empty()) {
          if (parking->getCodes().find(airline, 0) == string::npos) {
            continue;
          }
        }

        setParkingAvailable(parking, false);
        return parking;
    }

    return NULL;
}

bool FGAirportDynamics::hasParkings() const
{
    return !groundNetwork->allParkings().empty();
}

ParkingAssignment FGAirportDynamics::getAvailableParking(double radius, const string & flType,
                                            const string & acType,
                                            const string & airline)
{
  SG_UNUSED(acType); // sadly not used at the moment
  
  // most exact seach - airline codes must be present and match
  FGParking* result = innerGetAvailableParking(radius, flType, airline, true);
  if (result) {
    return ParkingAssignment(result, this);
  }
  
  // more tolerant - gates with empty airline codes are permitted
  result = innerGetAvailableParking(radius, flType, airline, false);
  if (result) {
    return ParkingAssignment(result, this);
  }

  // fallback - ignore the airline code entirely
  result = innerGetAvailableParking(radius, flType, string(), false);
  return result ? ParkingAssignment(result, this) : ParkingAssignment();
}

ParkingAssignment FGAirportDynamics::getParkingByName(const std::string& name) const
{
    const FGParkingList& parkings(groundNetwork->allParkings());
    FGParkingList::const_iterator it;
    for (it = parkings.begin(); it != parkings.end(); ++it) {
        if ((*it)->name() == name) {
            return ParkingAssignment(*it, const_cast<FGAirportDynamics*>(this));
        }
    }

  return ParkingAssignment();
}

void FGAirportDynamics::setParkingAvailable(FGParking* park, bool available)
{
  if (available) {
    releaseParking(park);
  } else {
    occupiedParkings.insert(park);
  }
}

bool FGAirportDynamics::isParkingAvailable(FGParking* parking) const
{
  return (occupiedParkings.find(parking) == occupiedParkings.end());
}

void FGAirportDynamics::releaseParking(FGParking* id)
{
  ParkingSet::iterator it = occupiedParkings.find(id);
  if (it == occupiedParkings.end()) {
    return;
  }
  
  occupiedParkings.erase(it);
}

class GetParkingsPredicate
{
    bool mustBeAvailable;
    std::string type;
    const FGAirportDynamics* dynamics;
public:
    GetParkingsPredicate(bool b, const std::string& ty, const FGAirportDynamics* dyn) :
        mustBeAvailable(b),
        type(ty),
        dynamics(dyn)
    {}

    bool operator()(const FGParkingRef& park) const
    {
        if (!type.empty() && (park->getType() != type))
            return true;

        if (mustBeAvailable && !dynamics->isParkingAvailable(park)) {
            return true;
        }

        return false;
    }
};

FGParkingList FGAirportDynamics::getParkings(bool onlyAvailable, const std::string &type) const
{
    FGParkingList result(groundNetwork->allParkings());

    GetParkingsPredicate pred(onlyAvailable, type, this);
    FGParkingList::iterator it = std::remove_if(result.begin(), result.end(), pred);
    result.erase(it, result.end());
    return result;
}

void FGAirportDynamics::setRwyUse(const FGRunwayPreference & ref)
{
    rwyPrefs = ref;
}

bool areRunwaysParallel(const FGRunwayRef& a, const FGRunwayRef& b)
{
    double hdgDiff = (b->headingDeg() - a->headingDeg());
    SG_NORMALIZE_RANGE(hdgDiff, -180.0, 180.0);
    return (fabs(hdgDiff) < 5.0);
}

double runwayScore(const FGRunwayRef& rwy)
{
    return rwy->lengthM() + rwy->widthM();
}

double runwayWindScore(const FGRunwayRef& runway, double windHeading,
                       double windSpeedKts)
{
    double hdgDiff = fabs(windHeading - runway->headingDeg()) * SG_DEGREES_TO_RADIANS;
    SGMiscd::normalizeAngle(hdgDiff);

    double crossWind = windSpeedKts * sin(hdgDiff);
    double tailWind = -windSpeedKts * cos(hdgDiff);

    return -(crossWind + tailWind);
}

typedef std::vector<FGRunwayRef> RunwayVec;


class FallbackRunwayGroup
{
public:
    FallbackRunwayGroup(const FGRunwayRef& rwy) :
        _groupScore(0.0)
    {
        runways.push_back(rwy);
        _leadRunwayScore = runwayScore(rwy);
        _groupScore += _leadRunwayScore;
    }

    bool canAccept(const FGRunwayRef& rwy) const
    {
        if (!areRunwaysParallel(runways.front(), rwy)) {
            return false;
        }

        return true;
    }

    void addRunway(const FGRunwayRef& rwy)
    {
        assert(areRunwaysParallel(runways.front(), rwy));
        double score = runwayScore(rwy);
        if (score < (0.5 * _leadRunwayScore)) {
            // drop the runway completely. this is to drop short parallel
            // runways from being active
            return;
        }

        runways.push_back(rwy);
        _groupScore += score;
    }

    void adjustScoreForWind(double windHeading, double windSpeedKts)
    {
        _basicScore = _groupScore;
        RunwayVec::iterator it;
        for (it = runways.begin(); it != runways.end(); ++it) {
            _groupScore += runwayWindScore(*it, windHeading, windSpeedKts);
        }
    }

    double groupScore() const
    {
        return _groupScore;
    }

    void getRunways(FGRunwayList& arrivals, FGRunwayList& departures)
    {
    // make the common cases very obvious
        if (runways.size() == 1) {
            arrivals.push_back(runways.front());
            departures.push_back(runways.front());
            return;
        }


    // becuase runways were sorted by score when building, they were added
    // by score also, so we can use a simple algorithim to assign
        for (int r=0; r < runways.size(); ++r) {
            if ((r % 2) == 0) {
                arrivals.push_back(runways[r]);
            } else {
                departures.push_back(runways[r]);
            }
        }
    }

    std::string dump()
    {
        ostringstream os;
        os << runways.front()->ident();
        for (int r=1; r <runways.size(); ++r) {
            os << ", " << runways[r]->ident();
        }

        os << " (score=" << _basicScore << ", wind score=" << _groupScore << ")";
        return os.str();
    }

private:
    RunwayVec runways;
    double _groupScore,
        _leadRunwayScore;
    double _basicScore;

};

class WindExclusionCheck
{
public:
    WindExclusionCheck(double windHeading, double windSpeedKts) :
        _windSpeedKts(windSpeedKts),
        _windHeading(windHeading)
    {}

    bool operator()(const FGRunwayRef& rwy) const
    {
        return (runwayWindScore(rwy, _windHeading, _windSpeedKts) > 30);
    }
private:
    double _windSpeedKts,
        _windHeading;
};

class SortByScore
{
public:
    bool operator()(const FGRunwayRef& a, const FGRunwayRef& b) const
    {
        return runwayScore(a) > runwayScore(b);
    }
};

class GroupSortByScore
{
public:
    bool operator()(const FallbackRunwayGroup& a, const FallbackRunwayGroup& b) const
    {
        return a.groupScore() > b.groupScore();
    }
};

string FGAirportDynamics::fallbackGetActiveRunway(int action, double heading)
{
    bool updateNeeded = false;
    if (_lastFallbackUpdate == SGTimeStamp()) {
        updateNeeded = true;
    } else {
        updateNeeded = (_lastFallbackUpdate.elapsedMSec() > (1000 * 60 * 15));
    }

    if (updateNeeded) {
        double windSpeed   = fgGetInt("/environment/metar/base-wind-speed-kt");
        double windHeading = fgGetInt("/environment/metar/base-wind-dir-deg");

        // discount runways based on cross / tail-wind
        WindExclusionCheck windCheck(windHeading, windSpeed);
        RunwayVec runways(parent()->getRunways());
        RunwayVec::iterator it = std::remove_if(runways.begin(), runways.end(),
                                                windCheck);
        runways.erase(it, runways.end());

        // sort highest scored to lowest scored
        std::sort(runways.begin(), runways.end(), SortByScore());

        std::vector<FallbackRunwayGroup> groups;
        std::vector<FallbackRunwayGroup>::iterator git;

        for (it = runways.begin(); it != runways.end(); ++it) {
            bool existingGroupDidAccept = false;
            for (git = groups.begin(); git != groups.end(); ++git) {
                if (git->canAccept(*it)) {
                    existingGroupDidAccept = true;
                    git->addRunway(*it);
                    break;
                }
            } // of existing groups iteration

            if (!existingGroupDidAccept) {
                // create a new group
                groups.push_back(FallbackRunwayGroup(*it));
            }
        } // of group building phase


        // select highest scored group based on cross/tail wind
        for (git = groups.begin(); git != groups.end(); ++git) {
            git->adjustScoreForWind(windHeading, windSpeed);
        }

        std::sort(groups.begin(), groups.end(), GroupSortByScore());

        {
            ostringstream os;
            os << parent()->ident() << " groups:";

            for (git = groups.begin(); git != groups.end(); ++git) {
                os << "\n\t" << git->dump();
            }

            std::string s = os.str();
            SG_LOG(SG_AI, SG_INFO, s);
        }

        // assign takeoff and landing runways
        FallbackRunwayGroup bestGroup = groups.front();
        
        _lastFallbackUpdate.stamp();
        _fallbackRunwayCounter = 0;
        _fallbackDepartureRunways.clear();
        _fallbackArrivalRunways.clear();
        bestGroup.getRunways(_fallbackArrivalRunways, _fallbackDepartureRunways);

        ostringstream os;
        os << "\tArrival:" << _fallbackArrivalRunways.front()->ident();
        for (int r=1; r <_fallbackArrivalRunways.size(); ++r) {
            os << ", " << _fallbackArrivalRunways[r]->ident();
        }
        os << "\n\tDeparture:" << _fallbackDepartureRunways.front()->ident();
        for (int r=1; r <_fallbackDepartureRunways.size(); ++r) {
            os << ", " << _fallbackDepartureRunways[r]->ident();
        }

        std::string s = os.str();
        SG_LOG(SG_AI, SG_INFO, parent()->ident() << " fallback runways assignments for "
               << static_cast<int>(windHeading) << "@" << static_cast<int>(windSpeed) << "\n" << s);
    }

    _fallbackRunwayCounter++;
    FGRunwayRef r;
    // ensure we cycle through possible runways where they exist
    if (action == 1) {
        r = _fallbackDepartureRunways[_fallbackRunwayCounter % _fallbackDepartureRunways.size()];
    } else {
        r = _fallbackArrivalRunways[_fallbackRunwayCounter % _fallbackArrivalRunways.size()];
    }

    return r->ident();
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
        runway = fallbackGetActiveRunway(action, heading);
        return true;
    }

    RunwayGroup *currRunwayGroup = 0;
    int nrActiveRunways = 0;
    time_t dayStart = fgGetLong("/sim/time/utc/day-seconds");
    if ((std::abs((long) (dayStart - lastUpdate)) > 600)
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

   char atisSequenceString[2];
   atisSequenceString[0] = 'a' + atisSequenceIndex;
   atisSequenceString[1] = 0;
   
   return globals->get_locale()->getLocalizedString(atisSequenceString, "atc", "unknown");
}

int FGAirportDynamics::updateAtisSequence(int interval, bool forceUpdate)
{
    double now = globals->get_sim_time_sec();
    if (atisSequenceIndex == -1) {
        // first computation
        atisSequenceTimeStamp = now;
        atisSequenceIndex = rand() % 26; // random initial sequence letters
        return atisSequenceIndex;
    }

    int steps = static_cast<int>((now - atisSequenceTimeStamp) / interval);
    atisSequenceTimeStamp += (interval * steps);
    if (forceUpdate && (steps == 0)) {
        ++steps; // a "special" ATIS update is required
    } 
    
    atisSequenceIndex = (atisSequenceIndex + steps) % 26;
    // return a huge value if no update occurred
    return (atisSequenceIndex + (steps ? 0 : 26*1000));
}
