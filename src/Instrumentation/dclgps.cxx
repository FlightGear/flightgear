// dclgps.cxx - a class to extend the operation of FG's current GPS
// code, and provide support for a KLN89-specific instrument.  It
// is envisioned that eventually this file and class will be split
// up between current FG code and new KLN89-specific code and removed.
//
// Written by David Luff, started 2005.
//
// Copyright (C) 2005 - David C Luff - david.luff@nottingham.ac.uk
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
//
// $Id$

#include "dclgps.hxx"

#include <simgear/sg_inlines.h>
#include <simgear/structure/commands.hxx>
#include <Main/fg_props.hxx>
#include <iostream>
SG_USING_STD(cout);

//using namespace std;

// Command callbacks for FlightGear

static bool do_kln89_msg_pressed(const SGPropertyNode* arg) {
	//cout << "do_kln89_msg_pressed called!\n";
	DCLGPS* gps = (DCLGPS*)globals->get_subsystem("kln89");
	gps->MsgPressed();
	return(true);
}

static bool do_kln89_obs_pressed(const SGPropertyNode* arg) {
	//cout << "do_kln89_obs_pressed called!\n";
	DCLGPS* gps = (DCLGPS*)globals->get_subsystem("kln89");
	gps->OBSPressed();
	return(true);
}

static bool do_kln89_alt_pressed(const SGPropertyNode* arg) {
	//cout << "do_kln89_alt_pressed called!\n";
	DCLGPS* gps = (DCLGPS*)globals->get_subsystem("kln89");
	gps->AltPressed();
	return(true);
}

static bool do_kln89_nrst_pressed(const SGPropertyNode* arg) {
	DCLGPS* gps = (DCLGPS*)globals->get_subsystem("kln89");
	gps->NrstPressed();
	return(true);
}

static bool do_kln89_dto_pressed(const SGPropertyNode* arg) {
	DCLGPS* gps = (DCLGPS*)globals->get_subsystem("kln89");
	gps->DtoPressed();
	return(true);
}

static bool do_kln89_clr_pressed(const SGPropertyNode* arg) {
	DCLGPS* gps = (DCLGPS*)globals->get_subsystem("kln89");
	gps->ClrPressed();
	return(true);
}

static bool do_kln89_ent_pressed(const SGPropertyNode* arg) {
	DCLGPS* gps = (DCLGPS*)globals->get_subsystem("kln89");
	gps->EntPressed();
	return(true);
}

static bool do_kln89_crsr_pressed(const SGPropertyNode* arg) {
	DCLGPS* gps = (DCLGPS*)globals->get_subsystem("kln89");
	gps->CrsrPressed();
	return(true);
}

static bool do_kln89_knob1left1(const SGPropertyNode* arg) {
	DCLGPS* gps = (DCLGPS*)globals->get_subsystem("kln89");
	gps->Knob1Left1();
	return(true);
}

static bool do_kln89_knob1right1(const SGPropertyNode* arg) {
	DCLGPS* gps = (DCLGPS*)globals->get_subsystem("kln89");
	gps->Knob1Right1();
	return(true);
}

static bool do_kln89_knob2left1(const SGPropertyNode* arg) {
	DCLGPS* gps = (DCLGPS*)globals->get_subsystem("kln89");
	gps->Knob2Left1();
	return(true);
}

static bool do_kln89_knob2right1(const SGPropertyNode* arg) {
	DCLGPS* gps = (DCLGPS*)globals->get_subsystem("kln89");
	gps->Knob2Right1();
	return(true);
}

// End command callbacks

GPSWaypoint::GPSWaypoint() {
    appType = GPS_APP_NONE;
}

GPSWaypoint::~GPSWaypoint() {}

string GPSWaypoint::GetAprId() {
	if(appType == GPS_IAF) return(id + 'i');
	else if(appType == GPS_FAF) return(id + 'f');
	else if(appType == GPS_MAP) return(id + 'm');
	else if(appType == GPS_MAHP) return(id + 'h');
	else return(id);
}

ostream& operator << (ostream& os, GPSAppWpType type) {
	switch(type) {
		case(GPS_IAF):       return(os << "IAF");
		case(GPS_IAP):       return(os << "IAP");
		case(GPS_FAF):       return(os << "FAF");
		case(GPS_MAP):       return(os << "MAP");
		case(GPS_MAHP):      return(os << "MAHP");
		case(GPS_HDR):       return(os << "HEADER");
		case(GPS_FENCE):     return(os << "FENCE");
		case(GPS_APP_NONE):  return(os << "NONE");
	}
	return(os << "ERROR - Unknown switch in GPSAppWpType operator << ");
}

FGIAP::FGIAP() {
}

FGIAP::~FGIAP() {
}

FGNPIAP::FGNPIAP() {
}

FGNPIAP::~FGNPIAP() {
}

GPSPage::GPSPage(DCLGPS* parent) {
	_parent = parent;
	_subPage = 0;
}

GPSPage::~GPSPage() {
}

void GPSPage::Update(double dt) {}

void GPSPage::Knob1Left1() {}
void GPSPage::Knob1Right1() {}

void GPSPage::Knob2Left1() {
	_parent->_activePage->LooseFocus();
	_subPage--;
	if(_subPage < 0) _subPage = _nSubPages - 1;
}

void GPSPage::Knob2Right1() {
	_parent->_activePage->LooseFocus();
	_subPage++;
	if(_subPage >= _nSubPages) _subPage = 0;
}

void GPSPage::CrsrPressed() {}
void GPSPage::EntPressed() {}
void GPSPage::ClrPressed() {}
void GPSPage::DtoPressed() {}
void GPSPage::NrstPressed() {}
void GPSPage::AltPressed() {}
void GPSPage::OBSPressed() {}
void GPSPage::MsgPressed() {}

string GPSPage::GPSitoa(int n) {
	char buf[4];
	// TODO - sanity check n!
	sprintf(buf, "%i", n);
	string s = buf;
	return(s);
}

void GPSPage::CleanUp() {}
void GPSPage::LooseFocus() {}
void GPSPage::SetId(const string& s) {}

// ------------------------------------------------------------------------------------- //

DCLGPS::DCLGPS(RenderArea2D* instrument) {
	_instrument = instrument;
	_nFields = 1;
	_maxFields = 2;
	_pages.clear();
	
	// Units - lets default to US units - FG can set them to other units from config during startup if desired.
	_altUnits = GPS_ALT_UNITS_FT;
	_baroUnits = GPS_PRES_UNITS_IN;
	_velUnits = GPS_VEL_UNITS_KT;
	_distUnits = GPS_DIST_UNITS_NM;

	_lon_node = fgGetNode("/instrumentation/gps/indicated-longitude-deg", true);
	_lat_node = fgGetNode("/instrumentation/gps/indicated-latitude-deg", true);
	_alt_node = fgGetNode("/instrumentation/gps/indicated-altitude-ft", true);
	_grnd_speed_node = fgGetNode("/instrumentation/gps/indicated-ground-speed-kt", true);
	_true_track_node = fgGetNode("/instrumentation/gps/indicated-track-true-deg", true);
	_mag_track_node = fgGetNode("/instrumentation/gps/indicated-track-magnetic-deg", true);
	
	// Use FG's position values at construction in case FG's gps has not run first update yet.
	_lon = fgGetDouble("/position/longitude-deg") * SG_DEGREES_TO_RADIANS;
	_lat = fgGetDouble("/position/latitude-deg") * SG_DEGREES_TO_RADIANS;
	_alt = fgGetDouble("/position/altitude-ft");
	// Note - we can depriciate _gpsLat and _gpsLon if we implement error handling in FG
	// gps code and not our own.
	_gpsLon = _lon;
	_gpsLat = _lat;
	_checkLon = _gpsLon;
	_checkLat = _gpsLat;
	_groundSpeed_ms = 0.0;
	_groundSpeed_kts = 0.0;
	_track = 0.0;
	_magTrackDeg = 0.0;
	
	// Sensible defaults.  These can be overriden by derived classes if desired.
	_cdiScales.clear();
	_cdiScales.push_back(5.0);
	_cdiScales.push_back(1.0);
	_cdiScales.push_back(0.3);
	_currentCdiScaleIndex = 0;
	_targetCdiScaleIndex = 0;
	_sourceCdiScaleIndex = 0;
	_cdiScaleTransition = false;
	_currentCdiScale = 5.0;
	
	_cleanUpPage = -1;
	
	_activeWaypoint.id.clear();
	_dist2Act = 0.0;
	_crosstrackDist = 0.0;
	_headingBugTo = true;
	_navFlagged = true;
	_waypointAlert = false;
	_departed = false;
	_departureTimeString = "----";
	_elapsedTime = 0.0;
	
	// Configuration Initialisation
	// Should this be in kln89.cxx ?
	_turnAnticipationEnabled = false;
	_suaAlertEnabled = false;
	_altAlertEnabled = false;
        
	_time = new SGTime;
	
	_messageStack.clear();
	
	_dto = false;
	
	_approachLoaded = false;
	_approachArm = false;
	_approachReallyArmed = false;
	_approachActive = false;
	_approachFP = new GPSFlightPlan;
}

