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

#include <algorithm>

#include <cstdlib>
#include <cstdio>

#include "AIFlightPlan.hxx"
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/timing/sg_time.hxx>

#include <Airports/airport.hxx>
#include <Airports/runways.hxx>
#include <Airports/dynamics.hxx>
#include <Airports/groundnetwork.hxx>

#include "AIAircraft.hxx"
#include "performancedata.hxx"

#include <Main/fg_props.hxx>
#include <Environment/environment_mgr.hxx>
#include <Environment/environment.hxx>
#include <FDM/LaRCsim/basic_aero.h>
#include <Navaids/navrecord.hxx>
#include <Traffic/Schedule.hxx>

using std::string;

/* FGAIFlightPlan::create()
 * dynamically create a flight plan for AI traffic, based on data provided by the
 * Traffic Manager, when reading a filed flightplan fails. (DT, 2004/07/10) 
 *
 * This is the top-level function, and the only one that is publicly available.
 *
 */


// Check lat/lon values during initialization;
bool FGAIFlightPlan::create(FGAIAircraft * ac, FGAirport * dep,
                            FGAirport * arr, int legNr, double alt,
                            double speed, double latitude,
                            double longitude, bool firstFlight,
                            double radius, const string & fltType,
                            const string & aircraftType,
                            const string & airline, double distance)
{
    bool retVal = true;
    int currWpt = wpt_iterator - waypoints.begin();
    switch (legNr) {
    case 1:
        retVal = createPushBack(ac, firstFlight, dep,
                                radius, fltType, aircraftType, airline);
        // Pregenerate the taxi leg.
        //if (retVal) {
        //    waypoints.back()->setName( waypoints.back()->getName() + string("legend")); 
        //    retVal = createTakeoffTaxi(ac, false, dep, radius, fltType, aircraftType, airline);
        //}
        break;
    case 2:
        retVal =  createTakeoffTaxi(ac, firstFlight, dep, radius, fltType,
                          aircraftType, airline);
        break;
    case 3:
        retVal = createTakeOff(ac, firstFlight, dep, speed, fltType);
        break;
    case 4:
        retVal = createClimb(ac, firstFlight, dep, arr, speed, alt, fltType);
        break;
    case 5:
        retVal = createCruise(ac, firstFlight, dep, arr, latitude, longitude, speed,
                     alt, fltType);
        break;
    case 6:
        retVal = createDescent(ac, arr, latitude, longitude, speed, alt, fltType,
                      distance);
        break;
    case 7:
        retVal = createLanding(ac, arr, fltType);
        break;
    case 8:
        retVal = createLandingTaxi(ac, arr, radius, fltType, aircraftType, airline);
        break;
    case 9:
        retVal = createParking(ac, arr, radius);
        break;
    default:
        //exit(1);
        SG_LOG(SG_AI, SG_ALERT,
               "AIFlightPlan::create() attempting to create unknown leg"
               " this is probably an internal program error");
        break;
    }
    wpt_iterator = waypoints.begin() + currWpt;
    //don't  increment leg right away, but only once we pass the actual last waypoint that was created.
    // to do so, mark the last waypoint with a special status flag
   if (retVal) {
        waypoints.back()->setName( waypoints.back()->getName() + string("legend")); 
        // "It's pronounced Leg-end" (Roger Glover (Deep Purple): come Hell or High Water DvD, 1993)
   }


    //leg++;
    return retVal;
}

FGAIWaypoint * FGAIFlightPlan::createOnGround(FGAIAircraft * ac,
                                   const std::string & aName,
                                   const SGGeod & aPos, double aElev,
                                   double aSpeed)
{
    FGAIWaypoint *wpt  = new FGAIWaypoint;
    wpt->setName         (aName                  );
    wpt->setLongitude    (aPos.getLongitudeDeg() );
    wpt->setLatitude     (aPos.getLatitudeDeg()  );
    wpt->setAltitude     (aElev                  );
    wpt->setSpeed        (aSpeed                 );
    wpt->setCrossat      (-10000.1               );
    wpt->setGear_down    (true                   );
    wpt->setFlaps        (0.0f                   );
    wpt->setSpoilers     (0.0f                   );
    wpt->setSpeedBrakes  (0.0f                   );
    wpt->setFinished     (false                  );
    wpt->setOn_ground    (true                   );
    wpt->setRouteIndex   (0                      );
    if (aSpeed > 0.0f) {
        wpt->setGroundLights();
    } else {
        wpt->setPowerDownLights();
    }
    return wpt;
}

FGAIWaypoint * FGAIFlightPlan::createOnRunway(FGAIAircraft * ac,
                                   const std::string & aName,
                                   const SGGeod & aPos, double aElev,
                                   double aSpeed)
{
    FGAIWaypoint * wpt = createOnGround(ac, aName, aPos, aElev, aSpeed);
    wpt->setTakeOffLights();
    return wpt;
}

FGAIWaypoint *    FGAIFlightPlan::createInAir(FGAIAircraft * ac,
                                const std::string & aName,
                                const SGGeod & aPos, double aElev,
                                double aSpeed)
{
    FGAIWaypoint * wpt = createOnGround(ac, aName, aPos, aElev, aSpeed);
    wpt->setGear_down  (false                   );
    wpt->setFlaps      (0.0f                    );
    wpt->setSpoilers   (0.0f                    );
    wpt->setSpeedBrakes(0.0f                    );
    wpt->setOn_ground  (false                   );
    wpt->setCrossat    (aElev                   );
    
    if (aPos.getElevationFt() < 10000.0f) {
        wpt->setApproachLights();
    } else {
        wpt->setCruiseLights();
    }
    return wpt;
}

