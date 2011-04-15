// groundnet.cxx - Implimentation of the FlightGear airport ground handling code
//
// Written by Durk Talsma, started June 2005.
//
// Copyright (C) 2004 Durk Talsma.
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

#include <math.h>
#include <algorithm>

#include <simgear/debug/logstream.hxx>
#include <simgear/route/waypoint.hxx>

#include <Airports/simple.hxx>
#include <Airports/dynamics.hxx>

#include <AIModel/AIAircraft.hxx>
#include <AIModel/AIFlightPlan.hxx>

#include "groundnetwork.hxx"

/***************************************************************************
 * FGTaxiSegment
 **************************************************************************/

void FGTaxiSegment::setStart(FGTaxiNodeVector * nodes)
{
    FGTaxiNodeVectorIterator i = nodes->begin();
    while (i != nodes->end()) {
        //cerr << "Scanning start node index" << (*i)->getIndex() << endl;
        if ((*i)->getIndex() == startNode) {
            start = (*i)->getAddress();
            (*i)->addSegment(this);
            return;
        }
        i++;
    }
    SG_LOG(SG_GENERAL, SG_ALERT,
           "Could not find start node " << startNode << endl);
}

void FGTaxiSegment::setEnd(FGTaxiNodeVector * nodes)
{
    FGTaxiNodeVectorIterator i = nodes->begin();
    while (i != nodes->end()) {
        //cerr << "Scanning end node index" << (*i)->getIndex() << endl;
        if ((*i)->getIndex() == endNode) {
            end = (*i)->getAddress();
            return;
        }
        i++;
    }
    SG_LOG(SG_GENERAL, SG_ALERT,
           "Could not find end node " << endNode << endl);
}



// There is probably a computationally cheaper way of 
// doing this.
void FGTaxiSegment::setTrackDistance()
{
    length = SGGeodesy::distanceM(start->getGeod(), end->getGeod());
}


void FGTaxiSegment::setCourseDiff(double crse)
{
    headingDiff = fabs(course - crse);

    if (headingDiff > 180)
        headingDiff = fabs(headingDiff - 360);
}


/***************************************************************************
 * FGTaxiRoute
 **************************************************************************/
bool FGTaxiRoute::next(int *nde)
{
    //for (intVecIterator i = nodes.begin(); i != nodes.end(); i++)
    //  cerr << "FGTaxiRoute contains : " << *(i) << endl;
    //cerr << "Offset from end: " << nodes.end() - currNode << endl;
    //if (currNode != nodes.end())
    //  cerr << "true" << endl;
    //else
    //  cerr << "false" << endl;
    //if (nodes.size() != (routes.size()) +1)
    //  cerr << "ALERT: Misconfigured TaxiRoute : " << nodes.size() << " " << routes.size() << endl;

    if (currNode == nodes.end())
        return false;
    *nde = *(currNode);
    if (currNode != nodes.begin())      // make sure route corresponds to the end node
        currRoute++;
    currNode++;
    return true;
};

bool FGTaxiRoute::next(int *nde, int *rte)
{
    //for (intVecIterator i = nodes.begin(); i != nodes.end(); i++)
    //  cerr << "FGTaxiRoute contains : " << *(i) << endl;
    //cerr << "Offset from end: " << nodes.end() - currNode << endl;
    //if (currNode != nodes.end())
    //  cerr << "true" << endl;
    //else
    //  cerr << "false" << endl;
    if (nodes.size() != (routes.size()) + 1) {
        SG_LOG(SG_GENERAL, SG_ALERT,
               "ALERT: Misconfigured TaxiRoute : " << nodes.
               size() << " " << routes.size());
        exit(1);
    }
    if (currNode == nodes.end())
        return false;
    *nde = *(currNode);
    //*rte = *(currRoute);
    if (currNode != nodes.begin())      // Make sure route corresponds to the end node
    {
        *rte = *(currRoute);
        currRoute++;
    } else {
        // If currNode points to the first node, this means the aircraft is not on the taxi node
        // yet. Make sure to return a unique identifyer in this situation though, because otherwise
        // the speed adjust AI code may be unable to resolve whether two aircraft are on the same 
        // taxi route or not. the negative of the preceding route seems a logical choice, as it is 
        // unique for any starting location. 
        // Note that this is probably just a temporary fix until I get Parking / tower control working.
        *rte = -1 * *(currRoute);
    }
    currNode++;
    return true;
};