DCLGPS::~DCLGPS() {
	delete _time;
	for(gps_waypoint_map_iterator itr = _waypoints.begin(); itr != _waypoints.end(); ++itr) {
		for(unsigned int i = 0; i < (*itr).second.size(); ++i) {
			delete(((*itr).second)[i]);
		}
	}
	delete _approachFP;		// Don't need to delete the waypoints inside since they point to
							// the waypoints in the approach database.
	// TODO - may need to delete the approach database!!
}

void DCLGPS::draw() {
	//cout << "draw called!\n";
	_instrument->draw();
}

void DCLGPS::init() {
	globals->get_commands()->addCommand("kln89_msg_pressed", do_kln89_msg_pressed);
	globals->get_commands()->addCommand("kln89_obs_pressed", do_kln89_obs_pressed);
	globals->get_commands()->addCommand("kln89_alt_pressed", do_kln89_alt_pressed);
	globals->get_commands()->addCommand("kln89_nrst_pressed", do_kln89_nrst_pressed);
	globals->get_commands()->addCommand("kln89_dto_pressed", do_kln89_dto_pressed);
	globals->get_commands()->addCommand("kln89_clr_pressed", do_kln89_clr_pressed);
	globals->get_commands()->addCommand("kln89_ent_pressed", do_kln89_ent_pressed);
	globals->get_commands()->addCommand("kln89_crsr_pressed", do_kln89_crsr_pressed);
	globals->get_commands()->addCommand("kln89_knob1left1", do_kln89_knob1left1);
	globals->get_commands()->addCommand("kln89_knob1right1", do_kln89_knob1right1);
	globals->get_commands()->addCommand("kln89_knob2left1", do_kln89_knob2left1);
	globals->get_commands()->addCommand("kln89_knob2right1", do_kln89_knob2right1);
	
	// Build the GPS-specific databases.
	// TODO - consider splitting into real life GPS database regions - eg Americas, Europe etc.
	// Note that this needs to run after FG's airport and nav databases are up and running
	_waypoints.clear();
	const airport_list* apts = globals->get_airports()->getAirportList();
	for(unsigned int i = 0; i < apts->size(); ++i) {
		FGAirport* a = (*apts)[i];
		GPSWaypoint* w = new GPSWaypoint;
		w->id = a->getId();
		w->lat = a->getLatitude() * SG_DEGREES_TO_RADIANS;
		w->lon = a->getLongitude() * SG_DEGREES_TO_RADIANS;
		w->type = GPS_WP_APT;
		gps_waypoint_map_iterator wtr = _waypoints.find(a->getId());
		if(wtr == _waypoints.end()) {
			gps_waypoint_array arr;
			arr.push_back(w);
			_waypoints[w->id] = arr;
		} else {
			wtr->second.push_back(w);
		}
	}
	nav_map_type navs = globals->get_navlist()->get_navaids();
	for(nav_map_iterator itr = navs.begin(); itr != navs.end(); ++itr) {
		nav_list_type nlst = itr->second;
		for(unsigned int i = 0; i < nlst.size(); ++i) {
			FGNavRecord* n = nlst[i];
			if(n->get_fg_type() == FG_NAV_VOR || n->get_fg_type() == FG_NAV_NDB) {	// We don't bother with ILS etc.
				GPSWaypoint* w = new GPSWaypoint;
				w->id = n->get_ident();
				w->lat = n->get_lat() * SG_DEGREES_TO_RADIANS;
				w->lon = n->get_lon() * SG_DEGREES_TO_RADIANS;
				w->type = (n->get_fg_type() == FG_NAV_VOR ? GPS_WP_VOR : GPS_WP_NDB);
				gps_waypoint_map_iterator wtr = _waypoints.find(n->get_ident());
				if(wtr == _waypoints.end()) {
					gps_waypoint_array arr;
					arr.push_back(w);
					_waypoints[w->id] = arr;
				} else {
					wtr->second.push_back(w);
				}
			}
		}
	}
	const fix_map_type* fixes = globals->get_fixlist()->getFixList();
	for(fix_map_const_iterator itr = fixes->begin(); itr != fixes->end(); ++itr) {
		FGFix f = itr->second;
		GPSWaypoint* w = new GPSWaypoint;
		w->id = f.get_ident();
		w->lat = f.get_lat() * SG_DEGREES_TO_RADIANS;
		w->lon = f.get_lon() * SG_DEGREES_TO_RADIANS;
		w->type = GPS_WP_INT;
		gps_waypoint_map_iterator wtr = _waypoints.find(f.get_ident());
		if(wtr == _waypoints.end()) {
			gps_waypoint_array arr;
			arr.push_back(w);
			_waypoints[w->id] = arr;
		} else {
			wtr->second.push_back(w);
		}
	}
	// TODO - add USR waypoints as well.
	
	// Not sure if this should be here, but OK for now.
	CreateDefaultFlightPlans();
	
	// Hack - hardwire some instrument approaches for testing.
	// TODO - read these from file - either all at startup or as needed.
	FGNPIAP* iap = new FGNPIAP;
	iap->_id = "KHWD";
	iap->_name = "VOR/DME OR GPS-B";
	iap->_abbrev = "VOR/D";
	iap->_rwyStr = "B";
	iap->_IAF.clear();
	iap->_IAP.clear();
	iap->_MAP.clear();
	// -------
	GPSWaypoint* wp = new GPSWaypoint;
	wp->id = "SUNOL";
	bool multi;
	// Nasty using the find any function here, but it saves converting data from FGFix etc. 
	const GPSWaypoint* fp = FindFirstById(wp->id, multi, true); 
	*wp = *fp;
	wp->appType = GPS_IAF;
	iap->_IAF.push_back(wp);
	// -------
	wp = new GPSWaypoint;
	wp->id = "MABRY";
	fp = FindFirstById(wp->id, multi, true); 
	*wp = *fp;
	wp->appType = GPS_IAF;
	iap->_IAF.push_back(wp);
	// -------
	wp = new GPSWaypoint;
	wp->id = "IMPLY";
	fp = FindFirstById(wp->id, multi, true); 
	*wp = *fp;
	wp->appType = GPS_IAP;
	iap->_IAP.push_back(wp);
	// -------
	wp = new GPSWaypoint;
	wp->id = "DECOT";
	fp = FindFirstById(wp->id, multi, true); 
	*wp = *fp;
	wp->appType = GPS_FAF;
	iap->_IAP.push_back(wp);
	// -------
	wp = new GPSWaypoint;
	wp->id = "MAPVV";
	fp = FindFirstById(wp->id, multi, true); 
	*wp = *fp;
	wp->appType = GPS_MAP;
	iap->_IAP.push_back(wp);
	// -------
	wp = new GPSWaypoint;
	wp->id = "OAK";
	fp = FindFirstById(wp->id, multi, true); 
	*wp = *fp;
	wp->appType = GPS_MAHP;
	iap->_MAP.push_back(wp);
	// -------
	_np_iap[iap->_id].push_back(iap);
	// -----------------------
	// -----------------------
	iap = new FGNPIAP;
	iap->_id = "KHWD";
	iap->_name = "VOR OR GPS-A";
	iap->_abbrev = "VOR-";
	iap->_rwyStr = "A";
	iap->_IAF.clear();
	iap->_IAP.clear();
	iap->_MAP.clear();
	// -------
	wp = new GPSWaypoint;
	wp->id = "SUNOL";
	// Nasty using the find any function here, but it saves converting data from FGFix etc. 
	fp = FindFirstById(wp->id, multi, true); 
	*wp = *fp;
	wp->appType = GPS_IAF;
	iap->_IAF.push_back(wp);
	// -------
	wp = new GPSWaypoint;
	wp->id = "MABRY";
	fp = FindFirstById(wp->id, multi, true); 
	*wp = *fp;
	wp->appType = GPS_IAF;
	iap->_IAF.push_back(wp);
	// -------
	wp = new GPSWaypoint;
	wp->id = "IMPLY";
	fp = FindFirstById(wp->id, multi, true); 
	*wp = *fp;
	wp->appType = GPS_IAP;
	iap->_IAP.push_back(wp);
	// -------
	wp = new GPSWaypoint;
	wp->id = "DECOT";
	fp = FindFirstById(wp->id, multi, true); 
	*wp = *fp;
	wp->appType = GPS_FAF;
	iap->_IAP.push_back(wp);
	// -------
	wp = new GPSWaypoint;
	wp->id = "MAPVV";
	fp = FindFirstById(wp->id, multi, true); 
	*wp = *fp;
	wp->appType = GPS_MAP;
	iap->_IAP.push_back(wp);
	// -------
	wp = new GPSWaypoint;
	wp->id = "OAK";
	fp = FindFirstById(wp->id, multi, true); 
	*wp = *fp;
	wp->appType = GPS_MAHP;
	iap->_MAP.push_back(wp);
	// -------
	_np_iap[iap->_id].push_back(iap);
	// ------------------
	// ------------------
	/*
	// Ugh - don't load this one - the waypoints required aren't in fix.dat.gz - result: program crash!
	// TODO - make the IAP loader robust to absent waypoints.
	iap = new FGNPIAP;
	iap->_id = "KHWD";
	iap->_name = "GPS RWY 28L";
	iap->_abbrev = "GPS";
	iap->_rwyStr = "28L";
	iap->_IAF.clear();
	iap->_IAP.clear();
	iap->_MAP.clear();
	// -------
	wp = new GPSWaypoint;
	wp->id = "SUNOL";
	// Nasty using the find any function here, but it saves converting data from FGFix etc. 
	fp = FindFirstById(wp->id, multi, true); 
	*wp = *fp;
	wp->appType = GPS_IAF;
	iap->_IAF.push_back(wp);
	// -------
	wp = new GPSWaypoint;
	wp->id = "SJC";
	fp = FindFirstById(wp->id, multi, true); 
	*wp = *fp;
	wp->appType = GPS_IAF;
	iap->_IAF.push_back(wp);
	// -------
	wp = new GPSWaypoint;
	wp->id = "JOCPI";
	fp = FindFirstById(wp->id, multi, true); 
	*wp = *fp;
	wp->appType = GPS_IAP;
	iap->_IAP.push_back(wp);
	// -------
	wp = new GPSWaypoint;
	wp->id = "SUDGE";
	fp = FindFirstById(wp->id, multi, true); 
	*wp = *fp;
	wp->appType = GPS_FAF;
	iap->_IAP.push_back(wp);
	// -------
	wp = new GPSWaypoint;
	wp->id = "RW28L";
	wp->appType = GPS_MAP;
	if(wp->id.substr(0, 2) == "RW" && wp->appType == GPS_MAP) {
		// Assume that this is a missed-approach point based on the runway number
		// Get the runway threshold location etc
	} else {
		fp = FindFirstById(wp->id, multi, true);
		if(fp == NULL) {
			cout << "Failed to find waypoint " << wp->id << " in database...\n";
		} else {
			*wp = *fp;
		}
	}
	iap->_IAP.push_back(wp);
	// -------
	wp = new GPSWaypoint;
	wp->id = "OAK";
	fp = FindFirstById(wp->id, multi, true); 
	*wp = *fp;
	wp->appType = GPS_MAHP;
	iap->_MAP.push_back(wp);
	// -------
	_np_iap[iap->_id].push_back(iap);
	*/
	iap = new FGNPIAP;
	iap->_id = "C83";
	iap->_name = "GPS RWY 30";
	iap->_abbrev = "GPS";
	iap->_rwyStr = "30";
	iap->_IAF.clear();
	iap->_IAP.clear();
	iap->_MAP.clear();
	// -------
	wp = new GPSWaypoint;
	wp->id = "PATYY";
	// Nasty using the find any function here, but it saves converting data from FGFix etc. 
	fp = FindFirstById(wp->id, multi, true); 
	*wp = *fp;
	wp->appType = GPS_IAF;
	iap->_IAF.push_back(wp);
	// -------
	wp = new GPSWaypoint;
	wp->id = "TRACY";
	fp = FindFirstById(wp->id, multi, true); 
	*wp = *fp;
	wp->appType = GPS_IAF;
	iap->_IAF.push_back(wp);
	// -------
	wp = new GPSWaypoint;
	wp->id = "TRACY";
	fp = FindFirstById(wp->id, multi, true); 
	*wp = *fp;
	wp->appType = GPS_IAP;
	iap->_IAP.push_back(wp);
	// -------
	wp = new GPSWaypoint;
	wp->id = "BABPI";
	fp = FindFirstById(wp->id, multi, true); 
	*wp = *fp;
	wp->appType = GPS_FAF;
	iap->_IAP.push_back(wp);
	// -------
	wp = new GPSWaypoint;
	wp->id = "AMOSY";
	wp->appType = GPS_MAP;
	if(wp->id.substr(0, 2) == "RW" && wp->appType == GPS_MAP) {
		// Assume that this is a missed-approach point based on the runway number
		// TODO: Get the runway threshold location etc
		cout << "TODO - implement missed-approach point based on rwy no.\n";
	} else {
		fp = FindFirstById(wp->id, multi, true);
		if(fp == NULL) {
			cout << "Failed to find waypoint " << wp->id << " in database...\n";
		} else {
			*wp = *fp;
			wp->appType = GPS_MAP;
		}
	}
	iap->_IAP.push_back(wp);
	// -------
	wp = new GPSWaypoint;
	wp->id = "HAIRE";
	fp = FindFirstById(wp->id, multi, true); 
	*wp = *fp;
	wp->appType = GPS_MAHP;
	iap->_MAP.push_back(wp);
	// -------
	_np_iap[iap->_id].push_back(iap);
}

