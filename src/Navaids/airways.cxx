// airways.cxx - storage of airways network, and routing between nodes
// Written by James Turner, started 2009.
//
// Copyright (C) 2009  Curtis L. Olson
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

#ifndef HAVE_CONFIG_H
# include "config.h"
#endif

#include "airways.hxx"

#include <algorithm>
#include <set>

#include <simgear/sg_inlines.h>
#include <simgear/structure/exception.hxx>
#include <simgear/misc/sgstream.hxx>
#include <simgear/misc/sg_path.hxx>

#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>

#include <Main/globals.hxx>
#include <Navaids/positioned.hxx>
#include <Navaids/waypoint.hxx>
#include <Navaids/NavDataCache.hxx>

using std::make_pair;
using std::string;
using std::set;
using std::vector;

//#define DEBUG_AWY_SEARCH 1

namespace flightgear
{

//////////////////////////////////////////////////////////////////////////////

class AStarOpenNode : public SGReferenced
{
public:
  AStarOpenNode(FGPositionedRef aNode, double aLegDist, 
    int aAirway,
    FGPositionedRef aDest, AStarOpenNode* aPrev) :
    node(aNode),
    airway(aAirway),
    previous(aPrev)
  { 
    distanceFromStart = aLegDist;
    if (previous) {
      distanceFromStart +=  previous->distanceFromStart;
    }
    
		directDistanceToDestination = SGGeodesy::distanceM(node->geod(), aDest->geod());
  }
  
  virtual ~AStarOpenNode()
  {
  }
  
  FGPositionedRef node;
  int airway;
  SGSharedPtr<AStarOpenNode> previous;
  double distanceFromStart; // aka 'g(x)'
  double directDistanceToDestination; // aka 'h(x)'
  
