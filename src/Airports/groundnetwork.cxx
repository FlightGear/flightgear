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

#include "groundnetwork.hxx"

#include <cmath>
#include <algorithm>
#include <fstream>
#include <map>
#include <boost/foreach.hpp>

#include <simgear/debug/logstream.hxx>
#include <simgear/scene/util/OsgMath.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/timing/timestamp.hxx>

#include <Airports/airport.hxx>
#include <Airports/runways.hxx>

#include <Scenery/scenery.hxx>

using std::string;

/***************************************************************************
 * FGTaxiSegment
 **************************************************************************/

FGTaxiSegment::FGTaxiSegment(FGTaxiNode* aStart, FGTaxiNode* aEnd) :
  startNode(aStart),
  endNode(aEnd),
  isActive(0),
  index(0),
  oppositeDirection(0)
{
    if (!aStart || !aEnd) {
        throw sg_exception("Missing node arguments creating FGTaxiSegment");
    }
}

SGGeod FGTaxiSegment::getCenter() const
{
  FGTaxiNode* start(getStart()), *end(getEnd());
  double heading, length, az2;
  SGGeodesy::inverse(start->geod(), end->geod(), heading, az2, length);
  return SGGeodesy::direct(start->geod(), heading, length * 0.5);
}

FGTaxiNodeRef FGTaxiSegment::getEnd() const
{
  return const_cast<FGTaxiNode*>(endNode);
}

FGTaxiNodeRef FGTaxiSegment::getStart() const
{
  return const_cast<FGTaxiNode*>(startNode);
}

double FGTaxiSegment::getLength() const
{
  return dist(getStart()->cart(), getEnd()->cart());
}

double FGTaxiSegment::getHeading() const
{
  return SGGeodesy::courseDeg(getStart()->geod(), getEnd()->geod());
}


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
bool FGTaxiRoute::next(FGTaxiNodeRef& node, int *rte)
{
    if (nodes.size() != (routes.size()) + 1) {
        SG_LOG(SG_GENERAL, SG_ALERT, "ALERT: Misconfigured TaxiRoute : " << nodes.size() << " " << routes.size());
        throw sg_range_exception("Misconfigured taxi route");
    }
    if (currNode == nodes.end())
        return false;
    node = *(currNode);
    if (currNode != nodes.begin()) {
        *rte = *(currRoute);
        currRoute++;
    } else {
        // Handle special case for the first node. 
        *rte = -1 * *(currRoute);
    }
    currNode++;
    return true;
};

/***************************************************************************
 * FGGroundNetwork()
 **************************************************************************/

FGGroundNetwork::FGGroundNetwork(FGAirport* airport) :
    parent(airport)
{
    hasNetwork = false;
    version = 0;
    networkInitialized = false;
}

FGGroundNetwork::~FGGroundNetwork()
{

  BOOST_FOREACH(FGTaxiSegment* seg, segments) {
    delete seg;
  }

  // owning references to ground-net nodes will also drop

  SG_LOG(SG_NAVAID, SG_INFO, "destroying ground net for " << parent->ident());
}

void FGGroundNetwork::init()
{
    if (networkInitialized) {
        SG_LOG(SG_GENERAL, SG_WARN, "duplicate ground-network init");
        return;
    }

    hasNetwork = true;
    int index = 1;  
  
  // establish pairing of segments
    BOOST_FOREACH(FGTaxiSegment* segment, segments) {
      segment->setIndex(index++);
      
      if (segment->oppositeDirection) {
        continue; // already established
      }
      
      FGTaxiSegment* opp = findSegment(segment->endNode, segment->startNode);
      if (opp) {
        assert(opp->oppositeDirection == NULL);
        segment->oppositeDirection = opp;
        opp->oppositeDirection = segment;
      }
    }
  
    networkInitialized = true;
}

FGTaxiNodeRef FGGroundNetwork::findNearestNode(const SGGeod & aGeod) const
{
    double d = DBL_MAX;
    SGVec3d cartPos = SGVec3d::fromGeod(aGeod);
    FGTaxiNodeRef result;

    FGTaxiNodeVector::const_iterator it;
    for (it = m_nodes.begin(); it != m_nodes.end(); ++it) {
        double localDistanceSqr = distSqr(cartPos, (*it)->cart());
        if (localDistanceSqr < d) {
            d = localDistanceSqr;
            result = *it;
        }
    }

    return result;
}

