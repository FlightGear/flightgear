/******************************************************************************
 * AIFlightPlanCreate.cxx
 * Written by Durk Talsma, started May, 2004.
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
#include "AIFlightPlan.hxx"
#include <simgear/math/sg_geodesy.hxx>
#include <Airports/runways.hxx>

#include <Environment/environment_mgr.hxx>
#include <Environment/environment.hxx>


/* FGAIFlightPlan::create()
 * dynamically create a flight plan for AI traffic, based on data provided by the
 * Traffic Manager, when reading a filed flightplan failes. (DT, 2004/07/10) 
 *
 * This is the top-level function, and the only one that publicly available.
 *
 */ 


// Check lat/lon values during initialization;
void FGAIFlightPlan::create(FGAirport *dep, FGAirport *arr, int legNr, double alt, double speed, 
			    double latitude, double longitude, bool firstFlight,
			    double radius, const string& fltType, const string& aircraftType, const string& airline)
{ 
  int currWpt = wpt_iterator - waypoints.begin();
  switch(legNr)
    {
    case 1:
      //cerr << "Creating Push_Back" << endl;
      createPushBack(firstFlight,dep, latitude, longitude, radius, fltType, aircraftType, airline);
      //cerr << "Done" << endl;
      break;
    case 2: 
      //cerr << "Creating Taxi" << endl;
      createTaxi(firstFlight, 1, dep, latitude, longitude, radius, fltType, aircraftType, airline);
      break;
    case 3: 
      //cerr << "Creating TAkeoff" << endl;
      createTakeOff(firstFlight, dep, speed);
      break;
    case 4: 
      //cerr << "Creating Climb" << endl;
      createClimb(firstFlight, dep, speed, alt);
      break;
    case 5: 
      //cerr << "Creating Cruise" << endl;
      createCruise(firstFlight, dep,arr, latitude, longitude, speed, alt);
      break;
    case 6: 
      //cerr << "Creating Decent" << endl;
      createDecent(arr);
      break;
    case 7: 
      //cerr << "Creating Landing" << endl;
      createLanding(arr);
      break;
    case 8: 
      //cerr << "Creating Taxi 2" << endl;
      createTaxi(false, 2, arr, latitude, longitude, radius, fltType, aircraftType, airline);
      break;
    case 9: 
      //cerr << "Creating Parking" << endl;
      createParking(arr);
      break;
    default:
      //exit(1);
      cerr << "Unknown case: " << legNr << endl;
    }
  wpt_iterator = waypoints.begin()+currWpt;
  leg++;
}

/*******************************************************************
 * createPushBack
 * initialize the Aircraft at the parking location
 ******************************************************************/
