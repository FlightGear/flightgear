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
#include <fstream>


#include <osg/Geode>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osg/Shape>

#include <simgear/debug/logstream.hxx>
#include <simgear/scene/material/EffectGeode.hxx>
#include <simgear/scene/material/matlib.hxx>
#include <simgear/scene/material/mat.hxx>

#include <Airports/simple.hxx>
#include <Airports/dynamics.hxx>

#include <AIModel/AIAircraft.hxx>
#include <AIModel/performancedata.hxx>
#include <AIModel/AIFlightPlan.hxx>

#include <ATC/atc_mgr.hxx>

#include <Scenery/scenery.hxx>

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
void FGTaxiSegment::setDimensions(double elevation)
{
    length = SGGeodesy::distanceM(start->getGeod(), end->getGeod());
    //heading = SGGeodesy::headingDeg(start->getGeod(), end->getGeod());

    double az2; //, distanceM;
    SGGeodesy::inverse(start->getGeod(), end->getGeod(), heading, az2, length);
    double coveredDistance = length * 0.5;
    SGGeodesy::direct(start->getGeod(), heading, coveredDistance, center, az2);
    //start->setElevation(elevation);
    //end->setElevation(elevation);
    //cerr << "Centerpoint = (" << center.getLatitudeDeg() << ", " << center.getLongitudeDeg() << "). Heading = " << heading << endl;
}


//void FGTaxiSegment::setCourseDiff(double crse)
//{
//    headingDiff = fabs(course - crse);

//    if (headingDiff > 180)
//        headingDiff = fabs(headingDiff - 360);
//}

void FGTaxiSegment::block(int id, time_t blockTime, time_t now)
{
    BlockListIterator i = blockTimes.begin();
    while (i != blockTimes.end()) {
        if (i->getId() == id) 
            break;
        i++;
    }
    if (i == blockTimes.end()) {
        blockTimes.push_back(Block(id, blockTime, now));
        sort(blockTimes.begin(), blockTimes.end());
    } else {
        i->updateTimeStamps(blockTime, now);
    }
}

// The segment has a block if any of the block times listed in the block list is
// smaller than the current time.
bool FGTaxiSegment::hasBlock(time_t now)
{
    for (BlockListIterator i = blockTimes.begin(); i != blockTimes.end(); i++) {
        if (i->getBlockTime() < now)
            return true;
    }
    return false;
}

void FGTaxiSegment::unblock(time_t now)
{
    if (blockTimes.size()) {
        BlockListIterator i = blockTimes.begin();
        if (i->getTimeStamp() < (now - 30)) {
            blockTimes.erase(i);
        }
    }
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

bool compare_trafficrecords(FGTrafficRecord a, FGTrafficRecord b)
{
    return (a.getIntentions().size() < b.getIntentions().size());
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
    group = 0;
    version = 0;
    networkInitialized = false;

}

FGGroundNetwork::~FGGroundNetwork()
{
    //cerr << "Running Groundnetwork Destructor " << endl;
    bool saveData = false;
    ofstream cachefile;
    if (fgGetBool("/sim/ai/groundnet-cache")) {
        SGPath cacheData(fgGetString("/sim/fg-home"));
        cacheData.append("ai");
        string airport = parent->getId();

        if ((airport) != "") {
            char buffer[128];
            ::snprintf(buffer, 128, "%c/%c/%c/",
                       airport[0], airport[1], airport[2]);
            cacheData.append(buffer);
            if (!cacheData.exists()) {
                cacheData.create_dir(0777);
            }
            cacheData.append(airport + "-groundnet-cache.txt");
            cachefile.open(cacheData.str().c_str());
            saveData = true;
        }
    }
    cachefile << "[GroundNetcachedata:ref:2011:09:04]" << endl;
    for (FGTaxiNodeVectorIterator node = nodes.begin();
            node != nodes.end(); node++) {
        if (saveData) {
            cachefile << (*node)->getIndex     () << " "
            << (*node)->getElevationM (parent->getElevation()*SG_FEET_TO_METER)   << " "
            << endl;
        }
        delete(*node);
    }
    nodes.clear();
    pushBackNodes.clear();
    for (FGTaxiSegmentVectorIterator seg = segments.begin();
            seg != segments.end(); seg++) {
        delete(*seg);
    }
    segments.clear();
    if (saveData) {
        cachefile.close();
    }
}

void FGGroundNetwork::saveElevationCache() {
    //cerr << "Running Groundnetwork Destructor " << endl;
    bool saveData = false;
    ofstream cachefile;
    if (fgGetBool("/sim/ai/groundnet-cache")) {
        SGPath cacheData(fgGetString("/sim/fg-home"));
        cacheData.append("ai");
        string airport = parent->getId();

        if ((airport) != "") {
            char buffer[128];
            ::snprintf(buffer, 128, "%c/%c/%c/",
                       airport[0], airport[1], airport[2]);
            cacheData.append(buffer);
            if (!cacheData.exists()) {
                cacheData.create_dir(0777);
            }
            cacheData.append(airport + "-groundnet-cache.txt");
            cachefile.open(cacheData.str().c_str());
            saveData = true;
        }
    }
    cachefile << "[GroundNetcachedata:ref:2011:09:04]" << endl;
    for (FGTaxiNodeVectorIterator node = nodes.begin();
            node != nodes.end(); node++) {
        if (saveData) {
            cachefile << (*node)->getIndex     () << " "
            << (*node)->getElevationM (parent->getElevation()*SG_FEET_TO_METER) << " "
            << endl;
        }
    }
    if (saveData) {
        cachefile.close();
    }
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
        n.setElevation(parent->getElevation()*SG_FEET_TO_METER);
        nodes.push_back(new FGTaxiNode(n));

        i++;
    }
}



