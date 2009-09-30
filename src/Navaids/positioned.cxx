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
#include <locale> // for char-traits toupper

#include <iostream>

#include <boost/algorithm/string/case_conv.hpp>

#include <simgear/math/sg_geodesy.hxx>
#include <simgear/timing/timestamp.hxx>
#include <simgear/debug/logstream.hxx>

#include "positioned.hxx"

typedef std::multimap<std::string, FGPositioned*> NamedPositionedIndex;
typedef std::pair<NamedPositionedIndex::const_iterator, NamedPositionedIndex::const_iterator> NamedIndexRange;

using std::lower_bound;
using std::upper_bound;

/**
 * Order positioned elements by type, then pointer address. This allows us to
 * use range searches (lower_ and upper_bound) to grab items of a particular
 * type out of bucket efficently.
 */
class OrderByType
{
public:
  bool operator()(const FGPositioned* a, const FGPositioned* b) const
  {
    if (a->type() == b->type()) return a < b;
    return a->type() < b->type();
  }
};

class LowerLimitOfType
{
public:
  bool operator()(const FGPositioned* a, const FGPositioned::Type b) const
  {
    return a->type() < b;
  }

  bool operator()(const FGPositioned::Type a, const FGPositioned* b) const
  {
    return a < b->type();
  }

  // The operator below is required by VS2005 in debug mode
  bool operator()(const FGPositioned* a, const FGPositioned* b) const
  {
    return a->type() < b->type();
  }
};


typedef std::set<FGPositioned*, OrderByType> BucketEntry;
typedef std::map<long int, BucketEntry> SpatialPositionedIndex;

static NamedPositionedIndex global_namedIndex;
static SpatialPositionedIndex global_spatialIndex;

SpatialPositionedIndex::iterator
bucketEntryForPositioned(FGPositioned* aPos)
{
  int bucketIndex = aPos->bucket().gen_index();
  SpatialPositionedIndex::iterator it = global_spatialIndex.find(bucketIndex);
  if (it != global_spatialIndex.end()) {
    return it;
  }
  
  // create a new BucketEntry
  return global_spatialIndex.insert(it, std::make_pair(bucketIndex, BucketEntry()));
}

static void
addToIndices(FGPositioned* aPos)
{
  assert(aPos);
  if (!aPos->ident().empty()) {
    global_namedIndex.insert(global_namedIndex.begin(), 
      std::make_pair(aPos->ident(), aPos));
  }
    
  SpatialPositionedIndex::iterator it = bucketEntryForPositioned(aPos);
  it->second.insert(aPos);
}

static void
removeFromIndices(FGPositioned* aPos)
{
  assert(aPos);
  
  if (!aPos->ident().empty()) {
    NamedPositionedIndex::iterator it = global_namedIndex.find(aPos->ident());
    while (it != global_namedIndex.end() && (it->first == aPos->ident())) {
      if (it->second == aPos) {
        global_namedIndex.erase(it);
        break;
      }
      
      ++it;
    } // of multimap walk
  }
  
  SpatialPositionedIndex::iterator sit = bucketEntryForPositioned(aPos);
  sit->second.erase(aPos);
}

static void
spatialFilterInBucket(const SGBucket& aBucket, FGPositioned::Filter* aFilter, FGPositioned::List& aResult)
{
  SpatialPositionedIndex::const_iterator it;
  it = global_spatialIndex.find(aBucket.gen_index());
  if (it == global_spatialIndex.end()) {
    return;
  }
  
  BucketEntry::const_iterator l = it->second.begin();
  BucketEntry::const_iterator u = it->second.end();

  if (!aFilter) { // pass everything
    aResult.insert(aResult.end(), l, u);
    return;
  }

  if (aFilter->hasTypeRange()) {
    // avoid many calls to the filter hook
    l = lower_bound(it->second.begin(), it->second.end(), aFilter->minType(), LowerLimitOfType());
    u = upper_bound(l, it->second.end(), aFilter->maxType(), LowerLimitOfType());
  }

  for ( ; l != u; ++l) {
    if ((*aFilter)(*l)) {
      aResult.push_back(*l);
    }
  }
}