void DCLGPS::bind() {
	fgTie("/instrumentation/gps/waypoint-alert", this, &DCLGPS::GetWaypointAlert);
	fgTie("/instrumentation/gps/leg-mode", this, &DCLGPS::GetLegMode);
	fgTie("/instrumentation/gps/obs-mode", this, &DCLGPS::GetOBSMode);
	fgTie("/instrumentation/gps/approach-arm", this, &DCLGPS::GetApproachArm);
	fgTie("/instrumentation/gps/approach-active", this, &DCLGPS::GetApproachActive);
	fgTie("/instrumentation/gps/cdi-deflection", this, &DCLGPS::GetCDIDeflection);
	fgTie("/instrumentation/gps/to-flag", this, &DCLGPS::GetToFlag);
}

void DCLGPS::unbind() {
	fgUntie("/instrumentation/gps/waypoint-alert");
	fgUntie("/instrumentation/gps/leg-mode");
	fgUntie("/instrumentation/gps/obs-mode");
	fgUntie("/instrumentation/gps/approach-arm");
	fgUntie("/instrumentation/gps/approach-active");
	fgUntie("/instrumentation/gps/cdi-deflection");
}

void DCLGPS::update(double dt) {
	//cout << "update called!\n";
	
	_lon = _lon_node->getDoubleValue() * SG_DEGREES_TO_RADIANS;
	_lat = _lat_node->getDoubleValue() * SG_DEGREES_TO_RADIANS;
	_alt = _alt_node->getDoubleValue();
	_groundSpeed_kts = _grnd_speed_node->getDoubleValue();
	_groundSpeed_ms = _groundSpeed_kts * 0.5144444444;
	_track = _true_track_node->getDoubleValue();
	_magTrackDeg = _mag_track_node->getDoubleValue();
	// Note - we can depriciate _gpsLat and _gpsLon if we implement error handling in FG
	// gps code and not our own.
	_gpsLon = _lon;
	_gpsLat = _lat;
	// Check for abnormal position slew
	if(GetGreatCircleDistance(_gpsLat, _gpsLon, _checkLat, _checkLon) > 1.0) {
		OrientateToActiveFlightPlan();
	}
	_checkLon = _gpsLon;
	_checkLat = _gpsLat;
	
	if(!_departed) {
		if(_groundSpeed_kts > 30.0) {
			_departed = true;
			string th = fgGetString("/instrumentation/clock/indicated-hour");
			string tm = fgGetString("/instrumentation/clock/indicated-min");
			if(th.size() == 1) th = "0" + th;
			if(tm.size() == 1) tm = "0" + tm;
			_departureTimeString = th + tm;
		}
	} else {
		// TODO - check - is this prone to drift error over time?
		// Should we difference the departure and current times?
		// What about when the user resets the time of day from the menu?
		_elapsedTime += dt;
	}

	_time->update(_gpsLon * SG_DEGREES_TO_RADIANS, _gpsLat * SG_DEGREES_TO_RADIANS, 0, 0);
	// FIXME - currently all the below assumes leg mode and no DTO or OBS cancelled.
	if(_activeFP->IsEmpty()) {
		// Not sure if we need to reset these each update or only when fp altered
		_activeWaypoint.id.clear();
		_navFlagged = true;
	} else if(_activeFP->waypoints.size() == 1) {
		_activeWaypoint.id.clear();
	} else {
		_navFlagged = false;
		if(_activeWaypoint.id.empty() || _fromWaypoint.id.empty()) {
			//cout << "Error, in leg mode with flightplan of 2 or more waypoints, but either active or from wp is NULL!\n";
			OrientateToActiveFlightPlan();
		}
		
		// Approach stuff
		if(_approachLoaded) {
			if(!_approachReallyArmed && !_approachActive) {
				// arm if within 30nm of airport.
				// TODO - let user cancel approach arm using external GPS-APR switch
				bool multi;
				const FGAirport* ap = FindFirstAptById(_approachID, multi, true);
				if(ap != NULL) {
					double d = GetGreatCircleDistance(_gpsLat, _gpsLon, ap->getLatitude() * SG_DEGREES_TO_RADIANS, ap->getLongitude() * SG_DEGREES_TO_RADIANS);
					if(d <= 30) {
						_approachArm = true;
						_approachReallyArmed = true;
						_messageStack.push_back("*Press ALT To Set Baro");
						// Not sure what we do if the user has already set CDI to 0.3 nm?
						_targetCdiScaleIndex = 1;
						if(_currentCdiScaleIndex == 1) {
							// No problem!
						} else if(_currentCdiScaleIndex == 0) {
							_sourceCdiScaleIndex = 0;
							_cdiScaleTransition = true;
							_cdiTransitionTime = 30.0;
							_currentCdiScale = _cdiScales[_currentCdiScaleIndex];
						}
					}
				}
			} else {
				// Check for approach active - we can only activate approach if it is really armed.
				if(_activeWaypoint.appType == GPS_FAF) {
					//cout << "Active waypoint is FAF, id is " << _activeWaypoint.id << '\n';
					if(GetGreatCircleDistance(_gpsLat, _gpsLon, _activeWaypoint.lat, _activeWaypoint.lon) <= 2.0 && !_obsMode) {
						// Assume heading is OK for now
						_approachArm = false;	// TODO - check - maybe arm is left on when actv comes on?
						_approachReallyArmed = false;
						_approachActive = true;
						_targetCdiScaleIndex = 2;
						if(_currentCdiScaleIndex == 2) {
							// No problem!
						} else if(_currentCdiScaleIndex == 1) {
							_sourceCdiScaleIndex = 1;
							_cdiScaleTransition = true;
							_cdiTransitionTime = 30.0;	// TODO - compress it if time to FAF < 30sec
							_currentCdiScale = _cdiScales[_currentCdiScaleIndex];
						} else {
							// Abort going active?
							_approachActive = false;
						}
					}
				}
			}
		}
		
		// CDI scale transition stuff
		if(_cdiScaleTransition) {
			if(fabs(_currentCdiScale - _cdiScales[_targetCdiScaleIndex]) < 0.001) {
				_currentCdiScale = _cdiScales[_targetCdiScaleIndex];
				_currentCdiScaleIndex = _targetCdiScaleIndex;
				_cdiScaleTransition = false;
			} else {
				double scaleDiff = (_targetCdiScaleIndex > _sourceCdiScaleIndex 
				                    ? _cdiScales[_sourceCdiScaleIndex] - _cdiScales[_targetCdiScaleIndex]
									: _cdiScales[_targetCdiScaleIndex] - _cdiScales[_sourceCdiScaleIndex]);
				//cout << "ScaleDiff = " << scaleDiff << '\n';
				if(_targetCdiScaleIndex > _sourceCdiScaleIndex) {
					// Scaling down eg. 5nm -> 1nm
					_currentCdiScale -= (scaleDiff * dt / _cdiTransitionTime);
					if(_currentCdiScale < _cdiScales[_targetCdiScaleIndex]) {
						_currentCdiScale = _cdiScales[_targetCdiScaleIndex];
						_currentCdiScaleIndex = _targetCdiScaleIndex;
						_cdiScaleTransition = false;
					}
				} else {
					_currentCdiScale += (scaleDiff * dt / _cdiTransitionTime);
					if(_currentCdiScale > _cdiScales[_targetCdiScaleIndex]) {
						_currentCdiScale = _cdiScales[_targetCdiScaleIndex];
						_currentCdiScaleIndex = _targetCdiScaleIndex;
						_cdiScaleTransition = false;
					}
				}
				//cout << "_currentCdiScale = " << _currentCdiScale << '\n';
			}
		} else {
			_currentCdiScale = _cdiScales[_currentCdiScaleIndex];
		}
		
		
		// Urgh - I've been setting the heading bug based on DTK,
		// bug I think it should be based on heading re. active waypoint
		// based on what the sim does after the final waypoint is passed.
		// (DTK remains the same, but if track is held == DTK heading bug
		// reverses to from once wp is passed).
		/*
		if(_fromWaypoint != NULL) {
			// TODO - how do we handle the change of track with distance over long legs?
			_dtkTrue = GetGreatCircleCourse(_fromWaypoint->lat, _fromWaypoint->lon, _activeWaypoint->lat, _activeWaypoint->lon) * SG_RADIANS_TO_DEGREES;
			_dtkMag = GetMagHeadingFromTo(_fromWaypoint->lat, _fromWaypoint->lon, _activeWaypoint->lat, _activeWaypoint->lon);
			// Don't change the heading bug if speed is too low otherwise it flickers to/from at rest
			if(_groundSpeed_ms > 5) {
				//cout << "track = " << _track << ", dtk = " << _dtkTrue << '\n'; 
				double courseDev = _track - _dtkTrue;
				//cout << "courseDev = " << courseDev << ", normalized = ";
				SG_NORMALIZE_RANGE(courseDev, -180.0, 180.0);
				//cout << courseDev << '\n';
				_headingBugTo = (fabs(courseDev) > 90.0 ? false : true);
			}
		} else {
			_dtkTrue = 0.0;
			_dtkMag = 0.0;
			// TODO - in DTO operation the position of initiation of DTO defines the "from waypoint".
		}
		*/
		if(!_activeWaypoint.id.empty()) {
			double hdgTrue = GetGreatCircleCourse(_gpsLat, _gpsLon, _activeWaypoint.lat, _activeWaypoint.lon) * SG_RADIANS_TO_DEGREES;
			if(_groundSpeed_ms > 5) {
				//cout << "track = " << _track << ", hdgTrue = " << hdgTrue << '\n'; 
				double courseDev = _track - hdgTrue;
				//cout << "courseDev = " << courseDev << ", normalized = ";
				SG_NORMALIZE_RANGE(courseDev, -180.0, 180.0);
				//cout << courseDev << '\n';
				_headingBugTo = (fabs(courseDev) > 90.0 ? false : true);
			}
			if(!_fromWaypoint.id.empty()) {
				_dtkTrue = GetGreatCircleCourse(_fromWaypoint.lat, _fromWaypoint.lon, _activeWaypoint.lat, _activeWaypoint.lon) * SG_RADIANS_TO_DEGREES;
				_dtkMag = GetMagHeadingFromTo(_fromWaypoint.lat, _fromWaypoint.lon, _activeWaypoint.lat, _activeWaypoint.lon);
			} else {
				_dtkTrue = 0.0;
				_dtkMag = 0.0;
			}
		}
		
		_dist2Act = GetGreatCircleDistance(_gpsLat, _gpsLon, _activeWaypoint.lat, _activeWaypoint.lon) * SG_NM_TO_METER;
		if(_groundSpeed_ms > 10.0) {
			_eta = _dist2Act / _groundSpeed_ms;
			if(_eta <= 36) {	// TODO - this is slightly different if turn anticipation is enabled.
				if(_headingBugTo) {
					_waypointAlert = true;	// TODO - not if the from flag is set.
				}
			}
			if(_eta < 60) {
				// Check if we should sequence to next leg.
				// Perhaps this should be done on distance instead, but 60s time (about 1 - 2 nm) seems reasonable for now.
				//double reverseHeading = GetGreatCircleCourse(_activeWaypoint->lat, _activeWaypoint->lon, _fromWaypoint->lat, _fromWaypoint->lon);
				// Hack - let's cheat and do it on heading bug for now.  TODO - that stops us 'cutting the corner'
				// when we happen to approach the inside turn of a waypoint - we should probably sequence at the midpoint
				// of the heading difference between legs in this instance.
				int idx = GetActiveWaypointIndex();
				bool finalLeg = (idx == (int)(_activeFP->waypoints.size()) - 1 ? true : false);
				bool finalDto = (_dto && idx == -1);	// Dto operation to a waypoint not in the flightplan - we don't sequence in this instance
				if(!_headingBugTo) {
					if(finalLeg) {
						// Do nothing - not sure if Dto should switch off when arriving at the final waypoint of a flightplan
					} else if(finalDto) {
						// Do nothing
					} else if(_activeWaypoint.appType == GPS_MAP) {
						// Don't sequence beyond the missed approach point
						cout << "ACTIVE WAYPOINT is MAP - not sequencing!!!!!\n";
					} else {
						cout << "Sequencing...\n";
						_fromWaypoint = _activeWaypoint;
						_activeWaypoint = *_activeFP->waypoints[idx + 1];
						_dto = false;
						// TODO - course alteration message format is dependent on whether we are slaved HSI/CDI indicator or not.
						// For now assume we are not.
						string s;
						if(fgGetBool("/instrumentation/nav[0]/slaved-to-gps")) {
							// TODO - avoid the hardwiring on nav[0]
							s = "Adj Nav Crs to ";
						} else {
							string s = "GPS Course is ";
						}
						double d = GetMagHeadingFromTo(_fromWaypoint.lat, _fromWaypoint.lon, _activeWaypoint.lat, _activeWaypoint.lon);
						while(d < 0.0) d += 360.0;
						while(d >= 360.0) d -= 360.0;
						char buf[4];
						snprintf(buf, 4, "%03i", (int)(d + 0.5));
						s += buf;
						_messageStack.push_back(s);
					}
					_waypointAlert = false;
				}
			}
		} else {
			_eta = 0.0;
		}
		
		/*
		// First attempt at a sensible cross-track correction calculation
		// Uh? - I think this is implemented further down the file!
		if(_fromWaypoint != NULL) {
				
		} else {
			_crosstrackDist = 0.0;
		}
		*/
	}
}