FGAIWaypoint * FGAIFlightPlan::clone(FGAIWaypoint * aWpt)
{
    FGAIWaypoint *wpt = new FGAIWaypoint;
    wpt->setName       ( aWpt->getName ()      );
    wpt->setLongitude  ( aWpt->getLongitude()  );
    wpt->setLatitude   ( aWpt->getLatitude()   );
    wpt->setAltitude   ( aWpt->getAltitude()   );
    wpt->setSpeed      ( aWpt->getSpeed()      );
    wpt->setCrossat    ( aWpt->getCrossat()    );
    wpt->setGear_down  ( aWpt->getGear_down()  );
    wpt->setFlaps      ( aWpt->getFlaps()      );
    wpt->setFinished   ( aWpt->isFinished()    );
    wpt->setOn_ground  ( aWpt->getOn_ground()  );
    wpt->setLandingLight (aWpt->getLandingLight() );
    wpt->setNavLight     (aWpt->getNavLight()  );  
    wpt->setStrobeLight  (aWpt->getStrobeLight() );
    wpt->setTaxiLight    (aWpt->getTaxiLight() ); 
    wpt->setRouteIndex ( 0                     );

    return wpt;
}


FGAIWaypoint * FGAIFlightPlan::cloneWithPos(FGAIAircraft * ac, FGAIWaypoint * aWpt,
                                 const std::string & aName,
                                 const SGGeod & aPos)
{
    FGAIWaypoint *wpt = clone(aWpt);
    wpt->setName       ( aName                   );
    wpt->setLongitude  ( aPos.getLongitudeDeg () );
    wpt->setLatitude   ( aPos.getLatitudeDeg  () );

    return wpt;
}



void FGAIFlightPlan::createDefaultTakeoffTaxi(FGAIAircraft * ac,
                                              FGAirport * aAirport,
                                              FGRunway * aRunway)
{
    SGGeod runwayTakeoff = aRunway->pointOnCenterline(5.0);
    double airportElev = aAirport->getElevation();

    FGAIWaypoint *wpt;
    wpt =
        createOnGround(ac, "Airport Center", aAirport->geod(), airportElev,
                       ac->getPerformance()->vTaxi());
    pushBackWaypoint(wpt);
    wpt =
        createOnRunway(ac, "Runway Takeoff", runwayTakeoff, airportElev,
                       ac->getPerformance()->vTaxi());
    wpt->setFlaps(0.5f);
    pushBackWaypoint(wpt);

    // Acceleration point, 105 meters into the runway,
    SGGeod accelPoint = aRunway->pointOnCenterline(105.0);
    wpt = createOnRunway(ac, "Accel", accelPoint, airportElev,
                         ac->getPerformance()->vRotate());
    pushBackWaypoint(wpt);
}

bool FGAIFlightPlan::createTakeoffTaxi(FGAIAircraft * ac, bool firstFlight,
                                       FGAirport * apt,
                                       double radius,
                                       const string & fltType,
                                       const string & acType,
                                       const string & airline)
{
    int route;
    // If this function is called during initialization,
    // make sure we obtain a valid gate ID first
    // and place the model at the location of the gate.
    if (firstFlight && apt->getDynamics()->hasParkings()) {
      gate =  apt->getDynamics()->getAvailableParking(radius, fltType,
                                                        acType, airline);
      if (!gate.isValid()) {
        SG_LOG(SG_AI, SG_WARN, "Could not find parking for a " <<
               acType <<
               " of flight type " << fltType <<
               " of airline     " << airline <<
               " at airport     " << apt->getId());
      }
    }

    const string& rwyClass = getRunwayClassFromTrafficType(fltType);

    // Only set this if it hasn't been set by ATC already.
    if (activeRunway.empty()) {
        //cerr << "Getting runway for " << ac->getTrafficRef()->getCallSign() << " at " << apt->getId() << endl;
        double depHeading = ac->getTrafficRef()->getCourse();
        apt->getDynamics()->getActiveRunway(rwyClass, 1, activeRunway,
                                            depHeading);
    }
    FGRunway * rwy = apt->getRunwayByIdent(activeRunway);
    assert( rwy != NULL );
    SGGeod runwayTakeoff = rwy->pointOnCenterline(5.0);

    FGGroundNetwork *gn = apt->groundNetwork();
    if (!gn->exists()) {
        createDefaultTakeoffTaxi(ac, apt, rwy);
        return true;
    }

    FGTaxiNodeRef runwayNode;
    if (gn->getVersion() > 0) {
        runwayNode = gn->findNearestNodeOnRunway(runwayTakeoff);
    } else {
        runwayNode = gn->findNearestNode(runwayTakeoff);
    }

    // A negative gateId indicates an overflow parking, use a
    // fallback mechanism for this. 
    // Starting from gate 0 in this case is a bit of a hack
    // which requires a more proper solution later on.
  //  delete taxiRoute;
  //  taxiRoute = new FGTaxiRoute;

    // Determine which node to start from.
    FGTaxiNodeRef node;
    // Find out which node to start from
    FGParking *park = gate.parking();
    if (park) {
        node = park->getPushBackPoint();
        if (node == 0) {
            // Handle case where parking doesn't have a node
            if (firstFlight) {
                node = park;
            } else {
                node = lastNodeVisited;
            }
        }
    }
    
    FGTaxiRoute taxiRoute;
    if ( runwayNode && node)
        taxiRoute = gn->findShortestRoute(node, runwayNode);

    // This may happen with buggy ground networks
    if (taxiRoute.size() <= 1) {
        createDefaultTakeoffTaxi(ac, apt, rwy);
        return true;
    }

    taxiRoute.first();
    FGTaxiNodeRef skipNode;

    //bool isPushBackPoint = false;
    if (firstFlight) {
        // If this is called during initialization, randomly
        // skip a number of waypoints to get a more realistic
        // taxi situation.
        int nrWaypointsToSkip = rand() % taxiRoute.size();
        // but make sure we always keep two active waypoints
        // to prevent a segmentation fault
        for (int i = 0; i < nrWaypointsToSkip - 3; i++) {
            taxiRoute.next(skipNode, &route);
        }
        
        gate.release(); // free up our gate as required
    } else {
        if (taxiRoute.size() > 1) {
            taxiRoute.next(skipNode, &route);     // chop off the first waypoint, because that is already the last of the pushback route
        }
    }

    // push each node on the taxi route as a waypoint
    
    //cerr << "Building taxi route" << endl;
    
    // Note that the line wpt->setRouteIndex was commented out by revision [afcdbd] 2012-01-01,
    // which breaks the rendering functions. 
    // These can probably be generated on the fly however. 
    while (taxiRoute.next(node, &route)) {
        char buffer[10];
        snprintf(buffer, 10, "%d", node->getIndex());
        FGAIWaypoint *wpt =
            createOnGround(ac, buffer, node->geod(), apt->getElevation(),
                           ac->getPerformance()->vTaxi());
        wpt->setRouteIndex(route);
        //cerr << "Nodes left " << taxiRoute->nodesLeft() << " ";
        if (taxiRoute.nodesLeft() == 1) {
            // Note that we actually have hold points in the ground network, but this is just an initial test.
            //cerr << "Setting departurehold point: " << endl;
            wpt->setName( wpt->getName() + string("DepartureHold"));
            wpt->setFlaps(0.5f);
            wpt->setTakeOffLights();
        }
        if (taxiRoute.nodesLeft() == 0) {
            wpt->setName(wpt->getName() + string("Accel"));
            wpt->setTakeOffLights();
            wpt->setFlaps(0.5f);
        }
        pushBackWaypoint(wpt);
    }
    // Acceleration point, 105 meters into the runway,
    SGGeod accelPoint = rwy->pointOnCenterline(105.0);
    FGAIWaypoint *wpt = createOnRunway(ac, "Accel", accelPoint, apt->getElevation(), ac->getPerformance()->vRotate());
    wpt->setFlaps(0.5f);
    pushBackWaypoint(wpt);

    //cerr << "[done]" << endl;
    return true;
}

