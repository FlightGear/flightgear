/******************************************************************************
 * AIFlightPlanCreateCruise.cxx
 * Written by Durk Talsma, started February, 2006.
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *
 **************************************************************************/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <fstream>

#include <Airports/airport.hxx>
#include <Airports/runways.hxx>
#include <Airports/dynamics.hxx>

#include <Environment/environment_mgr.hxx>
#include <Environment/environment.hxx>

#include <Traffic/Schedule.hxx>

#include "AIFlightPlan.hxx"
#include "AIAircraft.hxx"
#include "performancedata.hxx"

using std::string;

/*
void FGAIFlightPlan::evaluateRoutePart(double deplat,
                       double deplon,
                       double arrlat,
                       double arrlon)
{
  // First do a prescan of all the waypoints that are within a reasonable distance of the
  // ideal route.
  intVec nodes;
  int tmpNode, prevNode;

  SGGeoc dep(SGGeoc::fromDegM(deplon, deplat, 100.0));
  SGGeoc arr(SGGeoc::fromDegM(arrlon, arrlat, 100.0));
  
  SGVec3d a = SGVec3d::fromGeoc(dep);
  SGVec3d nb = normalize(SGVec3d::fromGeoc(arr));
  SGVec3d na = normalize(a);
  
  SGVec3d _cross = cross(nb, na);

  double angle = acos(dot(na, nb));
  const double angleStep = 0.05 * SG_DEGREES_TO_RADIANS;
  tmpNode = 0;
  for (double ang = 0.0; ang < angle; ang += angleStep)
  {  
      SGQuatd q = SGQuatd::fromAngleAxis(ang, _cross);
      SGGeod geod = SGGeod::fromCart(q.transform(a));

      prevNode = tmpNode;
      tmpNode = globals->get_airwaynet()->findNearestNode(geod);

      FGNode* node = globals->get_airwaynet()->findNode(tmpNode);
    
      if ((tmpNode != prevNode) && (SGGeodesy::distanceM(geod, node->getPosition()) < 25000)) {
        nodes.push_back(tmpNode);
      }
    }

    intVecIterator i = nodes.begin();
    intVecIterator j = nodes.end();
    while (i != nodes.end())
    {
        j = nodes.end();
        while (j != i)
        {
            j--;
            FGAirRoute routePart = globals->get_airwaynet()->findShortestRoute(*i, *j);
            if (!(routePart.empty()))
            {
                airRoute.add(routePart);
                i = j;
                break;
            }
        }
        i++;
    }
}

*/
/*
void FGAIFlightPlan::createCruise(bool firstFlight, FGAirport *dep,
                  FGAirport *arr, double latitude,
                  double longitude, double speed, double alt)
{
  bool useInitialWayPoint = true;
  bool useCurrentWayPoint = false;
  double heading;
  double lat, lon, az;
   double lat2, lon2, az2;
   double azimuth;
  waypoint *wpt;
  waypoint *init_waypoint;
    intVec ids;
    char buffer[32];
    SGPath routefile = globals->get_fg_root();
    init_waypoint = new waypoint;
    init_waypoint->name      = "Initial waypoint";
    init_waypoint->latitude  = latitude;
    init_waypoint->longitude = longitude;;
    //wpt->altitude  = apt->getElevation(); // should maybe be tn->elev too
    init_waypoint->altitude  = alt;
    init_waypoint->speed     = 450; //speed;
    init_waypoint->crossat   = -10000;
    init_waypoint->gear_down = false;
    init_waypoint->flaps_down= false;
    init_waypoint->finished  = false;
    init_waypoint->on_ground = false;
    pushBackWaypoint(init_waypoint);
    routefile.append("Data/AI/FlightPlans");
    snprintf(buffer, 32, "%s-%s.txt",
         dep->getId().c_str(),
         arr->getId().c_str());
    routefile.append(buffer);
    SG_LOG(SG_AI, SG_DEBUG, "trying to read " << routefile.c_str());
    //exit(1);
    if (routefile.exists())
      {
     sg_gzifstream in( routefile.str() );
    do {
      in >> route;
    } while (!(in.eof()));
      }
    else {
    //int runwayId = apt->getDynamics()->getGroundNetwork()->findNearestNode(lat2, lon2);
    //int startId = globals->get_airwaynet()->findNearestNode(dep->getLatitude(), dep->getLongitude());
    //int endId   = globals->get_airwaynet()->findNearestNode(arr->getLatitude(), arr->getLongitude());
    //FGAirRoute route;
    evaluateRoutePart(dep->getLatitude(), dep->getLongitude(),
              arr->getLatitude(), arr->getLongitude());
    //exit(1);
    }
    route.first();
    int node;
    if (route.empty()) {
      // if no route could be found, create a direct gps route...
      SG_LOG(SG_AI, SG_DEBUG, "still no route found from " << dep->getName() << " to << " << arr->getName());

      //exit(1);
    } else {
      while(route.next(&node))
    {
      FGNode *fn = globals->get_airwaynet()->findNode(node);
      SG_LOG(SG_AI, SG_BULK, "Checking status of each waypoint: " << fn->getIdent());

      SGWayPoint first(init_waypoint->longitude,
               init_waypoint->latitude,
               alt);
      SGWayPoint curr (fn->getLongitude(),
               fn->getLatitude(),
               alt);
      SGWayPoint arr  (arr->getLongitude(),
               arr->getLatitude(),
               alt);
      
      double crse, crsDiff;
      double dist;
      first.CourseAndDistance(arr,   &course, &distance);
      first.CourseAndDistance(curr, &crse, &dist);

      dist *= SG_METER_TO_NM;

      // We're only interested in the absolute value of crsDiff
      // wich should fall in the 0-180 deg range.
      crsDiff = fabs(crse-course);
      if (crsDiff > 180)
        crsDiff = 360-crsDiff;
      // These are the three conditions that we consider including
      // in our flight plan:
      // 1) current waypoint is less then 100 miles away OR
      // 2) curren waypoint is ahead of us, at any distance
      SG_LOG(SG_AI, SG_BULK, " Distance : " << dist << " : Course diff " << crsDiff 
           << " crs to dest : " << course
           << " crs to wpt  : " << crse);
      if ((dist > 20.0) && (crsDiff > 90.0))
        {
          //useWpt = false;
          // Once we start including waypoints, we have to continue, even though
          // one of the following way point would suffice.
          // so once is the useWpt flag is set to true, we cannot reset it to false.
          SG_LOG(SG_AI, SG_BULK, " discarding ");
          //   << ": Course difference = " << crsDiff
          //  << "Course = " << course
          // << "crse   = " << crse);
        }
      else {
        //i = ids.end()-1;
        SG_LOG(SG_AI, SG_BULK, " accepting ")

        //ids.pop_back();
        wpt = new waypoint;
        wpt->name      = "Airway"; // fixme: should be the name of the waypoint
        wpt->latitude  = fn->getLatitude();
        wpt->longitude = fn->getLongitude();
        //wpt->altitude  = apt->getElevation(); // should maybe be tn->elev too
        wpt->altitude  = alt;
        wpt->speed     = 450; //speed;
        wpt->crossat   = -10000;
        wpt->gear_down = false;
        wpt->flaps_down= false;
        wpt->finished  = false;
        wpt->on_ground = false;
        pushBackWaypoint(wpt);
      }

      if (!(routefile.exists()))
        {
          route.first();
          fstream outf( routefile.c_str(), fstream::out );
          while (route.next(&node))
        outf << node << endl;
        }
    }
    }
    arr->getDynamics()->getActiveRunway("com", 2, activeRunway);
    if (!(globals->get_runways()->search(arr->getId(),
                     activeRunway,
                     &rwy)))
  {
    SG_LOG(SG_AI, SG_WARN, "Failed to find runway for " << arr->getId());
    // Hmm, how do we handle a potential error like this?
    exit(1);
  }
    //string test;
    //arr->getActiveRunway(string("com"), 1, test);
    //exit(1);

    SG_LOG(SG_AI, SG_DEBUG, "Altitude = " << alt);
    SG_LOG(SG_AI, SG_DEBUG, "Done");
    SG_LOG(SG_AI, SG_DEBUG, "Creating cruise to " << arr->getId() << " " << latitude << " " << longitude);
    heading = rwy._heading;
    azimuth = heading + 180.0;
    while ( azimuth >= 360.0 ) { azimuth -= 360.0; }


    // Note: This places us at the location of the active
    // runway during initial cruise. This needs to be
    // fixed later.
    geo_direct_wgs_84 ( 0, rwy._lat, rwy._lon, azimuth,
            110000,
            &lat2, &lon2, &az2 );
    wpt = new waypoint;
    wpt->name      = "BOD"; //wpt_node->getStringValue("name", "END");
    wpt->latitude  = lat2;
    wpt->longitude = lon2;
    wpt->altitude  = alt;
    wpt->speed     = speed;
    wpt->crossat   = alt;
    wpt->gear_down = false;
    wpt->flaps_down= false;
    wpt->finished  = false;
    wpt->on_ground = false;
    pushBackWaypoint(wpt);
}
*/


