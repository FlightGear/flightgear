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
#include <simgear/structure/subsystem_mgr.hxx>

#include <Navaids/FlightPlan.hxx>

// forward decls
class SGPath;
class PropertyWatcher;

/**
 * Top level route manager class
 * 
 */

class FGRouteMgr : public SGSubsystem, 
                   public flightgear::FlightPlan::Delegate
{
public:
  FGRouteMgr();
  ~FGRouteMgr();

  void init ();
  void postinit ();
  void bind ();
  void unbind ();
  void update (double dt);
  
  bool isRouteActive() const;
         
  int currentIndex() const;
  
  void setFlightPlan(flightgear::FlightPlan* plan);
  flightgear::FlightPlan* flightPlan() const;
  
  void clearRoute();
  
  flightgear::Waypt* currentWaypt() const;
  
  int numLegs() const;
  
// deprecated
  int numWaypts() const
  { return numLegs(); }
  
// deprecated
  flightgear::Waypt* wayptAtIndex(int index) const;
  
  SGPropertyNode_ptr wayptNodeAtIndex(int index) const;
  
  void removeLegAtIndex(int aIndex);
  
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
  
  flightgear::WayptRef waypointFromString(const std::string& target);
  
  /**
   * Helper command to setup current airport/runway if necessary
   */
  void initAtPosition();

private:
    flightgear::FlightPlan* _plan;
  
    time_t _takeoffTime;
    time_t _touchdownTime;

    // automatic inputs
    SGPropertyNode_ptr magvar;
    
    // automatic outputs    
    SGPropertyNode_ptr departure; ///< departure airport information
    SGPropertyNode_ptr destination; ///< destination airport information
    SGPropertyNode_ptr alternate; ///< alternate airport information
    SGPropertyNode_ptr cruise; ///< cruise information
    
    SGPropertyNode_ptr totalDistance;
    SGPropertyNode_ptr distanceToGo;
    SGPropertyNode_ptr ete;
    SGPropertyNode_ptr elapsedFlightTime;
    
    SGPropertyNode_ptr active;
    SGPropertyNode_ptr airborne;
    
    SGPropertyNode_ptr wp0;
    SGPropertyNode_ptr wp1;
    SGPropertyNode_ptr wpn;
    
    
    SGPropertyNode_ptr _pathNode;
    SGPropertyNode_ptr _currentWpt;
    
    
    /** 
     * Signal property to notify people that the route was edited
     */
    SGPropertyNode_ptr _edited;
    
    /**
     * Signal property to notify when the last waypoint is reached
     */
    SGPropertyNode_ptr _finished;
    
    SGPropertyNode_ptr _flightplanChanged;
  
    void setETAPropertyFromDistance(SGPropertyNode_ptr aProp, double aDistance);
    
    /**
     * retrieve the cached path distance along a leg
     */
    double cachedLegPathDistanceM(int index) const;
    double cachedWaypointPathTotalDistance(int index) const;
  
    class InputListener : public SGPropertyChangeListener {
    public:
        InputListener(FGRouteMgr *m) : mgr(m) {}
        virtual void valueChanged (SGPropertyNode * prop);
    private:
        FGRouteMgr *mgr;
    };

    SGPropertyNode_ptr input;
    SGPropertyNode_ptr weightOnWheels;
    SGPropertyNode_ptr groundSpeed;
  
    InputListener *listener;
    SGPropertyNode_ptr mirror;    
  
    /**
     * Helper to keep various pieces of state in sync when the route is
     * modified (waypoints added, inserted, removed). Notably, this fires the
     * 'edited' signal.
     */
    virtual void waypointsChanged();
    
    void update_mirror();
    
    virtual void currentWaypointChanged();
    
    /**
     * Check if we've reached the final waypoint. 
     * Returns true if we have.
     */
    bool checkFinished();
    
    /**
     * Predicate for helping the UI - test if at least one waypoint was
     * entered by the user (as opposed to being generated by the route-manager)
     */
    bool haveUserWaypoints() const;
    
// tied getters and setters
    const char* getDepartureICAO() const;
    const char* getDepartureName() const;
    void setDepartureICAO(const char* aIdent);
    
    const char* getDepartureRunway() const;
    void setDepartureRunway(const char* aIdent);
  
    const char* getSID() const;
    void setSID(const char* aIdent);
  
    const char* getDestinationICAO() const;
    const char* getDestinationName() const;
    void setDestinationICAO(const char* aIdent);

    const char* getDestinationRunway() const;
    void setDestinationRunway(const char* aIdent);
  
    const char* getApproach() const;
    void setApproach(const char* aIdent);
  
    const char* getSTAR() const;
    void setSTAR(const char* aIdent);
  
    double getDepartureFieldElevation() const;  
    double getDestinationFieldElevation() const;  
};


#endif // _ROUTE_MGR_HXX