void FGAIFlightPlan::createPushBack(bool firstFlight, FGAirport *dep, 
				    double latitude,
				    double longitude,
				    double radius,
				    const string& fltType,
				    const string& aircraftType,
				    const string& airline)
{
  double heading;
  double lat;
  double lon;
  double lat2;
  double lon2;
  double az2;
  
  //int currWpt = wpt_iterator - waypoints.begin();
  // Erase all existing waypoints.
  //resetWaypoints();
  
  // We only need to get a valid parking if this is the first leg. 
  // Otherwise use the current aircraft position.
  if (firstFlight)
    {
      if (!(dep->getAvailableParking(&lat, &lon, &heading, &gateId, radius, fltType, aircraftType, airline)))
	{
	  cerr << "Could not find parking " << endl;
	}
    }
  else
    {
      dep->getParking(gateId, &lat, &lon, &heading);
      //lat     = latitude;
      //lon     = longitude;
      //heading = getHeading();
    }
  heading += 180.0;
  if (heading > 360)
    heading -= 360;
  waypoint *wpt = new waypoint;
  wpt->name      = "park";
  wpt->latitude  = lat;
  wpt->longitude = lon;
  wpt->altitude  = dep->getElevation();
  wpt->speed     = -10; 
  wpt->crossat   = -10000;
  wpt->gear_down = true;
  wpt->flaps_down= true;
  wpt->finished  = false;
  wpt->on_ground = true;
  waypoints.push_back(wpt); 
  
  // Add park twice, because it uses park once for initialization and once
  // to trigger the departure ATC message 
  geo_direct_wgs_84 ( 0, lat, lon, heading, 
		      10, 
		      &lat2, &lon2, &az2 );
  wpt = new waypoint;
  wpt->name      = "park2";
  wpt->latitude  = lat2;
  wpt->longitude = lon2;
  wpt->altitude  = dep->getElevation();
  wpt->speed     = -10; 
  wpt->crossat   = -10000;
  wpt->gear_down = true;
  wpt->flaps_down= true;
  wpt->finished  = false;
  wpt->on_ground = true;
  waypoints.push_back(wpt); 
  geo_direct_wgs_84 ( 0, lat, lon, heading, 
		      radius,                 // push back one entire aircraft radius
		      &lat2, &lon2, &az2 );
  wpt = new waypoint;
  wpt->name      = "taxiStart";
  wpt->latitude  = lat2;
  wpt->longitude = lon2;
  wpt->altitude  = dep->getElevation();
  wpt->speed     = 10; 
  wpt->crossat   = -10000;
  wpt->gear_down = true;
  wpt->flaps_down= true;
  wpt->finished  = false;
  wpt->on_ground = true;
  waypoints.push_back(wpt); 

  
}

/*******************************************************************
 * createCreate Taxi. 
 * initialize the Aircraft at the parking location
 ******************************************************************/
