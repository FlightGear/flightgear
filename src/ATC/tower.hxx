// FGTower - a class to provide tower control at towered airports.
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

#ifndef _FG_TOWER_HXX
#define _FG_TOWER_HXX

#include <simgear/compiler.h>
#include <simgear/math/point3d.hxx>
#include <simgear/misc/sgstream.hxx>
//#include <simgear/math/sg_geodesy.hxx>
#include <plib/sg.h>
//#include <Airports/runways.hxx>

#include STL_IOSTREAM
#include STL_STRING

SG_USING_STD(string);
SG_USING_STD(ios);

#include "ATC.hxx"
//#include "ATCmgr.hxx"
#include "ground.hxx"
#include "ATCProjection.hxx"
#include "AIPlane.hxx"

//DCL - a complete guess for now.
#define FG_TOWER_DEFAULT_RANGE 30

enum tower_traffic_type {
	CIRCUIT,
	INBOUND,
	OUTBOUND,
	TTT_UNKNOWN,	// departure, but we don't know if for circuits or leaving properly
	STRAIGHT_IN
	// Umm - what's the difference between INBOUND and STRAIGHT_IN ?
};

// Structure for holding details of a plane under tower control.
// Not fixed yet - may include more stuff later.
class TowerPlaneRec {
	
public:
	
	TowerPlaneRec();
	TowerPlaneRec(PlaneRec p);
	TowerPlaneRec(Point3D pt);
	TowerPlaneRec(PlaneRec p, Point3D pt);
	
	FGAIPlane* planePtr;	// This might move to the planeRec eventually
	PlaneRec plane;
	
	Point3D pos;
	double eta;		// seconds
	double dist_out;	// meters from theshold
	bool clearedToLand;
	bool clearedToLineUp;
	bool clearedToTakeOff;
	// ought to add time cleared to depart so we can nag if necessary
	bool holdShortReported;
	bool longFinalReported;
	bool longFinalAcknowledged;
	bool finalReported;
	bool finalAcknowledged;
	bool onRwy;		// is physically on the runway
	bool nextOnRwy;		// currently projected by tower to be the next on the runway

	double clearanceCounter;		// Hack for communication timing - counter since clearance requested in seconds 
	
	// Type of operation the plane is doing
	tower_traffic_type opType;
	
	// Whereabouts in circuit if doing circuits
	PatternLeg leg;
	
	bool isUser;	// true if this plane is the user
};


typedef list < TowerPlaneRec* > tower_plane_rec_list_type;
typedef tower_plane_rec_list_type::iterator tower_plane_rec_list_iterator;
typedef tower_plane_rec_list_type::const_iterator tower_plane_rec_list_const_iterator;


class FGTower : public FGATC {

public:
	
	FGTower();
	~FGTower();
	
	void Init();
	
	void Update(double dt);

	void RequestLandingClearance(string ID);
	void RequestDepartureClearance(string ID);	
	void ReportFinal(string ID);
	void ReportLongFinal(string ID);
	void ReportOuterMarker(string ID);
	void ReportMiddleMarker(string ID);
	void ReportInnerMarker(string ID);
	void ReportGoingAround(string ID);
	void ReportRunwayVacated(string ID);
	void ReportReadyForDeparture(string ID);
	
	// Contact tower when at a hold short for departure - for now we'll assume plane - maybe vehicles might want to cross runway eventually?
	void ContactAtHoldShort(PlaneRec plane, FGAIPlane* requestee, tower_traffic_type operation);
	
	// Public interface to the active runway - this will get more complex 
	// in the future and consider multi-runway use, airplane weight etc.
	inline string GetActiveRunway() { return activeRwy; }
	inline RunwayDetails GetActiveRunwayDetails() { return rwy; }
	
	inline void SetDisplay() { display = true; }
	inline void SetNoDisplay() { display = false; }
	
	inline string get_trans_ident() { return trans_ident; }
	inline atc_type GetType() { return TOWER; }
	
	inline FGGround* GetGroundPtr() { return ground; }
	
	// Returns true if positions of crosswind/downwind/base leg turns should be constrained by previous traffic
	// plus the constraint position as a rwy orientated orthopos (meters)
	bool GetCrosswindConstraint(double& cpos);
	bool GetDownwindConstraint(double& dpos);
	bool GetBaseConstraint(double& bpos);

private:
	FGATCMgr* ATCmgr;	
	// This is purely for synactic convienience to avoid writing globals->get_ATC_mgr()-> all through the code!

