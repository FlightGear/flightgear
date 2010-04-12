// route_mgr.cxx - manage a route (i.e. a collection of waypoints)
//
// Written by Curtis Olson, started January 2004.
//            Norman Vine
//            Melchior FRANZ
//
// Copyright (C) 2004  Curtis L. Olson  - http://www.flightgear.org/~curt
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

#ifdef HAVE_WINDOWS_H
#include <time.h>
#endif

#include <simgear/compiler.h>

#include "route_mgr.hxx"

#include <boost/algorithm/string/case_conv.hpp>

#include <simgear/misc/strutils.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/misc/sgstream.hxx>

#include <simgear/props/props_io.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/route/route.hxx>
#include <simgear/sg_inlines.h>

#include "Main/fg_props.hxx"
#include "Navaids/positioned.hxx"
#include "Airports/simple.hxx"
#include "Airports/runways.hxx"

#include "FDM/flight.hxx" // for getting ground speed

#define RM "/autopilot/route-manager/"

static double get_ground_speed() {
  // starts in ft/s so we convert to kts
  static const SGPropertyNode * speedup_node = fgGetNode("/sim/speed-up");

  double ft_s = cur_fdm_state->get_V_ground_speed()
      * speedup_node->getIntValue();
  double kts = ft_s * SG_FEET_TO_METER * 3600 * SG_METER_TO_NM;
  return kts;
}

FGRouteMgr::FGRouteMgr() :
    _route( new SGRoute ),
    input(fgGetNode( RM "input", true )),
    mirror(fgGetNode( RM "route", true ))
{
    listener = new InputListener(this);
    input->setStringValue("");
    input->addChangeListener(listener);
}


FGRouteMgr::~FGRouteMgr() {
    input->removeChangeListener(listener);
    
    delete listener;
    delete _route;
}


void FGRouteMgr::init() {
  SGPropertyNode_ptr rm(fgGetNode(RM));
  
  lon = fgGetNode( "/position/longitude-deg", true );
  lat = fgGetNode( "/position/latitude-deg", true );
  alt = fgGetNode( "/position/altitude-ft", true );
  magvar = fgGetNode("/environment/magnetic-variation-deg", true);
     
  departure = fgGetNode(RM "departure", true);
  departure->tie("airport", SGRawValueMethods<FGRouteMgr, const char*>(*this, 
    &FGRouteMgr::getDepartureICAO, &FGRouteMgr::setDepartureICAO));
  departure->tie("name", SGRawValueMethods<FGRouteMgr, const char*>(*this, 
    &FGRouteMgr::getDepartureName, NULL));
    
// init departure information from current location
  SGGeod pos = SGGeod::fromDegFt(lon->getDoubleValue(), lat->getDoubleValue(), alt->getDoubleValue());
  _departure = FGAirport::findClosest(pos, 20.0);
  if (_departure) {
    FGRunway* active = _departure->getActiveRunwayForUsage();
    departure->setStringValue("runway", active->ident().c_str());
  } else {
    departure->setStringValue("runway", "");
  }
  
  departure->getChild("etd", 0, true);
  departure->getChild("takeoff-time", 0, true);

  destination = fgGetNode(RM "destination", true);
  destination->getChild("airport", 0, true);
  
  destination->tie("airport", SGRawValueMethods<FGRouteMgr, const char*>(*this, 
    &FGRouteMgr::getDestinationICAO, &FGRouteMgr::setDestinationICAO));
  destination->tie("name", SGRawValueMethods<FGRouteMgr, const char*>(*this, 
    &FGRouteMgr::getDestinationName, NULL));
    
  destination->getChild("runway", 0, true);
  destination->getChild("eta", 0, true);
  destination->getChild("touchdown-time", 0, true);
  
  alternate = fgGetNode(RM "alternate", true);
  alternate->getChild("airport", 0, true);
  alternate->getChild("runway", 0, true);
  
  cruise = fgGetNode(RM "cruise", true);
  cruise->getChild("altitude-ft", 0, true);
  cruise->setDoubleValue("altitude-ft", 10000.0);
  cruise->getChild("flight-level", 0, true);
  cruise->getChild("speed-kts", 0, true);
  cruise->setDoubleValue("speed-kts", 160.0);
  
  totalDistance = fgGetNode(RM "total-distance", true);
  totalDistance->setDoubleValue(0.0);
  
  ete = fgGetNode(RM "ete", true);
  ete->setDoubleValue(0.0);
  
  elapsedFlightTime = fgGetNode(RM "flight-time", true);
  elapsedFlightTime->setDoubleValue(0.0);
  
  active = fgGetNode(RM "active", true);
  active->setBoolValue(false);
  
  airborne = fgGetNode(RM "airborne", true);
  airborne->setBoolValue(false);
    
  _edited = fgGetNode(RM "signals/edited", true);
  _finished = fgGetNode(RM "signals/finished", true);
  
  _currentWpt = fgGetNode(RM "current-wp", true);
  _currentWpt->tie(SGRawValueMethods<FGRouteMgr, int>
    (*this, &FGRouteMgr::currentWaypoint, &FGRouteMgr::jumpToIndex));
      
  // temporary distance / eta calculations, for backward-compatability
  wp0 = fgGetNode(RM "wp", 0, true);
  wp0->getChild("id", 0, true);
  wp0->getChild("dist", 0, true);
  wp0->getChild("eta", 0, true);
  wp0->getChild("bearing-deg", 0, true);
  
  wp1 = fgGetNode(RM "wp", 1, true);
  wp1->getChild("id", 0, true);
  wp1->getChild("dist", 0, true);
  wp1->getChild("eta", 0, true);
  
  wpn = fgGetNode(RM "wp-last", 0, true);
  wpn->getChild("dist", 0, true);
  wpn->getChild("eta", 0, true);
  
  _route->clear();
  _route->set_current(0);
  update_mirror();
  
  _pathNode = fgGetNode(RM "file-path", 0, true);
}


