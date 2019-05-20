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

#include "config.h"

#include "positioned.hxx"

#include <map>
#include <set>
#include <cstring> // strcmp
#include <algorithm> // for sort
#include <queue>
#include <memory>


#include <simgear/timing/timestamp.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/math/SGGeometry.hxx>
#include <simgear/sg_inlines.h>

#include "Navaids/PositionedOctree.hxx"

using std::string;
using namespace flightgear;

static void validateSGGeod(const SGGeod& geod)
{
  if (SGMisc<double>::isNaN(geod.getLatitudeDeg()) ||
      SGMisc<double>::isNaN(geod.getLongitudeDeg()))
  {
    throw sg_range_exception("position is invalid, NaNs");
  }
}

static bool validateFilter(FGPositioned::Filter* filter)
{
    if (filter->maxType() < filter->minType()) {
        SG_LOG(SG_GENERAL, SG_WARN, "invalid positioned filter specified");
        return false;
    }
    
    return true;
}

const PositionedID FGPositioned::TRANSIENT_ID = -2;

///////////////////////////////////////////////////////////////////////////////

FGPositioned::FGPositioned( PositionedID aGuid,
                            Type ty,
                            const std::string& aIdent,
                            const SGGeod& aPos ):
  mGuid(aGuid),
  mType(ty),
  mIdent(aIdent),
  mPosition(aPos),
  mCart(SGVec3d::fromGeod(mPosition))
{

}

FGPositioned::~FGPositioned()
{
}

// Static method
bool FGPositioned::isAirportType(FGPositioned* pos)
{
    if (!pos) {
      return false;
    }

    return (pos->type() >= AIRPORT) && (pos->type() <= SEAPORT);
}

// Static method
bool FGPositioned::isRunwayType(FGPositioned* pos)
{
  return (pos && pos->type() == RUNWAY);
}

// Static method
bool FGPositioned::isNavaidType(FGPositioned* pos)
{
  if (!pos) {
    return false;
  }

  switch (pos->type()) {
  case NDB:
  case VOR:
  case ILS:
  case LOC:
  case GS:
  case DME:
  case TACAN:
    return true;
  default:
    return false;
  }
}

FGPositionedRef
FGPositioned::createUserWaypoint(const std::string& aIdent, const SGGeod& aPos)
{
  NavDataCache* cache = NavDataCache::instance();
  TypeFilter filter(WAYPOINT);
  FGPositionedList existing = cache->findAllWithIdent(aIdent, &filter, true);
  if (!existing.empty()) {
    SG_LOG(SG_NAVAID, SG_WARN, "attempt to insert duplicate WAYPOINT:" << aIdent);
    return existing.front();
  }
  
  PositionedID id = cache->createPOI(WAYPOINT, aIdent, aPos);
  return cache->loadById(id);
}

bool FGPositioned::deleteUserWaypoint(const std::string& aIdent)
{
  NavDataCache* cache = NavDataCache::instance();
  return cache->removePOI(WAYPOINT, aIdent);
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
    {"heliport", HELIPORT},
    {"seaport", SEAPORT},
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
    {"clearance", FREQ_CLEARANCE},
    {"unicom", FREQ_UNICOM},
    {"runway", RUNWAY},
    {"helipad", HELIPAD},
    {"country", COUNTRY},
    {"city", CITY},
    {"town", TOWN},
    {"village", VILLAGE},
    {"taxiway", TAXIWAY},
    {"pavement", PAVEMENT},
    {"om", OM},
    {"mm", MM},
    {"im", IM},
    {"mobile-tacan", MOBILE_TACAN},
    {"obstacle", OBSTACLE},
    {"parking", PARKING},
    {"taxi-node",TAXI_NODE},

  // aliases
    {"localizer", LOC},
    {"gnd", FREQ_GROUND},
    {"twr", FREQ_TOWER},
    {"waypoint", WAYPOINT},
    {"apt", AIRPORT},
    {"arpt", AIRPORT},
    {"rwy", RUNWAY},
    {"any", INVALID},
    {"all", INVALID},
    {"outer-marker", OM},
    {"middle-marker", MM},
    {"inner-marker", IM},
    {"parking-stand", PARKING},

    {NULL, INVALID}
  };
  
  std::string lowerName = simgear::strutils::lowercase(aName);
  
  for (const NameTypeEntry* n = names; (n->_name != NULL); ++n) {
    if (::strcmp(n->_name, lowerName.c_str()) == 0) {
      return n->_ty;
    }
  }
  
  SG_LOG(SG_NAVAID, SG_WARN, "FGPositioned::typeFromName: couldn't match:" << aName);
  return INVALID;
}

