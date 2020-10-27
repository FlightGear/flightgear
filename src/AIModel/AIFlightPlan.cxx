// // FGAIFlightPlan - class for loading and storing  AI flight plans
// Written by David Culp, started May 2004
// - davidculp2@comcast.net
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
#  include <config.h>
#endif

#include <simgear/misc/sg_path.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/constants.h>
#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/timing/sg_time.hxx>

#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <Main/fg_init.hxx>
#include <Airports/airport.hxx>
#include <Airports/dynamics.hxx>
#include <Airports/runways.hxx>
#include <Airports/groundnetwork.hxx>

#include <Environment/environment_mgr.hxx>
#include <Environment/environment.hxx>

#include <Traffic/Schedule.hxx>

#include "AIFlightPlan.hxx"
#include "AIAircraft.hxx"

using std::string;

FGAIWaypoint::FGAIWaypoint() {
  speed       = 0;
  crossat     = 0;
  finished    = 0;
  gear_down   = 0;
  flaps_down  = 0;
  on_ground   = 0;
  routeIndex  = 0;
  time_sec    = 0;
  trackLength = 0;
}

bool FGAIWaypoint::contains(const string& target) {
    size_t found = name.find(target);
    if (found == string::npos)
        return false;
    else
        return true;
}

double FGAIWaypoint::getLatitude()
{
  return pos.getLatitudeDeg();
}

double FGAIWaypoint::getLongitude()
{
  return pos.getLongitudeDeg();
}

double FGAIWaypoint::getAltitude()
{
  return pos.getElevationFt();
}

void FGAIWaypoint::setLatitude(double lat)
{
  pos.setLatitudeDeg(lat);
}

void FGAIWaypoint::setLongitude(double lon)
{
  pos.setLongitudeDeg(lon);
}

void FGAIWaypoint::setAltitude(double alt)
{
  pos.setElevationFt(alt);
}

FGAIFlightPlan::FGAIFlightPlan() :
    sid(NULL),
    repeat(false),
    distance_to_go(0),
    lead_distance(0),
    leadInAngle(0),
    start_time(0),
    arrivalTime(0),
    leg(0),
    lastNodeVisited(0),
    isValid(true)
{
    wpt_iterator    = waypoints.begin();
}

FGAIFlightPlan::FGAIFlightPlan(const string& filename) :
    sid(NULL),
    repeat(false),
    distance_to_go(0),
    lead_distance(0),
    leadInAngle(0),
    start_time(0),
    arrivalTime(0),
    leg(10),
    lastNodeVisited(0),
    isValid(parseProperties(filename))
{
}


// This is a modified version of the constructor,
// Which not only reads the waypoints from a 
// Flight plan file, but also adds the current
// Position computed by the traffic manager, as well
// as setting speeds and altitude computed by the
// traffic manager. 
FGAIFlightPlan::FGAIFlightPlan(FGAIAircraft *ac,
                               const std::string& p,
                               double course,
                               time_t start,
                               FGAirport *dep,
                               FGAirport *arr,
                               bool firstLeg,
                               double radius,
                               double alt,
                               double lat,
                               double lon,
                               double speed,
                               const string& fltType,
                               const string& acType,
                               const string& airline) :
    sid(NULL),
    repeat(false),
    distance_to_go(0),
    lead_distance(0),
    leadInAngle(0),
    start_time(start),
    arrivalTime(0),
    leg(10),
    lastNodeVisited(0),
    isValid(false),
    departure(dep),
    arrival(arr)
{
  if (parseProperties(p)) {
    isValid = true;
  } else {
    createWaypoints(ac, course, start, dep, arr, firstLeg, radius,
                    alt, lat, lon, speed, fltType, acType, airline);
  }
}

FGAIFlightPlan::~FGAIFlightPlan()
{
  deleteWaypoints();
  //delete taxiRoute;
}

