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

#ifndef _FG_AILocalTraffic_HXX
#define _FG_AILocalTraffic_HXX

#include <plib/sg.h>
#include <plib/ssg.h>
#include <simgear/math/point3d.hxx>
#include <Main/fg_props.hxx>

#include "tower.hxx"
#include "AIPlane.hxx"
#include "ATCProjection.hxx"
#include "ground.hxx"

#include <string>
SG_USING_STD(string);

enum PatternLeg {
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

enum TaxiState {
	TD_INBOUND,
	TD_OUTBOUND,
	TD_NONE
};

enum OperatingState {
	IN_PATTERN,
	TAXIING,
	PARKED
};

// perhaps we could use an FGRunway instead of this
struct RunwayDetails {
	Point3D threshold_pos;
	Point3D end1ortho;	// ortho projection end1 (the threshold ATM)
	Point3D end2ortho;	// ortho projection end2 (the take off end in the current hardwired scheme)
	double mag_hdg;
	double mag_var;
	double hdg;		// true runway heading
	double length;	// In *METERS*
	int ID;		// 1 -> 36
	string rwyID;
};

struct StartofDescent {
	PatternLeg leg;
	double orthopos_x;
	double orthopos_y;
};

class FGAILocalTraffic : public FGAIPlane {
	
public:
	
	FGAILocalTraffic();
	~FGAILocalTraffic();
	
	// Initialise
	bool Init(string ICAO, OperatingState initialState = PARKED, PatternLeg initialLeg = DOWNWIND);
	
	// Run the internal calculations
	void Update(double dt);
	
	// Go out and practice circuits
	void FlyCircuits(int numCircuits, bool tag);
	
protected:
	
	// Attempt to enter the traffic pattern in a reasonably intelligent manner
	void EnterTrafficPattern(double dt);
	
	// Do what is necessary to land and parkup at home airport
	void ReturnToBase(double dt);
	
private:
	FGATCMgr* ATC;	
	// This is purely for synactic convienience to avoid writing globals->get_ATC_mgr()-> all through the code!

	// High-level stuff
	OperatingState operatingState;
	int circuitsToFly;	//Number of circuits still to do in this session NOT INCLUDING THE CURRENT ONE
	bool touchAndGo;	//True if circuits should be flown touch and go, false for full stop
	
	// Its possible that this might be moved out to the ground/airport class at some point.
	FGATCAlignedProjection ortho;	// Orthogonal mapping of the local area with the threshold at the origin
	// and the runway aligned with the y axis.
	
	// Airport/runway/pattern details
	string airportID;	// The ICAO code of the airport that we're operating around
	double aptElev;		// Airport elevation
	FGGround* ground;	// A pointer to the ground control.
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
	double nominal_taxi_speed;
	
	// Physical/rendering stuff
	double wheelOffset;		// Height above ground at which we need to render the plane whilst taxiing
	bool elevInitGood;		// We have had at least one good elev reading
	bool inAir;				// True when off the ground 
	
	// environment - some of this might get moved into FGAIPlane
	SGPropertyNode* wind_from_hdg;	//degrees
	SGPropertyNode* wind_speed_knots;		//knots
	
	// Pattern details that (may) change
	int numInPattern;		// Number of planes in the pattern (this might get more complicated if high performance GA aircraft fly a higher pattern eventually)
	int numAhead;		// More importantly - how many of them are ahead of us?
	double distToNext;		// And even more importantly, how near are we getting to the one immediately ahead?
	PatternLeg leg;		// Out current position in the pattern
	StartofDescent SoD;		// Start of descent calculated wrt wind, pattern size & altitude, glideslope etc

	// Taxiing details
	// At the moment this assumes that all taxiing in is to gates (a loose term that includes
	// any permitted parking spot) and that all taxiing out is to runways.
	bool parked;
	bool taxiing;
	TaxiState taxiState;
	double desiredTaxiHeading;
	double taxiTurnRadius;
	double nominalTaxiSpeed;
	Gate* ourGate;
	ground_network_path_type path;	// a path through the ground network for the plane to taxi
	unsigned int taxiPathPos;	// position of iterator in taxi path when applicable
	node* nextTaxiNode;	// next node in taxi path
	//Runway out_dest; //FIXME - implement this
	bool liningUp;	// Set true when the turn onto the runway heading is commenced when taxiing out

	void FlyTrafficPattern(double dt);

	// TODO - need to add something to define what option we are flying - Touch and go / Stop and go / Landing properly / others?

	void TransmitPatternPositionReport();

	void CalculateStartofDescent();

	void ExitRunway(Point3D orthopos);

	void StartTaxi();

	void Taxi(double dt);

	void GetNextTaxiNode();
	
	void DoGroundElev();
	
	void GetRwyDetails();
};

#endif  // _FG_AILocalTraffic_HXX