void FGRouteMgr::postinit() {
    string_list *waypoints = globals->get_initial_waypoints();
    if (waypoints) {
      vector<string>::iterator it;
      for (it = waypoints->begin(); it != waypoints->end(); ++it)
        new_waypoint(*it);
    }

    weightOnWheels = fgGetNode("/gear/gear[0]/wow", false);
    // check airbone flag agrees with presets
    
}


void FGRouteMgr::bind() { }
void FGRouteMgr::unbind() { }

bool FGRouteMgr::isRouteActive() const
{
  return active->getBoolValue();
}

void FGRouteMgr::update( double dt ) {
    if (dt <= 0.0) {
      // paused, nothing to do here
      return;
    }
  
    if (!active->getBoolValue()) {
      return;
    }
    
    double groundSpeed = get_ground_speed();
    if (airborne->getBoolValue()) {
      time_t now = time(NULL);
      elapsedFlightTime->setDoubleValue(difftime(now, _takeoffTime));
    } else { // not airborne
      if (weightOnWheels->getBoolValue() || (groundSpeed < 40)) {
        return;
      }
      
      airborne->setBoolValue(true);
      _takeoffTime = time(NULL); // start the clock
      departure->setIntValue("takeoff-time", _takeoffTime);
    }
    
  // basic course/distance information
    double wp_course, wp_distance;
    SGWayPoint wp = _route->get_current();
    wp.CourseAndDistance( lon->getDoubleValue(), lat->getDoubleValue(),
                          alt->getDoubleValue(), &wp_course, &wp_distance );

  // update wp0 / wp1 / wp-last for legacy users
    wp0->setDoubleValue("dist", wp_distance * SG_METER_TO_NM);
    wp_course -= magvar->getDoubleValue(); // expose magnetic bearing
    wp0->setDoubleValue("bearing-deg", wp_course);
    setETAPropertyFromDistance(wp0->getChild("eta"), wp_distance);
    
    if ((_route->current_index() + 1) < _route->size()) {
      wp = _route->get_waypoint(_route->current_index() + 1);
      double wp1_course, wp1_distance;
      wp.CourseAndDistance(lon->getDoubleValue(), lat->getDoubleValue(),
                          alt->getDoubleValue(), &wp1_course, &wp1_distance);
    
      wp1->setDoubleValue("dist", wp1_distance * SG_METER_TO_NM);
      setETAPropertyFromDistance(wp1->getChild("eta"), wp1_distance);
    }
    
    double totalDistanceRemaining = wp_distance; // distance to current waypoint
    for (int i=_route->current_index() + 1; i<_route->size(); ++i) {
      totalDistanceRemaining += _route->get_waypoint(i).get_distance();
    }
    
    wpn->setDoubleValue("dist", totalDistanceRemaining * SG_METER_TO_NM);
    ete->setDoubleValue(totalDistanceRemaining * SG_METER_TO_NM / groundSpeed * 3600.0);
    setETAPropertyFromDistance(wpn->getChild("eta"), totalDistanceRemaining);
    
    // get time now at destination tz as tm struct
    // add ete seconds
    // convert to string ... and stash in property
    //destination->setDoubleValue("eta", eta);
}