double DCLGPS::GetCDIDeflection() const {
	double xtd = CalcCrossTrackDeviation();	//nm
	return((xtd / _currentCdiScale) * 5.0 * 2.5 * -1.0);
}

void DCLGPS::DtoInitiate(const string& s) {
	//cout << "DtoInitiate, s = " << s << '\n';
	bool multi;
	const GPSWaypoint* wp = FindFirstById(s, multi, true);
	if(wp) {
		//cout << "Waypoint found, starting dto operation!\n";
		_dto = true;
		_activeWaypoint = *wp;
		_fromWaypoint.lat = _gpsLat;
		_fromWaypoint.lon = _gpsLon;
		_fromWaypoint.type = GPS_WP_VIRT;
		_fromWaypoint.id = "DTOWP";
	} else {
		//cout << "Waypoint not found, ignoring dto request\n";
		// Should bring up the user waypoint page, but we're not implementing that yet.
		_dto = false;	// TODO - implement this some day.
	}
}

void DCLGPS::DtoCancel() {
	if(_dto) {
		// i.e. don't bother reorientating if we're just cancelling a DTO button press
		// without having previously initiated DTO.
		OrientateToActiveFlightPlan();
	}
	_dto = false;
}

void DCLGPS::Knob1Left1() {}
void DCLGPS::Knob1Right1() {}
void DCLGPS::Knob2Left1() {}
void DCLGPS::Knob2Right1() {}
void DCLGPS::CrsrPressed() { _activePage->CrsrPressed(); }
void DCLGPS::EntPressed() { _activePage->EntPressed(); }
void DCLGPS::ClrPressed() { _activePage->ClrPressed(); }
void DCLGPS::DtoPressed() {}
void DCLGPS::NrstPressed() {}
void DCLGPS::AltPressed() {}

