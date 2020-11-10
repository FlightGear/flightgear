// FlightPlan.cxx - flight plan object

// Written by James Turner, started 2012.
//
// Copyright (C) 2012  Curtis L. Olson
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

#include "config.h"

#include "FlightPlan.hxx"

// std
#include <map>
#include <fstream>
#include <cstring>
#include <cassert>
#include <algorithm>

// SimGear
#include <simgear/structure/exception.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/magvar/magvar.hxx>
#include <simgear/timing/sg_time.hxx>
#include <simgear/io/iostreams/sgstream.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/xml/easyxml.hxx>

// FlightGear
#include <Main/globals.hxx>
#include "Main/fg_props.hxx"
#include <Navaids/procedure.hxx>
#include <Navaids/waypoint.hxx>
#include <Navaids/routePath.hxx>
#include <Navaids/airways.hxx>
#include <Autopilot/route_mgr.hxx>
#include <Aircraft/AircraftPerformance.hxx>

using std::string;
using std::vector;
using std::endl;
using std::fstream;

namespace {

const string_list static_icaoFlightRulesCode = {
    "V",
    "I",
    "Y",
    "Z"
};

const string_list static_icaoFlightTypeCode = {
    "S",
    "N",
    "G",
    "M",
    "X"
};

} // of anonymous namespace