void FGAIFlightPlan::createWaypoints(FGAIAircraft *ac,
                                     double course,
                                     time_t start,
                                     FGAirport *dep,
                                     FGAirport *arr,
                                     bool firstLeg,
                                     double radius,
                                     double alt,
                                     double lat,
                                     double lon,
                                     double speed,
                                     const string& fltType,
                                     const string& acType,
                                     const string& airline)
{
  time_t now = globals->get_time_params()->get_cur_time();
  time_t timeDiff = now-start;
  leg = 1;
  
  if ((timeDiff > 60) && (timeDiff < 1500))
    leg = 2;
  //else if ((timeDiff >= 1200) && (timeDiff < 1500)) {
    //leg = 3;
  //ac->setTakeOffStatus(2);
  //}
  else if ((timeDiff >= 1500) && (timeDiff < 2000))
    leg = 4;
  else if (timeDiff >= 2000)
    leg = 5;
  /*
   if (timeDiff >= 2000)
   leg = 5;
   */

    SG_LOG(SG_AI, SG_BULK, "Route from " << dep->getId() << " to " << arr->getId() <<
           ". Set leg to : " << leg << " " << ac->getTrafficRef()->getCallSign());
    
  wpt_iterator = waypoints.begin();
  bool dist = 0;
  isValid = create(ac, dep, arr, leg, alt, speed, lat, lon,
                   firstLeg, radius, fltType, acType, airline, dist);
  wpt_iterator = waypoints.begin();
}

bool FGAIFlightPlan::parseProperties(const std::string& filename)
{
  SGPath path( globals->get_fg_root() );
  path.append( "/AI/FlightPlans/" + filename );
  if (!path.exists()) {
    return false;
  }
  
  SGPropertyNode root;
  try {
    readProperties(path, &root);
  } catch (const sg_exception &e) {
    SG_LOG(SG_AI, SG_ALERT, "Error reading AI flight plan: " << path
           << "message:" << e.getFormattedMessage());
    return false;
  }
  
  SGPropertyNode * node = root.getNode("flightplan");
  for (int i = 0; i < node->nChildren(); i++) {
    FGAIWaypoint* wpt = new FGAIWaypoint;
    SGPropertyNode * wpt_node = node->getChild(i);
    wpt->setName       (wpt_node->getStringValue("name", "END"     ));
    wpt->setLatitude   (wpt_node->getDoubleValue("lat", 0          ));
    wpt->setLongitude  (wpt_node->getDoubleValue("lon", 0          ));
    wpt->setAltitude   (wpt_node->getDoubleValue("alt", 0          ));
    wpt->setSpeed      (wpt_node->getDoubleValue("ktas", 0         ));
    wpt->setCrossat    (wpt_node->getDoubleValue("crossat", -10000 ));
    wpt->setGear_down  (wpt_node->getBoolValue("gear-down", false  ));
    wpt->setFlaps_down (wpt_node->getBoolValue("flaps-down", false ));
    wpt->setOn_ground  (wpt_node->getBoolValue("on-ground", false  ));
    wpt->setTime_sec   (wpt_node->getDoubleValue("time-sec", 0     ));
    wpt->setTime       (wpt_node->getStringValue("time", ""        ));
    wpt->setFinished   ((wpt->getName() == "END"));
    pushBackWaypoint( wpt );
  }
  if( getLastWaypoint()->getName().compare("END") != 0  ) {
    SG_LOG(SG_AI, SG_ALERT, "FGAIFlightPlan::Flightplan missing END node" );
    return false;
  }

  
  wpt_iterator = waypoints.begin();
  return true;
}

FGAIWaypoint* FGAIFlightPlan::getPreviousWaypoint( void ) const
{
    if (empty())
        return nullptr;

    if (wpt_iterator == waypoints.begin()) {
        return nullptr;
    } else {
        wpt_vector_iterator prev = wpt_iterator;
        return *(--prev);
    }
}

FGAIWaypoint* FGAIFlightPlan::getCurrentWaypoint( void ) const
{
  if (wpt_iterator == waypoints.end())
      return 0;
  return *wpt_iterator;
}

