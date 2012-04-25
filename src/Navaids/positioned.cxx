// positioned.cxx - base class for objects which are positioned 
//
// Copyright (C) 2008 James Turner
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
#  include "config.h"
#endif

#include "positioned.hxx"

#include <map>
#include <set>
#include <algorithm> // for sort
#include <queue>
#include <memory>

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <osg/Math> // for osg::isNaN

#include <simgear/timing/timestamp.hxx>
#include <simgear/props/props.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/math/SGGeometry.hxx>
#include <simgear/sg_inlines.h>
#include <simgear/structure/commands.hxx>

#include "Airports/simple.hxx"
#include "Main/fg_props.hxx"

typedef std::multimap<std::string, FGPositioned*> NamedPositionedIndex;
typedef std::pair<NamedPositionedIndex::const_iterator, NamedPositionedIndex::const_iterator> NamedIndexRange;

using std::lower_bound;
using std::upper_bound;

static NamedPositionedIndex global_identIndex;
static NamedPositionedIndex global_nameIndex;

//////////////////////////////////////////////////////////////////////////////

namespace Octree
{

const double LEAF_SIZE = SG_NM_TO_METER * 8.0;
const double LEAF_SIZE_SQR = LEAF_SIZE * LEAF_SIZE;

/**
 * Decorate an object with a double value, and use that value to order 
 * items, for the purpoises of the STL algorithms
 */
template <class T>
class Ordered
{
public:
    Ordered(const T& v, double x) :
        _order(x),
        _inner(v)
    {
    }
    
    Ordered(const Ordered<T>& a) :
        _order(a._order),
        _inner(a._inner)
    {
    }
    
    Ordered<T>& operator=(const Ordered<T>& a)
    {
        _order = a._order;
        _inner = a._inner;
        return *this;
    }
    
    bool operator<(const Ordered<T>& other) const
    {
        return _order < other._order;
    }
    
    bool operator>(const Ordered<T>& other) const
    {
        return _order > other._order;
    }
    
    const T& get() const
        { return _inner; }
    
    double order() const
        { return _order; }
    
private:    
    double _order;
    T _inner;
};

class Node;
typedef Ordered<Node*> OrderedNode;
typedef std::greater<OrderedNode> FNPQCompare; 

/**
 * the priority queue is fundamental to our search algorithm. When searching,
 * we know the front of the queue is the nearest unexpanded node (to the search
 * location). The default STL pqueue returns the 'largest' item from top(), so
 * to get the smallest, we need to replace the default Compare functor (less<>)
 * with greater<>.
 */
typedef std::priority_queue<OrderedNode, std::vector<OrderedNode>, FNPQCompare> FindNearestPQueue;

typedef Ordered<FGPositioned*> OrderedPositioned;
typedef std::vector<OrderedPositioned> FindNearestResults;

Node* global_spatialOctree = NULL;

/**
 * Octree node base class, tracks its bounding box and provides various
 * queries relating to it
 */
class Node
{
public:
    bool contains(const SGVec3d& aPos) const
    {
        return intersects(aPos, _box);
    }

    double distSqrToNearest(const SGVec3d& aPos) const
    {
        return distSqr(aPos, _box.getClosestPoint(aPos));
    }
    
    virtual void insert(FGPositioned* aP) = 0;
    
    virtual void visit(const SGVec3d& aPos, double aCutoff, 
      FGPositioned::Filter* aFilter, 
      FindNearestResults& aResults, FindNearestPQueue&) = 0;
protected:
    Node(const SGBoxd &aBox) :
        _box(aBox)
    {
    }
    
    const SGBoxd _box;
};

class Leaf : public Node
{
public:
    Leaf(const SGBoxd& aBox) :
        Node(aBox)
    {
    }
    
    const FGPositioned::List& members() const
    { return _members; }
    
    virtual void insert(FGPositioned* aP)
    {
        _members.push_back(aP);
    }
    
