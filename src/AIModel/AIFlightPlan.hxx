/*
 * SPDX-FileName: AIFlightPlan.hxx
 * SPDX-FileComment: class for loading and storing AI flight plans
 * SPDX-FileCopyrightText: Written by David Culp, started May 2004 - davidculp2@comcast.net
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <string>
#include <vector>

#include <Airports/dynamics.hxx>
#include <Navaids/positioned.hxx>
#include <simgear/compiler.h>
#include <simgear/math/SGMath.hxx>
#include <simgear/structure/SGSharedPtr.hxx>
#include <simgear/structure/exception.hxx>

// forward decls
class SGPath;
class ParkingAssignment;

class FGAIWaypoint
{
private:
    std::string name = "unnamed";
    SGGeod pos;
    double speed = 0.0;
    double crossat = 0.0;
    bool finished = false;
    bool gear_down = true;
    double flaps = 0.0;
    double spoilers = 0.0;
    double speedbrakes = 0.0;
    bool on_ground;
    int routeIndex;           // For AI/ATC purposes;
    double time_sec;
    double trackLength = 0.0; // distance from previous FGAIWaypoint (for AI purposes);
    std::string time;

    bool beacon_light = false;
    bool landing_light = false;
    bool nav_light = false;
    bool strobe_light = false;
    bool taxi_light = false;
    bool cabin_light = false;

public:
    FGAIWaypoint();
    virtual ~FGAIWaypoint(){};

    void setName(const std::string& nam) { name = nam; };
    void setLatitude(double lat);
    void setLongitude(double lon);
    void setAltitude(double alt);
    void setPos(const SGGeod& aPos) { pos = aPos; }
    void setSpeed(double spd) { speed = spd; };
    void setCrossat(double val) { crossat = val; };
    void setFinished(bool fin) { finished = fin; };
    void setGear_down(bool grd) { gear_down = grd; };
    void setFlaps(double val) { flaps = val; };
    void setSpoilers(double val) { spoilers = val; };
    void setSpeedBrakes(double val) { speedbrakes = val; };
    void setOn_ground(bool grn) { on_ground = grn; };
    void setRouteIndex(int rte) { routeIndex = rte; };
    void setTime_sec(double ts) { time_sec = ts; };
    void setTrackLength(double tl) { trackLength = tl; };
    void setTime(const std::string& tme) { time = tme; };
    void setLights(bool beacon, bool cabin, bool ldg, bool nav, bool strobe, bool taxi)
    {
        beacon_light = beacon;
        cabin_light = cabin;
        landing_light = ldg;
        nav_light = nav;
        strobe_light = strobe;
        taxi_light = taxi;
    };

    //                                      beacon cabin  ldg    nav    strobe taxi
    void setPowerDownLights() {   setLights(false, true,  false, false, false, false); };
    void setGroundLights() {      setLights(true,  true,  false, true,  false, true); };
    void setCruiseLights() {      setLights(true,  true,  false, true,  true,  false); };
    void setTakeOffLights() {     setLights(true,  false, true,  true,  true,  false); };
    void setApproachLights() {    setLights(true,  false, true,  true,  true,  false); };
    void setBeaconLight(bool beacon) { beacon_light = beacon; };
    void setCabinLight(bool cabin) { cabin_light = cabin; };
    void setLandingLight(bool ldg) { landing_light = ldg; };
    void setNavLight(bool nav) { nav_light = nav; };
    void setStrobeLight(bool strobe) { strobe_light = strobe; };
    void setTaxiLight(bool taxi) { taxi_light = taxi; };

    bool contains(const std::string& name);

    const std::string& getName() { return name; };
    const SGGeod& getPos() { return pos; };
    double getLatitude() const;
    double getLongitude() const;
    double getAltitude() const;
    double getSpeed() { return speed; };

    double getCrossat() { return crossat; };
    bool getGear_down() { return gear_down; };
    double getFlaps() { return flaps; };
    double getSpoilers() { return spoilers; };
    double getSpeedBrakes() { return speedbrakes; };
    bool getOn_ground() { return on_ground; };
    bool getInAir() { return !on_ground; };
    int getRouteIndex() { return routeIndex; };
    bool isFinished() { return finished; };
    double getTime_sec() { return time_sec; };
    double getTrackLength() { return trackLength; };
    const std::string& getTime() { return time; };
    bool getBeaconLight() { return beacon_light; };
    bool getCabinLight() { return cabin_light; };
    bool getLandingLight() { return landing_light; };
    bool getNavLight() { return nav_light; };
    bool getStrobeLight() { return strobe_light; };
    bool getTaxiLight() { return taxi_light; };
};


class FGAIFlightPlan
{
public:
    FGAIFlightPlan();
    explicit FGAIFlightPlan(const std::string& filename);
    FGAIFlightPlan(FGAIAircraft*,
                   const std::string& p,
                   double course,
                   time_t start,
                   time_t remainingTime,
                   FGAirport* dep,
                   FGAirport* arr,
                   bool firstLeg,
                   double radius,
                   double alt,
                   double lat,
                   double lon,
                   double speed,
                   const std::string& fltType,
                   const std::string& acType,
                   const std::string& airline);
    virtual ~FGAIFlightPlan();

    /**
        @brief create a nearly empty FlightPlan for the user aircraft, based
     on the current position and route-manager data.
     */
    static FGAIFlightPlan* createDummyUserPlan();
    /**

     @brief read a flight-plan from a file.
     All current contents of the flight-plan are replaced, and the current waypoint is reset to the beginning
     */
    bool readFlightplan(const SGPath& file);

    bool readFlightplan(std::istream& stream, const sg_location& loc = sg_location{});

    FGAIWaypoint* getPreviousWaypoint(void) const;
    FGAIWaypoint* getCurrentWaypoint(void) const;
    FGAIWaypoint* getNextWaypoint(void) const;
    int getNextTurnAngle(void) const;
    void IncrementWaypoint(bool erase);
    void DecrementWaypoint();

    double getDistanceToGo(double lat, double lon, FGAIWaypoint* wp) const;
    int getLeg() const { return leg; };

    /** Set lead_distance_ft*/
    void setLeadDistance(double speed, double bearing, FGAIWaypoint* current, FGAIWaypoint* next);
    /** Set lead_distance_ft*/
    void setLeadDistance(double distance_ft);
    double getLeadDistance(void) const { return lead_distance_ft; }
    double getBearing(FGAIWaypoint* previous, FGAIWaypoint* next) const;
    double getBearing(const SGGeod& aPos, FGAIWaypoint* next) const;

    double checkTrackLength(const std::string& wptName) const;
    time_t getStartTime() const { return start_time; }
    time_t getArrivalTime() const { return arrivalTime; }

    bool create(FGAIAircraft*, FGAirport* dep, FGAirport* arr, int leg, double alt, double speed, double lat, double lon,
                bool firstLeg, double radius, const std::string& fltType, const std::string& aircraftType, const std::string& airline, double distance);
    bool createPushBack(FGAIAircraft*, bool, FGAirport*, double radius, const std::string&, const std::string&, const std::string&);
    bool createTakeOff(FGAIAircraft*, bool, FGAirport*, const SGGeod& pos, double speed, const std::string& flightType);

    void setLeg(int val)
    {
        SG_LOG(SG_AI, SG_BULK, "Set Leg " << leg);
        leg = val;
    }
    void setTime(time_t st) { start_time = st; }

    double getLeadInAngle() const { return leadInAngle; }
    const std::string& getRunway() const;

    void setRepeat(bool r) { repeat = r; }
    bool getRepeat(void) const { return repeat; }
    void restart(void);
    int getNrOfWayPoints() { return waypoints.size(); }

    int getRouteIndex(int i) const; // returns the AI related index of this current routes.

    const std::string& getRunway() { return activeRunway; }
    bool isActive(time_t time) { return time >= this->getStartTime(); }

    void incrementLeg()
    {
        SG_LOG(SG_AI, SG_BULK, "Increment Leg " << leg);
        leg++;
    };

    void setRunway(const std::string& rwy) { activeRunway = rwy; };
    const char* getRunwayClassFromTrafficType(const std::string& fltType);

    void addWaypoint(FGAIWaypoint* wpt);

    void setName(const std::string& n) { name = n; };
    const std::string& getName() { return name; };

    void setSID(FGAIFlightPlan* fp) { sid = fp; };
    FGAIFlightPlan* getSID() { return sid; };
    FGAIWaypoint* getWayPoint(int i) { return waypoints[i]; };
    FGAIWaypoint* getLastWaypoint() const;

    void shortenToFirst(unsigned int number, const std::string& name);

    void setGate(const ParkingAssignment& pka);
    FGParking* getParkingGate() const;

    FGAirportRef departureAirport() const;
    FGAirportRef arrivalAirport() const;

    bool empty() const;

