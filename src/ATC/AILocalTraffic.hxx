// FGAILocalTraffic - AIEntity derived class with enough logic to
// fly and interact with the traffic pattern.
//
// Written by David Luff, started March 2002.
//
// Copyright (C) 2002  David C. Luff - david.luff@nottingham.ac.uk
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

/*****************************************************************
*
* WARNING - Curt has some ideas about AI traffic so anything in here
* may get rewritten or scrapped.  Contact Curt curt@flightgear.org 
* before spending any time or effort on this code!!!
*
******************************************************************/

#ifndef _FG_AILocalTraffic_HXX
#define _FG_AILocalTraffic_HXX

#include <plib/sg.h>
#include <plib/ssg.h>
#include <simgear/math/point3d.hxx>

#include "tower.hxx"
#include "AIEntity.hxx"

typedef enum pattern_leg {
    TAKEOFF_ROLL,
    OUTWARD,
    TURN1,
    CROSSWIND,
    TURN2,
    DOWNWIND,
    TURN3,
    BASE,
    TURN4,
    FINAL,
    LANDING_ROLL
};

class FGAILocalTraffic : public FGAIEntity {

public:

    FGAILocalTraffic();
    ~FGAILocalTraffic();

    // Initialise
    void Init();

    // Run the internal calculations
    void Update();

private:

    char* airport;	// The ICAO code of the airport that we're operating around
    double freq;	// The frequency that we're operating on - might not need this eventually
    FGTower* tower;	// A pointer to the tower control.

    double mag_hdg;	// degrees - the heading that the physical aircraft is pointing
    double mag_var;	// degrees

    double vel;		// velocity along track in m/s
    double track;	// track - degrees relative to *magnetic* north
    double slope;	// Actual slope that the plane is flying (degrees) - +ve is uphill
    double AoA;		// degrees - difference between slope and pitch
    // We'll assume that the plane doesn't yaw or slip - the track/heading difference is to allow for wind

    // Performance characteristics of the plane in knots and ft/min
    double Vr;
    double best_rate_of_climb_speed;
    double best_rate_of_climb;
    double nominal_climb_speed;
    double nominal_climb_rate;
    double nominal_circuit_speed;
    double min_circuit_speed;
    double max_circuit_speed;
    double nominal_descent_rate;
    double nominal_approach_speed;
    double stall_speed_landing_config;

    // OK, what do we need to know whilst flying the pattern
    pattern_leg leg;

};

#endif  // _FG_AILocalTraffic_HXX