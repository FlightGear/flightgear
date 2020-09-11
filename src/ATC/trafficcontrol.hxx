// trafficcontrol.hxx - classes to manage AIModels based air traffic control
// Written by Durk Talsma, started September 2006.
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


#ifndef _TRAFFIC_CONTROL_HXX_
#define _TRAFFIC_CONTROL_HXX_

#include <Airports/airports_fwd.hxx>

#include <osg/Geode>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osg/Shape>

#include <simgear/compiler.h>
// There is probably a better include than sg_geodesy to get the SG_NM_TO_METER...
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/structure/SGReferenced.hxx>
#include <simgear/structure/SGSharedPtr.hxx>

class FGAIAircraft;
typedef std::vector<FGAIAircraft*> AircraftVec;
typedef std::vector<FGAIAircraft*>::iterator AircraftVecIterator;

class FGAIFlightPlan;
typedef std::vector<FGAIFlightPlan*>           FlightPlanVec;
typedef std::vector<FGAIFlightPlan*>::iterator FlightPlanVecIterator;
typedef std::map<std::string, FlightPlanVec>   FlightPlanVecMap;

class FGTrafficRecord;
typedef std::list<FGTrafficRecord> TrafficVector;
typedef std::list<FGTrafficRecord>::iterator TrafficVectorIterator;

class ActiveRunway;
typedef std::vector<ActiveRunway> ActiveRunwayVec;
typedef std::vector<ActiveRunway>::iterator ActiveRunwayVecIterator;

typedef std::vector<int> intVec;
typedef std::vector<int>::iterator intVecIterator;

/**************************************************************************************
 * class FGATCInstruction
 * like class FGATC Controller, this class definition should go into its own file
 * and or directory... For now, just testing this stuff out though...
 *************************************************************************************/
class FGATCInstruction
{
private:
    bool holdPattern;
    bool holdPosition;
    bool changeSpeed;
    bool changeHeading;
    bool changeAltitude;
    bool resolveCircularWait;

    double speed;
    double heading;
    double alt;
public:
    FGATCInstruction();
	
    bool hasInstruction   () const;
    bool getHoldPattern   () const {
        return holdPattern;
    };
    bool getHoldPosition  () const {
        return holdPosition;
    };
    bool getChangeSpeed   () const {
        return changeSpeed;
    };
    bool getChangeHeading () const {
        return changeHeading;
    };
    bool getChangeAltitude() const {
        return changeAltitude;
    };
    bool getCheckForCircularWait() const {
        return resolveCircularWait;
    };
	
    double getSpeed       () const {
        return speed;
    };
    double getHeading     () const {
        return heading;
    };
    double getAlt         () const {
        return alt;
    };

    void setHoldPattern   (bool val) {
        holdPattern    = val;
    };
    void setHoldPosition  (bool val) {
        holdPosition   = val;
    };
    void setChangeSpeed   (bool val) {
        changeSpeed    = val;
    };
    void setChangeHeading (bool val) {
        changeHeading  = val;
    };
    void setChangeAltitude(bool val) {
        changeAltitude = val;
    };
    void setResolveCircularWait (bool val) {
        resolveCircularWait = val;
    };
	
    void setSpeed       (double val) {
        speed   = val;
    };
    void setHeading     (double val) {
        heading = val;
    };
    void setAlt         (double val) {
        alt     = val;
    };
};





/**************************************************************************************
 * class FGTrafficRecord
 *************************************************************************************/
class FGTrafficRecord
{
private:
    int id, waitsForId;
    int currentPos;
    int leg;
    int frequencyId;
    int state;
    bool allowTransmission;
    bool allowPushback;
    int priority;
    time_t timer;
    intVec intentions;
    FGATCInstruction instruction;
    double latitude, longitude, heading, speed, altitude, radius;
    std::string runway;
    SGSharedPtr<FGAIAircraft> aircraft;


public:
    FGTrafficRecord();
    virtual ~FGTrafficRecord();