static void
spatialFind(const SGGeod& aPos, double aRange, 
  FGPositioned::Filter* aFilter, FGPositioned::List& aResult)
{
  SGBucket buck(aPos);
  double lat = aPos.getLatitudeDeg(),
    lon = aPos.getLongitudeDeg();
  
  int bx = (int)( aRange*SG_NM_TO_METER / buck.get_width_m() / 2);
  int by = (int)( aRange*SG_NM_TO_METER / buck.get_height_m() / 2 );
    
  // loop over bucket range 
  for ( int i=-bx; i<=bx; i++) {
    for ( int j=-by; j<=by; j++) {
      spatialFilterInBucket(sgBucketOffset(lon, lat, i, j), aFilter, aResult);
    } // of j-iteration
  } // of i-iteration  
}

/**
 */
class RangePredictate
{
public:
  RangePredictate(const SGGeod& aOrigin, double aRange) :
    mOrigin(SGVec3d::fromGeod(aOrigin)),
    mRangeSqr(aRange * aRange)
  { ; }
  
  bool operator()(const FGPositionedRef& aPos)
  {
    double dSqr = distSqr(aPos->cart(), mOrigin);
    return (dSqr > mRangeSqr);
  }
  
private:
  SGVec3d mOrigin;
  double mRangeSqr;
};

static void
filterListByRange(const SGGeod& aPos, double aRange, FGPositioned::List& aResult)
{
  RangePredictate pred(aPos, aRange * SG_NM_TO_METER);
  FGPositioned::List::iterator newEnd; 
  newEnd = std::remove_if(aResult.begin(), aResult.end(), pred);
  aResult.erase(newEnd, aResult.end());
}

class DistanceOrdering
{
public:
  DistanceOrdering(const SGGeod& aPos) :
    mPos(SGVec3d::fromGeod(aPos))
  { }
  
