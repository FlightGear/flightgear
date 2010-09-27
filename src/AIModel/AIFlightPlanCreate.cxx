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
#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>

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
void FGAIFlightPlan::create(FGAIAircraft * ac, FGAirport * dep,
                            FGAirport * arr, int legNr, double alt,
                            double speed, double latitude,
                            double longitude, bool firstFlight,
                            double radius, const string & fltType,
                            const string & aircraftType,
                            const string & airline, double distance)
{
    int currWpt = wpt_iterator - waypoints.begin();
    switch (legNr) {
    case 1:
        createPushBack(ac, firstFlight, dep, latitude, longitude,
                       radius, fltType, aircraftType, airline);
        break;
    case 2:
        createTakeoffTaxi(ac, firstFlight, dep, radius, fltType,
                          aircraftType, airline);
        break;
    case 3:
        createTakeOff(ac, firstFlight, dep, speed, fltType);
        break;
    case 4:
        createClimb(ac, firstFlight, dep, speed, alt, fltType);
        break;
    case 5:
        createCruise(ac, firstFlight, dep, arr, latitude, longitude, speed,
                     alt, fltType);
        break;
    case 6:
        createDescent(ac, arr, latitude, longitude, speed, alt, fltType,
                      distance);
        break;
    case 7:
        createLanding(ac, arr, fltType);
        break;
    case 8:
        createLandingTaxi(ac, arr, radius, fltType, aircraftType, airline);
        break;
    case 9:
        createParking(ac, arr, radius);
        break;
    default:
        //exit(1);
        SG_LOG(SG_INPUT, SG_ALERT,
               "AIFlightPlan::create() attempting to create unknown leg"
               " this is probably an internal program error");
    }
    wpt_iterator = waypoints.begin() + currWpt;
    leg++;
}

FGAIFlightPlan::waypoint *
    FGAIFlightPlan::createOnGround(FGAIAircraft * ac,
                                   const std::string & aName,
                                   const SGGeod & aPos, double aElev,
                                   double aSpeed)
{
    waypoint *wpt = new waypoint;
    wpt->name = aName;
    wpt->longitude = aPos.getLongitudeDeg();
    wpt->latitude = aPos.getLatitudeDeg();
    wpt->altitude = aElev;
    wpt->speed = aSpeed;
    wpt->crossat = -10000.1;
    wpt->gear_down = true;
    wpt->flaps_down = true;
    wpt->finished = false;
    wpt->on_ground = true;
    wpt->routeIndex = 0;
    return wpt;
}

FGAIFlightPlan::waypoint *
    FGAIFlightPlan::createInAir(FGAIAircraft * ac,
                                const std::string & aName,
                                const SGGeod & aPos, double aElev,
                                double aSpeed)
{
    waypoint *wpt = new waypoint;
    wpt->name = aName;
    wpt->longitude = aPos.getLongitudeDeg();
    wpt->latitude = aPos.getLatitudeDeg();
    wpt->altitude = aElev;
    wpt->speed = aSpeed;
    wpt->crossat = -10000.1;
    wpt->gear_down = false;
    wpt->flaps_down = false;
    wpt->finished = false;
    wpt->on_ground = false;
    wpt->routeIndex = 0;
    return wpt;
}

FGAIFlightPlan::waypoint *
    FGAIFlightPlan::cloneWithPos(FGAIAircraft * ac, waypoint * aWpt,
                                 const std::string & aName,
                                 const SGGeod & aPos)
{
    waypoint *wpt = new waypoint;
    wpt->name = aName;
    wpt->longitude = aPos.getLongitudeDeg();
    wpt->latitude = aPos.getLatitudeDeg();

    wpt->altitude = aWpt->altitude;
    wpt->speed = aWpt->speed;
    wpt->crossat = aWpt->crossat;
    wpt->gear_down = aWpt->gear_down;
    wpt->flaps_down = aWpt->flaps_down;
    wpt->finished = aWpt->finished;
    wpt->on_ground = aWpt->on_ground;
    wpt->routeIndex = 0;

    return wpt;
}