  /**
	 * aka 'f(x)'
	 */
	double totalCost() const {
		return distanceFromStart + directDistanceToDestination;
	}
};

typedef SGSharedPtr<AStarOpenNode> AStarOpenNodeRef;

////////////////////////////////////////////////////////////////////////////

Airway::Network* Airway::lowLevel()
{
  static Network* static_lowLevel = NULL;
  
  if (!static_lowLevel) {
    static_lowLevel = new Network;
    static_lowLevel->_networkID = 1;
  }
  
  return static_lowLevel;
}

Airway::Network* Airway::highLevel()
{
  static Network* static_highLevel = NULL;
  if (!static_highLevel) {
    static_highLevel = new Network;
    static_highLevel->_networkID = 2;
  }
  
  return static_highLevel;
}

Airway::Airway(const std::string& aIdent, double aTop, double aBottom) :
  _ident(aIdent),
  _topAltitudeFt(aTop),
  _bottomAltitudeFt(aBottom)
{
}

void Airway::load(const SGPath& path)
{
  std::string identStart, identEnd, name;
  double latStart, lonStart, latEnd, lonEnd;
  int type, base, top;
  //int airwayIndex = 0;
  //FGNode *n;

  sg_gzifstream in( path.str() );
  if ( !in.is_open() ) {
    SG_LOG( SG_NAVAID, SG_ALERT, "Cannot open file: " << path.str() );
    throw sg_io_exception("Could not open airways data", sg_location(path.str()));
  }
// toss the first two lines of the file
  in >> skipeol;
  in >> skipeol;

// read in each remaining line of the file
  while (!in.eof()) {
    in >> identStart;

    if (identStart == "99") {
      break;
    }
    
    in >> latStart >> lonStart >> identEnd >> latEnd >> lonEnd >> type >> base >> top >> name;
    in >> skipeol;

    // type = 1; low-altitude
    // type = 2; high-altitude
    Network* net = (type == 1) ? lowLevel() : highLevel();
  
    SGGeod startPos(SGGeod::fromDeg(lonStart, latStart)),
      endPos(SGGeod::fromDeg(lonEnd, latEnd));
    
    int awy = net->findAirway(name, top, base);
    net->addEdge(awy, startPos, identStart, endPos, identEnd);
  } // of file line iteration
}

int Airway::Network::findAirway(const std::string& aName, double aTop, double aBase)
{
  return NavDataCache::instance()->findAirway(_networkID, aName);
}

void Airway::Network::addEdge(int aWay, const SGGeod& aStartPos,
  const std::string& aStartIdent, 
  const SGGeod& aEndPos, const std::string& aEndIdent)
{
  FGPositionedRef start = FGPositioned::findClosestWithIdent(aStartIdent, aStartPos);
  FGPositionedRef end = FGPositioned::findClosestWithIdent(aEndIdent, aEndPos);
    
  if (!start) {
    SG_LOG(SG_NAVAID, SG_DEBUG, "unknown airways start pt: '" << aStartIdent << "'");
    start = FGPositioned::createUserWaypoint(aStartIdent, aStartPos);
    return;
  }
  
  if (!end) {
    SG_LOG(SG_NAVAID, SG_DEBUG, "unknown airways end pt: '" << aEndIdent << "'");
    end = FGPositioned::createUserWaypoint(aEndIdent, aEndPos);
    return;
  }
  
  NavDataCache::instance()->insertEdge(_networkID, aWay, start->guid(), end->guid());
}

//////////////////////////////////////////////////////////////////////////////

static double headingDiffDeg(double a, double b)
{
    double rawDiff = b - a;
    SG_NORMALIZE_RANGE(rawDiff, -180.0, 180.0);
    return rawDiff;
}
    
bool Airway::Network::inNetwork(PositionedID posID) const
{
  NetworkMembershipDict::iterator it = _inNetworkCache.find(posID);
  if (it != _inNetworkCache.end()) {
    return it->second; // cached, easy
  }
  
  bool r =  NavDataCache::instance()->isInAirwayNetwork(_networkID, posID);
  _inNetworkCache.insert(it, std::make_pair(posID, r));
  return r;
}

bool Airway::Network::route(WayptRef aFrom, WayptRef aTo, 
  WayptVec& aPath)
{
  if (!aFrom || !aTo) {
    throw sg_exception("invalid waypoints to route between");
  }
  
// find closest nodes on the graph to from/to
// if argument waypoints are directly on the graph (which is frequently the
// case), note this so we don't duplicate them in the output.

  FGPositionedRef from, to;
  bool exactTo, exactFrom;
  boost::tie(from, exactFrom) = findClosestNode(aFrom);
  boost::tie(to, exactTo) = findClosestNode(aTo);
  
#ifdef DEBUG_AWY_SEARCH
  SG_LOG(SG_NAVAID, SG_INFO, "from:" << from->ident() << "/" << from->name());
  SG_LOG(SG_NAVAID, SG_INFO, "to:" << to->ident() << "/" << to->name());
#endif

  bool ok = search2(from, to, aPath);
  if (!ok) {
    return false;
  }
  
  return cleanGeneratedPath(aFrom, aTo, aPath, exactTo, exactFrom);
}
  
bool Airway::Network::cleanGeneratedPath(WayptRef aFrom, WayptRef aTo, WayptVec& aPath,
                                bool exactTo, bool exactFrom)
{
  // path cleaning phase : various cases to handle here.
  // if either the TO or FROM waypoints were 'exact', i.e part of the enroute
  // structure, we don't want to duplicate them. This happens frequently with
  // published SIDs and STARs.
  // secondly, if the waypoints are NOT on the enroute structure, the course to
  // them may be a significant dog-leg. Check how the leg course deviates
  // from the direct course FROM->TO, and delete the first/last leg if it's more
  // than 90 degrees out.
  // note we delete a maximum of one leg, and no more. This is a heuristic - we
  // could check the next (previous) legs, but at some point we'll end up
  // deleting too much.
  
  const double MAX_DOG_LEG = 90.0;
  double enrouteCourse = SGGeodesy::courseDeg(aFrom->position(), aTo->position()),
  finalLegCourse = SGGeodesy::courseDeg(aPath.back()->position(), aTo->position());
  
  bool isDogLeg = fabs(headingDiffDeg(enrouteCourse, finalLegCourse)) > MAX_DOG_LEG;
  if (exactTo || isDogLeg) {
    aPath.pop_back();
  }
  
  // edge case - if from and to are equal, which can happen, don't
  // crash here. This happens routing EGPH -> EGCC; 'DCS' is common
  // to the EGPH departure and EGCC STAR.
  if (aPath.empty()) {
    return true;
  }
  
  double initialLegCourse = SGGeodesy::courseDeg(aFrom->position(), aPath.front()->position());
  isDogLeg = fabs(headingDiffDeg(enrouteCourse, initialLegCourse)) > MAX_DOG_LEG;
  if (exactFrom || isDogLeg) {
    aPath.erase(aPath.begin());
  }

  return true;
}

std::pair<FGPositionedRef, bool> 
Airway::Network::findClosestNode(WayptRef aRef)
{
  return findClosestNode(aRef->position());
}

class InAirwayFilter : public FGPositioned::Filter
{
public:
  InAirwayFilter(Airway::Network* aNet) :
    _net(aNet)
  { ; }
  
  virtual bool pass(FGPositioned* aPos) const
  {
    return _net->inNetwork(aPos->guid());
  }
  
  virtual FGPositioned::Type minType() const
  { return FGPositioned::WAYPOINT; }
  