void DCLGPS::OBSPressed() { 
	_obsMode = !_obsMode;
	if(_obsMode) {
		if(!_activeWaypoint.id.empty()) {
			_obsHeading = static_cast<int>(_dtkMag);
		}
		// TODO - the _fromWaypoint location will change as the OBS heading changes.
		// Might need to store the OBS initiation position somewhere in case it is needed again.
		SetOBSFromWaypoint();
	}
}

// Set the _fromWaypoint position based on the active waypoint and OBS radial.
void DCLGPS::SetOBSFromWaypoint() {
	if(!_obsMode) return;
	if(_activeWaypoint.id.empty()) return;
	
	// TODO - base the 180 deg correction on the to/from flag.
	_fromWaypoint = GetPositionOnMagRadial(_activeWaypoint, 10, _obsHeading + 180.0);
	_fromWaypoint.id = "OBSWP";
}

void DCLGPS::MsgPressed() {}

void DCLGPS::CDIFSDIncrease() {
	if(_currentCdiScaleIndex == 0) {
		_currentCdiScaleIndex = _cdiScales.size() - 1;
	} else {
		_currentCdiScaleIndex--;
	}
}

void DCLGPS::CDIFSDDecrease() {
	_currentCdiScaleIndex++;
	if(_currentCdiScaleIndex == _cdiScales.size()) {
		_currentCdiScaleIndex = 0;
	}
}

void DCLGPS::DrawChar(char c, int field, int px, int py, bool bold) {
}

void DCLGPS::DrawText(const string& s, int field, int px, int py, bool bold) {
}

void DCLGPS::SetBaroUnits(int n, bool wrap) {
	if(n < 1) {
		_baroUnits = (GPSPressureUnits)(wrap ? 3 : 1);
	} else if(n > 3) {
		_baroUnits = (GPSPressureUnits)(wrap ? 1 : 3);
	} else {
		_baroUnits = (GPSPressureUnits)n;
	}
}

void DCLGPS::CreateDefaultFlightPlans() {}

// Get the time to the active waypoint in seconds.
// Returns -1 if groundspeed < 30 kts
double DCLGPS::GetTimeToActiveWaypoint() {
	if(_groundSpeed_kts < 30.0) {
		return(-1.0);
	} else {
		return(_eta);
	}
}

// Get the time to the final waypoint in seconds.
// Returns -1 if groundspeed < 30 kts
double DCLGPS::GetETE() {
	if(_groundSpeed_kts < 30.0) {
		return(-1.0);
	} else {
		// TODO - handle OBS / DTO operation appropriately
		if(_activeFP->waypoints.empty()) {
			return(-1.0);
		} else {
			return(GetTimeToWaypoint(_activeFP->waypoints[_activeFP->waypoints.size() - 1]->id));
		}
	}
}

// Get the time to a given waypoint (spec'd by ID) in seconds.
// returns -1 if groundspeed is less than 30kts.
// If the waypoint is an unreached part of the active flight plan the time will be via each leg.
// otherwise it will be a direct-to time.
double DCLGPS::GetTimeToWaypoint(const string& id) {
	if(_groundSpeed_kts < 30.0) {
		return(-1.0);
	}
	
	double eta = 0.0;
	int n1 = GetActiveWaypointIndex();
	int n2 = GetWaypointIndex(id);
	if(n2 > n1) {
		eta = _eta;
		for(unsigned int i=n1+1; i<_activeFP->waypoints.size(); ++i) {
			GPSWaypoint* wp1 = _activeFP->waypoints[i-1];
			GPSWaypoint* wp2 = _activeFP->waypoints[i];
			double distm = GetGreatCircleDistance(wp1->lat, wp1->lon, wp2->lat, wp2->lon) * SG_NM_TO_METER;
			eta += (distm / _groundSpeed_ms);
		}
		return(eta);
	} else if(id == _activeWaypoint.id) {
		return(_eta);
	} else {
		bool multi;
		const GPSWaypoint* wp = FindFirstById(id, multi, true);
		if(wp == NULL) return(-1.0);
		double distm = GetGreatCircleDistance(_gpsLat, _gpsLon, wp->lat, wp->lon);
		return(distm / _groundSpeed_ms);
	}
	return(-1.0);	// Hopefully we never get here!
}

// Returns magnetic great-circle heading
// TODO - document units.
float DCLGPS::GetHeadingToActiveWaypoint() {
	if(_activeWaypoint.id.empty()) {
		return(-888);
	} else {
		double h = GetMagHeadingFromTo(_gpsLat, _gpsLon, _activeWaypoint.lat, _activeWaypoint.lon);
		while(h <= 0.0) h += 360.0;
		while(h > 360.0) h -= 360.0;
		return((float)h);
	}
}

// Returns magnetic great-circle heading
// TODO - what units?
float DCLGPS::GetHeadingFromActiveWaypoint() {
	if(_activeWaypoint.id.empty()) {
		return(-888);
	} else {
		double h = GetMagHeadingFromTo(_activeWaypoint.lat, _activeWaypoint.lon, _gpsLat, _gpsLon);
		while(h <= 0.0) h += 360.0;
		while(h > 360.0) h -= 360.0;
		return(h);
	}
}

void DCLGPS::ClearFlightPlan(int n) {
	for(unsigned int i=0; i<_flightPlans[n]->waypoints.size(); ++i) {
		delete _flightPlans[n]->waypoints[i];
	}
	_flightPlans[n]->waypoints.clear();
}

void DCLGPS::ClearFlightPlan(GPSFlightPlan* fp) {
	for(unsigned int i=0; i<fp->waypoints.size(); ++i) {
		delete fp->waypoints[i];
	}
	fp->waypoints.clear();
}

int DCLGPS::GetActiveWaypointIndex() {
	for(unsigned int i=0; i<_flightPlans[0]->waypoints.size(); ++i) {
		if(_flightPlans[0]->waypoints[i]->id == _activeWaypoint.id) return((int)i);
	}
	return(-1);
}

int DCLGPS::GetWaypointIndex(const string& id) {
	for(unsigned int i=0; i<_flightPlans[0]->waypoints.size(); ++i) {
		if(_flightPlans[0]->waypoints[i]->id == id) return((int)i);
	}
	return(-1);
}