void FGAIFlightPlan::createDefaultLandingTaxi(FGAIAircraft * ac,
                                              FGAirport * aAirport)
{
    SGGeod lastWptPos =
        SGGeod::fromDeg(waypoints.back()->getLongitude(),
                        waypoints.back()->getLatitude());
    double airportElev = aAirport->getElevation();

    FGAIWaypoint *wpt;
    wpt =
        createOnGround(ac, "Runway Exit", lastWptPos, airportElev,
                       ac->getPerformance()->vTaxi());
    pushBackWaypoint(wpt);
    wpt =
        createOnGround(ac, "Airport Center", aAirport->geod(), airportElev,
                       ac->getPerformance()->vTaxi());
    pushBackWaypoint(wpt);

    if (gate.isValid()) {
        wpt = createOnGround(ac, "ENDtaxi", gate.parking()->geod(), airportElev,
                         ac->getPerformance()->vTaxi());
        pushBackWaypoint(wpt);
    }
}

bool FGAIFlightPlan::createLandingTaxi(FGAIAircraft * ac, FGAirport * apt,
                                       double radius,
                                       const string & fltType,
                                       const string & acType,
                                       const string & airline)
{
    int route;
    gate = apt->getDynamics()->getAvailableParking(radius, fltType,
                                            acType, airline);

    SGGeod lastWptPos = waypoints.back()->getPos();
    FGGroundNetwork *gn = apt->groundNetwork();

    // Find a route from runway end to parking/gate.
    if (!gn->exists()) {
        createDefaultLandingTaxi(ac, apt);
        return true;
    }

    FGTaxiNodeRef runwayNode;
    if (gn->getVersion() == 1) {
        runwayNode = gn->findNearestNodeOnRunway(lastWptPos);
    } else {
        runwayNode = gn->findNearestNode(lastWptPos);
    }
    //cerr << "Using network node " << runwayId << endl;
    // A negative gateId indicates an overflow parking, use a
    // fallback mechanism for this. 
    // Starting from gate 0 doesn't work, so don't try it
    FGTaxiRoute taxiRoute;
    if (runwayNode && gate.isValid())
        taxiRoute = gn->findShortestRoute(runwayNode, gate.parking());

    if (taxiRoute.empty()) {
        createDefaultLandingTaxi(ac, apt);
        return true;
    }

    FGTaxiNodeRef node;
    taxiRoute.first();
    int size = taxiRoute.size();
    // Omit the last two waypoints, as 
    // those are created by createParking()
   // int route;
    for (int i = 0; i < size - 2; i++) {
        taxiRoute.next(node, &route);
        char buffer[10];
        snprintf(buffer, 10, "%d",  node->getIndex());
        FGAIWaypoint *wpt =
            createOnGround(ac, buffer, node->geod(), apt->getElevation(),
                           ac->getPerformance()->vTaxi());
        
        wpt->setRouteIndex(route);
        pushBackWaypoint(wpt);
    }
    return true;
}

static double accelDistance(double v0, double v1, double accel)
{
  double t = fabs(v1 - v0) / accel; // time in seconds to change velocity
  // area under the v/t graph: (t * v0) + (dV / 2t) where (dV = v1 - v0)
  return t * 0.5 * (v1 + v0); 
}