  virtual FGPositioned::Type maxType() const
  { return FGPositioned::NDB; }
  
private:
  Airway::Network* _net;
};

std::pair<FGPositionedRef, bool> 
Airway::Network::findClosestNode(const SGGeod& aGeod)
{
  InAirwayFilter f(this);
  FGPositionedRef r = FGPositioned::findClosest(aGeod, 800.0, &f);
  bool exact = false;
  
  if (r && (SGGeodesy::distanceM(aGeod, r->geod()) < 100.0)) {
    exact = true; // within 100 metres, let's call that exact
  }
  
  return make_pair(r, exact);
}

/////////////////////////////////////////////////////////////////////////////

typedef vector<AStarOpenNodeRef> OpenNodeHeap;

static void buildWaypoints(AStarOpenNodeRef aNode, WayptVec& aRoute)
{
// count the route length, and hence pre-size aRoute
  int count = 0;
  AStarOpenNodeRef n = aNode;
  for (; n != NULL; ++count, n = n->previous) {;}
  aRoute.resize(count);
  
// run over the route, creating waypoints
  for (n = aNode; n; n=n->previous) {
    aRoute[--count] = new NavaidWaypoint(n->node, NULL);
  }
}

/**
 * Inefficent (linear) helper to find an open node in the heap
 */
static AStarOpenNodeRef
findInOpen(const OpenNodeHeap& aHeap, FGPositioned* aPos)
{
  for (unsigned int i=0; i<aHeap.size(); ++i) {
    if (aHeap[i]->node == aPos) {
      return aHeap[i];
    }
  }
  
  return NULL;
}

class HeapOrder
{
public:
  bool operator()(AStarOpenNode* a, AStarOpenNode* b)
  {
    return a->totalCost() > b->totalCost();
  }
};

bool Airway::Network::search2(FGPositionedRef aStart, FGPositionedRef aDest,
  WayptVec& aRoute)
{  
  typedef set<PositionedID> ClosedNodeSet;
  
  OpenNodeHeap openNodes;
  ClosedNodeSet closedNodes;
  HeapOrder ordering;
  
  openNodes.push_back(new AStarOpenNode(aStart, 0.0, 0, aDest, NULL));
  
// A* open node iteration
  while (!openNodes.empty()) {
    std::pop_heap(openNodes.begin(), openNodes.end(), ordering);
    AStarOpenNodeRef x = openNodes.back();
    FGPositioned* xp = x->node;    
    openNodes.pop_back();
    closedNodes.insert(xp->guid());
  
#ifdef DEBUG_AWY_SEARCH
    SG_LOG(SG_NAVAID, SG_INFO, "x:" << xp->ident() << ", f(x)=" << x->totalCost());
#endif
    
  // check if xp is the goal; if so we're done, since there cannot be an open
  // node with lower f(x) value.
    if (xp == aDest) {
      buildWaypoints(x, aRoute);
      return true;
    }
    
  // adjacent (neighbour) iteration
    NavDataCache* cache = NavDataCache::instance();
    BOOST_FOREACH(AirwayEdge other, cache->airwayEdgesFrom(_networkID, xp->guid())) {
      if (closedNodes.count(other.second)) {
        continue; // closed, ignore
      }

      FGPositioned* yp = cache->loadById(other.second);
      double edgeDistanceM = SGGeodesy::distanceM(xp->geod(), yp->geod());
      AStarOpenNodeRef y = findInOpen(openNodes, yp);
      if (y) { // already open
        double g = x->distanceFromStart + edgeDistanceM;
        if (g > y->distanceFromStart) {
          // worse path, ignore
#ifdef DEBUG_AWY_SEARCH
          SG_LOG(SG_NAVAID, SG_INFO, "\tabandoning " << yp->ident() <<
           " path is worse: g(y)" << y->distanceFromStart << ", g'=" << g);
#endif
          continue;
        }
        
      // we need to update y. Unfortunately this means rebuilding the heap,
      // since y's score can change arbitrarily
#ifdef DEBUG_AWY_SEARCH
        SG_LOG(SG_NAVAID, SG_INFO, "\tfixing up previous for new path to " << yp->ident() << ", d =" << g);
#endif
        y->previous = x;
        y->distanceFromStart = g;
        y->airway = other.first;
        std::make_heap(openNodes.begin(), openNodes.end(), ordering);
      } else { // not open, insert a new node for y into the heap
        y = new AStarOpenNode(yp, edgeDistanceM, other.first, aDest, x);
#ifdef DEBUG_AWY_SEARCH
        SG_LOG(SG_NAVAID, SG_INFO, "\ty=" << yp->ident() << ", f(y)=" << y->totalCost());
#endif
        openNodes.push_back(y);
        std::push_heap(openNodes.begin(), openNodes.end(), ordering);
      }
    } // of neighbour iteration
  } // of open node iteration
  
  SG_LOG(SG_NAVAID, SG_INFO, "A* failed to find route");
  return false;
}

} // of namespace flightgear