void FGTaxiRoute::rewind(int route)
{
    int currPoint;
    int currRoute;
    first();
    do {
        if (!(next(&currPoint, &currRoute))) {
            SG_LOG(SG_GENERAL, SG_ALERT,
                   "Error in rewinding TaxiRoute: current" << currRoute <<
                   " goal " << route);
        }
    } while (currRoute != route);
}




/***************************************************************************
 * FGGroundNetwork()
 **************************************************************************/
bool compare_nodes(FGTaxiNode * a, FGTaxiNode * b)
{
    return (*a) < (*b);
}

bool compare_segments(FGTaxiSegment * a, FGTaxiSegment * b)
{
    return (*a) < (*b);
}

FGGroundNetwork::FGGroundNetwork()
{
    hasNetwork = false;
    foundRoute = false;
    totalDistance = 0;
    maxDistance = 0;
    //maxDepth    = 1000;
    count = 0;
    currTraffic = activeTraffic.begin();

}

FGGroundNetwork::~FGGroundNetwork()
{
    for (FGTaxiNodeVectorIterator node = nodes.begin();
         node != nodes.end(); node++) {
        delete(*node);
    }
    nodes.clear();
    pushBackNodes.clear();
    for (FGTaxiSegmentVectorIterator seg = segments.begin();
         seg != segments.end(); seg++) {
        delete(*seg);
    }
    segments.clear();
}

void FGGroundNetwork::addSegment(const FGTaxiSegment & seg)
{
    segments.push_back(new FGTaxiSegment(seg));
}

void FGGroundNetwork::addNode(const FGTaxiNode & node)
{
    nodes.push_back(new FGTaxiNode(node));
}

void FGGroundNetwork::addNodes(FGParkingVec * parkings)
{
    FGTaxiNode n;
    FGParkingVecIterator i = parkings->begin();
    while (i != parkings->end()) {
        n.setIndex(i->getIndex());
        n.setLatitude(i->getLatitude());
        n.setLongitude(i->getLongitude());
        nodes.push_back(new FGTaxiNode(n));

        i++;
    }
}



void FGGroundNetwork::init()
{
    hasNetwork = true;
    int index = 1;
    sort(nodes.begin(), nodes.end(), compare_nodes);
    //sort(segments.begin(), segments.end(), compare_segments());
    FGTaxiSegmentVectorIterator i = segments.begin();
    while (i != segments.end()) {
        (*i)->setStart(&nodes);
        (*i)->setEnd(&nodes);
        (*i)->setTrackDistance();
        (*i)->setIndex(index);
        if ((*i)->isPushBack()) {
            pushBackNodes.push_back((*i)->getEnd());
        }
        //SG_LOG(SG_GENERAL, SG_BULK,  "initializing segment " << (*i)->getIndex() << endl);
        //SG_LOG(SG_GENERAL, SG_BULK, "Track distance = "     << (*i)->getLength() << endl);
        //SG_LOG(SG_GENERAL, SG_BULK, "Track runs from "      << (*i)->getStart()->getIndex() << " to "
        //                                                    << (*i)->getEnd()->getIndex() << endl);
        i++;
        index++;
    }

    i = segments.begin();
    while (i != segments.end()) {
        FGTaxiSegmentVectorIterator j = (*i)->getEnd()->getBeginRoute();
        while (j != (*i)->getEnd()->getEndRoute()) {
            if ((*j)->getEnd()->getIndex() == (*i)->getStart()->getIndex()) {
//          int start1 = (*i)->getStart()->getIndex();
//          int end1   = (*i)->getEnd()  ->getIndex();
//          int start2 = (*j)->getStart()->getIndex();
//          int end2   = (*j)->getEnd()->getIndex();
//          int oppIndex = (*j)->getIndex();
                //cerr << "Opposite of  " << (*i)->getIndex() << " (" << start1 << "," << end1 << ") "
                //   << "happens to be " << oppIndex      << " (" << start2 << "," << end2 << ") " << endl;
                (*i)->setOpposite(*j);
                break;
            }
            j++;
        }
        i++;
    }
    //FGTaxiNodeVectorIterator j = nodes.begin();
    //while (j != nodes.end()) {
    //    if ((*j)->getHoldPointType() == 3) {
    //        pushBackNodes.push_back((*j));
    //    }
    //    j++;
    //}
    //cerr << "Done initializing ground network" << endl;
    //exit(1);
}

