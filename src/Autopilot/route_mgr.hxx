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

// forward decls
class SGRoute;
class SGPath;

class FGAirport;
typedef SGSharedPtr<FGAirport> FGAirportRef;

/**
 * Top level route manager class
 * 
 */

class FGRouteMgr : public SGSubsystem
{

private:
    SGRoute* _route;
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

    /**
     * Create a SGWayPoint from a string in the following format:
     *  - simple identifier
     *  - decimal-lon,decimal-lat
     *  - airport-id/runway-id
     *  - navaid/radial-deg/offset-nm
     */
    SGWayPoint* make_waypoint(const string& target);
    
    /**
     * Helper to keep various pieces of state in sync when the SGRoute is
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
    
    
    void loadPlainTextRoute(const SGPath& path);
    
// tied getters and setters
    const char* getDepartureICAO() const;
    const char* getDepartureName() const;
    void setDepartureICAO(const char* aIdent);
    
    const char* getDestinationICAO() const;
    const char* getDestinationName() const;
    void setDestinationICAO(const char* aIdent);
    
public:

    FGRouteMgr();
    ~FGRouteMgr();

    void init ();
    void postinit ();
    void bind ();
    void unbind ();
    void update (double dt);

    bool build ();

    void new_waypoint( const string& tgt_alt, int n = -1 );
    void add_waypoint( const SGWayPoint& wp, int n = -1 );
    SGWayPoint pop_waypoint( int i = 0 );

    SGWayPoint get_waypoint( int i ) const;
    int size() const;
        
    bool isRouteActive() const;
        
    int currentWaypoint() const;
       
    /**
     * Find a waypoint in the route, by position, and return its index, or
     * -1 if no matching waypoint was found in the route.
     */
    int findWaypoint(const SGGeod& aPos) const;
          
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
     *
     */
    void jumpToIndex(int index);
    
    /**
     * 
     */
    void setWaypointTargetAltitudeFt(unsigned int index, int altFt);
    
    void saveRoute();
    void loadRoute();
};


#endif // _ROUTE_MGR_HXX
