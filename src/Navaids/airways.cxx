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

#include "config.h"

#include "airways.hxx"

#include <tuple>
#include <algorithm>
#include <set>

#include <simgear/sg_inlines.h>
#include <simgear/structure/exception.hxx>
#include <simgear/io/iostreams/sgstream.hxx>
#include <simgear/misc/sg_path.hxx>

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

static std::vector<AirwayRef> static_airwaysCache;
typedef SGSharedPtr<FGPositioned> FGPositionedRef;

//////////////////////////////////////////////////////////////////////////////

class AStarOpenNode : public SGReferenced
{
public:
  AStarOpenNode(FGPositionedRef aNode, double aLegDist, 
    int aAirway,
    FGPositionedRef aDest, AStarOpenNode* aPrev) :
    node(aNode),
    previous(aPrev),
    airway(aAirway)
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
  SGSharedPtr<AStarOpenNode> previous;
  int airway;
  double distanceFromStart; // aka 'g(x)'
  double directDistanceToDestination; // aka 'h(x)'
  
  /**
	 * aka 'f(x)'
	 */
	double totalCost() const {
		return distanceFromStart + directDistanceToDestination;
	}
};

using AStarOpenNodeRef = SGSharedPtr<AStarOpenNode>;

////////////////////////////////////////////////////////////////////////////

Airway::Network* Airway::lowLevel()
{
  static Network* static_lowLevel = nullptr;
  
  if (!static_lowLevel) {
      static_lowLevel = new Network;
      static_lowLevel->_networkID = Airway::LowLevel;
  }
  
  return static_lowLevel;
}

Airway::Network* Airway::highLevel()
{
  static Network* static_highLevel = nullptr;
  if (!static_highLevel) {
    static_highLevel = new Network;
      static_highLevel->_networkID = Airway::HighLevel;
  }
  
  return static_highLevel;
}

Airway::Airway(const std::string& aIdent,
               const Level level,
               int dbId,
               int aTop, int aBottom) :
  _ident(aIdent),
  _level(level),
  _cacheId(dbId),
  _topAltitudeFt(aTop),
  _bottomAltitudeFt(aBottom)
{
    assert((level == HighLevel) || (level == LowLevel));
    static_airwaysCache.push_back(this);
}

void Airway::loadAWYDat(const SGPath& path)
{
  std::string identStart, identEnd, name;
  double latStart, lonStart, latEnd, lonEnd;
  int type, base, top;

  sg_gzifstream in( path );
  if ( !in.is_open() ) {
    SG_LOG( SG_NAVAID, SG_ALERT, "Cannot open file: " << path );
    throw sg_io_exception("Could not open airways data", path);
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

    // type = 1; low-altitude (victor)
    // type = 2; high-altitude (jet)
    Network* net = (type == 1) ? lowLevel() : highLevel();
  
    SGGeod startPos(SGGeod::fromDeg(lonStart, latStart)),
      endPos(SGGeod::fromDeg(lonEnd, latEnd));

    if (type == 1) {

    } else if (type == 2) {

    } else {
        SG_LOG(SG_NAVAID, SG_DEV_WARN, "unknown airway type:" << type << " for " << name);
        continue;
    }

    auto pieces = simgear::strutils::split(name, "-");
    for (auto p : pieces) {
        int awy = net->findAirway(p);
        net->addEdge(awy, startPos, identStart, endPos, identEnd);
    }
  } // of file line iteration
}

WayptVec::const_iterator Airway::find(WayptRef wpt) const
{
    assert(!_elements.empty());
    if (wpt->type() == "via") {
      // map vias to their end navaid / fix, so chaining them
      // together works. (Temporary waypoint is discarded after search)
      wpt = new NavaidWaypoint(wpt->source(), wpt->owner());
    }

    return std::find_if(_elements.begin(), _elements.end(),
                        [wpt] (const WayptRef& w)
                        {
                            if (!w) return false;
                            return w->matches(wpt);
                        });
}