void FGAIFlightPlan::createTaxi(bool firstFlight, int direction, FGAirport *apt, double latitude, double longitude, double radius, const string& fltType, const string& acType, const string& airline)
{
  double wind_speed;
  double wind_heading;
  double heading;
  //FGRunway rwy;
  double lat, lon, az;
  double lat2, lon2, az2;
  //int direction;
  waypoint *wpt;

  // Erase all existing waypoints.
  //   wpt_vector_iterator i= waypoints.begin();
  //resetWaypoints();
  //int currWpt = wpt_iterator - waypoints.begin();
  if (direction == 1)
    {
      //string name;
      // "NOTE: this is currently fixed to "com" for commercial traffic
      // Should be changed to be used dynamically to allow "gen" and "mil"
      // as well
      apt->getActiveRunway("com", 1, activeRunway);
      if (!(globals->get_runways()->search(apt->getId(), 
					    activeRunway, 
					    &rwy)))
	{
	  cout << "Failed to find runway for " << apt->getId() << endl;
	  // Hmm, how do we handle a potential error like this?
	  exit(1);
	}
      //string test;
      //apt->getActiveRunway(string("com"), 1, test);
      //exit(1);
      
      heading = rwy._heading;
      double azimuth = heading + 180.0;
      while ( azimuth >= 360.0 ) { azimuth -= 360.0; }
      geo_direct_wgs_84 ( 0, rwy._lat, rwy._lon, azimuth, 
			  rwy._length * SG_FEET_TO_METER * 0.5 - 5.0,
			  &lat2, &lon2, &az2 );
      if (apt->getGroundNetwork()->exists())
	{
	  intVec ids;
	  int runwayId = apt->getGroundNetwork()->findNearestNode(lat2, lon2);
	  //int currId   = apt->getGroundNetwork()->findNearestNode(latitude,longitude);
	  //exit(1);
	  
	  // A negative gateId indicates an overflow parking, use a
	  // fallback mechanism for this. 
	  // Starting from gate 0 is a bit of a hack...
	  FGTaxiRoute route;
	  if (gateId >= 0)
	    route = apt->getGroundNetwork()->findShortestRoute(gateId, runwayId);
	  else
	    route = apt->getGroundNetwork()->findShortestRoute(0, runwayId);
	  intVecIterator i;
	  //cerr << "creating route : ";
	  // No route found: go from gate directly to runway
	  if (route.empty()) {
	    //Add the runway startpoint;
	    wpt = new waypoint;
	    wpt->name      = "Airport Center";
	    wpt->latitude  = latitude;
	    wpt->longitude = longitude;
	    wpt->altitude  = apt->getElevation();
	    wpt->speed     = 15; 
	    wpt->crossat   = -10000;
	    wpt->gear_down = true;
	    wpt->flaps_down= true;
	    wpt->finished  = false;
	    wpt->on_ground = true;
	    waypoints.push_back(wpt);
	    
	    //Add the runway startpoint;
	    wpt = new waypoint;
	    wpt->name      = "Runway Takeoff";
	    wpt->latitude  = lat2;
	    wpt->longitude = lon2;
	    wpt->altitude  = apt->getElevation();
	    wpt->speed     = 15; 
	    wpt->crossat   = -10000;
	    wpt->gear_down = true;
	    wpt->flaps_down= true;
	    wpt->finished  = false;
	    wpt->on_ground = true;
	    waypoints.push_back(wpt);	
	  } else {
	    int node;
	    route.first();
	    while(route.next(&node))
	      {
		//i = ids.end()-1;
		//cerr << "Creating Node: " << node << endl;
		FGTaxiNode *tn = apt->getGroundNetwork()->findNode(node);
		//ids.pop_back();  
		wpt = new waypoint;
		wpt->name      = "taxiway"; // fixme: should be the name of the taxiway
		wpt->latitude  = tn->getLatitude();
		wpt->longitude = tn->getLongitude();
		wpt->altitude  = apt->getElevation(); // should maybe be tn->elev too
		wpt->speed     = 15; 
		wpt->crossat   = -10000;
		wpt->gear_down = true;
		wpt->flaps_down= true;
		wpt->finished  = false;
		wpt->on_ground = true;
		waypoints.push_back(wpt);
	      }
	    cerr << endl;
	  }
	  //exit(1);
	}
      else 
	{
	  //Add the runway startpoint;
	  wpt = new waypoint;
	  wpt->name      = "Airport Center";
	  wpt->latitude  = apt->getLatitude();
	  wpt->longitude = apt->getLongitude();
	  wpt->altitude  = apt->getElevation();
	  wpt->speed     = 15; 
	  wpt->crossat   = -10000;
	  wpt->gear_down = true;
	  wpt->flaps_down= true;
	  wpt->finished  = false;
	  wpt->on_ground = true;
	  waypoints.push_back(wpt);
	  
	  //Add the runway startpoint;
	  wpt = new waypoint;
	  wpt->name      = "Runway Takeoff";
	  wpt->latitude  = lat2;
	  wpt->longitude = lon2;
	  wpt->altitude  = apt->getElevation();
	  wpt->speed     = 15; 
	  wpt->crossat   = -10000;
	  wpt->gear_down = true;
	  wpt->flaps_down= true;
	  wpt->finished  = false;
	  wpt->on_ground = true;
	  waypoints.push_back(wpt);
	  //wpt = new waypoint;
	  //wpt->finished = false;
	  //waypoints.push_back(wpt);  // add one more to prevent a segfault. 
	}
    }
  else  // Landing taxi
    {
      //string name;
      // "NOTE: this is currently fixed to "com" for commercial traffic
      // Should be changed to be used dynamically to allow "gen" and "mil"
      // as well
      //apt->getActiveRunway("com", 1, name);
      //if (!(globals->get_runways()->search(apt->getId(), 
      //				    name, 
      //			    &rwy)))
      //{//
      //cout << "Failed to find runway for " << apt->getId() << endl;
      // Hmm, how do we handle a potential error like this?
      // exit(1);
      //	}
      //string test;
      //apt->getActiveRunway(string("com"), 1, test);
      //exit(1);
      
      //heading = rwy._heading;
      //double azimuth = heading + 180.0;
      //while ( azimuth >= 360.0 ) { azimuth -= 360.0; }
      //geo_direct_wgs_84 ( 0, rwy._lat, rwy._lon, azimuth, 
      //		  rwy._length * SG_FEET_TO_METER * 0.5 - 5.0,
      //		  &lat2, &lon2, &az2 );
      apt->getAvailableParking(&lat, &lon, &heading, &gateId, radius, fltType, acType, airline);
      heading += 180.0;
      if (heading > 360)
	heading -= 360;
      geo_direct_wgs_84 ( 0, lat, lon, heading, 
			  100,
			  &lat2, &lon2, &az2 );
      double lat3 = (*(waypoints.end()-1))->latitude;
      double lon3 = (*(waypoints.end()-1))->longitude;
      cerr << (*(waypoints.end()-1))->name << endl;
      if (apt->getGroundNetwork()->exists())
	{
	  intVec ids;
	  int runwayId = apt->getGroundNetwork()->findNearestNode(lat3, lon3);
	  //int currId   = apt->getGroundNetwork()->findNearestNode(latitude,longitude);
	  //exit(1);
	  
	  // A negative gateId indicates an overflow parking, use a
	  // fallback mechanism for this. 
	  // Starting from gate 0 is a bit of a hack...
	  FGTaxiRoute route;
	  if (gateId >= 0)
	    route = apt->getGroundNetwork()->findShortestRoute(runwayId, gateId);
	  else
	    route = apt->getGroundNetwork()->findShortestRoute(runwayId, 0);
	  intVecIterator i;
	  //cerr << "creating route : ";
	  // No route found: go from gate directly to runway
	  if (route.empty()) {
	    //Add the runway startpoint;
	    wpt = new waypoint;
	    wpt->name      = "Airport Center";
	    wpt->latitude  = latitude;
	    wpt->longitude = longitude;
	    wpt->altitude  = apt->getElevation();
	    wpt->speed     = 15; 
	    wpt->crossat   = -10000;
	    wpt->gear_down = true;
	    wpt->flaps_down= true;
	    wpt->finished  = false;
	    wpt->on_ground = true;
	    waypoints.push_back(wpt);
	    
	    //Add the runway startpoint;
	    wpt = new waypoint;
	    wpt->name      = "Runway Takeoff";
	    wpt->latitude  = lat2;
	    wpt->longitude = lon2;
	    wpt->altitude  = apt->getElevation();
	    wpt->speed     = 15; 
	    wpt->crossat   = -10000;
	    wpt->gear_down = true;
	    wpt->flaps_down= true;
	    wpt->finished  = false;
	    wpt->on_ground = true;
	    waypoints.push_back(wpt);	
	  } else {
	    int node;
	    route.first();
	    while(route.next(&node))
	      {
		//i = ids.end()-1;
		//cerr << "Creating Node: " << node << endl;
		FGTaxiNode *tn = apt->getGroundNetwork()->findNode(node);
		//ids.pop_back();  
		wpt = new waypoint;
		wpt->name      = "taxiway"; // fixme: should be the name of the taxiway
		wpt->latitude  = tn->getLatitude();
		wpt->longitude = tn->getLongitude();
		wpt->altitude  = apt->getElevation(); // should maybe be tn->elev too
		wpt->speed     = 15; 
		wpt->crossat   = -10000;
		wpt->gear_down = true;
		wpt->flaps_down= true;
		wpt->finished  = false;
		wpt->on_ground = true;
		waypoints.push_back(wpt);
	      }
	    cerr << endl;
	  }
	  //exit(1);
	}
      else
	{
	  //Add the runway startpoint;
	  wpt = new waypoint;
	  wpt->name      = "Airport Center";
	  wpt->latitude  = apt->getLatitude();
	  wpt->longitude = apt->getLongitude();
	  wpt->altitude  = apt->getElevation();
	  wpt->speed     = 15; 
	  wpt->crossat   = -10000;
	  wpt->gear_down = true;
	  wpt->flaps_down= true;
	  wpt->finished  = false;
	  wpt->on_ground = true;
	  waypoints.push_back(wpt);
	  
	}




      // Add the final destination waypoint
      wpt = new waypoint;
      wpt->name      = "Begin Parking"; //apt->getId(); //wpt_node->getStringValue("name", "END");
      wpt->latitude  = lat2;
      wpt->longitude = lon2;
      wpt->altitude  = apt->getElevation();
      wpt->speed     = 15; 
      wpt->crossat   = -10000;
      wpt->gear_down = true;
      wpt->flaps_down= true;
      wpt->finished  = false;
      wpt->on_ground = true;
      waypoints.push_back(wpt); 

     
    }
  // wpt_iterator = waypoints.begin();
  //if (!firstFlight)
  // wpt_iterator++; 
  //wpt_iterator = waypoints.begin()+currWpt;
}