void DCLGPS::OrientateToFlightPlan(GPSFlightPlan* fp) {
	//cout << "Orientating...\n";
	//cout << "_lat = " << _lat << ", _lon = " << _lon << ", _gpsLat = " << _gpsLat << ", gpsLon = " << _gpsLon << '\n'; 
	if(fp->IsEmpty()) {
		_activeWaypoint.id.clear();
		_navFlagged = true;
	} else {
		_navFlagged = false;
		if(fp->waypoints.size() == 1) {
			// TODO - may need to flag nav here if not dto or obs, or possibly handle it somewhere else.
			_activeWaypoint = *fp->waypoints[0];
			_fromWaypoint.id.clear();
		} else {
			// FIXME FIXME FIXME
			_fromWaypoint = *fp->waypoints[0];
			_activeWaypoint = *fp->waypoints[1];
			double dmin = 1000000;	// nm!!
			// For now we will simply start on the leg closest to our current position.
			// It's possible that more fancy algorithms may take either heading or track
			// into account when setting inital leg - I'm not sure.
			// This method should handle most cases perfectly OK though.
			for(unsigned int i = 1; i < fp->waypoints.size(); ++i) {
				//cout << "Pass " << i << ", dmin = " << dmin << ", leg is " << fp->waypoints[i-1]->id << " to " << fp->waypoints[i]->id << '\n';
				// First get the cross track correction.
				double d0 = fabs(CalcCrossTrackDeviation(*fp->waypoints[i-1], *fp->waypoints[i]));
				// That is the shortest distance away we could be though - check for
				// longer distances if we are 'off the end' of the leg.
				double ht1 = GetGreatCircleCourse(fp->waypoints[i-1]->lat, fp->waypoints[i-1]->lon, 
				                                  fp->waypoints[i]->lat, fp->waypoints[i]->lon) 
												  * SG_RADIANS_TO_DEGREES;
				// not simply the reverse of the above due to great circle navigation.
				double ht2 = GetGreatCircleCourse(fp->waypoints[i]->lat, fp->waypoints[i]->lon, 
				                                  fp->waypoints[i-1]->lat, fp->waypoints[i-1]->lon) 
												  * SG_RADIANS_TO_DEGREES;
				double hw1 = GetGreatCircleCourse(_gpsLat, _gpsLon,
				                                  fp->waypoints[i]->lat, fp->waypoints[i]->lon) 
												  * SG_RADIANS_TO_DEGREES;
				double hw2 = GetGreatCircleCourse(_gpsLat, _gpsLon, 
				                                  fp->waypoints[i-1]->lat, fp->waypoints[i-1]->lon) 
												  * SG_RADIANS_TO_DEGREES;
				double h1 = ht1 - hw1;
				double h2 = ht2 - hw2;
				//cout << "d0, h1, h2 = " << d0 << ", " << h1 << ", " << h2 << '\n';
				//cout << "Normalizing...\n";
				SG_NORMALIZE_RANGE(h1, -180.0, 180.0);
				SG_NORMALIZE_RANGE(h2, -180.0, 180.0);
				//cout << "d0, h1, h2 = " << d0 << ", " << h1 << ", " << h2 << '\n';
				if(fabs(h1) > 90.0) {
					// We are past the end of the to waypoint
					double d = GetGreatCircleDistance(_gpsLat, _gpsLon, fp->waypoints[i]->lat, fp->waypoints[i]->lon);
					if(d > d0) d0 = d;
					//cout << "h1 triggered, d0 now = " << d0 << '\n';
				} else if(fabs(h2) > 90.0) {
					// We are past the end (not yet at!) the from waypoint
					double d = GetGreatCircleDistance(_gpsLat, _gpsLon, fp->waypoints[i-1]->lat, fp->waypoints[i-1]->lon);
					if(d > d0) d0 = d;
					//cout << "h2 triggered, d0 now = " << d0 << '\n';
				}
				if(d0 < dmin) {
					//cout << "THIS LEG NOW ACTIVE!\n";
					dmin = d0;
					_fromWaypoint = *fp->waypoints[i-1];
					_activeWaypoint = *fp->waypoints[i];
				}
			}
		}
	}
}

void DCLGPS::OrientateToActiveFlightPlan() {
	OrientateToFlightPlan(_activeFP);
}	

/***************************************/

// Utility function - create a flightplan from a list of waypoint ids and types
void DCLGPS::CreateFlightPlan(GPSFlightPlan* fp, vector<string> ids, vector<GPSWpType> wps) {
	if(fp == NULL) fp = new GPSFlightPlan;
	unsigned int i;
	if(!fp->waypoints.empty()) {
		for(i=0; i<fp->waypoints.size(); ++i) {
			delete fp->waypoints[i];
		}
		fp->waypoints.clear();
	}
	if(ids.size() != wps.size()) {
		cout << "ID and Waypoint types list size mismatch in GPS::CreateFlightPlan - no flightplan created!\n";
		return;
	}
	for(i=0; i<ids.size(); ++i) {
		bool multi;
		const FGAirport* ap;
		FGNavRecord* np;
		GPSWaypoint* wp = new GPSWaypoint;
		wp->type = wps[i];
		switch(wp->type) {
		case GPS_WP_APT:
			ap = FindFirstAptById(ids[i], multi, true);
			if(ap == NULL) {
				// error
				delete wp;
			} else {
				wp->lat = ap->getLatitude() * SG_DEGREES_TO_RADIANS;
				wp->lon = ap->getLongitude() * SG_DEGREES_TO_RADIANS;
				wp->id = ids[i];
				fp->waypoints.push_back(wp);
			}
			break;
		case GPS_WP_VOR:
			np = FindFirstVorById(ids[i], multi, true);
			if(np == NULL) {
				// error
				delete wp;
			} else {
				wp->lat = np->get_lat() * SG_DEGREES_TO_RADIANS;
				wp->lon = np->get_lon() * SG_DEGREES_TO_RADIANS;
				wp->id = ids[i];
				fp->waypoints.push_back(wp);
			}
			break;
		case GPS_WP_NDB:
			np = FindFirstNDBById(ids[i], multi, true);
			if(np == NULL) {
				// error
				delete wp;
			} else {
				wp->lat = np->get_lat() * SG_DEGREES_TO_RADIANS;
				wp->lon = np->get_lon() * SG_DEGREES_TO_RADIANS;
				wp->id = ids[i];
				fp->waypoints.push_back(wp);
			}
			break;
		case GPS_WP_INT:
			// TODO TODO
			break;
		case GPS_WP_USR:
			// TODO
			break;
		}
	}
}

/***************************************/

const GPSWaypoint* DCLGPS::ActualFindFirstById(const string& id, bool exact) {
    gps_waypoint_map_const_iterator itr;
    if(exact) {
        itr = _waypoints.find(id);
    } else {
        itr = _waypoints.lower_bound(id);
    }
    if(itr == _waypoints.end()) {
        return(NULL);
    } else {
		// TODO - don't just return the first one - either return all or the nearest one.
        return((itr->second)[0]);
    }
}

const GPSWaypoint* DCLGPS::FindFirstById(const string& id, bool &multi, bool exact) {
	multi = false;
	if(exact) return(ActualFindFirstById(id, exact));
	
	// OK, that was the easy case, now the fuzzy case
	const GPSWaypoint* w1 = ActualFindFirstById(id);
	if(w1 == NULL) return(w1);
	
	// The non-trivial code from here to the end of the function is all to deal with the fact that
	// the KLN89 alphabetical order (numbers AFTER letters) differs from ASCII order (numbers BEFORE letters).
	string id2 = id;
	//string id3 = id+'0';
	string id4 = id+'A';
	// Increment the last char to provide the boundary.  Note that 'Z' -> '[' but we also need to check '0' for all since GPS has numbers after letters
	//bool alfa = isalpha(id2[id2.size() - 1]);
	id2[id2.size() - 1] = id2[id2.size() - 1] + 1;
	const GPSWaypoint* w2 = ActualFindFirstById(id2);
	//FGAirport* a3 = globals->get_airports()->findFirstById(id3);
	const GPSWaypoint* w4 = ActualFindFirstById(id4);
	//cout << "Strings sent were " << id << ", " << id2 << " and " << id4 << '\n';
	//cout << "Airports returned were (a1, a2, a4): " << a1->getId() << ", " << a2->getId() << ", " << a4->getId() << '\n';
	//cout << "Pointers were " << a1 << ", " << a2 << ", " << a4 << '\n';
	
	// TODO - the below handles the imediately following char OK
	// eg id = "KD" returns "KDAA" instead of "KD5"
	// but it doesn't handle numbers / letters further down the string,
	// eg - id = "I" returns "IA01" instead of "IAN"
	// We either need to provide a custom comparison operator, or recurse this function if !isalpha further down the string.
	// (Currenly fixed with recursion).
	
	if(w4 != w2) { // A-Z match - preferred
		//cout << "A-Z match!\n";
		if(w4->id.size() - id.size() > 2) {
			// Check for numbers further on
			for(unsigned int i=id.size(); i<w4->id.size(); ++i) {
				if(!isalpha(w4->id[i])) {
					//cout << "SUBSTR is " << (a4->getId()).substr(0, i) << '\n';
					return(FindFirstById(w4->id.substr(0, i), multi, exact));
				}
			}
		}
		return(w4);
	} else if(w1 != w2) {  // 0-9 match
		//cout << "0-9 match!\n";
		if(w1->id.size() - id.size() > 2) {
			// Check for numbers further on
			for(unsigned int i=id.size(); i<w1->id.size(); ++i) {
				if(!isalpha(w1->id[i])) {
					//cout << "SUBSTR2 is " << (a4->getId()).substr(0, i) << '\n';
					return(FindFirstById(w1->id.substr(0, i), multi, exact));
				}
			}
		}
		return(w1);
	} else {  // No match
		return(NULL);
	}
	return NULL;
}