    void setId(int val)  {
        id = val;
    };
    void setRadius(double rad) {
        radius = rad;
    };
    void setPositionAndIntentions(int pos, FGAIFlightPlan *route);
    void setRunway(const std::string& rwy) {
        runway = rwy;
    };
    void setLeg(int lg) {
        leg = lg;
    };
    int getId() const {
        return id;
    };
    int getState() const {
        return state;
    };
    void setState(int s) {
        state = s;
    }
    FGATCInstruction getInstruction() const {
        return instruction;
    };
    bool hasInstruction() {
        return instruction.hasInstruction();
    };
    void setPositionAndHeading(double lat, double lon, double hdg, double spd, double alt);
    bool checkPositionAndIntentions(FGTrafficRecord &other);
    int  crosses                   (FGGroundNetwork *, FGTrafficRecord &other);
    bool isOpposing                (FGGroundNetwork *, FGTrafficRecord &other, int node);
    
    bool isActive(int margin) const;

    bool onRoute(FGGroundNetwork *, FGTrafficRecord &other);

    bool getSpeedAdjustment() const {
        return instruction.getChangeSpeed();
    };

    double getLatitude () const {
        return latitude ;
    };
    double getLongitude() const {
        return longitude;
    };
    double getHeading  () const {
        return heading  ;
    };
    double getSpeed    () const {
        return speed    ;
    };
    double getAltitude () const {
        return altitude ;
    };
    double getRadius   () const {
        return radius   ;
    };

    int getWaitsForId  () const {
        return waitsForId;
    };

    void setSpeedAdjustment(double spd);
    void setHeadingAdjustment(double heading);
    void clearSpeedAdjustment  () {
        instruction.setChangeSpeed  (false);
    };
    void clearHeadingAdjustment() {
        instruction.setChangeHeading(false);
    };

    bool hasHeadingAdjustment() const {
        return instruction.getChangeHeading();
    };
    bool hasHoldPosition() const {
        return instruction.getHoldPosition();
    };
    void setHoldPosition (bool inst) {
        instruction.setHoldPosition(inst);
    };

    void setWaitsForId(int id) {
        waitsForId = id;
    };

    void setResolveCircularWait()   {
        instruction.setResolveCircularWait(true);
    };
    void clearResolveCircularWait() {
        instruction.setResolveCircularWait(false);
    };

    const std::string& getRunway() const {
        return runway;
    };
    //void setCallSign(string clsgn) { callsign = clsgn; };
    void setAircraft(FGAIAircraft *ref);

    void updateState() {
        state++;
        allowTransmission=true;
    };
    //string getCallSign() { return callsign; };
    FGAIAircraft *getAircraft() const;

    int getTime() const {
        return timer;
    };
    int getLeg() const {
        return leg;
    };
    void setTime(time_t time) {
        timer = time;
    };

    bool pushBackAllowed() const;
    bool allowTransmissions() const {
        return allowTransmission;
    };
    void allowPushBack() { allowPushback =true;};
    void denyPushBack () { allowPushback = false;};
    void suppressRepeatedTransmissions () {
        allowTransmission=false;
    };
    void allowRepeatedTransmissions () {
        allowTransmission=true;
    };
    void nextFrequency() {
        frequencyId++;
    };
    int  getNextFrequency() const {
        return frequencyId;
    };
    intVec& getIntentions() {
        return intentions;
    };
    int getCurrentPosition() const {
        return currentPos;
    };
    void setPriority(int p) { priority = p; };
    int getPriority() const { return priority; };
};

/***********************************************************************
 * Active runway, a utility class to keep track of which aircraft has
 * clearance for a given runway.
 **********************************************************************/
class ActiveRunway
{
private:
    std::string rwy;
    int currentlyCleared;
    double distanceToFinal;
    TimeVector estimatedArrivalTimes;
    AircraftVec departureQueue;

public:
    ActiveRunway(const std::string& r, int cc) {
        rwy = r;
        currentlyCleared = cc;
        distanceToFinal = 6.0 * SG_NM_TO_METER;
    };

