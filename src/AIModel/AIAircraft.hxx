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

#include <string>
SG_USING_STD(string);


class FGAIAircraft : public FGAIBase {

private:
    typedef struct {
        double accel;
        double decel;
        double climb_rate;
        double descent_rate;
        double takeoff_speed;
        double climb_speed;
        double cruise_speed;
        double descent_speed;
        double land_speed;
    } PERF_STRUCT;

public:
    enum aircraft_e {
        LIGHT = 0,
        WW2_FIGHTER,
        JET_TRANSPORT,
        JET_FIGHTER,
        TANKER,
        UFO
    };
    static const PERF_STRUCT settings[];

    FGAIAircraft(FGAISchedule *ref=0);
    ~FGAIAircraft();

    virtual void readFromScenario(SGPropertyNode* scFileNode);

    virtual bool init(bool search_in_AI_path=false);
    virtual void bind();
    virtual void unbind();
    virtual void update(double dt);

    void setPerformance(const std::string& perfString);
    void SetPerformance(const PERF_STRUCT *ps);
    void setFlightPlan(const std::string& fp, bool repat = false);
    void SetFlightPlan(FGAIFlightPlan *f);
    void initializeFlightPlan();
    FGAIFlightPlan* GetFlightPlan() const { return fp; };
    void AccelTo(double speed);
    void PitchTo(double angle);
    void RollTo(double angle);
    void YawTo(double angle);
    void ClimbTo(double altitude);
    void TurnTo(double heading);
    void ProcessFlightPlan( double dt, time_t now );
    void setCallSign(const string& );
    void setTACANChannelID(const string& );

    void getGroundElev(double dt);
    void doGroundAltitude();
    void loadNextLeg  ();

    void setAcType(const string& ac) { acType = ac; };
    void setCompany(const string& comp) { company = comp;};

    void announcePositionToController();
    void processATC(FGATCInstruction instruction);

    inline void SetTanker(bool setting) { isTanker = setting; };
    virtual const char* getTypeString(void) const { return "aircraft"; }

private:
    FGAISchedule *trafficRef;
    FGATCController *controller, *prevController; 

    bool hdg_lock;
    bool alt_lock;
    double dt_count;
    double dt_elev_count;
    double headingChangeRate;
    double groundTargetSpeed;
    double groundOffset;
    double dt;

    const PERF_STRUCT *performance;
    bool use_perf_vs;
    SGPropertyNode_ptr refuel_node;
    bool isTanker;

    void Run(double dt);
    double sign(double x);

    string acType;
    string company;

    int spinCounter;
    double prevSpeed;
    double prev_dist_to_go;

  bool holdPos;

    bool _getGearDown() const;
    bool reachedWaypoint;
    string callsign;             // The callsign of this tanker.
    string TACAN_channel_id;     // The TACAN channel of this tanker
    bool contact;                // set if this tanker is within fuelling range
};


#endif  // _FG_AIAircraft_HXX