/*******************************************************************
 * CreateCruise
 * initialize the Aircraft at the parking location
 *
 * Note that this is the original version that does not 
 * do any dynamic route computation.
 ******************************************************************/
bool FGAIFlightPlan::createCruise(FGAIAircraft *ac, bool firstFlight, FGAirport *dep, 
                  FGAirport *arr, double latitude, 
                  double longitude, double speed, 
                  double alt, const string& fltType)
{
  double vCruise = ac->getPerformance()->vCruise();
  FGAIWaypoint *wpt;
  wpt = createInAir(ac, "Cruise", SGGeod::fromDeg(longitude, latitude), alt, vCruise);
  pushBackWaypoint(wpt); 
  
  const string& rwyClass = getRunwayClassFromTrafficType(fltType);
  double heading = ac->getTrafficRef()->getCourse();
  arr->getDynamics()->getActiveRunway(rwyClass, 2, activeRunway, heading);
  if (!arr->hasRunwayWithIdent(activeRunway)) {
      return false;
  }

  FGRunway* rwy = arr->getRunwayByIdent(activeRunway);
  assert( rwy != NULL );
  // begin descent 110km out
  SGGeod beginDescentPoint     = rwy->pointOnCenterline(0);
  SGGeod secondaryDescentPoint = rwy->pointOnCenterline(-10000);
  
  wpt = createInAir(ac, "BOD", beginDescentPoint,  alt, vCruise);
  pushBackWaypoint(wpt); 
  wpt = createInAir(ac, "BOD2", secondaryDescentPoint, alt, vCruise);
  pushBackWaypoint(wpt); 
  return true;
}
