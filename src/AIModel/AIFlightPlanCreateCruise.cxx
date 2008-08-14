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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 **************************************************************************/
#include <fstream>
#include <iostream>
#include "AIFlightPlan.hxx"
#include <simgear/math/polar3d.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/route/waypoint.hxx>

#include <Navaids/awynet.hxx>
#include <Airports/runways.hxx>
#include <Airports/dynamics.hxx>

#include <Environment/environment_mgr.hxx>
#include <Environment/environment.hxx>

using std::iostream;

void FGAIFlightPlan::evaluateRoutePart(double deplat,
				       double deplon,
				       double arrlat,
				       double arrlon)
{
  // First do a prescan of all the waypoints that are within a reasonable distance of the
  // ideal route.
  intVec nodes;
  int tmpNode, prevNode;


  SGWayPoint first (deplon,
		    deplat,
		    100);
  SGWayPoint sec (arrlon,
		  arrlat,
		  100);
  double course, distance;
  first.CourseAndDistance(sec, &course, &distance);
  distance *= SG_METER_TO_NM;

  SGVec3d a = SGVec3d::fromGeoc(SGGeoc::fromDegM(deplon, deplat, 1));
  SGVec3d b = SGVec3d::fromGeoc(SGGeoc::fromDegM(arrlon, arrlat, 1));
  SGVec3d _cross = cross(b, a);

  double angle = sgACos(dot(a, b));
  tmpNode = 0;
  for (double ang = 0.0; ang < angle; ang += 0.05)
    {
      sgdVec3 newPos;
      sgdMat4 matrix;
      //cerr << "Angle = " << ang << endl;
      sgdMakeRotMat4(matrix, ang, _cross.sg());
      for(int j = 0; j < 3; j++)
	{
	  newPos[j] =0.0;
	  for (int k = 0; k<3; k++)
	    {
	      newPos[j] += matrix[j][k]*a[k];
	    }
	}
      //cerr << "1"<< endl;
      SGGeoc geoc = SGGeoc::fromCart(SGVec3d(newPos[0], newPos[1], newPos[2]));

      double midlat = geoc.getLatitudeDeg();
      double midlon = geoc.getLongitudeDeg();

      prevNode = tmpNode;
      tmpNode = globals->get_airwaynet()->findNearestNode(midlat, midlon);

      double nodelat = globals->get_airwaynet()->findNode(tmpNode)->getLatitude  ();
      double nodelon = globals->get_airwaynet()->findNode(tmpNode)->getLongitude ();
      SGWayPoint curr(midlat,
      	              midlon,
      	              100);
      SGWayPoint node(nodelat,
      	              nodelon,
      	              100);
      curr.CourseAndDistance(node, &course, &distance);
      if ((distance < 25000) && (tmpNode != prevNode))
      	nodes.push_back(tmpNode);
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
    waypoints.push_back(init_waypoint);
    routefile.append("Data/AI/FlightPlans");
    snprintf(buffer, 32, "%s-%s.txt",
	     dep->getId().c_str(),
	     arr->getId().c_str());
    routefile.append(buffer);
    cerr << "trying to read " << routefile.c_str()<< endl;
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
      cerr << "still no route found from " << dep->getName() << " to << " << arr->getName() <<endl;

      //exit(1);
    } else {
      while(route.next(&node))
	{
	  FGNode *fn = globals->get_airwaynet()->findNode(node);
	  //cerr << "Checking status of each waypoint: " << fn->getIdent();

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
	  //cerr << " Distance : " << dist << " : Course diff " << crsDiff 
	  //     << " crs to dest : " << course
	  //     << " crs to wpt  : " << crse;
	  if ((dist > 20.0) && (crsDiff > 90.0))
	    {
	      //useWpt = false;
	      // Once we start including waypoints, we have to continue, even though
	      // one of the following way point would suffice.
	      // so once is the useWpt flag is set to true, we cannot reset it to false.
	      //cerr << " discarding " << endl;
	      //   << ": Course difference = " << crsDiff
	      //  << "Course = " << course
	      // << "crse   = " << crse << endl;
	    }
	  else {
	    //i = ids.end()-1;
	    //cerr << " accepting " << endl;

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
	    waypoints.push_back(wpt);
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
    cout << "Failed to find runway for " << arr->getId() << endl;
    // Hmm, how do we handle a potential error like this?
    exit(1);
  }
    //string test;
    //arr->getActiveRunway(string("com"), 1, test);
    //exit(1);

    //cerr << "Altitude = " << alt << endl;
    //cerr << "Done" << endl;
    //if (arr->getId() == "EHAM")
    //  {
    //    cerr << "Creating cruise to EHAM " << latitude << " " << longitude << endl;
    //  }
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
    waypoints.push_back(wpt);
}
*/


/*******************************************************************
 * CreateCruise
 * initialize the Aircraft at the parking location
 *
 * Note that this is the original version that does not 
 * do any dynamic route computation.
 ******************************************************************/
void FGAIFlightPlan::createCruise(bool firstFlight, FGAirport *dep, 
				  FGAirport *arr, double latitude, 
				  double longitude, double speed, 
				  double alt, const string& fltType)
{
  double wind_speed;
  double wind_heading;
  double heading;
  double lat, lon, az;
  double lat2, lon2, az2;
  double azimuth;
  waypoint *wpt;

  wpt = new waypoint;
  wpt->name      = "Cruise"; //wpt_node->getStringValue("name", "END");
  wpt->latitude  = latitude;
  wpt->longitude = longitude;
  wpt->altitude  = alt;
  wpt->speed     = speed; 
  wpt->crossat   = -10000;
  wpt->gear_down = false;
  wpt->flaps_down= false;
  wpt->finished  = false;
  wpt->on_ground = false;
  wpt->routeIndex = 0;
  waypoints.push_back(wpt); 
  
 
  string rwyClass = getRunwayClassFromTrafficType(fltType);
  arr->getDynamics()->getActiveRunway(rwyClass, 2, activeRunway);
  rwy = arr->getRunwayByIdent(activeRunway);
  
  heading = rwy._heading;
  azimuth = heading + 180.0;
  while ( azimuth >= 360.0 ) { azimuth -= 360.0; }
  
  
  geo_direct_wgs_84 ( 0, rwy._lat, rwy._lon, azimuth, 
		      110000,
		      &lat2, &lon2, &az2 );
  wpt = new waypoint;
  wpt->name      = "BOD";
  wpt->latitude  = lat2;
  wpt->longitude = lon2;
  wpt->altitude  = alt;
  wpt->speed     = speed; 
  wpt->crossat   = alt;
  wpt->gear_down = false;
  wpt->flaps_down= false;
  wpt->finished  = false;
  wpt->on_ground = false;
  wpt->routeIndex = 0;
  waypoints.push_back(wpt); 
}
