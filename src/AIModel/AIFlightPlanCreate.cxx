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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "AIFlightPlan.hxx"
#include <simgear/math/sg_geodesy.hxx>
#include <Airports/runways.hxx>
#include <Airports/dynamics.hxx>
#include "AIAircraft.hxx"
#include "performancedata.hxx"

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
void FGAIFlightPlan::create(FGAIAircraft *ac, FGAirport *dep, FGAirport *arr, int legNr, 
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
      createTakeoffTaxi(firstFlight, dep, radius, fltType, aircraftType, airline);
      break;
    case 3: 
      createTakeOff(ac, firstFlight, dep, speed, fltType);
      break;
    case 4: 
      createClimb(firstFlight, dep, speed, alt, fltType);
      break;
    case 5: 
      createCruise(firstFlight, dep,arr, latitude, longitude, speed, alt, fltType);
      break;
    case 6: 
      createDecent(arr, fltType);
      break;
    case 7: 
      createLanding(arr);
      break;
    case 8: 
      createLandingTaxi(arr, radius, fltType, aircraftType, airline);
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

FGAIFlightPlan::waypoint*
FGAIFlightPlan::createOnGround(const std::string& aName, const SGGeod& aPos, double aElev, double aSpeed)
{
  waypoint* wpt = new waypoint;
  wpt->name = aName;
  wpt->longitude = aPos.getLongitudeDeg();
  wpt->latitude = aPos.getLatitudeDeg();
  wpt->altitude  = aElev;
  wpt->speed     = aSpeed; 
  wpt->crossat   = -10000;
  wpt->gear_down = true;
  wpt->flaps_down= true;
  wpt->finished  = false;
  wpt->on_ground = true;
  wpt->routeIndex= 0;
  return wpt;
}

FGAIFlightPlan::waypoint*
FGAIFlightPlan::createInAir(const std::string& aName, const SGGeod& aPos, double aElev, double aSpeed)
{
  waypoint* wpt = new waypoint;
  wpt->name = aName;
  wpt->longitude = aPos.getLongitudeDeg();
  wpt->latitude = aPos.getLatitudeDeg();
  wpt->altitude  = aElev;
  wpt->speed     = aSpeed; 
  wpt->crossat   = -10000;
  wpt->gear_down = false;
  wpt->flaps_down= false;
  wpt->finished  = false;
  wpt->on_ground = false;
  wpt->routeIndex= 0;
  return wpt;
}

FGAIFlightPlan::waypoint*
FGAIFlightPlan::cloneWithPos(waypoint* aWpt, const std::string& aName, const SGGeod& aPos)
{
  waypoint* wpt = new waypoint;
  wpt->name = aName;
  wpt->longitude = aPos.getLongitudeDeg();
  wpt->latitude = aPos.getLatitudeDeg();
  
  wpt->altitude  = aWpt->altitude;
  wpt->speed     = aWpt->speed; 
  wpt->crossat   = aWpt->crossat;
  wpt->gear_down = aWpt->gear_down;
  wpt->flaps_down= aWpt->flaps_down;
  wpt->finished  = aWpt->finished;
  wpt->on_ground = aWpt->on_ground;
  wpt->routeIndex = 0;
  
  return wpt;
}

void FGAIFlightPlan::createDefaultTakeoffTaxi(FGAirport* aAirport, FGRunway* aRunway)
{
  SGGeod runwayTakeoff = aRunway->pointOnCenterline(5.0);
  double airportElev = aAirport->getElevation();
  
  waypoint* wpt;
  wpt = createOnGround("Airport Center", aAirport->geod(), airportElev, 15);
  waypoints.push_back(wpt);
  wpt = createOnGround("Runway Takeoff", runwayTakeoff, airportElev, 15);
  waypoints.push_back(wpt);	
}

void FGAIFlightPlan::createTakeoffTaxi(bool firstFlight, 
				FGAirport *apt,
				double radius, const string& fltType, 
				const string& acType, const string& airline)
{
  double heading, lat, lon;
  
  // If this function is called during initialization,
  // make sure we obtain a valid gate ID first
  // and place the model at the location of the gate.
  if (firstFlight) {
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
  }
  
  string rwyClass = getRunwayClassFromTrafficType(fltType);
  apt->getDynamics()->getActiveRunway(rwyClass, 1, activeRunway);
  rwy = apt->getRunwayByIdent(activeRunway);
  SGGeod runwayTakeoff = rwy->pointOnCenterline(5.0);

  FGGroundNetwork* gn = apt->getDynamics()->getGroundNetwork();
  if (!gn->exists()) {
    createDefaultTakeoffTaxi(apt, rwy);
    return;
  }
  
  intVec ids;
  int runwayId = gn->findNearestNode(runwayTakeoff);

  // A negative gateId indicates an overflow parking, use a
  // fallback mechanism for this. 
  // Starting from gate 0 in this case is a bit of a hack
  // which requires a more proper solution later on.
  delete taxiRoute;
  taxiRoute = new FGTaxiRoute;

  // Determine which node to start from.
  int node = 0;
  // Find out which node to start from
  FGParking *park = apt->getDynamics()->getParking(gateId);
  if (park) {
    node = park->getPushBackPoint();
  }
  
  if (node == -1) {
    node = gateId;
  }
  
  // HAndle case where parking doens't have a node
  if ((node == 0) && park) {
    if (firstFlight) {
      node = gateId;
    } else {
      node = lastNodeVisited;
    }
  }

  *taxiRoute = gn->findShortestRoute(node, runwayId);
  intVecIterator i;
	 
  if (taxiRoute->empty()) {
    createDefaultTakeoffTaxi(apt, rwy);
    return;
  }
  
  taxiRoute->first();
  //bool isPushBackPoint = false;
  if (firstFlight) {
    // If this is called during initialization, randomly
    // skip a number of waypoints to get a more realistic
    // taxi situation.
    int nrWaypointsToSkip = rand() % taxiRoute->size();
    // but make sure we always keep two active waypoints
    // to prevent a segmentation fault
    for (int i = 0; i < nrWaypointsToSkip-2; i++) {
      taxiRoute->next(&node);
    }
    apt->getDynamics()->releaseParking(gateId);
  } else {
    if (taxiRoute->size() > 1) {
      taxiRoute->next(&node); // chop off the first waypoint, because that is already the last of the pushback route
    }
  }
  
  // push each node on the taxi route as a waypoint
  int route;
  while(taxiRoute->next(&node, &route)) {
		char buffer[10];
		snprintf (buffer, 10, "%d", node);
		FGTaxiNode *tn = apt->getDynamics()->getGroundNetwork()->findNode(node);
    waypoint* wpt = createOnGround(buffer, tn->geod(), apt->getElevation(), 15);
    wpt->routeIndex = route;
		waypoints.push_back(wpt);
  }
}

void FGAIFlightPlan::createDefaultLandingTaxi(FGAirport* aAirport)
{
  SGGeod lastWptPos = 
    SGGeod::fromDeg(waypoints.back()->longitude, waypoints.back()->latitude);
  double airportElev = aAirport->getElevation();
  
  waypoint* wpt;
  wpt = createOnGround("Runway Exit", lastWptPos, airportElev, 15);
  waypoints.push_back(wpt);	
  wpt = createOnGround("Airport Center", aAirport->geod(), airportElev, 15);
  waypoints.push_back(wpt);
  
  double heading, lat, lon;
  aAirport->getDynamics()->getParking(gateId, &lat, &lon, &heading);
  wpt = createOnGround("END", SGGeod::fromDeg(lon, lat), airportElev, 15);
  waypoints.push_back(wpt);
}

void FGAIFlightPlan::createLandingTaxi(FGAirport *apt,
				double radius, const string& fltType, 
				const string& acType, const string& airline)
{
  double heading, lat, lon;
  apt->getDynamics()->getAvailableParking(&lat, &lon, &heading, 
        &gateId, radius, fltType, acType, airline);
  
  SGGeod lastWptPos = 
    SGGeod::fromDeg(waypoints.back()->longitude, waypoints.back()->latitude);
  FGGroundNetwork* gn = apt->getDynamics()->getGroundNetwork();
  
   // Find a route from runway end to parking/gate.
  if (!gn->exists()) {
    createDefaultLandingTaxi(apt);
    return;
  }
  
	intVec ids;
  int runwayId = gn->findNearestNode(lastWptPos);
  // A negative gateId indicates an overflow parking, use a
  // fallback mechanism for this. 
  // Starting from gate 0 is a bit of a hack...
  //FGTaxiRoute route;
  delete taxiRoute;
  taxiRoute = new FGTaxiRoute;
  if (gateId >= 0)
    *taxiRoute = gn->findShortestRoute(runwayId, gateId);
  else
    *taxiRoute = gn->findShortestRoute(runwayId, 0);
  intVecIterator i;
  
  if (taxiRoute->empty()) {
    createDefaultLandingTaxi(apt);
    return;
  }
  
  int node;
  taxiRoute->first();
  int size = taxiRoute->size();
  // Omit the last two waypoints, as 
  // those are created by createParking()
  int route;
  for (int i = 0; i < size-2; i++) {
    taxiRoute->next(&node, &route);
    char buffer[10];
    snprintf (buffer, 10, "%d", node);
    FGTaxiNode *tn = gn->findNode(node);
    waypoint* wpt = createOnGround(buffer, tn->geod(), apt->getElevation(), 15);
    wpt->routeIndex = route;
    waypoints.push_back(wpt);
  }
}

/*******************************************************************
 * CreateTakeOff 
 * initialize the Aircraft at the parking location
 ******************************************************************/
void FGAIFlightPlan::createTakeOff(FGAIAircraft *ac, bool firstFlight, FGAirport *apt, double speed, const string &fltType)
{
    double accel   = ac->getPerformance()->acceleration();
    double vRotate = ac->getPerformance()->vRotate();
    // Acceleration = dV / dT
    // Acceleration X dT = dV
    // dT = dT / Acceleration
    //d = (Vf^2 - Vo^2) / (2*a)
    double accelTime = (vRotate - 15) / accel;
    //cerr << "Using " << accelTime << " as total acceleration time" << endl;
    double accelDistance = (vRotate*vRotate - 15*15) / (2*accel);
    //cerr << "Using " << accelDistance << " " << accel << " " << vRotate << endl;
    waypoint *wpt;
    // Get the current active runway, based on code from David Luff
    // This should actually be unified and extended to include 
    // Preferential runway use schema's 
    // NOTE: DT (2009-01-18: IIRC, this is currently already the case, 
    // because the getActive runway function takes care of that.
    if (firstFlight)
    {
        string rwyClass = getRunwayClassFromTrafficType(fltType);
        apt->getDynamics()->getActiveRunway(rwyClass, 1, activeRunway);
        rwy = apt->getRunwayByIdent(activeRunway);
    }

    double airportElev = apt->getElevation();
    // Acceleration point, 105 meters into the runway,
    SGGeod accelPoint = rwy->pointOnCenterline(105.0);
    wpt = createOnGround("accel", accelPoint, airportElev, speed);
    waypoints.push_back(wpt); 
 
    //Start Climbing to 3000 ft. Let's do this 
    // at the center of the runway for now:
    SGGeod rotate = rwy->pointOnCenterline(105.0+accelDistance);
    wpt = cloneWithPos(wpt, "SOC", rotate);
    wpt->altitude  = airportElev+1000;
    wpt->on_ground = false;
    waypoints.push_back(wpt);

    wpt = cloneWithPos(wpt, "3000 ft", rwy->end());
    wpt->altitude  = airportElev+3000;
    waypoints.push_back(wpt);

    // Finally, add two more waypoints, so that aircraft will remain under
    // Tower control until they have reached the 3000 ft climb point
    SGGeod pt = rwy->pointOnCenterline(5000 + rwy->lengthM() * 0.5);
    wpt = cloneWithPos(wpt, "5000 ft", pt);
    wpt->altitude  = airportElev+5000;
    waypoints.push_back(wpt);
}
  
/*******************************************************************
 * CreateClimb
 * initialize the Aircraft at the parking location
 ******************************************************************/
void FGAIFlightPlan::createClimb(bool firstFlight, FGAirport *apt, double speed, double alt, const string &fltType)
{
  waypoint *wpt;

  if (firstFlight) {
    string rwyClass = getRunwayClassFromTrafficType(fltType);
    apt->getDynamics()->getActiveRunway(rwyClass, 1, activeRunway);
    rwy = apt->getRunwayByIdent(activeRunway);
  }
  
  SGGeod climb1 = rwy->pointOnCenterline(10*SG_NM_TO_METER);
  wpt = createInAir("10000ft climb", climb1, speed, 10000);
  wpt->gear_down = true;
  wpt->flaps_down= true;
  waypoints.push_back(wpt); 

  SGGeod climb2 = rwy->pointOnCenterline(20*SG_NM_TO_METER);
  wpt = cloneWithPos(wpt, "18000ft climb", climb2);
  wpt->altitude  = 18000;
  waypoints.push_back(wpt); 
}


/*******************************************************************
 * CreateDecent
 * initialize the Aircraft at the parking location
 ******************************************************************/
void FGAIFlightPlan::createDecent(FGAirport *apt, const string &fltType)
{
  // Ten thousand ft. Slowing down to 240 kts
  waypoint *wpt;

  //Beginning of Decent
  //string name;
  // allow "mil" and "gen" as well
  string rwyClass = getRunwayClassFromTrafficType(fltType);
  apt->getDynamics()->getActiveRunway(rwyClass, 2, activeRunway);
  rwy = apt->getRunwayByIdent(activeRunway);
     
  SGGeod descent1 = rwy->pointOnCenterline(-100000); // 100km out
  wpt = createInAir("Dec 10000ft", descent1, apt->getElevation(), 240);
  wpt->crossat   = 10000;
  waypoints.push_back(wpt);  
  
  // Three thousand ft. Slowing down to 160 kts
  SGGeod descent2 = rwy->pointOnCenterline(-8*SG_NM_TO_METER); // 8nm out
  wpt = createInAir("DEC 3000ft", descent2, apt->getElevation(), 160);
  wpt->crossat   = 3000;
  wpt->gear_down = true;
  wpt->flaps_down= true;
  waypoints.push_back(wpt);
}
/*******************************************************************
 * CreateLanding
 * initialize the Aircraft at the parking location
 ******************************************************************/
void FGAIFlightPlan::createLanding(FGAirport *apt)
{
  // Ten thousand ft. Slowing down to 150 kts
  waypoint *wpt;
  double aptElev = apt->getElevation();
  //Runway Threshold
  wpt = createOnGround("Threshold", rwy->threshold(), aptElev, 150);
  wpt->crossat = apt->getElevation();
  waypoints.push_back(wpt); 

 // Roll-out
  wpt = createOnGround("Center", rwy->geod(), aptElev, 30);
  waypoints.push_back(wpt);

  SGGeod rollOut = rwy->pointOnCenterline(rwy->lengthM() * 0.9);
  wpt = createOnGround("Roll Out", rollOut, aptElev, 15);
  wpt->crossat   = apt->getElevation();
  waypoints.push_back(wpt); 
}

/*******************************************************************
 * CreateParking
 * initialize the Aircraft at the parking location
 ******************************************************************/
void FGAIFlightPlan::createParking(FGAirport *apt, double radius)
{
  waypoint* wpt;
  double aptElev = apt->getElevation();
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
  wpt = createOnGround("taxiStart", SGGeod::fromDeg(lon2, lat2), aptElev, 10);
  waypoints.push_back(wpt);
  
  geo_direct_wgs_84 ( 0, lat, lon, heading, 
 		      0.1 *radius,           
 		      &lat2, &lon2, &az2 );
          
  wpt = createOnGround("taxiStart2", SGGeod::fromDeg(lon2, lat2), aptElev, 10);
  waypoints.push_back(wpt);   

  wpt = createOnGround("END", SGGeod::fromDeg(lon, lat), aptElev, 10);
  waypoints.push_back(wpt);
}

/**
 *
 * @param fltType a string describing the type of
 * traffic, normally used for gate assignments
 * @return a converted string that gives the runway
 * preference schedule to be used at aircraft having
 * a preferential runway schedule implemented (i.e.
 * having a rwyprefs.xml file
 * 
 * Currently valid traffic types for gate assignment:
 * - gate (commercial gate)
 * - cargo (commercial gargo),
 * - ga (general aviation) ,
 * - ul (ultralight),
 * - mil-fighter (military - fighter),
 * - mil-transport (military - transport)
 *
 * Valid runway classes:
 * - com (commercial traffic: jetliners, passenger and cargo)
 * - gen (general aviation)
 * - ul (ultralight: I can imagine that these may share a runway with ga on some airports)
 * - mil (all military traffic)
 */
string FGAIFlightPlan::getRunwayClassFromTrafficType(string fltType)
{
    if ((fltType == "gate") || (fltType == "cargo")) { 
	return string("com");
    }
    if (fltType == "ga") {
        return string ("gen");
    }
    if (fltType == "ul") {
        return string("ul");
    }
    if ((fltType == "mil-fighter") || (fltType == "mil-transport")) { 
	return string("mil");
    }
   return string("com");
}
