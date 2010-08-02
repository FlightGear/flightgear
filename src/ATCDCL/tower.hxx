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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#ifndef _FG_TOWER_HXX
#define _FG_TOWER_HXX

#include <simgear/compiler.h>
#include <simgear/math/SGMath.hxx>
#include <simgear/misc/sgstream.hxx>

#include <iosfwd>
#include <string>
#include <list>

#include "ATC.hxx"
#include "ATCProjection.hxx"

class FGATCMgr;
class FGGround;

//DCL - a complete guess for now.
#define FG_TOWER_DEFAULT_RANGE 30

enum tower_traffic_type {
	CIRCUIT,
	INBOUND,	// CIRCUIT traffic gets changed to INBOUND when on final of the full-stop circuit.
	OUTBOUND,
	TTT_UNKNOWN,	// departure, but we don't know if for circuits or leaving properly
	STRAIGHT_IN
};

ostream& operator << (ostream& os, tower_traffic_type ttt);

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
	LANDING_ROLL,
	LEG_UNKNOWN
};

ostream& operator << (ostream& os, PatternLeg pl);

enum LandingType {
	FULL_STOP,
	STOP_AND_GO,
	TOUCH_AND_GO,
	AIP_LT_UNKNOWN
};

ostream& operator << (ostream& os, LandingType lt);

enum tower_callback_type {
	USER_REQUEST_VFR_DEPARTURE = 1,
	USER_REQUEST_VFR_ARRIVAL = 2,
	USER_REQUEST_VFR_ARRIVAL_FULL_STOP = 3,
	USER_REQUEST_VFR_ARRIVAL_TOUCH_AND_GO = 4,
	USER_REPORT_3_MILE_FINAL = 5,
	USER_REPORT_DOWNWIND = 6,
	USER_REPORT_RWY_VACATED = 7,
	USER_REPORT_GOING_AROUND = 8,
	USER_REQUEST_TAKE_OFF = 9
};

// TODO - need some differentiation of IFR and VFR traffic in order to give the former priority.

// Structure for holding details of a plane under tower control.
// Not fixed yet - may include more stuff later.
class TowerPlaneRec {
	
public:
	
	TowerPlaneRec();
	TowerPlaneRec(const PlaneRec& p);
	TowerPlaneRec(const SGGeod& pt);
	TowerPlaneRec(const PlaneRec& p, const SGGeod& pt);
	
	PlaneRec plane;
	
	SGGeod pos;
	double eta;		// seconds
	double dist_out;	// meters from theshold
	bool clearedToLand;
	bool clearedToLineUp;
	bool clearedToTakeOff;
	// ought to add time cleared to depart so we can nag if necessary
	bool holdShortReported;
	bool lineUpReported;
	bool downwindReported;
	bool longFinalReported;
	bool longFinalAcknowledged;
	bool finalReported;
	bool finalAcknowledged;
	bool rwyVacatedReported;
	bool rwyVacatedAcknowledged;
	bool goAroundReported;		// set true if plane informs tower that it's going around.
	bool instructedToGoAround;	// set true if plane told by tower to go around.
	bool onRwy;		// is physically on the runway
	bool nextOnRwy;		// currently projected by tower to be the next on the runway
	bool gearWasUp;          // Tell to ATC about gear
	bool gearUpReported;     // Tell to pilot about landing gear
	
	bool vfrArrivalReported;
	bool vfrArrivalAcknowledged;

	// Type of operation the plane is doing
	tower_traffic_type opType;
	
	// Whereabouts in circuit if doing circuits
	PatternLeg leg;
	
	LandingType landingType;
	bool isUser;	// true if this plane is the user
};


typedef std::list < TowerPlaneRec* > tower_plane_rec_list_type;
typedef tower_plane_rec_list_type::iterator tower_plane_rec_list_iterator;
typedef tower_plane_rec_list_type::const_iterator tower_plane_rec_list_const_iterator;


class FGTower : public FGATC {

public:
	
	FGTower();
	~FGTower();
	
	void Init();
	
	void Update(double dt);
	
	void ReceiveUserCallback(int code);

	// Contact tower for VFR approach
	// eg "Cessna Charlie Foxtrot Golf Foxtrot Sierra eight miles South of the airport for full stop with Bravo"
	// This function probably only called via user interaction - AI planes will have an overloaded function taking a planerec.
	void VFRArrivalContact(const std::string& ID, const LandingType& opt = AIP_LT_UNKNOWN);
	