    virtual void visit(const SGVec3d& aPos, double aCutoff, 
      FGPositioned::Filter* aFilter, 
      FindNearestResults& aResults, FindNearestPQueue&)
    {
        int previousResultsSize = aResults.size();
        int addedCount = 0;
        
        for (unsigned int i=0; i<_members.size(); ++i) {
            FGPositioned* p = _members[i];
            double d2 = distSqr(aPos, p->cart());
            if (d2 > aCutoff) {
                continue;
            }
            
            if (aFilter) {
              if (aFilter->hasTypeRange() && !aFilter->passType(p->type())) {
                continue;
              }
      
              if (!aFilter->pass(p)) {
                continue;
              }
            } // of have a filter

            ++addedCount;
            aResults.push_back(OrderedPositioned(p, d2));
        }
        
        if (addedCount == 0) {
          return;
        }
        
      // keep aResults sorted
        // sort the new items, usually just one or two items
        std::sort(aResults.begin() + previousResultsSize, aResults.end());
        
        // merge the two sorted ranges together - in linear time
        std::inplace_merge(aResults.begin(), 
          aResults.begin() + previousResultsSize, aResults.end());
      }
private:
    FGPositioned::List _members;
};

class Branch : public Node
{
public:
    Branch(const SGBoxd& aBox) :
        Node(aBox)
    {
        memset(children, 0, sizeof(Node*) * 8);
    }
    
    virtual void insert(FGPositioned* aP)
    {
        SGVec3d cart(aP->cart());
        assert(contains(cart));
        int childIndex = 0;
        
        SGVec3d center(_box.getCenter());
    // tests must match indices in SGbox::getCorner
        if (cart.x() < center.x()) {
            childIndex += 1;
        }
        
        if (cart.y() < center.y()) {
            childIndex += 2;
        }
        
        if (cart.z() < center.z()) {
            childIndex += 4;
        }
        
        Node* child = children[childIndex];
        if (!child) { // lazy building of children
            SGBoxd cb(boxForChild(childIndex));            
            double d2 = dot(cb.getSize(), cb.getSize());
            if (d2 < LEAF_SIZE_SQR) {
                child = new Leaf(cb);
            } else {
                child = new Branch(cb);
            }
            
            children[childIndex] = child; 
        }
        
        child->insert(aP);
    }
    
    virtual void visit(const SGVec3d& aPos, double aCutoff, 
      FGPositioned::Filter*, 
      FindNearestResults&, FindNearestPQueue& aQ)
    {    
        for (unsigned int i=0; i<8; ++i) {
            if (!children[i]) {
                continue;
            }
            
            double d2 = children[i]->distSqrToNearest(aPos);
            if (d2 > aCutoff) {
                continue; // exceeded cutoff
            }
            
            aQ.push(Ordered<Node*>(children[i], d2));
        } // of child iteration
    }
    
    
private:
    /**
     * Return the box for a child touching the specified corner
     */
    SGBoxd boxForChild(unsigned int aCorner) const
    {
        SGBoxd r(_box.getCenter());
        r.expandBy(_box.getCorner(aCorner));
        return r;
    }
    
    Node* children[8];
};

void findNearestN(const SGVec3d& aPos, unsigned int aN, double aCutoffM, FGPositioned::Filter* aFilter, FGPositioned::List& aResults)
{
    aResults.clear();
    FindNearestPQueue pq;
    FindNearestResults results;
    pq.push(Ordered<Node*>(global_spatialOctree, 0));
    double cut = aCutoffM * aCutoffM;
    
    while (!pq.empty()) {
        if (!results.empty()) {
          // terminate the search if we have sufficent results, and we are
          // sure no node still on the queue contains a closer match
          double furthestResultOrder = results.back().order();
          if ((results.size() >= aN) && (furthestResultOrder < pq.top().order())) {
            break;
          }
        }
        
        Node* nd = pq.top().get();
        pq.pop();
        
        nd->visit(aPos, cut, aFilter, results, pq);
    } // of queue iteration
    
    // depending on leaf population, we may have (slighty) more results
    // than requested
    unsigned int numResults = std::min((unsigned int) results.size(), aN);
  // copy results out
    aResults.resize(numResults);
    for (unsigned int r=0; r<numResults; ++r) {
      aResults[r] = results[r].get();
    }
}

void findAllWithinRange(const SGVec3d& aPos, double aRangeM, FGPositioned::Filter* aFilter, FGPositioned::List& aResults)
{
    aResults.clear();
    FindNearestPQueue pq;
    FindNearestResults results;
    pq.push(Ordered<Node*>(global_spatialOctree, 0));
    double rng = aRangeM * aRangeM;
    
    while (!pq.empty()) {
        Node* nd = pq.top().get();
        pq.pop();
        
        nd->visit(aPos, rng, aFilter, results, pq);
    } // of queue iteration
    
    unsigned int numResults = results.size();
  // copy results out
    aResults.resize(numResults);
    for (unsigned int r=0; r<numResults; ++r) {
      aResults[r] = results[r].get();
    }
}

} // of namespace Octree