private:
    FGAIFlightPlan* sid;
    typedef std::vector<FGAIWaypoint*> wpt_vector_type;
    typedef wpt_vector_type::const_iterator wpt_vector_iterator;


    wpt_vector_type waypoints;
    wpt_vector_iterator wpt_iterator;

    bool repeat;
    double distance_to_go = 0;
    //FIXME ft
    double lead_distance_ft = 0;
    double leadInAngle = 0;
    double nextTurnAngle = 0;
    time_t start_time;
    time_t arrivalTime; // For AI/ATC purposes.
    int leg = 0;
    ParkingAssignment gate;
    FGTaxiNodeRef lastNodeVisited;
    std::string activeRunway;
    std::string name;
    bool isValid;
    FGAirportRef departure, arrival;

    void createPushBackFallBack(FGAIAircraft*, bool, FGAirport*, double radius, const std::string&, const std::string&, const std::string&);
    bool createClimb(FGAIAircraft*, bool, FGAirport*, FGAirport* arrival, double, double, const std::string&);
    bool createCruise(FGAIAircraft*, bool, FGAirport*, FGAirport*, const SGGeod& current, double, double, const std::string&);
    bool createDescent(FGAIAircraft*, FGAirport*, const SGGeod& current, double speed, double alt, const std::string&, double distance);
    bool createHold(FGAIAircraft*, FGAirport*, const SGGeod& current, double speed, double alt, const std::string&, double distance);
    bool createLanding(FGAIAircraft*, FGAirport*, const std::string&);
    bool createParking(FGAIAircraft*, FGAirport*, double radius);
    void deleteWaypoints();
    void resetWaypoints();
    void eraseLastWaypoint();
    void pushBackWaypoint(FGAIWaypoint* wpt);

    /**Create an arc flightplan around a center from startAngle to endAngle.*/
    void createArc(FGAIAircraft* ac, const SGGeod& center, int startAngle, int endAngle, int increment, int radius, double aElev, double altDiff, double aSpeed, const char* pattern);
    /**Create a line flightplan from start with dist and altitude diff*/
    void createLine(FGAIAircraft* ac, const SGGeod& startPoint, double azimuth, double dist, double aElev, double dAlt, double vDescent, const char* pattern);
    bool createLandingTaxi(FGAIAircraft*, FGAirport* apt, double radius, const std::string& fltType, const std::string& acType, const std::string& airline);
    void createDefaultLandingTaxi(FGAIAircraft*, FGAirport* aAirport);
    void createDefaultTakeoffTaxi(FGAIAircraft*, FGAirport* aAirport, FGRunway* aRunway);
    bool createTakeoffTaxi(FGAIAircraft*, bool firstFlight, FGAirport* apt, double radius, const std::string& fltType, const std::string& acType, const std::string& airline);

    double getTurnRadius(double, bool);

    FGAIWaypoint* createOnGround(FGAIAircraft*, const std::string& aName, const SGGeod& aPos, double aElev, double aSpeed);
    FGAIWaypoint* createOnRunway(FGAIAircraft*, const std::string& aName, const SGGeod& aPos, double aElev, double aSpeed);
    FGAIWaypoint* createInAir(FGAIAircraft*, const std::string& aName, const SGGeod& aPos, double aElev, double aSpeed);
    FGAIWaypoint* cloneWithPos(FGAIAircraft*, FGAIWaypoint* aWpt, const std::string& aName, const SGGeod& aPos);
    FGAIWaypoint* clone(FGAIWaypoint* aWpt);


    //void createCruiseFallback(bool, FGAirport*, FGAirport*, double, double, double, double);
    void evaluateRoutePart(double deplat, double deplon, double arrlat, double arrlon);

    /**
   * look for and parse an PropertyList flight-plan file - essentially
   * a flat list waypoint objects, encoded to properties
   */
    bool parseProperties(const std::string& filename);

    /**
   * Creates the waypoints for this Flightplan.
   */

    void createWaypoints(FGAIAircraft* ac,
                         double course,
                         time_t start,
                         time_t remainingTime,
                         FGAirport* dep,
                         FGAirport* arr,
                         bool firstLeg,
                         double radius,
                         double alt,
                         double lat,
                         double lon,
                         double speed,
                         const std::string& fltType,
                         const std::string& acType,
                         const std::string& airline);

public:
    wpt_vector_iterator getFirstWayPoint() { return waypoints.begin(); };
    wpt_vector_iterator getLastWayPoint() { return waypoints.end(); };
    bool isValidPlan() { return isValid; };
};