int FGGroundNetwork::findNearestNode(const SGGeod & aGeod)
{
    double minDist = HUGE_VAL;
    int index = -1;

    for (FGTaxiNodeVectorIterator itr = nodes.begin(); itr != nodes.end();
         itr++) {
        double d = SGGeodesy::distanceM(aGeod, (*itr)->getGeod());
        if (d < minDist) {
            minDist = d;
            index = (*itr)->getIndex();
            //cerr << "Minimum distance of " << minDist << " for index " << index << endl;
        }
    }

    return index;
}

int FGGroundNetwork::findNearestNode(double lat, double lon)
{
    return findNearestNode(SGGeod::fromDeg(lon, lat));
}

FGTaxiNode *FGGroundNetwork::findNode(unsigned idx)
{                               /*
                                   for (FGTaxiNodeVectorIterator 
                                   itr = nodes.begin();
                                   itr != nodes.end(); itr++)
                                   {
                                   if (itr->getIndex() == idx)
                                   return itr->getAddress();
                                   } */

    if ((idx >= 0) && (idx < nodes.size()))
        return nodes[idx]->getAddress();
    else
        return 0;
}

FGTaxiSegment *FGGroundNetwork::findSegment(unsigned idx)
{                               /*
                                   for (FGTaxiSegmentVectorIterator 
                                   itr = segments.begin();
                                   itr != segments.end(); itr++)
                                   {
                                   if (itr->getIndex() == idx)
                                   return itr->getAddress();
                                   } 
                                 */
    if ((idx > 0) && (idx <= segments.size()))
        return segments[idx - 1]->getAddress();
    else {
        //cerr << "Alert: trying to find invalid segment " << idx << endl;
        return 0;
    }
}


FGTaxiRoute FGGroundNetwork::findShortestRoute(int start, int end,
                                               bool fullSearch)
{
//implements Dijkstra's algorithm to find shortest distance route from start to end
//taken from http://en.wikipedia.org/wiki/Dijkstra's_algorithm

    //double INFINITE = 100000000000.0;
    // initialize scoring values
    int nParkings = parent->getDynamics()->getNrOfParkings();
    FGTaxiNodeVector *currNodesSet;
    if (fullSearch) {
        currNodesSet = &nodes;
    } else {
        currNodesSet = &pushBackNodes;
    }

    for (FGTaxiNodeVectorIterator
         itr = currNodesSet->begin(); itr != currNodesSet->end(); itr++) {
        (*itr)->setPathScore(HUGE_VAL); //infinity by all practical means
        (*itr)->setPreviousNode(0);     //
        (*itr)->setPreviousSeg(0);      //
    }

    FGTaxiNode *firstNode = findNode(start);
    firstNode->setPathScore(0);

    FGTaxiNode *lastNode = findNode(end);

    FGTaxiNodeVector unvisited(*currNodesSet);  // working copy

    while (!unvisited.empty()) {
        FGTaxiNode *best = *(unvisited.begin());
        for (FGTaxiNodeVectorIterator
             itr = unvisited.begin(); itr != unvisited.end(); itr++) {
            if ((*itr)->getPathScore() < best->getPathScore())
                best = (*itr);
        }

        FGTaxiNodeVectorIterator newend =
            remove(unvisited.begin(), unvisited.end(), best);
        unvisited.erase(newend, unvisited.end());

        if (best == lastNode) { // found route or best not connected
            break;
        } else {
            for (FGTaxiSegmentVectorIterator
                 seg = best->getBeginRoute();
                 seg != best->getEndRoute(); seg++) {
                if (fullSearch || (*seg)->isPushBack()) {
                    FGTaxiNode *tgt = (*seg)->getEnd();
                    double alt =
                        best->getPathScore() + (*seg)->getLength() +
                        (*seg)->getPenalty(nParkings);
                    if (alt < tgt->getPathScore()) {    // Relax (u,v)
                        tgt->setPathScore(alt);
                        tgt->setPreviousNode(best);
                        tgt->setPreviousSeg(*seg);      //
                    }
                } else {
                    //   // cerr << "Skipping TaxiSegment " << (*seg)->getIndex() << endl;
                }
            }
        }
    }

    if (lastNode->getPathScore() == HUGE_VAL) {
        // no valid route found
        if (fullSearch) {
            SG_LOG(SG_GENERAL, SG_ALERT,
                   "Failed to find route from waypoint " << start << " to "
                   << end << " at " << parent->getId());
        }
        FGTaxiRoute empty;
        return empty;
        //exit(1); //TODO exit more gracefully, no need to stall the whole sim with broken GN's
    } else {
        // assemble route from backtrace information
        intVec nodes, routes;
        FGTaxiNode *bt = lastNode;
        while (bt->getPreviousNode() != 0) {
            nodes.push_back(bt->getIndex());
            routes.push_back(bt->getPreviousSegment()->getIndex());
            bt = bt->getPreviousNode();
        }
        nodes.push_back(start);
        reverse(nodes.begin(), nodes.end());
        reverse(routes.begin(), routes.end());

        return FGTaxiRoute(nodes, routes, lastNode->getPathScore(), 0);
    }
}