void FGRouteMgr::setETAPropertyFromDistance(SGPropertyNode_ptr aProp, double aDistance) {
    double speed = get_ground_speed();
    if (speed < 1.0) {
      aProp->setStringValue("--:--");
      return;
    }
  
    char eta_str[64];
    double eta = aDistance * SG_METER_TO_NM / get_ground_speed();
    if ( eta >= 100.0 ) { 
        eta = 99.999; // clamp
    }
    
    if ( eta < (1.0/6.0) ) {
      eta *= 60.0; // within 10 minutes, bump up to min/secs
    }
    
    int major = (int)eta, 
        minor = (int)((eta - (int)eta) * 60.0);
    snprintf( eta_str, 64, "%d:%02d", major, minor );
    aProp->setStringValue( eta_str );
}

void FGRouteMgr::add_waypoint( const SGWayPoint& wp, int n )
{
  _route->add_waypoint( wp, n );
    
  if ((n >= 0) && (_route->current_index() > n)) {
    _route->set_current(_route->current_index() + 1);
  }
  
  waypointsChanged();
}

void FGRouteMgr::waypointsChanged()
{
  double routeDistanceNm = _route->total_distance() * SG_METER_TO_NM;
  totalDistance->setDoubleValue(routeDistanceNm);
  double cruiseSpeedKts = cruise->getDoubleValue("speed", 0.0);
  if (cruiseSpeedKts > 1.0) {
    // very very crude approximation, doesn't allow for climb / descent
    // performance or anything else at all
    ete->setDoubleValue(routeDistanceNm / cruiseSpeedKts * (60.0 * 60.0));
  }

  update_mirror();
  _edited->fireValueChanged();
  checkFinished();
}

SGWayPoint FGRouteMgr::pop_waypoint( int n ) {
  if ( _route->size() <= 0 ) {
    return SGWayPoint();
  }
  
  if ( n < 0 ) {
    n = _route->size() - 1;
  }
  
  if (_route->current_index() > n) {
    _route->set_current(_route->current_index() - 1);
  }

  SGWayPoint wp = _route->get_waypoint(n);
  _route->delete_waypoint(n);
    
  waypointsChanged();
  return wp;
}


bool FGRouteMgr::build() {
    return true;
}


void FGRouteMgr::new_waypoint( const string& target, int n ) {
    SGWayPoint* wp = make_waypoint( target );
    if (!wp) {
        return;
    }
    
    add_waypoint( *wp, n );
    delete wp;
}


