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
typedef std::list<SGSharedPtr<FGTrafficRecord>> TrafficVector;
typedef std::list<SGSharedPtr<FGTrafficRecord>>::iterator TrafficVectorIterator;

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
    int  requestedArrivalTime{0};
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
    void setRunwaySlot (int val) {
        requestedArrivalTime = val;
    };
    int getRunwaySlot () const {
        return requestedArrivalTime;
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
 * Represents the interaction of an AI Aircraft and ATC
 *************************************************************************************/
class FGTrafficRecord : public SGReferenced
{
private:
    int id;
    int waitsForId;
    int currentPos;
    int leg;
    int frequencyId;
    int state;
    bool allowTransmission;
    bool allowPushback;
    int priority;
    int  plannedArrivalTime{0};
    time_t timer;
    intVec intentions;
    FGATCInstruction instruction;
    SGGeod pos;
    double heading, speed, altitude, radius;
    std::string callsign;
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
    /**
     * Return the current ATC State of type @see ATCMessageState
    */
    int getState() const {
        return state;
    };
    /**
     * Set the current ATC State of type @see ATCMessageState
    */
    void setState(int s) {
        state = s;
    }
    FGATCInstruction getInstruction() const {
        return instruction;
    };
    bool hasInstruction() const {
        return instruction.hasInstruction();
    };
    void setPositionAndHeading(double lat, double lon, double hdg, double spd, double alt);
    bool checkPositionAndIntentions(FGTrafficRecord &other);
    int  crosses                   (FGGroundNetwork *, FGTrafficRecord &other);
    bool isOpposing                (FGGroundNetwork *, FGTrafficRecord &other, int node);

    bool isActive(int margin) const;
    bool isDead() const;
    void clearATCController() const;

    bool onRoute(FGGroundNetwork *, FGTrafficRecord &other);

    bool getSpeedAdjustment() const {
        return instruction.getChangeSpeed();
    };
    void setPlannedArrivalTime   (int val) {
        plannedArrivalTime    = val;
    };
    /**Arrival time planned by aircraft.*/
    int getPlannedArrivalTime () const {
        return plannedArrivalTime;
    };
    void setRunwaySlot( int val ) {
        if (plannedArrivalTime) {
            SG_LOG(SG_ATC, SG_BULK, callsign << "| Runwayslot " << (val-plannedArrivalTime));
        }
        instruction.setRunwaySlot(val);
    };
    /**Arrival time requested by ATC.*/
    int getRunwaySlot() {
        return instruction.getRunwaySlot();
    };
    SGGeod getPos() {
        return pos;
    }
    double getHeading  () const {
        return heading  ;
    };
    double getSpeed    () const {
        return speed    ;
    };
    double getFAltitude () const {
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

    void setCallsign(const std::string& clsgn) { callsign = clsgn; };
    const std::string& getCallsign() const {
        return callsign;
    };

    const std::string& getRunway() const {
        return runway;
    };

    void setAircraft(FGAIAircraft *ref);

    void updateState() {
        state++;
        allowTransmission=true;
    };

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
    const std::string rwy;
    int currentlyCleared;
    double distanceToFinal;
    TimeVector estimatedArrivalTimes;

    using AircraftRefVec = std::vector<SGSharedPtr<FGAIAircraft>>;
    AircraftRefVec departureQueue;

public:
    ActiveRunway(const std::string& r, int cc);

    const std::string& getRunwayName() const
    {
        return rwy;
    };
    /**Get id of cleared AI Aircraft*/
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
    void addToDepartureQueue(FGAIAircraft *ac);

    void setCleared(int number) {
        currentlyCleared = number;
    };
    time_t requestTimeSlot(time_t eta);
    //time_t requestTimeSlot(time_t eta, std::string wakeCategory);
    void slotHousekeeping(time_t newEta);
    int getdepartureQueueSize() {
        return departureQueue.size();
    };

    SGSharedPtr<FGAIAircraft> getFirstAircraftInDepartureQueue() const;

    SGSharedPtr<FGAIAircraft> getFirstOfStatus(int stat) const;

    void removeFromDepartureQueue(int id);

    void updateDepartureQueue();

    void printDepartureQueue();
};


#endif // _TRAFFIC_CONTROL_HXX