void FGGroundNetwork::init()
{
    if (networkInitialized) {
        FGATCController::init();
        //cerr << "FGground network already initialized" << endl;
        return;
    }
    hasNetwork = true;
    nextSave = 0;
    int index = 1;
    sort(nodes.begin(), nodes.end(), compare_nodes);
    //sort(segments.begin(), segments.end(), compare_segments());
    FGTaxiSegmentVectorIterator i = segments.begin();
    while (i != segments.end()) {
        (*i)->setStart(&nodes);
        (*i)->setEnd(&nodes);
        (*i)->setDimensions(parent->getElevation() * SG_FEET_TO_METER);
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
    if (fgGetBool("/sim/ai/groundnet-cache")) {
        SGPath cacheData(fgGetString("/sim/fg-home"));
        cacheData.append("ai");
        string airport = parent->getId();

        if ((airport) != "") {
            char buffer[128];
            ::snprintf(buffer, 128, "%c/%c/%c/",
                       airport[0], airport[1], airport[2]);
            cacheData.append(buffer);
            if (!cacheData.exists()) {
                cacheData.create_dir(0777);
            }
            int index;
            double elev;
            cacheData.append(airport + "-groundnet-cache.txt");
            if (cacheData.exists()) {
                ifstream data(cacheData.c_str());
                string revisionStr;
                data >> revisionStr;
                if (revisionStr != "[GroundNetcachedata:ref:2011:09:04]") {
                    SG_LOG(SG_GENERAL, SG_ALERT,"GroundNetwork Warning: discarding outdated cachefile " <<
                           cacheData.c_str() << " for Airport " << airport);
                } else {
                    for (FGTaxiNodeVectorIterator i = nodes.begin();
                            i != nodes.end();
                            i++) {
                        (*i)->setElevation(parent->getElevation() * SG_FEET_TO_METER);
                        data >> index >> elev;
                        if (data.eof())
                            break;
                        if (index != (*i)->getIndex()) {
                            SG_LOG(SG_GENERAL, SG_ALERT, "Index read from ground network cache at airport " << airport << " does not match index in the network itself");
                        } else {
                            (*i)->setElevation(elev);
                        }
                    }
                }
            }
        }
    }
    //cerr << "Finished initializing " << parent->getId() << " groundnetwork " << endl;
    networkInitialized = true;
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

int FGGroundNetwork::findNearestNodeOnRunway(const SGGeod & aGeod)
{
    double minDist = HUGE_VAL;
    int index = -1;

    for (FGTaxiNodeVectorIterator itr = nodes.begin(); itr != nodes.end();
            itr++) {
        if (!((*itr)->getIsOnRunway())) {
            continue;
        }
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

    if (idx < nodes.size())
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
    if (!firstNode)
    {
        SG_LOG(SG_GENERAL, SG_ALERT,
               "Error in ground network. Failed to find first waypoint: " << start
               << " at " << ((parent) ? parent->getId() : "<unknown>"));
        return FGTaxiRoute();
    }
    firstNode->setPathScore(0);

    FGTaxiNode *lastNode = findNode(end);
    if (!lastNode)
    {
        SG_LOG(SG_GENERAL, SG_ALERT,
               "Error in ground network. Failed to find last waypoint: " << end
               << " at " << ((parent) ? parent->getId() : "<unknown>"));
        return FGTaxiRoute();
    }

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
                    if (!tgt)
                    {
                        SG_LOG(SG_GENERAL, SG_ALERT,
                               "Error in ground network. Found empty segment "
                               << " at " << ((parent) ? parent->getId() : "<unknown>"));
                        return FGTaxiRoute();
                    }
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
        if (leg == 2) {
            activeTraffic.push_front(rec);
        } else {
            activeTraffic.push_back(rec);   
        }
        
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
               "AI error: Aircraft without traffic record is signing off at " << SG_ORIGIN);
    } else {
        i = activeTraffic.erase(i);
    }
}
/**
 * The ground network can deal with the following states:
 * 0 =  Normal; no action required
 * 1 = "Acknowledge "Hold position
 * 2 = "Acknowledge "Resume taxi".
 * 3 = "Issue TaxiClearance"
 * 4 = Acknowledge Taxi Clearance"
 * 5 = Post acknowlegde taxiclearance: Start taxiing
 * 6 = Report runway
 * 7 = Acknowledge report runway
 * 8 = Switch tower frequency
 * 9 = Acknowledge switch tower frequency
 *************************************************************************************************************************/
bool FGGroundNetwork::checkTransmissionState(int minState, int maxState, TrafficVectorIterator i, time_t now, AtcMsgId msgId,
        AtcMsgDir msgDir)
{
    int state = i->getState();
    if ((state >= minState) && (state <= maxState) && available) {
        if ((msgDir == ATC_AIR_TO_GROUND) && isUserAircraft(i->getAircraft())) {
            //cerr << "Checking state " << state << " for " << i->getAircraft()->getCallSign() << endl;
            static SGPropertyNode_ptr trans_num = globals->get_props()->getNode("/sim/atc/transmission-num", true);
            int n = trans_num->getIntValue();
            if (n == 0) {
                trans_num->setIntValue(-1);
                // PopupCallback(n);
                //cerr << "Selected transmission message " << n << endl;
                //FGATCManager *atc = (FGATCManager*) globals->get_subsystem("atc");
                FGATCDialogNew::instance()->removeEntry(1);
            } else {
                //cerr << "creating message for " << i->getAircraft()->getCallSign() << endl;
                transmit(&(*i), &(*parent->getDynamics()), msgId, msgDir, false);
                return false;
            }
        }
        transmit(&(*i), &(*parent->getDynamics()), msgId, msgDir, true);
        i->updateState();
        lastTransmission = now;
        available = false;
        return true;
    }
    return false;
}

void FGGroundNetwork::updateAircraftInformation(int id, double lat, double lon,
        double heading, double speed, double alt,
        double dt)
{
    time_t currentTime = time(NULL);
    if (nextSave < currentTime) {
        saveElevationCache();
        nextSave = currentTime + 100 + rand() % 200;
    }
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
               "AI error: updating aircraft without traffic record at " << SG_ORIGIN);
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
        //if (checkForCircularWaits(id)) {
        //    i->setResolveCircularWait();
        //}
    } else {
        current->setHoldPosition(true);
        int state = current->getState();
        time_t now = time(NULL) + fgGetLong("/sim/time/warp");
        if ((now - lastTransmission) > 15) {
            available = true;
        }
        if (checkTransmissionState(0,2, current, now, MSG_REQUEST_TAXI_CLEARANCE, ATC_AIR_TO_GROUND)) {
            current->setState(3);
        }
        if (checkTransmissionState(3,3, current, now, MSG_ISSUE_TAXI_CLEARANCE, ATC_GROUND_TO_AIR)) {
            current->setState(4);
        }
        if (checkTransmissionState(4,4, current, now, MSG_ACKNOWLEDGE_TAXI_CLEARANCE, ATC_AIR_TO_GROUND)) {
            current->setState(5);
        }
        if ((state == 5) && available) {
            current->setState(0);
            current->getAircraft()->setTaxiClearanceRequest(false);
            current->setHoldPosition(false);
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

    TrafficVectorIterator current, closest, closestOnNetwork;
    TrafficVectorIterator i = activeTraffic.begin();
    bool otherReasonToSlowDown = false;
//    bool previousInstruction;
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
               "AI error: Trying to access non-existing aircraft in FGGroundNetwork::checkSpeedAdjustment at " << SG_ORIGIN);
    }
    current = i;
    //closest = current;