int FGTaxiSegment::getPenalty(int nGates)
{
    int penalty = 0;
    if (end->getIndex() < nGates) {
        penalty += 10000;
    }
    if (end->getIsOnRunway()) { // For now. In future versions, need to find out whether runway is active.
        penalty += 1000;
    }
    return penalty;
}

/* ATC Related Functions */

void FGGroundNetwork::announcePosition(int id,
                                       FGAIFlightPlan * intendedRoute,
                                       int currentPosition, double lat,
                                       double lon, double heading,
                                       double speed, double alt,
                                       double radius, int leg,
                                       FGAIAircraft * aircraft)
{
    init();
    TrafficVectorIterator i = activeTraffic.begin();
    // Search search if the current id alread has an entry
    // This might be faster using a map instead of a vector, but let's start by taking a safe route
    if (activeTraffic.size()) {
        //while ((i->getId() != id) && i != activeTraffic.end()) {
        while (i != activeTraffic.end()) {
            if (i->getId() == id) {
                break;
            }
            i++;
        }
    }
    // Add a new TrafficRecord if no one exsists for this aircraft.
    if (i == activeTraffic.end() || (activeTraffic.size() == 0)) {
        FGTrafficRecord rec;
        rec.setId(id);
        rec.setLeg(leg);
        rec.setPositionAndIntentions(currentPosition, intendedRoute);
        rec.setPositionAndHeading(lat, lon, heading, speed, alt);
        rec.setRadius(radius);  // only need to do this when creating the record.
        rec.setAircraft(aircraft);
        activeTraffic.push_back(rec);
    } else {
        i->setPositionAndIntentions(currentPosition, intendedRoute);
        i->setPositionAndHeading(lat, lon, heading, speed, alt);
    }
}

void FGGroundNetwork::signOff(int id)
{
    TrafficVectorIterator i = activeTraffic.begin();
    // Search search if the current id alread has an entry
    // This might be faster using a map instead of a vector, but let's start by taking a safe route
    if (activeTraffic.size()) {
        //while ((i->getId() != id) && i != activeTraffic.end()) {
        while (i != activeTraffic.end()) {
            if (i->getId() == id) {
                break;
            }
            i++;
        }
    }
    if (i == activeTraffic.end() || (activeTraffic.size() == 0)) {
        SG_LOG(SG_GENERAL, SG_ALERT,
               "AI error: Aircraft without traffic record is signing off");
    } else {
        i = activeTraffic.erase(i);
    }
}

