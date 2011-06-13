// route_mgr.hxx - manage a route (i.e. a collection of waypoints)
//
// Written by Curtis Olson, started January 2004.
//
// Copyright (C) 2004  Curtis L. Olson  - http://www.flightgear.org/~curt
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


#ifndef _ROUTE_MGR_HXX
#define _ROUTE_MGR_HXX 1

#include <simgear/props/props.hxx>
#include <simgear/route/waypoint.hxx>
#include <simgear/structure/subsystem_mgr.hxx>

#include <Navaids/route.hxx>

// forward decls
class SGPath;
class PropertyWatcher;

class FGAirport;
class FGRunway;

typedef SGSharedPtr<FGAirport> FGAirportRef;

/**
 * Top level route manager class
 * 
 */

class FGRouteMgr : public SGSubsystem
{
public:
  FGRouteMgr();
  ~FGRouteMgr();

  void init ();
  void postinit ();
  void bind ();
  void unbind ();
  void update (double dt);

  void insertWayptAtIndex(flightgear::Waypt* aWpt, int aIndex);
  flightgear::WayptRef removeWayptAtIndex(int index);
  
  void clearRoute();
  
  typedef enum {
    ROUTE_HIGH_AIRWAYS, ///< high-level airways routing
    ROUTE_LOW_AIRWAYS, ///< low-level airways routing
    ROUTE_VOR ///< VOR-VOR routing
  } RouteType;
  
  /**
   * Insert waypoints from index-1 to index. In practice this means you can
   * 'fill in the gaps' between defined waypoints. If index=0, the departure
   * airport is used as index-1; if index is -1, the destination airport is
   * used as the final waypoint.
   */
  bool routeToIndex(int index, RouteType aRouteType);

  void autoRoute();
        
  bool isRouteActive() const;
         
  int currentIndex() const
    { return _currentIndex; }
    
  flightgear::Waypt* currentWaypt() const;
  flightgear::Waypt* nextWaypt() const;
  flightgear::Waypt* previousWaypt() const;
  
  const flightgear::WayptVec& waypts() const
    { return _route; }
  
  int numWaypts() const
    { return _route.size(); }
    
  flightgear::Waypt* wayptAtIndex(int index) const;
             
  /**
   * Find a waypoint in the route, by position, and return its index, or
   * -1 if no matching waypoint was found in the route.
   */
  int findWayptIndex(const SGGeod& aPos) const;
        
  /**
   * Activate a built route. This checks for various mandatory pieces of
   * data, such as departure and destination airports, and creates waypoints
   * for them on the route structure.
   *
   * returns true if the route was activated successfully, or false if the
   * route could not be activated for some reason
   */
  bool activate();

  /**
   * Step to the next waypoint on the active route
   */
  void sequence();
  
  /**
   * Set the current waypoint to the specified index.
   */
  void jumpToIndex(int index);
  
  bool saveRoute(const SGPath& p);
  bool loadRoute(const SGPath& p);
  
  /**
   * Helper command to setup current airport/runway if necessary
   */
  void initAtPosition();
  
    /**
     * Create a WayPoint from a string in the following format:
     *  - simple identifier
     *  - decimal-lon,decimal-lat
     *  - airport-id/runway-id
     *  - navaid/radial-deg/offset-nm
     */
    flightgear::WayptRef waypointFromString(const std::string& target);
    
    FGAirportRef departureAirport() const;
    FGAirportRef destinationAirport() const;
    
    FGRunway* departureRunway() const;
    FGRunway* destinationRunway() const;
private:
  flightgear::WayptVec _route;
  int _currentIndex;
  
    time_t _takeoffTime;
    time_t _touchdownTime;
    FGAirportRef _departure;
    FGAirportRef _destination;
    
    // automatic inputs
    SGPropertyNode_ptr lon;
    SGPropertyNode_ptr lat;
    SGPropertyNode_ptr alt;
    SGPropertyNode_ptr magvar;
    
    // automatic outputs    
    SGPropertyNode_ptr departure; ///< departure airport information
    SGPropertyNode_ptr destination; ///< destination airport information
    SGPropertyNode_ptr alternate; ///< alternate airport information
    SGPropertyNode_ptr cruise; ///< cruise information
    
    SGPropertyNode_ptr totalDistance;
    SGPropertyNode_ptr ete;
    SGPropertyNode_ptr elapsedFlightTime;
    
    SGPropertyNode_ptr active;
    SGPropertyNode_ptr airborne;
    
    SGPropertyNode_ptr wp0;
    SGPropertyNode_ptr wp1;
    SGPropertyNode_ptr wpn;
    
    
    SGPropertyNode_ptr _pathNode;
    SGPropertyNode_ptr _currentWpt;
    
    /// integer property corresponding to the RouteType enum
    SGPropertyNode_ptr _routingType;
    
    /** 
     * Signal property to notify people that the route was edited
     */
    SGPropertyNode_ptr _edited;
    
    /**
     * Signal property to notify when the last waypoint is reached
     */
    SGPropertyNode_ptr _finished;
    
    void setETAPropertyFromDistance(SGPropertyNode_ptr aProp, double aDistance);
    
    class InputListener : public SGPropertyChangeListener {
    public:
        InputListener(FGRouteMgr *m) : mgr(m) {}
        virtual void valueChanged (SGPropertyNode * prop);
    private:
        FGRouteMgr *mgr;
    };

    SGPropertyNode_ptr input;
    SGPropertyNode_ptr weightOnWheels;
    
    InputListener *listener;
    SGPropertyNode_ptr mirror;    
    
    void departureChanged();
    void buildDeparture(flightgear::WayptRef enroute, flightgear::WayptVec& wps);
    
    void arrivalChanged();
    void buildArrival(flightgear::WayptRef enroute, flightgear::WayptVec& wps);
    
    /**
     * Helper to keep various pieces of state in sync when the route is
     * modified (waypoints added, inserted, removed). Notably, this fires the
     * 'edited' signal.
     */
    void waypointsChanged();
    
    void update_mirror();
    
    void currentWaypointChanged();
    
    /**
     * Parse a route/wp node (from a saved, property-lsit formatted route)
     */
    void parseRouteWaypoint(SGPropertyNode* aWP);
    
    /**
     * Check if we've reached the final waypoint. 
     * Returns true if we have.
     */
    bool checkFinished();
    
    
    bool loadPlainTextRoute(const SGPath& path);
    
    void loadVersion1XMLRoute(SGPropertyNode_ptr routeData);
    void loadVersion2XMLRoute(SGPropertyNode_ptr routeData);
    void loadXMLRouteHeader(SGPropertyNode_ptr routeData);
    flightgear::WayptRef parseVersion1XMLWaypt(SGPropertyNode* aWP);
    
    /**
     * Predicate for helping the UI - test if at least one waypoint was
     * entered by the user (as opposed to being generated by the route-manager)
     */
    bool haveUserWaypoints() const;
    
// tied getters and setters
    const char* getDepartureICAO() const;
    const char* getDepartureName() const;
    void setDepartureICAO(const char* aIdent);
    
    const char* getDestinationICAO() const;
    const char* getDestinationName() const;
    void setDestinationICAO(const char* aIdent);

    PropertyWatcher* _departureWatcher;
    PropertyWatcher* _arrivalWatcher;
};


#endif // _ROUTE_MGR_HXX