//    previousInstruction = current->getSpeedAdjustment();
    double mindist = HUGE_VAL;
    if (activeTraffic.size()) {
        double course, dist, bearing, az2; // minbearing,
        SGGeod curr(SGGeod::fromDegM(lon, lat, alt));
        //TrafficVector iterator closest;
        closest = current;
        closestOnNetwork = current;
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
                closestOnNetwork = i;
//                minbearing = bearing;
                
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
                    //cerr << "Current aircraft " << current->getAircraft()->getTrafficRef()->getCallSign()
                    //     << " is closest to " << i->getAircraft()->getTrafficRef()->getCallSign()
                    //     << ", which has status " << i->getAircraft()->isScheduledForTakeoff()
                    //     << endl;
                    mindist = dist;
                    closest = i;
//                    minbearing = bearing;
                    otherReasonToSlowDown = true;
                }
            }
        }
        // Finally, check UserPosition
        // Note, as of 2011-08-01, this should no longer be necessecary.
        /*
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
        */
        current->clearSpeedAdjustment();
        bool needBraking = false;
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
                if (closest->getId() != current->getId()) {
                    current->setSpeedAdjustment(closest->getSpeed() *
                                                (mindist / 100));
                    needBraking = true;
                    
//                     if (
//                         closest->getAircraft()->getTakeOffStatus() &&
//                         (current->getAircraft()->getTrafficRef()->getDepartureAirport() ==  closest->getAircraft()->getTrafficRef()->getDepartureAirport()) &&
//                         (current->getAircraft()->GetFlightPlan()->getRunway() == closest->getAircraft()->GetFlightPlan()->getRunway())
//                     )
//                         current->getAircraft()->scheduleForATCTowerDepartureControl(1);
                } else {
                    current->setSpeedAdjustment(0);     // This can only happen when the user aircraft is the one closest
                }
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
        if ((closest->getId() == closestOnNetwork->getId()) && (current->getPriority() < closest->getPriority()) && needBraking) {
            swap(current, closest);
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
    time_t now = time(NULL) + fgGetLong("/sim/time/warp");
    if (i == activeTraffic.end() || (activeTraffic.size() == 0)) {
        SG_LOG(SG_GENERAL, SG_ALERT,
               "AI error: Trying to access non-existing aircraft in FGGroundNetwork::checkHoldPosition at " << SG_ORIGIN);
    }
    current = i;
    // 
    if (current->getAircraft()->getTakeOffStatus() == 1) {
        current->setHoldPosition(true);
        return;
    }
    if (current->getAircraft()->getTakeOffStatus() == 2) {
        //cerr << current->getAircraft()->getCallSign() << ". Taxi in position and hold" << endl;
        current->setHoldPosition(false);
        current->clearSpeedAdjustment();
        return;
    }
    bool origStatus = current->hasHoldPosition();
    current->setHoldPosition(false);
    SGGeod curr(SGGeod::fromDegM(lon, lat, alt));
    int currentRoute = i->getCurrentPosition();
    int nextRoute;
    if (i->getIntentions().size()) {
        nextRoute    = (*(i->getIntentions().begin()));
    } else {
        nextRoute = 0;
    }       
    if (currentRoute > 0) {
        FGTaxiSegment *tx = findSegment(currentRoute);
        FGTaxiSegment *nx;
        if (nextRoute) {
            nx = findSegment(nextRoute);
        } else {
            nx = tx;
        }
        //if (tx->hasBlock(now) || nx->hasBlock(now) ) {
        //   current->setHoldPosition(true);
        //}
        SGGeod start(SGGeod::fromDeg((i->getLongitude()), (i->getLatitude())));
        SGGeod end  (SGGeod::fromDeg(nx->getStart()->getLongitude(), nx->getStart()->getLatitude()));

        double distance = SGGeodesy::distanceM(start, end);
        if (nx->hasBlock(now) && (distance < i->getRadius() * 4)) {
            current->setHoldPosition(true);
        } else {
            intVecIterator ivi = i->getIntentions().begin();
            while (ivi != i->getIntentions().end()) {
                if ((*ivi) > 0) {
                    distance += segments[(*ivi)-1]->getLength();
                    if ((segments[(*ivi)-1]->hasBlock(now)) && (distance < i->getRadius() * 4)) {
                        current->setHoldPosition(true);
                        break;
                    }
                }
                ivi++;
            }
        } 
    }
    bool currStatus = current->hasHoldPosition();
    current->setHoldPosition(origStatus);
    // Either a Hold Position or a resume taxi transmission has been issued
    if ((now - lastTransmission) > 2) {
        available = true;
    }
    if (current->getState() == 0) {
        if ((origStatus != currStatus) && available) {
            //cerr << "Issueing hold short instrudtion " << currStatus << " " << available << endl;
            if (currStatus == true) { // No has a hold short instruction
                transmit(&(*current), &(*parent->getDynamics()), MSG_HOLD_POSITION, ATC_GROUND_TO_AIR, true);
                //cerr << "Transmittin hold short instrudtion " << currStatus << " " << available << endl;
                current->setState(1);
            } else {
                transmit(&(*current), &(*parent->getDynamics()), MSG_RESUME_TAXI, ATC_GROUND_TO_AIR, true);
                //cerr << "Transmittig resume instrudtion " << currStatus << " " << available << endl;
                current->setState(2);
            }
            lastTransmission = now;
            available = false;
            // Don't act on the changed instruction until the transmission is confirmed
            // So set back to original status
            //cerr << "Current state " << current->getState() << endl;
        }

    }
    // 6 = Report runway
    // 7 = Acknowledge report runway
    // 8 = Switch tower frequency
    //9 = Acknowledge switch tower frequency

    //int state = current->getState();
    if (checkTransmissionState(1,1, current, now, MSG_ACKNOWLEDGE_HOLD_POSITION, ATC_AIR_TO_GROUND)) {
        current->setState(0);
        current->setHoldPosition(true);
    }
    if (checkTransmissionState(2,2, current, now, MSG_ACKNOWLEDGE_RESUME_TAXI, ATC_AIR_TO_GROUND)) {
        current->setState(0);
        current->setHoldPosition(false);
    }
    if (current->getAircraft()->getTakeOffStatus() && (current->getState() == 0)) {
        //cerr << "Scheduling " << current->getAircraft()->getCallSign() << " for hold short" << endl;
        current->setState(6);
    }
    if (checkTransmissionState(6,6, current, now, MSG_REPORT_RUNWAY_HOLD_SHORT, ATC_AIR_TO_GROUND)) {
    }
    if (checkTransmissionState(7,7, current, now, MSG_ACKNOWLEDGE_REPORT_RUNWAY_HOLD_SHORT, ATC_GROUND_TO_AIR)) {
    }
    if (checkTransmissionState(8,8, current, now, MSG_SWITCH_TOWER_FREQUENCY, ATC_GROUND_TO_AIR)) {
    }
    if (checkTransmissionState(9,9, current, now, MSG_ACKNOWLEDGE_SWITCH_TOWER_FREQUENCY, ATC_AIR_TO_GROUND)) {
    }



    //current->setState(0);
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
               "AI error: Trying to access non-existing aircraft in FGGroundNetwork::checkForCircularWaits at " << SG_ORIGIN);
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
               "AI error: checking ATC instruction for aircraft without traffic record at " << SG_ORIGIN);
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
               "AI error: requesting ATC instruction for aircraft without traffic record at " << SG_ORIGIN);
    } else {
        return i->getInstruction();
    }
    return FGATCInstruction();
}