//////////////////////////////////////////////////////////////////////////////

static void
addToIndices(FGPositioned* aPos)
{
  assert(aPos);
  if (!aPos->ident().empty()) {
    std::string u(boost::to_upper_copy(aPos->ident()));
    
    global_identIndex.insert(global_identIndex.begin(), 
      std::make_pair(u, aPos));
  }
  
  if (!aPos->name().empty()) {
    std::string u(boost::to_upper_copy(aPos->name()));
    
    global_nameIndex.insert(global_nameIndex.begin(), 
                             std::make_pair(u, aPos));
  }

  if (!Octree::global_spatialOctree) {
    double RADIUS_EARTH_M = 7000 * 1000.0; // 7000km is plenty
    SGVec3d earthExtent(RADIUS_EARTH_M, RADIUS_EARTH_M, RADIUS_EARTH_M);
    Octree::global_spatialOctree = new Octree::Branch(SGBox<double>(-earthExtent, earthExtent));
  }
  Octree::global_spatialOctree->insert(aPos);
}

static void
removeFromIndices(FGPositioned* aPos)
{
  assert(aPos);
  
  if (!aPos->ident().empty()) {
    std::string u(boost::to_upper_copy(aPos->ident()));
    NamedPositionedIndex::iterator it = global_identIndex.find(u);
    while (it != global_identIndex.end() && (it->first == u)) {
      if (it->second == aPos) {
        global_identIndex.erase(it);
        break;
      }
      
      ++it;
    } // of multimap walk
  }
  
  if (!aPos->name().empty()) {
    std::string u(boost::to_upper_copy(aPos->name()));
    NamedPositionedIndex::iterator it = global_nameIndex.find(u);
    while (it != global_nameIndex.end() && (it->first == u)) {
      if (it->second == aPos) {
        global_nameIndex.erase(it);
        break;
      }
      
      ++it;
    } // of multimap walk
  }
}

//////////////////////////////////////////////////////////////////////////////

class OrderByName
{
public:
  bool operator()(FGPositioned* a, FGPositioned* b) const
  {
    return a->name() < b->name();
  }
};

void findInIndex(NamedPositionedIndex& aIndex, const std::string& aFind, std::vector<FGPositioned*>& aResult)
{
  NamedPositionedIndex::const_iterator it = aIndex.begin();
  NamedPositionedIndex::const_iterator end = aIndex.end();

  bool haveFilter = !aFind.empty();

  for (; it != end; ++it) {
    FGPositioned::Type ty = it->second->type();
    if ((ty < FGPositioned::AIRPORT) || (ty > FGPositioned::SEAPORT)) {
      continue;
    }
    
    if (haveFilter && it->first.find(aFind) == std::string::npos) {
      continue;
    }
    
    aResult.push_back(it->second);
  } // of index iteration
}

/**
 * A special purpose helper (imported by FGAirport::searchNamesAndIdents) to
 * implement the AirportList dialog. It's unfortunate that it needs to reside
 * here, but for now it's least ugly solution.
 */