bool Airway::canVia(const WayptRef& from, const WayptRef& to) const
{
    loadWaypoints();

    auto fit = find(from);
    auto tit = find(to);

    if ((fit == _elements.end()) || (tit == _elements.end())) {
        return false;
    }
    
    if (fit < tit) {
        // forward progression
        for (++fit; fit != tit; ++fit) {
            if (*fit == nullptr) {
                // traversed an airway discontinuity
                return false;
            }
        }
    } else {
        // reverse progression
        for (--fit; fit != tit; --fit) {
            if (*fit == nullptr) {
                // traversed an airway discontinuity
                return false;
            }
        }
    }

    return true;
}

WayptVec Airway::via(const WayptRef& from, const WayptRef& to) const
{
    loadWaypoints();

    WayptVec v;
    auto fit = find(from);
    auto tit = find(to);

    if ((fit == _elements.end()) || (tit == _elements.end())) {
        throw sg_exception("bad VIA transition points");
    }

    if (fit == tit) {
        // will cause duplicate point but that seems better than
        // return an empty
        v.push_back(*tit);
        return v;
    }

    // establish the ordering of the transitions, i.e are we moving forward or
    // backard along the airway.
    if (fit < tit) {
        // forward progression
        for (++fit; fit != tit; ++fit) {
            v.push_back(*fit);
        }
    } else {
        // reverse progression
        for (--fit; fit != tit; --fit) {
            v.push_back(*fit);
        }
    }

    v.push_back(*tit);
    return v;
}

bool Airway::containsNavaid(const FGPositionedRef &navaid) const
{
    if (!navaid)
        return false;
    
    loadWaypoints();
    auto it = std::find_if(_elements.begin(), _elements.end(),
                           [navaid](WayptRef w)
    {
        if (!w) return false;
        return w->matches(navaid);
    });
    return (it != _elements.end());
}

int Airway::Network::findAirway(const std::string& aName)
{
    const Level level = _networkID;
    auto it = std::find_if(static_airwaysCache.begin(), static_airwaysCache.end(),
                           [aName, level](const AirwayRef& awy)
    { return (awy->_level == level) && (awy->ident() == aName); });
    if (it != static_airwaysCache.end()) {
        return (*it)->_cacheId;
    }

    return NavDataCache::instance()->findAirway(_networkID, aName, true);
}

AirwayRef Airway::findByIdent(const std::string& aIdent, Level level)
{
    auto it = std::find_if(static_airwaysCache.begin(), static_airwaysCache.end(),
                           [aIdent, level](const AirwayRef& awy)
    { 
      if ((level != Both) && (awy->_level != level)) return false;
      return (awy->ident() == aIdent); 
    });
    if (it != static_airwaysCache.end()) {
        return *it;
    }

    auto ndc = NavDataCache::instance();
    int airwayId = 0;
    if (level == Both) {
        airwayId = ndc->findAirway(HighLevel, aIdent, false);
        if (airwayId == 0) {
            level = LowLevel; // not found in HighLevel, try LowLevel
        } else {
            level = HighLevel; // fix up, so Airway ctro see a valid value
        }
    }

    if (airwayId == 0) {
        airwayId = ndc->findAirway(level, aIdent, false);
        if (airwayId == 0) {
            return {};
        }
    }

    return ndc->loadAirway(airwayId);
}

    AirwayRef Airway::loadByCacheId(int cacheId)
    {
        auto it = std::find_if(static_airwaysCache.begin(), static_airwaysCache.end(),
                               [cacheId](const AirwayRef& awy)
                               { return (awy->_cacheId == cacheId); });
        if (it != static_airwaysCache.end()) {
            return *it;
        }

        return NavDataCache::instance()->loadAirway(cacheId);
        ;
    }
    
void Airway::loadWaypoints() const
{
    NavDataCache* ndc = NavDataCache::instance();
    for (auto id : ndc->airwayWaypts(_cacheId)) {
        if (id == 0) {
            _elements.push_back({});
        } else {
            FGPositionedRef pos = ndc->loadById(id);
            auto wp = new NavaidWaypoint(pos, const_cast<Airway*>(this));
            wp->setFlag(WPT_VIA);
            wp->setFlag(WPT_GENERATED);
            _elements.push_back(wp);
        }
    }
}
    