// Host specific lookup functions
// TODO - add the ASCII / alphabetical stuff from the Atlas version
FGNavRecord* DCLGPS::FindFirstVorById(const string& id, bool &multi, bool exact) {
	// NOTE - at the moment multi is never set.
	multi = false;
	//if(exact) return(_overlays->FindFirstVorById(id, exact));
	
	nav_list_type nav = globals->get_navlist()->findFirstByIdent(id, FG_NAV_VOR, exact);
	
	if(nav.size() > 1) multi = true;
	//return(nav.empty() ? NULL : *(nav.begin()));
	
	// The above is sort of what we want - unfortunately we can't guarantee no NDB/ILS at the moment
	if(nav.empty()) return(NULL);
	
	for(nav_list_iterator it = nav.begin(); it != nav.end(); ++it) {
		if((*it)->get_type() == 3) return(*it);
	}
	return(NULL);	// Shouldn't get here!
}
#if 0
Overlays::NAV* DCLGPS::FindFirstVorById(const string& id, bool &multi, bool exact) {
	// NOTE - at the moment multi is never set.
	multi = false;
	if(exact) return(_overlays->FindFirstVorById(id, exact));
	
	// OK, that was the easy case, now the fuzzy case
	Overlays::NAV* n1 = _overlays->FindFirstVorById(id);
	if(n1 == NULL) return(n1);
	
	string id2 = id;
	string id3 = id+'0';
	string id4 = id+'A';
	// Increment the last char to provide the boundary.  Note that 'Z' -> '[' but we also need to check '0' for all since GPS has numbers after letters
	bool alfa = isalpha(id2[id2.size() - 1]);
	id2[id2.size() - 1] = id2[id2.size() - 1] + 1;
	Overlays::NAV* n2 = _overlays->FindFirstVorById(id2);
	//Overlays::NAV* n3 = _overlays->FindFirstVorById(id3);
	//Overlays::NAV* n4 = _overlays->FindFirstVorById(id4);
	//cout << "Strings sent were " << id << ", " << id2 << ", " << id3 << ", " << id4 << '\n';
	
	
	if(alfa) {
		if(n1 != n2) { // match
			return(n1);
		} else {
			return(NULL);
		}
	}
	
	/*
	if(n1 != n2) {
		// Something matches - the problem is the number/letter preference order is reversed between the GPS and the STL
		if(n4 != n2) {
			// There's a letter match - return that
			return(n4);
		} else {
			// By definition we must have a number match
			if(n3 == n2) cout << "HELP - LOGIC FLAW in find VOR!\n";
			return(n3);
		}
	} else {
		// No match
		return(NULL);
	}
	*/
	return NULL;
}
#endif //0

// TODO - add the ASCII / alphabetical stuff from the Atlas version
FGNavRecord* DCLGPS::FindFirstNDBById(const string& id, bool &multi, bool exact) {
	// NOTE - at the moment multi is never set.
	multi = false;
	//if(exact) return(_overlays->FindFirstVorById(id, exact));
	
	nav_list_type nav = globals->get_navlist()->findFirstByIdent(id, FG_NAV_NDB, exact);
	
	if(nav.size() > 1) multi = true;
	//return(nav.empty() ? NULL : *(nav.begin()));
	
	// The above is sort of what we want - unfortunately we can't guarantee no NDB/ILS at the moment
	if(nav.empty()) return(NULL);
	
	for(nav_list_iterator it = nav.begin(); it != nav.end(); ++it) {
		if((*it)->get_type() == 2) return(*it);
	}
	return(NULL);	// Shouldn't get here!
}
#if 0
Overlays::NAV* DCLGPS::FindFirstNDBById(const string& id, bool &multi, bool exact) {
	// NOTE - at the moment multi is never set.
	multi = false;
	if(exact) return(_overlays->FindFirstNDBById(id, exact));
	
	// OK, that was the easy case, now the fuzzy case
	Overlays::NAV* n1 = _overlays->FindFirstNDBById(id);
	if(n1 == NULL) return(n1);
	
	string id2 = id;
	string id3 = id+'0';
	string id4 = id+'A';
	// Increment the last char to provide the boundary.  Note that 'Z' -> '[' but we also need to check '0' for all since GPS has numbers after letters
	bool alfa = isalpha(id2[id2.size() - 1]);
	id2[id2.size() - 1] = id2[id2.size() - 1] + 1;
	Overlays::NAV* n2 = _overlays->FindFirstNDBById(id2);
	//Overlays::NAV* n3 = _overlays->FindFirstNDBById(id3);
	//Overlays::NAV* n4 = _overlays->FindFirstNDBById(id4);
	//cout << "Strings sent were " << id << ", " << id2 << ", " << id3 << ", " << id4 << '\n';
	
	
	if(alfa) {
		if(n1 != n2) { // match
			return(n1);
		} else {
			return(NULL);
		}
	}
	
	/*
	if(n1 != n2) {
		// Something matches - the problem is the number/letter preference order is reversed between the GPS and the STL
		if(n4 != n2) {
			// There's a letter match - return that
			return(n4);
		} else {
			// By definition we must have a number match
			if(n3 == n2) cout << "HELP - LOGIC FLAW in find VOR!\n";
			return(n3);
		}
	} else {
		// No match
		return(NULL);
	}
	*/
	return NULL;
}
#endif //0

// TODO - add the ASCII / alphabetical stuff from the Atlas version
const FGFix* DCLGPS::FindFirstIntById(const string& id, bool &multi, bool exact) {
	// NOTE - at the moment multi is never set, and indeed can't be
	// since FG can only map one Fix per ID at the moment.
	multi = false;
	if(exact) return(globals->get_fixlist()->findFirstByIdent(id, exact));
	
	const FGFix* f1 = globals->get_fixlist()->findFirstByIdent(id, exact);
	if(f1 == NULL) return(f1);
	
	// The non-trivial code from here to the end of the function is all to deal with the fact that
	// the KLN89 alphabetical order (numbers AFTER letters) differs from ASCII order (numbers BEFORE letters).
	// It is copied from the airport version which is definately needed, but at present I'm not actually 
	// sure if any fixes in FG or real-life have numbers in them!
	string id2 = id;
	//string id3 = id+'0';
	string id4 = id+'A';
	// Increment the last char to provide the boundary.  Note that 'Z' -> '[' but we also need to check '0' for all since GPS has numbers after letters
	//bool alfa = isalpha(id2[id2.size() - 1]);
	id2[id2.size() - 1] = id2[id2.size() - 1] + 1;
	const FGFix* f2 = globals->get_fixlist()->findFirstByIdent(id2);
	//const FGFix* a3 = globals->get_fixlist()->findFirstByIdent(id3);
	const FGFix* f4 = globals->get_fixlist()->findFirstByIdent(id4);
	
	// TODO - the below handles the imediately following char OK
	// eg id = "KD" returns "KDAA" instead of "KD5"
	// but it doesn't handle numbers / letters further down the string,
	// eg - id = "I" returns "IA01" instead of "IAN"
	// We either need to provide a custom comparison operator, or recurse this function if !isalpha further down the string.
	// (Currenly fixed with recursion).
	
	if(f4 != f2) { // A-Z match - preferred
		//cout << "A-Z match!\n";
		if(f4->get_ident().size() - id.size() > 2) {
			// Check for numbers further on
			for(unsigned int i=id.size(); i<f4->get_ident().size(); ++i) {
				if(!isalpha(f4->get_ident()[i])) {
					//cout << "SUBSTR is " << (a4->getId()).substr(0, i) << '\n';
					return(FindFirstIntById(f4->get_ident().substr(0, i), multi, exact));
				}
			}
		}
		return(f4);
	} else if(f1 != f2) {  // 0-9 match
		//cout << "0-9 match!\n";
		if(f1->get_ident().size() - id.size() > 2) {
			// Check for numbers further on
			for(unsigned int i=id.size(); i<f1->get_ident().size(); ++i) {
				if(!isalpha(f1->get_ident()[i])) {
					//cout << "SUBSTR2 is " << (a4->getId()).substr(0, i) << '\n';
					return(FindFirstIntById(f1->get_ident().substr(0, i), multi, exact));
				}
			}
		}
		return(f1);
	} else {  // No match
		return(NULL);
	}
		
	return NULL;	// Don't think we can ever get here.
}