void FGGroundNetwork::updateAircraftInformation(int id, double lat, double lon,
                                                double heading, double speed, double alt,
                                                double dt)
{
    // Check whether aircraft are on hold due to a preceding pushback. If so, make sure to 
    // Transmit air-to-ground "Ready to taxi request:
    // Transmit ground to air approval / hold
    // Transmit confirmation ... 
    // Probably use a status mechanism similar to the Engine start procedure in the startup controller.


    TrafficVectorIterator i = activeTraffic.begin();
    // Search search if the current id has an entry
    // This might be faster using a map instead of a vector, but let's start by taking a safe route
    TrafficVectorIterator current, closest;
    if (activeTraffic.size()) {
        //while ((i->getId() != id) && i != activeTraffic.end()) {
        while (i != activeTraffic.end()) {
            if (i->getId() == id) {
                break;
            }
            i++;
        }
    }
    // update position of the current aircraft
    if (i == activeTraffic.end() || (activeTraffic.size() == 0)) {
        SG_LOG(SG_GENERAL, SG_ALERT,
               "AI error: updating aircraft without traffic record");
    } else {
        i->setPositionAndHeading(lat, lon, heading, speed, alt);
        current = i;
    }

    setDt(getDt() + dt);

    // Update every three secs, but add some randomness
    // to prevent all IA objects doing this in synchrony
    //if (getDt() < (3.0) + (rand() % 10))
    //  return;
    //else
    //  setDt(0);
    current->clearResolveCircularWait();
    current->setWaitsForId(0);
    checkSpeedAdjustment(id, lat, lon, heading, speed, alt);
    bool needsTaxiClearance = current->getAircraft()->getTaxiClearanceRequest();
    if (!needsTaxiClearance) {
        checkHoldPosition(id, lat, lon, heading, speed, alt);
        if (checkForCircularWaits(id)) {
            i->setResolveCircularWait();
        }
    } else {
        current->setHoldPosition(true);
        int state = current->getState();
        time_t now = time(NULL) + fgGetLong("/sim/time/warp");
        if ((now - lastTransmission) > 15) {
            available = true;
        }
        if ((state < 3) && available) {
             transmit(&(*current), MSG_REQUEST_TAXI_CLEARANCE, ATC_AIR_TO_GROUND, true);
             current->setState(3);
             lastTransmission = now;
             available = false;
        }
        if ((state == 3) && available) {
            transmit(&(*current), MSG_ISSUE_TAXI_CLEARANCE, ATC_GROUND_TO_AIR, true);
            current->setState(4);
            lastTransmission = now;
            available = false;
        }
        if ((state == 4) && available) {
            transmit(&(*current), MSG_ACKNOWLEDGE_TAXI_CLEARANCE, ATC_AIR_TO_GROUND, true);
            current->setState(5);
            lastTransmission = now;
            available = false;
        }
        if ((state == 5) && available) {
            current->setState(0);
            current->getAircraft()->setTaxiClearanceRequest(false);
            current->setHoldPosition(true);
            available = false;
        }

    }

}

/**
   Scan for a speed adjustment change. Find the nearest aircraft that is in front
   and adjust speed when we get too close. Only do this when current position and/or
   intentions of the current aircraft match current taxiroute position of the proximate
   aircraft. For traffic that is on other routes we need to issue a "HOLD Position"
   instruction. See below for the hold position instruction.

   Note that there currently still is one flaw in the logic that needs to be addressed. 
   There can be situations where one aircraft is in front of the current aircraft, on a separate
   route, but really close after an intersection coming off the current route. This
   aircraft is still close enough to block the current aircraft. This situation is currently
   not addressed yet, but should be.
*/

void FGGroundNetwork::checkSpeedAdjustment(int id, double lat,
                                           double lon, double heading,
                                           double speed, double alt)
{

    TrafficVectorIterator current, closest;
    TrafficVectorIterator i = activeTraffic.begin();
    bool otherReasonToSlowDown = false;
    bool previousInstruction;
    if (activeTraffic.size()) {
        //while ((i->getId() != id) && (i != activeTraffic.end()))
        while (i != activeTraffic.end()) {
            if (i->getId() == id) {
                break;
            }
            i++;
        }
    } else {
        return;
    }
    if (i == activeTraffic.end() || (activeTraffic.size() == 0)) {
        SG_LOG(SG_GENERAL, SG_ALERT,
               "AI error: Trying to access non-existing aircraft in FGGroundNetwork::checkSpeedAdjustment");
    }
    current = i;
    //closest = current;

    previousInstruction = current->getSpeedAdjustment();
    double mindist = HUGE_VAL;
    if (activeTraffic.size()) {
        double course, dist, bearing, minbearing, az2;
        SGGeod curr(SGGeod::fromDegM(lon, lat, alt));
        //TrafficVector iterator closest;
        closest = current;
        for (TrafficVectorIterator i = activeTraffic.begin();
             i != activeTraffic.end(); i++) {
            if (i == current) {
                continue;
            }

            SGGeod other(SGGeod::fromDegM(i->getLongitude(),
                                          i->getLatitude(),
                                          i->getAltitude()));
            SGGeodesy::inverse(curr, other, course, az2, dist);
            bearing = fabs(heading - course);
            if (bearing > 180)
                bearing = 360 - bearing;
            if ((dist < mindist) && (bearing < 60.0)) {
                mindist = dist;
                closest = i;
                minbearing = bearing;
            }
        }
        //Check traffic at the tower controller
        if (towerController->hasActiveTraffic()) {
            for (TrafficVectorIterator i =
                 towerController->getActiveTraffic().begin();
                 i != towerController->getActiveTraffic().end(); i++) {
                //cerr << "Comparing " << current->getId() << " and " << i->getId() << endl;
                SGGeod other(SGGeod::fromDegM(i->getLongitude(),
                                              i->getLatitude(),
                                              i->getAltitude()));
                SGGeodesy::inverse(curr, other, course, az2, dist);
                bearing = fabs(heading - course);
                if (bearing > 180)
                    bearing = 360 - bearing;
                if ((dist < mindist) && (bearing < 60.0)) {
                    mindist = dist;
                    closest = i;
                    minbearing = bearing;
                    otherReasonToSlowDown = true;
                }
            }
        }
        // Finally, check UserPosition
        double userLatitude = fgGetDouble("/position/latitude-deg");
        double userLongitude = fgGetDouble("/position/longitude-deg");
        SGGeod user(SGGeod::fromDeg(userLongitude, userLatitude));
        SGGeodesy::inverse(curr, user, course, az2, dist);

        bearing = fabs(heading - course);
        if (bearing > 180)
            bearing = 360 - bearing;
        if ((dist < mindist) && (bearing < 60.0)) {
            mindist = dist;
            //closest = i;
            minbearing = bearing;
            otherReasonToSlowDown = true;
        }

        current->clearSpeedAdjustment();

        if (current->checkPositionAndIntentions(*closest)
            || otherReasonToSlowDown) {
            double maxAllowableDistance =
                (1.1 * current->getRadius()) +
                (1.1 * closest->getRadius());
            if (mindist < 2 * maxAllowableDistance) {
                if (current->getId() == closest->getWaitsForId())
                    return;
                else
                    current->setWaitsForId(closest->getId());
                if (closest->getId() != current->getId())
                    current->setSpeedAdjustment(closest->getSpeed() *
                                                (mindist / 100));
                else
                    current->setSpeedAdjustment(0);     // This can only happen when the user aircraft is the one closest
                if (mindist < maxAllowableDistance) {
                    //double newSpeed = (maxAllowableDistance-mindist);
                    //current->setSpeedAdjustment(newSpeed);
                    //if (mindist < 0.5* maxAllowableDistance)
                    //  {
                    current->setSpeedAdjustment(0);
                    //  }
                }
            }
        }
    }
}

