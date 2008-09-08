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

#include <simgear/math/sg_geodesy.hxx>

#include "positioned.hxx"

typedef std::multimap<std::string, FGPositioned*> NamedPositionedIndex;
typedef std::pair<NamedPositionedIndex::const_iterator, NamedPositionedIndex::const_iterator> NamedIndexRange;

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
  global_namedIndex.insert(global_namedIndex.begin(), 
    std::make_pair(aPos->ident(), aPos));
    
  SpatialPositionedIndex::iterator it = bucketEntryForPositioned(aPos);
  it->second.insert(aPos);
}

static void
removeFromIndices(FGPositioned* aPos)
{
  assert(aPos);
  
  NamedPositionedIndex::iterator it = global_namedIndex.find(aPos->ident());
  while (it != global_namedIndex.end() && (it->first == aPos->ident())) {
    if (it->second == aPos) {
      global_namedIndex.erase(it);
      break;
    }
    
    ++it;
  }
  
  SpatialPositionedIndex::iterator sit = bucketEntryForPositioned(aPos);
  sit->second.erase(aPos);
}

static void
spatialFilterInBucket(const SGBucket& aBucket, const FGPositioned::Filter& aFilter, FGPositioned::List& aResult)
{
  SpatialPositionedIndex::const_iterator it;
  it = global_spatialIndex.find(aBucket.gen_index());
  if (it == global_spatialIndex.end()) {
    return;
  }
  
  BucketEntry::const_iterator l = it->second.begin();
  BucketEntry::const_iterator u = it->second.end();

  for ( ; l != u; ++l) {
    if (aFilter(*l)) {
      aResult.push_back(*l);
    }
  }
}

static void
spatialFind(const SGGeod& aPos, double aRange, 
  const FGPositioned::Filter& aFilter, FGPositioned::List& aResult)
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
};

static void
spatialFindTyped(const SGGeod& aPos, double aRange, FGPositioned::Type aLower, FGPositioned::Type aUpper, FGPositioned::List& aResult)
{
  SGBucket buck(aPos);
  double lat = aPos.getLatitudeDeg(),
    lon = aPos.getLongitudeDeg();
  
  int bx = (int)( aRange*SG_NM_TO_METER / buck.get_width_m() / 2);
  int by = (int)( aRange*SG_NM_TO_METER / buck.get_height_m() / 2 );
    
  // loop over bucket range 
  for ( int i=-bx; i<=bx; i++) {
    for ( int j=-by; j<=by; j++) {
      buck = sgBucketOffset(lon, lat, i, j);
      
      SpatialPositionedIndex::const_iterator it;
      it = global_spatialIndex.find(buck.gen_index());
      if (it == global_spatialIndex.end()) {
        continue;
      }
      
      BucketEntry::const_iterator l = std::lower_bound(it->second.begin(), it->second.end(), aLower, LowerLimitOfType());
      BucketEntry::const_iterator u = std::upper_bound(l, it->second.end(), aUpper, LowerLimitOfType());
      
      for ( ; l != u; ++l) {
        aResult.push_back(*l);
      }
      
    } // of j-iteration
  } // of i-iteration  
}

/**
 * Cartesian range predicate. Note that for really long ranges, might need to
 * to use geodetic / geocentric distance instead
 */
class RangePredictate
{
public:
  RangePredictate(const Point3D& aOrigin, double aRange) :
    mOrigin(aOrigin),
    mRangeSquared(aRange * aRange)
  { ; }
  
  bool operator()(const FGPositionedRef& aPos)
  {
    Point3D p(Point3D::fromSGGeod(aPos->geod()));
    bool ok = (mOrigin.distance3Dsquared(p) > mRangeSquared);
    if (ok) {
      double x = sqrt(mOrigin.distance3Dsquared(p) - mRangeSquared);
      x *= SG_METER_TO_NM;
      //std::cout << "pos:" << aPos->ident() << " failed range check by " << x << std::endl;
    }
    return ok;
  }
  
private:
  Point3D mOrigin;
  double mRangeSquared;
};

static void
filterListByRange(const SGGeod& aPos, double aRange, FGPositioned::List& aResult)
{
  RangePredictate pred(Point3D::fromSGGeod(aPos), aRange * SG_NM_TO_METER);
  FGPositioned::List::iterator newEnd; 
  newEnd = std::remove_if(aResult.begin(), aResult.end(), pred);
  aResult.erase(newEnd, aResult.end());
}

class DistanceOrdering
{
public:
  DistanceOrdering(const SGGeod& aPos) :
    mPos(Point3D::fromSGGeod(aPos))
  { }
  
  bool operator()(const FGPositionedRef& a, const FGPositionedRef& b) const
  {
    return mPos.distance3Dsquared(Point3D::fromSGGeod(a->geod())) < 
      mPos.distance3Dsquared(Point3D::fromSGGeod(b->geod()));
  }

private:
  Point3D mPos;
};

static void
sortByDistance(const SGGeod& aPos, FGPositioned::List& aResult)
{
  std::sort(aResult.begin(), aResult.end(), DistanceOrdering(aPos));
}