FGAIWaypoint* FGAIFlightPlan::getNextWaypoint( void ) const
{
  wpt_vector_iterator i = waypoints.end();
  i--;  // end() points to one element after the last one. 
  if (wpt_iterator == i) {
    return 0;
  } else {
    wpt_vector_iterator next = wpt_iterator;
    return *(++next);
  }
}

void FGAIFlightPlan::IncrementWaypoint(bool eraseWaypoints )
{
    if (empty())
        return;

    if (eraseWaypoints)
    {
        if (wpt_iterator == waypoints.begin())
            wpt_iterator++;
        else
        if (!waypoints.empty())
        {
            delete *(waypoints.begin());
            waypoints.erase(waypoints.begin());
            wpt_iterator = waypoints.begin();
            wpt_iterator++;
        }
    }
    else
        wpt_iterator++;
}

void FGAIFlightPlan::DecrementWaypoint(bool eraseWaypoints )
{
    if (empty())
        return;

    if (eraseWaypoints)
    {
        if (wpt_iterator == waypoints.end())
            wpt_iterator--;
        else
        if (!waypoints.empty())
        {
            delete *(waypoints.end()-1);
            waypoints.erase(waypoints.end()-1);
            wpt_iterator = waypoints.end();
            wpt_iterator--;
        }
    }
    else
        wpt_iterator--;
}

void FGAIFlightPlan::eraseLastWaypoint()
{
    if (empty())
        return;

    delete (waypoints.back());
    waypoints.pop_back();;
    wpt_iterator = waypoints.begin();
    wpt_iterator++;
}

// gives distance in meters from a position to a waypoint
double FGAIFlightPlan::getDistanceToGo(double lat, double lon, FGAIWaypoint* wp) const{
  return SGGeodesy::distanceM(SGGeod::fromDeg(lon, lat), wp->getPos());
}

// sets distance in feet from a lead point to the current waypoint
void FGAIFlightPlan::setLeadDistance(double speed, double bearing, 
                                     FGAIWaypoint* current, FGAIWaypoint* next){
  double turn_radius;
  // Handle Ground steering
  // At a turn rate of 30 degrees per second, it takes 12 seconds to do a full 360 degree turn
  // So, to get an estimate of the turn radius, calculate the cicumference of the circle
  // we travel on. Get the turn radius by dividing by PI (*2).
  if (speed < 0.5) {
        lead_distance = 0.5;
        return;
  }
  if (speed < 25) {
       turn_radius = ((360/30)*fabs(speed)) / (2*M_PI);
  } else 
      turn_radius = 0.1911 * speed * speed; // an estimate for 25 degrees bank

  double inbound = bearing;
  double outbound = getBearing(current, next);
  leadInAngle = fabs(inbound - outbound);
  if (leadInAngle > 180.0) 
    leadInAngle = 360.0 - leadInAngle;
  //if (leadInAngle < 30.0) // To prevent lead_dist from getting so small it is skipped 
  //  leadInAngle = 30.0;
  
  //lead_distance = turn_radius * sin(leadInAngle * SG_DEGREES_TO_RADIANS); 
  lead_distance = turn_radius * tan((leadInAngle * SG_DEGREES_TO_RADIANS)/2);
  /*
  if ((lead_distance > (3*turn_radius)) && (current->on_ground == false)) {
      SG_LOG(SG_AI, SG_ALERT, "Warning: Lead-in distance is large. Inbound = " << inbound
            << ". Outbound = " << outbound << ". Lead in angle = " << leadInAngle  << ". Turn radius = " << turn_radius);
       lead_distance = 3 * turn_radius;
       return;
  }
  if ((leadInAngle > 90) && (current->on_ground == true)) {
      lead_distance = turn_radius * tan((90 * SG_DEGREES_TO_RADIANS)/2);
      return;
  }*/
}

