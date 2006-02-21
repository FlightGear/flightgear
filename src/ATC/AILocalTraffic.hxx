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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#ifndef _FG_AILocalTraffic_HXX
#define _FG_AILocalTraffic_HXX

#include <simgear/math/point3d.hxx>
#include <Main/fg_props.hxx>

#include "tower.hxx"
#include "AIPlane.hxx"
#include "ATCProjection.hxx"
#include "ground.hxx"

#include <string>
SG_USING_STD(string);

enum TaxiState {
	TD_INBOUND,
	TD_OUTBOUND,
	TD_NONE,
	TD_LINING_UP
};

enum OperatingState {
	IN_PATTERN,
	TAXIING,
	PARKED,
	EN_ROUTE
};

struct StartOfDescent {
	PatternLeg leg;
	double x;	// Runway aligned orthopos
	double y;	// ditto
};

class FGAILocalTraffic : public FGAIPlane {
	
public:
	
	// At the moment we expect the expanded short form callsign - eventually we will just want the reg + type.
	FGAILocalTraffic();
	~FGAILocalTraffic();
	
	// Initialise
	bool Init(const string& callsign, const string& ICAO, OperatingState initialState = PARKED, PatternLeg initialLeg = DOWNWIND);
	
	// Run the internal calculations
	void Update(double dt);
	
	// Go out and practice circuits
	void FlyCircuits(int numCircuits, bool tag);
	
	// Return what type of landing we're doing on this circuit
	LandingType GetLandingOption();
	
	// TODO - this will get more complex and moved into the main class
	// body eventually since the position approved to taxi to will have
	// to be passed.
	inline void ApproveTaxiRequest() {taxiRequestCleared = true;}
	
	inline void DenyTaxiRequest() {taxiRequestCleared = false;}
	
	void RegisterTransmission(int code);
	
	// Process callbacks sent by base class
	// (These codes are not related to the codes above)
	void ProcessCallback(int code);
	
	// This is a hack and will probably go eventually
	inline bool AtHoldShort() {return holdingShort;}
	
protected:
	
	// Attempt to enter the traffic pattern in a reasonably intelligent manner
	void EnterTrafficPattern(double dt);
	
	// Set up the internal state to be consistent for a downwind entry.
	void DownwindEntry();
	
	// Ditto for straight-in
	void StraightInEntry(bool des = false);
	
	// Do what is necessary to land and parkup at home airport
	void ReturnToBase(double dt);
	
	// Airport/runway/pattern details
	string airportID;	// The ICAO code of the airport that we're operating around
	double aptElev;		// Airport elevation
	FGGround* ground;	// A pointer to the ground control.
	FGTower* tower;	// A pointer to the tower control.
	bool _controlled;	// Set true if we find tower control working for the airport, false otherwise.
	RunwayDetails rwy;
	double patternDirection;	// 1 for right, -1 for left (This is double because we multiply/divide turn rates
	// with it to get RH/LH turns - DON'T convert it to int under ANY circumstances!!
	double glideAngle;		// Assumed to be visual glidepath angle for FGAILocalTraffic - can be found at www.airnav.com
	// Its conceivable that patternDirection and glidePath could be moved into the RunwayDetails structure.
	
	// Its possible that this might be moved out to the ground/airport class at some point.
	FGATCAlignedProjection ortho;	// Orthogonal mapping of the local area with the threshold at the origin
	// and the runway aligned with the y axis.
	
	void GetAirportDetails(const string& id);
	
	void GetRwyDetails(const string& id);
	
	double responseCounter;		// timer in seconds to allow response to requests to be a little while after them
	// Will almost certainly get moved to FGAIPlane.	
	
private:
	FGATCMgr* ATC;	
	// This is purely for synactic convienience to avoid writing globals->get_ATC_mgr()-> all through the code!

	// High-level stuff
	OperatingState operatingState;
	int circuitsToFly;	//Number of circuits still to do in this session NOT INCLUDING THE CURRENT ONE
	bool touchAndGo;	//True if circuits should be flown touch and go, false for full stop
	bool transmitted;	// Set true when a position report for the current leg has been transmitted.
	
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
	//PatternLeg leg;		// Our current position in the pattern - now moved to FGAIPlane
	StartOfDescent SoD;		// Start of descent calculated wrt wind, pattern size & altitude, glideslope etc
	bool descending;		// We're in the coming down phase of the pattern
	double targetDescentRate;	// m/s

	// Taxiing details
	// At the moment this assumes that all taxiing in is to gates (a loose term that includes
	// any permitted parking spot) and that all taxiing out is to runways.
	bool parked;
	bool taxiing;
	bool taxiRequestPending;
	bool taxiRequestCleared;
	TaxiState taxiState;
	double desiredTaxiHeading;
	double taxiTurnRadius;
	double nominalTaxiSpeed;
	Gate* ourGate;
	ground_network_path_type path;	// a path through the ground network for the plane to taxi
	unsigned int taxiPathPos;	// position of iterator in taxi path when applicable
	node* nextTaxiNode;	// next node in taxi path
	node* holdShortNode;
	//Runway out_dest; //FIXME - implement this
	bool holdingShort;
	bool reportReadyForDeparture;	// set true when ATC has requested that the plane report when ready for departure
	bool clearedToLineUp;
	bool clearedToTakeOff;
	bool _clearedToLand;	// also implies cleared for the option.
	bool liningUp;	// Set true when the turn onto the runway heading is commenced when taxiing out
	bool goAround;	// Set true if need to go-around
	bool goAroundCalled;	// Set true during go-around only after we have called our go-around on the radio
	bool contactTower;	// we have been told to contact tower
	bool contactGround;	// we have been told to contact ground
	bool changeFreq;	// true when we need to change frequency
	bool _taxiToGA;		// Temporary mega-hack indicating we are to taxi to the GA parking and disconnect from tower control.
	bool _removeSelf;	// Indicates that we wish to remove this instance.  The use of a variable is a hack to allow time for messages to purge before removal, due to the fagility of the current dialog system.
	atc_type changeFreqType;	// the service we need to change to
	bool freeTaxi;	// False if the airport has a facilities file with a logical taxi network defined, true if we need to calculate our own taxiing points.
	
	// Hack for getting close to the runway when atan can go pear-shaped
	double _savedSlope;

	void FlyTrafficPattern(double dt);

	// TODO - need to add something to define what option we are flying - Touch and go / Stop and go / Landing properly / others?

	void TransmitPatternPositionReport();

	void CalculateSoD(double base_leg_pos, double downwind_leg_pos, bool pattern_direction);

	void ExitRunway(const Point3D& orthopos);

	void StartTaxi();

	void Taxi(double dt);

	void GetNextTaxiNode();
	
	void DoGroundElev();
	
	// Set when the plane should be invisible *regardless of distance from user*.
	bool _invisible;
};

#endif  // _FG_AILocalTraffic_HXX