/**
   Check for "Hold position instruction".
   The hold position should be issued under the following conditions:
   1) For aircraft entering or crossing a runway with active traffic on it, or landing aircraft near it
   2) For taxiing aircraft that use one taxiway in opposite directions
   3) For crossing or merging taxiroutes.
*/

void FGGroundNetwork::checkHoldPosition(int id, double lat,
                                        double lon, double heading,
                                        double speed, double alt)
{
    TrafficVectorIterator current;
    TrafficVectorIterator i = activeTraffic.begin();
    if (activeTraffic.size()) {
        //while ((i->getId() != id) && i != activeTraffic.end()) 
        while (i != activeTraffic.end()) {
            if (i->getId() == id) {
                break;
            }
            i++;
        }
    } else {
        return;
    }
    if (i == activeTraffic.end() || (activeTraffic.size() == 0)) {
        SG_LOG(SG_GENERAL, SG_ALERT,
               "AI error: Trying to access non-existing aircraft in FGGroundNetwork::checkHoldPosition");
    }
    current = i;
    bool origStatus = current->hasHoldPosition();
    current->setHoldPosition(false);
    SGGeod curr(SGGeod::fromDegM(lon, lat, alt));

    for (i = activeTraffic.begin(); i != activeTraffic.end(); i++) {
        if (i->getId() != current->getId()) {
            int node = current->crosses(this, *i);
            if (node != -1) {
                FGTaxiNode *taxiNode = findNode(node);

                // Determine whether it's save to continue or not. 
                // If we have a crossing route, there are two possibilities:
                // 1) This is an interestion
                // 2) This is oncoming two-way traffic, using the same taxiway.
                //cerr << "Hold check 1 : " << id << " has common node " << node << endl;

                SGGeod other(SGGeod::
                             fromDegM(i->getLongitude(), i->getLatitude(),
                                      i->getAltitude()));
                bool needsToWait;
                bool opposing;
                if (current->isOpposing(this, *i, node)) {
                    needsToWait = true;
                    opposing = true;
                    //cerr << "Hold check 2 : " << node << "  has opposing segment " << endl;
                    // issue a "Hold Position" as soon as we're close to the offending node
                    // For now, I'm doing this as long as the other aircraft doesn't
                    // have a hold instruction as soon as we're within a reasonable 
                    // distance from the offending node.
                    // This may be a bit of a conservative estimate though, as it may
                    // be well possible that both aircraft can both continue to taxi 
                    // without crashing into each other.
                } else {
                    opposing = false;
                    if (SGGeodesy::distanceM(other, taxiNode->getGeod()) > 200) // 2.0*i->getRadius())
                    {
                        needsToWait = false;
                        //cerr << "Hold check 3 : " << id <<"  Other aircraft approaching node is still far away. (" << dist << " nm). Can safely continue " 
                        //           << endl;
                    } else {
                        needsToWait = true;
                        //cerr << "Hold check 4: " << id << "  Would need to wait for other aircraft : distance = " << dist << " meters" << endl;
                    }
                }

                double dist =
                    SGGeodesy::distanceM(curr, taxiNode->getGeod());
                if (!(i->hasHoldPosition())) {

                    if ((dist < 200) && //2.5*current->getRadius()) && 
                        (needsToWait) && (i->onRoute(this, *current)) &&
                        //((i->onRoute(this, *current)) || ((!(i->getSpeedAdjustment())))) &&
                        (!(current->getId() == i->getWaitsForId())))
                        //(!(i->getSpeedAdjustment()))) // &&
                        //(!(current->getSpeedAdjustment())))

                    {
                        current->setHoldPosition(true);
                        current->setWaitsForId(i->getId());
                        //cerr << "Hold check 5: " << current->getCallSign() <<"  Setting Hold Position: distance to node ("  << node << ") "
                        //           << dist << " meters. Waiting for " << i->getCallSign();
                        //if (opposing)
                        //cerr <<" [opposing] " << endl;
                        //else
                        //        cerr << "[non-opposing] " << endl;
                        //if (i->hasSpeefAdjustment())
                        //        {
                        //          cerr << " (which in turn waits for ) " << i->
                    } else {
                        //cerr << "Hold check 6: " << id << "  No need to hold yet: Distance to node : " << dist << " nm"<< endl;
                    }
                }
            }
        }
    }
    bool currStatus = current->hasHoldPosition();

    // Either a Hold Position or a resume taxi transmission has been issued
    time_t now = time(NULL) + fgGetLong("/sim/time/warp");
    if ((now - lastTransmission) > 2) {
        available = true;
    }
    if ((origStatus != currStatus) && available) {
        //cerr << "Issueing hold short instrudtion " << currStatus << " " << available << endl;
        if (currStatus == true) { // No has a hold short instruction
           transmit(&(*current), MSG_HOLD_POSITION, ATC_GROUND_TO_AIR, true);
           //cerr << "Transmittin hold short instrudtion " << currStatus << " " << available << endl;
           current->setState(1);
        } else {
           transmit(&(*current), MSG_RESUME_TAXI, ATC_GROUND_TO_AIR, true);
           //cerr << "Transmittig resume instrudtion " << currStatus << " " << available << endl;
           current->setState(2);
        }
        lastTransmission = now;
        available = false;
        // Don't act on the changed instruction until the transmission is confirmed
        // So set back to original status
        current->setHoldPosition(origStatus);
        //cerr << "Current state " << current->getState() << endl;
    } else {
    }
    int state = current->getState();
    if ((state == 1) && (available)) {
        //cerr << "ACKNOWLEDGE HOLD" << endl;
        transmit(&(*current), MSG_ACKNOWLEDGE_HOLD_POSITION, ATC_AIR_TO_GROUND, true);
        current->setState(0);
        current->setHoldPosition(true);
        lastTransmission = now;
        available = false;

    }
    if ((state == 2) && (available)) {
        //cerr << "ACKNOWLEDGE RESUME" << endl;
        transmit(&(*current), MSG_ACKNOWLEDGE_RESUME_TAXI, ATC_AIR_TO_GROUND, true);
        current->setState(0);
        current->setHoldPosition(false);
        lastTransmission = now;
        available = false;
    }
}