FGTaxiNodeRef FGGroundNetwork::findNearestNodeOnRunway(const SGGeod & aGeod, FGRunway* aRunway) const
{
    SG_UNUSED(aRunway);

    double d = DBL_MAX;
    SGVec3d cartPos = SGVec3d::fromGeod(aGeod);
    FGTaxiNodeRef result = 0;
    FGTaxiNodeVector::const_iterator it;
    for (it = m_nodes.begin(); it != m_nodes.end(); ++it) {
        if (!(*it)->getIsOnRunway())
            continue;

        double localDistanceSqr = distSqr(cartPos, (*it)->cart());
        if (localDistanceSqr < d) {
            d = localDistanceSqr;
            result = *it;
        }
    }

    return result;
}

FGTaxiSegment *FGGroundNetwork::findOppositeSegment(unsigned int index) const
{
    FGTaxiSegment* seg = findSegment(index);
    if (!seg)
        return NULL;
    return seg->opposite();
}

const FGParkingList &FGGroundNetwork::allParkings() const
{
    return m_parkings;
}

FGTaxiSegment *FGGroundNetwork::findSegment(unsigned idx) const
{ 
    if ((idx > 0) && (idx <= segments.size()))
        return segments[idx - 1];
    else {
        //cerr << "Alert: trying to find invalid segment " << idx << endl;
        return 0;
    }
}

FGTaxiSegment* FGGroundNetwork::findSegment(const FGTaxiNode* from, const FGTaxiNode* to) const
{
  if (from == 0) {
    return NULL;
  }
  
  // completely boring linear search of segments. Can be improved if/when
  // this ever becomes a hot-spot
    BOOST_FOREACH(FGTaxiSegment* seg, segments) {
      if (seg->startNode != from) {
        continue;
      }
      
      if ((to == 0) || (seg->endNode == to)) {
        return seg;
      }
    }
  
    return NULL; // not found
}

static int edgePenalty(FGTaxiNode* tn)
{
  return (tn->type() == FGPositioned::PARKING ? 10000 : 0) +
    (tn->getIsOnRunway() ? 1000 : 0);
}

class ShortestPathData
{
public:
  ShortestPathData() :
    score(HUGE_VAL)
  {}
  
  double score;
  FGTaxiNodeRef previousNode;
};

FGTaxiRoute FGGroundNetwork::findShortestRoute(FGTaxiNode* start, FGTaxiNode* end, bool fullSearch)
{
    if (!start || !end) {
        throw sg_exception("Bad arguments to findShortestRoute");
    }
//implements Dijkstra's algorithm to find shortest distance route from start to end
//taken from http://en.wikipedia.org/wiki/Dijkstra's_algorithm
    FGTaxiNodeVector unvisited(m_nodes);
    std::map<FGTaxiNode*, ShortestPathData> searchData;

    searchData[start].score = 0.0;

    while (!unvisited.empty()) {
        // find lowest scored unvisited
        FGTaxiNodeRef best = unvisited.front();
        BOOST_FOREACH(FGTaxiNodeRef i, unvisited) {
            if (searchData[i].score < searchData[best].score) {
                best = i;
            }
        }
      
      // remove 'best' from the unvisited set
        FGTaxiNodeVectorIterator newend =
            remove(unvisited.begin(), unvisited.end(), best);
        unvisited.erase(newend, unvisited.end());

        if (best == end) { // found route or best not connected
            break;
        }
      
        BOOST_FOREACH(FGTaxiNodeRef target, segmentsFrom(best)) {
            double edgeLength = dist(best->cart(), target->cart());
            double alt = searchData[best].score + edgeLength + edgePenalty(target);
            if (alt < searchData[target].score) {    // Relax (u,v)
                searchData[target].score = alt;
                searchData[target].previousNode = best;
            }
        } // of outgoing arcs/segments from current best node iteration
    } // of unvisited nodes remaining

    if (searchData[end].score == HUGE_VAL) {
        // no valid route found
        if (fullSearch) {
            SG_LOG(SG_GENERAL, SG_ALERT,
                   "Failed to find route from waypoint " << start << " to "
                   << end << " at " << parent->getId());
        }
      
        return FGTaxiRoute();
    }
  
    // assemble route from backtrace information
    FGTaxiNodeVector nodes;
    intVec routes;
    FGTaxiNode *bt = end;
    
    while (searchData[bt].previousNode != 0) {
        nodes.push_back(bt);
        FGTaxiSegment *segment = findSegment(searchData[bt].previousNode, bt);
        int idx = segment->getIndex();
        routes.push_back(idx);
        bt = searchData[bt].previousNode;
        
    }
    nodes.push_back(start);
    reverse(nodes.begin(), nodes.end());
    reverse(routes.begin(), routes.end());
    return FGTaxiRoute(nodes, routes, searchData[end].score, 0);
}