// find the horizontal distance to gain the specific altiude, holding
// a constant pitch angle. Used to compute distance based on standard FD/AP
// PITCH mode prior to VS or CLIMB engaging. Visually, we want to avoid
// a dip in the nose angle after rotation, during initial climb-out.
static double pitchDistance(double pitchAngleDeg, double altGainM)
{
  return altGainM / tan(pitchAngleDeg * SG_DEGREES_TO_RADIANS);
}

/*******************************************************************
 * CreateTakeOff 
 * A note on units: 
 *  - Speed -> knots -> nm/hour
 *  - distance along runway =-> meters 
 *  - accel / decel -> is given as knots/hour, but this is highly questionable:
 *  for a jet_transport performance class, a accel / decel rate of 5 / 2 is 
 *  given respectively. According to performance data.cxx, a value of kts / second seems
 *  more likely however. 
 * 
 ******************************************************************/
bool FGAIFlightPlan::createTakeOff(FGAIAircraft * ac, bool firstFlight,
                                   FGAirport * apt, double speed,
                                   const string & fltType)
{
    const double ACCEL_POINT = 105.0;
  // climb-out angle in degrees. could move this to the perf-db but this
  // value is pretty sane
    const double INITIAL_PITCH_ANGLE = 10.0;
  
    double accel = ac->getPerformance()->acceleration();
    double vTaxi = ac->getPerformance()->vTaxi();
    double vRotate = ac->getPerformance()->vRotate();
    double vTakeoff = ac->getPerformance()->vTakeoff();

    double accelMetric = accel * SG_KT_TO_MPS;
    double vTaxiMetric = vTaxi * SG_KT_TO_MPS;
    double vRotateMetric = vRotate * SG_KT_TO_MPS;
    double vTakeoffMetric = vTakeoff * SG_KT_TO_MPS;
   
    FGAIWaypoint *wpt;
    // Get the current active runway, based on code from David Luff
    // This should actually be unified and extended to include 
    // Preferential runway use schema's 
    // NOTE: DT (2009-01-18: IIRC, this is currently already the case, 
    // because the getActive runway function takes care of that.
    if (firstFlight) {
        const string& rwyClass = getRunwayClassFromTrafficType(fltType);
        double heading = ac->getTrafficRef()->getCourse();
        apt->getDynamics()->getActiveRunway(rwyClass, 1, activeRunway,
                                            heading);
    }

    // this is Sentry issue FLIGHTGEAR-DS : happens after reposition,
    // likely firstFlight is false, but activeRunway is stale
    if (!apt->hasRunwayWithIdent(activeRunway)) {
        SG_LOG(SG_AI, SG_WARN, "FGAIFlightPlan::createTakeOff: invalid active runway:" << activeRunway);
        return false;
    }
  
    FGRunway * rwy = apt->getRunwayByIdent(activeRunway);
    if (!rwy)
        return false;
    
    double airportElev = apt->getElevation();
    
    // distance from the runway threshold to accelerate to rotation speed.
    double d = accelDistance(vTaxiMetric, vRotateMetric, accelMetric) + ACCEL_POINT;
    SGGeod rotatePoint = rwy->pointOnCenterline(d);
    wpt = createOnRunway(ac, "rotate", rotatePoint, airportElev, vRotate);
    wpt->setFlaps(0.5f);
    pushBackWaypoint(wpt);

    // After rotation, we still need to accelerate to the take-off speed.
    double t  = d + accelDistance(vRotateMetric, vTakeoffMetric, accelMetric);
    SGGeod takeoffPoint = rwy->pointOnCenterline(t);
    wpt = createOnRunway(ac, "takeoff", takeoffPoint, airportElev, vTakeoff);
    wpt->setGear_down(true);
    wpt->setFlaps(0.5f);
    pushBackWaypoint(wpt);

    double vRef = vTakeoff + 20; // climb-out at v2 + 20kts

    // We want gear-up to take place at ~400ft AGL.  However, the flightplan
    // will move onto the next leg once it gets within 2xspeed of the next waypoint.  
    // With closely spaced waypoints on climb-out this can occur almost immediately,
    // so we put the waypoint further away.
    double gearUpDist = t + 2*vRef*SG_FEET_TO_METER + pitchDistance(INITIAL_PITCH_ANGLE, 400 * SG_FEET_TO_METER);
    SGGeod gearUpPoint = rwy->pointOnCenterline(gearUpDist);  
    wpt = createInAir(ac, "gear-up", gearUpPoint, airportElev + 400, vRef);
    wpt->setFlaps(0.5f);
    pushBackWaypoint(wpt);
  
    // limit climbout speed to 240kts below 10000'
    double vClimbBelow10000 = std::min(240.0, ac->getPerformance()->vClimb());
  
    // create two climb-out points. This is important becuase the first climb point will
    // be a (sometimes large) turn towards the destination, and we don't want to
    // commence that turn below 2000'
    double climbOut = t + 2*vClimbBelow10000*SG_FEET_TO_METER + pitchDistance(INITIAL_PITCH_ANGLE, 2000 * SG_FEET_TO_METER);
    SGGeod climbOutPoint = rwy->pointOnCenterline(climbOut);
    wpt = createInAir(ac, "2000'", climbOutPoint, airportElev + 2000, vClimbBelow10000);
    pushBackWaypoint(wpt);
  
    climbOut = t + 2*vClimbBelow10000*SG_FEET_TO_METER + pitchDistance(INITIAL_PITCH_ANGLE, 2500 * SG_FEET_TO_METER);
    SGGeod climbOutPoint2 = rwy->pointOnCenterline(climbOut);
    wpt = createInAir(ac, "2500'", climbOutPoint2, airportElev + 2500, vClimbBelow10000);
    pushBackWaypoint(wpt);

    return true;
}