	// Figure out if a given position lies on the active runway
	// Might have to change when we consider more than one active rwy.
	bool OnActiveRunway(Point3D pt);
	
	// Figure out if a given position lies on a runway or not
	bool OnAnyRunway(Point3D pt);
	
	// Calculate the eta of each plane to the threshold.
	// For ground traffic this is the fastest they can get there.
	// For air traffic this is the middle approximation.
	void doThresholdETACalc();
	
	// Order the list of traffic as per expected threshold use and flag any conflicts
	bool doThresholdUseOrder();
	
	void doCommunication();
	
	void IssueLandingClearance(TowerPlaneRec* tpr);
	void IssueGoAround(TowerPlaneRec* tpr);
	void IssueDepartureClearance(TowerPlaneRec* tpr);
	
	bool display;		// Flag to indicate whether we should be outputting to the ATC display.
	bool displaying;		// Flag to indicate whether we are outputting to the ATC display.
	
	// environment - need to make sure we're getting the surface winds and not winds aloft.
	SGPropertyNode* wind_from_hdg;	//degrees
	SGPropertyNode* wind_speed_knots;		//knots
	
	double aptElev;		// Airport elevation
	string activeRwy;	// Active runway number - For now we'll disregard multiple / alternate runway operation.
	RunwayDetails rwy;	// Assumed to be the active one for now.
	bool rwyOccupied;	// Active runway occupied flag.  For now we'll disregard land-and-hold-short operations.
	FGATCAlignedProjection ortho;	// Orthogonal mapping of the local area with the active runway threshold at the origin
	
	// Figure out which runways are active.
	// For now we'll just be simple and do one active runway - eventually this will get much more complex
	// This is a private function - public interface to the results of this is through GetActiveRunway
	void DoRwyDetails();
	
	// Need a data structure to hold details of the various active planes
	// or possibly another data structure with the positions of the inactive planes.
	// Need a data structure to hold outstanding communications from aircraft.
	
	// Linked-list of planes on approach ordered with nearest first (timewise).
	// Includes planes that have landed but not yet vacated runway.
	// Somewhat analagous to the paper strips used (used to be used?) in real life.
	// Doesn't include planes in circuit until they turn onto base/final?
	tower_plane_rec_list_type appList;
	tower_plane_rec_list_iterator appListItr;
	
	// List of departed planes (planes doing circuits go into circuitList not depList after departure)
	tower_plane_rec_list_type depList;
	tower_plane_rec_list_iterator depListItr;
	
	// List of planes in the circuit (ordered by nearest to landing first)
	tower_plane_rec_list_type circuitList;
	tower_plane_rec_list_iterator circuitListItr;
	
	// List of planes waiting to depart
	tower_plane_rec_list_type holdList;
	tower_plane_rec_list_iterator holdListItr;
		
	// List of planes on rwy
	tower_plane_rec_list_type rwyList;
	tower_plane_rec_list_iterator rwyListItr;

	// List of all planes due to use a given rwy arranged in projected order of rwy use
	tower_plane_rec_list_type trafficList;	// TODO - needs to be expandable to more than one rwy
	tower_plane_rec_list_iterator trafficListItr;

	// Ground can be separate or handled by tower in real life.
	// In the program we will always use a separate FGGround class, but we need to know
	// whether it is supposed to be separate or not to give the correct instructions.
	bool separateGround;	// true if ground control is separate
	FGGround* ground;	// The ground control associated with this airport.
	
	// for failure modeling
	string trans_ident;		// transmitted ident
	bool tower_failed;		// tower failed?
	
    // Pointers to current users position and orientation
    SGPropertyNode* user_lon_node;
    SGPropertyNode* user_lat_node;
    SGPropertyNode* user_elev_node;
    SGPropertyNode* user_hdg_node;
	
	// Details of the general traffic flow etc in the circuit
	double crosswind_leg_pos;	// Distance from threshold crosswind leg is being turned to in meters (actual operation - *not* ideal circuit)
	double downwind_leg_pos;	// Actual offset distance in meters from the runway that planes are flying the downwind leg
	// Currently not sure whether the above should be always +ve or just take the natural orthopos sign (+ve for RH circuit, -ve for LH).
	double base_leg_pos;		// Actual offset distance from the threshold (-ve) that planes are turning to base leg.
	
	friend istream& operator>> ( istream&, FGTower& );
};

#endif  //_FG_TOWER_HXX