SGWayPoint* FGRouteMgr::make_waypoint(const string& tgt ) {
    string target(boost::to_upper_copy(tgt));
    
    
    double alt = -9999.0;
    // extract altitude
    size_t pos = target.find( '@' );
    if ( pos != string::npos ) {
        alt = atof( target.c_str() + pos + 1 );
        target = target.substr( 0, pos );
        if ( !strcmp(fgGetString("/sim/startup/units"), "feet") )
            alt *= SG_FEET_TO_METER;
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
        return new SGWayPoint( lon, lat, alt, SGWayPoint::WGS84, buf);
    }    

    SGGeod basePosition;
    if (_route->size() > 0) {
        SGWayPoint wp = get_waypoint(_route->size()-1);
        basePosition = wp.get_target();
    } else {
        // route is empty, use current position
        basePosition = SGGeod::fromDeg(
            fgGetNode("/position/longitude-deg")->getDoubleValue(), 
            fgGetNode("/position/latitude-deg")->getDoubleValue());
    }
    
    vector<string> pieces(simgear::strutils::split(target, "/"));


    FGPositionedRef p = FGPositioned::findClosestWithIdent(pieces.front(), basePosition);
    if (!p) {
      SG_LOG( SG_AUTOPILOT, SG_INFO, "Unable to find FGPositioned with ident:" << pieces.front());
      return NULL;
    }
    
    SGGeod geod = SGGeod::fromGeodM(p->geod(), alt);
    if (pieces.size() == 1) {
      // simple case
      return new SGWayPoint(geod, target, p->name());
    }
        
    if (pieces.size() == 3) {
      // navaid/radial/distance-nm notation
      double radial = atof(pieces[1].c_str()),
        distanceNm = atof(pieces[2].c_str()),
        az2;
      radial += magvar->getDoubleValue(); // convert to true bearing
      SGGeod offsetPos;
      SGGeodesy::direct(geod, radial, distanceNm * SG_NM_TO_METER, offsetPos, az2);
      offsetPos.setElevationM(alt);
      
      SG_LOG(SG_AUTOPILOT, SG_INFO, "final offset radial is " << radial);
      return new SGWayPoint(offsetPos, p->ident() + pieces[2], target);
    }
    
    if (pieces.size() == 2) {
      FGAirport* apt = dynamic_cast<FGAirport*>(p.ptr());
      if (!apt) {
        SG_LOG(SG_AUTOPILOT, SG_INFO, "Waypoint is not an airport:" << pieces.front());
        return NULL;
      }
      
      if (!apt->hasRunwayWithIdent(pieces[1])) {
        SG_LOG(SG_AUTOPILOT, SG_INFO, "No runway: " << pieces[1] << " at " << pieces[0]);
        return NULL;
      }
      
      FGRunway* runway = apt->getRunwayByIdent(pieces[1]);
      SGGeod t = runway->threshold();
      return new SGWayPoint(t.getLongitudeDeg(), t.getLatitudeDeg(), alt, SGWayPoint::WGS84, pieces[1]);
    }
    
    SG_LOG(SG_AUTOPILOT, SG_INFO, "Unable to parse waypoint:" << target);
    return NULL;
}


// mirror internal route to the property system for inspection by other subsystems
void FGRouteMgr::update_mirror() {
    mirror->removeChildren("wp");
    for (int i = 0; i < _route->size(); i++) {
        SGWayPoint wp = _route->get_waypoint(i);
        SGPropertyNode *prop = mirror->getChild("wp", i, 1);

        const SGGeod& pos(wp.get_target());
        prop->setStringValue("id", wp.get_id().c_str());
        prop->setStringValue("name", wp.get_name().c_str());
        prop->setDoubleValue("longitude-deg", pos.getLongitudeDeg());
        prop->setDoubleValue("latitude-deg",pos.getLatitudeDeg());
        prop->setDoubleValue("altitude-m", pos.getElevationM());
        prop->setDoubleValue("altitude-ft", pos.getElevationFt());
    }
    // set number as listener attachment point
    mirror->setIntValue("num", _route->size());
}

// command interface /autopilot/route-manager/input:
//
//   @CLEAR             ... clear route
//   @POP               ... remove first entry
//   @DELETE3           ... delete 4th entry
//   @INSERT2:KSFO@900  ... insert "KSFO@900" as 3rd entry
//   KSFO@900           ... append "KSFO@900"
//
void FGRouteMgr::InputListener::valueChanged(SGPropertyNode *prop)
{
    const char *s = prop->getStringValue();
    if (strlen(s) == 0) {
      return;
    }
    
    if (!strcmp(s, "@CLEAR"))
        mgr->init();
    else if (!strcmp(s, "@ACTIVATE"))
        mgr->activate();
    else if (!strcmp(s, "@LOAD")) {
      mgr->loadRoute();
    } else if (!strcmp(s, "@SAVE")) {
      mgr->saveRoute();
    } else if (!strcmp(s, "@POP")) {
      SG_LOG(SG_AUTOPILOT, SG_WARN, "route-manager @POP command is deprecated");
    } else if (!strcmp(s, "@NEXT")) {
      mgr->jumpToIndex(mgr->currentWaypoint() + 1);
    } else if (!strcmp(s, "@PREVIOUS")) {
      mgr->jumpToIndex(mgr->currentWaypoint() - 1);
    } else if (!strncmp(s, "@JUMP", 5)) {
      mgr->jumpToIndex(atoi(s + 5));
    } else if (!strncmp(s, "@DELETE", 7))
        mgr->pop_waypoint(atoi(s + 7));
    else if (!strncmp(s, "@INSERT", 7)) {
        char *r;
        int pos = strtol(s + 7, &r, 10);
        if (*r++ != ':')
            return;
        while (isspace(*r))
            r++;
        if (*r)
            mgr->new_waypoint(r, pos);
    } else
        mgr->new_waypoint(s);
}

