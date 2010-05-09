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

#include <map>
#include <set>
#include <algorithm> // for sort
#include <queue>

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <simgear/timing/timestamp.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/math/SGGeometry.hxx>

#include "positioned.hxx"

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
    global_identIndex.insert(global_identIndex.begin(), 
      std::make_pair(aPos->ident(), aPos));
  }
  
  if (!aPos->name().empty()) {
    global_nameIndex.insert(global_nameIndex.begin(), 
                             std::make_pair(aPos->name(), aPos));
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
    NamedPositionedIndex::iterator it = global_identIndex.find(aPos->ident());
    while (it != global_identIndex.end() && (it->first == aPos->ident())) {
      if (it->second == aPos) {
        global_identIndex.erase(it);
        break;
      }
      
      ++it;
    } // of multimap walk
  }
  
  if (!aPos->name().empty()) {
    NamedPositionedIndex::iterator it = global_nameIndex.find(aPos->name());
    while (it != global_nameIndex.end() && (it->first == aPos->name())) {
      if (it->second == aPos) {
        global_nameIndex.erase(it);
        break;
      }
      
      ++it;
    } // of multimap walk
  }
}

class DistanceOrdering
{
public:
  DistanceOrdering(const SGGeod& aPos) :
    mPos(SGVec3d::fromGeod(aPos))
  { }
  
  bool operator()(const FGPositionedRef& a, const FGPositionedRef& b) const
  {
    if (!a || !b) {
      throw sg_exception("empty reference passed to DistanceOrdering");
    }
  
    double dA = distSqr(a->cart(), mPos),
      dB = distSqr(b->cart(), mPos);
    return dA < dB;
  }

private:
  SGVec3d mPos;
};

static void
sortByDistance(const SGGeod& aPos, FGPositioned::List& aResult)
{
  std::sort(aResult.begin(), aResult.end(), DistanceOrdering(aPos));
}

