// FGAIFlightPlan - class for loading and storing  AI flight plans
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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


#include "AIFlightPlan.hxx"
#include <simgear/misc/sg_path.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/constants.h>
#ifdef __BORLANDC__
#  define exception c_exception
#endif
#include <simgear/props/props.hxx>
#include <Main/globals.hxx>
#include <Main/fg_props.hxx>


FGAIFlightPlan::FGAIFlightPlan(string filename)
{
  int i;
  SGPath path( globals->get_fg_root() );
  path.append( ("/Data/AI/FlightPlans/" + filename).c_str() );
  SGPropertyNode root;

  try {
      readProperties(path.str(), &root);
  } catch (const sg_exception &e) {
      SG_LOG(SG_GENERAL, SG_ALERT,
       "Error reading AI flight plan: ");
       cout << path.str() << endl;
      return;
  }

  SGPropertyNode * node = root.getNode("flightplan");
  for (i = 0; i < node->nChildren(); i++) { 
     //cout << "Reading waypoint " << i << endl;        
     waypoint* wpt = new waypoint;
     waypoints.push_back( wpt );
     SGPropertyNode * wpt_node = node->getChild(i);
     wpt->name      = wpt_node->getStringValue("name", "END");
     wpt->latitude  = wpt_node->getDoubleValue("lat", 0);
     wpt->longitude = wpt_node->getDoubleValue("lon", 0);
     wpt->altitude  = wpt_node->getDoubleValue("alt", 0);
     wpt->speed     = wpt_node->getDoubleValue("ktas", 0);
     wpt->crossat   = wpt_node->getDoubleValue("crossat", -10000);
     wpt->gear_down = wpt_node->getBoolValue("gear-down", false);
     wpt->flaps_down= wpt_node->getBoolValue("flaps-down", false);
   }

  wpt_iterator = waypoints.begin();
  //cout << waypoints.size() << " waypoints read." << endl;
}


FGAIFlightPlan::~FGAIFlightPlan()
{
  waypoints.clear();
}


FGAIFlightPlan::waypoint*
FGAIFlightPlan::getPreviousWaypoint( void )
{
  if (wpt_iterator == waypoints.begin()) {
    return 0;
  } else {
    wpt_vector_iterator prev = wpt_iterator;
    return *(--prev);
  }
}

FGAIFlightPlan::waypoint*
FGAIFlightPlan::getCurrentWaypoint( void )
{
  return *wpt_iterator;
}

FGAIFlightPlan::waypoint*
FGAIFlightPlan::getNextWaypoint( void )
{
  if (wpt_iterator == waypoints.end()) {
    return 0;
  } else {
    wpt_vector_iterator next = wpt_iterator;
    return *(++next);
  }
}

void FGAIFlightPlan::IncrementWaypoint( void )
{
  wpt_iterator++;
}

// gives distance in feet from a position to a waypoint
double FGAIFlightPlan::getDistanceToGo(double lat, double lon, waypoint* wp){
   // get size of a degree at the present latitude
   // this won't work over large distances
   double ft_per_deg_lat = 366468.96 - 3717.12 * cos(lat / SG_RADIANS_TO_DEGREES);
   double ft_per_deg_lon = 365228.16 * cos(lat / SG_RADIANS_TO_DEGREES);
   double lat_diff_ft = fabs(wp->latitude - lat) * ft_per_deg_lat;
   double lon_diff_ft = fabs(wp->longitude - lon) * ft_per_deg_lon;
   return sqrt((lat_diff_ft * lat_diff_ft) + (lon_diff_ft * lon_diff_ft));
}

// sets distance in feet from a lead point to the current waypoint
void FGAIFlightPlan::setLeadDistance(double speed, double bearing, 
                                     waypoint* current, waypoint* next){
  double turn_radius = 0.1911 * speed * speed; // an estimate for 25 degrees bank
  double inbound = bearing;
  double outbound = getBearing(current, next);
  double diff = fabs(inbound - outbound);
  if (diff > 180.0) diff = 360.0 - diff;
  lead_distance = turn_radius * sin(diff * SG_DEGREES_TO_RADIANS); 
}

void FGAIFlightPlan::setLeadDistance(double distance_ft){
  lead_distance = distance_ft;
}


double FGAIFlightPlan::getBearing(waypoint* first, waypoint* second){
  return getBearing(first->latitude, first->longitude, second);
}


double FGAIFlightPlan::getBearing(double lat, double lon, waypoint* wp){
  double latd = lat;
  double lond = lon;
  double latt = wp->latitude;
  double lont = wp->longitude;
  double ft_per_deg_lat = 366468.96 - 3717.12 * cos(lat/SG_RADIANS_TO_DEGREES);
  double ft_per_deg_lon = 365228.16 * cos(lat/SG_RADIANS_TO_DEGREES);

  if (lond < 0.0) lond+=360.0;
  if (lont < 0.0) lont+=360.0;
  latd+=90.0;
  latt+=90.0;

  double lat_diff = (latt - latd) * ft_per_deg_lat;
  double lon_diff = (lont - lond) * ft_per_deg_lon;
  double angle = atan(fabs(lat_diff / lon_diff)) * SG_RADIANS_TO_DEGREES;

  bool southerly = true;
  if (latt > latd) southerly = false;
  bool easterly = false;
  if (lont > lond) easterly = true;
  if (southerly && easterly) return 90.0 + angle;
  if (!southerly && easterly) return 90.0 - angle;
  if (southerly && !easterly) return 270.0 - angle;
  if (!southerly && !easterly) return 270.0 + angle; 
}