char** searchAirportNamesAndIdents(const std::string& aFilter)
{
// note this is a vector of raw pointers, not smart pointers, because it
// may get very large and smart-pointer-atomicity-locking then becomes a
// bottleneck for this case.
  std::vector<FGPositioned*> matches;
  if (!aFilter.empty()) {
    std::string filter = boost::to_upper_copy(aFilter);
    findInIndex(global_identIndex, filter, matches);
    findInIndex(global_nameIndex, filter, matches);
  } else {
    
    findInIndex(global_identIndex, std::string(), matches);
  }
  
// sort alphabetically on name
  std::sort(matches.begin(), matches.end(), OrderByName());
  
// convert results to format comptible with puaList
  unsigned int numMatches = matches.size();
  char** result = new char*[numMatches + 1];
  result[numMatches] = NULL; // end-of-list marker
  
  // nasty code to avoid excessive string copying and allocations.
  // We format results as follows (note whitespace!):
  //   ' name-of-airport-chars   (ident)'
  // so the total length is:
  //    1 + strlen(name) + 4 + strlen(icao) + 1 + 1 (for the null)
  // which gives a grand total of 7 + name-length + icao-length.
  // note the ident can be three letters (non-ICAO local strip), four
  // (default ICAO) or more (extended format ICAO)
  for (unsigned int i=0; i<numMatches; ++i) {
    int nameLength = matches[i]->name().size();
    int icaoLength = matches[i]->ident().size();
    char* entry = new char[7 + nameLength + icaoLength];
    char* dst = entry;
    *dst++ = ' ';
    memcpy(dst, matches[i]->name().c_str(), nameLength);
    dst += nameLength;
    *dst++ = ' ';
    *dst++ = ' ';
    *dst++ = ' ';
    *dst++ = '(';
    memcpy(dst, matches[i]->ident().c_str(), icaoLength);
    dst += icaoLength;
    *dst++ = ')';
    *dst++ = 0;
    result[i] = entry;
  }
  
  return result;
}

static void validateSGGeod(const SGGeod& geod)
{
  if (osg::isNaN(geod.getLatitudeDeg()) ||
      osg::isNaN(geod.getLongitudeDeg()))
  {
    throw sg_range_exception("position is invalid, NaNs");
  }
}

///////////////////////////////////////////////////////////////////////////////

bool
FGPositioned::Filter::hasTypeRange() const
{
  assert(minType() <= maxType());
  return (minType() != INVALID) && (maxType() != INVALID);
}

bool
FGPositioned::Filter::passType(Type aTy) const
{
  assert(hasTypeRange());
  return (minType() <= aTy) && (maxType() >= aTy);
}

static FGPositioned::List 
findAll(const NamedPositionedIndex& aIndex, 
                             const std::string& aName,
                             FGPositioned::Filter* aFilter,
                             bool aExact)
{
  FGPositioned::List result;
  if (aName.empty()) {
    return result;
  }
  
  std::string name = boost::to_upper_copy(aName);
  NamedPositionedIndex::const_iterator upperBound;
  
  if (aExact) {
    upperBound = aIndex.upper_bound(name);
  } else {
    std::string upperBoundId = name;
    upperBoundId[upperBoundId.size()-1]++;
    upperBound = aIndex.lower_bound(upperBoundId);
  }
  
  NamedPositionedIndex::const_iterator it = aIndex.lower_bound(name);
  
  for (; it != upperBound; ++it) {
    FGPositionedRef candidate = it->second;
    if (aFilter) {
      if (aFilter->hasTypeRange() && !aFilter->passType(candidate->type())) {
        continue;
      }
      
      if (!aFilter->pass(candidate)) {
        continue;
      }
    }
    
    result.push_back(candidate);
  }
  
  return result;
}

///////////////////////////////////////////////////////////////////////////////

FGPositioned::FGPositioned(Type ty, const std::string& aIdent, const SGGeod& aPos) :
  mPosition(aPos),
  mType(ty),
  mIdent(aIdent)
{  
}

void FGPositioned::init(bool aIndexed)
{
  SGReferenced::get(this); // hold an owning ref, for the moment
  mCart = SGVec3d::fromGeod(mPosition);
  
  if (aIndexed) {
    assert(mType != TAXIWAY && mType != PAVEMENT);
    addToIndices(this);
  }
}

