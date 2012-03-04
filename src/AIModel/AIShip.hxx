// FGAIShip - AIBase derived class creates an AI ship
//
// Written by David Culp, started November 2003.
// with major amendments and additions by Vivian Meazza, 2004 - 2007 
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

#ifndef _FG_AISHIP_HXX
#define _FG_AISHIP_HXX

#include "AIBase.hxx"
#include "AIFlightPlan.hxx"
#include <simgear/scene/material/mat.hxx>

class FGAIManager;

class FGAIShip : public FGAIBase {

public:

    FGAIShip(object_type ot = otShip);
    virtual ~FGAIShip();

    virtual void readFromScenario(SGPropertyNode* scFileNode);

    virtual bool init(bool search_in_AI_path=false);
    virtual void bind();
    virtual void update(double dt);
    virtual void reinit();

    void setFlightPlan(FGAIFlightPlan* f);
    void setRudder(float r);
    void setRoll(double rl);
    void ProcessFlightPlan( double dt);
    void AccelTo(double speed);
    void PitchTo(double angle);
    void RollTo(double angle);
    void YawTo(double angle);
    void ClimbTo(double altitude);
    void TurnTo(double heading);
    void setCurrName(const string&);
    void setNextName(const string&);
    void setPrevName(const string&);
    void setLeadAngleGain(double g);
    void setLeadAngleLimit(double l);
    void setLeadAngleProp(double p);
    void setRudderConstant(double rc);
    void setSpeedConstant(double sc);
    void setFixedTurnRadius(double ft);
    void setRollFactor(double rf);

    void setTunnel(bool t);
    void setInitialTunnel(bool t);

    void setWPNames();
    void setWPPos();

    double sign(double x);

    bool _hdg_lock;
    bool _serviceable;
    bool _waiting;
    bool _new_waypoint;
    bool _tunnel, _initial_tunnel;
    bool _restart;

    virtual const char* getTypeString(void) const { return "ship"; }
    double _rudder_constant, _speed_constant, _hdg_constant, _limit ;
    double _elevation_m, _elevation_ft;
    double _missed_range, _tow_angle, _wait_count, _missed_count,_wp_range;
    double _dt_count, _next_run;

    FGAIWaypoint* prev; // the one behind you
    FGAIWaypoint* curr; // the one ahead
    FGAIWaypoint* next; // the next plus 1

protected:

private:


    void setRepeat(bool r);
    void setRestart(bool r);
    void setMissed(bool m);

    void setServiceable(bool s);
    void Run(double dt);
    void setStartTime(const string&);
    void setUntilTime(const string&);
    //void setWPPos();
    void setWPAlt();
    void setXTrackError();

    SGGeod wppos;

    const SGMaterial* _material;

    double getRange(double lat, double lon, double lat2, double lon2) const;
    double getCourse(double lat, double lon, double lat2, double lon2) const;
    double getDaySeconds();
    double processTimeString(const string& time);

    bool initFlightPlan();
    bool advanceFlightPlan (double elapsed_sec, double day_sec);

    float _rudder, _tgt_rudder;

    double _roll_constant, _roll_factor;
    double _sp_turn_radius_ft, _rd_turn_radius_ft, _fixed_turn_radius;
    double _old_range, _range_rate;
    double _missed_time_sec;
    double _start_sec;
    double _day;
    double _lead_angle;
    double _lead_angle_gain, _lead_angle_limit, _proportion;
    double _course;
    double _xtrack_error;
    double _curr_alt, _prev_alt;

    string _prev_name, _curr_name, _next_name;
    string _path;
    string _start_time, _until_time;

    bool _repeat;
    bool _fp_init;
    bool _missed;
   

};

#endif  // _FG_AISHIP_HXX