/*******************************************************************
 * CreateTakeOff 
 * initialize the Aircraft at the parking location
 ******************************************************************/
void FGAIFlightPlan::createTakeOff(bool firstFlight, FGAirport *apt, double speed)
{
  double wind_speed;
  double wind_heading;
  double heading;
  //FGRunway rwy;
  double lat, lon, az;
  double lat2, lon2, az2;
  //int direction;
  waypoint *wpt;
  
  
  // Erase all existing waypoints.
  // wpt_vector_iterator i= waypoints.begin();
  //while(waypoints.begin() != waypoints.end())
  //  {      
  //    delete *(i);
  //    waypoints.erase(i);
  //  }
  //resetWaypoints();
  
  
  // Get the current active runway, based on code from David Luff
  // This should actually be unified and extended to include 
  // Preferential runway use schema's 
  if (firstFlight)
    {
      //string name;
       // "NOTE: this is currently fixed to "com" for commercial traffic
      // Should be changed to be used dynamically to allow "gen" and "mil"
      // as well
      apt->getActiveRunway("com", 1, activeRunway);
	if (!(globals->get_runways()->search(apt->getId(), 
					      activeRunway, 
					      &rwy)))
	  {
	    cout << "Failed to find runway for " << apt->getId() << endl;
	    // Hmm, how do we handle a potential error like this?
	    exit(1);
	  }
	//string test;
      //apt->getActiveRunway(string("com"), 1, test);
      //exit(1);
    }
  
  heading = rwy._heading;
  double azimuth = heading + 180.0;
  while ( azimuth >= 360.0 ) { azimuth -= 360.0; }
  geo_direct_wgs_84 ( 0, rwy._lat, rwy._lon, azimuth, 
		      rwy._length * SG_FEET_TO_METER * 0.5 - 105.0,
		      &lat2, &lon2, &az2 );
  wpt = new waypoint; 
  wpt->name      = "accel"; 
  wpt->latitude  = lat2; 
  wpt->longitude = lon2; 
  wpt->altitude  = apt->getElevation();
  wpt->speed     = speed;  
  wpt->crossat   = -10000;
  wpt->gear_down = true;
  wpt->flaps_down= true;
  wpt->finished  = false;
  wpt->on_ground = true;
  waypoints.push_back(wpt); 
  
  lat = lat2;
  lon = lon2;
  az  = az2;
  
  //Next: the Start of Climb
  geo_direct_wgs_84 ( 0, lat, lon, heading, 
  2560 * SG_FEET_TO_METER,
  &lat2, &lon2, &az2 );
  
  wpt = new waypoint;
  wpt->name      = "SOC";
  wpt->latitude  = lat2;
  wpt->longitude = lon2;
  wpt->altitude  = apt->getElevation()+3000;
  wpt->speed     = speed; 
  wpt->crossat   = -10000;
  wpt->gear_down = true;
  wpt->flaps_down= true;
  wpt->finished  = false;
  wpt->on_ground = false;
  waypoints.push_back(wpt);
  //  waypoints.push_back(wpt);
  //waypoints.push_back(wpt);  // add one more to prevent a segfault. 
  // wpt_iterator = waypoints.begin();
  //if (!firstFlight)
  // wpt_iterator++;
}
 