FGPositioned::~FGPositioned()
{
  //std::cout << "destroying:" << mIdent << "/" << nameForType(mType) << std::endl;
  removeFromIndices(this);
}

FGPositioned*
FGPositioned::createUserWaypoint(const std::string& aIdent, const SGGeod& aPos)
{
  FGPositioned* wpt = new FGPositioned(WAYPOINT, aIdent, aPos);
  wpt->init(true);
  return wpt;
}

const SGVec3d&
FGPositioned::cart() const
{
  return mCart;
}

FGPositioned::Type FGPositioned::typeFromName(const std::string& aName)
{
  if (aName.empty() || (aName == "")) {
    return INVALID;
  }

  typedef struct {
    const char* _name;
    Type _ty;
  } NameTypeEntry;
  
  const NameTypeEntry names[] = {
    {"airport", AIRPORT},
    {"vor", VOR},
    {"loc", LOC},
    {"ils", ILS},
    {"gs", GS},
    {"ndb", NDB},
    {"wpt", WAYPOINT},
    {"fix", FIX},
    {"tacan", TACAN},
    {"dme", DME},
    {"atis", FREQ_ATIS},
    {"awos", FREQ_AWOS},
    {"tower", FREQ_TOWER},
    {"ground", FREQ_GROUND},
    {"approach", FREQ_APP_DEP},
    {"departure", FREQ_APP_DEP},
  // aliases
    {"gnd", FREQ_GROUND},
    {"twr", FREQ_TOWER},
    {"waypoint", WAYPOINT},
    {"apt", AIRPORT},
    {"arpt", AIRPORT},
    {"any", INVALID},
    {"all", INVALID},
    
    {NULL, INVALID}
  };
  
  std::string lowerName(boost::to_lower_copy(aName));
  
  for (const NameTypeEntry* n = names; (n->_name != NULL); ++n) {
    if (::strcmp(n->_name, lowerName.c_str()) == 0) {
      return n->_ty;
    }
  }
  
  SG_LOG(SG_GENERAL, SG_WARN, "FGPositioned::typeFromName: couldn't match:" << aName);
  return INVALID;
}

const char* FGPositioned::nameForType(Type aTy)
{
 switch (aTy) {
 case RUNWAY: return "runway";
 case TAXIWAY: return "taxiway";
 case PAVEMENT: return "pavement";
 case PARK_STAND: return "parking stand";
 case FIX: return "fix";
 case VOR: return "VOR";
 case NDB: return "NDB";
 case ILS: return "ILS";
 case LOC: return "localiser";
 case GS: return "glideslope";
 case OM: return "outer-marker";
 case MM: return "middle-marker";
 case IM: return "inner-marker";
 case AIRPORT: return "airport";
 case HELIPORT: return "heliport";
 case SEAPORT: return "seaport";
 case WAYPOINT: return "waypoint";
 case DME: return "dme";
 case TACAN: return "tacan";
 case FREQ_TOWER: return "tower";
 case FREQ_ATIS: return "atis";
 case FREQ_AWOS: return "awos";
 case FREQ_GROUND: return "ground";
 case FREQ_CLEARANCE: return "clearance";
 case FREQ_UNICOM: return "unicom";
 case FREQ_APP_DEP: return "approach-departure";
 default:
  return "unknown";
 }
}

///////////////////////////////////////////////////////////////////////////////
// search / query functions

FGPositionedRef
FGPositioned::findClosestWithIdent(const std::string& aIdent, const SGGeod& aPos, Filter* aFilter)
{
  validateSGGeod(aPos);

  FGPositioned::List r(findAll(global_identIndex, aIdent, aFilter, true));
  if (r.empty()) {
    return FGPositionedRef();
  }
  
  sortByRange(r, aPos);
  return r.front();
}

FGPositioned::List
FGPositioned::findWithinRange(const SGGeod& aPos, double aRangeNm, Filter* aFilter)
{
  validateSGGeod(aPos);

  List result;
  Octree::findAllWithinRange(SGVec3d::fromGeod(aPos), 
    aRangeNm * SG_NM_TO_METER, aFilter, result);
  return result;
}