FGAIFlightPlan::waypoint * FGAIFlightPlan::clone(waypoint * aWpt)
{
    waypoint *wpt = new waypoint;
    wpt->name = aWpt->name;
    wpt->longitude = aWpt->longitude;
    wpt->latitude = aWpt->latitude;

    wpt->altitude = aWpt->altitude;
    wpt->speed = aWpt->speed;
    wpt->crossat = aWpt->crossat;
    wpt->gear_down = aWpt->gear_down;
    wpt->flaps_down = aWpt->flaps_down;
    wpt->finished = aWpt->finished;
    wpt->on_ground = aWpt->on_ground;
    wpt->routeIndex = 0;

    return wpt;
}


void FGAIFlightPlan::createDefaultTakeoffTaxi(FGAIAircraft * ac,
                                              FGAirport * aAirport,
                                              FGRunway * aRunway)
{
    SGGeod runwayTakeoff = aRunway->pointOnCenterline(5.0);
    double airportElev = aAirport->getElevation();

    waypoint *wpt;
    wpt =
        createOnGround(ac, "Airport Center", aAirport->geod(), airportElev,
                       ac->getPerformance()->vTaxi());
    waypoints.push_back(wpt);
    wpt =
        createOnGround(ac, "Runway Takeoff", runwayTakeoff, airportElev,
                       ac->getPerformance()->vTaxi());
    waypoints.push_back(wpt);
}

void FGAIFlightPlan::createTakeoffTaxi(FGAIAircraft * ac, bool firstFlight,
                                       FGAirport * apt,
                                       double radius,
                                       const string & fltType,
                                       const string & acType,
                                       const string & airline)
{
    double heading, lat, lon;

    // If this function is called during initialization,
    // make sure we obtain a valid gate ID first
    // and place the model at the location of the gate.
    if (firstFlight) {
        if (!(apt->getDynamics()->getAvailableParking(&lat, &lon,
                                                      &heading, &gateId,
                                                      radius, fltType,
                                                      acType, airline))) {
            SG_LOG(SG_INPUT, SG_WARN, "Could not find parking for a " <<
                   acType <<
                   " of flight type " << fltType <<
                   " of airline     " << airline <<
                   " at airport     " << apt->getId());
        }
    }

    string rwyClass = getRunwayClassFromTrafficType(fltType);

    // Only set this if it hasn't been set by ATC already.
    if (activeRunway.empty()) {
        //cerr << "Getting runway for " << ac->getTrafficRef()->getCallSign() << " at " << apt->getId() << endl;
        double depHeading = ac->getTrafficRef()->getCourse();
        apt->getDynamics()->getActiveRunway(rwyClass, 1, activeRunway,
                                            depHeading);
    }
    rwy = apt->getRunwayByIdent(activeRunway);
    SGGeod runwayTakeoff = rwy->pointOnCenterline(5.0);

    FGGroundNetwork *gn = apt->getDynamics()->getGroundNetwork();
    if (!gn->exists()) {
        createDefaultTakeoffTaxi(ac, apt, rwy);
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
        createDefaultTakeoffTaxi(ac, apt, rwy);
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
        for (int i = 0; i < nrWaypointsToSkip - 3; i++) {
            taxiRoute->next(&node);
        }
        apt->getDynamics()->releaseParking(gateId);
    } else {
        if (taxiRoute->size() > 1) {
            taxiRoute->next(&node);     // chop off the first waypoint, because that is already the last of the pushback route
        }
    }

    // push each node on the taxi route as a waypoint
    int route;
    while (taxiRoute->next(&node, &route)) {
        char buffer[10];
        snprintf(buffer, 10, "%d", node);
        FGTaxiNode *tn =
            apt->getDynamics()->getGroundNetwork()->findNode(node);
        waypoint *wpt =
            createOnGround(ac, buffer, tn->getGeod(), apt->getElevation(),
                           ac->getPerformance()->vTaxi());
        wpt->routeIndex = route;
        waypoints.push_back(wpt);
    }
}