/*******************************************************************
 * CreateClimb
 * initialize the Aircraft at the parking location
 ******************************************************************/
void FGAIFlightPlan::createClimb(bool firstFlight, FGAirport *apt, double speed, double alt)
{
  double wind_speed;
  double wind_heading;
  double heading;
  //FGRunway rwy;
  double lat, lon, az;
  double lat2, lon2, az2;
  //int direction;
  waypoint *wpt;

  // Erase all existing waypoints.
  // wpt_vector_iterator i= waypoints.begin();
  //while(waypoints.begin() != waypoints.end())
  //  {      
  //    delete *(i);
  //    waypoints.erase(i);
  //  }
  //resetWaypoints();
  
  
  // Get the current active runway, based on code from David Luff
  // This should actually be unified and extended to include 
  // Preferential runway use schema's 
  if (firstFlight)
    {
      //string name;
      // "NOTE: this is currently fixed to "com" for commercial traffic
      // Should be changed to be used dynamically to allow "gen" and "mil"
      // as well
      apt->getActiveRunway("com", 1, activeRunway);
	if (!(globals->get_runways()->search(apt->getId(), 
					      activeRunway, 
					      &rwy)))
	  {
	    cout << "Failed to find runway for " << apt->getId() << endl;
	    // Hmm, how do we handle a potential error like this?
	    exit(1);
	  }
	//string test;
	//apt->getActiveRunway(string("com"), 1, test);
      //exit(1);
    }
  
  
  heading = rwy._heading;
  double azimuth = heading + 180.0;
  while ( azimuth >= 360.0 ) { azimuth -= 360.0; }
  geo_direct_wgs_84 ( 0, rwy._lat, rwy._lon, heading, 
		      10*SG_NM_TO_METER,
		      &lat2, &lon2, &az2 );
  wpt = new waypoint;
  wpt->name      = "10000ft climb";
  wpt->latitude  = lat2;
  wpt->longitude = lon2;
  wpt->altitude  = 10000;
  wpt->speed     = speed; 
  wpt->crossat   = -10000;
  wpt->gear_down = true;
  wpt->flaps_down= true;
  wpt->finished  = false;
  wpt->on_ground = false;
  waypoints.push_back(wpt); 
  

  geo_direct_wgs_84 ( 0, rwy._lat, rwy._lon, heading, 
		      20*SG_NM_TO_METER,
		      &lat2, &lon2, &az2 );
  wpt = new waypoint;
  wpt->name      = "18000ft climb";
  wpt->latitude  = lat2;
  wpt->longitude = lon2;
  wpt->altitude  = 18000;
  wpt->speed     = speed; 
  wpt->crossat   = -10000;
  wpt->gear_down = true;
  wpt->flaps_down= true;
  wpt->finished  = false;
  wpt->on_ground = false;
  waypoints.push_back(wpt); 
  //waypoints.push_back(wpt); 
  //waypoints.push_back(wpt);  // add one more to prevent a segfault. 
  // wpt_iterator = waypoints.begin();
  //if (!firstFlight)
  // wpt_iterator++;
}