	void RequestDepartureClearance(const std::string& ID);
	void RequestTakeOffClearance(const std::string& ID);
	void ReportFinal(const std::string& ID);
	void ReportLongFinal(const std::string& ID);
	void ReportOuterMarker(const std::string& ID);
	void ReportMiddleMarker(const std::string& ID);
	void ReportInnerMarker(const std::string& ID);
	void ReportRunwayVacated(const std::string& ID);
	void ReportReadyForDeparture(const std::string& ID);
	void ReportDownwind(const std::string& ID);
	void ReportGoingAround(const std::string& ID);
	
	// Public interface to the active runway - this will get more complex 
	// in the future and consider multi-runway use, airplane weight etc.
	inline const std::string& GetActiveRunway() const { return activeRwy; }
	inline const RunwayDetails& GetActiveRunwayDetails() const { return rwy; }
	// Get the pattern direction of the active rwy.
	inline int GetPatternDirection() const { return rwy.patternDirection; }
	
	inline const std::string& get_trans_ident() const { return trans_ident; }
	
	inline FGGround* const GetGroundPtr() const { return ground; }
	
	// Returns true if positions of crosswind/downwind/base leg turns should be constrained by previous traffic
	// plus the constraint position as a rwy orientated orthopos (meters)
	bool GetCrosswindConstraint(double& cpos);
	bool GetDownwindConstraint(double& dpos);
	bool GetBaseConstraint(double& bpos);
	
	std::string GenText(const std::string& m, int c);
	std::string GetWeather();
	std::string GetATISID();

private:
	FGATCMgr* ATCmgr;
	// This is purely for synactic convienience to avoid writing globals->get_ATC_mgr()-> all through the code!
	
	// Respond to a transmission
	void Respond();
	
	void ProcessRunwayVacatedReport(TowerPlaneRec* t);
	void ProcessDownwindReport(TowerPlaneRec* t);
	
	// Remove all options from the user dialog choice
	void RemoveAllUserDialogOptions();
	
	// Periodic checks on the various traffic.
	void CheckHoldList(double dt);
	void CheckCircuitList(double dt);
	void CheckRunwayList(double dt);
	void CheckApproachList(double dt);
	void CheckDepartureList(double dt);
	
	// Currently this assumes we *are* next on the runway and doesn't check for planes about to land - 
	// this should be done prior to calling this function.
	void ClearHoldingPlane(TowerPlaneRec* t);
	
	// Find a pointer to plane of callsign ID within the internal data structures
	TowerPlaneRec* FindPlane(const std::string& ID);
	
	// Remove and delete all instances of a plane with a given ID
	void RemovePlane(const std::string& ID);
	
	// Figure out if a given position lies on the active runway
	// Might have to change when we consider more than one active rwy.
	bool OnActiveRunway(const SGGeod& pt);
	
	// Figure out if a given position lies on a runway or not
	bool OnAnyRunway(const SGGeod& pt, bool onGround);
	
	// Calculate the eta of a plane to the threshold.
	// For ground traffic this is the fastest they can get there.
	// For air traffic this is the middle approximation.
	void CalcETA(TowerPlaneRec* tpr, bool printout = false);
	
	// Iterate through all the std::lists and call CalcETA for all the planes.
	void doThresholdETACalc();
	
	// Order the std::list of traffic as per expected threshold use and flag any conflicts
	bool doThresholdUseOrder();
	
	// Calculate the crow-flys distance of a plane to the threshold in meters
	double CalcDistOutM(TowerPlaneRec* tpr);

	// Calculate the crow-flys distance of a plane to the threshold in miles
	double CalcDistOutMiles(TowerPlaneRec* tpr);
	
	/*
	void doCommunication();
	*/
	
	void IssueLandingClearance(TowerPlaneRec* tpr);
	void IssueGoAround(TowerPlaneRec* tpr);
	void IssueDepartureClearance(TowerPlaneRec* tpr);
	
	unsigned int update_count;	// Convienince counter for speading computational load over several updates
	unsigned int update_count_max;	// ditto.
	
	double timeSinceLastDeparture;	// Time in seconds since last departure from active rwy.
	bool departed;	// set true when the above needs incrementing with time, false when it doesn't.
	