  bool operator()(const FGPositionedRef& a, const FGPositionedRef& b) const
  {
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
namedFindClosest(const std::string& aIdent, const SGGeod& aOrigin, FGPositioned::Filter* aFilter)
{
  NamedIndexRange range = global_namedIndex.equal_range(aIdent);
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

static FGPositioned::List
spatialGetClosest(const SGGeod& aPos, unsigned int aN, double aCutoffNm, FGPositioned::Filter* aFilter)
{
  FGPositioned::List result;
  int radius = 1; // start at 1, radius 0 is handled explicitly
  SGBucket buck;
  double lat = aPos.getLatitudeDeg(),
    lon = aPos.getLongitudeDeg();
  // final cutoff is in metres, and scaled to account for testing the corners
  // of the 'box' instead of the centre of each edge
  double cutoffM = aCutoffNm * SG_NM_TO_METER * 1.5;
  
  // base case, simplifes loop to do it seperately here
  spatialFilterInBucket(sgBucketOffset(lon, lat, 0, 0), aFilter, result);

  for (;result.size() < aN; ++radius) {
    // cutoff check
    double az1, az2, d1, d2;
    SGGeodesy::inverse(aPos, sgBucketOffset(lon, lat, -radius, -radius).get_center(), az1, az2, d1);
    SGGeodesy::inverse(aPos, sgBucketOffset(lon, lat, radius, radius).get_center(), az1, az2, d2);  
      
    if ((d1 > cutoffM) && (d2 > cutoffM)) {
      //std::cerr << "spatialGetClosest terminating due to range cutoff" << std::endl;
      break;
    }
    
    FGPositioned::List hits;
    for ( int i=-radius; i<=radius; i++) {
      spatialFilterInBucket(sgBucketOffset(lon, lat, i, -radius), aFilter, hits);
      spatialFilterInBucket(sgBucketOffset(lon, lat, -radius, i), aFilter, hits);
      spatialFilterInBucket(sgBucketOffset(lon, lat, i, radius), aFilter, hits);
      spatialFilterInBucket(sgBucketOffset(lon, lat, radius, i), aFilter, hits);
    }

    result.insert(result.end(), hits.begin(), hits.end()); // append
  } // of outer loop
  
  sortByDistance(aPos, result);
  if (result.size() > aN) {
    result.resize(aN); // truncate at requested number of matches
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
  
  NamedPositionedIndex::const_iterator it = global_namedIndex.begin();
  NamedPositionedIndex::const_iterator end = global_namedIndex.end();
  
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
  //    1 + strlen(name) + 4 + 4 (for the ident) + 1 + 1 (for the null)
  // which gives a grand total of 11 + the length of the name.
  // note the ident is sometimes only three letters for non-ICAO small strips
  for (unsigned int i=0; i<numMatches; ++i) {
    int nameLength = matches[i]->name().size();
    int icaoLength = matches[i]->ident().size();
    char* entry = new char[nameLength + 11];
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

SGBucket
FGPositioned::bucket() const
{
  return SGBucket(mPosition);
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
  return namedFindClosest(aIdent, aPos, aFilter);
}

FGPositioned::List
FGPositioned::findWithinRange(const SGGeod& aPos, double aRangeNm, Filter* aFilter)
{
  List result;
  spatialFind(aPos, aRangeNm, aFilter, result);
  filterListByRange(aPos, aRangeNm, result);
  return result;
}

FGPositioned::List
FGPositioned::findAllWithIdentSortedByRange(const std::string& aIdent, const SGGeod& aPos, Filter* aFilter)
{
  List result;
  NamedIndexRange range = global_namedIndex.equal_range(aIdent);
  for (; range.first != range.second; ++range.first) {
    if (aFilter && !aFilter->pass(range.first->second)) {
      continue;
    }
    
    result.push_back(range.first->second);
  }
  
  sortByDistance(aPos, result);
  return result;
}

FGPositionedRef
FGPositioned::findClosest(const SGGeod& aPos, double aCutoffNm, Filter* aFilter)
{
   FGPositioned::List l(spatialGetClosest(aPos, 1, aCutoffNm, aFilter));
   if (l.empty()) {
      return NULL;
   }
   
   assert(l.size() == 1);
   return l.front();
}

FGPositioned::List
FGPositioned::findClosestN(const SGGeod& aPos, unsigned int aN, double aCutoffNm, Filter* aFilter)
{
  return spatialGetClosest(aPos, aN, aCutoffNm, aFilter);
}

FGPositionedRef
FGPositioned::findNextWithPartialId(FGPositionedRef aCur, const std::string& aId, Filter* aFilter)
{
  // It is essential to bound our search, to avoid iterating all the way to the end of the database.
  // Do this by generating a second ID with the final character incremented by 1.
  // e.g., if the partial ID is "KI", we wish to search "KIxxx" but not "KJ".
  std::string upperBoundId = aId;
  upperBoundId[upperBoundId.size()-1]++;
  NamedPositionedIndex::const_iterator upperBound = global_namedIndex.lower_bound(upperBoundId);

  NamedIndexRange range = global_namedIndex.equal_range(aId);
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
    range = global_namedIndex.equal_range(range.second->first);
  }

  return NULL; // Reached the end of the valid sequence with no match.  
}
  
FGPositionedRef
FGPositioned::findWithPartialId(const std::string& aId, Filter* aFilter, int aOffset)
{
  // see comment in findNextWithPartialId concerning upperBoundId
  std::string upperBoundId = aId;
  upperBoundId[upperBoundId.size()-1]++;
  NamedPositionedIndex::const_iterator upperBound = global_namedIndex.lower_bound(upperBoundId);

  NamedIndexRange range = global_namedIndex.equal_range(aId);
  
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

      if (aOffset == 0) {
        return candidate;
      } else {
        --aOffset; // seen one more valid result, decrement the count
      }
    }

    // Unable to match the filter with this range - try the next range.
    range = global_namedIndex.equal_range(range.second->first);
  }

  return NULL; // Reached the end of the valid sequence with no match.  
}

/**
 * Wrapper filter which proxies to an inner filter, but also ensures the
 * ident starts with supplied partial ident.
 */
class PartialIdentFilter : public FGPositioned::Filter
{
public:
  PartialIdentFilter(const std::string& ident, FGPositioned::Filter* filter) :
    _ident(ident),
    _inner(filter)
  { ; }
  
  virtual bool pass(FGPositioned* aPos) const
  {
    if (!_inner->pass(aPos)) {
      return false;
    }
    
    return (::strncmp(aPos->ident().c_str(), _ident.c_str(), _ident.size()) == 0);
  }
    
  virtual FGPositioned::Type minType() const
  { return _inner->minType(); }
    
  virtual FGPositioned::Type maxType() const
  { return _inner->maxType(); }
    
private:
  std::string _ident;
  FGPositioned::Filter* _inner;
};

FGPositionedRef
FGPositioned::findClosestWithPartialId(const SGGeod& aPos, const std::string& aId, Filter* aFilter, int aOffset)
{
  PartialIdentFilter pf(aId, aFilter);
  List matches = spatialGetClosest(aPos, aOffset + 1, 1000.0, &pf);
  
  if ((int) matches.size() <= aOffset) {
    SG_LOG(SG_GENERAL, SG_INFO, "FGPositioned::findClosestWithPartialId, couldn't match enough with prefix:" << aId);
    return NULL; // couldn't find a match within the cutoff distance
  }
  
  return matches[aOffset];
}