void FGGroundNetwork::unblockAllSegments(time_t now)
{
    FGTaxiSegmentVector::iterator tsi;
    for ( tsi = segments.begin(); tsi != segments.end(); tsi++) {
        (*tsi)->unblock(now);
    }
}

void FGGroundNetwork::blockSegmentsEndingAt(FGTaxiSegment *seg, int blockId, time_t blockTime, time_t now)
{
    if (!seg)
        throw sg_exception("Passed invalid segment");

    FGTaxiNode *node = seg->getEnd();
    FGTaxiSegmentVector::iterator tsi;
    for ( tsi = segments.begin(); tsi != segments.end(); tsi++) {
        FGTaxiSegment* otherSegment = *tsi;
        if ((otherSegment->getEnd() == node) && (otherSegment != seg)) {
            otherSegment->block(blockId, blockTime, now);
        }
    }
}

FGTaxiNodeRef FGGroundNetwork::findNodeByIndex(int index) const
{
   FGTaxiNodeVector::const_iterator it;
   for (it = m_nodes.begin(); it != m_nodes.end(); ++it) {
       if ((*it)->getIndex() == index) {
           return *it;
       }
   }

   return FGTaxiNodeRef();
}

FGParkingRef FGGroundNetwork::getParkingByIndex(unsigned int index) const
{
    FGTaxiNodeRef tn = findNodeByIndex(index);
    if (!tn.valid() || (tn->type() != FGPositioned::PARKING)) {
        return FGParkingRef();
    }

    return FGParkingRef(static_cast<FGParking*>(tn.ptr()));
}

void FGGroundNetwork::addSegment(const FGTaxiNodeRef &from, const FGTaxiNodeRef &to)
{
    FGTaxiSegment* seg = new FGTaxiSegment(from, to);
    segments.push_back(seg);

    FGTaxiNodeVector::iterator it = std::find(m_nodes.begin(), m_nodes.end(), from);
    if (it == m_nodes.end()) {
        m_nodes.push_back(from);
    }

    it = std::find(m_nodes.begin(), m_nodes.end(), to);
    if (it == m_nodes.end()) {
        m_nodes.push_back(to);
    }
}

void FGGroundNetwork::addParking(const FGParkingRef &park)
{
    m_parkings.push_back(park);


    FGTaxiNodeVector::iterator it = std::find(m_nodes.begin(), m_nodes.end(), park);
    if (it == m_nodes.end()) {
        m_nodes.push_back(park);
    }
}

FGTaxiNodeVector FGGroundNetwork::segmentsFrom(const FGTaxiNodeRef &from) const
{
    FGTaxiNodeVector result;
    FGTaxiSegmentVector::const_iterator it;
    for (it = segments.begin(); it != segments.end(); ++it) {
        if ((*it)->getStart() == from) {
            result.push_back((*it)->getEnd());
        }
    }

    return result;
}

const intVec& FGGroundNetwork::getTowerFrequencies() const
{
    return freqTower;
}

const intVec& FGGroundNetwork::getGroundFrequencies() const
{
    return freqGround;
}