/*******************************************************************
 * CreateCruise
 * initialize the Aircraft at the parking location
 ******************************************************************/
void FGAIFlightPlan::createCruise(bool firstFlight, FGAirport *dep, FGAirport *arr, double latitude, double longitude, double speed, double alt)
{
  double wind_speed;
  double wind_heading;
  double heading;
  //FGRunway rwy;
  double lat, lon, az;
  double lat2, lon2, az2;
  double azimuth;
  //int direction;
  waypoint *wpt;

  // Erase all existing waypoints.
  // wpt_vector_iterator i= waypoints.begin();
  //while(waypoints.begin() != waypoints.end())
  //  {      
  //    delete *(i);
  //    waypoints.erase(i);
  //  }
  //resetWaypoints();

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
  waypoints.push_back(wpt); 
  //Beginning of Decent
 
  //string name;
  // should be changed dynamically to allow "gen" and "mil"
  arr->getActiveRunway("com", 2, activeRunway);
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
  //waypoints.push_back(wpt);
  //waypoints.push_back(wpt);  // add one more to prevent a segfault.  
  //wpt_iterator = waypoints.begin();
  //if (!firstFlight)
  // wpt_iterator++;
}

/*******************************************************************
 * CreateDecent
 * initialize the Aircraft at the parking location
 ******************************************************************/