const FGAirport* DCLGPS::FindFirstAptById(const string& id, bool &multi, bool exact) {
	// NOTE - at the moment multi is never set.
	//cout << "FindFirstAptById, id = " << id << '\n';
	multi = false;
	if(exact) return(globals->get_airports()->findFirstById(id, exact));
	
	// OK, that was the easy case, now the fuzzy case
	const FGAirport* a1 = globals->get_airports()->findFirstById(id);
	if(a1 == NULL) return(a1);
	
	// The non-trivial code from here to the end of the function is all to deal with the fact that
	// the KLN89 alphabetical order (numbers AFTER letters) differs from ASCII order (numbers BEFORE letters).
	string id2 = id;
	//string id3 = id+'0';
	string id4 = id+'A';
	// Increment the last char to provide the boundary.  Note that 'Z' -> '[' but we also need to check '0' for all since GPS has numbers after letters
	//bool alfa = isalpha(id2[id2.size() - 1]);
	id2[id2.size() - 1] = id2[id2.size() - 1] + 1;
	const FGAirport* a2 = globals->get_airports()->findFirstById(id2);
	//FGAirport* a3 = globals->get_airports()->findFirstById(id3);
	const FGAirport* a4 = globals->get_airports()->findFirstById(id4);
	//cout << "Strings sent were " << id << ", " << id2 << " and " << id4 << '\n';
	//cout << "Airports returned were (a1, a2, a4): " << a1->getId() << ", " << a2->getId() << ", " << a4->getId() << '\n';
	//cout << "Pointers were " << a1 << ", " << a2 << ", " << a4 << '\n';
	
	// TODO - the below handles the imediately following char OK
	// eg id = "KD" returns "KDAA" instead of "KD5"
	// but it doesn't handle numbers / letters further down the string,
	// eg - id = "I" returns "IA01" instead of "IAN"
	// We either need to provide a custom comparison operator, or recurse this function if !isalpha further down the string.
	// (Currenly fixed with recursion).
	
	if(a4 != a2) { // A-Z match - preferred
		//cout << "A-Z match!\n";
		if(a4->getId().size() - id.size() > 2) {
			// Check for numbers further on
			for(unsigned int i=id.size(); i<a4->getId().size(); ++i) {
				if(!isalpha(a4->getId()[i])) {
					//cout << "SUBSTR is " << (a4->getId()).substr(0, i) << '\n';
					return(FindFirstAptById(a4->getId().substr(0, i), multi, exact));
				}
			}
		}
		return(a4);
	} else if(a1 != a2) {  // 0-9 match
		//cout << "0-9 match!\n";
		if(a1->getId().size() - id.size() > 2) {
			// Check for numbers further on
			for(unsigned int i=id.size(); i<a1->getId().size(); ++i) {
				if(!isalpha(a1->getId()[i])) {
					//cout << "SUBSTR2 is " << (a4->getId()).substr(0, i) << '\n';
					return(FindFirstAptById(a1->getId().substr(0, i), multi, exact));
				}
			}
		}
		return(a1);
	} else {  // No match
		return(NULL);
	}
		
	return NULL;
}

FGNavRecord* DCLGPS::FindClosestVor(double lat_rad, double lon_rad) {
	return(globals->get_navlist()->findClosest(lon_rad, lat_rad, 0.0, FG_NAV_VOR));
}

//----------------------------------------------------------------------------------------------------------

// Takes lat and lon in RADIANS!!!!!!!
double DCLGPS::GetMagHeadingFromTo(double latA, double lonA, double latB, double lonB) {
	double h = GetGreatCircleCourse(latA, lonA, latB, lonB);
	h *= SG_RADIANS_TO_DEGREES;
	// TODO - use the real altitude below instead of 0.0!
	//cout << "MagVar = " << sgGetMagVar(_gpsLon, _gpsLat, 0.0, _time->getJD()) * SG_RADIANS_TO_DEGREES << '\n';
	h -= sgGetMagVar(_gpsLon, _gpsLat, 0.0, _time->getJD()) * SG_RADIANS_TO_DEGREES;
	while(h >= 360.0) h -= 360.0;
	while(h < 0.0) h += 360.0;
	return(h);
}

// ---------------- Great Circle formulae from "The Aviation Formulary" -------------
// Note that all of these assume that the world is spherical.

double Rad2Nm(double theta) {
	return(((180.0*60.0)/SG_PI)*theta);
}

double Nm2Rad(double d) {
	return((SG_PI/(180.0*60.0))*d);
}

/* QUOTE:

The great circle distance d between two points with coordinates {lat1,lon1} and {lat2,lon2} is given by:

d=acos(sin(lat1)*sin(lat2)+cos(lat1)*cos(lat2)*cos(lon1-lon2))

A mathematically equivalent formula, which is less subject to rounding error for short distances is:

d=2*asin(sqrt((sin((lat1-lat2)/2))^2 + 
                 cos(lat1)*cos(lat2)*(sin((lon1-lon2)/2))^2))
				 
*/

// Returns distance in nm, takes lat & lon in RADIANS
double DCLGPS::GetGreatCircleDistance(double lat1, double lon1, double lat2, double lon2) const {
	double d = 2.0 * asin(sqrt(((sin((lat1-lat2)/2.0))*(sin((lat1-lat2)/2.0))) +
	           cos(lat1)*cos(lat2)*(sin((lon1-lon2)/2.0))*(sin((lon1-lon2)/2.0))));
	return(Rad2Nm(d));
}

// fmod dosen't do what we want :-( 
static double mod(double d1, double d2) {
	return(d1 - d2*floor(d1/d2));
}

// Returns great circle course from point 1 to point 2
// Input and output in RADIANS.
double DCLGPS::GetGreatCircleCourse (double lat1, double lon1, double lat2, double lon2) const {
	//double h = 0.0;
	/*
	// Special case the poles
	if(cos(lat1) < SG_EPSILON) {
		if(lat1 > 0) {
			// Starting from North Pole
			h = SG_PI;
		} else {
			// Starting from South Pole
			h = 2.0 * SG_PI;
		}
	} else {
		// Urgh - the formula below is for negative lon +ve !!!???
		double d = GetGreatCircleDistance(lat1, lon1, lat2, lon2);
		cout << "d = " << d;
		d = Nm2Rad(d);
		//cout << ", d_theta = " << d;
		//cout << ", and d = " << Rad2Nm(d) << ' ';
		if(sin(lon2 - lon1) < 0) {
			cout << " A ";
			h = acos((sin(lat2)-sin(lat1)*cos(d))/(sin(d)*cos(lat1)));
		} else {
			cout << " B ";
			h = 2.0 * SG_PI - acos((sin(lat2)-sin(lat1)*cos(d))/(sin(d)*cos(lat1)));
		}
	}
	cout << h * SG_RADIANS_TO_DEGREES << '\n';
	*/
	
	return( mod(atan2(sin(lon2-lon1)*cos(lat2),
            cos(lat1)*sin(lat2)-sin(lat1)*cos(lat2)*cos(lon2-lon1)),
            2.0*SG_PI) );
}

// Return a position on a radial from wp1 given distance d (nm) and magnetic heading h (degrees)
// Note that d should be less that 1/4 Earth diameter!
GPSWaypoint DCLGPS::GetPositionOnMagRadial(const GPSWaypoint& wp1, double d, double h) {
	h += sgGetMagVar(wp1.lon, wp1.lat, 0.0, _time->getJD()) * SG_RADIANS_TO_DEGREES;
	return(GetPositionOnRadial(wp1, d, h));
}

// Return a position on a radial from wp1 given distance d (nm) and TRUE heading h (degrees)
// Note that d should be less that 1/4 Earth diameter!
GPSWaypoint DCLGPS::GetPositionOnRadial(const GPSWaypoint& wp1, double d, double h) {
	while(h < 0.0) h += 360.0;
	while(h > 360.0) h -= 360.0;
	
	h *= SG_DEGREES_TO_RADIANS;
	d *= (SG_PI / (180.0 * 60.0));
	
	double lat=asin(sin(wp1.lat)*cos(d)+cos(wp1.lat)*sin(d)*cos(h));
	double lon;
	if(cos(lat)==0) {
		lon=wp1.lon;      // endpoint a pole
	} else {
		lon=mod(wp1.lon+asin(sin(h)*sin(d)/cos(lat))+SG_PI,2*SG_PI)-SG_PI;
	}
	
	GPSWaypoint wp;
	wp.lat = lat;
	wp.lon = lon;
	wp.type = GPS_WP_VIRT;
	return(wp);
}

// Returns cross-track deviation in Nm.
double DCLGPS::CalcCrossTrackDeviation() const {
	return(CalcCrossTrackDeviation(_fromWaypoint, _activeWaypoint));
}

// Returns cross-track deviation of the current position between two arbitary waypoints in nm.
double DCLGPS::CalcCrossTrackDeviation(const GPSWaypoint& wp1, const GPSWaypoint& wp2) const {
	//if(wp1 == NULL || wp2 == NULL) return(0.0);
	if(wp1.id.empty() || wp2.id.empty()) return(0.0);
	double xtd = asin(sin(Nm2Rad(GetGreatCircleDistance(wp1.lat, wp1.lon, _gpsLat, _gpsLon))) 
	                  * sin(GetGreatCircleCourse(wp1.lat, wp1.lon, _gpsLat, _gpsLon) - GetGreatCircleCourse(wp1.lat, wp1.lon, wp2.lat, wp2.lon)));
	return(Rad2Nm(xtd));
}