//    SGWayPoint( const double lon = 0.0, const double lat = 0.0,
//		const double alt = 0.0, const modetype m = WGS84,
//		const string& s = "", const string& n = "" );

bool FGRouteMgr::activate()
{
  if (isRouteActive()) {
    SG_LOG(SG_AUTOPILOT, SG_WARN, "duplicate route-activation, no-op");
    return false;
  }

  // only add departure waypoint if we're not airborne, so that
  // in-air route activation doesn't confuse matters.
  if (weightOnWheels->getBoolValue() && _departure) {
    string runwayId(departure->getStringValue("runway"));
    FGRunway* runway = NULL;
    if (_departure->hasRunwayWithIdent(runwayId)) {
      runway = _departure->getRunwayByIdent(runwayId);
    } else {
      SG_LOG(SG_AUTOPILOT, SG_INFO, 
        "route-manager, departure runway not found:" << runwayId);
      runway = _departure->getActiveRunwayForUsage();
    }
    
    SGWayPoint swp(runway->threshold(), 
      _departure->ident() + "-" + runway->ident(), runway->name());
    add_waypoint(swp, 0);
  }
  
  if (_destination) {
    string runwayId = (destination->getStringValue("runway"));
    if (_destination->hasRunwayWithIdent(runwayId)) {
      FGRunway* runway = _destination->getRunwayByIdent(runwayId);
      SGWayPoint swp(runway->end(), 
        _destination->ident() + "-" + runway->ident(), runway->name());
      add_waypoint(swp);
    } else {
      // quite likely, since destination runway may not be known until enroute
      // probably want a listener on the 'destination' node to allow an enroute
      // update
      add_waypoint(SGWayPoint(_destination->geod(), _destination->ident(), _destination->name()));
    }
  }

  _route->set_current(0);
  active->setBoolValue(true);
  SG_LOG(SG_AUTOPILOT, SG_INFO, "route-manager, activate route ok");
  return true;
}


void FGRouteMgr::sequence()
{
  if (!active->getBoolValue()) {
    SG_LOG(SG_AUTOPILOT, SG_ALERT, "trying to sequence waypoints with no active route");
    return;
  }
  
  if (checkFinished()) {
    return;
  }
  
  _route->increment_current();
  currentWaypointChanged();
  _currentWpt->fireValueChanged();
}

bool FGRouteMgr::checkFinished()
{
  int lastWayptIndex = _route->size() - 1;
  if (_route->current_index() < lastWayptIndex) {
    return false;
  }
  
  SG_LOG(SG_AUTOPILOT, SG_INFO, "reached end of active route");
  _finished->fireValueChanged();
  active->setBoolValue(false);
  return true;
}

void FGRouteMgr::jumpToIndex(int index)
{
  if ((index < 0) || (index >= _route->size())) {
    SG_LOG(SG_AUTOPILOT, SG_ALERT, "passed invalid index (" << 
      index << ") to FGRouteMgr::jumpToIndex");
    return;
  }

  if (_route->current_index() == index) {
    return; // no-op
  }
  
  _route->set_current(index);
  currentWaypointChanged();
  _currentWpt->fireValueChanged();
}

