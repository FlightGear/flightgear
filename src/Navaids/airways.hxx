// airways.hxx - storage of airways network, and routing between nodes
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

#ifndef FG_AIRWAYS_HXX
#define FG_AIRWAYS_HXX

#include <map>
#include <vector>

#include <Navaids/route.hxx>
#include <Navaids/positioned.hxx>

class SGPath;
typedef SGSharedPtr<FGPositioned> FGPositionedRef; 

namespace flightgear {

// forward declare some helpers
struct SearchContext;
class AdjacentWaypoint;
class InAirwayFilter;
class Airway;

using AirwayRef = SGSharedPtr<Airway>;

class Airway : public RouteBase
{
public:
    enum Level {
        UnknownLevel = 0,
        LowLevel = 1, /// Victor airways
        HighLevel = 2,  /// Jet airways
        Both = 3
    };
    
  std::string ident() const override
  { return _ident; }
  
  int cacheId() const
  { return _cacheId; }

  Level level() const
  { return _level; }

  static void loadAWYDat(const SGPath& path);
  
    double topAltitudeFt() const
    { return _topAltitudeFt; }
    
    double bottomAltitudeFt() const
    { return _bottomAltitudeFt; }

    static AirwayRef loadByCacheId(int cacheId);

    static AirwayRef findByIdent(const std::string& aIdent, Level level);

    /**
     * Find the airway based on its ident. IF both high- and low- level idents
     * exist, select the one which can route between the from and to waypoints
     * correctly, preferring high-level airways.
     */
    static AirwayRef findByIdentAndVia(const std::string& aIdent, const WayptRef& from, const WayptRef& to);
    
    /**
     * Find an airway by ident, and containing a particula rnavaid/fix. 
     */
    static AirwayRef findByIdentAndNavaid(const std::string& aIdent, const FGPositionedRef nav);

    WayptRef findEnroute(const std::string& aIdent) const;

    bool canVia(const WayptRef& from, const WayptRef& to) const;

    WayptVec via(const WayptRef& from, const WayptRef& to) const;

    bool containsNavaid(const FGPositionedRef& navaid) const;

    

  /**
   * Track a network of airways
   *
   */
  class Network
  {
  public:
    friend class Airway;
    friend class InAirwayFilter;
    
  
    /**
     * Principal routing algorithm. Attempts to find the best route beween
     * two points. If either point is part of the airway network (e.g, a SID
     * or STAR transition), it will <em>not</em> be duplicated in the result
     * path.
     *
     * Returns true if a route could be found, or false otherwise.
     */
    bool route(WayptRef aFrom, WayptRef aTo, WayptVec& aPath);

    /**
     * Overloaded version working with a raw SGGeod
     */

    std::pair<FGPositionedRef, bool> findClosestNode(const SGGeod& aGeod);

    FGPositionedRef findNodeByIdent(const std::string& ident, const SGGeod& near) const;
  private:    
    void addEdge(int aWay, const SGGeod& aStartPos,
                const std::string& aStartIdent, 
                const SGGeod& aEndPos, const std::string& aEndIdent);
  
    int findAirway(const std::string& aName);

    bool cleanGeneratedPath(WayptRef aFrom, WayptRef aTo, WayptVec& aPath,
                            bool exactTo, bool exactFrom);
      
    bool search2(FGPositionedRef aStart, FGPositionedRef aDest, WayptVec& aRoute);
  
    /**
     * Test if a positioned item is part of this airway network or not.
     */
    bool inNetwork(PositionedID pos) const;
    
    /**
     * Find the closest node on the network, to the specified waypoint
     *
     * May return NULL,false if no match could be found; the search is
     * internally limited to avoid very poor performance; for example, 
     * in the middle of an ocean.
     *
     * The second return value indicates if the returned value is
     * equal (true) or distinct (false) to the input waypoint.
     * Equality here means being physically within a close tolerance,
     * on the order of a hundred metres.
     */
    std::pair<FGPositionedRef, bool> findClosestNode(WayptRef aRef);
    
    /**
     * cache which positioned items are in this network
     */
    typedef std::map<PositionedID, bool> NetworkMembershipDict;
    mutable NetworkMembershipDict _inNetworkCache;
    
    Level _networkID;
  };


  static Network* highLevel();
  static Network* lowLevel();
  
private:
  Airway(const std::string& aIdent, const Level level, int dbId, int aTop, int aBottom);

  void loadWaypoints() const;

  WayptVec::const_iterator find(WayptRef wpt) const;

  friend class Network;
    friend class NavDataCache;
    
  const std::string _ident;
  const Level _level;
  const int _cacheId;

  int _topAltitudeFt;
  int _bottomAltitudeFt;

  mutable WayptVec _elements;
};

} // of namespace flightgear


#endif //of FG_AIRWAYS_HXX