void FGAIFlightPlan::createDefaultLandingTaxi(FGAIAircraft * ac,
                                              FGAirport * aAirport)
{
    SGGeod lastWptPos =
        SGGeod::fromDeg(waypoints.back()->longitude,
                        waypoints.back()->latitude);
    double airportElev = aAirport->getElevation();

    waypoint *wpt;
    wpt =
        createOnGround(ac, "Runway Exit", lastWptPos, airportElev,
                       ac->getPerformance()->vTaxi());
    waypoints.push_back(wpt);
    wpt =
        createOnGround(ac, "Airport Center", aAirport->geod(), airportElev,
                       ac->getPerformance()->vTaxi());
    waypoints.push_back(wpt);

    double heading, lat, lon;
    aAirport->getDynamics()->getParking(gateId, &lat, &lon, &heading);
    wpt =
        createOnGround(ac, "END", SGGeod::fromDeg(lon, lat), airportElev,
                       ac->getPerformance()->vTaxi());
    waypoints.push_back(wpt);
}

void FGAIFlightPlan::createLandingTaxi(FGAIAircraft * ac, FGAirport * apt,
                                       double radius,
                                       const string & fltType,
                                       const string & acType,
                                       const string & airline)
{
    double heading, lat, lon;
    apt->getDynamics()->getAvailableParking(&lat, &lon, &heading,
                                            &gateId, radius, fltType,
                                            acType, airline);

    SGGeod lastWptPos =
        SGGeod::fromDeg(waypoints.back()->longitude,
                        waypoints.back()->latitude);
    FGGroundNetwork *gn = apt->getDynamics()->getGroundNetwork();

    // Find a route from runway end to parking/gate.
    if (!gn->exists()) {
        createDefaultLandingTaxi(ac, apt);
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
        createDefaultLandingTaxi(ac, apt);
        return;
    }

    int node;
    taxiRoute->first();
    int size = taxiRoute->size();
    // Omit the last two waypoints, as 
    // those are created by createParking()
    int route;
    for (int i = 0; i < size - 2; i++) {
        taxiRoute->next(&node, &route);
        char buffer[10];
        snprintf(buffer, 10, "%d", node);
        FGTaxiNode *tn = gn->findNode(node);
        waypoint *wpt =
            createOnGround(ac, buffer, tn->getGeod(), apt->getElevation(),
                           ac->getPerformance()->vTaxi());
        wpt->routeIndex = route;
        waypoints.push_back(wpt);
    }
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
void FGAIFlightPlan::createTakeOff(FGAIAircraft * ac, bool firstFlight,
                                   FGAirport * apt, double speed,
                                   const string & fltType)
{
    double accel = ac->getPerformance()->acceleration();
    double vTaxi = ac->getPerformance()->vTaxi();
    double vRotate = ac->getPerformance()->vRotate();
    double vTakeoff = ac->getPerformance()->vTakeoff();
    //double vClimb = ac->getPerformance()->vClimb();

    double accelMetric = (accel * SG_NM_TO_METER) / 3600;
    double vTaxiMetric = (vTaxi * SG_NM_TO_METER) / 3600;
    double vRotateMetric = (vRotate * SG_NM_TO_METER) / 3600;
    double vTakeoffMetric = (vTakeoff * SG_NM_TO_METER) / 3600;
    //double vClimbMetric = (vClimb * SG_NM_TO_METER) / 3600;
    // Acceleration = dV / dT
    // Acceleration X dT = dV
    // dT = dT / Acceleration
    //d = (Vf^2 - Vo^2) / (2*a)
    //double accelTime = (vRotate - vTaxi) / accel;
    //cerr << "Using " << accelTime << " as total acceleration time" << endl;
    double accelDistance =
        (vRotateMetric * vRotateMetric -
         vTaxiMetric * vTaxiMetric) / (2 * accelMetric);
    //cerr << "Using " << accelDistance << " " << accelMetric << " " << vRotateMetric << endl;
    waypoint *wpt;
    // Get the current active runway, based on code from David Luff
    // This should actually be unified and extended to include 
    // Preferential runway use schema's 
    // NOTE: DT (2009-01-18: IIRC, this is currently already the case, 
    // because the getActive runway function takes care of that.
    if (firstFlight) {
        string rwyClass = getRunwayClassFromTrafficType(fltType);
        double heading = ac->getTrafficRef()->getCourse();
        apt->getDynamics()->getActiveRunway(rwyClass, 1, activeRunway,
                                            heading);
        rwy = apt->getRunwayByIdent(activeRunway);
    }

    double airportElev = apt->getElevation();
    // Acceleration point, 105 meters into the runway,
    SGGeod accelPoint = rwy->pointOnCenterline(105.0);
    wpt = createOnGround(ac, "accel", accelPoint, airportElev, vRotate);
    waypoints.push_back(wpt);


    accelDistance =
        (vTakeoffMetric * vTakeoffMetric -
         vTaxiMetric * vTaxiMetric) / (2 * accelMetric);
    //cerr << "Using " << accelDistance << " " << accelMetric << " " << vTakeoffMetric << endl;
    accelPoint = rwy->pointOnCenterline(105.0 + accelDistance);
    wpt = createOnGround(ac, "rotate", accelPoint, airportElev, vTakeoff);
    waypoints.push_back(wpt);

    accelDistance =
        ((vTakeoffMetric * 1.1) * (vTakeoffMetric * 1.1) -
         vTaxiMetric * vTaxiMetric) / (2 * accelMetric);
    //cerr << "Using " << accelDistance << " " << accelMetric << " " << vTakeoffMetric << endl;
    accelPoint = rwy->pointOnCenterline(105.0 + accelDistance);
    wpt =
        createOnGround(ac, "rotate", accelPoint, airportElev + 1000,
                       vTakeoff * 1.1);
    wpt->on_ground = false;
    waypoints.push_back(wpt);

    wpt = cloneWithPos(ac, wpt, "3000 ft", rwy->end());
    wpt->altitude = airportElev + 3000;
    waypoints.push_back(wpt);

    // Finally, add two more waypoints, so that aircraft will remain under
    // Tower control until they have reached the 3000 ft climb point
    SGGeod pt = rwy->pointOnCenterline(5000 + rwy->lengthM() * 0.5);
    wpt = cloneWithPos(ac, wpt, "5000 ft", pt);
    wpt->altitude = airportElev + 5000;
    waypoints.push_back(wpt);
}

/*******************************************************************
 * CreateClimb
 * initialize the Aircraft at the parking location
 ******************************************************************/
void FGAIFlightPlan::createClimb(FGAIAircraft * ac, bool firstFlight,
                                 FGAirport * apt, double speed, double alt,
                                 const string & fltType)
{
    waypoint *wpt;
//  bool planLoaded = false;
    string fPLName;
    double vClimb = ac->getPerformance()->vClimb();

    if (firstFlight) {
        string rwyClass = getRunwayClassFromTrafficType(fltType);
        double heading = ac->getTrafficRef()->getCourse();
        apt->getDynamics()->getActiveRunway(rwyClass, 1, activeRunway,
                                            heading);
        rwy = apt->getRunwayByIdent(activeRunway);
    }
    if (sid) {
        for (wpt_vector_iterator i = sid->getFirstWayPoint();
             i != sid->getLastWayPoint(); i++) {
            waypoints.push_back(clone(*(i)));
            //cerr << " Cloning waypoint " << endl;
        }
    } else {
        SGGeod climb1 = rwy->pointOnCenterline(10 * SG_NM_TO_METER);
        wpt = createInAir(ac, "10000ft climb", climb1, vClimb, 10000);
        wpt->gear_down = true;
        wpt->flaps_down = true;
        waypoints.push_back(wpt);

        SGGeod climb2 = rwy->pointOnCenterline(20 * SG_NM_TO_METER);
        wpt = cloneWithPos(ac, wpt, "18000ft climb", climb2);
        wpt->altitude = 18000;
        waypoints.push_back(wpt);
    }
}



/*******************************************************************
 * CreateDescent
 * Generate a flight path from the last waypoint of the cruise to 
 * the permission to land point
 ******************************************************************/
void FGAIFlightPlan::createDescent(FGAIAircraft * ac, FGAirport * apt,
                                   double latitude, double longitude,
                                   double speed, double alt,
                                   const string & fltType,
                                   double requiredDistance)
{
    bool reposition = false;
    waypoint *wpt;
    double vDescent = ac->getPerformance()->vDescent();
    double vApproach = ac->getPerformance()->vApproach();


    //Beginning of Descent 
    string rwyClass = getRunwayClassFromTrafficType(fltType);
    double heading = ac->getTrafficRef()->getCourse();
    apt->getDynamics()->getActiveRunway(rwyClass, 2, activeRunway,
                                        heading);
    rwy = apt->getRunwayByIdent(activeRunway);



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


    double dAlt = alt - (apt->getElevation() + 2000);

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
    double lat = refPoint.getLatitudeDeg();
    double lon = refPoint.getLongitudeDeg();
    //cerr << "Reference point (" << lat << ", " << lon << ")." << endl;
    lat = initialTarget.getLatitudeDeg();
    lon = initialTarget.getLongitudeDeg();
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
    time_t now = time(NULL) + fgGetLong("/sim/time/warp");
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


    lat = secondaryTarget.getLatitudeDeg();
    lon = secondaryTarget.getLongitudeDeg();
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

    lat = tertiaryTarget.getLatitudeDeg();
    lon = tertiaryTarget.getLongitudeDeg();
    //cerr << "tertiary Target point (" << lat << ", " << lon << ")." << endl;


    for (int i = 1; i < nPoints; i++) {
        SGGeod result;
        double currentDist = i * (newDistance / nPoints);
        double currentAltitude = alt - (i * (dAlt / nPoints));
        SGGeodesy::direct(origin, azimuth, currentDist, result, dummyAz2);
        snprintf(buffer, 16, "descent%03d", i);
        wpt = createInAir(ac, buffer, result, currentAltitude, vDescent);
        wpt->crossat = currentAltitude;
        wpt->trackLength = (newDistance / nPoints);
        waypoints.push_back(wpt);
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
    double trackLength = (2 * M_PI * initialTurnRadius) / 360.0;
    for (int i = startval; i != endval; i += increment) {
        SGGeod result;
        double currentAltitude = apt->getElevation() + 2000;
        SGGeodesy::direct(secondaryTarget, i,
                          initialTurnRadius, result, dummyAz2);
        snprintf(buffer, 16, "turn%03d", i);
        wpt = createInAir(ac, buffer, result, currentAltitude, vDescent);
        wpt->crossat = currentAltitude;
        wpt->trackLength = trackLength;
        //cerr << "Track Length : " << wpt->trackLength;
        waypoints.push_back(wpt);
        //cerr << "  Position : " << result.getLatitudeDeg() << " " << result.getLongitudeDeg() << " " << currentAltitude << endl;
    }


    // The approach leg should bring the aircraft to approximately 4-6 out, after which the landing phase should take over. 
    //cerr << "Phase 3: Approach" << endl;
    distanceOut -= distanceCovered;
    for (int i = 1; i < nPoints; i++) {
        SGGeod result;
        double currentDist = i * (distanceOut / nPoints);
        double currentAltitude =
            apt->getElevation() + 2000 - (i * 2000 / nPoints);
        snprintf(buffer, 16, "final%03d", i);
        result = rwy->pointOnCenterline((-distanceOut) + currentDist);
        wpt = createInAir(ac, buffer, result, currentAltitude, vApproach);
        wpt->crossat = currentAltitude;
        wpt->trackLength = (distanceOut / nPoints);
        // account for the extra distance due to an extended downwind leg
        if (i == 1) {
            wpt->trackLength += distanceCovered;
        }
        //cerr << "Track Length : " << wpt->trackLength;
        waypoints.push_back(wpt);
        //cerr << "  Position : " << result.getLatitudeDeg() << " " << result.getLongitudeDeg() << " " << currentAltitude << endl;
    }

    //cerr << "Done" << endl;

    // Erase the two bogus BOD points: Note check for conflicts with scripted AI flightPlans
    IncrementWaypoint(true);
    IncrementWaypoint(true);

    if (reposition) {
        double tempDistance;
        //double minDistance = HUGE_VAL;
        string wptName;
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


}

/*******************************************************************
 * CreateLanding
 * Create a flight path from the "permision to land" point (currently
   hardcoded at 5000 meters from the threshold) to the threshold, at
   a standard glide slope angle of 3 degrees. 
 ******************************************************************/
void FGAIFlightPlan::createLanding(FGAIAircraft * ac, FGAirport * apt,
                                   const string & fltType)
{
    double vTouchdown = ac->getPerformance()->vTouchdown();
    //double vTaxi = ac->getPerformance()->vTaxi();

    //string rwyClass = getRunwayClassFromTrafficType(fltType);
    //double heading = ac->getTrafficRef()->getCourse();
    //apt->getDynamics()->getActiveRunway(rwyClass, 2, activeRunway, heading);
    //rwy = apt->getRunwayByIdent(activeRunway);


    waypoint *wpt;
    double aptElev = apt->getElevation();

    SGGeod coord;
    char buffer[12];
    for (int i = 1; i < 10; i++) {
        snprintf(buffer, 12, "wpt%d", i);
        coord = rwy->pointOnCenterline(rwy->lengthM() * (i / 10.0));
        wpt = createOnGround(ac, buffer, coord, aptElev, (vTouchdown / i));
        wpt->crossat = apt->getElevation();
        waypoints.push_back(wpt);
    }

    /*
       //Runway Threshold
       wpt = createOnGround(ac, "Threshold", rwy->threshold(), aptElev, vTouchdown);
       wpt->crossat = apt->getElevation();
       waypoints.push_back(wpt); 

       // Roll-out
       wpt = createOnGround(ac, "Center", rwy->geod(), aptElev, vTaxi*2);
       waypoints.push_back(wpt);

       SGGeod rollOut = rwy->pointOnCenterline(rwy->lengthM() * 0.9);
       wpt = createOnGround(ac, "Roll Out", rollOut, aptElev, vTaxi);
       wpt->crossat   = apt->getElevation();
       waypoints.push_back(wpt); 
     */
}

/*******************************************************************
 * CreateParking
 * initialize the Aircraft at the parking location
 ******************************************************************/
void FGAIFlightPlan::createParking(FGAIAircraft * ac, FGAirport * apt,
                                   double radius)
{
    waypoint *wpt;
    double aptElev = apt->getElevation();
    double lat = 0.0, lat2 = 0.0;
    double lon = 0.0, lon2 = 0.0;
    double az2 = 0.0;
    double heading = 0.0;

    double vTaxi = ac->getPerformance()->vTaxi();
    double vTaxiReduced = vTaxi * (2.0 / 3.0);
    apt->getDynamics()->getParking(gateId, &lat, &lon, &heading);
    heading += 180.0;
    if (heading > 360)
        heading -= 360;
    geo_direct_wgs_84(0, lat, lon, heading,
                      2.2 * radius, &lat2, &lon2, &az2);
    wpt =
        createOnGround(ac, "taxiStart", SGGeod::fromDeg(lon2, lat2),
                       aptElev, vTaxiReduced);
    waypoints.push_back(wpt);

    geo_direct_wgs_84(0, lat, lon, heading,
                      0.1 * radius, &lat2, &lon2, &az2);

    wpt =
        createOnGround(ac, "taxiStart2", SGGeod::fromDeg(lon2, lat2),
                       aptElev, vTaxiReduced);
    waypoints.push_back(wpt);

    wpt =
        createOnGround(ac, "END", SGGeod::fromDeg(lon, lat), aptElev,
                       vTaxiReduced);
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
        return string("gen");
    }
    if (fltType == "ul") {
        return string("ul");
    }
    if ((fltType == "mil-fighter") || (fltType == "mil-transport")) {
        return string("mil");
    }
    return string("com");
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
