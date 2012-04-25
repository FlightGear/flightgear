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

#include <string>
#include <vector>

#include <simgear/structure/SGSharedPtr.hxx>
#include <simgear/math/SGMath.hxx>

class FGPositioned;
class SGPropertyNode;

typedef SGSharedPtr<FGPositioned> FGPositionedRef;

class FGPositioned : public SGReferenced
{
public:

  typedef enum {
    INVALID = 0,
    AIRPORT,
    HELIPORT,
    SEAPORT,
    RUNWAY,
    TAXIWAY,
    PAVEMENT,
    PARK_STAND,
    WAYPOINT,
    FIX,
    VOR,
    NDB,
    ILS,
    LOC,
    GS,
    OM,
    MM,
    IM,
    DME,
    TACAN,
    OBSTACLE,
    FREQ_GROUND,
    FREQ_TOWER,
    FREQ_ATIS,
    FREQ_AWOS,
    FREQ_APP_DEP,
    FREQ_ENROUTE,
    FREQ_CLEARANCE,
    FREQ_UNICOM,
    LAST_TYPE
  } Type;

  typedef std::vector<FGPositionedRef> List;
  
  virtual ~FGPositioned();
  
  Type type() const
  { return mType; }

  const std::string& ident() const
  { return mIdent; }

  /**
   * Return the name of this positioned. By default this is the same as the
   * ident, but for many derived classes it's more meaningful - the aiport or
   * navaid name, for example.
   */
  virtual const std::string& name() const
  { return mIdent; }

  const SGGeod& geod() const
  { return mPosition; }

  /**
   *  The cartesian position associated with this object
   */
  const SGVec3d& cart() const;

  double latitude() const
  { return mPosition.getLatitudeDeg(); }
  
  double longitude() const
  { return mPosition.getLongitudeDeg(); }
  
  double elevation() const
  { return mPosition.getElevationFt(); }
  
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
    
    /**
     * Test if this filter has a non-empty type range
     */
    bool hasTypeRange() const;
    
    /**
     * Assuming hasTypeRange is true, test if a given type passes the range
     */
    bool passType(Type aTy) const;
    
    bool operator()(FGPositioned* aPos) const
    { return pass(aPos); }
  };
  
  class TypeFilter : public Filter
  {
  public:
    TypeFilter(Type aTy);
    virtual bool pass(FGPositioned* aPos) const;
    void addType(Type aTy);
  private:
      std::vector<Type> types;
  };
    
  static List findWithinRange(const SGGeod& aPos, double aRangeNm, Filter* aFilter = NULL);
        
  static FGPositionedRef findClosestWithIdent(const std::string& aIdent, const SGGeod& aPos, Filter* aFilter = NULL);
  
  /**
   * Find the next item with the specified partial ID, after the 'current' item
   * Note this function is not hyper-efficient, particular where the partial id
   * spans a large number of candidates.
   *
   * @param aCur - Current item, or NULL to retrieve the first item with partial id
   * @param aId - the (partial) id to lookup
   */
  static FGPositionedRef findNextWithPartialId(FGPositionedRef aCur, const std::string& aId, Filter* aFilter = NULL);
  
  /**
   * Find all items with the specified ident
   * @param aFilter - optional filter on items
   */
  static List findAllWithIdent(const std::string& aIdent, Filter* aFilter = NULL, bool aExact = true);
  
  /**
   * As above, but searches names instead of idents
   */
  static List findAllWithName(const std::string& aName, Filter* aFilter = NULL, bool aExact = true);
  
  /**
   * Sort an FGPositionedList by distance from a position
   */
  static void sortByRange(List&, const SGGeod& aPos);
  
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
  static List findClosestN(const SGGeod& aPos, unsigned int aN, double aCutoffNm, Filter* aFilter = NULL);
  
  /**
   * Map a candidate type string to a real type. Returns INVALID if the string
   * does not correspond to a defined type.
   */
  static Type typeFromName(const std::string& aName);
  
  /**
   * Map a type to a human-readable string
   */
  static const char* nameForType(Type aTy);
  
  static FGPositioned* createUserWaypoint(const std::string& aIdent, const SGGeod& aPos);
protected:
  
  FGPositioned(Type ty, const std::string& aIdent, const SGGeod& aPos);
  
  void init(bool aIndexed);
  
  // can't be const right now, navrecord at least needs to fix up the position
  // after navaids are parsed
  SGGeod mPosition; 
  
  SGVec3d mCart; // once mPosition is const, this can be const too
  const Type mType;
  const std::string mIdent;
};

#endif // of FG_POSITIONED_HXX
