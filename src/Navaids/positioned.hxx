// positioned.hxx - base class for objects which are positioned
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

#ifndef FG_POSITIONED_HXX
#define FG_POSITIONED_HXX

#include <cassert>
#include <string>
#include <vector>
#include <stdint.h>

#include <simgear/sg_inlines.h>
#include <simgear/structure/SGSharedPtr.hxx>
#include <simgear/math/SGMath.hxx>

class FGPositioned;
typedef SGSharedPtr<FGPositioned> FGPositionedRef;
typedef std::vector<FGPositionedRef> FGPositionedList;

typedef int64_t PositionedID;
typedef std::vector<PositionedID> PositionedIDVec;

namespace flightgear { class NavDataCache; }

class FGPositioned : public SGReferenced
{
public:
    static const PositionedID TRANSIENT_ID;

  typedef enum {
    INVALID = 0,
    AIRPORT,
    HELIPORT,
    SEAPORT,
    RUNWAY,
    HELIPAD,
    TAXIWAY,
    PAVEMENT,
    WAYPOINT,
    FIX,
    NDB,
    VOR,
    ILS,
    LOC,
    GS,
    OM,
    MM,
    IM,
/// important that DME & TACAN are adjacent to keep the TacanFilter
/// efficient - DMEs are proxies for TACAN/VORTAC stations
    DME,
    TACAN,
    MOBILE_TACAN,
    OBSTACLE,
/// an actual airport tower - not a radio comms facility!
/// some airports have multiple towers, eg EHAM, although our data source
/// doesn't necessarily include them     
    TOWER,
    FREQ_GROUND,
    FREQ_TOWER,
    FREQ_ATIS,
    FREQ_AWOS,
    FREQ_APP_DEP,
    FREQ_ENROUTE,
    FREQ_CLEARANCE,
    FREQ_UNICOM,
// groundnet items
    PARKING,  ///< parking position - might be a gate, or stand
    TAXI_NODE,
// POI items
    COUNTRY,
    CITY,
    TOWN,
    VILLAGE,
      
    LAST_TYPE
  } Type;

  virtual ~FGPositioned();
  
  Type type() const
  { return mType; }

  // True for the following types: AIRPORT, HELIPORT, SEAPORT.
  // False for other types, as well as if pos == nullptr.
  static bool isAirportType(FGPositioned* pos);
  // True for the following type: RUNWAY.
  // False for other types, as well as if pos == nullptr.
  static bool isRunwayType(FGPositioned* pos);
  // True for the following types: NDB, VOR, ILS, LOC, GS, DME, TACAN.
  // False for other types, as well as if pos == nullptr.
  static bool isNavaidType(FGPositioned* pos);

  const char* typeString() const
  { return nameForType(mType); }

  const std::string& ident() const
  { return mIdent; }

  /**
   * Return the name of this positioned. By default this is the same as the
   * ident, but for many derived classes it's more meaningful - the aiport or
   * navaid name, for example.
   */
  virtual const std::string& name() const
  { return mIdent; }

  virtual const SGGeod& geod() const
  { return mPosition; }
  
  PositionedID guid() const
  { return mGuid; }

  /**
   *  The cartesian position associated with this object
   */
  virtual const SGVec3d& cart() const;

  double latitude() const
  { return geod().getLatitudeDeg(); }
  
  double longitude() const
  { return geod().getLongitudeDeg(); }
  
  double elevation() const
  { return geod().getElevationFt(); }
  
  double elevationM() const
  { return geod().getElevationM(); }

  /**
   * Predicate class to support custom filtering of FGPositioned queries
   * Default implementation of this passes any FGPositioned instance.
   */
  class Filter
  {
  public:
    virtual ~Filter() { ; }
    
    /**
     * Over-rideable filter method. Default implementation returns true.
     */
    virtual bool pass(FGPositioned* aPos) const
    { return true; }
    
    virtual Type minType() const
    { return INVALID; }
    
    virtual Type maxType() const
    { return INVALID; }
    
    
    bool operator()(FGPositioned* aPos) const
    { return pass(aPos); }
  };
  
  class TypeFilter : public Filter
  {
  public:
    TypeFilter(Type aTy = INVALID);
      
    TypeFilter(std::initializer_list<Type> types);
      
    bool pass(FGPositioned* aPos) const override;
    
    Type minType() const override
    { return mMinType; }
    
    Type maxType() const override
    { return mMaxType; }
    
    void addType(Type aTy);
      
    static TypeFilter fromString(const std::string& aFilterSpec);
  private:
      
