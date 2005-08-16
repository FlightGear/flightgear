// FGAIShip - AIBase derived class creates an AI ship
//
// Written by David Culp, started November 2003.
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

#ifndef _FG_AISHIP_HXX
#define _FG_AISHIP_HXX

#include "AIBase.hxx"
class FGAIManager;

class FGAIShip : public FGAIBase {
	
public:
	
	FGAIShip(FGAIManager* mgr);
	~FGAIShip();
	
	bool init();
        virtual void bind();
        virtual void unbind();
	void update(double dt);
        void setFlightPlan(FGAIFlightPlan* f);
        void setName(const string&);
        void setRudder(float r);
        void setRoll(double rl);
        
        void ProcessFlightPlan( double dt );

        void AccelTo(double speed);
        void PitchTo(double angle);
        void RollTo(double angle);
        void YawTo(double angle);
        void ClimbTo(double altitude);
        void TurnTo(double heading);
	    bool hdg_lock;
	
protected:

	string name; // The name of this ship.

private:

        float rudder, tgt_rudder;
        double rudder_constant, roll_constant, speed_constant, hdg_constant;

	void Run(double dt);
        double sign(double x);	
};

#endif  // _FG_AISHIP_HXX
