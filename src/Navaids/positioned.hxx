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

#include <vector>

#include <simgear/structure/SGSharedPtr.hxx>
#include <simgear/bucket/newbucket.hxx>

class FGPositioned;

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
    PARK_STAND,
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
    WAYPOINT, // user-defined waypoint
    FREQ_GND,
    FREQ_TWR,
    FREQ_ATIS,
    LAST_TYPE
  } Type;

  typedef std::vector<FGPositionedRef> List;
  
  virtual ~FGPositioned();
  
  Type type() const
  { return mType; }

  const std::string& ident() const
  { return mIdent; }

  const SGGeod& geod() const
  { return mPosition; }

  SGBucket bucket() const;
  
  double latitude() const
  { return mPosition.getLatitudeDeg(); }
  
  double longitude() const
  { return mPosition.getLongitudeDeg(); }
  
  double elevation() const
  { return mPosition.getElevationFt(); }
  
  /**
   * Predicate class to support custom filtering of FGPositioned queries
   */
  class Filter
  {
  public:
    virtual ~Filter() { ; }
    
    virtual bool pass(FGPositioned* aPos) const = 0;
    
    bool operator()(FGPositioned* aPos) const
    { return pass(aPos); }
  };
  
  static List findWithinRange(const SGGeod& aPos, double aRangeNm, const Filter& aFilter);
      
  static List findWithinRangeByType(const SGGeod& aPos, double aRangeNm, Type aTy);

  static FGPositionedRef findClosestWithIdent(const std::string& aIdent, double aLat, double aLon);
  
  static FGPositionedRef findClosestWithIdent(const std::string& aIdent, const SGGeod& aPos);
  
  static List findAllWithIdent(const std::string& aIdent);
  
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
  static List findClosestN(const SGGeod& aPos, unsigned int aN, double aCutoffNm, const Filter& aFilter);
  
  /**
   * Debug helper, map a type to a human-readable string
   */
  static const char* nameForType(Type aTy);
protected:
  FGPositioned(Type ty, const std::string& aIdent, double aLat, double aLon, double aElev);
  
  FGPositioned(Type ty, const std::string& aIdent, const SGGeod& aPos);
  
  SGGeod mPosition; // can't be const right now
  
  Type mType;
  std::string mIdent;
};

#endif // of FG_POSITIONED_HXX