/**
 * Check whether situations occur where the current aircraft is waiting for itself
 * due to higher order interactions. 
 * A 'circular' wait is a situation where a waits for b, b waits for c, and c waits
 * for a. Ideally each aircraft only waits for one other aircraft, so by tracing 
 * through this list of waiting aircraft, we can check if we'd eventually end back 
 * at the current aircraft.
 *
 * Note that we should consider the situation where we are actually checking aircraft
 * d, which is waiting for aircraft a. d is not part of the loop, but is held back by
 * the looping aircraft. If we don't check for that, this function will get stuck into
 * endless loop.
 */

bool FGGroundNetwork::checkForCircularWaits(int id)
{
    //cerr << "Performing Wait check " << id << endl;
    int target = 0;
    TrafficVectorIterator current, other;
    TrafficVectorIterator i = activeTraffic.begin();
    int trafficSize = activeTraffic.size();
    if (trafficSize) {
        while (i != activeTraffic.end()) {
            if (i->getId() == id) {
                break;
            }
            i++;
        }
    } else {
        return false;
    }
    if (i == activeTraffic.end() || (trafficSize == 0)) {
        SG_LOG(SG_GENERAL, SG_ALERT,
               "AI error: Trying to access non-existing aircraft in FGGroundNetwork::checkForCircularWaits");
    }

    current = i;
    target = current->getWaitsForId();
    //bool printed = false; // Note that this variable is for debugging purposes only.
    int counter = 0;

    if (id == target) {
        //cerr << "aircraft waits for user" << endl;
        return false;
    }


    while ((target > 0) && (target != id) && counter++ < trafficSize) {
        //printed = true;
        TrafficVectorIterator i = activeTraffic.begin();
        if (trafficSize) {
            //while ((i->getId() != id) && i != activeTraffic.end()) 
            while (i != activeTraffic.end()) {
                if (i->getId() == target) {
                    break;
                }
                i++;
            }
        } else {
            return false;
        }
        if (i == activeTraffic.end() || (trafficSize == 0)) {
            //cerr << "[Waiting for traffic at Runway: DONE] " << endl << endl;;
            // The target id is not found on the current network, which means it's at the tower
            //SG_LOG(SG_GENERAL, SG_ALERT, "AI error: Trying to access non-existing aircraft in FGGroundNetwork::checkForCircularWaits");
            return false;
        }
        other = i;
        target = other->getWaitsForId();

        // actually this trap isn't as impossible as it first seemed:
        // the setWaitsForID(id) is set to current when the aircraft
        // is waiting for the user controlled aircraft. 
        //if (current->getId() == other->getId()) {
        //    cerr << "Caught the impossible trap" << endl;
        //    cerr << "Current = " << current->getId() << endl;
        //    cerr << "Other   = " << other  ->getId() << endl;
        //    for (TrafficVectorIterator at = activeTraffic.begin();
        //          at != activeTraffic.end();
        //          at++) {
        //        cerr << "currently active aircraft : " << at->getCallSign() << " with Id " << at->getId() << " waits for " << at->getWaitsForId() << endl;
        //    }
        //    exit(1);
        if (current->getId() == other->getId())
            return false;
        //}
        //cerr << current->getCallSign() << " (" << current->getId()  << ") " << " -> " << other->getCallSign() 
        //     << " (" << other->getId()  << "); " << endl;;
        //current = other;
    }






    //if (printed)
    //   cerr << "[done] " << endl << endl;;
    if (id == target) {
        SG_LOG(SG_GENERAL, SG_WARN,
               "Detected circular wait condition: Id = " << id <<
               "target = " << target);
        return true;
    } else {
        return false;
    }
}