/*******************************************************************
 * CreateClimb
 * initialize the Aircraft in a climb.
 ******************************************************************/
bool FGAIFlightPlan::createClimb(FGAIAircraft * ac, bool firstFlight,
                                 FGAirport * apt, FGAirport* arrival,
                                 double speed, double alt,
                                 const string & fltType)
{
    FGAIWaypoint *wpt;
  //  string fPLName;
    double vClimb = ac->getPerformance()->vClimb();
  
    if (firstFlight) {
        const string& rwyClass = getRunwayClassFromTrafficType(fltType);
        double heading = ac->getTrafficRef()->getCourse();
        apt->getDynamics()->getActiveRunway(rwyClass, 1, activeRunway,
                                            heading);
    }
    if (sid) {
        for (wpt_vector_iterator i = sid->getFirstWayPoint();
             i != sid->getLastWayPoint(); i++) {
            pushBackWaypoint(clone(*(i)));
            //cerr << " Cloning waypoint " << endl;
        }
    } else {
        if (!apt->hasRunwayWithIdent(activeRunway))
            return false;

        FGRunwayRef runway = apt->getRunwayByIdent(activeRunway);
        SGGeod cur = runway->end();
        if (!waypoints.empty()) {
          cur = waypoints.back()->getPos();
        }
      
      // compute course towards destination
        double course = SGGeodesy::courseDeg(cur, arrival->geod());
      
        SGGeod climb1 = SGGeodesy::direct(cur, course, 10 * SG_NM_TO_METER);
        wpt = createInAir(ac, "10000ft climb", climb1, 10000, vClimb);
        pushBackWaypoint(wpt);

        SGGeod climb2 = SGGeodesy::direct(cur, course, 20 * SG_NM_TO_METER);
        wpt = createInAir(ac, "18000ft climb", climb2, 18000, vClimb);
        pushBackWaypoint(wpt);
    }
    return true;
}

/*******************************************************************
 * CreateDescent
 * Generate a flight path from the last waypoint of the cruise to 
 * the permission to land point
 ******************************************************************/
