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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
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
 * This is the top-level function, and the only one that is publicly available.
 *
 */ 


// Check lat/lon values during initialization;
void FGAIFlightPlan::create(FGAirport *dep, FGAirport *arr, int legNr, 
			    double alt, double speed, double latitude, 
			    double longitude, bool firstFlight,double radius, 
			    const string& fltType, const string& aircraftType, 
			    const string& airline)
{ 
  int currWpt = wpt_iterator - waypoints.begin();
  switch(legNr)
    {
    case 1:
      createPushBack(firstFlight,dep, latitude, longitude, 
		     radius, fltType, aircraftType, airline);
      break;
    case 2: 
      createTaxi(firstFlight, 1, dep, latitude, longitude, 
		 radius, fltType, aircraftType, airline);
      break;
    case 3: 
      createTakeOff(firstFlight, dep, speed);
      break;
    case 4: 
      createClimb(firstFlight, dep, speed, alt);
      break;
    case 5: 
      createCruise(firstFlight, dep,arr, latitude, longitude, speed, alt);
      break;
    case 6: 
      createDecent(arr);
      break;
    case 7: 
      createLanding(arr);
      break;
    case 8: 
      createTaxi(false, 2, arr, latitude, longitude, radius, 
		 fltType, aircraftType, airline);
      break;
      case 9: 
	createParking(arr, radius);
      break;
    default:
      //exit(1);
      SG_LOG(SG_INPUT, SG_ALERT, "AIFlightPlan::create() attempting to create unknown leg"
	     " this is probably an internal program error");
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
      if (!(dep->getDynamics()->getAvailableParking(&lat, &lon, 
 						    &heading, &gateId, 
 						    radius, fltType, 
 						    aircraftType, airline)))
 	{
 	  SG_LOG(SG_INPUT, SG_WARN, "Could not find parking for a " << 
 		 aircraftType <<
 		 " of flight type " << fltType << 
 		 " of airline     " << airline <<
 		 " at airport     " << dep->getId());
 	}
    }
  else
    {
      dep->getDynamics()->getParking(gateId, &lat, &lon, &heading);
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
 		      2.2*radius,           
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
void FGAIFlightPlan::createTaxi(bool firstFlight, int direction, 
				FGAirport *apt, double latitude, double longitude, 
				double radius, const string& fltType, 
				const string& acType, const string& airline)
{
  double wind_speed;
  double wind_heading;
  double heading;
  double lat, lon, az;
  double lat2, lon2, az2;
  waypoint *wpt;

   if (direction == 1)
    {
      // If this function is called during initialization,
      // make sure we obtain a valid gate ID first
      // and place the model at the location of the gate.
      if (firstFlight)
	{
	  if (!(apt->getDynamics()->getAvailableParking(&lat, &lon, 
							&heading, &gateId, 
							radius, fltType, 
							acType, airline)))
	    {
	      SG_LOG(SG_INPUT, SG_WARN, "Could not find parking for a " << 
		     acType <<
		     " of flight type " << fltType <<
		     " of airline     " << airline <<
		     " at airport     " << apt->getId());
	    }
	  //waypoint *wpt = new waypoint;
	  //wpt->name      = "park";
	  //wpt->latitude  = lat;
	  //wpt->longitude = lon;
	  //wpt->altitude  = apt->getElevation();
	  //wpt->speed     = -10; 
	  //wpt->crossat   = -10000;
	  //wpt->gear_down = true;
	  //wpt->flaps_down= true;
	  //wpt->finished  = false;
	  //wpt->on_ground = true;
	  //waypoints.push_back(wpt);  
	}
      // "NOTE: this is currently fixed to "com" for commercial traffic
      // Should be changed to be used dynamically to allow "gen" and "mil"
      // as well
      apt->getDynamics()->getActiveRunway("com", 1, activeRunway);
      if (!(globals->get_runways()->search(apt->getId(), 
					    activeRunway, 
					    &rwy)))
	{
	   SG_LOG(SG_INPUT, SG_ALERT, "Failed to find runway " << 
		  activeRunway << 
		  " at airport     " << apt->getId());
	   exit(1);
	} 

      // Determine the beginning of he runway
      heading = rwy._heading;
      double azimuth = heading + 180.0;
      while ( azimuth >= 360.0 ) { azimuth -= 360.0; }
      geo_direct_wgs_84 ( 0, rwy._lat, rwy._lon, azimuth, 
			  rwy._length * SG_FEET_TO_METER * 0.5 - 5.0,
			  &lat2, &lon2, &az2 );

      if (apt->getDynamics()->getGroundNetwork()->exists())
	{
	  intVec ids;
	  int runwayId = apt->getDynamics()->getGroundNetwork()->findNearestNode(lat2, 
										 lon2);

	  
	  // A negative gateId indicates an overflow parking, use a
	  // fallback mechanism for this. 
	  // Starting from gate 0 in this case is a bit of a hack
	  // which requires a more proper solution later on.
	  FGTaxiRoute route;
	  if (gateId >= 0)
	    route = apt->getDynamics()->getGroundNetwork()->findShortestRoute(gateId, 
									      runwayId);
	  else
	    route = apt->getDynamics()->getGroundNetwork()->findShortestRoute(0, runwayId);
	  intVecIterator i;
	 
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
	    bool isPushBackPoint = false;
	    if (firstFlight) {
	      // If this is called during initialization, randomly
	      // skip a number of waypoints to get a more realistic
	      // taxi situation.
	      isPushBackPoint = true;
	      int nrWaypoints = route.size();
	      int nrWaypointsToSkip = rand() % nrWaypoints;
	      // but make sure we always keep two active waypoints
	      // to prevent a segmentation fault
	      for (int i = 0; i < nrWaypointsToSkip-2; i++) {
		isPushBackPoint = false;
		route.next(&node);
	      }
	    }
	    else {
	      //chop off the first two waypoints, because
	      // those have already been created
	      // by create pushback
	      int size = route.size();
	      if (size > 2) {
		route.next(&node);
		route.next(&node);
	      }
	    }
	    while(route.next(&node))
	      {
		FGTaxiNode *tn = apt->getDynamics()->getGroundNetwork()->findNode(node);
		//ids.pop_back();  
		wpt = new waypoint;
		wpt->name      = "taxiway"; // fixme: should be the name of the taxiway
		wpt->latitude  = tn->getLatitude();
		wpt->longitude = tn->getLongitude();
		// Elevation is currently disregarded when on_ground is true
		// because the AIModel obtains a periodic ground elevation estimate.
		wpt->altitude  = apt->getElevation();
		if (isPushBackPoint) {
		  wpt->speed = -10;
		  isPushBackPoint = false;
		}
		else {
		  wpt->speed     = 15; 
		}
		wpt->crossat   = -10000;
		wpt->gear_down = true;
		wpt->flaps_down= true;
		wpt->finished  = false;
		wpt->on_ground = true;
		waypoints.push_back(wpt);
	      }
	    cerr << endl;
	  }
	}
      else 
	{
	  // This is the fallback mechanism, in case no ground network is available
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
	}
    }
  else  // Landing taxi
    {
      apt->getDynamics()->getAvailableParking(&lat, &lon, &heading, 
					      &gateId, radius, fltType, 
					      acType, airline);
     
      double lat3 = (*(waypoints.end()-1))->latitude;
      double lon3 = (*(waypoints.end()-1))->longitude;
      //cerr << (*(waypoints.end()-1))->name << endl;
      
      // Find a route from runway end to parking/gate.
      if (apt->getDynamics()->getGroundNetwork()->exists())
	{
	  intVec ids;
	  int runwayId = apt->getDynamics()->getGroundNetwork()->findNearestNode(lat3, 
										 lon3);
	  // A negative gateId indicates an overflow parking, use a
	  // fallback mechanism for this. 
	  // Starting from gate 0 is a bit of a hack...
	  FGTaxiRoute route;
	  if (gateId >= 0)
	    route = apt->getDynamics()->getGroundNetwork()->findShortestRoute(runwayId, 
									      gateId);
	  else
	    route = apt->getDynamics()->getGroundNetwork()->findShortestRoute(runwayId, 0);
	  intVecIterator i;
	 
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
	    int size = route.size();
	    // Omit the last two waypoints, as 
	    // those are created by createParking()
	    for (int i = 0; i < size-2; i++)
	      {
		route.next(&node);
		FGTaxiNode *tn = apt->getDynamics()->getGroundNetwork()->findNode(node);
		wpt = new waypoint;
		wpt->name      = "taxiway"; // fixme: should be the name of the taxiway
		wpt->latitude  = tn->getLatitude();
		wpt->longitude = tn->getLongitude();
		wpt->altitude  = apt->getElevation();
		wpt->speed     = 15; 
		wpt->crossat   = -10000;
		wpt->gear_down = true;
		wpt->flaps_down= true;
		wpt->finished  = false;
		wpt->on_ground = true;
		waypoints.push_back(wpt);
	      }
	  }
	}
      else
	{
	  // Use a fallback mechanism in case no ground network is available
	  // obtain the location of the gate entrance point 
	  heading += 180.0;
	  if (heading > 360)
	    heading -= 360;
	  geo_direct_wgs_84 ( 0, lat, lon, heading, 
			      100,
			      &lat2, &lon2, &az2 );
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

	  //waypoint* wpt;
	  //double lat;
	  //double lon;
	  //double heading;
	  apt->getDynamics()->getParking(gateId, &lat, &lon, &heading);
	  heading += 180.0;
	  if (heading > 360)
	    heading -= 360; 
	  
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
	}
      
    }
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
  double lat, lon, az;
  double lat2, lon2, az2;
  waypoint *wpt;
  
  // Get the current active runway, based on code from David Luff
  // This should actually be unified and extended to include 
  // Preferential runway use schema's 
  if (firstFlight)
    {
      //string name;
       // "NOTE: this is currently fixed to "com" for commercial traffic
      // Should be changed to be used dynamically to allow "gen" and "mil"
      // as well
      apt->getDynamics()->getActiveRunway("com", 1, activeRunway);
	if (!(globals->get_runways()->search(apt->getId(), 
					      activeRunway, 
					      &rwy)))
	  {
	    SG_LOG(SG_INPUT, SG_ALERT, "Failed to find runway " << 
		   activeRunway << 
		   " at airport     " << apt->getId());
	    exit(1);
	  }
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

 
  if (firstFlight)
    {
      //string name;
      // "NOTE: this is currently fixed to "com" for commercial traffic
      // Should be changed to be used dynamically to allow "gen" and "mil"
      // as well
      apt->getDynamics()->getActiveRunway("com", 1, activeRunway);
	if (!(globals->get_runways()->search(apt->getId(), 
					      activeRunway, 
					      &rwy)))
	  {
	    SG_LOG(SG_INPUT, SG_ALERT, "Failed to find runway " << 
		   activeRunway << 
		   " at airport     " << apt->getId());
	    exit(1);
	  }
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
}


/*******************************************************************
 * CreateCruise
 * initialize the Aircraft at the parking location
 ******************************************************************/
void FGAIFlightPlan::createCruise(bool firstFlight, FGAirport *dep, 
				  FGAirport *arr, double latitude, 
				  double longitude, double speed, 
				  double alt)
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
  waypoints.push_back(wpt); 
  
 
  // should be changed dynamically to allow "gen" and "mil"
  arr->getDynamics()->getActiveRunway("com", 2, activeRunway);
  if (!(globals->get_runways()->search(arr->getId(), 
				       activeRunway, 
				       &rwy)))
    {
      SG_LOG(SG_INPUT, SG_ALERT, "Failed to find runway " << 
	     activeRunway << 
	     " at airport     " << arr->getId());
      exit(1);
    }
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
  waypoints.push_back(wpt); 
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

  //Beginning of Decent
  //string name;
  // allow "mil" and "gen" as well
  apt->getDynamics()->getActiveRunway("com", 2, activeRunway);
  if (!(globals->get_runways()->search(apt->getId(), 
				       activeRunway, 
				       &rwy)))
    {
      SG_LOG(SG_INPUT, SG_ALERT, "Failed to find runway " << 
	     activeRunway << 
	     " at airport     " << apt->getId());
      exit(1);
    }
  
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
}
/*******************************************************************
 * CreateLanding
 * initialize the Aircraft at the parking location
 ******************************************************************/
void FGAIFlightPlan::createLanding(FGAirport *apt)
{
  // Ten thousand ft. Slowing down to 150 kts
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
}

/*******************************************************************
 * CreateParking
 * initialize the Aircraft at the parking location
 ******************************************************************/
void FGAIFlightPlan::createParking(FGAirport *apt, double radius)
{
  waypoint* wpt;
  double lat, lat2;
  double lon, lon2;
  double az2;
  double heading;
  apt->getDynamics()->getParking(gateId, &lat, &lon, &heading);
  heading += 180.0;
  if (heading > 360)
    heading -= 360; 
  geo_direct_wgs_84 ( 0, lat, lon, heading, 
 		      2.2*radius,           
 		      &lat2, &lon2, &az2 );
  wpt = new waypoint;
  wpt->name      = "taxiStart";
  wpt->latitude  = lat2;
  wpt->longitude = lon2;
  wpt->altitude  = apt->getElevation();
  wpt->speed     = 10; 
  wpt->crossat   = -10000;
  wpt->gear_down = true;
  wpt->flaps_down= true;
  wpt->finished  = false;
  wpt->on_ground = true;
  waypoints.push_back(wpt); 
  geo_direct_wgs_84 ( 0, lat, lon, heading, 
 		      0.1 *radius,           
 		      &lat2, &lon2, &az2 );
  wpt = new waypoint;
  wpt->name      = "taxiStart";
  wpt->latitude  = lat2;
  wpt->longitude = lon2;
  wpt->altitude  = apt->getElevation();
  wpt->speed     = 10; 
  wpt->crossat   = -10000;
  wpt->gear_down = true;
  wpt->flaps_down= true;
  wpt->finished  = false;
  wpt->on_ground = true;
  waypoints.push_back(wpt);   

  wpt = new waypoint;
  wpt->name      = "END"; //wpt_node->getStringValue("name", "END");
  wpt->latitude  = lat;
  wpt->longitude = lon;
  wpt->altitude  = apt->getElevation();
  wpt->speed     = 15; 
  wpt->crossat   = -10000;
  wpt->gear_down = true;
  wpt->flaps_down= true;
  wpt->finished  = false;
  wpt->on_ground = true;
  waypoints.push_back(wpt);
}