// Note that this function is copied from simgear. for maintanance purposes, it's probabtl better to make a general function out of that.
static void WorldCoordinate(osg::Matrix& obj_pos, double lat,
                            double lon, double elev, double hdg, double slope)
{
    SGGeod geod = SGGeod::fromDegM(lon, lat, elev);
    obj_pos = makeZUpFrame(geod);
    // hdg is not a compass heading, but a counter-clockwise rotation
    // around the Z axis
    obj_pos.preMult(osg::Matrix::rotate(hdg * SGD_DEGREES_TO_RADIANS,
                                        0.0, 0.0, 1.0));
    obj_pos.preMult(osg::Matrix::rotate(slope * SGD_DEGREES_TO_RADIANS,
                                        0.0, 1.0, 0.0));
}




void FGGroundNetwork::render(bool visible)
{

    SGMaterialLib *matlib = globals->get_matlib();
    if (group) {
        //int nr = ;
        globals->get_scenery()->get_scene_graph()->removeChild(group);
        //while (group->getNumChildren()) {
        //  cerr << "Number of children: " << group->getNumChildren() << endl;
        //simgear::EffectGeode* geode = (simgear::EffectGeode*) group->getChild(0);
        //osg::MatrixTransform *obj_trans = (osg::MatrixTransform*) group->getChild(0);
        //geode->releaseGLObjects();
        //group->removeChild(geode);
        //delete geode;
        group = 0;
    }
    if (visible) {
        group = new osg::Group;
        FGScenery * local_scenery = globals->get_scenery();
        // double elevation_meters = 0.0;
//        double elevation_feet = 0.0;
        time_t now = time(NULL) + fgGetLong("/sim/time/warp");
        //for ( FGTaxiSegmentVectorIterator i = segments.begin(); i != segments.end(); i++) {
        //double dx = 0;
        for   (TrafficVectorIterator i = activeTraffic.begin(); i != activeTraffic.end(); i++) {
            // Handle start point
            int pos = i->getCurrentPosition() - 1;
            if (pos >= 0) {

                SGGeod start(SGGeod::fromDeg((i->getLongitude()), (i->getLatitude())));
                SGGeod end  (SGGeod::fromDeg(segments[pos]->getEnd()->getLongitude(), segments[pos]->getEnd()->getLatitude()));

                double length = SGGeodesy::distanceM(start, end);
                //heading = SGGeodesy::headingDeg(start->getGeod(), end->getGeod());

                double az2, heading; //, distanceM;
                SGGeodesy::inverse(start, end, heading, az2, length);
                double coveredDistance = length * 0.5;
                SGGeod center;
                SGGeodesy::direct(start, heading, coveredDistance, center, az2);
                //cerr << "Active Aircraft : Centerpoint = (" << center.getLatitudeDeg() << ", " << center.getLongitudeDeg() << "). Heading = " << heading << endl;
                ///////////////////////////////////////////////////////////////////////////////
                // Make a helper function out of this
                osg::Matrix obj_pos;
                osg::MatrixTransform *obj_trans = new osg::MatrixTransform;
                obj_trans->setDataVariance(osg::Object::STATIC);
                // Experimental: Calculate slope here, based on length, and the individual elevations
                double elevationStart;
                if (isUserAircraft((i)->getAircraft())) {
                    elevationStart = fgGetDouble("/position/ground-elev-m");
                } else {
                    elevationStart = ((i)->getAircraft()->_getAltitude());
                }
                double elevationEnd   = segments[pos]->getEnd()->getElevationM(parent->getElevation()*SG_FEET_TO_METER);
                //cerr << "Using elevation " << elevationEnd << endl;

                if ((elevationEnd == 0) || (elevationEnd = parent->getElevation())) {
                    SGGeod center2 = end;
                    center2.setElevationM(SG_MAX_ELEVATION_M);
                    if (local_scenery->get_elevation_m( center2, elevationEnd, NULL )) {
//                        elevation_feet = elevationEnd * SG_METER_TO_FEET + 0.5;
                        //elevation_meters += 0.5;
                    }
                    else {
                        elevationEnd = parent->getElevation();
                    }
                    segments[pos]->getEnd()->setElevation(elevationEnd);
                }
                double elevationMean  = (elevationStart + elevationEnd) / 2.0;
                double elevDiff       = elevationEnd - elevationStart;

                double slope = atan2(elevDiff, length) * SGD_RADIANS_TO_DEGREES;

                //cerr << "1. Using mean elevation : " << elevationMean << " and " << slope << endl;

                WorldCoordinate( obj_pos, center.getLatitudeDeg(), center.getLongitudeDeg(), elevationMean+ 0.5, -(heading), slope );

                obj_trans->setMatrix( obj_pos );
                //osg::Vec3 center(0, 0, 0)

                float width = length /2.0;
                osg::Vec3 corner(-width, 0, 0.25f);
                osg::Vec3 widthVec(2*width + 1, 0, 0);
                osg::Vec3 heightVec(0, 1, 0);
                osg::Geometry* geometry;
                geometry = osg::createTexturedQuadGeometry(corner, widthVec, heightVec);
                simgear::EffectGeode* geode = new simgear::EffectGeode;
                geode->setName("test");
                geode->addDrawable(geometry);
                //osg::Node *custom_obj;
                SGMaterial *mat;
                if (segments[pos]->hasBlock(now)) {
                    mat = matlib->find("UnidirectionalTaperRed");
                } else {
                    mat = matlib->find("UnidirectionalTaperGreen");
                }
                if (mat)
                    geode->setEffect(mat->get_effect());
                obj_trans->addChild(geode);
                // wire as much of the scene graph together as we can
                //->addChild( obj_trans );
                group->addChild( obj_trans );
                /////////////////////////////////////////////////////////////////////
            } else {
                //cerr << "BIG FAT WARNING: current position is here : " << pos << endl;
            }
            for (intVecIterator j = (i)->getIntentions().begin(); j != (i)->getIntentions().end(); j++) {
                osg::Matrix obj_pos;
                int k = (*j)-1;
                if (k >= 0) {
                    osg::MatrixTransform *obj_trans = new osg::MatrixTransform;
                    obj_trans->setDataVariance(osg::Object::STATIC);

                    // Experimental: Calculate slope here, based on length, and the individual elevations
                    double elevationStart = segments[k]->getStart()->getElevationM(parent->getElevation()*SG_FEET_TO_METER);
                    double elevationEnd   = segments[k]->getEnd  ()->getElevationM(parent->getElevation()*SG_FEET_TO_METER);
                    if ((elevationStart == 0)  || (elevationStart == parent->getElevation())) {
                        SGGeod center2 = segments[k]->getStart()->getGeod();
                        center2.setElevationM(SG_MAX_ELEVATION_M);
                        if (local_scenery->get_elevation_m( center2, elevationStart, NULL )) {
//                            elevation_feet = elevationStart * SG_METER_TO_FEET + 0.5;
                            //elevation_meters += 0.5;
                        }
                        else {
                            elevationStart = parent->getElevation();
                        }
                        segments[k]->getStart()->setElevation(elevationStart);
                    }
                    if ((elevationEnd == 0) || (elevationEnd == parent->getElevation())) {
                        SGGeod center2 = segments[k]->getEnd()->getGeod();
                        center2.setElevationM(SG_MAX_ELEVATION_M);
                        if (local_scenery->get_elevation_m( center2, elevationEnd, NULL )) {
//                            elevation_feet = elevationEnd * SG_METER_TO_FEET + 0.5;
                            //elevation_meters += 0.5;
                        }
                        else {
                            elevationEnd = parent->getElevation();
                        }
                        segments[k]->getEnd()->setElevation(elevationEnd);
                    }

                    double elevationMean  = (elevationStart + elevationEnd) / 2.0;
                    double elevDiff       = elevationEnd - elevationStart;
                    double length         = segments[k]->getLength();
                    double slope = atan2(elevDiff, length) * SGD_RADIANS_TO_DEGREES;

                    // cerr << "2. Using mean elevation : " << elevationMean << " and " << slope << endl;


                    WorldCoordinate( obj_pos, segments[k]->getLatitude(), segments[k]->getLongitude(), elevationMean+ 0.5, -(segments[k]->getHeading()), slope );

                    obj_trans->setMatrix( obj_pos );
                    //osg::Vec3 center(0, 0, 0)

                    float width = segments[k]->getLength() /2.0;
                    osg::Vec3 corner(-width, 0, 0.25f);
                    osg::Vec3 widthVec(2*width + 1, 0, 0);
                    osg::Vec3 heightVec(0, 1, 0);
                    osg::Geometry* geometry;
                    geometry = osg::createTexturedQuadGeometry(corner, widthVec, heightVec);
                    simgear::EffectGeode* geode = new simgear::EffectGeode;
                    geode->setName("test");
                    geode->addDrawable(geometry);
                    //osg::Node *custom_obj;
                    SGMaterial *mat;
                    if (segments[k]->hasBlock(now)) {
                        mat = matlib->find("UnidirectionalTaperRed");
                    } else {
                        mat = matlib->find("UnidirectionalTaperGreen");
                    }
                    if (mat)
                        geode->setEffect(mat->get_effect());
                    obj_trans->addChild(geode);
                    // wire as much of the scene graph together as we can
                    //->addChild( obj_trans );
                    group->addChild( obj_trans );
                }
            }
            //dx += 0.1;
        }
        globals->get_scenery()->get_scene_graph()->addChild(group);
    }
}