AirwayRef Airway::findByIdentAndVia(const std::string& aIdent, const WayptRef& from, const WayptRef& to)
{
    AirwayRef hi = findByIdent(aIdent, HighLevel);
    if (hi && hi->canVia(from, to)) {
        return hi;
    }
    
    AirwayRef low = findByIdent(aIdent, LowLevel);
    if (low && low->canVia(from, to)) {
        return low;
    }
    
    return nullptr;
}
    
AirwayRef Airway::findByIdentAndNavaid(const std::string& aIdent, const FGPositionedRef nav)
{
    AirwayRef hi = findByIdent(aIdent, HighLevel);
    if (hi && hi->containsNavaid(nav)) {
        return hi;
    }
    
    AirwayRef low = findByIdent(aIdent, LowLevel);
    if (low && low->containsNavaid(nav)) {
        return low;
    }
    
    return nullptr;
}

WayptRef Airway::findEnroute(const std::string &aIdent) const
{
    loadWaypoints();
    auto it = std::find_if(_elements.begin(), _elements.end(),
                           [&aIdent](WayptRef w)
    {
        if (!w) return false;
        return w->ident() == aIdent;
    });
    
    if (it != _elements.end())
        return *it;
    return {};
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
  }
  
  if (!end) {
    SG_LOG(SG_NAVAID, SG_DEBUG, "unknown airways end pt: '" << aEndIdent << "'");
    end = FGPositioned::createUserWaypoint(aEndIdent, aEndPos);
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
  std::tie(from, exactFrom) = findClosestNode(aFrom);
  std::tie(to, exactTo) = findClosestNode(aTo);
  
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
    if (aRef->source()) {
        // we can check directly
        if (inNetwork(aRef->source()->guid())) {
            return std::make_pair(aRef->source(), true);
        }
    }
  
    return findClosestNode(aRef->position());
}

class InAirwayFilter : public FGPositioned::Filter
{
public:
  InAirwayFilter(const Airway::Network* aNet) :
    _net(aNet)
  { ; }
  
  virtual bool pass(FGPositioned* aPos) const
  {
    return _net->inNetwork(aPos->guid());
  }
  
  virtual FGPositioned::Type minType() const
  { return FGPositioned::WAYPOINT; }
  
  virtual FGPositioned::Type maxType() const
  { return FGPositioned::VOR; }
  
private:
  const Airway::Network* _net;
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

FGPositionedRef
Airway::Network::findNodeByIdent(const std::string& ident, const SGGeod& near) const
{
    InAirwayFilter f(this);
    return FGPositioned::findClosestWithIdent(ident, near, &f);
}

/////////////////////////////////////////////////////////////////////////////

typedef vector<AStarOpenNodeRef> OpenNodeHeap;

static void buildWaypoints(AStarOpenNodeRef aNode, WayptVec& aRoute)
{
// count the route length, and hence pre-size aRoute
  size_t count = 0;
  AStarOpenNodeRef n = aNode;
  for (; n != nullptr; ++count, n = n->previous) {;}
  aRoute.resize(count);
  
// run over the route, creating waypoints
  for (n = aNode; n; n=n->previous) {
      // get / create airway to be the owner for this waypoint
      AirwayRef awy = Airway::loadByCacheId(n->airway);
      auto wp = new NavaidWaypoint(n->node, awy);
      if (awy) {
          wp->setFlag(WPT_VIA);
      }
      wp->setFlag(WPT_GENERATED);
      aRoute[--count] = wp;
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
  
  return nullptr;
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
  
  openNodes.push_back(new AStarOpenNode(aStart, 0.0, 0, aDest, nullptr));
  
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
    for (auto other : cache->airwayEdgesFrom(_networkID, xp->guid())) {
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
