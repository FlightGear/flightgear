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
#include "AIPlane.hxx"
#include "ATCProjection.hxx"

typedef enum PatternLeg {
    TAKEOFF_ROLL,
    CLIMBOUT,
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

// perhaps we could use an FGRunway instead of this
typedef struct RunwayDetails {
    Point3D threshold_pos;
    Point3D end1ortho;	// ortho projection end1 (the threshold ATM)
    Point3D end2ortho;	// ortho projection end2 (the take off end in the current hardwired scheme)
    double mag_hdg;
    double mag_var;
    double hdg;		// true runway heading
};

typedef struct StartofDescent {
    PatternLeg leg;
    double orthopos_x;
    double orthopos_y;
};

class FGAILocalTraffic : public FGAIPlane {

public:

    FGAILocalTraffic();
    ~FGAILocalTraffic();

    // Initialise
    void Init();

    // Run the internal calculations
    void Update(double dt);

protected:

    // Attempt to enter the traffic pattern in a reasonably intelligent manner
    void EnterTrafficPattern(double dt);

private:
    FGATCAlignedProjection ortho;	// Orthogonal mapping of the local area with the threshold at the origin
					// and the runway aligned with the y axis.

    // Airport/runway/pattern details
    char* airport;	// The ICAO code of the airport that we're operating around
    FGTower* tower;	// A pointer to the tower control.
    RunwayDetails rwy;
    double patternDirection;	// 1 for right, -1 for left (This is double because we multiply/divide turn rates
				// with it to get RH/LH turns - DON'T convert it to int under ANY circumstances!!
    double glideAngle;		// Assumed to be visual glidepath angle for FGAILocalTraffic - can be found at www.airnav.com
    // Its conceivable that patternDirection and glidePath could be moved into the RunwayDetails structure.

    // Performance characteristics of the plane in knots and ft/min - some of this might get moved out into FGAIPlane
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
    double nominal_final_speed;
    double stall_speed_landing_config;

    // environment - some of this might get moved into FGAIPlane
    double wind_from_hdg;	// degrees
    double wind_speed_knots;	// knots

    // Pattern details that (may) change
    int numInPattern;		// Number of planes in the pattern (this might get more complicated if high performance GA aircraft fly a higher pattern eventually)
    int numAhead;		// More importantly - how many of them are ahead of us?
    double distToNext;		// And even more importantly, how near are we getting to the one immediately ahead?
    PatternLeg leg;		// Out current position in the pattern
    StartofDescent SoD;		// Start of descent calculated wrt wind, pattern size & altitude, glideslope etc

    void FlyTrafficPattern(double dt);

    // TODO - need to add something to define what option we are flying - Touch and go / Stop and go / Landing properly / others?

    void TransmitPatternPositionReport();

    void CalculateStartofDescent();
};

#endif  // _FG_AILocalTraffic_HXX
