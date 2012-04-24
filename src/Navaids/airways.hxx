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

class FGPositioned;
typedef SGSharedPtr<FGPositioned> FGPositionedRef; 

namespace flightgear {

// forward declare some helpers
struct SearchContext;
class AdjacentWaypoint;
class InAirwayFilter;

class Airway : public RouteBase
{
public:
  virtual std::string ident() const
  { return _ident; }
  
  static void load();
  
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
  private:    
    void addEdge(Airway* aWay, const SGGeod& aStartPos, 
                const std::string& aStartIdent, 
                const SGGeod& aEndPos, const std::string& aEndIdent);
  
    Airway* findAirway(const std::string& aName, double aTop, double aBase);
    
    typedef std::multimap<FGPositioned*, AdjacentWaypoint*> AdjacencyMap;
    AdjacencyMap _graph;
    
    typedef std::vector<AdjacentWaypoint*> AdjacentWaypointVec;

    typedef std::map<std::string, Airway*> AirwayDict;
    AirwayDict _airways;

    bool search2(FGPositionedRef aStart, FGPositionedRef aDest, WayptVec& aRoute);
  
    /**
     * Test if a positioned item is part of this airway network or not.
     */
    bool inNetwork(const FGPositioned* aRef) const;
    
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
     * Overloaded version working with a raw SGGeod
     */
    std::pair<FGPositionedRef, bool> findClosestNode(const SGGeod& aGeod);
  };


  static Network* highLevel();
  static Network* lowLevel();
  
private:
  Airway(const std::string& aIdent, double aTop, double aBottom);

  friend class Network;
  
  static Network* static_highLevel;
  static Network* static_lowLevel;
  
  std::string _ident;
  double _topAltitudeFt;
  double _bottomAltitudeFt;

  // high-level vs low-level flag
  // ... ?
  
  WayptVec _elements;
};

} // of namespace flightgear


#endif //of FG_AIRWAYS_HXX