bool FGAIFlightPlan::createDescent(FGAIAircraft * ac, FGAirport * apt,
                                   double latitude, double longitude,
                                   double speed, double alt,
                                   const string & fltType,
                                   double requiredDistance)
{
    bool reposition = false;
    FGAIWaypoint *wpt;
    double vDescent = ac->getPerformance()->vDescent();
    double vApproach = ac->getPerformance()->vApproach();

    //Beginning of Descent 
    const string& rwyClass = getRunwayClassFromTrafficType(fltType);
    double heading = ac->getTrafficRef()->getCourse();
    apt->getDynamics()->getActiveRunway(rwyClass, 2, activeRunway,
                                        heading);
    if (!apt->hasRunwayWithIdent(activeRunway))
        return false;

    FGRunwayRef rwy = apt->getRunwayByIdent(activeRunway);

    // Create a slow descent path that ends 250 lateral to the runway.
    double initialTurnRadius = getTurnRadius(vDescent, true);
    //double finalTurnRadius = getTurnRadius(vApproach, true);

// get length of the downwind leg for the intended runway
    double distanceOut = apt->getDynamics()->getApproachController()->getRunway(rwy->name())->getApproachDistance();    //12 * SG_NM_TO_METER;
    //time_t previousArrivalTime=  apt->getDynamics()->getApproachController()->getRunway(rwy->name())->getEstApproachTime();


    SGGeod current = SGGeod::fromDegM(longitude, latitude, 0);
    SGGeod initialTarget = rwy->pointOnCenterline(-distanceOut);
    SGGeod refPoint = rwy->pointOnCenterline(0);
    double distance = SGGeodesy::distanceM(current, initialTarget);
    double azimuth = SGGeodesy::courseDeg(current, initialTarget);
    double dummyAz2;

    // To prevent absurdly steep approaches, compute the origin from where the approach should have started
    SGGeod origin;

    if (ac->getTrafficRef()->getCallSign() ==
        fgGetString("/ai/track-callsign")) {
        //cerr << "Reposition information: Actual distance " << distance << ". required distance " << requiredDistance << endl;
        //exit(1);
    }

    if (distance < requiredDistance * 0.8) {
        reposition = true;
        SGGeodesy::direct(initialTarget, azimuth,
                          -requiredDistance, origin, dummyAz2);

        distance = SGGeodesy::distanceM(current, initialTarget);
        azimuth = SGGeodesy::courseDeg(current, initialTarget);
    } else {
        origin = current;
    }

    double dAlt = 0; //  = alt - (apt->getElevation() + 2000);
    FGTaxiNodeRef tn;
    if (apt->groundNetwork()) {
        tn = apt->groundNetwork()->findNearestNode(refPoint);
    }
  
    if (tn) {
        dAlt = alt - ((tn->getElevationFt()) + 2000);
    } else {
        dAlt = alt - (apt->getElevation() + 2000);
    }
  
    double nPoints = 100;
    char buffer[16];

    // The descent path contains the following phases:
    // 1) a linear glide path from the initial position to
    // 2) a semi circle turn to final
    // 3) approach

    //cerr << "Phase 1: Linear Descent path to runway" << rwy->name() << endl;
    // Create an initial destination point on a semicircle
    //cerr << "lateral offset : " << lateralOffset << endl;
    //cerr << "Distance       : " << distance      << endl;
    //cerr << "Azimuth        : " << azimuth       << endl;
    //cerr << "Initial Lateral point: " << lateralOffset << endl;
//    double lat = refPoint.getLatitudeDeg();
//    double lon = refPoint.getLongitudeDeg();
    //cerr << "Reference point (" << lat << ", " << lon << ")." << endl;
//    lat = initialTarget.getLatitudeDeg();
//    lon = initialTarget.getLongitudeDeg();
    //cerr << "Initial Target point (" << lat << ", " << lon << ")." << endl;

    double ratio = initialTurnRadius / distance;
    if (ratio > 1.0)
        ratio = 1.0;
    if (ratio < -1.0)
        ratio = -1.0;

    double newHeading = asin(ratio) * SG_RADIANS_TO_DEGREES;
    double newDistance =
        cos(newHeading * SG_DEGREES_TO_RADIANS) * distance;
    //cerr << "new distance " << newDistance << ". additional Heading " << newHeading << endl;
    double side = azimuth - rwy->headingDeg();
    double lateralOffset = initialTurnRadius;
    if (side < 0)
        side += 360;
    if (side < 180) {
        lateralOffset *= -1;
    }
    // Calculate the ETA at final, based on remaining distance, and approach speed.
    // distance should really consist of flying time to terniary target, plus circle 
    // but the distance to secondary target should work as a reasonable approximation
    // aditionally add the amount of distance covered by making a turn of "side"
    double turnDistance = (2 * M_PI * initialTurnRadius) * (side / 360.0);
    time_t remaining =
        (turnDistance + distance) / ((vDescent * SG_NM_TO_METER) / 3600.0);
    time_t now = globals->get_time_params()->get_cur_time();

    //if (ac->getTrafficRef()->getCallSign() == fgGetString("/ai/track-callsign")) {
    //     cerr << "   Arrival time estimation: turn angle " <<  side << ". Turn distance " << turnDistance << ". Linear distance " << distance << ". Time to go " << remaining << endl;
    //     //exit(1);
    //}

    time_t eta = now + remaining;
    //choose a distance to the runway such that it will take at least 60 seconds more
    // time to get there than the previous aircraft.
    // Don't bother when aircraft need to be repositioned, because that marks the initialization phased...

    time_t newEta;

    if (reposition == false) {
        newEta =
            apt->getDynamics()->getApproachController()->getRunway(rwy->
                                                                   name
                                                                   ())->
            requestTimeSlot(eta);
    } else {
        newEta = eta;
    }
    //if ((eta < (previousArrivalTime+60)) && (reposition == false)) {
    arrivalTime = newEta;
    time_t additionalTimeNeeded = newEta - eta;
    double distanceCovered =
        ((vApproach * SG_NM_TO_METER) / 3600.0) * additionalTimeNeeded;
    distanceOut += distanceCovered;
    //apt->getDynamics()->getApproachController()->getRunway(rwy->name())->setEstApproachTime(eta+additionalTimeNeeded);
    //cerr << "Adding additional distance: " << distanceCovered << " to allow " << additionalTimeNeeded << " seconds of flying time" << endl << endl;
    //} else {
    //apt->getDynamics()->getApproachController()->getRunway(rwy->name())->setEstApproachTime(eta);
    //}
    //cerr << "Timing information : Previous eta: " << previousArrivalTime << ". Current ETA : " << eta << endl;

    SGGeod secondaryTarget =
        rwy->pointOffCenterline(-distanceOut, lateralOffset);
    initialTarget = rwy->pointOnCenterline(-distanceOut);
    distance = SGGeodesy::distanceM(origin, secondaryTarget);
    azimuth = SGGeodesy::courseDeg(origin, secondaryTarget);


//    lat = secondaryTarget.getLatitudeDeg();
//    lon = secondaryTarget.getLongitudeDeg();
    //cerr << "Secondary Target point (" << lat << ", " << lon << ")." << endl;
    //cerr << "Distance       : " << distance      << endl;
    //cerr << "Azimuth        : " << azimuth       << endl;


    ratio = initialTurnRadius / distance;
    if (ratio > 1.0)
        ratio = 1.0;
    if (ratio < -1.0)
        ratio = -1.0;
    newHeading = asin(ratio) * SG_RADIANS_TO_DEGREES;
    newDistance = cos(newHeading * SG_DEGREES_TO_RADIANS) * distance;
    //cerr << "new distance realative to secondary target: " << newDistance << ". additional Heading " << newHeading << endl;
    if (side < 180) {
        azimuth += newHeading;
    } else {
        azimuth -= newHeading;
    }

    SGGeod tertiaryTarget;
    SGGeodesy::direct(origin, azimuth,
                      newDistance, tertiaryTarget, dummyAz2);

//    lat = tertiaryTarget.getLatitudeDeg();
//    lon = tertiaryTarget.getLongitudeDeg();
    //cerr << "tertiary Target point (" << lat << ", " << lon << ")." << endl;


    for (int i = 1; i < nPoints; i++) {
        SGGeod result;
        double currentDist = i * (newDistance / nPoints);
        double currentAltitude = alt - (i * (dAlt / nPoints));
        SGGeodesy::direct(origin, azimuth, currentDist, result, dummyAz2);
        snprintf(buffer, 16, "descent%03d", i);
        wpt = createInAir(ac, buffer, result, currentAltitude, vDescent);
        wpt->setCrossat(currentAltitude);
        wpt->setTrackLength((newDistance / nPoints));
        pushBackWaypoint(wpt);
        //cerr << "Track Length : " << wpt->trackLength;
        //cerr << "  Position : " << result.getLatitudeDeg() << " " << result.getLongitudeDeg() << " " << currentAltitude << endl;
    }

    //cerr << "Phase 2: Circle " << endl;
    double initialAzimuth =
        SGGeodesy::courseDeg(secondaryTarget, tertiaryTarget);
    double finalAzimuth =
        SGGeodesy::courseDeg(secondaryTarget, initialTarget);

    //cerr << "Angles from secondary target: " << initialAzimuth << " " << finalAzimuth << endl;
    int increment, startval, endval;
    // circle right around secondary target if orig of position is to the right of the runway
    // i.e. use negative angles; else circle leftward and use postivi
    if (side < 180) {
        increment = -1;
        startval = floor(initialAzimuth);
        endval = ceil(finalAzimuth);
        if (endval > startval) {
            endval -= 360;
        }
    } else {
        increment = 1;
        startval = ceil(initialAzimuth);
        endval = floor(finalAzimuth);
        if (endval < startval) {
            endval += 360;
        }

    }

    //cerr << "creating circle between " << startval << " and " << endval << " using " << increment << endl;
    //FGTaxiNode * tn = apt->getDynamics()->getGroundNetwork()->findNearestNode(initialTarget);
    double currentAltitude = 0;
    if (tn) {
        currentAltitude = (tn->getElevationFt()) + 2000;
    } else {
        currentAltitude = apt->getElevation() + 2000;
    }
        
    double trackLength = (2 * M_PI * initialTurnRadius) / 360.0;
    for (int i = startval; i != endval; i += increment) {
        SGGeod result;
        //double currentAltitude = apt->getElevation() + 2000;
        
        SGGeodesy::direct(secondaryTarget, i,
                          initialTurnRadius, result, dummyAz2);
        snprintf(buffer, 16, "turn%03d", i);
        wpt = createInAir(ac, buffer, result, currentAltitude, vDescent);
        wpt->setCrossat(currentAltitude);
        wpt->setTrackLength(trackLength);
        //cerr << "Track Length : " << wpt->trackLength;
        pushBackWaypoint(wpt);
        //cerr << "  Position : " << result.getLatitudeDeg() << " " << result.getLongitudeDeg() << " " << currentAltitude << endl;
    }


    // The approach leg should bring the aircraft to approximately 4-6 nm out, after which the landing phase should take over. 
    //cerr << "Phase 3: Approach" << endl;
    
    //cerr << "Done" << endl;

    // Erase the two bogus BOD points: Note check for conflicts with scripted AI flightPlans
    IncrementWaypoint(true);
    IncrementWaypoint(true);

    if (reposition) {
        double tempDistance;
        //double minDistance = HUGE_VAL;
        //string wptName;
        tempDistance = SGGeodesy::distanceM(current, initialTarget);
        time_t eta =
            tempDistance / ((vDescent * SG_NM_TO_METER) / 3600.0) + now;
        time_t newEta =
            apt->getDynamics()->getApproachController()->getRunway(rwy->
                                                                   name
                                                                   ())->
            requestTimeSlot(eta);
        arrivalTime = newEta;
        double newDistance =
            ((vDescent * SG_NM_TO_METER) / 3600.0) * (newEta - now);
        //cerr << "Repositioning information : eta" << eta << ". New ETA " << newEta << ". Diff = " << (newEta - eta) << ". Distance = " << tempDistance << ". New distance = " << newDistance << endl;
        IncrementWaypoint(true);        // remove waypoint BOD2
        while (checkTrackLength("final001") > newDistance) {
            IncrementWaypoint(true);
        }
        //cerr << "Repositioning to waypoint " << (*waypoints.begin())->name << endl;
        ac->resetPositionFromFlightPlan();
    }
    waypoints[1]->setName( (waypoints[1]->getName() + string("legend"))); 
    return true;
}