void FGAIFlightPlan::createDecent(FGAirport *apt)
{

  // Ten thousand ft. Slowing down to 240 kts
  double wind_speed;
  double wind_heading;
  double heading;
  //FGRunway rwy;
  double lat, lon, az;
  double lat2, lon2, az2;
  double azimuth;
  //int direction;
  waypoint *wpt;

  //// Erase all existing waypoints.
  // wpt_vector_iterator i= waypoints.begin();
  //while(waypoints.begin() != waypoints.end())
  //  {      
  //    delete *(i);
  //    waypoints.erase(i);
  //  }
  //resetWaypoints();

  //Beginning of Decent
  //string name;
  // allow "mil" and "gen" as well
  apt->getActiveRunway("com", 2, activeRunway);
    if (!(globals->get_runways()->search(apt->getId(), 
					  activeRunway, 
					  &rwy)))
      {
	cout << "Failed to find runway for " << apt->getId() << endl;
	// Hmm, how do we handle a potential error like this?
	exit(1);
      }
    //string test;
    //apt->getActiveRunway(string("com"), 1, test);
  //exit(1);

  //cerr << "Done" << endl;
  heading = rwy._heading;
  azimuth = heading + 180.0;
  while ( azimuth >= 360.0 ) { azimuth -= 360.0; }
  
  
  
  geo_direct_wgs_84 ( 0, rwy._lat, rwy._lon, azimuth, 
		      100000,
		      &lat2, &lon2, &az2 );
  
  wpt = new waypoint;
  wpt->name      = "Dec 10000ft"; //wpt_node->getStringValue("name", "END");
  wpt->latitude  = lat2;
  wpt->longitude = lon2;
  wpt->altitude  = apt->getElevation();
  wpt->speed     = 240; 
  wpt->crossat   = 10000;
  wpt->gear_down = false;
  wpt->flaps_down= false;
  wpt->finished  = false;
  wpt->on_ground = false;
  waypoints.push_back(wpt);  

  // Three thousand ft. Slowing down to 160 kts
  geo_direct_wgs_84 ( 0, rwy._lat, rwy._lon, azimuth, 
		     8*SG_NM_TO_METER,
		     &lat2, &lon2, &az2 );
  wpt = new waypoint;
  wpt->name      = "DEC 3000ft"; //wpt_node->getStringValue("name", "END");
  wpt->latitude  = lat2;
  wpt->longitude = lon2;
  wpt->altitude  = apt->getElevation();
  wpt->speed     = 160; 
  wpt->crossat   = 3000;
  wpt->gear_down = true;
  wpt->flaps_down= true;
  wpt->finished  = false;
  wpt->on_ground = false;
  waypoints.push_back(wpt);
  //waypoints.push_back(wpt);
  //waypoints.push_back(wpt);  // add one more to prevent a segfault. 
  //wpt_iterator = waypoints.begin();
  //wpt_iterator++;
  //if (apt->getId() == "EHAM")
  //  {
  //    cerr << "Created Decend to EHAM " << lat2 << " " << lon2 << ": Runway = " << rwy._rwy_no 
  //	   << "heading " << heading << endl;
  //  }
}
/*******************************************************************
 * CreateLanding
 * initialize the Aircraft at the parking location
 ******************************************************************/