namespace flightgear {

// implemented in route.cxx
const char* restrictionToString(RouteRestriction aRestrict);

typedef std::vector<FlightPlan::DelegateFactoryRef> FPDelegateFactoryVec;
static FPDelegateFactoryVec static_delegateFactories;
  
FlightPlan::FlightPlan() :
  _currentIndex(-1),
    _followLegTrackToFix(true),
    _aircraftCategory(ICAO_AIRCRAFT_CATEGORY_C),
  _departureRunway(nullptr),
  _destinationRunway(nullptr),
  _sid(nullptr),
  _star(nullptr),
  _approach(nullptr),
  _totalDistance(0.0)
{
  _departureChanged = _arrivalChanged = _waypointsChanged = _currentWaypointChanged = false;
  _cruiseDataChanged = false;

  for (auto factory : static_delegateFactories) {
    Delegate* d = factory->createFlightPlanDelegate(this);
    if (d) { // factory might not always create a delegate
      d->_factory = factory; // record for clean-up purposes
      addDelegate(d);
    }
  }
}
  
FlightPlan::~FlightPlan()
{
// clean up delegates
    for (auto d : _delegates) {
        if (d->_factory) {
            auto f = d->_factory;
            f->destroyFlightPlanDelegate(this,  d);
        }
    }
}
  
FlightPlan* FlightPlan::clone(const string& newIdent) const
{
  FlightPlan* c = new FlightPlan();
  c->_ident = newIdent.empty() ? _ident : newIdent;
  c->lockDelegates();
  
// copy destination / departure data.
  c->setDeparture(_departure);
  c->setDeparture(_departureRunway);
  
  if (_approach) {
    c->setApproach(_approach, _approachTransition);
  } else if (_destinationRunway) {
    c->setDestination(_destinationRunway);
  } else if (_destination) {
    c->setDestination(_destination);
  }

  c->setSTAR(_star, _starTransition);
  c->setSID(_sid, _sidTransition);

  // mark data as unchanged since this is a clean plan
  c->_arrivalChanged = false;
  c->_departureChanged = false;

  // copy cruise data
  if (_cruiseFlightLevel > 0) {
      c->setCruiseFlightLevel(_cruiseFlightLevel);
  } else if (_cruiseAltitudeFt > 0) {
      c->setCruiseAltitudeFt(_cruiseAltitudeFt);
  }

  if (_cruiseAirspeedMach > 0) {
      c->setCruiseSpeedMach(_cruiseAirspeedMach);
  } else if (_cruiseAirspeedKnots > 0) {
      c->setCruiseSpeedKnots(_cruiseAirspeedKnots);
  }

  c->_didLoadFP = true; // set the loaded flag to give delegates a chance

  // copy legs
  c->_waypointsChanged = true;
  for (int l=0; l < numLegs(); ++l) {
    c->_legs.push_back(_legs[l]->cloneFor(c));
  }
  c->unlockDelegates();
  return c;
}

void FlightPlan::setIdent(const string& s)
{
  _ident = s;
}
  
string FlightPlan::ident() const
{
  return _ident;
}
  
FlightPlan::LegRef FlightPlan::insertWayptAtIndex(Waypt* aWpt, int aIndex)
{
  if (!aWpt) {
    return nullptr;
  }
  
  WayptVec wps;
  wps.push_back(aWpt);
  
  int index = aIndex;
  if ((aIndex == -1) || (aIndex > (int) _legs.size())) {
    index = _legs.size();
  }
  
  insertWayptsAtIndex(wps, index);
  return legAtIndex(index);
}
  
void FlightPlan::insertWayptsAtIndex(const WayptVec& wps, int aIndex)
{
  if (wps.empty()) {
    return;
  }
  
  int index = aIndex;
  if ((aIndex == -1) || (aIndex > (int) _legs.size())) {
    index = _legs.size();
  }
  
  auto it = _legs.begin() + index;
  int endIndex = index + wps.size() - 1;
  if (_currentIndex >= endIndex) {
    _currentIndex += wps.size();
  }
 
  LegVec newLegs;
  for (WayptRef wp : wps) {
      newLegs.push_back(LegRef{new Leg(this, wp)});
  }
  
  lockDelegates();
  _waypointsChanged = true;
  _legs.insert(it, newLegs.begin(), newLegs.end());
  unlockDelegates();
}

void FlightPlan::deleteIndex(int aIndex)
{
  int index = aIndex;
  if (aIndex < 0) { // negative indices count the the end
    index = _legs.size() + index;
  }
  
  if ((index < 0) || (index >= numLegs())) {
    SG_LOG(SG_NAVAID, SG_WARN, "removeAtIndex with invalid index:" << aIndex);
    return;
  }
  
  lockDelegates();
  _waypointsChanged = true;
  
  auto it = _legs.begin() + index;
  LegRef l = *it;
  _legs.erase(it);
  l->_parent = nullptr; // orphan the leg so it's clear from Nasal
    
  if (_currentIndex == index) {
    // current waypoint was removed
    _currentWaypointChanged = true;
  } else if (_currentIndex > index) {
    --_currentIndex; // shift current index down if necessary
  }
  
  unlockDelegates();
}

void FlightPlan::clearAll()
{
    lockDelegates();
    _departure.clear();
    _departureRunway = nullptr;
    _destinationRunway = nullptr;
    _destination.clear();
    _sid.clear();
    _sidTransition.clear();
    _star.clear();
    _starTransition.clear();
    _approach.clear();
    _approachTransition.clear();
    _alternate.clear();

    _cruiseAirspeedMach = 0.0;
    _cruiseAirspeedKnots = 0;
    _cruiseFlightLevel = 0;
    _cruiseAltitudeFt = 0;

    clearLegs();
    unlockDelegates();
}

void FlightPlan::clearLegs()
{
    // some badly behaved CDU implementations call clear on a Nasal timer
    // during startup.
    if (_legs.empty() && (_currentIndex < 0)) {
        return;
    }

  lockDelegates();
  _waypointsChanged = true;
  _currentWaypointChanged = true;
  _arrivalChanged = true;
  _departureChanged = true;
  _cruiseDataChanged = true;

  _currentIndex = -1;
  _legs.clear();  
  
    notifyCleared();
  unlockDelegates();
}
  
int FlightPlan::clearWayptsWithFlag(WayptFlag flag)
{
  int count = 0;
// first pass, fix up currentIndex
  for (int i=0; i<_currentIndex; ++i) {
    const auto& l = _legs.at(i);
    if (l->waypoint()->flag(flag)) {
      ++count;
    }
  }

  // test if the current leg will be removed
  bool currentIsBeingCleared = false;
  Leg* curLeg = currentLeg();
  if (curLeg) {
    currentIsBeingCleared = curLeg->waypoint()->flag(flag);
  }
  
  _currentIndex -= count;
    
    // if we're clearing the current waypoint, what shall we do with the
    // index? there's various options, but safest is to select no waypoint
    // and let the use re-activate.
    // http://code.google.com/p/flightgear-bugs/issues/detail?id=1134
    if (currentIsBeingCleared) {
        SG_LOG(SG_GENERAL, SG_INFO, "FlightPlan::clearWayptsWithFlag: currentIsBeingCleared:" << currentIsBeingCleared);
        _currentIndex = -1;
    }
  
// now delete and remove
    int numDeleted = 0;
    auto it = std::remove_if(_legs.begin(), _legs.end(),
        [flag, &numDeleted](const LegRef& leg)
    {
        if (leg->waypoint()->flag(flag)) {
            ++numDeleted;
            return true;
        }
        return false;
    });
  if (it == _legs.end()) {
    return 0; // nothing was cleared, don't fire the delegate
  }
  
  lockDelegates();
  _waypointsChanged = true;
  if ((count > 0) || currentIsBeingCleared) {
    _currentWaypointChanged = true;
  }
  
  _legs.erase(it, _legs.end());
    
  if (_legs.empty()) { // maybe all legs were deleted
      notifyCleared();
  }
  
  unlockDelegates();
  return numDeleted;
}
    
bool FlightPlan::isActive() const
{
    return (_currentIndex >= 0);
}

void FlightPlan::setCurrentIndex(int index)
{
  if ((index < -1) || (index >= numLegs())) {
    throw sg_range_exception("invalid leg index", "FlightPlan::setCurrentIndex");
  }
  
  if (index == _currentIndex) {
    return;
  }
  
  lockDelegates();
  _currentIndex = index;
  _currentWaypointChanged = true;
  unlockDelegates();
}

void FlightPlan::sequence()
{
    lockDelegates();
    for (auto d : _delegates) {
        d->sequence();
    }
    unlockDelegates();
}
    
void FlightPlan::finish()
{
  if (_currentIndex == -1) {
    return;
  }
  
  lockDelegates();
  _currentIndex = -1;
  _currentWaypointChanged = true;
  
  for (auto d : _delegates) {
    d->endOfFlightPlan();
  }
  
  unlockDelegates();
}
  
int FlightPlan::findWayptIndex(const SGGeod& aPos) const
{  
  for (int i=0; i<numLegs(); ++i) {
    if (_legs[i]->waypoint()->matches(aPos)) {
      return i;
    }
  }
  
  return -1;
}
  
int FlightPlan::findWayptIndex(const FGPositionedRef aPos) const
{
  for (int i=0; i<numLegs(); ++i) {
    if (_legs[i]->waypoint()->matches(aPos)) {
      return i;
    }
  }
  
  return -1;
}

FlightPlan::LegRef FlightPlan::currentLeg() const
{
  if ((_currentIndex < 0) || (_currentIndex >= numLegs()))
    return nullptr;
  return legAtIndex(_currentIndex);
}

FlightPlan::LegRef FlightPlan::previousLeg() const
{
  if (_currentIndex <= 0) {
    return nullptr;
  }
  
  return legAtIndex(_currentIndex - 1);
}

FlightPlan::LegRef FlightPlan::nextLeg() const
{
  if ((_currentIndex < 0) || ((_currentIndex + 1) >= numLegs())) {
    return nullptr;
  }
  
  return legAtIndex(_currentIndex + 1);
}

FlightPlan::LegRef FlightPlan::legAtIndex(int index) const
{
  if ((index < 0) || (index >= numLegs())) {
    throw sg_range_exception("index out of range", "FlightPlan::legAtIndex");
  }
  
    return _legs.at(index);
}
  
int FlightPlan::findLegIndex(const Leg* l) const
{
  for (unsigned int i=0; i<_legs.size(); ++i) {
    if (_legs.at(i).get() == l) {
      return i;
    }
  }
  
  return -1;
}

void FlightPlan::setDeparture(FGAirport* apt)
{
  if (apt == _departure) {
    return;
  }
  
  lockDelegates();
  _departureChanged = true;
  _departure = apt;
  _departureRunway = nullptr;
  clearSID();
  unlockDelegates();
}
  
void FlightPlan::setDeparture(FGRunway* rwy)
{
  if (_departureRunway == rwy) {
    return;
  }
  
  lockDelegates();
  _departureChanged = true;

  _departureRunway = rwy;
  if (rwy->airport() != _departure) {
    _departure = rwy->airport();
    clearSID();
  }
  unlockDelegates();
}
  
void FlightPlan::clearDeparture()
{
  lockDelegates();
  _departureChanged = true;
  _departure = nullptr;
  _departureRunway = nullptr;
  clearSID();
  unlockDelegates();
}
  
void FlightPlan::setSID(SID* sid, const std::string& transition)
{
  if ((sid == _sid) && (_sidTransition == transition)) {
    return;
  }
  
  lockDelegates();
  _departureChanged = true;
  _sid = sid;
  _sidTransition = transition;
  unlockDelegates();
}
  
void FlightPlan::setSID(Transition* trans)
{
  if (!trans) {
    setSID(static_cast<SID*>(nullptr));
    return;
  }
  
  if (trans->parent()->type() != PROCEDURE_SID)
    throw sg_exception("FlightPlan::setSID: transition does not belong to a SID");
  
  setSID(static_cast<SID*>(trans->parent()), trans->ident());
}
  
void FlightPlan::clearSID()
{
  lockDelegates();
  _departureChanged = true;
  _sid = nullptr;
  _sidTransition.clear();
  unlockDelegates();
}
  
Transition* FlightPlan::sidTransition() const
{
  if (!_sid || _sidTransition.empty()) {
    return nullptr;
  }
  
  return _sid->findTransitionByName(_sidTransition);
}

void FlightPlan::setDestination(FGAirport* apt)
{
  if (apt == _destination) {
    return;
  }
  
  lockDelegates();
  _arrivalChanged = true;
  _destination = apt;
  _destinationRunway = nullptr;
  clearSTAR();
  setApproach(static_cast<Approach*>(nullptr));
  unlockDelegates();
}
    
void FlightPlan::setDestination(FGRunway* rwy)
{
  if (_destinationRunway == rwy) {
    return;
  }
  
  lockDelegates();
  _arrivalChanged = true;
  _destinationRunway = rwy;
  if (rwy && (_destination != rwy->airport())) {
    _destination = rwy->airport();
    clearSTAR();
  }
  
  unlockDelegates();
}
  
void FlightPlan::clearDestination()
{
  lockDelegates();
  _arrivalChanged = true;
  _destination = nullptr;
  _destinationRunway = nullptr;
  clearSTAR();
  setApproach(static_cast<Approach*>(nullptr));
  unlockDelegates();
}

FGAirportRef FlightPlan::alternate() const
{
    return _alternate;
}

void FlightPlan::setAlternate(FGAirportRef alt)
{
    lockDelegates();
    _alternate = alt;
    _arrivalChanged = true;
    unlockDelegates();
}

void FlightPlan::setSTAR(STAR* star, const std::string& transition)
{
  if ((_star == star) && (_starTransition == transition)) {
    return;
  }
  
  lockDelegates();
  _arrivalChanged = true;
  _star = star;
  _starTransition = transition;
  unlockDelegates();
}
  
void FlightPlan::setSTAR(Transition* trans)
{
  if (!trans) {
    setSTAR((STAR*) NULL);
    return;
  }
  
  if (trans->parent()->type() != PROCEDURE_STAR)
    throw sg_exception("FlightPlan::setSTAR: transition does not belong to a STAR");
  
  setSTAR((STAR*) trans->parent(), trans->ident());
}

void FlightPlan::clearSTAR()
{

  lockDelegates();
  _arrivalChanged = true;
  _star = nullptr;
  _starTransition.clear();
  unlockDelegates();
}

void FlightPlan::setEstimatedDurationMinutes(int mins)
{
    _estimatedDuration = mins;
}

void FlightPlan::computeDurationMinutes()
{
    if ((_cruiseAirspeedMach < 0.01) && (_cruiseAirspeedKnots < 10)) {
        SG_LOG(SG_AUTOPILOT, SG_WARN, "can't compute duration, no cruise speed set");
        return;
    }

    if ((_cruiseAltitudeFt < 100) && (_cruiseFlightLevel < 10)) {
        SG_LOG(SG_AUTOPILOT, SG_WARN, "can't compute duration, no cruise altitude set");
        return;
    }


}
  
Transition* FlightPlan::starTransition() const
{
  if (!_star || _starTransition.empty()) {
    return nullptr;
  }
  
  return _star->findTransitionByName(_starTransition);
}

void FlightPlan::setApproach(flightgear::Approach* app, const std::string& trans)
{
    if ((_approach == app) && (trans == _approachTransition)) {
        return;
    }

  lockDelegates();
  _arrivalChanged = true;
  _approach = app;
  _approachTransition = trans;
  if (app) {
    // keep runway + airport in sync
    if (_destinationRunway != _approach->runway()) {
      _destinationRunway = _approach->runway();
    }
    
    if (_destination != _destinationRunway->airport()) {
      _destination = _destinationRunway->airport();
    }
  }
  unlockDelegates();
}

void FlightPlan::setApproach(Transition* approachWithTrans)
{
    if (!approachWithTrans) {
        setApproach((Approach*)nullptr);
        return;
    }

    if (!Approach::isApproach(approachWithTrans->parent()->type()))
        throw sg_exception("FlightPlan::setApproach: transition does not belong to an approach");

    setApproach(static_cast<Approach*>(approachWithTrans->parent()),
                approachWithTrans->ident());
}

Transition* FlightPlan::approachTransition() const
{
    if (!_approach || _approachTransition.empty()) {
        return nullptr;
    }

    return _approach->findTransitionByName(_approachTransition);
}

bool FlightPlan::save(std::ostream& stream) const
{
    try {
        SGPropertyNode_ptr d(new SGPropertyNode);
        saveToProperties(d);
        writeProperties(stream, d, true);
        return true;
    } catch (sg_exception& e) {
        SG_LOG(SG_NAVAID, SG_ALERT, "Failed to save flight-plan " << e.getMessage());
        return false;
    }
}
    
bool FlightPlan::save(const SGPath& path) const
{
  try {
    SGPropertyNode_ptr d(new SGPropertyNode);
    saveToProperties(d);
    writeProperties(path, d, true /* write-all */);
    return true;
  } catch (sg_exception& e) {
    SG_LOG(SG_NAVAID, SG_ALERT, "Failed to save flight-plan '" << path << "'. " << e.getMessage());
    return false;
  }
}

void FlightPlan::saveToProperties(SGPropertyNode* d) const
{
    d->setIntValue("version", 2);
    
    // general data
    d->setStringValue("flight-rules", static_icaoFlightRulesCode[static_cast<int>(_flightRules)]);
    d->setStringValue("flight-type", static_icaoFlightTypeCode[static_cast<int>(_flightType)]);
    if (!_callsign.empty()) {
        d->setStringValue("callsign", _callsign);
    }
    if (!_remarks.empty()) {
        d->setStringValue("remarks", _remarks);
    }
    if (!_aircraftType.empty()) {
        d->setStringValue("aircraft-type", _aircraftType);
    }
    d->setIntValue("estimated-duration-minutes", _estimatedDuration);

    if (_departure) {
        d->setStringValue("departure/airport", _departure->ident());
        if (_sid) {
            d->setStringValue("departure/sid", _sid->ident());
            if (!_sidTransition.empty())
                d->setStringValue("departure/sid_trans", _sidTransition);
        }
        
        if (_departureRunway) {
            d->setStringValue("departure/runway", _departureRunway->ident());
        }
    }
    
    if (_destination) {
        d->setStringValue("destination/airport", _destination->ident());
        if (_star) {
             d->setStringValue("destination/star", _star->ident());
             if (!_starTransition.empty())
                 d->setStringValue("destination/star_trans", _starTransition);
        }
        
        if (_approach) {
            d->setStringValue("destination/approach", _approach->ident());
            if (!_approachTransition.empty())
                d->setStringValue("destination/approach_trans", _approachTransition);
        }
        
        if (_destinationRunway) {
            d->setStringValue("destination/runway", _destinationRunway->ident());
        }
    }
    
    if (_alternate) {
        d->setStringValue("alternate", _alternate->ident());
    }
    
    // cruise data
    if (_cruiseFlightLevel > 0) {
        d->setIntValue("cruise/flight-level", _cruiseFlightLevel);
    } else if (_cruiseAltitudeFt > 0) {
        d->setIntValue("cruise/altitude-ft", _cruiseAltitudeFt);
    }
    
    if (_cruiseAirspeedMach > 0.0) {
        d->setDoubleValue("cruise/mach", _cruiseAirspeedMach);
    } else if (_cruiseAirspeedKnots > 0) {
        d->setIntValue("cruise/knots", _cruiseAirspeedKnots);
    }
    
    // route nodes
    SGPropertyNode* routeNode = d->getChild("route", 0, true);
    for (unsigned int i=0; i<_legs.size(); ++i) {
        auto  leg = _legs.at(i);
        Waypt* wpt = leg->waypoint();
        auto legNode = routeNode->getChild("wp", i, true);
        wpt->saveAsNode(legNode);
        leg->writeToProperties(legNode);
    } // of waypoint iteration
}

static bool anyWaypointsWithFlag(FlightPlan* plan, WayptFlag flag)
{
    bool r = false;
    plan->forEachLeg([&r, flag](FlightPlan::Leg* l) {
        if (l->waypoint()->flags() & flag) {
            r = true;
        }
    });
    
    return r;
}

bool FlightPlan::load(const SGPath& path)
{
    if (!path.exists()) {
        SG_LOG(SG_NAVAID, SG_ALERT, "Failed to load flight-plan '" << path
               << "'. The file does not exist.");
        return false;
    }
    
    SG_LOG(SG_NAVAID, SG_INFO, "going to read flight-plan from:" << path);
    
    bool Status = false;
    lockDelegates();
    
    // try different file formats
    if (loadGpxFormat(path)) { // GPX format
        _arrivalChanged = true;
        _departureChanged = true;
        Status = true;
    } else if (loadXmlFormat(path)) { // XML property data
        // we don't want to re-compute the arrival / departure after
        // a load, since we assume the flight-plan had it specified already
        // especially, the XML might have a SID/STAR embedded, which we don't
        // want to lose
        
        // however, we do want to run the normal delegate if no procedure was
        // defined. We'll use the presence of waypoints tagged to decide
        const bool hasArrival = anyWaypointsWithFlag(this, WPT_ARRIVAL);
        const bool hasDeparture = anyWaypointsWithFlag(this, WPT_DEPARTURE);
        _arrivalChanged = !hasArrival;
        _departureChanged = !hasDeparture;
        Status = true;
    } else if (loadPlainTextFormat(path)) { // simple textual list of waypoints
        _arrivalChanged = true;
        _departureChanged = true;
        Status = true;
    }
    
    if (Status == true) {
        setIdent(path.file_base());
    }
    
    _cruiseDataChanged = true;
    _waypointsChanged = true;
    _didLoadFP = true;

    unlockDelegates();
    
    return Status;
}
    
bool FlightPlan::load(std::istream &stream)
{
    SGPropertyNode_ptr routeData(new SGPropertyNode);
    try {
        readProperties(stream, routeData);
    } catch (sg_exception& e) {
        SG_LOG(SG_NAVAID, SG_ALERT, "Failed to load flight-plan '" << e.getOrigin()
               << "'. " << e.getMessage());
        return false;
    }
    
    if (!routeData.valid())
        return false;
    
    bool Status = false;
    lockDelegates();
    try {
        int version = routeData->getIntValue("version", 1);
        if (version == 2) {
            loadVersion2XMLRoute(routeData);
            Status = true;
        } else {
            throw sg_io_exception("unsupported XML route version");
        }
    } catch (sg_exception& e) {
        SG_LOG(SG_NAVAID, SG_ALERT, "Failed to load flight-plan '" << e.getOrigin()
               << "'. " << e.getMessage());
        Status = false;
    }
    
    // we don't want to re-compute the arrival / departure after
     // a load, since we assume the flight-plan had it specified already
     // especially, the XML might have a SID/STAR embedded, which we don't
     // want to lose
    
     // however, we do want to run the normal delegate if no procedure was
     // defined. We'll use the presence of waypoints tagged to decide
     const bool hasArrival = anyWaypointsWithFlag(this, WPT_ARRIVAL);
     const bool hasDeparture = anyWaypointsWithFlag(this, WPT_DEPARTURE);
     _arrivalChanged = !hasArrival;
     _departureChanged = !hasDeparture;
    
    _cruiseDataChanged = true;
    _waypointsChanged = true;
    _didLoadFP = true;

    unlockDelegates();
    
    return Status;
}

/** XML loader for GPX file format */
class GpxXmlVisitor : public XMLVisitor
{
public:
    GpxXmlVisitor(FlightPlan* fp) : _fp(fp), _lat(-9999), _lon(-9999) {}