FGPositioned::List
FGPositioned::findAllWithIdent(const std::string& aIdent, Filter* aFilter, bool aExact)
{
  return findAll(global_identIndex, aIdent, aFilter, aExact);
}

FGPositioned::List
FGPositioned::findAllWithName(const std::string& aName, Filter* aFilter, bool aExact)
{
  return findAll(global_nameIndex, aName, aFilter, aExact);
}

FGPositionedRef
FGPositioned::findClosest(const SGGeod& aPos, double aCutoffNm, Filter* aFilter)
{
  validateSGGeod(aPos);
  
  List l(findClosestN(aPos, 1, aCutoffNm, aFilter));
  if (l.empty()) {
    return NULL;
  }

  assert(l.size() == 1);
  return l.front();
}

FGPositioned::List
FGPositioned::findClosestN(const SGGeod& aPos, unsigned int aN, double aCutoffNm, Filter* aFilter)
{
  validateSGGeod(aPos);
  
  List result;
  Octree::findNearestN(SGVec3d::fromGeod(aPos), aN, aCutoffNm * SG_NM_TO_METER, aFilter, result);
  return result;
}

FGPositionedRef
FGPositioned::findNextWithPartialId(FGPositionedRef aCur, const std::string& aId, Filter* aFilter)
{
  if (aId.empty()) {
    return NULL;
  }
  
  std::string id(boost::to_upper_copy(aId));

  // It is essential to bound our search, to avoid iterating all the way to the end of the database.
  // Do this by generating a second ID with the final character incremented by 1.
  // e.g., if the partial ID is "KI", we wish to search "KIxxx" but not "KJ".
  std::string upperBoundId = id;
  upperBoundId[upperBoundId.size()-1]++;
  NamedPositionedIndex::const_iterator upperBound = global_identIndex.lower_bound(upperBoundId);

  NamedIndexRange range = global_identIndex.equal_range(id);
  while (range.first != upperBound) {
    for (; range.first != range.second; ++range.first) {
      FGPositionedRef candidate = range.first->second;
      if (aCur == candidate) {
        aCur = NULL; // found our start point, next match will pass
        continue;
      }

      if (aFilter) {
        if (aFilter->hasTypeRange() && !aFilter->passType(candidate->type())) {
          continue;
        }

        if (!aFilter->pass(candidate)) {
          continue;
        }
      }

      if (!aCur) {
        return candidate;
      }
    }

    // Unable to match the filter with this range - try the next range.
    range = global_identIndex.equal_range(range.second->first);
  }

  return NULL; // Reached the end of the valid sequence with no match.  
}
  
void
FGPositioned::sortByRange(List& aResult, const SGGeod& aPos)
{
  validateSGGeod(aPos);
  
  SGVec3d cartPos(SGVec3d::fromGeod(aPos));
// computer ordering values
  Octree::FindNearestResults r;
  List::iterator it = aResult.begin(), lend = aResult.end();
  for (; it != lend; ++it) {
    double d2 = distSqr((*it)->cart(), cartPos);
    r.push_back(Octree::OrderedPositioned(*it, d2));
  }
  
// sort
  std::sort(r.begin(), r.end());
  
// convert to a plain list
  unsigned int count = aResult.size();
  for (unsigned int i=0; i<count; ++i) {
    aResult[i] = r[i].get();
  }
}

FGPositioned::TypeFilter::TypeFilter(Type aTy)
{
  addType(aTy);
}

void FGPositioned::TypeFilter::addType(Type aTy)
{
  if (aTy == INVALID) {
    return;
    
  }
  
  types.push_back(aTy);
}

bool
FGPositioned::TypeFilter::pass(FGPositioned* aPos) const
{
  if (types.empty()) {
    return true;
  }
  
    std::vector<Type>::const_iterator it = types.begin(),
        end = types.end();
    for (; it != end; ++it) {
        return aPos->type() == *it;
    }
    
    return false;
}