static FGPositionedRef
namedFindClosestTyped(const std::string& aIdent, const SGGeod& aOrigin, 
  FGPositioned::Type aLower, FGPositioned::Type aUpper)
{
  NamedIndexRange range = global_namedIndex.equal_range(aIdent);
  if (range.first == range.second) return NULL;
  
// common case, only one result. looks a bit ugly because these are
// sequential iterators, not random-access ones
  NamedPositionedIndex::const_iterator check = range.first;
  if (++check == range.second) {
    // excellent, only one match in the range - all we care about is the type
    FGPositioned::Type ty = range.first->second->type();
    if ((ty < aLower) || (ty > aUpper)) {
      return NULL; // type check failed
    }
    
    return range.first->second;
  } // of short-circuit logic for single-element range
  
// multiple matches, we need to actually check the distance to each one
  double minDist = HUGE_VAL;
  FGPositionedRef result;
  Point3D origin(Point3D::fromSGGeod(aOrigin));
  
  for (; range.first != range.second; ++range.first) {
  // filter by type
    FGPositioned::Type ty = range.first->second->type();
    if ((ty < aLower) || (ty > aUpper)) {
      continue;
    }
    
  // find distance
    Point3D p(Point3D::fromSGGeod(range.first->second->geod()));
    double ds = origin.distance3Dsquared(p);
    if (ds < minDist) {
      minDist = ds;
      result = range.first->second;
    }
  }
  
  return result;
}

static FGPositioned::List
spatialGetClosest(const SGGeod& aPos, unsigned int aN, double aCutoffNm, const FGPositioned::Filter& aFilter)
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
  
  if (result.size() > aN) {
    result.resize(aN); // truncate at requested number of matches
  }
  
  sortByDistance(aPos, result);
  return result;
}

///////////////////////////////////////////////////////////////////////////////

FGPositioned::FGPositioned() :
  mType(INVALID)
{
}

FGPositioned::FGPositioned(Type ty, const std::string& aIdent, double aLat, double aLon, double aElev) :
  mType(ty),
  mIdent(aIdent),
  mPosition(SGGeod::fromDegFt(aLon, aLat, aElev))
{
  //addToIndices(this);
  //SGReferenced::get(this); // hold an owning ref, for the moment
}

FGPositioned::FGPositioned(Type ty, const std::string& aIdent, const SGGeod& aPos) :
  mType(ty),
  mIdent(aIdent),
  mPosition(aPos)
{
  //addToIndices(this);
  //SGReferenced::get(this); // hold an owning ref, for the moment
}

FGPositioned::~FGPositioned()
{
  //std::cout << "~FGPositioned:" << mIdent << std::endl;
  //removeFromIndices(this);
}

SGBucket
FGPositioned::bucket() const
{
  return SGBucket(mPosition);
}

const char* FGPositioned::nameForType(Type aTy)
{
 switch (aTy) {
 case FIX: return "fix";
 case VOR: return "VOR";
 case NDB: return "NDB";
 case OM: return "outer-marker";
 case MM: return "middle-marker";
 case IM: return "inner-marker";
 case AIRPORT: return "airport";
 case HELIPORT: return "heliport";
 case SEAPORT: return "seaport";
 case WAYPOINT: return "waypoint";
 default:
  return "unknown";
 }
}

///////////////////////////////////////////////////////////////////////////////
// search / query functions

FGPositionedRef
FGPositioned::findClosestWithIdent(const std::string& aIdent, double aLat, double aLon)
{
  return findClosestWithIdent(aIdent, SGGeod::fromDeg(aLon, aLat));
}

FGPositionedRef
FGPositioned::findClosestWithIdent(const std::string& aIdent, const SGGeod& aPos)
{
  return namedFindClosestTyped(aIdent, aPos, INVALID, LAST_TYPE);
}

FGPositioned::List
FGPositioned::findWithinRangeByType(const SGGeod& aPos, double aRangeNm, Type aTy)
{
  List result;
  spatialFindTyped(aPos, aRangeNm, aTy, aTy, result);
  filterListByRange(aPos, aRangeNm, result);
  return result;
}

FGPositioned::List
FGPositioned::findWithinRange(const SGGeod& aPos, double aRangeNm, const Filter& aFilter)
{
  List result;
  spatialFind(aPos, aRangeNm, aFilter, result);
  filterListByRange(aPos, aRangeNm, result);
  return result;
}

FGPositioned::List
FGPositioned::findAllWithIdent(const std::string& aIdent)
{
  List result;
  NamedIndexRange range = global_namedIndex.equal_range(aIdent);
  for (; range.first != range.second; ++range.first) {
    result.push_back(range.first->second);
  }
  
  return result;
}

FGPositioned::List
FGPositioned::findClosestN(const SGGeod& aPos, unsigned int aN, double aCutoffNm, const Filter& aFilter)
{
  return spatialGetClosest(aPos, aN, aCutoffNm, aFilter);
}