const char* FGPositioned::nameForType(Type aTy)
{
 switch (aTy) {
 case RUNWAY: return "runway";
 case HELIPAD: return "helipad";
 case TAXIWAY: return "taxiway";
 case PAVEMENT: return "pavement";
 case PARKING: return "parking stand";
 case FIX: return "fix";
 case VOR: return "VOR";
 case NDB: return "NDB";
 case ILS: return "ILS";
 case LOC: return "localizer";
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
 case TAXI_NODE: return "taxi-node";
 case COUNTRY: return "country";
 case CITY: return "city";
 case TOWN: return "town";
 case VILLAGE: return "village";
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
  return NavDataCache::instance()->findClosestWithIdent(aIdent, aPos, aFilter);
}

FGPositionedRef
FGPositioned::findFirstWithIdent(const std::string& aIdent, Filter* aFilter)
{
  if (aIdent.empty()) {
    return NULL;
  }
  
  FGPositionedList r =
    NavDataCache::instance()->findAllWithIdent(aIdent, aFilter, true);
  if (r.empty()) {
    return NULL;
  }
  
  return r.front();
}

FGPositionedList
FGPositioned::findWithinRange(const SGGeod& aPos, double aRangeNm, Filter* aFilter)
{
  validateSGGeod(aPos);

    if (!validateFilter(aFilter)) {
        return FGPositionedList();
    }
    
  FGPositionedList result;
  Octree::findAllWithinRange(SGVec3d::fromGeod(aPos), 
    aRangeNm * SG_NM_TO_METER, aFilter, result, 0xffffff);
  return result;
}

FGPositionedList
FGPositioned::findWithinRangePartial(const SGGeod& aPos, double aRangeNm, Filter* aFilter, bool& aPartial)
{
  validateSGGeod(aPos);
  
    if (!validateFilter(aFilter)) {
        return FGPositionedList();
    }
    
  int limitMsec = 32;
  FGPositionedList result;
  aPartial = Octree::findAllWithinRange(SGVec3d::fromGeod(aPos),
                             aRangeNm * SG_NM_TO_METER, aFilter, result,
                                        limitMsec);
  return result;
}

FGPositionedList
FGPositioned::findAllWithIdent(const std::string& aIdent, Filter* aFilter, bool aExact)
{
    if (!validateFilter(aFilter)) {
        return FGPositionedList();
    }
    
  return NavDataCache::instance()->findAllWithIdent(aIdent, aFilter, aExact);
}

FGPositionedList
FGPositioned::findAllWithName(const std::string& aName, Filter* aFilter, bool aExact)
{
    if (!validateFilter(aFilter)) {
        return FGPositionedList();
    }
    
  return NavDataCache::instance()->findAllWithName(aName, aFilter, aExact);
}

FGPositionedRef
FGPositioned::findClosest(const SGGeod& aPos, double aCutoffNm, Filter* aFilter)
{
  validateSGGeod(aPos);
  
    if (!validateFilter(aFilter)) {
        return NULL;
    }
    
  FGPositionedList l(findClosestN(aPos, 1, aCutoffNm, aFilter));
  if (l.empty()) {
    return NULL;
  }

  assert(l.size() == 1);
  return l.front();
}

FGPositionedList
FGPositioned::findClosestN(const SGGeod& aPos, unsigned int aN, double aCutoffNm, Filter* aFilter)
{
  validateSGGeod(aPos);
  
  FGPositionedList result;
  int limitMsec = 0xffff;
  Octree::findNearestN(SGVec3d::fromGeod(aPos), aN, aCutoffNm * SG_NM_TO_METER, aFilter, result, limitMsec);
  return result;
}

FGPositionedList
FGPositioned::findClosestNPartial(const SGGeod& aPos, unsigned int aN, double aCutoffNm, Filter* aFilter, bool &aPartial)
{
    validateSGGeod(aPos);
    
    FGPositionedList result;
    int limitMsec = 32;
    aPartial = Octree::findNearestN(SGVec3d::fromGeod(aPos), aN, aCutoffNm * SG_NM_TO_METER, aFilter, result,
                        limitMsec);
    return result;
}

void
FGPositioned::sortByRange(FGPositionedList& aResult, const SGGeod& aPos)
{
  validateSGGeod(aPos);
  
  SGVec3d cartPos(SGVec3d::fromGeod(aPos));
// computer ordering values
  Octree::FindNearestResults r;
  FGPositionedList::iterator it = aResult.begin(), lend = aResult.end();
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

//------------------------------------------------------------------------------
void FGPositioned::modifyPosition(const SGGeod& newPos)
{
  const_cast<SGGeod&>(mPosition) = newPos;
  const_cast<SGVec3d&>(mCart) = SGVec3d::fromGeod(newPos);
}

//------------------------------------------------------------------------------
void FGPositioned::invalidatePosition()
{
  const_cast<SGGeod&>(mPosition) = SGGeod::fromDeg(999,999);
  const_cast<SGVec3d&>(mCart) = SGVec3d::zeros();
}

//------------------------------------------------------------------------------
FGPositionedRef FGPositioned::loadByIdImpl(PositionedID id)
{
  return flightgear::NavDataCache::instance()->loadById(id);
}

FGPositioned::TypeFilter::TypeFilter(Type aTy)
{
  addType(aTy);
}

FGPositioned::TypeFilter::TypeFilter(std::initializer_list<Type> types)
{
    for (auto t : types) {
        addType(t);
    }
}

void FGPositioned::TypeFilter::addType(Type aTy)
{
  if (aTy == INVALID) {
    return;
  }
  
  types.push_back(aTy);
  mMinType = std::min(mMinType, aTy);
  mMaxType = std::max(mMaxType, aTy);
}

FGPositioned::TypeFilter
FGPositioned::TypeFilter::fromString(const std::string& aFilterSpec)
{
  if (aFilterSpec.empty()) {
    throw sg_format_exception("empty filter spec:", aFilterSpec);
  }
  
  string_list parts = simgear::strutils::split(aFilterSpec, ",");
  TypeFilter f;
  
  for (std::string token : parts) {
    f.addType(typeFromName(token));
  }
  
  return f;
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
        if (aPos->type() == *it) {
            return true;
        }
    }
    
    return false;
}