void FGAIFlightPlan::createLanding(FGAirport *apt)
{
  // Ten thousand ft. Slowing down to 240 kts
  double wind_speed;
  double wind_heading;
  double heading;
  //FGRunway rwy;
  double lat, lon, az;
  double lat2, lon2, az2;
  double azimuth;
  //int direction;
  waypoint *wpt;

  
  heading = rwy._heading;
  azimuth = heading + 180.0;
  while ( azimuth >= 360.0 ) { azimuth -= 360.0; }

  //Runway Threshold
 geo_direct_wgs_84 ( 0, rwy._lat, rwy._lon, azimuth, 
		     rwy._length*0.45 * SG_FEET_TO_METER,
		     &lat2, &lon2, &az2 );
  wpt = new waypoint;
  wpt->name      = "Threshold"; //wpt_node->getStringValue("name", "END");
  wpt->latitude  = lat2;
  wpt->longitude = lon2;
  wpt->altitude  = apt->getElevation();
  wpt->speed     = 150; 
  wpt->crossat   = apt->getElevation();
  wpt->gear_down = true;
  wpt->flaps_down= true;
  wpt->finished  = false;
  wpt->on_ground = true;
  waypoints.push_back(wpt); 

 //Full stop at the runway centerpoint
 geo_direct_wgs_84 ( 0, rwy._lat, rwy._lon, azimuth, 
		     rwy._length*0.45,
		      &lat2, &lon2, &az2 );
  wpt = new waypoint;
  wpt->name      = "Center"; //wpt_node->getStringValue("name", "END");
  wpt->latitude  = rwy._lat;
  wpt->longitude = rwy._lon;
  wpt->altitude  = apt->getElevation();
  wpt->speed     = 30; 
  wpt->crossat   = -10000;
  wpt->gear_down = true;
  wpt->flaps_down= true;
  wpt->finished  = false;
  wpt->on_ground = true;
  waypoints.push_back(wpt);

 geo_direct_wgs_84 ( 0, rwy._lat, rwy._lon, heading, 
		     rwy._length*0.45 * SG_FEET_TO_METER,
		     &lat2, &lon2, &az2 );
  wpt = new waypoint;
  wpt->name      = "Threshold"; //wpt_node->getStringValue("name", "END");
  wpt->latitude  = lat2;
  wpt->longitude = lon2;
  wpt->altitude  = apt->getElevation();
  wpt->speed     = 15; 
  wpt->crossat   = apt->getElevation();
  wpt->gear_down = true;
  wpt->flaps_down= true;
  wpt->finished  = false;
  wpt->on_ground = true;
  waypoints.push_back(wpt); 
  //waypoints.push_back(wpt); 
  //waypoints.push_back(wpt);  // add one more to prevent a segfault. 
  //wpt_iterator = waypoints.begin();
  //wpt_iterator++;

  //if (apt->getId() == "EHAM")
  //{
  //  cerr << "Created Landing to EHAM " << lat2 << " " << lon2 << ": Runway = " << rwy._rwy_no 
  //	 << "heading " << heading << endl;
  //}
}

/*******************************************************************
 * CreateParking
 * initialize the Aircraft at the parking location
 ******************************************************************/
void FGAIFlightPlan::createParking(FGAirport *apt)
{
  waypoint* wpt;
  double lat;
  double lon;
  double heading;
  apt->getParking(gateId, &lat, &lon, &heading);
  heading += 180.0;
  if (heading > 360)
    heading -= 360; 

  // Erase all existing waypoints.
  // wpt_vector_iterator i= waypoints.begin();
  //while(waypoints.begin() != waypoints.end())
  //  {      
  //    delete *(i);
  //    waypoints.erase(i);
  //  }
  //resetWaypoints();
  // And finally one more named "END"
  wpt = new waypoint;
  wpt->name      = "END"; //wpt_node->getStringValue("name", "END");
  wpt->latitude  = lat;
  wpt->longitude = lon;
  wpt->altitude  = 19;
  wpt->speed     = 15; 
  wpt->crossat   = -10000;
  wpt->gear_down = true;
  wpt->flaps_down= true;
  wpt->finished  = false;
  wpt->on_ground = true;
  waypoints.push_back(wpt);
  //waypoints.push_back(wpt);
  //waypoints.push_back(wpt);  // add one more to prevent a segfault. 
  //wpt_iterator = waypoints.begin();
  //wpt_iterator++;
}