// Note that this function is probably obsolete...
bool FGGroundNetwork::hasInstruction(int id)
{
    TrafficVectorIterator i = activeTraffic.begin();
    // Search search if the current id has an entry
    // This might be faster using a map instead of a vector, but let's start by taking a safe route
    if (activeTraffic.size()) {
        //while ((i->getId() != id) && i != activeTraffic.end()) {
        while (i != activeTraffic.end()) {
            if (i->getId() == id) {
                break;
            }
            i++;
        }
    }
    if (i == activeTraffic.end() || (activeTraffic.size() == 0)) {
        SG_LOG(SG_GENERAL, SG_ALERT,
               "AI error: checking ATC instruction for aircraft without traffic record");
    } else {
        return i->hasInstruction();
    }
    return false;
}

FGATCInstruction FGGroundNetwork::getInstruction(int id)
{
    TrafficVectorIterator i = activeTraffic.begin();
    // Search search if the current id has an entry
    // This might be faster using a map instead of a vector, but let's start by taking a safe route
    if (activeTraffic.size()) {
        //while ((i->getId() != id) && i != activeTraffic.end()) {
        while (i != activeTraffic.end()) {
            if (i->getId() == id) {
                break;
            }
            i++;
        }
    }
    if (i == activeTraffic.end() || (activeTraffic.size() == 0)) {
        SG_LOG(SG_GENERAL, SG_ALERT,
               "AI error: requesting ATC instruction for aircraft without traffic record");
    } else {
        return i->getInstruction();
    }
    return FGATCInstruction();
}