    void startElement (const char * name, const XMLAttributes &atts) override;
    void endElement (const char * name) override;
    void data (const char * s, int length) override;

    const WayptVec& waypoints() const { return _waypoints; }
private:
    FlightPlan* _fp;
    double      _lat, _lon, _elevationM;
    string      _element;
    string      _waypoint;
    WayptVec    _waypoints;
};

void GpxXmlVisitor::startElement(const char * name, const XMLAttributes &atts)
{
    _element = name;
    if (!strcmp(name, "rtept")) {
        _waypoint.clear();
        _lat = _lon = _elevationM = -9999;
        const char* slat = atts.getValue("lat");
        const char* slon = atts.getValue("lon");
        if (slat && slon) {
            _lat = atof(slat);
            _lon = atof(slon);
        }
    }
}

void GpxXmlVisitor::data(const char * s, int length)
{
    // use "name" when given, otherwise use "cmt" (comment) as ID
    if ((_element == "name") ||
        ((_waypoint.empty()) && (_element == "cmt")))
    {
        _waypoint = std::string(s, static_cast<size_t>(length));
    }

    if (_element == "ele") {
        _elevationM = atof(s);
    }
}

void GpxXmlVisitor::endElement(const char * name)
{
    _element.clear();
    if (!strcmp(name, "rtept")) {
        if (_lon > -9990.0) {
            const auto geod = SGGeod::fromDeg(_lon, _lat);
            auto pos = FGPositioned::findClosestWithIdent(_waypoint, geod);
            WayptRef wp;

            if (pos) {
                // check distance
                const auto dist = SGGeodesy::distanceM(geod, pos->geod());
                if (dist < 800.0) {
                    wp = new NavaidWaypoint(pos, _fp);
                }
            }

            if (!wp) {
                wp = new BasicWaypt(geod, _waypoint, _fp);
            }

            if (_elevationM > -9990.0) {
                wp->setAltitude(_elevationM * SG_METER_TO_FEET, RESTRICT_AT);
            }
            _waypoints.push_back(wp);
        }
    }
}

/** Load a flightplan in GPX format */
bool FlightPlan::loadGpxFormat(const SGPath& path)
{
    if (path.lower_extension() != "gpx") {
        // not a valid GPX file
        return false;
    }

    GpxXmlVisitor gpxVisitor(this);
    try {
        readXML(path, gpxVisitor);
    } catch (sg_exception& e) {
        // XML parsing fails => not a GPX XML file
        SG_LOG(SG_NAVAID, SG_ALERT, "Failed to load flight-plan in GPX format: '" << e.getOrigin()
                     << "'. " << e.getMessage());
        return false;
    }

    if (gpxVisitor.waypoints().empty()) {
        SG_LOG(SG_NAVAID, SG_ALERT, "Failed to load flight-plan in GPX format. No route found.");
        return false;
    }

    clearAll();

    // copy in case we need to modify
    WayptVec wps = gpxVisitor.waypoints();
    // detect airports
    const auto depApt = FGAirport::findByIdent(wps.front()->ident());
    const auto destApt = FGAirport::findByIdent(wps.back()->ident());

    if (depApt) {
        wps.erase(wps.begin());
        setDeparture(depApt);
    }

    // for a single-element waypoint list consisting of a single airport ID,
    // don't crash
    if (destApt && !wps.empty()) {
        wps.pop_back();
        setDestination(destApt);
    }

    insertWayptsAtIndex(wps, -1);

    return true;
}

/** Load a flightplan in FlightGear XML property format */
bool FlightPlan::loadXmlFormat(const SGPath& path)
{
  SGPropertyNode_ptr routeData(new SGPropertyNode);
  try {
    readProperties(path, routeData);
  } catch (sg_exception& e) {
     SG_LOG(SG_NAVAID, SG_ALERT, "Failed to load flight-plan '" << e.getOrigin()
             << "'. " << e.getMessage());
    // XML parsing fails => not a property XML file
    return false;
  }

  if (routeData.valid())
  {
    try {
      int version = routeData->getIntValue("version", 1);
      if (version == 1) {
        loadVersion1XMLRoute(routeData);
      } else if (version == 2) {
        loadVersion2XMLRoute(routeData);
      } else {
        throw sg_io_exception("unsupported XML route version");
      }
      return true;
    } catch (sg_exception& e) {
      SG_LOG(SG_NAVAID, SG_ALERT, "Failed to load flight-plan '" << e.getOrigin()
             << "'. " << e.getMessage());
    }
  }

  return false;
}

void FlightPlan::loadXMLRouteHeader(SGPropertyNode_ptr routeData)
{
    // general info

    const auto rules = routeData->getStringValue("flight-rules", "V");
    auto it = std::find(static_icaoFlightRulesCode.begin(),
                        static_icaoFlightRulesCode.end(), rules);
    _flightRules = static_cast<ICAOFlightRules>(std::distance(static_icaoFlightRulesCode.begin(), it));

    const auto type = routeData->getStringValue("flight-type", "X");
    auto it2 = std::find(static_icaoFlightTypeCode.begin(),
                        static_icaoFlightTypeCode.end(), type);
    _flightType = static_cast<ICAOFlightType>(std::distance(static_icaoFlightTypeCode.begin(), it2));

    _callsign = routeData->getStringValue("callsign");
    _remarks = routeData->getStringValue("remarks");
    _aircraftType = routeData->getStringValue("aircraft-type");
    _estimatedDuration = routeData->getIntValue("estimated-duration-minutes");

  // departure nodes
  SGPropertyNode* dep = routeData->getChild("departure");
  if (dep) {
    string depIdent = dep->getStringValue("airport");
    setDeparture((FGAirport*) fgFindAirportID(depIdent));
    if (_departure) {
      string rwy(dep->getStringValue("runway"));
      if (_departure->hasRunwayWithIdent(rwy)) {
        setDeparture(_departure->getRunwayByIdent(rwy));
      }
    
      if (dep->hasChild("sid")) {
          // previously, we would write a transition id for 'SID' if set,
          // but this is ambigous. Starting with 2020.2, we only every try
          // to parse this value as a SID, and look for a seperate sid_trans
          // value
          const string trans = dep->getStringValue("sid_trans");
          const auto sid = dep->getStringValue("sid");
          setSID(_departure->findSIDWithIdent(sid), trans);
      }
    }
  }
  
  // destination
  SGPropertyNode* dst = routeData->getChild("destination");
  if (dst) {
    setDestination((FGAirport*) fgFindAirportID(dst->getStringValue("airport")));
    if (_destination) {
      string rwy(dst->getStringValue("runway"));
      if (_destination->hasRunwayWithIdent(rwy)) {
        setDestination(_destination->getRunwayByIdent(rwy));
      }
      
      if (dst->hasChild("star")) {
          // prior to 2020.2 we would attempt to treat 'star' as a
          // transiiton ID, but this is ambiguous. Look for a seperate value now
          const auto star = dst->getStringValue("star");
          const string trans = dst->getStringValue("star_trans");
          setSTAR(_destination->findSTARWithIdent(star), trans);
      } // of STAR processing
      
      if (dst->hasChild("approach")) {
          auto app = _destination->findApproachWithIdent(dst->getStringValue("approach"));
          const auto trans = dst->getStringValue("approach_trans");
          setApproach(app, trans);
      }
    }
  }
  
  // alternate
  if (routeData->hasChild("alternate")) {
      setAlternate((FGAirport*) fgFindAirportID(routeData->getStringValue("alternate")));
  }
  
  // cruise
  SGPropertyNode* crs = routeData->getChild("cruise");
  if (crs) {
      if (crs->hasChild("flight-level")) {
          _cruiseFlightLevel = crs->getIntValue("flight-level");
      } else if (crs->hasChild("altitude-ft")) {
          _cruiseAltitudeFt = crs->getIntValue("altitude-ft");
      }

      if (crs->hasChild("mach")) {
          _cruiseAirspeedMach = crs->getDoubleValue("mach");
      } else if (crs->hasChild("knots")) {
          _cruiseAirspeedKnots = crs->getIntValue("knots");
      }
  } // of cruise data loading
}

void FlightPlan::loadVersion2XMLRoute(SGPropertyNode_ptr routeData)
{
  loadXMLRouteHeader(routeData);
  
  // route nodes
  _legs.clear();
  SGPropertyNode_ptr routeNode = routeData->getChild("route", 0);
  if (routeNode.valid()) {
    for (auto wpNode : routeNode->getChildren("wp")) {
        auto wp = Waypt::createFromProperties(this, wpNode);
        if (!wp) {
            continue;
        }

      LegRef l = new Leg{this, wp};
      // sync leg restrictions with waypoint ones
        if (wp->speedRestriction() != RESTRICT_NONE) {
            l->setSpeed(wp->speedRestriction(), wp->speed());
        }
        
        if (wp->altitudeRestriction() != RESTRICT_NONE) {
            l->setAltitude(wp->altitudeRestriction(), wp->altitudeFt());
        }
        
        if (wpNode->hasChild("hold-count")) {
            l->setHoldCount(wpNode->getIntValue("hold-count"));
        }
      _legs.push_back(l);
    } // of route iteration
  }
  _waypointsChanged = true;
}

void FlightPlan::loadVersion1XMLRoute(SGPropertyNode_ptr routeData)
{
  loadXMLRouteHeader(routeData);
  
  // _legs nodes
  _legs.clear();
  SGPropertyNode_ptr routeNode = routeData->getChild("route", 0);    
  for (int i=0; i<routeNode->nChildren(); ++i) {
    SGPropertyNode_ptr wpNode = routeNode->getChild("wp", i);
    LegRef l = new Leg(this, parseVersion1XMLWaypt(wpNode));
    _legs.push_back(l);
  } // of route iteration
  _waypointsChanged = true;
}

WayptRef FlightPlan::parseVersion1XMLWaypt(SGPropertyNode* aWP)
{
  SGGeod lastPos;
  if (!_legs.empty()) {
    lastPos = _legs.back()->waypoint()->position();
  } else if (_departure) {
    lastPos = _departure->geod();
  }
  
  WayptRef w;
  string ident(aWP->getStringValue("ident"));
  if (aWP->hasChild("longitude-deg")) {
    // explicit longitude/latitude
    w = new BasicWaypt(SGGeod::fromDeg(aWP->getDoubleValue("longitude-deg"), 
                                       aWP->getDoubleValue("latitude-deg")), ident, this);
    
  } else {
    string nid = aWP->getStringValue("navid", ident.c_str());
    FGPositionedRef p = FGPositioned::findClosestWithIdent(nid, lastPos);
    SGGeod pos;
    
    if (p) {
      pos = p->geod();
    } else {
      SG_LOG(SG_GENERAL, SG_WARN, "unknown navaid in flightplan:" << nid);
      pos = SGGeod::fromDeg(aWP->getDoubleValue("longitude-deg"),
                            aWP->getDoubleValue("latitude-deg"));
    }
    
    if (aWP->hasChild("offset-nm") && aWP->hasChild("offset-radial")) {
      double radialDeg = aWP->getDoubleValue("offset-radial");
      // convert magnetic radial to a true radial!
      radialDeg += magvarDegAt(pos);
      double offsetNm = aWP->getDoubleValue("offset-nm");
      double az2;
      SGGeodesy::direct(pos, radialDeg, offsetNm * SG_NM_TO_METER, pos, az2);
    }
    
    w = new BasicWaypt(pos, ident, this);
  }
  
  double altFt = aWP->getDoubleValue("altitude-ft", -9999.9);
  if (altFt > -9990.0) {
    w->setAltitude(altFt, RESTRICT_AT);
  }
  
  return w;
}

/** Load a flightplan in FlightGear plain-text format */
bool FlightPlan::loadPlainTextFormat(const SGPath& path)
{
  try {
    sg_gzifstream in(path);
    if (!in.is_open()) {
      throw sg_io_exception("Cannot open file for reading.");
    }
    
    _legs.clear();
    while (!in.eof()) {
      string line;
      getline(in, line, '\n');
      // trim CR from end of line, if found
      if (line[line.size() - 1] == '\r') {
        line.erase(line.size() - 1, 1);
      }
      
      line = simgear::strutils::strip(line);
      if (line.empty() || (line[0] == '#')) {
        continue; // ignore empty/comment lines
      }
      
      WayptRef w = waypointFromString(line);
      if (!w) {
        throw sg_io_exception("Failed to create waypoint from line '" + line + "'.");
      }
      
      _legs.push_back(LegRef{new Leg(this, w)});
    } // of line iteration
  } catch (sg_exception& e) {
    SG_LOG(SG_NAVAID, SG_ALERT, "Failed to load route from: '" << path << "'. " << e.getMessage());
    _legs.clear();
    return false;
  }
  
  return true;
}  

double FlightPlan::magvarDegAt(const SGGeod& pos) const
{
  double jd = globals->get_time_params()->getJD();
  return sgGetMagVar(pos, jd) * SG_RADIANS_TO_DEGREES;
}
  

namespace
{

WayptRef intersectionFromString(FGPositionedRef p1,
                                const SGGeod& basePosition,
                                const double magvar,
                                const string_list& pieces)
{
    assert(pieces.size() == 4);
    // navid/radial/navid/radial notation
    FGPositionedRef p2 = FGPositioned::findClosestWithIdent(pieces[2], basePosition);
    if (!p2) {
      SG_LOG( SG_NAVAID, SG_INFO, "Unable to find FGPositioned with ident:" << pieces[2]);
      return nullptr;
    }

    double r1 = atof(pieces[1].c_str()),
    r2 = atof(pieces[3].c_str());
    r1 += magvar;
    r2 += magvar;

    SGGeod intersection;
    bool ok = SGGeodesy::radialIntersection(p1->geod(), r1, p2->geod(), r2, intersection);
    if (!ok) {
      SG_LOG(SG_NAVAID, SG_INFO, "no valid intersection for:" << pieces[0]
              << "/" << pieces[2]);
      return NULL;
    }

    std::string name = p1->ident() + "-" + p2->ident();
    return new BasicWaypt(intersection, name, nullptr);
}

WayptRef wayptFromLonLatString(const std::string& target)
{
    // permit lat,lon/radial/offset format
    string_list pieces(simgear::strutils::split(target, "/"));
    if ((pieces.size() != 1) && (pieces.size() != 3)) {
        return {};
    }
    
    SGGeod g;
    const bool defaultToLonLat = true; // parseStringAsGeod would otherwise default to lat,lon
    if (!simgear::strutils::parseStringAsGeod(pieces[0], &g, defaultToLonLat)) {
        return {};
    }
    
    if (pieces.size() == 3) {
        // process offset
        const double bearing = std::stod(pieces[1]);
        const double distanceNm = std::stod(pieces[2]);
        g = SGGeodesy::direct(g, bearing, distanceNm * SG_NM_TO_METER);
    }
    
    // build a short name
    const int lonDeg = static_cast<int>(g.getLongitudeDeg());
    const int latDeg = static_cast<int>(g.getLatitudeDeg());

    char buf[32];
    char ew = (lonDeg < 0) ? 'W' : 'E';
    char ns = (latDeg < 0) ? 'S' : 'N';
    snprintf(buf, 32, "%c%03d%c%03d", ew, std::abs(lonDeg), ns, std::abs(latDeg));

    return new BasicWaypt(g, buf, nullptr);
}

WayptRef viaFromString(const SGGeod& basePosition, const std::string& target)
{
    assert(target.find("VIA ") == 0);
    string_list pieces(simgear::strutils::split(target, "/"));
    if ((pieces.size() != 4) || (pieces[2] != "TO")) {
        SG_LOG( SG_NAVAID, SG_WARN, "Malformed VIA specification string:" << target);
        return nullptr;
    }
    
    // TO navaid is pieces[3]
    FGPositionedRef nav = FGPositioned::findClosestWithIdent(pieces[3], basePosition, nullptr);
    if (!nav) {
        SG_LOG( SG_NAVAID, SG_WARN, "TO navaid:" << pieces[3] << " unknown");
        return nullptr;
    }
    
    // airway ident is pieces[1]
    AirwayRef airway = Airway::findByIdentAndNavaid(pieces[1], nav);
    if (!airway) {
        SG_LOG( SG_NAVAID, SG_WARN, "Unknown airway:" << pieces[1]);
        return nullptr;
    }

    return new Via(nullptr, airway, nav);
}

} // of anonymous namespace

WayptRef FlightPlan::waypointFromString(const string& tgt )
{
  string target = simgear::strutils::uppercase(tgt);
  // extract altitude
  double altFt = 0.0;
  RouteRestriction altSetting = RESTRICT_NONE;
  
  size_t pos = target.find( '@' );
  if ( pos != string::npos ) {
    altFt = atof( target.c_str() + pos + 1 );
    target = target.substr( 0, pos );
    if ( !strcmp(fgGetString("/sim/startup/units"), "meter") )
      altFt *= SG_METER_TO_FEET;
    altSetting = RESTRICT_AT;
  }
  
  // check for lon,lat
    WayptRef wpt = wayptFromLonLatString(target);

  SGGeod basePosition;
  if (_legs.empty()) {
    // route is empty, use departure position / aircraft position
    basePosition = _departure ? _departure->geod() : globals->get_aircraft_position();
  } else {
    basePosition = _legs.back()->waypoint()->position();
  }
  
  const double magvar = magvarDegAt(basePosition);

  if (wpt) {
      // already handled in the lat/lon test above
  } else if (target.find("VIA ") == 0) {
      wpt = viaFromString(basePosition, target);
  } else {
      string_list pieces(simgear::strutils::split(target, "/"));
      FGPositionedRef p = FGPositioned::findClosestWithIdent(pieces.front(), basePosition);
      if (!p) {
        SG_LOG( SG_NAVAID, SG_INFO, "Unable to find FGPositioned with ident:" << pieces.front());
        return nullptr;
      }

      if (pieces.size() == 1) {
        wpt = new NavaidWaypoint(p, this);
      } else if (pieces.size() == 3) {
        // navaid/radial/distance-nm notation
        double radial = atof(pieces[1].c_str()),
        distanceNm = atof(pieces[2].c_str());
        radial += magvar;
        wpt = new OffsetNavaidWaypoint(p, this, radial, distanceNm);
      } else if (pieces.size() == 2) {
        FGAirport* apt = dynamic_cast<FGAirport*>(p.ptr());
        if (!apt) {
          SG_LOG(SG_NAVAID, SG_INFO, "Waypoint is not an airport:" << pieces.front());
          return nullptr;
        }

        if (!apt->hasRunwayWithIdent(pieces[1])) {
          SG_LOG(SG_NAVAID, SG_INFO, "No runway: " << pieces[1] << " at " << pieces[0]);
          return nullptr;
        }

        FGRunway* runway = apt->getRunwayByIdent(pieces[1]);
        wpt = new NavaidWaypoint(runway, this);
      } else if (pieces.size() == 4) {
        wpt = intersectionFromString(p, basePosition, magvar, pieces);
      }
  }
  
  if (!wpt) {
    SG_LOG(SG_NAVAID, SG_INFO, "Unable to parse waypoint:" << target);
    return nullptr;
  }
  
  if (altSetting != RESTRICT_NONE) {
    wpt->setAltitude(altFt, altSetting);
  }
  return wpt;
}


void FlightPlan::activate()
{
  FGRouteMgr* routeManager = globals->get_subsystem<FGRouteMgr>();
  if (routeManager) {
    if (routeManager->flightPlan() != this) {
      SG_LOG(SG_NAVAID, SG_DEBUG, "setting new flight-plan on route-manager");
      routeManager->setFlightPlan(this);
    }
  }
  
  lockDelegates();
  
  _currentIndex = 0;
  _currentWaypointChanged = true;
  
  for (unsigned int i=1; i < _legs.size(); ) {
    if (_legs[i]->waypoint()->type() == "via") {
      WayptRef preceeding = _legs[i - 1]->waypoint();
      Via* via = static_cast<Via*>(_legs[i]->waypoint());
      WayptVec wps = via->expandToWaypoints(preceeding);
      
      // delete the VIA leg
      auto it = _legs.begin() + i;
      LegRef l = *it;
      _legs.erase(it);
      
      // create new legs and insert
        it = _legs.begin() + i;
      
      LegVec newLegs;
      for (WayptRef wp : wps) {
          newLegs.push_back(LegRef{new Leg(this, wp)});
      }
      
      _waypointsChanged = true;
      _legs.insert(it, newLegs.begin(), newLegs.end());
    } else {
      ++i; // normal case, no expansion
    }
  }
  
  for (auto d : _delegates) {
    d->activated();
  }
  
  unlockDelegates();
}

FlightPlan::Leg::Leg(FlightPlan* owner, WayptRef wpt) :
  _parent(owner),
  _waypt(wpt)
{
  if (!wpt.valid()) {
    throw sg_exception("can't create FlightPlan::Leg without underlying waypoint");
  }
}

FlightPlan::Leg* FlightPlan::Leg::cloneFor(FlightPlan* owner) const
{
  Leg* c = new Leg(owner, _waypt);
// clone local data
  c->_speed = _speed;
  c->_speedRestrict = _speedRestrict;
  c->_altitudeFt = _altitudeFt;
  c->_altRestrict = _altRestrict;
  
  return c;
}
  
FlightPlan::Leg* FlightPlan::Leg::nextLeg() const
{
  if ((index() + 1) >= _parent->_legs.size())
    return nullptr;
    
  return _parent->legAtIndex(index() + 1);
}

unsigned int FlightPlan::Leg::index() const
{
  return _parent->findLegIndex(this);
}

int FlightPlan::Leg::altitudeFt() const
{
  if (_altRestrict != RESTRICT_NONE) {
    return _altitudeFt;
  }
  
  return _waypt->altitudeFt();
}

int FlightPlan::Leg::speed() const
{
  if (_speedRestrict != RESTRICT_NONE) {
    return _speed;
  }
  
  return _waypt->speed();
}

int FlightPlan::Leg::speedKts() const
{
  return speed();
}
  
double FlightPlan::Leg::speedMach() const
{
  if (!isMachRestrict(_speedRestrict)) {
    return 0.0;
  }
  
  return -(_speed / 100.0);
}

RouteRestriction FlightPlan::Leg::altitudeRestriction() const
{
  if (_altRestrict != RESTRICT_NONE) {
    return _altRestrict;
  }
  
  return _waypt->altitudeRestriction();
}
  
RouteRestriction FlightPlan::Leg::speedRestriction() const
{
  if (_speedRestrict != RESTRICT_NONE) {
    return _speedRestrict;
  }
  
  return _waypt->speedRestriction();
}
  
void FlightPlan::Leg::setSpeed(RouteRestriction ty, double speed)
{
  _speedRestrict = ty;
  if (isMachRestrict(ty)) {
    _speed = (speed * -100); 
  } else {
    _speed = speed;
  }
}
  
void FlightPlan::Leg::setAltitude(RouteRestriction ty, int altFt)
{
  _altRestrict = ty;
  _altitudeFt = altFt;
}

double FlightPlan::Leg::courseDeg() const
{
  return _courseDeg;
}
  
double FlightPlan::Leg::distanceNm() const
{
  return _pathDistance;
}
  
double FlightPlan::Leg::distanceAlongRoute() const
{
  return _distanceAlongPath;
}
  
    
bool FlightPlan::Leg::convertWaypointToHold()
{
  const auto wty = _waypt->type();
  if (wty == "hold") {
    return true;
  }
  
  if ((wty != "basic") && (wty != "navaid")) {
    SG_LOG(SG_INSTR, SG_WARN, "convertWaypointToHold: cannot convert waypt " << index() << " " << _waypt->ident() << " to a hold");
    return false;
  }
  
  auto hold = new Hold(_waypt->position(), _waypt->ident(), const_cast<FlightPlan*>(_parent));
  
  // default to a 1 minute hold with the radial being our arrival radial
  hold->setHoldTime(60.0);
  hold->setHoldRadial(_courseDeg);
  _waypt = hold;  // we drop our reference to the old waypoint
  
  markWaypointDirty();
  
  return true;
}
    
bool FlightPlan::Leg::setHoldCount(int count)
{
  if (count == 0) {
    _holdCount = count;
    return true;
  }
    
  if (!convertWaypointToHold()) {
    return false;
  }
  
  _holdCount = count;
  markWaypointDirty();
  return true;
}
  
