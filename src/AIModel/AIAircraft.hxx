// FGAIAircraft - AIBase derived class creates an AI aircraft
//
// Written by David Culp, started October 2003.
//
// Copyright (C) 2003  David P. Culp - davidculp2@comcast.net
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

#ifndef _FG_AIAircraft_HXX
#define _FG_AIAircraft_HXX

#include "AIManager.hxx"
#include "AIBase.hxx"

#include <Traffic/SchedFlight.hxx>
#include <Traffic/Schedule.hxx>
#include <ATC/trafficcontrol.hxx>

#include <string>

class PerformanceData;

class FGAIAircraft : public FGAIBase {

public:
    FGAIAircraft(FGAISchedule *ref=0);
    ~FGAIAircraft();

    virtual void readFromScenario(SGPropertyNode* scFileNode);

    // virtual bool init(bool search_in_AI_path=false);
    virtual void bind();
    virtual void update(double dt);

    void setPerformance(const std::string& perfString);
    void setPerformance(PerformanceData *ps);

    void setFlightPlan(const std::string& fp, bool repat = false);
    void SetFlightPlan(FGAIFlightPlan *f);
    void initializeFlightPlan();
    FGAIFlightPlan* GetFlightPlan() const { return fp; };
    void ProcessFlightPlan( double dt, time_t now );
    time_t checkForArrivalTime(string wptName);
    
    void AccelTo(double speed);
    void PitchTo(double angle);
    void RollTo(double angle);
    void YawTo(double angle);
    void ClimbTo(double altitude);
    void TurnTo(double heading);
    
    void getGroundElev(double dt); //TODO these 3 really need to be public?
    void doGroundAltitude();
    bool loadNextLeg  (double dist=0);
    void resetPositionFromFlightPlan();
    double getBearing(double crse);

    void setAcType(const std::string& ac) { acType = ac; };
    void setCompany(const std::string& comp) { company = comp;};

    void announcePositionToController(); //TODO have to be public?
    void processATC(FGATCInstruction instruction);
    void setTaxiClearanceRequest(bool arg) { needsTaxiClearance = arg; };
    bool getTaxiClearanceRequest() { return needsTaxiClearance; };
    FGAISchedule * getTrafficRef() { return trafficRef; };
    void setTrafficRef(FGAISchedule *ref) { trafficRef = ref; };
    void resetTakeOffStatus() { takeOffStatus = 0;};
    void setTakeOffStatus(int status) { takeOffStatus = status; };
    void scheduleForATCTowerDepartureControl(int state);

    //inline bool isScheduledForTakeoff() { return scheduledForTakeoff; };

    virtual const char* getTypeString(void) const { return "aircraft"; }

    std::string GetTransponderCode() { return transponderCode; };
    void SetTransponderCode(const std::string& tc) { transponderCode = tc;};

    // included as performance data needs them, who else?
    inline PerformanceData* getPerformance() { return _performance; };
    inline bool onGround() const { return no_roll; };
    inline double getSpeed() const { return speed; };
    inline double getRoll() const { return roll; };
    inline double getPitch() const { return pitch; };
    inline double getAltitude() const { return altitude_ft; };
    inline double getVerticalSpeed() const { return vs; };
    inline double altitudeAGL() const { return props->getFloatValue("position/altitude-agl-ft");};
    inline double airspeed() const { return props->getFloatValue("velocities/airspeed-kt");};
    std::string atGate();

    int getTakeOffStatus() { return takeOffStatus; };

    void checkTcas();
    double calcVerticalSpeed(double vert_ft, double dist_m, double speed, double error);

    FGATCController * getATCController() { return controller; };
    
protected:
    void Run(double dt);

private:
    FGAISchedule *trafficRef;
    FGATCController *controller, 
                    *prevController,
                    *towerController; // Only needed to make a pre-announcement

    bool hdg_lock;
    bool alt_lock;
    double dt_count;
    double dt_elev_count;
    double headingChangeRate;
    double headingError;
    double minBearing;
    double speedFraction;
    double groundTargetSpeed;
    double groundOffset;
    double dt;

    bool use_perf_vs;
    SGPropertyNode_ptr refuel_node;

    // helpers for Run
    //TODO sort out which ones are better protected virtuals to allow
    //subclasses to override specific behaviour
    bool fpExecutable(time_t now);
    void handleFirstWaypoint(void);
    bool leadPointReached(FGAIWaypoint* curr);
    bool handleAirportEndPoints(FGAIWaypoint* prev, time_t now);
    bool reachedEndOfCruise(double&);
    bool aiTrafficVisible(void);
    void controlHeading(FGAIWaypoint* curr);
    void controlSpeed(FGAIWaypoint* curr,
                      FGAIWaypoint* next);
    
    void updatePrimaryTargetValues(bool& flightplanActive, bool& aiOutOfSight);
    
    void updateSecondaryTargetValues();
    void updatePosition();
    void updateHeading();
    void updateBankAngleTarget();
    void updateVerticalSpeedTarget();
    void updatePitchAngleTarget();
    void updateActualState();
    void handleATCRequests();
    void checkVisibility();
    inline bool isStationary() { return ((fabs(speed)<=0.0001)&&(fabs(tgt_speed)<=0.0001));}
    inline bool needGroundElevation() { if (!isStationary()) _needsGroundElevation=true;return _needsGroundElevation;}
   

    double sign(double x);

    std::string acType;
    std::string company;
    std::string transponderCode;

    int spinCounter;
    double prevSpeed;
    double prev_dist_to_go;

    bool holdPos;

    bool _getGearDown() const;

    const char * _getTransponderCode() const;

    bool reachedWaypoint;
    bool needsTaxiClearance;
    bool _needsGroundElevation;
    int  takeOffStatus; // 1 = joined departure cue; 2 = Passed DepartureHold waypoint; handover control to tower; 0 = any other state. 
    time_t timeElapsed;

    PerformanceData* _performance; // the performance data for this aircraft
    
   void assertSpeed(double speed);
};


#endif  // _FG_AIAircraft_HXX
