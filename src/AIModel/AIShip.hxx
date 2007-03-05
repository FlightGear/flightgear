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

class FGAIManager;

class FGAIShip : public FGAIBase {

public:

    FGAIShip(object_type ot = otShip);
    virtual ~FGAIShip();

    virtual void readFromScenario(SGPropertyNode* scFileNode);

    virtual bool init(bool search_in_AI_path=false);
    virtual void bind();
    virtual void unbind();
    virtual void update(double dt);
    void setFlightPlan(FGAIFlightPlan* f);
    void setName(const string&);
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

    bool _hdg_lock;

    virtual const char* getTypeString(void) const { return "ship"; }

protected:

    string _name; // The _name of this ship.

private:

    FGAIFlightPlan::waypoint* prev; // the one behind you
    FGAIFlightPlan::waypoint* curr; // the one ahead
    FGAIFlightPlan::waypoint* next; // the next plus 1

    virtual void reinit() { init(); }

    void setRepeat(bool r);
    void setMissed(bool m);
    void setWPNames();
    void Run(double dt);
    double getRange(double lat, double lon, double lat2, double lon2) const;
    double getCourse(double lat, double lon, double lat2, double lon2) const;
    double sign(double x);

    bool initFlightPlan();

    float _rudder, _tgt_rudder;

    double _rudder_constant, _roll_constant, _speed_constant, _hdg_constant;
    double _sp_turn_radius_ft, _rd_turn_radius_ft;
    double _wp_range, _old_range, _range_rate;
    double _dt_count, _missed_count, _wait_count;
    double _next_run;
    double _missed_time_sec;

    string _prev_name, _curr_name, _next_name;

    bool _repeat;
    bool _fp_init;
    bool _new_waypoint;
    bool _missed, _waiting;

};

#endif  // _FG_AISHIP_HXX