static FGPositionedRef
namedFindClosest(const NamedPositionedIndex& aIndex, const std::string& aName, 
                 const SGGeod& aOrigin, FGPositioned::Filter* aFilter)
{
  NamedIndexRange range = aIndex.equal_range(aName);
  if (range.first == range.second) {
    return NULL;
  }
  
// common case, only one result. looks a bit ugly because these are
// sequential iterators, not random-access ones
  NamedPositionedIndex::const_iterator check = range.first;
  if (++check == range.second) {
    // excellent, only one match in the range
    FGPositioned* r = range.first->second;
    if (aFilter) {
      if (aFilter->hasTypeRange() && !aFilter->passType(r->type())) {
        return NULL;
      }
      
      if (!aFilter->pass(r)) {
        return NULL;
      }
    } // of have a filter
  
    return r;
  } // of short-circuit logic for single-element range
  
// multiple matches, we need to actually check the distance to each one
  double minDist = HUGE_VAL;
  FGPositionedRef result;
  NamedPositionedIndex::const_iterator it = range.first;
  SGVec3d cartOrigin(SGVec3d::fromGeod(aOrigin));
  
  for (; it != range.second; ++it) {
    FGPositioned* r = it->second;
    if (aFilter) {
      if (aFilter->hasTypeRange() && !aFilter->passType(r->type())) {
        continue;
      }
      
      if (!aFilter->pass(r)) {
        continue;
      }
    }
    
  // find distance
    double d2 = distSqr(cartOrigin, r->cart());
    if (d2 < minDist) {
      minDist = d2;
      result = r;
    }
  }
  
  return result;
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

/**
 * A special purpose helper (imported by FGAirport::searchNamesAndIdents) to
 * implement the AirportList dialog. It's unfortunate that it needs to reside
 * here, but for now it's least ugly solution.
 */
char** searchAirportNamesAndIdents(const std::string& aFilter)
{
  const std::ctype<char> &ct = std::use_facet<std::ctype<char> >(std::locale());
  std::string filter(aFilter);
  bool hasFilter = !filter.empty();
  if (hasFilter) {
    ct.toupper((char *)filter.data(), (char *)filter.data() + filter.size());
  }
  
  NamedPositionedIndex::const_iterator it = global_identIndex.begin();
  NamedPositionedIndex::const_iterator end = global_identIndex.end();
  
  // note this is a vector of raw pointers, not smart pointers, because it
  // may get very large and smart-pointer-atomicity-locking then becomes a
  // bottleneck for this case.
  std::vector<FGPositioned*> matches;
  std::string upper;
  
  for (; it != end; ++it) {
    FGPositioned::Type ty = it->second->type();
    if ((ty < FGPositioned::AIRPORT) || (ty > FGPositioned::SEAPORT)) {
      continue;
    }
    
    if (hasFilter && (it->second->ident().find(aFilter) == std::string::npos)) {
      upper = it->second->name(); // string copy, sadly
      ct.toupper((char *)upper.data(), (char *)upper.data() + upper.size());
      if (upper.find(aFilter) == std::string::npos) {
        continue;
      }
    }
    
    matches.push_back(it->second);
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
findAllSortedByRange(const NamedPositionedIndex& aIndex, 
                             const std::string& aName, const SGGeod& aPos, FGPositioned::Filter* aFilter)
{
  FGPositioned::List result;
  NamedIndexRange range = aIndex.equal_range(aName);
  for (; range.first != range.second; ++range.first) {
    FGPositioned* candidate = range.first->second;
    if (aFilter) {
      if (aFilter->hasTypeRange() && !aFilter->passType(candidate->type())) {
        continue;
      }
      
      if (!aFilter->pass(candidate)) {
        continue;
      }
    }
    
    result.push_back(range.first->second);
  }
  
  sortByDistance(aPos, result);
  return result;
}

static FGPositionedRef
findWithPartial(const NamedPositionedIndex& aIndex, const std::string& aName, 
                    FGPositioned::Filter* aFilter, int aOffset, bool& aNext)
{
  // see comment in findNextWithPartialId concerning upperBoundId
  std::string upperBoundId = aName;
  upperBoundId[upperBoundId.size()-1]++;
  NamedPositionedIndex::const_iterator upperBound = aIndex.lower_bound(upperBoundId);
  
  NamedIndexRange range = aIndex.equal_range(aName);
  FGPositionedRef result;
  
  while (range.first != upperBound) {
    for (; range.first != range.second; ++range.first) {
      FGPositionedRef candidate = range.first->second;
      if (aFilter) {
        if (aFilter->hasTypeRange() && !aFilter->passType(candidate->type())) {
          continue;
        }
        
        if (!aFilter->pass(candidate)) {
          continue;
        }
      }
      
      if (result) {
        aNext = true;
        return result;
      } else if (aOffset == 0) {
        // okay, found our result. we need to go around once more to set aNext
        result = candidate;
      } else {
        --aOffset; // seen one more valid result, decrement the count
      }
    }
    
    // Unable to match the filter with this range - try the next range.
    range = aIndex.equal_range(range.second->first);
  }
  
  // if we fell out, we reached the end of the valid range. We might have a
  // valid result, but we definitiely don't have a next result.
  aNext = false;
  return result;  
}

///////////////////////////////////////////////////////////////////////////////

FGPositioned::FGPositioned(Type ty, const std::string& aIdent, const SGGeod& aPos, bool aIndexed) :
  mType(ty),
  mPosition(aPos),
  mIdent(aIdent)
{  
  SGReferenced::get(this); // hold an owning ref, for the moment
  
  if (aIndexed) {
    assert(ty != TAXIWAY && ty != PAVEMENT);
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
  return new FGPositioned(WAYPOINT, aIdent, aPos, true);
}

SGVec3d
FGPositioned::cart() const
{
  return SGVec3d::fromGeod(mPosition);
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
    {"ndb", NDB},
    {"wpt", WAYPOINT},
    {"fix", FIX},
    {"tacan", TACAN},
    {"dme", DME},
  // aliases
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
 default:
  return "unknown";
 }
}

///////////////////////////////////////////////////////////////////////////////
// search / query functions

FGPositionedRef
FGPositioned::findClosestWithIdent(const std::string& aIdent, const SGGeod& aPos, Filter* aFilter)
{
  return namedFindClosest(global_identIndex, aIdent, aPos, aFilter);
}

FGPositioned::List
FGPositioned::findWithinRange(const SGGeod& aPos, double aRangeNm, Filter* aFilter)
{
  List result;
  Octree::findAllWithinRange(SGVec3d::fromGeod(aPos), 
    aRangeNm * SG_NM_TO_METER, aFilter, result);
  return result;
}

FGPositioned::List
FGPositioned::findAllWithIdentSortedByRange(const std::string& aIdent, const SGGeod& aPos, Filter* aFilter)
{
  return findAllSortedByRange(global_identIndex, aIdent, aPos, aFilter);
}

FGPositioned::List
FGPositioned::findAllWithNameSortedByRange(const std::string& aName, const SGGeod& aPos, Filter* aFilter)
{
  return findAllSortedByRange(global_nameIndex, aName, aPos, aFilter);
}

FGPositionedRef
FGPositioned::findClosest(const SGGeod& aPos, double aCutoffNm, Filter* aFilter)
{
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
  
FGPositionedRef
FGPositioned::findWithPartialId(const std::string& aId, Filter* aFilter, int aOffset, bool& aNext)
{
  return findWithPartial(global_identIndex, aId, aFilter, aOffset, aNext);
}


FGPositionedRef
FGPositioned::findWithPartialName(const std::string& aName, Filter* aFilter, int aOffset, bool& aNext)
{
  return findWithPartial(global_nameIndex, aName, aFilter, aOffset, aNext);
}

/**
 * Wrapper filter which proxies to an inner filter, but also ensures the
 * ident starts with supplied partial ident.
 */
class PartialIdentFilter : public FGPositioned::Filter
{
public:
  PartialIdentFilter(const std::string& ident, FGPositioned::Filter* filter) :
    _inner(filter)
  {
    _ident = boost::to_upper_copy(ident);
  }
  
  virtual bool pass(FGPositioned* aPos) const
  {
    if (!_inner->pass(aPos)) {
      return false;
    }
    
    return (boost::algorithm::starts_with(aPos->ident(), _ident));
  }
    
  virtual FGPositioned::Type minType() const
  { return _inner->minType(); }
    
  virtual FGPositioned::Type maxType() const
  { return _inner->maxType(); }
    
private:
  std::string _ident;
  FGPositioned::Filter* _inner;
};

static FGPositionedRef
findClosestWithPartial(const SGGeod& aPos, FGPositioned::Filter* aFilter, int aOffset, bool& aNext)
{
  // why aOffset +2 ? at offset=3, we want the fourth search result, but also
  // to know if the fifth result exists (to set aNext flag for iterative APIs)
  FGPositioned::List matches;
  Octree::findNearestN(SGVec3d::fromGeod(aPos), aOffset + 2, 1000 * SG_NM_TO_METER, aFilter, matches);
  
  if ((int) matches.size() <= aOffset) {
    SG_LOG(SG_GENERAL, SG_INFO, "findClosestWithPartial, couldn't match enough with prefix");
    aNext = false;
    return NULL; // couldn't find a match within the cutoff distance
  }
  
  aNext = ((int) matches.size() >= (aOffset + 2));
  return matches[aOffset];

}

FGPositionedRef
FGPositioned::findClosestWithPartialId(const SGGeod& aPos, const std::string& aId, Filter* aFilter, int aOffset, bool& aNext)
{
  PartialIdentFilter pf(aId, aFilter);
  return findClosestWithPartial(aPos, &pf, aOffset, aNext);
}

/**
 * Wrapper filter which proxies to an inner filter, but also ensures the
 * name starts with supplied partial name.
 */
class PartialNameFilter : public FGPositioned::Filter
{
public:
  PartialNameFilter(const std::string& nm, FGPositioned::Filter* filter) :
  _inner(filter)
  {
    _name = nm;
  }
  
  virtual bool pass(FGPositioned* aPos) const
  {
    if (!_inner->pass(aPos)) {
      return false;
    }
    
    return (boost::algorithm::istarts_with(aPos->name(), _name));
  }
  
  virtual FGPositioned::Type minType() const
  { return _inner->minType(); }
  
  virtual FGPositioned::Type maxType() const
  { return _inner->maxType(); }
  
private:
  std::string _name;
  FGPositioned::Filter* _inner;
};

FGPositionedRef
FGPositioned::findClosestWithPartialName(const SGGeod& aPos, const std::string& aName, Filter* aFilter, int aOffset, bool& aNext)
{
  PartialNameFilter pf(aName, aFilter);
  return findClosestWithPartial(aPos, &pf, aOffset, aNext);
}

