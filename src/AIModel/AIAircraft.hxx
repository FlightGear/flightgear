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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

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

        enum aircraft_e {LIGHT=0, WW2_FIGHTER, JET_TRANSPORT, JET_FIGHTER, TANKER};
        static const PERF_STRUCT settings[];
	
	FGAIAircraft(FGAIManager* mgr,   FGAISchedule *ref=0);
	~FGAIAircraft();
	
	bool init();
        virtual void bind();
        virtual void unbind();
	void update(double dt);

        void SetPerformance(const PERF_STRUCT *ps);
        void SetFlightPlan(FGAIFlightPlan *f);
        void AccelTo(double speed);
        void PitchTo(double angle);
        void RollTo(double angle);
        void YawTo(double angle);
        void ClimbTo(double altitude);
        void TurnTo(double heading);
        void ProcessFlightPlan( double dt );

        inline void SetTanker(bool setting) { isTanker = setting; };

private:
   FGAISchedule *trafficRef;
  
        bool hdg_lock;
        bool alt_lock;
        double dt_count;  
        double dt; 

        const PERF_STRUCT *performance;
        bool use_perf_vs;
        SGPropertyNode* refuel_node;
        bool isTanker;

	void Run(double dt);
        double sign(double x);	

        bool _getGearDown() const;
};


#endif  // _FG_AIAircraft_HXX