  void FlightPlan::Leg::markWaypointDirty()
  {
    auto fp = owner();
    fp->lockDelegates();
    fp->_waypointsChanged = true;
    fp->unlockDelegates();
  }
  
int FlightPlan::Leg::holdCount() const
{
  return _holdCount;
}

void FlightPlan::Leg::writeToProperties(SGPropertyNode* aProp) const
{
    if (_speedRestrict != RESTRICT_NONE) {
        aProp->setStringValue("speed-restrict", restrictionToString(_speedRestrict));
        if (_speedRestrict == SPEED_RESTRICT_MACH) {
            aProp->setDoubleValue("speed", speedMach());
        } else {
            aProp->setDoubleValue("speed", _speed);
        }
    }
  
    if (_altRestrict != RESTRICT_NONE) {
        aProp->setStringValue("alt-restrict", restrictionToString(_altRestrict));
        aProp->setDoubleValue("altitude-ft", _altitudeFt);
    }
    
    if (_holdCount > 0) {
        aProp->setDoubleValue("hold-count", _holdCount);
    }
}


void FlightPlan::rebuildLegData()
{
  _totalDistance = 0.0;
  double totalDistanceIncludingMissed = 0.0;
  RoutePath path(this);
  
  for (unsigned int l=0; l<_legs.size(); ++l) {
    _legs[l]->_courseDeg = path.trackForIndex(l);
    _legs[l]->_pathDistance = path.distanceForIndex(l) * SG_METER_TO_NM;

    totalDistanceIncludingMissed += _legs[l]->_pathDistance;
    // distance along path includes our own leg distance
    _legs[l]->_distanceAlongPath = totalDistanceIncludingMissed;
    
    // omit missed-approach waypoints from total distance calculation
    if (!_legs[l]->waypoint()->flag(WPT_MISS)) {
      _totalDistance += _legs[l]->_pathDistance;
    }
} // of legs iteration
  
}
  
SGGeod FlightPlan::pointAlongRoute(int aIndex, double aOffsetNm) const
{
    RoutePath rp(this);
    return rp.positionForDistanceFrom(aIndex, aOffsetNm * SG_NM_TO_METER);
}
    
void FlightPlan::lockDelegates()
{
  if (_delegateLock == 0) {
    assert(!_departureChanged && !_arrivalChanged && 
           !_waypointsChanged && !_currentWaypointChanged);
  }
  
  ++_delegateLock;
  if (_delegateLock > 10) {
    SG_LOG(SG_GENERAL, SG_ALERT, "hmmm");
  }
}

void FlightPlan::unlockDelegates()
{
  assert(_delegateLock > 0);
  if (_delegateLock > 1) {
    --_delegateLock;
    return;
  }
  
    if (_didLoadFP) {
        _didLoadFP = false;
        for (auto d : _delegates) {
          d->loaded();
        }
    }
    
  if (_departureChanged) {
    _departureChanged = false;
    for (auto d : _delegates) {
      d->departureChanged();
    }
  }
  
  if (_arrivalChanged) {
    _arrivalChanged = false;
    for (auto d : _delegates) {
      d->arrivalChanged();
    }
  }
  
  if (_cruiseDataChanged) {
      _cruiseDataChanged = false;
      for (auto d : _delegates) {
        d->cruiseChanged();
      }
  }

  if (_waypointsChanged) {
    _waypointsChanged = false;
    rebuildLegData();
    for (auto d : _delegates) {
      d->waypointsChanged();
    }
  }
  
  if (_currentWaypointChanged) {
    _currentWaypointChanged = false;
    for (auto d : _delegates) {
      d->currentWaypointChanged();
    }
  }
  
  --_delegateLock;
}
  
void FlightPlan::registerDelegateFactory(DelegateFactoryRef df)
{
  auto it = std::find(static_delegateFactories.begin(), static_delegateFactories.end(), df);
  if (it != static_delegateFactories.end()) {
    throw sg_exception("duplicate delegate factory registration");
  }
  
  static_delegateFactories.push_back(df);
}
  
void FlightPlan::unregisterDelegateFactory(DelegateFactoryRef df)
{
  auto it = std::find(static_delegateFactories.begin(), static_delegateFactories.end(), df);
  if (it == static_delegateFactories.end()) {
    return;
  }
  
  static_delegateFactories.erase(it);
}
  
void FlightPlan::addDelegate(Delegate* d)
{
  assert(d);
#ifndef NDEBUG
  auto it = std::find(_delegates.begin(), _delegates.end(), d);
#endif
  assert(it == _delegates.end());
  _delegates.push_back(d);
}

void FlightPlan::removeDelegate(Delegate* d)
{
  assert(d);
  auto it = std::find(_delegates.begin(), _delegates.end(), d);
  assert(it != _delegates.end());
  _delegates.erase(it);
}
  
void FlightPlan::notifyCleared()
{
    for (auto d : _delegates) {
        d->cleared();
    }
}

FlightPlan::Delegate::Delegate()
{
}

FlightPlan::Delegate::~Delegate()
{  
}

void FlightPlan::setFollowLegTrackToFixes(bool tf)
{
    _followLegTrackToFix = tf;
}

bool FlightPlan::followLegTrackToFixes() const
{
    return _followLegTrackToFix;
}

std::string FlightPlan::icaoAircraftCategory() const
{
    std::string r;
    r.push_back(_aircraftCategory);
    return r;
}

void FlightPlan::setIcaoAircraftCategory(const std::string& cat)
{
    if (cat.empty()) {
        throw sg_range_exception("Invalid ICAO aircraft category:", cat);
    }

    if ((cat[0] < ICAO_AIRCRAFT_CATEGORY_A) || (cat[0] > ICAO_AIRCRAFT_CATEGORY_E)) {
        throw sg_range_exception("Invalid ICAO aircraft category:", cat);
    }

    _aircraftCategory = cat[0];
}

void FlightPlan::setIcaoAircraftType(const string &ty)
{
    _aircraftType = ty;
}

bool FlightPlan::parseICAOLatLon(const std::string& s, SGGeod& p)
{
    if (s.size() == 7) {
        double latDegrees = std::stoi(s);
        double lonDegrees = std::stoi(s.substr(3, 3));
        if (s[2] == 'S') latDegrees = -latDegrees;
        if (s[6] == 'W') lonDegrees = -lonDegrees;
        p = SGGeod::fromDeg(lonDegrees, latDegrees);
        return true;
    } else if (s.size() == 11) {
        // problem here is we have minutes, not decimal degrees
        double latDegrees = std::stoi(s.substr(0, 2));
        double latMinutes = std::stoi(s.substr(2, 2));
        latDegrees += (latMinutes / 60.0);
        double lonDegrees = std::stoi(s.substr(5, 3));
        double lonMinutes = std::stoi(s.substr(8, 2));
        lonDegrees += (lonMinutes / 60.0);

        if (s[4] == 'S') latDegrees = -latDegrees;
        if (s[10] == 'W') lonDegrees = -lonDegrees;
        p = SGGeod::fromDeg(lonDegrees, latDegrees);
        return true;
    }

    return false;
}

bool FlightPlan::parseICAORouteString(const std::string& routeData)
{
    auto tokens = simgear::strutils::split(routeData);
    if (tokens.empty())
        return false;
    
    std::string tk;
    std::string nextToken;
    
    FGAirportRef firstICAO = FGAirport::findByIdent(tokens.front());
    unsigned int i = 0;

    if (firstICAO) {
        // route string starts with an airport, let's see if it matches
        // our departure airport
        if (!_departure) {
            setDeparture(firstICAO);
        } else if (_departure != firstICAO) {
            SG_LOG(SG_AUTOPILOT, SG_WARN, "ICAO route begins with an airport which is not the departure airport:" << tokens.front());
            return false;
        }
        ++i; // either way, skip the ICAO token now
    }

    WayptVec enroute;
    SGGeod currentPos;
    if (_departure)
        currentPos = _departure->geod();

    for (; i < tokens.size(); ++i) {
        tk = tokens.at(i);
        // update current position for next search
        if (!enroute.empty()) {
            currentPos = enroute.back()->position();
        }

        if (isdigit(tk.front())) {
            // might be a lat lon (but some airway idents also start with a digit...)
            SGGeod geod;
            bool ok = parseICAOLatLon(tk, geod);
            if (ok) {
                enroute.push_back(new BasicWaypt(geod, tk, this));
                continue;
            }
        }

        nextToken = (i < (tokens.size() - 1)) ? tokens.at(i+1) : std::string();

        if (tk == "DCT") {
            if (nextToken.empty()) {
                SG_LOG(SG_AUTOPILOT, SG_WARN, "ICAO route DIRECT segment missing waypoint");
                return false;
            }

            FGPositionedRef wpt = FGPositioned::findClosestWithIdent(nextToken, currentPos);
            if (!wpt) {
                SG_LOG(SG_AUTOPILOT, SG_WARN, "ICAO route waypoint not found:" << nextToken);
                return false;
            }
            enroute.push_back(new NavaidWaypoint(wpt, this));
            ++i;
        } else if (tk == "STAR") {
            // look for a STAR based on the preceeding transition point
            auto starTrans = _destination->selectSTARByEnrouteTransition(enroute.back()->source());
            if (!starTrans) {
                SG_LOG(SG_AUTOPILOT, SG_WARN, "ICAO route couldn't find STAR transitioning from " <<
                       enroute.back()->source()->ident());
            } else {
                setSTAR(starTrans);
            }
        } else if (tk == "SID") {
            // select a SID based on the next point
            FGPositionedRef wpt = FGPositioned::findClosestWithIdent(nextToken, currentPos);
            auto sidTrans = _departure->selectSIDByEnrouteTransition(wpt);
            if (!sidTrans) {
                SG_LOG(SG_AUTOPILOT, SG_WARN, "ICAO route couldn't find SID transitioning to " << nextToken);
            } else {
                setSID(sidTrans);
            }
            ++i;
        } else {
            if (nextToken.empty()) {
                SG_LOG(SG_AUTOPILOT, SG_WARN, "ICAO route airway segment missing transition:" << tk);
                return false;
            }
            
            auto nav = Airway::highLevel()->findNodeByIdent(nextToken, currentPos);
            if (!nav)
                nav = Airway::lowLevel()->findNodeByIdent(nextToken, currentPos);
            if (!nav) {
                SG_LOG(SG_AUTOPILOT, SG_WARN, "ICAO route waypoint not found:" << nextToken);
                return false;
            }
            
            WayptRef toNav = new NavaidWaypoint(nav, nullptr); // temp waypoint for lookup
            WayptRef previous;
            if (enroute.empty()) {
                if (_sid) {
                    if (!_sidTransition.empty()) {
                        previous = _sid->findTransitionByName(_sidTransition)->enroute();
                    } else {
                        // final wayypoint of common section
                        previous = _sid->common().back();
                    }
                } else {
                    SG_LOG(SG_AUTOPILOT, SG_WARN, "initial airway needs anchor point from SID:" << tk);
                    return false;
                }
            } else {
                previous = enroute.back();
            }
            
            AirwayRef  way = Airway::findByIdentAndVia(tk, enroute.back(), toNav);
            if (way) {
                enroute.push_back(new Via(this, way, nav));
                ++i;
            } else {
                SG_LOG(SG_AUTOPILOT, SG_WARN, "ICAO route unknown airway:" << tk);
                return false;
            }
        }
    } // of token iteration
    
    lockDelegates();
    _waypointsChanged = true;

    SG_LOG(SG_AUTOPILOT, SG_INFO, "adding waypoints from string");
    // rebuild legs from waypoints we created
    for (auto l : _legs) {
        delete l;
    }
    _legs.clear();
    insertWayptsAtIndex(enroute, 0);

    unlockDelegates();

    SG_LOG(SG_AUTOPILOT, SG_INFO, "legs now:" << numLegs());

    return true;
}

std::string FlightPlan::asICAORouteString() const
{
    std::string result;
    if (!_sidTransition.empty())
        result += _sidTransition + " ";
    
    for (auto l : _legs) {
        const auto wpt = l->waypoint();
        
        AirwayRef nextLegAirway;
        if (l->nextLeg() && l->nextLeg()->waypoint()->flag(WPT_VIA)) {
            nextLegAirway = static_cast<Airway*>(l->nextLeg()->waypoint()->owner());
        }
        
        if (wpt->flag(WPT_GENERATED)) {
            if (wpt->flag(WPT_VIA)) {
                AirwayRef awy = static_cast<Airway*>(wpt->owner());
                if (awy == nextLegAirway) {
                    // skipepd entirely, next leg will output the airway
                    continue;
                }
                
                result += awy->ident() + " ";
            }
        } else if (wpt->type() == "navaid") {
            // technically we need DCT unless both ends of the leg are
            // defined geographically
            result += "DCT ";
        }
        result += wpt->icaoDescription() + " ";
    }

    if (!_starTransition.empty())
        result += _starTransition;

    return result;
}

void FlightPlan::setFlightRules(ICAOFlightRules rules)
{
    _flightRules = rules;
}

ICAOFlightRules FlightPlan::flightRules() const
{
    return _flightRules;
}

void FlightPlan::setCallsign(const std::string& callsign)
{
    _callsign = callsign;
}
    
void FlightPlan::setRemarks(const std::string& remarks)
{
    _remarks = remarks;
}

void FlightPlan::setFlightType(ICAOFlightType type)
{
    _flightType = type;
}

ICAOFlightType FlightPlan::flightType() const
{
    return _flightType;
}
    
void FlightPlan::setCruiseSpeedKnots(int kts)
{
    lockDelegates();
    _cruiseAirspeedKnots = kts;
    _cruiseAirspeedMach = 0.0;
    _cruiseDataChanged = true;
    unlockDelegates();
}

int FlightPlan::cruiseSpeedKnots() const
{
    return _cruiseAirspeedKnots;
}

void FlightPlan::setCruiseSpeedMach(double mach)
{
    lockDelegates();
    _cruiseDataChanged = true;
    _cruiseAirspeedMach = mach;
    _cruiseAirspeedKnots = 0;
    unlockDelegates();
}

double FlightPlan::cruiseSpeedMach() const
{
    return _cruiseAirspeedMach;
}

void FlightPlan::setCruiseFlightLevel(int flightLevel)
{
    lockDelegates();
    _cruiseDataChanged = true;
    _cruiseFlightLevel = flightLevel;
    _cruiseAltitudeFt = 0;
    unlockDelegates();
}

int FlightPlan::cruiseFlightLevel() const
{
    return _cruiseFlightLevel;
}

void FlightPlan::setCruiseAltitudeFt(int altFt)
{
    lockDelegates();
    _cruiseDataChanged = true;
    _cruiseAltitudeFt = altFt;
    _cruiseFlightLevel = 0;
    unlockDelegates();
}

int FlightPlan::cruiseAltitudeFt() const
{
    return _cruiseAltitudeFt;
}

void FlightPlan::forEachLeg(const LegVisitor& lv)
{
    std::for_each(_legs.begin(), _legs.end(), lv);
}


int FlightPlan::indexOfFirstNonDepartureWaypoint() const
{
    const auto numLegs = _legs.size();
    for (int i = 0; i < numLegs; ++i) {
        if (!(_legs.at(i)->waypoint()->flags() & WPT_DEPARTURE))
            return i ;
    }
    
    // all waypoints are marked as departure
    return -1;
}

int FlightPlan::indexOfFirstArrivalWaypoint() const
{
    const auto numLegs = _legs.size();
    for (int i = 0; i < numLegs; ++i) {
        if (_legs.at(i)->waypoint()->flags() & WPT_ARRIVAL)
            return i;
    }
    
    // no waypoints are marked as arrival
    return -1;
}

int FlightPlan::indexOfFirstApproachWaypoint() const
{
    const auto numLegs = _legs.size();
    for (int i = 0; i < numLegs; ++i) {
        if (_legs.at(i)->waypoint()->flags() & WPT_APPROACH)
            return i;
    }
    
    // no waypoints are marked as arrival
    return -1;
}

int FlightPlan::indexOfDestinationRunwayWaypoint() const
{
    if (!_destinationRunway)
        return -1;
    
    // work backwards in case the departure and destination match
    // this way we'll find the one we want
    for (int i = numLegs() - 1; i >= 0; i--) {
        if (_legs.at(i)->waypoint()->source() == _destinationRunway) {
            return i;
        }
    }
    
    return -1;
}

void FlightPlan::DelegateFactory::destroyFlightPlanDelegate(FlightPlan* fp, Delegate* d)
{
    // mimic old behaviour before destroyFlightPlanDelegate was added
    SG_UNUSED(fp);
    delete d;
}


} // of namespace flightgear