    const std::string& getRunwayName() const
    {
        return rwy;
    };
    int getCleared() const
    {
        return currentlyCleared;
    };
    double getApproachDistance() const
    {
        return distanceToFinal;
    };
    //time_t getEstApproachTime() { return estimatedArrival; };

    //void setEstApproachTime(time_t time) { estimatedArrival = time; };
    void addTodepartureQueue(FGAIAircraft *ac) {
        departureQueue.push_back(ac);
    };
    void setCleared(int number) {
        currentlyCleared = number;
    };
    time_t requestTimeSlot(time_t eta);
    //time_t requestTimeSlot(time_t eta, std::string wakeCategory);
    void slotHousekeeping(time_t newEta);
    int getdepartureQueueSize() {
        return departureQueue.size();
    };
    FGAIAircraft* getFirstAircraftIndepartureQueue() {
        return departureQueue.size() ? *(departureQueue.begin()) : NULL;
    };
    FGAIAircraft* getFirstOfStatus(int stat);
    void updatedepartureQueue() {
        departureQueue.erase(departureQueue.begin());
    }
    void printdepartureQueue();
};

/**
 * class FGATCController
 * NOTE: this class serves as an abstraction layer for all sorts of ATC controllers.
 *************************************************************************************/
class FGATCController
{
private:


protected:
    bool initialized;
    bool available;
    time_t lastTransmission;

    double dt_count;
    osg::Group* group;

    std::string formatATCFrequency3_2(int );
    std::string genTransponderCode(const std::string& fltRules);
    bool isUserAircraft(FGAIAircraft*);

    void eraseDeadTraffic(TrafficVector& vec);
public:
    typedef enum {
        MSG_ANNOUNCE_ENGINE_START,
        MSG_REQUEST_ENGINE_START,
        MSG_PERMIT_ENGINE_START,
        MSG_DENY_ENGINE_START,
        MSG_ACKNOWLEDGE_ENGINE_START,
        MSG_REQUEST_PUSHBACK_CLEARANCE,
        MSG_PERMIT_PUSHBACK_CLEARANCE,
        MSG_HOLD_PUSHBACK_CLEARANCE,
        MSG_ACKNOWLEDGE_SWITCH_GROUND_FREQUENCY,
        MSG_INITIATE_CONTACT,
        MSG_ACKNOWLEDGE_INITIATE_CONTACT,
        MSG_REQUEST_TAXI_CLEARANCE,
        MSG_ISSUE_TAXI_CLEARANCE,
        MSG_ACKNOWLEDGE_TAXI_CLEARANCE,
        MSG_HOLD_POSITION,
        MSG_ACKNOWLEDGE_HOLD_POSITION,
        MSG_RESUME_TAXI,
        MSG_ACKNOWLEDGE_RESUME_TAXI,
        MSG_REPORT_RUNWAY_HOLD_SHORT,
        MSG_ACKNOWLEDGE_REPORT_RUNWAY_HOLD_SHORT,
        MSG_SWITCH_TOWER_FREQUENCY,
        MSG_ACKNOWLEDGE_SWITCH_TOWER_FREQUENCY
    } AtcMsgId;

    typedef enum {
        ATC_AIR_TO_GROUND,
        ATC_GROUND_TO_AIR
    } AtcMsgDir;
    FGATCController();
    virtual ~FGATCController();
    void init();

    virtual void announcePosition(int id, FGAIFlightPlan *intendedRoute, int currentRoute,
                                  double lat, double lon,
                                  double hdg, double spd, double alt, double radius, int leg,
                                  FGAIAircraft *aircraft) = 0;
    virtual void             signOff(int id) = 0;
    virtual void             updateAircraftInformation(int id, double lat, double lon,
            double heading, double speed, double alt, double dt) = 0;
    virtual bool             hasInstruction(int id) = 0;
    virtual FGATCInstruction getInstruction(int id) = 0;

