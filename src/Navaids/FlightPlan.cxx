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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "FlightPlan.hxx"

// std
#include <map>
#include <fstream>
#include <cassert>

// Boost
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>

// SimGear
#include <simgear/structure/exception.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/magvar/magvar.hxx>
#include <simgear/timing/sg_time.hxx>
#include <simgear/misc/sgstream.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/xml/easyxml.hxx>

// FlightGear
#include <Main/globals.hxx>
#include "Main/fg_props.hxx"
#include <Navaids/procedure.hxx>
#include <Navaids/waypoint.hxx>
#include <Navaids/routePath.hxx>

using std::string;
using std::vector;
using std::endl;
using std::fstream;

namespace flightgear {

typedef std::vector<FlightPlan::DelegateFactory*> FPDelegateFactoryVec;
static FPDelegateFactoryVec static_delegateFactories;
  
FlightPlan::FlightPlan() :
  _delegateLock(0),
  _currentIndex(-1),
    _followLegTrackToFix(true),
    _aircraftCategory(ICAO_AIRCRAFT_CATEGORY_C),
  _departureRunway(NULL),
  _destinationRunway(NULL),
  _sid(NULL),
  _star(NULL),
  _approach(NULL),
  _totalDistance(0.0),
  _delegate(NULL)
{
  _departureChanged = _arrivalChanged = _waypointsChanged = _currentWaypointChanged = false;
  
  BOOST_FOREACH(DelegateFactory* factory, static_delegateFactories) {
    Delegate* d = factory->createFlightPlanDelegate(this);
    if (d) { // factory might not always create a delegate
      d->_deleteWithPlan = true;
      addDelegate(d);
    }
  }
}
  
FlightPlan::~FlightPlan()
{
// delete all delegates which we own.
  Delegate* d = _delegate;
  while (d) {
    Delegate* cur = d;
    d = d->_inner;
    if (cur->_deleteWithPlan) {
      delete cur;
    }
  }
    
// delete legs
    BOOST_FOREACH(Leg* l, _legs) {
        delete l;
    }
}
  
FlightPlan* FlightPlan::clone(const string& newIdent) const
{
  FlightPlan* c = new FlightPlan();
  c->_ident = newIdent.empty() ? _ident : newIdent;
  c->lockDelegate();
  
// copy destination / departure data.
  c->setDeparture(_departure);
  c->setDeparture(_departureRunway);
  
  if (_approach) {
    c->setApproach(_approach);
  } else if (_destinationRunway) {
    c->setDestination(_destinationRunway);
  } else if (_destination) {
    c->setDestination(_destination);
  }
  
  c->setSTAR(_star);
  c->setSID(_sid);
  
// copy legs
  c->_waypointsChanged = true;
  for (int l=0; l < numLegs(); ++l) {
    c->_legs.push_back(_legs[l]->cloneFor(c));
  }
  c->unlockDelegate();
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
  
FlightPlan::Leg* FlightPlan::insertWayptAtIndex(Waypt* aWpt, int aIndex)
{
  if (!aWpt) {
    return NULL;
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
  
  LegVec::iterator it = _legs.begin();
  it += index;
  
  int endIndex = index + wps.size() - 1;
  if (_currentIndex >= endIndex) {
    _currentIndex += wps.size();
  }
 
  LegVec newLegs;
  BOOST_FOREACH(WayptRef wp, wps) {
    newLegs.push_back(new Leg(this, wp));
  }
  
  lockDelegate();
  _waypointsChanged = true;
  _legs.insert(it, newLegs.begin(), newLegs.end());
  unlockDelegate();
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
  
  lockDelegate();
  _waypointsChanged = true;
  
  LegVec::iterator it = _legs.begin();
  it += index;
  Leg* l = *it;
  _legs.erase(it);
  delete l;
  
  if (_currentIndex == index) {
    // current waypoint was removed
    _currentWaypointChanged = true;
  } else if (_currentIndex > index) {
    --_currentIndex; // shift current index down if necessary
  }
  
  unlockDelegate();
}
  
void FlightPlan::clear()
{
  lockDelegate();
  _waypointsChanged = true;
  _currentWaypointChanged = true;
  _arrivalChanged = true;
  _departureChanged = true;
  
  _currentIndex = -1;
  BOOST_FOREACH(Leg* l, _legs) {
    delete l;
  }
  _legs.clear();  
  
  if (_delegate) {
    _delegate->runCleared();
  }
  unlockDelegate();
}
  
class RemoveWithFlag
{
public:
  RemoveWithFlag(WayptFlag f) : flag(f), delCount(0) { }
  
  int numDeleted() const { return delCount; }
  
  bool operator()(FlightPlan::Leg* leg) const
  {
    if (leg->waypoint()->flag(flag)) {
      delete leg;
      ++delCount;
      return true;
    }
    
    return false;
  }
private:
  WayptFlag flag;
  mutable int delCount;
};
  
int FlightPlan::clearWayptsWithFlag(WayptFlag flag)
{
  int count = 0;
// first pass, fix up currentIndex
  for (int i=0; i<_currentIndex; ++i) {
    Leg* l = _legs[i];
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
        SG_LOG(SG_GENERAL, SG_INFO, "currentIsBeingCleared:" << currentIsBeingCleared);
        _currentIndex = -1;
    }
  
// now delete and remove
  RemoveWithFlag rf(flag);
  LegVec::iterator it = std::remove_if(_legs.begin(), _legs.end(), rf);
  if (it == _legs.end()) {
    return 0; // nothing was cleared, don't fire the delegate
  }
  
  lockDelegate();
  _waypointsChanged = true;
  if ((count > 0) || currentIsBeingCleared) {
    _currentWaypointChanged = true;
  }
  
  _legs.erase(it, _legs.end());
    
  if (_legs.empty()) { // maybe all legs were deleted
    if (_delegate) {
      _delegate->runCleared();
    }
  }
  
  unlockDelegate();
  return rf.numDeleted();
}
  
void FlightPlan::setCurrentIndex(int index)
{
  if ((index < -1) || (index >= numLegs())) {
    throw sg_range_exception("invalid leg index", "FlightPlan::setCurrentIndex");
  }
  
  if (index == _currentIndex) {
    return;
  }
  
  lockDelegate();
  _currentIndex = index;
  _currentWaypointChanged = true;
  unlockDelegate();
}
    
void FlightPlan::finish()
{
    if (_currentIndex == -1) {
        return;
    }
    
    lockDelegate();
    _currentIndex = -1;
    _currentWaypointChanged = true;
    
    if (_delegate) {
        _delegate->runFinished();
    }
    
    unlockDelegate();
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
    if (_legs[i]->waypoint()->source() == aPos) {
      return i;
    }
  }
  
  return -1;
}

FlightPlan::Leg* FlightPlan::currentLeg() const
{
  if ((_currentIndex < 0) || (_currentIndex >= numLegs()))
    return NULL;
  return legAtIndex(_currentIndex);
}

FlightPlan::Leg* FlightPlan::previousLeg() const
{
  if (_currentIndex <= 0) {
    return NULL;
  }
  
  return legAtIndex(_currentIndex - 1);
}

FlightPlan::Leg* FlightPlan::nextLeg() const
{
  if ((_currentIndex < 0) || ((_currentIndex + 1) >= numLegs())) {
    return NULL;
  }
  
  return legAtIndex(_currentIndex + 1);
}

FlightPlan::Leg* FlightPlan::legAtIndex(int index) const
{
  if ((index < 0) || (index >= numLegs())) {
    throw sg_range_exception("index out of range", "FlightPlan::legAtIndex");
  }
  
  return _legs[index];
}
  
int FlightPlan::findLegIndex(const Leg *l) const
{
  for (unsigned int i=0; i<_legs.size(); ++i) {
    if (_legs[i] == l) {
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
  
  lockDelegate();
  _departureChanged = true;
  _departure = apt;
  _departureRunway = NULL;
  setSID((SID*)NULL);
  unlockDelegate();
}
  
void FlightPlan::setDeparture(FGRunway* rwy)
{
  if (_departureRunway == rwy) {
    return;
  }
  
  lockDelegate();
  _departureChanged = true;

  _departureRunway = rwy;
  if (rwy->airport() != _departure) {
    _departure = rwy->airport();
    setSID((SID*)NULL);
  }
  unlockDelegate();
}
  
void FlightPlan::setSID(SID* sid, const std::string& transition)
{
  if (sid == _sid) {
    return;
  }
  
  lockDelegate();
  _departureChanged = true;
  _sid = sid;
  _sidTransition = transition;
  unlockDelegate();
}
  
void FlightPlan::setSID(Transition* trans)
{
  if (!trans) {
    setSID((SID*) NULL);
    return;
  }
  
  if (trans->parent()->type() != PROCEDURE_SID)
    throw sg_exception("FlightPlan::setSID: transition does not belong to a SID");
  
  setSID((SID*) trans->parent(), trans->ident());
}
  
Transition* FlightPlan::sidTransition() const
{
  if (!_sid || _sidTransition.empty()) {
    return NULL;
  }
  
  return _sid->findTransitionByName(_sidTransition);
}

void FlightPlan::setDestination(FGAirport* apt)
{
  if (apt == _destination) {
    return;
  }
  
  lockDelegate();
  _arrivalChanged = true;
  _destination = apt;
  _destinationRunway = NULL;
  setSTAR((STAR*)NULL);
  setApproach(NULL);
  unlockDelegate();
}
    
void FlightPlan::setDestination(FGRunway* rwy)
{
  if (_destinationRunway == rwy) {
    return;
  }
  
  lockDelegate();
  _arrivalChanged = true;
  _destinationRunway = rwy;
  if (_destination != rwy->airport()) {
    _destination = rwy->airport();
    setSTAR((STAR*)NULL);
  }
  
  unlockDelegate();
}
  
void FlightPlan::setSTAR(STAR* star, const std::string& transition)
{
  if (_star == star) {
    return;
  }
  
  lockDelegate();
  _arrivalChanged = true;
  _star = star;
  _starTransition = transition;
  unlockDelegate();
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

Transition* FlightPlan::starTransition() const
{
  if (!_star || _starTransition.empty()) {
    return NULL;
  }
  
  return _star->findTransitionByName(_starTransition);
}
  
void FlightPlan::setApproach(flightgear::Approach *app)
{
  if (_approach == app) {
    return;
  }
  
  lockDelegate();
  _arrivalChanged = true;
  _approach = app;
  if (app) {
    // keep runway + airport in sync
    if (_destinationRunway != _approach->runway()) {
      _destinationRunway = _approach->runway();
    }
    
    if (_destination != _destinationRunway->airport()) {
      _destination = _destinationRunway->airport();
    }
  }
  unlockDelegate();
}
  
bool FlightPlan::save(const SGPath& path)
{
  SG_LOG(SG_NAVAID, SG_INFO, "Saving route to " << path.str());
  try {
    SGPropertyNode_ptr d(new SGPropertyNode);
    d->setIntValue("version", 2);
    
    if (_departure) {
      d->setStringValue("departure/airport", _departure->ident());
      if (_sid) {
        d->setStringValue("departure/sid", _sid->ident());
      }
      
      if (_departureRunway) {
        d->setStringValue("departure/runway", _departureRunway->ident());
      }
    }
    
    if (_destination) {
      d->setStringValue("destination/airport", _destination->ident());
      if (_star) {
        d->setStringValue("destination/star", _star->ident());
      }
      
      if (_approach) {
        d->setStringValue("destination/approach", _approach->ident());
      }
      
      //d->setStringValue("destination/transition", destination->getStringValue("transition"));
      
      if (_destinationRunway) {
        d->setStringValue("destination/runway", _destinationRunway->ident());
      }
    }
    
    // route nodes
    SGPropertyNode* routeNode = d->getChild("route", 0, true);
    for (unsigned int i=0; i<_legs.size(); ++i) {
      Waypt* wpt = _legs[i]->waypoint();
      wpt->saveAsNode(routeNode->getChild("wp", i, true));
    } // of waypoint iteration
    writeProperties(path.str(), d, true /* write-all */);
    return true;
  } catch (sg_exception& e) {
    SG_LOG(SG_NAVAID, SG_ALERT, "Failed to save flight-plan '" << path.str() << "'. " << e.getMessage());
    return false;
  }
}

bool FlightPlan::load(const SGPath& path)
{
  if (!path.exists())
  {
    SG_LOG(SG_NAVAID, SG_ALERT, "Failed to load flight-plan '" << path.str()
           << "'. The file does not exist.");
    return false;
  }

  SG_LOG(SG_NAVAID, SG_INFO, "going to read flight-plan from:" << path.str());
  
  bool Status = false;
  lockDelegate();

  // try different file formats
  if (loadGpxFormat(path)) // GPX format
      Status = true;
  else
  if (loadXmlFormat(path)) // XML property data
      Status = true;
  else
  if (loadPlainTextFormat(path)) // simple textual list of waypoints
      Status = true;

  _waypointsChanged = true;
  unlockDelegate();

  return Status;
}

/** XML loader for GPX file format */
class GpxXmlVisitor : public XMLVisitor
{
public:
    GpxXmlVisitor(FlightPlan* fp) : _fp(fp), _lat(-9999), _lon(-9999) {}

    virtual void startElement (const char * name, const XMLAttributes &atts);
    virtual void endElement (const char * name);
    virtual void data (const char * s, int length);

private:
    FlightPlan* _fp;
    double      _lat, _lon;
    string      _element;
    string      _waypoint;
};

void GpxXmlVisitor::startElement(const char * name, const XMLAttributes &atts)
{
    _element = name;
    if (strcmp(name, "rtept")==0)
    {
        _waypoint = "";
        _lat = _lon = -9999;

        const char* slat = atts.getValue("lat");
        const char* slon = atts.getValue("lon");
        if (slat && slon)
        {
            _lat = atof(slat);
            _lon = atof(slon);
        }
    }
}

void GpxXmlVisitor::data(const char * s, int length)
{
    // use "name" when given, otherwise use "cmt" (comment) as ID
    if ((_element == "name")||
        ((_waypoint == "")&&(_element == "cmt")))
    {
        char* buf = (char*) malloc(length+1);
        memcpy(buf, s, length);
        buf[length] = 0;
        _waypoint = buf;
        free(buf);
    }
}

void GpxXmlVisitor::endElement(const char * name)
{
    _element = "";
    if (strcmp(name, "rtept") == 0)
    {
        if (_lon > -9990.0)
        {
            _fp->insertWayptAtIndex(new BasicWaypt(SGGeod::fromDeg(_lon, _lat), _waypoint.c_str(), NULL), -1);
        }
    }
}

/** Load a flightplan in GPX format */
bool FlightPlan::loadGpxFormat(const SGPath& path)
{
    if (path.lower_extension() != "gpx")
    {
        // not a valid GPX file
        return false;
    }

    _legs.clear();
    GpxXmlVisitor gpxVistor(this);
    try
    {
        readXML(path.str(), gpxVistor);
    } catch (sg_exception& e)
    {
        // XML parsing fails => not a GPX XML file
        SG_LOG(SG_NAVAID, SG_ALERT, "Failed to load flight-plan in GPX format: '" << e.getOrigin()
                     << "'. " << e.getMessage());
        return false;
    }

    if (numLegs() == 0)
    {
        SG_LOG(SG_NAVAID, SG_ALERT, "Failed to load flight-plan in GPX format. No route found.");
        return false;
    }

    return true;
}

/** Load a flightplan in FlightGear XML property format */
bool FlightPlan::loadXmlFormat(const SGPath& path)
{
  SGPropertyNode_ptr routeData(new SGPropertyNode);
  try {
    readProperties(path.str(), routeData);
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
        setSID(_departure->findSIDWithIdent(dep->getStringValue("sid")));
      }
   // departure->setStringValue("transition", dep->getStringValue("transition"));
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
        setSTAR(_destination->findSTARWithIdent(dst->getStringValue("star")));
      }
      
      if (dst->hasChild("approach")) {
        setApproach(_destination->findApproachWithIdent(dst->getStringValue("approach")));
      }
    }
    
   // destination->setStringValue("transition", dst->getStringValue("transition"));
  }
  
  // alternate
  SGPropertyNode* alt = routeData->getChild("alternate");
  if (alt) {
    //alternate->setStringValue(alt->getStringValue("airport"));
  }
  
  // cruise
  SGPropertyNode* crs = routeData->getChild("cruise");
  if (crs) {
 //   cruise->setDoubleValue("speed-kts", crs->getDoubleValue("speed-kts"));
   // cruise->setDoubleValue("mach", crs->getDoubleValue("mach"));
   // cruise->setDoubleValue("altitude-ft", crs->getDoubleValue("altitude-ft"));
  } // of cruise data loading
  
}

void FlightPlan::loadVersion2XMLRoute(SGPropertyNode_ptr routeData)
{
  loadXMLRouteHeader(routeData);
  
  // route nodes
  _legs.clear();
  SGPropertyNode_ptr routeNode = routeData->getChild("route", 0);
  if (routeNode.valid()) {
    for (int i=0; i<routeNode->nChildren(); ++i) {
      SGPropertyNode_ptr wpNode = routeNode->getChild("wp", i);
      Leg* l = new Leg(this, Waypt::createFromProperties(NULL, wpNode));
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
    Leg* l = new Leg(this, parseVersion1XMLWaypt(wpNode));
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
                                       aWP->getDoubleValue("latitude-deg")), ident, NULL);
    
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
    
    w = new BasicWaypt(pos, ident, NULL);
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
    sg_gzifstream in(path.str().c_str());
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
      
      _legs.push_back(new Leg(this, w));
    } // of line iteration
  } catch (sg_exception& e) {
    SG_LOG(SG_NAVAID, SG_ALERT, "Failed to load route from: '" << path.str() << "'. " << e.getMessage());
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
  
WayptRef FlightPlan::waypointFromString(const string& tgt )
{
  string target(boost::to_upper_copy(tgt));
  WayptRef wpt;
  
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
  pos = target.find( ',' );
  if ( pos != string::npos ) {
    double lon = atof( target.substr(0, pos).c_str());
    double lat = atof( target.c_str() + pos + 1);
    char buf[32];
    char ew = (lon < 0.0) ? 'W' : 'E';
    char ns = (lat < 0.0) ? 'S' : 'N';
    snprintf(buf, 32, "%c%03d%c%03d", ew, (int) fabs(lon), ns, (int)fabs(lat));
    
    wpt = new BasicWaypt(SGGeod::fromDeg(lon, lat), buf, NULL);
    if (altSetting != RESTRICT_NONE) {
      wpt->setAltitude(altFt, altSetting);
    }
    return wpt;
  }
  
  SGGeod basePosition;
  if (_legs.empty()) {
    // route is empty, use current position
    basePosition = globals->get_aircraft_position();
  } else {
    basePosition = _legs.back()->waypoint()->position();
  }
  
  string_list pieces(simgear::strutils::split(target, "/"));
  FGPositionedRef p = FGPositioned::findClosestWithIdent(pieces.front(), basePosition);
  if (!p) {
    SG_LOG( SG_NAVAID, SG_INFO, "Unable to find FGPositioned with ident:" << pieces.front());
    return NULL;
  }
  
  double magvar = magvarDegAt(basePosition);
  
  if (pieces.size() == 1) {
    wpt = new NavaidWaypoint(p, NULL);
  } else if (pieces.size() == 3) {
    // navaid/radial/distance-nm notation
    double radial = atof(pieces[1].c_str()),
    distanceNm = atof(pieces[2].c_str());
    radial += magvar;
    wpt = new OffsetNavaidWaypoint(p, NULL, radial, distanceNm);
  } else if (pieces.size() == 2) {
    FGAirport* apt = dynamic_cast<FGAirport*>(p.ptr());
    if (!apt) {
      SG_LOG(SG_NAVAID, SG_INFO, "Waypoint is not an airport:" << pieces.front());
      return NULL;
    }
    
    if (!apt->hasRunwayWithIdent(pieces[1])) {
      SG_LOG(SG_NAVAID, SG_INFO, "No runway: " << pieces[1] << " at " << pieces[0]);
      return NULL;
    }
    
    FGRunway* runway = apt->getRunwayByIdent(pieces[1]);
    wpt = new NavaidWaypoint(runway, NULL);
  } else if (pieces.size() == 4) {
    // navid/radial/navid/radial notation     
    FGPositionedRef p2 = FGPositioned::findClosestWithIdent(pieces[2], basePosition);
    if (!p2) {
      SG_LOG( SG_NAVAID, SG_INFO, "Unable to find FGPositioned with ident:" << pieces[2]);
      return NULL;
    }
    
    double r1 = atof(pieces[1].c_str()),
    r2 = atof(pieces[3].c_str());
    r1 += magvar;
    r2 += magvar;
    
    SGGeod intersection;
    bool ok = SGGeodesy::radialIntersection(p->geod(), r1, p2->geod(), r2, intersection);
    if (!ok) {
      SG_LOG(SG_NAVAID, SG_INFO, "no valid intersection for:" << target);
      return NULL;
    }
    
    std::string name = p->ident() + "-" + p2->ident();
    wpt = new BasicWaypt(intersection, name, NULL);
  }
  
  if (!wpt) {
    SG_LOG(SG_NAVAID, SG_INFO, "Unable to parse waypoint:" << target);
    return NULL;
  }
  
  if (altSetting != RESTRICT_NONE) {
    wpt->setAltitude(altFt, altSetting);
  }
  return wpt;
}
  
FlightPlan::Leg::Leg(FlightPlan* owner, WayptRef wpt) :
  _parent(owner),
  _speedRestrict(RESTRICT_NONE),
  _altRestrict(RESTRICT_NONE),
  _waypt(wpt)
{
  if (!wpt.valid()) {
    throw sg_exception("can't create FlightPlan::Leg without underlying waypoint");
  }
  _speed = _altitudeFt = 0;
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
    
void FlightPlan::lockDelegate()
{
  if (_delegateLock == 0) {
    assert(!_departureChanged && !_arrivalChanged && 
           !_waypointsChanged && !_currentWaypointChanged);
  }
  
  ++_delegateLock;
}

void FlightPlan::unlockDelegate()
{
  assert(_delegateLock > 0);
  if (_delegateLock > 1) {
    --_delegateLock;
    return;
  }
  
  if (_departureChanged) {
    _departureChanged = false;
    if (_delegate) {
      _delegate->runDepartureChanged();
    }
  }
  
  if (_arrivalChanged) {
    _arrivalChanged = false;
    if (_delegate) {
      _delegate->runArrivalChanged();
    }
  }
  
  if (_waypointsChanged) {
    _waypointsChanged = false;
    rebuildLegData();
    if (_delegate) {
      _delegate->runWaypointsChanged();
    }
  }
  
  if (_currentWaypointChanged) {
    _currentWaypointChanged = false;
    if (_delegate) {
      _delegate->runCurrentWaypointChanged();
    }
  }

  --_delegateLock;
}
  
void FlightPlan::registerDelegateFactory(DelegateFactory* df)
{
  FPDelegateFactoryVec::iterator it = std::find(static_delegateFactories.begin(),
                                                static_delegateFactories.end(), df);
  if (it != static_delegateFactories.end()) {
    throw  sg_exception("duplicate delegate factory registration");
  }
  
  static_delegateFactories.push_back(df);
}
  
void FlightPlan::unregisterDelegateFactory(DelegateFactory* df)
{
  FPDelegateFactoryVec::iterator it = std::find(static_delegateFactories.begin(),
                                                static_delegateFactories.end(), df);
  if (it == static_delegateFactories.end()) {
    return;
  }
  
  static_delegateFactories.erase(it);
}
  
void FlightPlan::addDelegate(Delegate* d)
{
  // wrap any existing delegate(s) in the new one
  d->_inner = _delegate;
  _delegate = d;
}

void FlightPlan::removeDelegate(Delegate* d)
{
  if (d == _delegate) {
    _delegate = _delegate->_inner;
  } else if (_delegate) {
    _delegate->removeInner(d);
  }
}
  
FlightPlan::Delegate::Delegate() :
  _deleteWithPlan(false),
  _inner(NULL)
{
}

FlightPlan::Delegate::~Delegate()
{  
}

void FlightPlan::Delegate::removeInner(Delegate* d)
{
  if (!_inner) {
      throw sg_exception("FlightPlan delegate not found");
  }
  
  if (_inner == d) {
    // replace with grand-child
    _inner = d->_inner;
  } else { // recurse downwards
    _inner->removeInner(d);
  }
}

void FlightPlan::Delegate::runDepartureChanged()
{
  if (_inner) _inner->runDepartureChanged();
  departureChanged();
}

void FlightPlan::Delegate::runArrivalChanged()
{
  if (_inner) _inner->runArrivalChanged();
  arrivalChanged();
}

void FlightPlan::Delegate::runWaypointsChanged()
{
  if (_inner) _inner->runWaypointsChanged();
  waypointsChanged();
}
  
void FlightPlan::Delegate::runCurrentWaypointChanged()
{
  if (_inner) _inner->runCurrentWaypointChanged();
  currentWaypointChanged();
}

void FlightPlan::Delegate::runCleared()
{
  if (_inner) _inner->runCleared();
  cleared();
}  

void FlightPlan::Delegate::runFinished()
{
    if (_inner) _inner->runFinished();
    endOfFlightPlan();
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

    
} // of namespace flightgear