/**
 * compute the distance along the centerline, to the ILS glideslope
 * transmitter. Return -1 if there's no GS for the runway
 */
static double runwayGlideslopeTouchdownDistance(FGRunway* rwy)
{
  FGNavRecord* gs = rwy->glideslope();
  if (!gs) {
    return -1;
  }
  
  SGVec3d runwayPosCart = SGVec3d::fromGeod(rwy->pointOnCenterline(0.0));
  // compute a unit vector in ECF cartesian space, from the runway beginning to the end
  SGVec3d runwayDirectionVec = normalize(SGVec3d::fromGeod(rwy->end()) - runwayPosCart);
  SGVec3d gsTransmitterVec = gs->cart() - runwayPosCart;
  
// project the gsTransmitterVec along the runwayDirctionVec to get out
// final value (in metres)
  double dist = dot(runwayDirectionVec, gsTransmitterVec);
  return dist;
}

/*******************************************************************
 * CreateLanding
 * Create a flight path from the "permision to land" point (currently
   hardcoded at 5000 meters from the threshold) to the threshold, at
   a standard glide slope angle of 3 degrees. 
   Position : 50.0354 8.52592 384 364 11112
 ******************************************************************/
bool FGAIFlightPlan::createLanding(FGAIAircraft * ac, FGAirport * apt,
                                   const string & fltType)
{
    double vTouchdown = ac->getPerformance()->vTouchdown();
    double vTaxi      = ac->getPerformance()->vTaxi();
    double decel     = ac->getPerformance()->decelerationOnGround();
    double vApproach = ac->getPerformance()->vApproach();
  
    double vTouchdownMetric = vTouchdown  * SG_KT_TO_MPS;
    double vTaxiMetric      = vTaxi       * SG_KT_TO_MPS;
    double decelMetric      = decel       * SG_KT_TO_MPS;

    char buffer[12];
    FGRunway * rwy = apt->getRunwayByIdent(activeRunway);
    if (!rwy)
        return false;
    
    SGGeod threshold = rwy->threshold();
    double currElev = threshold.getElevationFt();
  
    double touchdownDistance = runwayGlideslopeTouchdownDistance(rwy);
    if (touchdownDistance < 0.0) {
      double landingLength = rwy->lengthM() - (rwy->displacedThresholdM());
      // touchdown 25% of the way along the landing area
      touchdownDistance = rwy->displacedThresholdM() + (landingLength * 0.25);
    }
  
    SGGeod coord;
  // find glideslope entry point, 2000' above touchdown elevation
    double glideslopeEntry = -((2000 * SG_FEET_TO_METER) / tan(3.0)) + touchdownDistance;
    FGAIWaypoint *wpt = createInAir(ac, "Glideslope begin", rwy->pointOnCenterline(glideslopeEntry),
                                  currElev + 2000, vApproach);
    wpt->setGear_down(true);
    wpt->setFlaps(1.0f);
    wpt->setSpeedBrakes(1.0f);
    pushBackWaypoint(wpt);
  
  // deceleration point, 500' above touchdown elevation - slow from approach speed
  // to touchdown speed
    double decelPoint = -((500 * SG_FEET_TO_METER) / tan(3.0)) + touchdownDistance;
    wpt = createInAir(ac, "500' decel", rwy->pointOnCenterline(decelPoint),
                                  currElev + 500, vTouchdown);
    wpt->setGear_down(true);
    wpt->setFlaps(1.0f);
    wpt->setSpeedBrakes(1.0f);
    pushBackWaypoint(wpt);
  
  // compute elevation above the runway start, based on a 3-degree glideslope
    double heightAboveRunwayStart = touchdownDistance *
      tan(3.0 * SG_DEGREES_TO_RADIANS) * SG_METER_TO_FEET;
    wpt = createInAir(ac, "CrossThreshold", rwy->begin(),
                      heightAboveRunwayStart + currElev, vTouchdown);
    wpt->setGear_down(true);
    wpt->setFlaps(1.0f);
    wpt->setSpeedBrakes(1.0f);
    pushBackWaypoint(wpt);
  
    double rolloutDistance = accelDistance(vTouchdownMetric, vTaxiMetric, decelMetric);
  
    int nPoints = 50;
    for (int i = 1; i < nPoints; i++) {
        snprintf(buffer, 12, "landing03%d", i);
        double t = ((double) i) / nPoints;
        coord = rwy->pointOnCenterline(touchdownDistance + (rolloutDistance * t));
        double vel = (vTouchdownMetric * (1.0 - t)) + (vTaxiMetric * t);
        wpt = createOnRunway(ac, buffer, coord, currElev, vel);
        wpt->setFlaps(1.0f);
        wpt->setSpeedBrakes(1.0f);
        wpt->setSpoilers(1.0f);
        wpt->setCrossat(currElev);
        pushBackWaypoint(wpt);
    }
  
    wpt->setSpeed(vTaxi);
    double mindist = (1.1 * rolloutDistance) + touchdownDistance;

    FGGroundNetwork *gn = apt->groundNetwork();
    if (!gn) {
      return true;
    }
  
    coord = rwy->pointOnCenterline(mindist);
    FGTaxiNodeRef tn;
    if (gn->getVersion() > 0) {
        tn = gn->findNearestNodeOnRunway(coord, rwy);
    } else {
        tn = gn->findNearestNode(coord);
    }
      
    if (tn) {
        wpt = createOnRunway(ac, buffer, tn->geod(), currElev, vTaxi);
        wpt->setFlaps(1.0f);
        wpt->setSpeedBrakes(1.0f);
        wpt->setSpoilers(0.0f);
        pushBackWaypoint(wpt);
    }

    return true;
}