	// environment - need to make sure we're getting the surface winds and not winds aloft.
	SGPropertyNode_ptr wind_from_hdg;	//degrees
	SGPropertyNode_ptr wind_speed_knots;		//knots
	
	double aptElev;		// Airport elevation
	std::string activeRwy;	// Active runway number - For now we'll disregard multiple / alternate runway operation.
	RunwayDetails rwy;	// Assumed to be the active one for now.
	bool rwyOccupied;	// Active runway occupied flag.  For now we'll disregard land-and-hold-short operations.
	FGATCAlignedProjection ortho;	// Orthogonal mapping of the local area with the active runway threshold at the origin
	FGATCAlignedProjection ortho_temp;	// Ortho for any runway (needed to get plane position in airport)
	
	// Figure out which runways are active.
	// For now we'll just be simple and do one active runway - eventually this will get much more complex
	// This is a private function - public interface to the results of this is through GetActiveRunway
	void DoRwyDetails();
	
	// Need a data structure to hold details of the various active planes
	// or possibly another data structure with the positions of the inactive planes.
	// Need a data structure to hold outstanding communications from aircraft.
	
	// Linked-list of planes on approach to active rwy ordered with nearest first (timewise).
	// Includes planes that have landed but not yet vacated runway.
	// Somewhat analagous to the paper strips used (used to be used?) in real life.
	// Doesn't include planes in circuit until they turn onto base/final?
	// TODO - may need to alter this for operation to more than one active rwy.
	tower_plane_rec_list_type appList;
	tower_plane_rec_list_iterator appListItr;

	// What should we do with planes approaching the airport to join the circuit somewhere
	// but not on straight-in though? - put them in here for now.	
	tower_plane_rec_list_type circuitAppList;
	tower_plane_rec_list_iterator circuitAppListItr;
	
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
	
	// List of planes that have vacated the runway inbound but not yet handed off to ground
	tower_plane_rec_list_type vacatedList;
	tower_plane_rec_list_iterator vacatedListItr;
	
	// Returns true if successful
        bool RemoveFromTrafficList(const std::string& id);
	bool RemoveFromAppList(const std::string& id);
	bool RemoveFromRwyList(const std::string& id);
	
	// Return the ETA of plane no. list_pos (1-based) in the traffic list.
	// i.e. list_pos = 1 implies next to use runway.
	double GetTrafficETA(unsigned int list_pos, bool printout = false);
	
	// Add a tower plane rec with ETA to the traffic list in the correct position ETA-wise.
	// Returns true if this could cause a threshold ETA conflict with other traffic, false otherwise.
	bool AddToTrafficList(TowerPlaneRec* t, bool holding = false);
	
	bool AddToCircuitList(TowerPlaneRec* t);
	
	// Add to vacated list only if not already present
	void AddToVacatedList(TowerPlaneRec* t);
	
	void AddToHoldingList(TowerPlaneRec* t);

	// Ground can be separate or handled by tower in real life.
	// In the program we will always use a separate FGGround class, but we need to know
	// whether it is supposed to be separate or not to give the correct instructions.
	bool separateGround;	// true if ground control is separate
	FGGround* ground;	// The ground control associated with this airport.
	
	bool _departureControlled;	// true if we need to hand off departing traffic to departure control
	//FGDeparture* _departure;	// The relevant departure control (once we've actually written it!)
	
	// for failure modeling
	std::string trans_ident;		// transmitted ident
	bool tower_failed;		// tower failed?
	
	// Pointers to current users position and orientation
	SGPropertyNode_ptr user_lon_node;
	SGPropertyNode_ptr user_lat_node;
	SGPropertyNode_ptr user_elev_node;
	SGPropertyNode_ptr user_hdg_node;
	
	// Details of the general traffic flow etc in the circuit
	double crosswind_leg_pos;	// Distance from threshold crosswind leg is being turned to in meters (actual operation - *not* ideal circuit)
	double downwind_leg_pos;	// Actual offset distance in meters from the runway that planes are flying the downwind leg
	// Currently not sure whether the above should be always +ve or just take the natural orthopos sign (+ve for RH circuit, -ve for LH).
	double base_leg_pos;		// Actual offset distance from the threshold (-ve) that planes are turning to base leg.
	
	double nominal_crosswind_leg_pos;
	double nominal_downwind_leg_pos;
	double nominal_base_leg_pos;
	
        friend std::istream& operator>> ( std::istream&, FGTower& );
};

#endif  //_FG_TOWER_HXX