void FGRouteMgr::currentWaypointChanged()
{
  SGWayPoint previous = _route->get_previous();
  SGWayPoint cur = _route->get_current();
  
  wp0->getChild("id")->setStringValue(cur.get_id());
  if ((_route->current_index() + 1) < _route->size()) {
    wp1->getChild("id")->setStringValue(_route->get_next().get_id());
  } else {
    wp1->getChild("id")->setStringValue("");
  }
  
  SG_LOG(SG_AUTOPILOT, SG_INFO, "route manager, current-wp is now " << _route->current_index());
}

int FGRouteMgr::findWaypoint(const SGGeod& aPos) const
{  
  for (int i=0; i<_route->size(); ++i) {
    double d = SGGeodesy::distanceM(aPos, _route->get_waypoint(i).get_target());
    if (d < 200.0) { // 200 metres seems close enough
      return i;
    }
  }
  
  return -1;
}

SGWayPoint FGRouteMgr::get_waypoint( int i ) const
{
  return _route->get_waypoint(i);
}

int FGRouteMgr::size() const
{
  return _route->size();
}

int FGRouteMgr::currentWaypoint() const
{
  return _route->current_index();
}

void FGRouteMgr::setWaypointTargetAltitudeFt(unsigned int index, int altFt)
{
  SGWayPoint wp = _route->get_waypoint(index);
  wp.setTargetAltFt(altFt);
  // simplest way to update a waypoint is to remove and re-add it
  _route->delete_waypoint(index);
  _route->add_waypoint(wp, index);
  waypointsChanged();
}

void FGRouteMgr::saveRoute()
{
  SGPath path(_pathNode->getStringValue());
  SG_LOG(SG_IO, SG_INFO, "Saving route to " << path.str());
  try {
    writeProperties(path.str(), mirror, false, SGPropertyNode::ARCHIVE);
  } catch (const sg_exception &e) {
    SG_LOG(SG_IO, SG_WARN, "Error saving route:" << e.getMessage());
    //guiErrorMessage("Error writing autosave.xml: ", e);
  }
}

void FGRouteMgr::loadRoute()
{
  // deactivate route first
  active->setBoolValue(false);
  
  SGPropertyNode_ptr routeData(new SGPropertyNode);
  SGPath path(_pathNode->getStringValue());
  
  SG_LOG(SG_IO, SG_INFO, "going to read flight-plan from:" << path.str());
    
  try {
    readProperties(path.str(), routeData);
  } catch (sg_exception& e) {
    // if XML parsing fails, the file might be simple textual list of waypoints
    loadPlainTextRoute(path);
    return;
  }
  
  try {
  // departure nodes
    SGPropertyNode* dep = routeData->getChild("departure");
    if (!dep) {
      throw sg_io_exception("malformed route file, no departure node");
    }
    
    string depIdent = dep->getStringValue("airport");
    _departure = (FGAirport*) fgFindAirportID(depIdent);

        
  // destination
    SGPropertyNode* dst = routeData->getChild("destination");
    if (!dst) {
      throw sg_io_exception("malformed route file, no destination node");
    }
    
    _destination = (FGAirport*) fgFindAirportID(dst->getStringValue("airport"));
    destination->setStringValue("runway", dst->getStringValue("runway"));

  // alternate
    SGPropertyNode* alt = routeData->getChild("alternate");
    if (alt) {
      alternate->setStringValue(alt->getStringValue("airport"));
    } // of cruise data loading
    
  // cruise
    SGPropertyNode* crs = routeData->getChild("cruise");
    if (crs) {
      cruise->setDoubleValue("speed-kts", crs->getDoubleValue("speed-kts"));
      cruise->setDoubleValue("mach", crs->getDoubleValue("mach"));
      cruise->setDoubleValue("altitude-ft", crs->getDoubleValue("altitude-ft"));
    } // of cruise data loading

  // route nodes
    _route->clear();
    SGPropertyNode_ptr _route = routeData->getChild("route", 0);
    SGGeod lastPos = (_departure ? _departure->geod() : SGGeod());
    
    for (int i=0; i<_route->nChildren(); ++i) {
      SGPropertyNode_ptr wp = _route->getChild("wp", i);
      parseRouteWaypoint(wp);
    } // of route iteration
  } catch (sg_exception& e) {
    SG_LOG(SG_IO, SG_WARN, "failed to load flight-plan (from '" << e.getOrigin()
      << "'):" << e.getMessage());
  }
}

