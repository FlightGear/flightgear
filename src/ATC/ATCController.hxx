// Extracted from trafficcontrol.hxx - classes to manage AIModels based air traffic control
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

#ifndef ATC_CONTROLLER_HXX
#define ATC_CONTROLLER_HXX

#include <Airports/airports_fwd.hxx>

#include <osg/Geode>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osg/Shape>

#include <simgear/compiler.h>
#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/structure/SGReferenced.hxx>
#include <simgear/structure/SGSharedPtr.hxx>

#include <ATC/AirportGroundRadar.hxx>
#include <ATC/trafficcontrol.hxx>

namespace ATCMessageState
{
    enum Type
    {
        // 0 =  Normal; no action required
        NORMAL,
        // 1 = "Acknowledge "Hold position
        ACK_HOLD,
        // 2 = "Acknowledge "Resume taxi".
        ACK_RESUME_TAXI,
        // 3 = "Issue TaxiClearance"
        TAXI_CLEARED = 3,
        // 4 = Acknowledge Taxi Clearance"
        ACK_TAXI_CLEARED = 4,
        // 5 = Post acknowlegde taxiclearance: Start taxiing
        START_TAXI = 5,
        // 6 = Report runway
        REPORT_RUNWAY = 6,
        // 7 = Acknowledge report runway
        ACK_REPORT_RUNWAY = 7,
        // 8 = Switch tower frequency
        SWITCH_GROUND_TOWER = 8,
        // 9 = Acknowledge switch tower frequency
        ACK_SWITCH_GROUND_TOWER = 9,
        // 10 = Cleared for takeoff
        CLEARED_TAKEOFF,
        ACK_CLEARED_TAKEOFF,
        ANNOUNCE_ARRIVAL,
        ACK_ARRIVAL,
        HOLD,
        CLEARED_TO_LAND,
        ACK_CLEARED_TO_LAND,
        LANDING_TAXI
    };
}

/**
 * class FGATCController
 * NOTE: this class serves as an abstraction layer for all sorts of ATC controllers.
 *************************************************************************************/
class FGATCController
{
private:


protected:
    // guard variable to avoid modifying state during destruction
    bool _isDestroying = false;
    bool initialized;
    bool available;
    time_t lastTransmission;
    TrafficVector activeTraffic;

    double dt_count;
    osg::Group* group;
    FGAirportDynamics *parent = nullptr;
    /*Shared Groundradar. All controllers of an airport share it.*/
    SGSharedPtr<AirportGroundRadar> airportGroundRadar;


    std::string formatATCFrequency3_2(int );
    std::string genTransponderCode(const std::string& fltRules);
    bool isUserAircraft(FGAIAircraft*);
    void clearTrafficControllers();
    TrafficVectorIterator searchActiveTraffic(int id);
    void eraseDeadTraffic();
    /**Returns the frequency to be used. */
    virtual int getFrequency() = 0;
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
        MSG_CLEARED_FOR_TAKEOFF,
        MSG_ACKNOWLEDGE_CLEARED_FOR_TAKEOFF,
        MSG_SWITCH_TOWER_FREQUENCY,
        MSG_ACKNOWLEDGE_SWITCH_TOWER_FREQUENCY,
        MSG_ARRIVAL,
        MSG_ACKNOWLEDGE_ARRIVAL,
        MSG_HOLD,
        MSG_ACKNOWLEDGE_HOLD,
        MSG_CLEARED_TO_LAND,
        MSG_ACKNOWLEDGE_CLEARED_TO_LAND
    } AtcMsgId;

    typedef enum {
        ATC_AIR_TO_GROUND,
        ATC_GROUND_TO_AIR
    } AtcMsgDir;

    FGATCController();
    virtual ~FGATCController();
    void init();
    void setAirportGroundRadar(SGSharedPtr<AirportGroundRadar> groundRadar);

    virtual void announcePosition(int id, FGAIFlightPlan *intendedRoute, int currentRoute,
                                  double lat, double lon,
                                  double hdg, double spd, double alt, double radius, int leg,
                                  FGAIAircraft *aircraft) = 0;
    virtual void             updateAircraftInformation(int id, SGGeod geod,
            double heading, double speed, double alt, double dt) = 0;
    bool checkTransmissionState(int minState, int MaxState, TrafficVectorIterator i, time_t now, AtcMsgId msgId,
                                AtcMsgDir msgDir);

    virtual void signOff(int id);
    bool hasInstruction(int id);
    FGATCInstruction getInstruction(int id);

    bool hasActiveTraffic() {
        return ! activeTraffic.empty();
    };
    TrafficVector &getActiveTraffic() {
        return activeTraffic;
    };

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

#endif