    double getDt() {
        return dt_count;
    };
    void   setDt(double dt) {
        dt_count = dt;
    };
    void transmit(FGTrafficRecord *rec, FGAirportDynamics *parent, AtcMsgId msgId, AtcMsgDir msgDir, bool audible);
    std::string getGateName(FGAIAircraft *aircraft);
    virtual void render(bool) = 0;
    virtual std::string getName()  = 0;

    virtual void update(double) = 0;


private:

    AtcMsgDir lastTransmissionDirection;
};

/******************************************************************************
 * class FGTowerControl
 *****************************************************************************/
class FGTowerController : public FGATCController
{
private:
    TrafficVector activeTraffic;
    ActiveRunwayVec activeRunways;
    FGAirportDynamics *parent;

public:
    FGTowerController(FGAirportDynamics *parent);
    virtual ~FGTowerController();

    virtual void announcePosition(int id, FGAIFlightPlan *intendedRoute, int currentRoute,
                                  double lat, double lon,
                                  double hdg, double spd, double alt, double radius, int leg,
                                  FGAIAircraft *aircraft);
    virtual void             signOff(int id);
    virtual void             updateAircraftInformation(int id, double lat, double lon,
            double heading, double speed, double alt, double dt);
    virtual bool             hasInstruction(int id);
    virtual FGATCInstruction getInstruction(int id);

    virtual void render(bool);
    virtual std::string getName();
    virtual void update(double dt);
    bool hasActiveTraffic() {
        return ! activeTraffic.empty();
    };
    TrafficVector &getActiveTraffic() {
        return activeTraffic;
    };
};

/******************************************************************************
 * class FGStartupController
 * handle
 *****************************************************************************/

class FGStartupController : public FGATCController
{
private:
    TrafficVector activeTraffic;
    //ActiveRunwayVec activeRunways;
    FGAirportDynamics *parent;

public:
    FGStartupController(FGAirportDynamics *parent);
    virtual ~FGStartupController();

    virtual void announcePosition(int id, FGAIFlightPlan *intendedRoute, int currentRoute,
                                  double lat, double lon,
                                  double hdg, double spd, double alt, double radius, int leg,
                                  FGAIAircraft *aircraft);
    virtual void             signOff(int id);
    virtual void             updateAircraftInformation(int id, double lat, double lon,
            double heading, double speed, double alt, double dt);
    virtual bool             hasInstruction(int id);
    virtual FGATCInstruction getInstruction(int id);

    virtual void render(bool);
    virtual std::string getName();
    virtual void update(double dt);

    bool hasActiveTraffic() {
        return ! activeTraffic.empty();
    };
    TrafficVector &getActiveTraffic() {
        return activeTraffic;
    };

    // Hpoefully, we can move this function to the base class, but I need to verify what is needed for the other controllers before doing so.
    bool checkTransmissionState(int st, time_t now, time_t startTime, TrafficVectorIterator i, AtcMsgId msgId,
                                AtcMsgDir msgDir);

};

/******************************************************************************
 * class FGTowerControl
 *****************************************************************************/
class FGApproachController : public FGATCController
{
private:
    TrafficVector activeTraffic;
    ActiveRunwayVec activeRunways;
    FGAirportDynamics *parent;

public:
    FGApproachController(FGAirportDynamics * parent);
    virtual ~FGApproachController();
    
    virtual void announcePosition(int id, FGAIFlightPlan *intendedRoute, int currentRoute,
                                  double lat, double lon,
                                  double hdg, double spd, double alt, double radius, int leg,
                                  FGAIAircraft *aircraft);
    virtual void             signOff(int id);
    virtual void             updateAircraftInformation(int id, double lat, double lon,
            double heading, double speed, double alt, double dt);
    virtual bool             hasInstruction(int id);
    virtual FGATCInstruction getInstruction(int id);

    virtual void render(bool);
    virtual std::string getName();
    virtual void update(double dt);

    ActiveRunway* getRunway(const std::string& name);

    bool hasActiveTraffic() {
        return ! activeTraffic.empty();
    };
    TrafficVector &getActiveTraffic() {
        return activeTraffic;
    };
};


#endif // _TRAFFIC_CONTROL_HXX