void FGRouteMgr::parseRouteWaypoint(SGPropertyNode* aWP)
{
  SGGeod lastPos;
  if (_route->size() > 0) {
    lastPos = get_waypoint(_route->size()-1).get_target();
  } else {
    // route is empty, use departure airport position
    const FGAirport* apt = fgFindAirportID(departure->getStringValue("airport"));
    assert(apt); // shouldn't have got this far with an invalid airport
    lastPos = apt->geod();
  }

  SGPropertyNode_ptr altProp = aWP->getChild("altitude-ft");
  double altM = -9999.0;
  if (altProp) {
    altM = altProp->getDoubleValue() * SG_FEET_TO_METER;
  }
      
  string ident(aWP->getStringValue("ident"));
  if (aWP->hasChild("longitude-deg")) {
    // explicit longitude/latitude
    SGWayPoint swp(aWP->getDoubleValue("longitude-deg"),
      aWP->getDoubleValue("latitude-deg"), altM, 
      SGWayPoint::WGS84, ident, aWP->getStringValue("name"));
    add_waypoint(swp);
  } else if (aWP->hasChild("navid")) {
    // lookup by navid (possibly with offset)
    string nid(aWP->getStringValue("navid"));
    FGPositionedRef p = FGPositioned::findClosestWithIdent(nid, lastPos);
    if (!p) {
      throw sg_io_exception("bad route file, unknown navid:" + nid);
    }
    
    SGGeod pos(p->geod());
    if (aWP->hasChild("offset-nm") && aWP->hasChild("offset-radial")) {
      double radialDeg = aWP->getDoubleValue("offset-radial");
      // convert magnetic radial to a true radial!
      radialDeg += magvar->getDoubleValue();
      double offsetNm = aWP->getDoubleValue("offset-nm");
      double az2;
      SGGeodesy::direct(p->geod(), radialDeg, offsetNm * SG_NM_TO_METER, pos, az2);
    }
    
    SGWayPoint swp(pos.getLongitudeDeg(), pos.getLatitudeDeg(), altM, 
      SGWayPoint::WGS84, ident, "");
    add_waypoint(swp);
  } else {
    // lookup by ident (symbolic waypoint)
    FGPositionedRef p = FGPositioned::findClosestWithIdent(ident, lastPos);
    if (!p) {
      throw sg_io_exception("bad route file, unknown waypoint:" + ident);
    }
    
    SGWayPoint swp(p->longitude(), p->latitude(), altM, 
      SGWayPoint::WGS84, p->ident(), p->name());
    add_waypoint(swp);
  }
}

void FGRouteMgr::loadPlainTextRoute(const SGPath& path)
{
  sg_gzifstream in(path.str().c_str());
  if (!in.is_open()) {
    return;
  }
  
  _route->clear();
  while (!in.eof()) {
    string line;
    getline(in, line, '\n');
  // trim CR from end of line, if found
    if (line[line.size() - 1] == '\r') {
      line.erase(line.size() - 1, 1);
    }
    
    new_waypoint(line, -1);
  } // of line iteration
}

const char* FGRouteMgr::getDepartureICAO() const
{
  if (!_departure) {
    return "";
  }
  
  return _departure->ident().c_str();
}

const char* FGRouteMgr::getDepartureName() const
{
  if (!_departure) {
    return "";
  }
  
  return _departure->name().c_str();
}

void FGRouteMgr::setDepartureICAO(const char* aIdent)
{
  if ((aIdent == NULL) || (strlen(aIdent) < 4)) {
    _departure = NULL;
  } else {
    _departure = FGAirport::findByIdent(aIdent);
  }
}

const char* FGRouteMgr::getDestinationICAO() const
{
  if (!_destination) {
    return "";
  }
  
  return _destination->ident().c_str();
}

const char* FGRouteMgr::getDestinationName() const
{
  if (!_destination) {
    return "";
  }
  
  return _destination->name().c_str();
}

void FGRouteMgr::setDestinationICAO(const char* aIdent)
{
  if ((aIdent == NULL) || (strlen(aIdent) < 4)) {
    _destination = NULL;
  } else {
    _destination = FGAirport::findByIdent(aIdent);
  }
}
    