string FGGroundNetwork::getName() {
    return string(parent->getId() + "-ground");
}

void FGGroundNetwork::update(double dt)
{
    time_t now = time(NULL) + fgGetLong("/sim/time/warp");
    for (FGTaxiSegmentVectorIterator tsi = segments.begin(); tsi != segments.end(); tsi++) {
        (*tsi)->unblock(now);
    }
    int priority = 1;
    //sort(activeTraffic.begin(), activeTraffic.end(), compare_trafficrecords);
    // Handle traffic that is under ground control first; this way we'll prevent clutter at the gate areas.
    // Don't allow an aircraft to pushback when a taxiing aircraft is currently using part of the intended route.
    for   (TrafficVectorIterator i = parent->getDynamics()->getStartupController()->getActiveTraffic().begin();
            i != parent->getDynamics()->getStartupController()->getActiveTraffic().end(); i++) {
        i->allowPushBack();
        i->setPriority(priority++);
        // in meters per second;
        double vTaxi = (i->getAircraft()->getPerformance()->vTaxi() * SG_NM_TO_METER) / 3600;
        if (i->isActive(0)) {

            // Check for all active aircraft whether it's current pos segment is
            // an opposite of one of the departing aircraft's intentions
            for (TrafficVectorIterator j = activeTraffic.begin(); j != activeTraffic.end(); j++) {
                int pos = j->getCurrentPosition();
                if (pos > 0) {
                    FGTaxiSegment *seg = segments[pos-1]->opposite();
                    if (seg) {
                        int posReverse = seg->getIndex();
                        for (intVecIterator k = i->getIntentions().begin(); k != i->getIntentions().end(); k++) {
                            if ((*k) == posReverse) {
                                i->denyPushBack();
                                segments[posReverse-1]->block(i->getId(), now, now);
                            }
                        }
                    }
                }
            }
            // if the current aircraft is still allowed to pushback, we can start reserving a route for if by blocking all the entry taxiways.
            if (i->pushBackAllowed()) {
                double length = 0;
                int pos = i->getCurrentPosition();
                if (pos > 0) {
                    FGTaxiSegment *seg = segments[pos-1];
                    FGTaxiNode *node = seg->getEnd();
                    length = seg->getLength();
                    for (FGTaxiSegmentVectorIterator tsi = segments.begin(); tsi != segments.end(); tsi++) {
                        if (((*tsi)->getEnd() == node) && ((*tsi) != seg)) {
                            (*tsi)->block(i->getId(), now, now);
                        }
                    }
                }
                for (intVecIterator j = i->getIntentions().begin(); j != i->getIntentions().end(); j++) {
                    int pos = (*j);
                    if (pos > 0) {
                        FGTaxiSegment *seg = segments[pos-1];
                        FGTaxiNode *node = seg->getEnd();
                        length += seg->getLength();
                        time_t blockTime = now + (length / vTaxi);
                        for (FGTaxiSegmentVectorIterator tsi = segments.begin(); tsi != segments.end(); tsi++) {
                            if (((*tsi)->getEnd() == node) && ((*tsi) != seg)) {
                                (*tsi)->block(i->getId(), blockTime-30, now);
                            }
                        }
                    }
                }
            }
        }
    }
    for   (TrafficVectorIterator i = activeTraffic.begin(); i != activeTraffic.end(); i++) {
        double length = 0;
        double vTaxi = (i->getAircraft()->getPerformance()->vTaxi() * SG_NM_TO_METER) / 3600;
        i->setPriority(priority++);
        int pos = i->getCurrentPosition();
        if (pos > 0) {
            length = segments[pos-1]->getLength();
            if (segments[pos-1]->hasBlock(now)) {
                //SG_LOG(SG_GENERAL, SG_ALERT, "Taxiway incursion for AI aircraft" << i->getAircraft()->getCallSign());
            }

        }
        intVecIterator ivi;
        for (ivi = i->getIntentions().begin(); ivi != i->getIntentions().end(); ivi++) {
            int segIndex = (*ivi);
            if (segIndex > 0) {
                if (segments[segIndex-1]->hasBlock(now))
                    break;
            }
        }
        //after this, ivi points just behind the last valid unblocked taxi segment.
        for (intVecIterator j = i->getIntentions().begin(); j != ivi; j++) {
            int pos = (*j);
            if (pos > 0) {
                FGTaxiSegment *seg = segments[pos-1];
                FGTaxiNode *node = seg->getEnd();
                length += seg->getLength();
                for (FGTaxiSegmentVectorIterator tsi = segments.begin(); tsi != segments.end(); tsi++) {
                    if (((*tsi)->getEnd() == node) && ((*tsi) != seg)) {
                        time_t blockTime = now + (length / vTaxi);
                        (*tsi)->block(i->getId(), blockTime - 30, now);
                    }
                }
            }
        }
    }
}