void FGAIFlightPlan::setLeadDistance(double distance_ft){
  lead_distance = distance_ft;
}


double FGAIFlightPlan::getBearing(FGAIWaypoint* first, FGAIWaypoint* second) const
{
  return SGGeodesy::courseDeg(first->getPos(), second->getPos());
}

double FGAIFlightPlan::getBearing(const SGGeod& aPos, FGAIWaypoint* wp) const
{
  return SGGeodesy::courseDeg(aPos, wp->getPos());
}

void FGAIFlightPlan::deleteWaypoints()
{
  for (wpt_vector_iterator i = waypoints.begin(); i != waypoints.end();i++)
    delete (*i);
  waypoints.clear();
  wpt_iterator = waypoints.begin();
}

// Delete all waypoints except the last, 
// which we will recycle as the first waypoint in the next leg;
void FGAIFlightPlan::resetWaypoints()
{
  if (waypoints.begin() == waypoints.end())
    return;
  else
    {
      FGAIWaypoint *wpt = new FGAIWaypoint;
      wpt_vector_iterator i = waypoints.end();
      i--;
      wpt->setName        ( (*i)->getName()       );
      wpt->setPos         ( (*i)->getPos()        );
      wpt->setCrossat     ( (*i)->getCrossat()    );
      wpt->setGear_down   ( (*i)->getGear_down()  );
      wpt->setFlaps_down  ( (*i)->getFlaps_down() );
      wpt->setFinished    ( false                 );
      wpt->setOn_ground   ( (*i)->getOn_ground()  );
      SG_LOG(SG_AI, SG_DEBUG, "Recycling waypoint " << wpt->getName());
      deleteWaypoints();
      pushBackWaypoint(wpt);
    }
}

void FGAIFlightPlan::pushBackWaypoint(FGAIWaypoint *wpt)
{
  // std::vector::push_back invalidates waypoints
  //  so we should restore wpt_iterator after push_back
  //  (or it could be an index in the vector)
  size_t pos = wpt_iterator - waypoints.begin();
  waypoints.push_back(wpt);
  wpt_iterator = waypoints.begin() + pos;
}

// Start flightplan over from the beginning
void FGAIFlightPlan::restart()
{
  wpt_iterator = waypoints.begin();
}

int FGAIFlightPlan::getRouteIndex(int i) {
  if ((i > 0) && (i < (int)waypoints.size())) {
    return waypoints[i]->getRouteIndex();
  }
  else
    return 0;
}

double FGAIFlightPlan::checkTrackLength(const string& wptName) const {
    // skip the first two waypoints: first one is behind, second one is partially done;
    double trackDistance = 0;
    wpt_vector_iterator wptvec = waypoints.begin();
    wptvec++;
    wptvec++;
    while ((wptvec != waypoints.end()) && (!((*wptvec)->contains(wptName)))) {
           trackDistance += (*wptvec)->getTrackLength();
           wptvec++;
    }
    if (wptvec == waypoints.end()) {
        trackDistance = 0; // name not found
    }
    return trackDistance;
}

void FGAIFlightPlan::shortenToFirst(unsigned int number, string name)
{
    while (waypoints.size() > number + 3) {
        eraseLastWaypoint();
    }
    (waypoints.back())->setName((waypoints.back())->getName() + name);
}

void FGAIFlightPlan::setGate(const ParkingAssignment& pka)
{
  gate = pka;
}

FGParking* FGAIFlightPlan::getParkingGate()
{
  return gate.parking();
}

FGAirportRef FGAIFlightPlan::departureAirport() const
{
    return departure;
}

FGAirportRef FGAIFlightPlan::arrivalAirport() const
{
    return arrival;
}

FGAIFlightPlan* FGAIFlightPlan::createDummyUserPlan()
{
    FGAIFlightPlan* fp = new FGAIFlightPlan;
    fp->isValid = false;
    return fp;
}

bool FGAIFlightPlan::empty() const
{
    return waypoints.empty();
}
