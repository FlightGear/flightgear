// route_mgr.hxx - manage a route (i.e. a collection of waypoints)
/*
 * SPDX-FileCopyrightText: (C) 2004 Curtis L. Olson http://www.flightgear.org/~curt
 * SPDX-License-Identifier: GPL-2.0-or-later
 */


#pragma once

#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>

#include <Navaids/FlightPlan.hxx>

// forward decls
class SGPath;
class PropertyWatcher;
class RoutePath;

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

    // Subsystem API.
    void bind() override;
    void init() override;
    void postinit() override;
    void unbind() override;
    void update(double dt) override;

    // Subsystem identification.
    static const char* staticSubsystemClassId() { return "route-manager"; }

    bool isRouteActive() const;

    int currentIndex() const;

    void setFlightPlan(const flightgear::FlightPlanRef& plan);
    flightgear::FlightPlanRef flightPlan() const;

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
     * deactivate the route if active
     */
    void deactivate();

    /**
     * Set the current waypoint to the specified index.
     */
    void jumpToIndex(int index);

    bool saveRoute(const SGPath& p);
    bool loadRoute(const SGPath& p);

    /**
        @brief Buiild a waypoint from a string description. Passed to the FlightPlan code to do
     the actual parsing, see that method for details of syntax.
     
        Insert position is used to indicate which existing route waypoint(s) to use, to select between
     ambiguous names.
     */
    flightgear::WayptRef waypointFromString(const std::string& target, int insertPosition);

private:
    bool commandDefineUserWaypoint(const SGPropertyNode * arg, SGPropertyNode * root);
    bool commandDeleteUserWaypoint(const SGPropertyNode * arg, SGPropertyNode * root);

    flightgear::FlightPlanRef _plan;

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
    SGPropertyNode_ptr _isRoute;

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

    std::unique_ptr<RoutePath> _routePath;

    /**
     * Helper to keep various pieces of state in sync when the route is
     * modified (waypoints added, inserted, removed). Notably, this fires the
     * 'edited' signal.
     */
    void waypointsChanged() override;
    void arrivalChanged() override;
    void departureChanged() override;

    void update_mirror();

    void currentWaypointChanged() override;
    
    // tied getters and setters
    std::string getDepartureICAO() const;
    std::string getDepartureName() const;
    void setDepartureICAO(const std::string& aIdent);

    std::string getDepartureRunway() const;
    void setDepartureRunway(const std::string& aIdent);

    std::string getSID() const;
    void setSID(const std::string& aIdent);

    std::string getDestinationICAO() const;
    std::string getDestinationName() const;
    void setDestinationICAO(const std::string& aIdent);

    std::string getDestinationRunway() const;
    void setDestinationRunway(const std::string& aIdent);

    std::string getApproach() const;
    void setApproach(const std::string& aIdent);

    std::string getSTAR() const;
    void setSTAR(const std::string& aIdent);

    double getDepartureFieldElevation() const;
    double getDestinationFieldElevation() const;

    int getCruiseAltitudeFt() const;
    void setCruiseAltitudeFt(int ft);

    int getCruiseFlightLevel() const;
    void setCruiseFlightLevel(int fl);

    int getCruiseSpeedKnots() const;
    void setCruiseSpeedKnots(int kts);

    double getCruiseSpeedMach() const;
    void setCruiseSpeedMach(double m);

    std::string getAlternate() const;
    std::string getAlternateName() const;
    void setAlternate(const std::string &icao);
};