    std::vector<Type> types;
    Type mMinType = LAST_TYPE,
      mMaxType = INVALID;
  };
  
  static FGPositionedList findWithinRange(const SGGeod& aPos, double aRangeNm, Filter* aFilter);
  
  static FGPositionedList findWithinRangePartial(const SGGeod& aPos, double aRangeNm, Filter* aFilter, bool& aPartial);
        
  static FGPositionedRef findClosestWithIdent(const std::string& aIdent, const SGGeod& aPos, Filter* aFilter = NULL);

  static FGPositionedRef findFirstWithIdent(const std::string& aIdent, Filter* aFilter);

  /**
   * Find all items with the specified ident
   * @param aFilter - optional filter on items
   */
  static FGPositionedList findAllWithIdent(const std::string& aIdent, Filter* aFilter = NULL, bool aExact = true);
  
  /**
   * As above, but searches names instead of idents
   */
  static FGPositionedList findAllWithName(const std::string& aName, Filter* aFilter = NULL, bool aExact = true);
  
  /**
   * Sort an FGPositionedList by distance from a position
   */
  static void sortByRange(FGPositionedList&, const SGGeod& aPos);
  
  /**
   * Find the closest item to a position, which pass the specified filter
   * A cutoff range in NM must be specified, to constrain the search acceptably.
   * Very large cutoff values will make this slow.
   * 
   * @result The closest item passing the filter, or NULL
   * @param aCutoffNm - maximum distance to search within, in nautical miles
   */
  static FGPositionedRef findClosest(const SGGeod& aPos, double aCutoffNm, Filter* aFilter = NULL);
  
  /**
   * Find the closest N items to a position, which pass the specified filter
   * A cutoff range in NM must be specified, to constrain the search acceptably.
   * Very large cutoff values will make this slow.
   * 
   * @result The matches (possibly less than N, depending on the filter and cutoff),
   *    sorted by distance from the search pos
   * @param aN - number of matches to find
   * @param aCutoffNm - maximum distance to search within, in nautical miles
   */
  static FGPositionedList findClosestN(const SGGeod& aPos, unsigned int aN, double aCutoffNm, Filter* aFilter = NULL);
    
  /**
   * Same as above, but with a time-bound in msec too.
   */
  static FGPositionedList findClosestNPartial(const SGGeod& aPos, unsigned int aN, double aCutoffNm, Filter* aFilter,
                           bool& aPartial);
  
  template<class T>
  static SGSharedPtr<T> loadById(PositionedID id)
  {
    return static_pointer_cast<T>( loadByIdImpl(id) );
  }

  template<class T>
  static SGSharedPtr<T> loadById(const PositionedIDVec& id_vec, size_t index)
  {
    assert(index >= 0 && index < id_vec.size());
    return loadById<T>(id_vec[index]);
  }

  template<class T>
  static std::vector<SGSharedPtr<T> > loadAllById(const PositionedIDVec& id_vec)
  {
    std::vector<SGSharedPtr<T> > vec(id_vec.size());

    for(size_t i = 0; i < id_vec.size(); ++i)
      vec[i] = loadById<T>(id_vec[i]);

    return vec;
  }

  /**
   * Map a candidate type string to a real type. Returns INVALID if the string
   * does not correspond to a defined type.
   */
  static Type typeFromName(const std::string& aName);

  /**
   * Map a type to a human-readable string
   */
  static const char* nameForType(Type aTy);

  static FGPositionedRef createUserWaypoint(const std::string& aIdent, const SGGeod& aPos);
  static bool deleteUserWaypoint(const std::string& aIdent);
protected:
  friend class flightgear::NavDataCache;

  FGPositioned(PositionedID aGuid, Type ty, const std::string& aIdent, const SGGeod& aPos);

  void modifyPosition(const SGGeod& newPos);
  void invalidatePosition();

  static FGPositionedRef loadByIdImpl(PositionedID id);

  const PositionedID mGuid;
  const Type mType;
  const std::string mIdent;
  
private:
  SG_DISABLE_COPY(FGPositioned);

  const SGGeod mPosition;
  const SGVec3d mCart;
};

template <class T>
T* fgpositioned_cast(FGPositioned* p)
{
    if (T::isType(p->type())) {
        return static_cast<T*>(p);
    }

    return nullptr;
}

template <class T>
T* fgpositioned_cast(FGPositionedRef p)
{
    if (!p) return nullptr;

    if (T::isType(p->type())) {
        return static_cast<T*>(p.ptr());
    }

    return nullptr;
}

#endif // of FG_POSITIONED_HXX