/*******************************************************************
 * CreateParking
 * initialize the Aircraft at the parking location
 ******************************************************************/
bool FGAIFlightPlan::createParking(FGAIAircraft * ac, FGAirport * apt,
                                   double radius)
{
    FGAIWaypoint *wpt;
    double aptElev = apt->getElevation();
    double vTaxi = ac->getPerformance()->vTaxi();
    double vTaxiReduced = vTaxi * (2.0 / 3.0);
    if (!gate.isValid()) {
      wpt = createOnGround(ac, "END-Parking", apt->geod(), aptElev,
                           vTaxiReduced);
      pushBackWaypoint(wpt);
      return true;
    }
  
    FGParking* parking = gate.parking();
    double heading = SGMiscd::normalizePeriodic(0, 360, parking->getHeading() + 180.0);
    double az; // unused
    SGGeod pos;
  
    SGGeodesy::direct(parking->geod(), heading, 2.2 * parking->getRadius(),
                      pos, az);
  
    wpt = createOnGround(ac, "taxiStart", pos, aptElev, vTaxiReduced);
    pushBackWaypoint(wpt);

    SGGeodesy::direct(parking->geod(), heading, 0.1 * parking->getRadius(),
                    pos, az);
    wpt = createOnGround(ac, "taxiStart2", pos, aptElev, vTaxiReduced);
    pushBackWaypoint(wpt);

    wpt = createOnGround(ac, "END-Parking", parking->geod(), aptElev,
                       vTaxiReduced);
    pushBackWaypoint(wpt);
    return true;
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
const char* FGAIFlightPlan::getRunwayClassFromTrafficType(const string& fltType)
{
    if ((fltType == "gate") || (fltType == "cargo")) {
        return "com";
    }
    if (fltType == "ga") {
        return "gen";
    }
    if (fltType == "ul") {
        return "ul";
    }
    if ((fltType == "mil-fighter") || (fltType == "mil-transport")) {
        return "mil";
    }
    return "com";
}


double FGAIFlightPlan::getTurnRadius(double speed, bool inAir)
{
    double turn_radius;
    if (inAir == false) {
        turn_radius = ((360 / 30) * fabs(speed)) / (2 * M_PI);
    } else {
        turn_radius = 0.1911 * speed * speed;   // an estimate for 25 degrees bank
    }
    return turn_radius;
}
