// kln89_page.cxx - a class to manage the simulation of a KLN89
//                  GPS unit.  Note that this is primarily the 
//                  simulation of the user interface and display
//                  - the core GPS calculations such as position
//                  and waypoint sequencing are done (or should 
//                  be done) by FG code. 
//
// Written by David Luff, started 2005.
//
// Copyright (C) 2005 - David C Luff - daveluff AT ntlworld.com
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
//
// $Id$

#include "kln89.hxx"
#include "kln89_page.hxx"
#include "kln89_page_apt.hxx"
#include "kln89_page_vor.hxx"
#include "kln89_page_ndb.hxx"
#include "kln89_page_int.hxx"
#include "kln89_page_usr.hxx"
#include "kln89_page_act.hxx"
#include "kln89_page_nav.hxx"
#include "kln89_page_fpl.hxx"
#include "kln89_page_cal.hxx"
#include "kln89_page_set.hxx"
#include "kln89_page_oth.hxx"
#include "kln89_page_alt.hxx"
#include "kln89_page_dir.hxx"
#include "kln89_page_nrst.hxx"
#include "kln89_symbols.hxx"
#include <iostream>

#include <ATCDCL/ATCProjection.hxx>

#include <Main/fg_props.hxx>
#include <simgear/structure/commands.hxx>
#include <Airports/simple.hxx>

using std::cout;
using std::string;

// Command callbacks for FlightGear

static bool do_kln89_msg_pressed(const SGPropertyNode* arg) {
	//cout << "do_kln89_msg_pressed called!\n";
	KLN89* gps = (KLN89*)globals->get_subsystem("kln89");
	gps->MsgPressed();
	return(true);
}

static bool do_kln89_obs_pressed(const SGPropertyNode* arg) {
	//cout << "do_kln89_obs_pressed called!\n";
	KLN89* gps = (KLN89*)globals->get_subsystem("kln89");
	gps->OBSPressed();
	return(true);
}

static bool do_kln89_alt_pressed(const SGPropertyNode* arg) {
	//cout << "do_kln89_alt_pressed called!\n";
	KLN89* gps = (KLN89*)globals->get_subsystem("kln89");
	gps->AltPressed();
	return(true);
}

static bool do_kln89_nrst_pressed(const SGPropertyNode* arg) {
	KLN89* gps = (KLN89*)globals->get_subsystem("kln89");
	gps->NrstPressed();
	return(true);
}

static bool do_kln89_dto_pressed(const SGPropertyNode* arg) {
	KLN89* gps = (KLN89*)globals->get_subsystem("kln89");
	gps->DtoPressed();
	return(true);
}

static bool do_kln89_clr_pressed(const SGPropertyNode* arg) {
	KLN89* gps = (KLN89*)globals->get_subsystem("kln89");
	gps->ClrPressed();
	return(true);
}

static bool do_kln89_ent_pressed(const SGPropertyNode* arg) {
	KLN89* gps = (KLN89*)globals->get_subsystem("kln89");
	gps->EntPressed();
	return(true);
}

static bool do_kln89_crsr_pressed(const SGPropertyNode* arg) {
	KLN89* gps = (KLN89*)globals->get_subsystem("kln89");
	gps->CrsrPressed();
	return(true);
}

static bool do_kln89_knob1left1(const SGPropertyNode* arg) {
	KLN89* gps = (KLN89*)globals->get_subsystem("kln89");
	gps->Knob1Left1();
	return(true);
}

static bool do_kln89_knob1right1(const SGPropertyNode* arg) {
	KLN89* gps = (KLN89*)globals->get_subsystem("kln89");
	gps->Knob1Right1();
	return(true);
}

static bool do_kln89_knob2left1(const SGPropertyNode* arg) {
	KLN89* gps = (KLN89*)globals->get_subsystem("kln89");
	gps->Knob2Left1();
	return(true);
}

static bool do_kln89_knob2right1(const SGPropertyNode* arg) {
	KLN89* gps = (KLN89*)globals->get_subsystem("kln89");
	gps->Knob2Right1();
	return(true);
}

// End command callbacks

KLN89::KLN89(RenderArea2D* instrument) 
: DCLGPS(instrument) {
	_mode = KLN89_MODE_DISP;
	_blink = false;
	_cum_dt = 0.0;
	_nFields = 2;
	_maxFields = 2;
	_xBorder = 0;
	_yBorder = 4;
	// ..Field..[0] => no fields in action
	_xFieldBorder[0] = 0;
	_xFieldBorder[1] = 0;
	_yFieldBorder[0] = 0;
	_yFieldBorder[1] = 0;
	_xFieldBorder[2] = 2;
	_yFieldBorder[2] = 0;
	_xFieldStart[0] = 0;
	_xFieldStart[1] = 0;
	_xFieldStart[2] = 45;
	_yFieldStart[0] = 0;
	_yFieldStart[1] = 0;
	_yFieldStart[2] = 0;
	
	//_pixelated = true;
	_pixelated = false;

	// Cyclic pages
	_pages.clear();
	KLN89Page* apt_page = new KLN89AptPage(this);
	_pages.push_back(apt_page);
	KLN89Page* vor_page = new KLN89VorPage(this);
	_pages.push_back(vor_page);
	KLN89Page* ndb_page = new KLN89NDBPage(this);
	_pages.push_back(ndb_page);
	KLN89Page* int_page = new KLN89IntPage(this);
	_pages.push_back(int_page);
	KLN89Page* usr_page = new KLN89UsrPage(this);
	_pages.push_back(usr_page);
	KLN89Page* act_page = new KLN89ActPage(this);
	_pages.push_back(act_page);
	KLN89Page* nav_page = new KLN89NavPage(this);
	_pages.push_back(nav_page);
	KLN89Page* fpl_page = new KLN89FplPage(this);
	_pages.push_back(fpl_page);
	KLN89Page* cal_page = new KLN89CalPage(this);
	_pages.push_back(cal_page);
	KLN89Page* set_page = new KLN89SetPage(this);
	_pages.push_back(set_page);
	KLN89Page* oth_page = new KLN89OthPage(this);
	_pages.push_back(oth_page);
	_nPages = _pages.size();
	_curPage = 0;
	
	// Other pages
	_alt_page = new KLN89AltPage(this);
	_dir_page = new KLN89DirPage(this);
	_nrst_page = new KLN89NrstPage(this);
	
	_activePage = apt_page;
	_obsMode = false;
	_dto = false;
	_fullLegMode = true;
	_obsHeading = 215;

	// User-settable configuration.  Eventually this should be user-achivable in order that settings can be persistent between sessions.
	_altUnits = GPS_ALT_UNITS_FT;
	_baroUnits = GPS_PRES_UNITS_IN;
	_velUnits = GPS_VEL_UNITS_KT;
	_distUnits = GPS_DIST_UNITS_NM;
	_suaAlertEnabled = false;
	_altAlertEnabled = false;
	_minDisplayBrightness = 4;
	_defaultFirstChar = 'A';	
	
	if(_baroUnits == GPS_PRES_UNITS_IN) {
		_userBaroSetting = 2992;
	} else {
		_userBaroSetting = 1013;
	}
	
	_maxFlightPlans = 26;
	for(unsigned int i=0; i<_maxFlightPlans; ++i) {
		GPSFlightPlan* fp = new GPSFlightPlan;
		fp->waypoints.clear();
		_flightPlans.push_back(fp);
	}
	_activeFP = _flightPlans[0];
	
	_entJump = _clrJump = -1;
	_jumpRestoreCrsr = false;
	
	_dispMsg = false;
	
	_dtoReview = false;

	// Moving map stuff
	_mapOrientation = 0;
	_mapHeading = 0.0;
	_mapHeadingUpdateTimer = 0.0;
	_drawSUA = false;
	_drawVOR = false;
	_drawApt = true;
	//_mapScaleIndex = 20;
	_mapScaleIndex = 7;	// I think that the above is more accurate for no-flightplan default, but this is more sane for initial testing!
	_mapScaleAuto = true;
	
	// Mega-hack - hardwire airport town and state names for the FG base area since we don't have any data for these at the moment
	// TODO - do this better one day!
	_airportTowns["KSFO"] = "San Francisco";
	_airportTowns["KSQL"] = "San Carlos";
	_airportTowns["KPAO"] = "Palo Alto";
	_airportTowns["KNUQ"] = "Mountain View";
	_airportTowns["KSJC"] = "San Jose";
	_airportTowns["KRHV"] = "San Jose";
	_airportTowns["E16"] = "San Martin";
	_airportTowns["KWVI"] = "Watsonville";
	_airportTowns["KOAK"] = "Oakland";
	_airportTowns["KHWD"] = "Hayward";
	_airportTowns["KLVK"] = "Livermore";
	_airportTowns["KCCR"] = "Concord";
	_airportTowns["KTCY"] = "Tracy";
	_airportTowns["KSCK"] = "Stockton";
	_airportTowns["KHAF"] = "Half Moon Bay";
	
	_airportStates["KSFO"] = "CA";
	_airportStates["KSQL"] = "CA";
	_airportStates["KPAO"] = "CA";
	_airportStates["KNUQ"] = "CA";
	_airportStates["KSJC"] = "CA";
	_airportStates["KRHV"] = "CA";
	_airportStates["E16"] = "CA";
	_airportStates["KWVI"] = "CA";
	_airportStates["KOAK"] = "CA";
	_airportStates["KHWD"] = "CA";
	_airportStates["KLVK"] = "CA";
	_airportStates["KCCR"] = "CA";
	_airportStates["KTCY"] = "CA";
	_airportStates["KSCK"] = "CA";
	_airportStates["KHAF"] = "CA";
}

KLN89::~KLN89() {
	for(unsigned int i=0; i<_pages.size(); ++i) {
		delete _pages[i];
	}
	
	delete _alt_page;
	delete _dir_page;
	delete _nrst_page;
	
	for(unsigned int i=0; i<_maxFlightPlans; ++i) {
		ClearFlightPlan(i);
		delete _flightPlans[i];
	}
}

void KLN89::bind() {
	fgTie("/instrumentation/gps/message-alert", this, &KLN89::GetMsgAlert);
	DCLGPS::bind();
}

void KLN89::unbind() {
	fgUntie("/instrumentation/gps/message-alert");
	DCLGPS::unbind();
}

void KLN89::init() {
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
	
	DCLGPS::init();
}

void KLN89::update(double dt) {
	// Run any positional calc's required first
	DCLGPS::update(dt);
	
	// Set the display brightness.  This should be reduced in response to falling light
	// (i.e. nighttime), or the user covering the photocell that detects the light level.
	// At the moment I don't know how to detect nighttime or actual light level, so only
	// respond to the photocell being obscured.
	// TODO - reduce the brightness in response to nighttime / lowlight.
	float rgba[4] = {1.0, 0.0, 0.0, 1.0};
	if(fgGetBool("/instrumentation/kln89/photocell-obscured")) {
		rgba[0] -= (9 - _minDisplayBrightness) * 0.05;
	}
	_instrument->SetPixelColor(rgba);
	
	_cum_dt += dt;
	if(_blink) {
		if(_cum_dt > 0.2) {
			_cum_dt = 0.0;
			_blink = false;
		}
	} else {
		if(_cum_dt > 0.8) {
			_cum_dt = 0.0;
			_blink = true;
		}
	}
	
	_mapHeadingUpdateTimer += dt;
	if(_mapHeadingUpdateTimer > 1.0) {
		UpdateMapHeading();
		_mapHeadingUpdateTimer = 0.0;
	}
	
	_instrument->Flush();
	_instrument->DrawBackground();
	
	if(_dispMsg) {
		if(_messageStack.empty()) {
			DrawText("No Message", 0, 5, 2);
		} else {
			// TODO - parse the message string for special strings that indicate degrees signs etc!
			DrawText(*_messageStack.begin(), 0, 0, 3);
		}
		return;
	} else {
		if(!_messageStack.empty()) {
			DrawMessageAlert();
		}
	}
	
	// Draw the indicator that shows which page we are on.
	if(_curPage == 6 && _activePage->GetSubPage() == 3) {
		// Don't draw the bar on the nav-4 page
	} else if((_activePage != _nrst_page) && (_activePage != _dir_page) && (_activePage != _alt_page) && (!_dispMsg)) {
		// Don't draw the bar on the NRST, DTO or MSG pages
		DrawBar(_curPage);
	}
	
	_activePage->Update(dt);
}

void KLN89::CreateDefaultFlightPlans() {
	// TODO - read these in from preferences.xml or similar instead!!!!
	// Create some hardwired default flightplans for testing.
	vector<string> ids;
	vector<GPSWpType> wps;
	
	ids.clear();
	wps.clear();
	ids.push_back("KLSN");
	wps.push_back(GPS_WP_APT);
	ids.push_back("VOLTA");
	wps.push_back(GPS_WP_INT);
	ids.push_back("C83");
	wps.push_back(GPS_WP_APT);
	CreateFlightPlan(_flightPlans[5], ids, wps);
	
	ids.clear();
	wps.clear();
	ids.push_back("KCCR");
	wps.push_back(GPS_WP_APT);
	ids.push_back("KHAF");
	wps.push_back(GPS_WP_APT);
	CreateFlightPlan(_flightPlans[4], ids, wps);
	
	ids.clear();
	wps.clear();
	ids.push_back("KLVK");
	wps.push_back(GPS_WP_APT);
	ids.push_back("OAK");
	wps.push_back(GPS_WP_VOR);
	ids.push_back("PORTE");
	wps.push_back(GPS_WP_INT);
	ids.push_back("KHAF");
	wps.push_back(GPS_WP_APT);
	CreateFlightPlan(_flightPlans[3], ids, wps);
	
	ids.clear();
	wps.clear();
	ids.push_back("KDPA");
	wps.push_back(GPS_WP_APT);
	ids.push_back("OBK");
	wps.push_back(GPS_WP_VOR);
	ids.push_back("ENW");
	wps.push_back(GPS_WP_VOR);
	ids.push_back("KRAC");
	wps.push_back(GPS_WP_APT);
	CreateFlightPlan(_flightPlans[2], ids, wps);
	//cout << "Size of FP2 WP list is " << _flightPlans[2]->waypoints.size() << '\n';
	
	ids.clear();
	wps.clear();
	ids.push_back("KSFO");
	ids.push_back("KOAK");
	wps.push_back(GPS_WP_APT);
	wps.push_back(GPS_WP_APT);
	CreateFlightPlan(_flightPlans[1], ids, wps);
	
	ids.clear();
	wps.clear();
	//ids.push_back("KOSH");
	ids.push_back("KSFO");
	ids.push_back("KHAF");
	ids.push_back("OSI");
	ids.push_back("KSQL");
	//ids.push_back("KPAO");
	//ids.push_back("KHWD");
	wps.push_back(GPS_WP_APT);
	wps.push_back(GPS_WP_APT);
	wps.push_back(GPS_WP_VOR);
	wps.push_back(GPS_WP_APT);
	//wps.push_back(GPS_WP_APT);
	//wps.push_back(GPS_WP_APT);
	CreateFlightPlan(_flightPlans[0], ids, wps);
	
	/*
	ids.clear();
	wps.clear();
	ids.push_back("KLVK");
	ids.push_back("KHWD");
	wps.push_back(GPS_WP_APT);
	wps.push_back(GPS_WP_APT);
	CreateFlightPlan(_flightPlans[0], ids, wps);
	*/
}

void KLN89::SetBaroUnits(int n, bool wrap) {
	if(n < 1) {
		_baroUnits = (KLN89PressureUnits)(wrap ? 3 : 1);
	} else if(n > 3) {
		_baroUnits = (KLN89PressureUnits)(wrap ? 1 : 3);
	} else {
		_baroUnits = (KLN89PressureUnits)n;
	}
}

void KLN89::Knob1Right1() {
	if(_mode == KLN89_MODE_DISP) {
		_activePage->LooseFocus();
		if(_cleanUpPage >= 0) {
			_pages[(unsigned int)_cleanUpPage]->CleanUp();
			_cleanUpPage = -1;
		}
		_curPage++;
		if(_curPage >= _pages.size()) _curPage = 0;
		_activePage = _pages[_curPage];
	} else {
		_activePage->Knob1Right1();
	}
	update(0.0);
}

void KLN89::Knob1Left1() {
	if(_mode == KLN89_MODE_DISP) {
		_activePage->LooseFocus();
		if(_cleanUpPage >= 0) {
			_pages[(unsigned int)_cleanUpPage]->CleanUp();
			_cleanUpPage = -1;
		}
		if(_curPage == 0) {
			_curPage = _pages.size() - 1;
		} else {
			_curPage--;
		}
		_activePage = _pages[_curPage];
	} else {
		_activePage->Knob1Left1();
	}
	update(0.0);
}

void KLN89::Knob2Left1() {
	_activePage->Knob2Left1();
}

void KLN89::Knob2Right1() {
	_activePage->Knob2Right1();
}

void KLN89::CrsrPressed() {
	_dispMsg = false;
	// CRSR cannot be switched off on nrst page.
	if(_activePage == _nrst_page) { return; }
	// CRSR is always off when inner-knob is out on nav4 page.
	if(_curPage == 6 && _activePage->GetSubPage() == 3 && fgGetBool("/instrumentation/kln89/scan-pull")) { return; }
	if(_cleanUpPage >= 0) {
		_pages[(unsigned int)_cleanUpPage]->CleanUp();
		_cleanUpPage = -1;
	}
	_jumpRestoreCrsr = false;
	_entJump = _clrJump = -1;
	((KLN89Page*)_activePage)->SetEntInvert(false);
	if(_mode == KLN89_MODE_DISP) {
		_mode = KLN89_MODE_CRSR;
		_activePage->CrsrPressed();
	} else {
		_mode = KLN89_MODE_DISP;
		_activePage->CrsrPressed();
	}
	update(0.0);
}

void KLN89::EntPressed() {
	if(_entJump >= 0) {
		if(_curPage < 5) {
			// one of the data pages.  Signal ent pressed to it here, and ent pressed to the call back page a few lines further down.
			// Ie. 2 ent pressed signals in this case is deliberate.
			_activePage->EntPressed();
		}
		_curPage = _entJump;
		_activePage = _pages[(unsigned int)_entJump];
		if(_jumpRestoreCrsr) _mode = KLN89_MODE_CRSR;
		_entJump = _clrJump = -1;
	}
	if(_activePage == _dir_page) {
		_dir_page->EntPressed();
		_mode = KLN89_MODE_DISP;
		_activePage = _pages[_curPage];
	} else {
		_activePage->EntPressed();
	}
}

void KLN89::ClrPressed() {
	if(_clrJump >= 0) {
		_curPage = _clrJump;
		_activePage = _pages[(unsigned int)_clrJump];
		if(_jumpRestoreCrsr) _mode = KLN89_MODE_CRSR;
		_entJump = _clrJump = -1;
	}
	_activePage->ClrPressed();
}

void KLN89::DtoPressed() {
	if(_activePage != _dir_page) {
		// Figure out which waypoint the dir page should display, according to the following rules:
		// 1. If the FPL 0 page is displayed AND the cursor is over one of the waypoints, display that waypoint.
		// 2. If the NAV 4 page is displayed with the inner knob pulled out, display the waypoint highlighted in the lower RH corner of the nav page.
		// 3. If any of APT, VOR, NDB, INT, USR or ACT pages is displayed then display the waypoint being viewed.
		// 4. If none of the above, display the active waypoint, unless the active waypoint is the MAP of an approach and it has been flown past 
		// (no waypoint sequence past the MAP), in which case display the first waypoint of the missed approach procedure.
		// 5. If none of the above (i.e. no active waypoint) then display blanks.
		if(_curPage <= 5) {
			// APT, VOR, NDB, INT, USR or ACT
			if(!_activePage->GetId().empty()) {	// Guard against no user waypoints defined
				_dir_page->SetId(_activePage->GetId());
			} else {
				_dir_page->SetId(_activeWaypoint.id);
			}
		} else if(_curPage == 6 && _activePage->GetSubPage() == 3 && fgGetBool("/instrumentation/kln89/scan-pull") && _activeFP->waypoints.size()) {
			// NAV 4
			_dir_page->SetId(((KLN89NavPage*)_activePage)->GetNav4WpId());
		} else if(_curPage == 7 && _activePage->GetSubPage() == 0 && _mode == KLN89_MODE_CRSR) {
			// FPL 0
			if(!_activePage->GetId().empty()) {
				//cout << "Not empty!!!\n";
				_dir_page->SetId(_activePage->GetId());
			} else {
				//cout << "empty :-(\n";
				_dir_page->SetId(_activeWaypoint.id);
			}
		} else {
			_dir_page->SetId(_activeWaypoint.id);
		}
		// This need to come after the bit before otherwise the FPL or NAV4 page clears their current ID when it looses focus.
		_activePage->LooseFocus();
		_activePage = _dir_page;
		_mode = KLN89_MODE_CRSR;
	}
}

void KLN89::NrstPressed() {
	if(_activePage != _nrst_page) {
		_activePage->LooseFocus();	// TODO - check whether we should call loose focus here
		_lastActivePage = _activePage;
		_activePage = _nrst_page;
		_lastMode = _mode;
		_mode = KLN89_MODE_CRSR;
	} else {
		_activePage = _lastActivePage;
		_mode = _lastMode;
	}
}
	
void KLN89::AltPressed() {
	if(_activePage != _alt_page) {
		_activePage->LooseFocus();	// TODO - check whether we should call loose focus here
		_lastActivePage = _activePage;
		_alt_page->SetSubPage(0);
		_activePage = _alt_page;
		_lastMode = _mode;
		_mode = KLN89_MODE_CRSR;
	} else {
		_alt_page->LooseFocus();
		if(_alt_page->GetSubPage() == 0) {
			_alt_page->SetSubPage(1);
			_mode = KLN89_MODE_CRSR;
		} else {
			_activePage = _lastActivePage;
			_mode = _lastMode;
		}
	}
}

void KLN89::OBSPressed() {
	ToggleOBSMode();
	if(_obsMode) {
		if(!fgGetBool("/instrumentation/nav/slaved-to-gps")) {
			// NOTE: this only applies to ORS 02 firmware, in ORS 01
			// CRSR mode is not automatically set when OBS is started.
			_mode = KLN89_MODE_CRSR;
		}
		_activePage->OBSPressed();
	}
}

void KLN89::MsgPressed() {
	// TODO - handle persistent messages such as SUA alerting.
	// (The message annunciation flashes before first view, but afterwards remains continuously lit with the message available
	// until the potential conflict no longer pertains).
	if(_dispMsg && _messageStack.size()) {
		_messageStack.pop_front();
	}
	_dispMsg = !_dispMsg;
}

void KLN89::ToggleOBSMode() {
	DCLGPS::ToggleOBSMode();
}

void KLN89::DtoInitiate(const string& id) {
	_dtoReview = false;
	// Set the current page to NAV1
	_curPage = 6;
	_activePage = _pages[_curPage];
	_activePage->SetSubPage(0);
	// TODO - need to output a scratchpad message with the new course, but we don't know it yet!
	// Call the base class to actually initiate the DTO.
	DCLGPS::DtoInitiate(id);
}

void KLN89::SetMinDisplayBrightness(int n) {
	_minDisplayBrightness = n;
	if(_minDisplayBrightness < 1) _minDisplayBrightness = 1;
	if(_minDisplayBrightness > 9) _minDisplayBrightness = 9;
}

void KLN89::DecrementMinDisplayBrightness() {
	_minDisplayBrightness--;
	if(_minDisplayBrightness < 1) _minDisplayBrightness = 1;
}

void KLN89::IncrementMinDisplayBrightness() {
	_minDisplayBrightness++;
	if(_minDisplayBrightness > 9) _minDisplayBrightness = 9;
}

void KLN89::DrawBar(int page) {
	int px = 1 + (page * 15);
	int py = 1;
	for(int i=0; i<7; ++i) {
		// Ugh - this is crude and inefficient!
		_instrument->DrawPixel(px+i, py);
		_instrument->DrawPixel(px+i, py+1);
	}
}

// Convert moving map to instrument co-ordinates
void KLN89::MapToInstrument(int &x, int &y) {
	x += _xBorder + _xFieldBorder[2] + _xFieldStart[2];
}

// Draw a pixel specified in instrument co-ords, but clipped to the map region
//void KLN89::DrawInstrMapPixel(int x, int y) {

/*
// Clip, translate and draw a map pixel
// If we didn't need per-pixel clipping, it would be cheaper to translate object rather than pixel positions.
void KLN89::DrawMapPixel(int x, int y, bool invert) {
	if(x < 0 || x > 111 || y < 0 || y > 39)  return;
	x += _xBorder + _xFieldBorder[2] + _xFieldStart[2];
	_instrument->DrawPixel(x, y, invert);
}
*/

// HACK - use something FG provides
static double gps_min(const double &a, const double &b) {
	return(a <= b ? a : b);
}

#if 0
static double gps_max(const double &a, const double &b) {
	return(a >= b ? a : b);
}
#endif

void KLN89::UpdateMapHeading() {
	switch(_mapOrientation) {
	case 0:		// North up
		_mapHeading = 0.0;
		break;
	case 1:		// DTK up
		_mapHeading = _dtkTrue;
		break;
	case 2:		// Track up
		_mapHeading = _track;
		break;
	}
}		

// The screen area allocated to the moving map is 111 x 40 pixels.
// In North up mode, the user position marker is at 57, 20. (Map co-ords).
void KLN89::DrawMap(bool draw_avs) {
	// Set the clipping region to the moving map part of the display
	int xstart = _xBorder + _xFieldBorder[2] + _xFieldStart[2];
	_instrument->SetClipRegion(xstart, 0, xstart + 110, 39);
	
	_mapScaleUnits = (int)_distUnits;
	_mapScale = (double)(KLN89MapScales[_mapScaleUnits][_mapScaleIndex]);
	
	//cout << "Map scale = " << _mapScale << '\n';
	
	double mapScaleMeters = _mapScale * (_mapScaleUnits == 0 ? SG_NM_TO_METER : 1000);
	
	// TODO - use an aligned projection when either DTK or TK up!
	FGATCAlignedProjection mapProj(SGGeod::fromRad(_gpsLon, _gpsLat), _mapHeading);
	double meter_per_pix = (_mapOrientation == 0 ? mapScaleMeters / 20.0f : mapScaleMeters / 29.0f);
//	SGGeod bottomLeft = mapProj.ConvertFromLocal(SGVec3d(gps_max(-57.0 * meter_per_pix, -50000), gps_max((_mapOrientation == 0 ? -20.0 * meter_per_pix : -11.0 * meter_per_pix), -25000), 0.0));
//	SGGeod topRight = mapProj.ConvertFromLocal(SGVec3d(gps_min(54.0 * meter_per_pix, 50000), gps_min((_mapOrientation == 0 ? 20.0 * meter_per_pix : 29.0 * meter_per_pix), 25000), 0.0));



	
	// Draw Airport labels first (but not one's that are waypoints)
	// Draw Airports first (but not one's that are waypoints)
	// Ditto for VORs (not sure if SUA/VOR/Airport ordering is important or not).
	// Ditto for SUA
	// Then flighttrack
	// Then waypoints
	// Then waypoint labels (not sure if this should be before or after waypoints)
	// Then user pos.
	// Annotation then gets drawn by Nav page, NOT this function.

	if(_drawApt && draw_avs) {
		/*
		bool have_apt = _overlays->FindArpByRegion(&apt, bottomLeft.lat(), bottomLeft.lon(), topRight.lat(), topRight.lon());
		//cout << "Vors enclosed are: ";
		// Draw all the labels first...
		for(unsigned int i=0; i<apt.size(); ++i) {
			//cout << nav[i]->id << ' ';
			Point3D p = mapProj.ConvertToLocal(Point3D(apt[i]->lon * SG_RADIANS_TO_DEGREES, apt[i]->lat * SG_RADIANS_TO_DEGREES, 0.0));
			//cout << p << " .... ";
			int mx = int(p.x() / meter_per_pix) + 56;
			int my = int(p.y() / meter_per_pix) + (_mapOrientation == 0 ? 19 : 10);
			//cout << "mx = " << mx << ", my = " << my << '\n';
			bool right_align = (p.x() < 0.0);
			DrawLabel(apt[i]->id, mx + (right_align ? -2 : 3), my + (p.y() < 0.0 ? -7 : 3), right_align);
			// I think that we probably should have -1 in the right_align case above to match the real life instrument.
		}
		// ...and then all the Apts.
		for(unsigned int i=0; i<apt.size(); ++i) {
			//cout << nav[i]->id << ' ';
			Point3D p = mapProj.ConvertToLocal(Point3D(apt[i]->lon * SG_RADIANS_TO_DEGREES, apt[i]->lat * SG_RADIANS_TO_DEGREES, 0.0));
			//cout << p << " .... ";
			int mx = int(p.x() / meter_per_pix) + 56;
			int my = int(p.y() / meter_per_pix) + (_mapOrientation == 0 ? 19 : 10);
			//cout << "mx = " << mx << ", my = " << my << '\n';
			DrawApt(mx, my);
		}
		//cout << '\n';
		*/
	}
	/*
	if(_drawVOR && draw_avs) {
		Overlays::nav_array_type nav;
		bool have_vor = _overlays->FindVorByRegion(&nav, bottomLeft.lat(), bottomLeft.lon(), topRight.lat(), topRight.lon());
		//cout << "Vors enclosed are: ";
		// Draw all the labels first...
		for(unsigned int i=0; i<nav.size(); ++i) {
			//cout << nav[i]->id << ' ';
			Point3D p = mapProj.ConvertToLocal(Point3D(nav[i]->lon * SG_RADIANS_TO_DEGREES, nav[i]->lat * SG_RADIANS_TO_DEGREES, 0.0));
			//cout << p << " .... ";
			int mx = int(p.x() / meter_per_pix) + 56;
			int my = int(p.y() / meter_per_pix) + (_mapOrientation == 0 ? 19 : 10);
			//cout << "mx = " << mx << ", my = " << my << '\n';
			bool right_align = (p.x() < 0.0);
			DrawLabel(nav[i]->id, mx + (right_align ? -2 : 3), my + (p.y() < 0.0 ? -7 : 3), right_align);
			// I think that we probably should have -1 in the right_align case above to match the real life instrument.
		}
		// ...and then all the VORs.
		for(unsigned int i=0; i<nav.size(); ++i) {
			//cout << nav[i]->id << ' ';
			Point3D p = mapProj.ConvertToLocal(Point3D(nav[i]->lon * SG_RADIANS_TO_DEGREES, nav[i]->lat * SG_RADIANS_TO_DEGREES, 0.0));
			//cout << p << " .... ";
			int mx = int(p.x() / meter_per_pix) + 56;
			int my = int(p.y() / meter_per_pix) + (_mapOrientation == 0 ? 19 : 10);
			//cout << "mx = " << mx << ", my = " << my << '\n';
			DrawVOR(mx, my);
		}
		//cout << '\n';
	}
	*/
	
	// FlightTrack
	if(_activeFP->waypoints.size() > 1) {
		vector<int> xvec, yvec, qvec;	// qvec stores the quadrant that each waypoint label should
										// be drawn in (relative to the waypoint). 
										// 1 = NE, 2 = SE, 3 = SW, 4 = NW.
		double save_h = 0.0; // Each pass, save a heading from the previous one for label quadrant determination.
		bool drawTrack = true;
		for(unsigned int i=1; i<_activeFP->waypoints.size(); ++i) {
			GPSWaypoint* wp0 = _activeFP->waypoints[i-1];
			GPSWaypoint* wp1 = _activeFP->waypoints[i];
			SGVec3d p0 = mapProj.ConvertToLocal(SGGeod::fromRad(wp0->lon, wp0->lat));
			SGVec3d p1 = mapProj.ConvertToLocal(SGGeod::fromRad(wp1->lon, wp1->lat));
			int mx0 = int(p0.x() / meter_per_pix + 0.5) + 56;
			int my0 = int(p0.y() / meter_per_pix + 0.5) + (_mapOrientation == 0 ? 19 : 10);
			int mx1 = int(p1.x() / meter_per_pix + 0.5) + 56;
			int my1 = int(p1.y() / meter_per_pix + 0.5) + (_mapOrientation == 0 ? 19 : 10);
			if(i == 1) {
				xvec.push_back(mx0);
				yvec.push_back(my0);
				double h = GetGreatCircleCourse(wp0->lat, wp0->lon, wp1->lat, wp1->lon) * SG_RADIANS_TO_DEGREES;
				// Adjust for map orientation
				h -= _mapHeading;
				qvec.push_back(GetLabelQuadrant(h));
				//cout << "i = " << i << ", h = " << h << ", qvec[0] = " << qvec[0] << '\n';
			}
			xvec.push_back(mx1);
			yvec.push_back(my1);
			if(drawTrack) { DrawLine(mx0, my0, mx1, my1); }
			if(i != 1) {
				double h = GetGreatCircleCourse(wp0->lat, wp0->lon, wp1->lat, wp1->lon) * SG_RADIANS_TO_DEGREES;
				// Adjust for map orientation
				h -= _mapHeading;
				qvec.push_back(GetLabelQuadrant(save_h, h));
			}
			save_h = GetGreatCircleCourse(wp1->lat, wp1->lon, wp0->lat, wp0->lon) * SG_RADIANS_TO_DEGREES;
			// Adjust for map orientation
			save_h -= _mapHeading;
			if(i == _activeFP->waypoints.size() - 1) {
				qvec.push_back(GetLabelQuadrant(save_h));
			}
			// Don't draw flight track beyond the missed approach point of an approach
			if(_approachLoaded) {
				//cout << "Waypoints are " << wp0->id << " and " << wp1->id << '\n';
				//cout << "Types are " << wp0->appType << " and " << wp1->appType << '\n';
				if(wp1->appType == GPS_MAP) {
					drawTrack = false;
				}
			}
		}
		// ASSERT(xvec.size() == yvec.size() == qvec.size() == _activeFP->waypoints.size());
		for(unsigned int i=0; i<xvec.size(); ++i) {
			DrawWaypoint(xvec[i], yvec[i]);
			bool right_align = (qvec[i] > 2);
			bool top = (qvec[i] == 1 || qvec[i] == 4);
			// TODO - not sure if labels should be drawn in sequence with waypoints and flightpaths,
			// or all before or all afterwards.  Doesn't matter a huge deal though.
			DrawLabel(_activeFP->waypoints[i]->id, xvec[i] + (right_align ? -2 : 3), yvec[i] + (top ? 3 : -7), right_align);
		}
	}
	
	// User pos
	if(_mapOrientation == 0) {
		// North up
		DrawUser1(56, 19);
	} else if(_mapOrientation == 1) {
		// DTK up
		DrawUser1(56, 10);
	} else if(_mapOrientation == 2) {
		// TK up
		DrawUser2(56, 10);
	} else {
		// Heading up
		// TODO - don't know what to do here!
	}
	
	// And finally, reset the clip region to stop the rest of the code going pear-shaped!
	_instrument->ResetClipRegion();
}

// Get the quadrant to draw the label of the start or end waypoint (i.e. one with only one track from it).
// Heading specified FROM the waypoint.
// 4 | 1
// -----
// 3 | 2
int KLN89::GetLabelQuadrant(double h) {
	while(h < 0.0) h += 360.0;
	while(h > 360.0) h -= 360.0;
	if(h < 90.0) return(3);
	if(h < 180.0) return(4);
	if(h < 270.0) return(1);
	return(2);
}

// Get the quadrant to draw the label of an en-route waypoint,
// with BOTH tracks specified as headings FROM the waypoint.
// 4 | 1
// -----
// 3 | 2
int KLN89::GetLabelQuadrant(double h1, double h2) {
	while(h1 < 0.0) h1 += 360.0;
	while(h1 > 360.0) h1 -= 360.0;
	while(h2 < 0.0) h2 += 360.0;
	while(h2 > 360.0) h2 -= 360.0;
	double max_min_diff = 0.0;
	int quad = 1;
	for(int i=0; i<4; ++i) {
		double h = 45 + (90 * i);
		double diff1 = fabs(h - h1);
		if(diff1 > 180) diff1 = 360 - diff1;
		double diff2 = fabs(h - h2);
		if(diff2 > 180) diff2 = 360 - diff2;
		double min_diff = gps_min(diff1, diff2);
		if(min_diff > max_min_diff) {
			max_min_diff = min_diff;
			quad = i + 1;
		}
	}
	//cout << "GetLabelQuadrant, h1 = " << h1 << ", h2 = " << h2 << ", quad = " << quad << '\n';
	return(quad);
}

// Draw the diamond style of user pos
// 
//    o
//   oxo
//  oxxxo
// oxxxxxo
//  oxxxo
//   oxo
//    o
// 
void KLN89::DrawUser1(int x, int y) {
	MapToInstrument(x, y);
	int min_j = 0, max_j = 0;
	for(int i=-3; i<=3; ++i) {
		for(int j=min_j; j<=max_j; ++j) {
			_instrument->DrawPixel(x+j, y+i, (j == min_j || j == max_j ? true : false));
		}
		if(i < 0) {
			min_j--;
			max_j++;
		} else {
			min_j++;
			max_j--;
		}
	}
}

// Draw the airplane style of user pos
// Define the origin to be the midpoint of the *fuselage*
void KLN89::DrawUser2(int x, int y) {
	MapToInstrument(x, y);
	
	// Draw the background as three black quads first
	_instrument->DrawQuad(x-2, y-3, x+2, y-1, true);
	_instrument->DrawQuad(x-3, y, x+3, y+2, true);
	_instrument->DrawQuad(x-1, y+3, x+1, y+3, true);
	
	if(_pixelated) {
		for(int j=y-2; j<=y+2; ++j) {
			_instrument->DrawPixel(x, j);
		}
		for(int i=x-1; i<=x+1; ++i) {
			_instrument->DrawPixel(i, y-2);
		}
		for(int i=x-2; i<=x+2; ++i) {
			_instrument->DrawPixel(i, y+1);
		}
	} else {
		_instrument->DrawQuad(x, y-2, x, y+2);
		_instrument->DrawQuad(x-1, y-2, x+1, y-2);
		_instrument->DrawQuad(x-2, y+1, x+2, y+1);
	}
}

// Draw an airport symbol on the moving map
//
//  ooo
// ooxoo
// oxxxo
// ooxoo
//  ooo
//
void KLN89::DrawApt(int x, int y) {
	MapToInstrument(x, y);
	
	int j = y-2;
	int i;
	for(i=x-1; i<=x+1; ++i) _instrument->DrawPixel(i, j, true);
	++j;
	for(i=x-2; i<=x+2; ++i) _instrument->DrawPixel(i, j, (i != x ? true : false));
	++j;
	for(i=x-2; i<=x+2; ++i) _instrument->DrawPixel(i, j, (std::abs(i - x) > 1 ? true : false));
	++j;
	for(i=x-2; i<=x+2; ++i) _instrument->DrawPixel(i, j, (i != x ? true : false));
	++j;
	for(i=x-1; i<=x+1; ++i) _instrument->DrawPixel(i, j, true);
}

// Draw a waypoint on the moving map
//
// ooooo
// oxxxo
// oxxxo
// oxxxo
// ooooo
//
void KLN89::DrawWaypoint(int x, int y) {
	MapToInstrument(x, y);
	_instrument->SetDebugging(true);
	
	// Draw black background
	_instrument->DrawQuad(x-2, y-2, x+2, y+2, true);
	
	// Draw the coloured square
	if(_pixelated) {
		for(int i=x-1; i<=x+1; ++i) {
			for(int j=y-1; j<=y+1; ++j) {
				_instrument->DrawPixel(i, j);
			}
		}
	} else {
		_instrument->DrawQuad(x-1, y-1, x+1, y+1);
	}
	_instrument->SetDebugging(false);
}

// Draw a VOR on the moving map
//
// ooooo
// oxxxo
// oxoxo
// oxxxo
// ooooo
//
void KLN89::DrawVOR(int x, int y) {
	// Cheat - draw a waypoint and then a black pixel in the middle.
	// Need to call Waypoint draw *before* translating co-ords.
	DrawWaypoint(x, y);
	MapToInstrument(x, y);
	_instrument->DrawPixel(x, y, true);
}

// Draw a line on the moving map
void KLN89::DrawLine(int x1, int y1, int x2, int y2) {
	MapToInstrument(x1, y1);
	MapToInstrument(x2, y2);
	_instrument->DrawLine(x1, y1, x2, y2);
}

void KLN89::DrawMapUpArrow(int x, int y) {
	MapToInstrument(x, y);
	if(_pixelated) {
		for(int j=0; j<7; ++j) {
			_instrument->DrawPixel(x + 2, y + j);
		}
	} else {
		_instrument->DrawQuad(x+2, y, x+2, y+6);
	}
	_instrument->DrawPixel(x, y+4);
	_instrument->DrawPixel(x+1, y+5);
	_instrument->DrawPixel(x+3, y+5);
	_instrument->DrawPixel(x+4, y+4);
}

// Draw a quad on the moving map
void KLN89::DrawMapQuad(int x1, int y1, int x2, int y2, bool invert) {
	MapToInstrument(x1, y1);
	MapToInstrument(x2, y2);
	_instrument->DrawQuad(x1, y1, x2, y2, invert);
}

// Draw an airport or waypoint label on the moving map
// Specify position by the map pixel co-ordinate of the left or right, bottom, of the *visible* portion of the label.
// The black background quad will automatically overlap this by 1 pixel.
void KLN89::DrawLabel(const string& s, int x1, int y1, bool right_align) {
	MapToInstrument(x1, y1);
	if(!right_align) {
		for(unsigned int i=0; i<s.size(); ++i) {
			char c = s[i];
			x1 += DrawSmallChar(c, x1, y1);
			x1 ++;
		}
	} else {
		for(int i=(int)(s.size()-1); i>=0; --i) {
			char c = s[i];
			x1 -= DrawSmallChar(c, x1, y1, right_align);
			x1--;
		}
	}
}

void KLN89::DrawCDI() {
	// Scale
	for(int i=0; i<5; ++i) {
		DrawSpecialChar(2, 2, 3+i, 2);
		DrawSpecialChar(1, 2, 9+i, 2);
	}
	
	int field = 2;
	int px = 8 * 7 + _xBorder + _xFieldBorder[field] + _xFieldStart[field] + 2;
	int py = 2 * 9 + _yBorder + _yFieldBorder[field] + _yFieldStart[field];
	
	// Deflection bar
	// Every 7 pixels deflection left or right is one dot on the scale, and hence 1/5 FSD.
	// Maximum deflection is 37 pixels left, or 38 pixels right !?!
	double xtd = CalcCrossTrackDeviation();
	int deflect;
	if(_cdiScaleTransition) {
		double dots = (xtd / _currentCdiScale) * 5.0;
		deflect = (int)(dots * 7.0 * -1.0);
		// TODO - for all these I think I should add 0.5 before casting to int, and *then* multiply by -1.  Possibly!
	} else {
		if(0 == _currentCdiScaleIndex) {	// 5.0nm FSD => 1 nm per dot => 7 pixels per nm.
			deflect = (int)(xtd * 7.0 * -1.0);	// The -1.0 is because we move the 'needle' indicating the course, not the plane.
		} else if(1 == _currentCdiScaleIndex) {
			deflect = (int)(xtd * 35.0 * -1.0);
		} else {	// 0.3 == _cdiScale
			deflect = (int)(xtd * 116.6666666667 * -1.0);
		}
	}
	if(deflect > 38) deflect = 38;
	if(deflect < -37) deflect = -37;
	if(_pixelated) {
		for(int j=0; j<9; ++j) {
			_instrument->DrawPixel(px + deflect, py+j);
			_instrument->DrawPixel(px + deflect + 1, py+j);
		}
	} else {
		_instrument->DrawQuad(px + deflect, py, px + deflect + 1, py + 8);
	}
	
	// To/From indicator
	px-=4;
	py+=2;
	for(int j=4; j>=0; --j) {
		int k = 10 - (2*j);
		for(int i=0; i<k; ++i) {		
			_instrument->DrawPixel(px+j+i, (_headingBugTo ? py+j : py+4-j));
			// At the extremities, draw the outlining dark pixel
			if(i == 0 || i == k-1) {
				_instrument->DrawPixel(px+j+i, (_headingBugTo ? py+j+1 : py+3-j), true);
			}
		}
	}
}

void KLN89::DrawLegTail(int py) {
	int px = 0 * 7 + _xBorder + _xFieldBorder[2] + _xFieldStart[2];
	py = py * 9 + _yBorder + _yFieldBorder[2] + _yFieldStart[2];
	
	px++;
	py+=3;
	py++;	// Hack - not sure if this represents a border issue.
	
	for(int i=0; i<9; ++i) _instrument->DrawPixel(px, py+i);
	for(int i2=0; i2<5; ++i2) _instrument->DrawPixel(px+i2, py+9);
}

void KLN89::DrawLongLegTail(int py) {
	int px = 0 * 7 + _xBorder + _xFieldBorder[2] + _xFieldStart[2];
	py = py * 9 + _yBorder + _yFieldBorder[2] + _yFieldStart[2];
	
	px++;
	py+=3;
	py++;	// Hack - not sure if this represents a border issue.
	
	for(int i=0; i<18; ++i) _instrument->DrawPixel(px, py+i);
	for(int i2=0; i2<5; ++i2) _instrument->DrawPixel(px+i2, py+18);
}

void KLN89::DrawHalfLegTail(int py) {
}

void KLN89::DrawDivider() {
	int px = _xFieldStart[2] - 1;
	int py = _yBorder;
	for(int i=0; i<36; ++i) {
		_instrument->DrawPixel(px, py+i);
	}
}

void KLN89::DrawEnt(int field, int px, int py) {
	px = px * 7 + _xBorder + _xFieldBorder[field] + _xFieldStart[field];
	py = py * 9 + _yBorder + _yFieldBorder[field] + _yFieldStart[field] + 1;
	
	px++;	// Not sure why we need px++, but it seems to work!
	py++;
	
	// E
	for(int i=0; i<5; ++i) _instrument->DrawPixel(px, py+i);
	_instrument->DrawPixel(px+1, py);
	_instrument->DrawPixel(px+2, py);
	_instrument->DrawPixel(px+1, py+2);
	_instrument->DrawPixel(px+1, py+4);
	_instrument->DrawPixel(px+2, py+4);
	
	px += 4;
	// N
	for(int i=0; i<4; ++i) _instrument->DrawPixel(px, py+i);
	_instrument->DrawPixel(px+1, py+2);
	_instrument->DrawPixel(px+2, py+1);
	for(int i=0; i<4; ++i) _instrument->DrawPixel(px+3, py+i);
	
	px += 5;
	// T
	_instrument->DrawPixel(px, py+3);
	for(int i=0; i<4; ++i) _instrument->DrawPixel(px+1, py+i);
	_instrument->DrawPixel(px+2, py+3);
}

void KLN89::DrawMessageAlert() {
	// TODO - draw the proper message indicator
	if(!_blink) {
		int px = _xBorder + _xFieldBorder[1] + _xFieldStart[1];
		int py = 1 * 9 + _yBorder + _yFieldBorder[1] + _yFieldStart[1] + 1;
		
		px++;	// Not sure why we need px++, but it seems to work!
		py++;

		DrawText("  ", 1, 0, 1, false, 99);
		_instrument->DrawQuad(px+1, py-1, px+2, py+5, true);
		_instrument->DrawQuad(px+3, py+3, px+3, py+5, true);
		_instrument->DrawQuad(px+4, py+2, px+4, py+4, true);
		_instrument->DrawQuad(px+5, py+1, px+6, py+3, true);
		_instrument->DrawQuad(px+7, py+2, px+7, py+4, true);
		_instrument->DrawQuad(px+8, py+3, px+8, py+5, true);
		_instrument->DrawQuad(px+9, py-1, px+10, py+5, true);
	}
}

void KLN89::Underline(int field, int px, int py, int len) {
	px = px * 7 + _xBorder + _xFieldBorder[field] + _xFieldStart[field];
	py = py * 9 + _yBorder + _yFieldBorder[field] + _yFieldStart[field];
	for(int i=0; i<(len*7); ++i) {
		_instrument->DrawPixel(px, py);
		++px;
	}
}

void KLN89::DrawKPH(int field, int cx, int cy) {
	// Add some border
	int px = cx * 7 + _xBorder + _xFieldBorder[field] + _xFieldStart[field];
	int py = cy * 9 + _yBorder + _yFieldBorder[field] + _yFieldStart[field];
	
	px++;
	py++;
	
	for(int j=0; j<=4; ++j) {
		_instrument->DrawPixel(px, py + 2 +j);
		_instrument->DrawPixel(px + 8, py + j);
		if(j <= 1) {
			_instrument->DrawPixel(px + 11, py + j);
			_instrument->DrawPixel(px + 9 + j, py + 2);
		}
	}
	
	for(int i=0; i<=6; ++i) {
		if(i <= 2) {
			_instrument->DrawPixel(px + 1 + i, py + 4 + i);
			_instrument->DrawPixel(px + 1 + i, py + (4 - i));
		}
		_instrument->DrawPixel(px + 2 + i, py + i);
	}
}

void KLN89::DrawDTO(int field, int cx, int cy) {
	DrawSpecialChar(6, field, cx, cy);
	if(!(_waypointAlert && _blink)) {
		DrawSpecialChar(3, field, cx+1, cy);
	}
	
	int px = cx * 7 + _xBorder + _xFieldBorder[field] + _xFieldStart[field];
	int py = cy * 9 + _yBorder + _yFieldBorder[field] + _yFieldStart[field];
	
	px++;
	py++;
	
	// Fill in the gap between the 'D' and the arrow.
	_instrument->DrawPixel(px+5, py+3);
}

// Takes character position
void KLN89::DrawChar(char c, int field, int px, int py, bool bold, bool invert) {
	// Ignore field for now
	// Add some border
	px = px * 7 + _xBorder + _xFieldBorder[field] + _xFieldStart[field];
	py = py * 9 + _yBorder + _yFieldBorder[field] + _yFieldStart[field];
	
	// Draw an orange background for inverted characters
	if(invert) {
		for(int i=0; i<7; ++i) {
			for(int j=0; j<9; ++j) {
				_instrument->DrawPixel(px + i, py + j);
			}
		}
	}
				
	if(c < 33) return;  // space
	
	// Render normal decimal points in bold floats
	if(c == '.') bold = false;
	
	++py;	// Shift the char up by one pixel
	for(int j=7; j>=0; --j) {
		char c1 = (bold ? NumbersBold[c-48][j] : UpperAlpha[c-33][j]);
		// Don't do the last column for now (ie. j = 1, not 0)
		for(int i=5; i>=0; --i) {
			if(c1 & (01 << i)) {
				_instrument->DrawPixel(px, py, invert);
			}
			++px;
		}
		px -= 6;
		++py;
	}
}

// Takes pixel position
void KLN89::DrawFreeChar(char c, int x, int y, bool draw_background) {
	
	if(draw_background) {
		_instrument->DrawQuad(x, y, x+6, y+8, true);
	}		
				
	if(c < 33) return;  // space
	
	++y;	// Shift the char up by one pixel
	for(int j=7; j>=0; --j) {
		char c1 = UpperAlpha[c-33][j];
		// Don't do the last column for now (ie. j = 1, not 0)
		for(int i=5; i>=0; --i) {
			if(c1 & (01 << i)) {
				_instrument->DrawPixel(x, y);
			}
			++x;
		}
		x -= 6;
		++y;
	}
}

// Takes instrument pixel co-ordinates.
// Position is specified by the bottom of the *visible* portion, by default the left position unless align_right is true.
// The return value is the pixel width of the visible portion
int KLN89::DrawSmallChar(char c, int x, int y, bool align_right) {
	// calculate the index into the SmallChar array
	int idx;
	if(c > 47 && c < 58) {
		// number
		idx = c - 48;
	} else if(c > 64 && c < 91) {
		// Uppercase letter
		idx = c - 55;
	} else {
		return(0);
	}
	
	char n = SmallChar[idx][0];		// Width of visible portion
	if(align_right) x -= n;
	
	// background
	_instrument->DrawQuad(x - 1, y - 1, x + n, y + 5, true);
	
	for(int j=7; j>=3; --j) {
		char c1 = SmallChar[idx][j];
		for(int i=n-1; i>=0; --i) {
			if(c1 & (01 << i)) {
				_instrument->DrawPixel(x, y);
			}
			++x;
		}
		x -= n;
		++y;
	}
	
	return(n);
}

// Takes character position
void KLN89::DrawSpecialChar(char c, int field, int cx, int cy, bool bold) {
	if(c > 7) {
		cout << "ERROR - requested special char outside array bounds!\n";
		return;  // Increment this as SpecialChar grows
	}
	
	// Convert character to pixel position.
	// Ignore field for now
	// Add some border
	int px = cx * 7 + _xBorder + _xFieldBorder[field] + _xFieldStart[field];
	int py = cy * 9 + _yBorder + _yFieldBorder[field] + _yFieldStart[field];
	++py;	// Total hack - the special chars were coming out 1 pixel too low!
	for(int i=7; i>=0; --i) {
		char c1 = SpecialChar[(int)c][i];
		// Don't do the last column for now (ie. j = 1, not 0)
		for(int j=5; j>=0; --j) {
			if(c1 & (01 << j)) {
				_instrument->DrawPixel(px, py);
			}
			++px;
		}
		px -= 6;
		++py;
	}
}

void KLN89::DrawText(const string& s, int field, int px, int py, bool bold, int invert) {
	for(int i = 0; i < (int)s.size(); ++i) {
		DrawChar(s[(unsigned int)i], field, px+i, py, bold, (invert == i || invert == 99));
	}
}

void KLN89::DrawMapText(const string& s, int x, int y, bool draw_background) {
	MapToInstrument(x, y);
	if(draw_background) {
		//_instrument->DrawQuad(x, y, x + (7 * s.size()) - 1, y + 8, true);
		_instrument->DrawQuad(x - 1, y, x + (7 * s.size()) - 2, y + 8, true);
		// The minus 1 and minus 2 are an ugly hack to disguise the fact that I've lost track of exactly what's going on!
	}
	
	for(int i = 0; i < (int)s.size(); ++i) {
		DrawFreeChar(s[(unsigned int)i], x+(i * 7)-1, y);
	}
}

void KLN89::DrawLatitude(double d, int field, int px, int py) {
	DrawChar((d >= 0 ? 'N' : 'S'), field, px, py);
	d = fabs(d);
	px += 1;
	// TODO - sanity check input to ensure major lat field can only ever by 2 chars wide
	char buf[8];
	// Don't know whether to zero pad the below for single digits or not?
	//cout << d << ", " << (int)d << '\n';
	// 3 not 2 in size before for trailing \0
	int n = snprintf(buf, 3, "%i", (int)d);
	string s = buf;
	//cout << s << "... " << n << '\n';
	DrawText(s, field, px+(3-n), py);
	n = snprintf(buf, 7, "%05.2f'", ((double)(d - (int)d)) * 60.0f);
	s = buf;
	px += 3;
	DrawSpecialChar(0, field, px, py);	// Degrees symbol
	px++;
	DrawText(s, field, px, py);
}

void KLN89::DrawLongitude(double d, int field, int px, int py) {
	DrawChar((d >= 0 ? 'E' : 'W'), field, px, py);
	d = fabs(d);
	px += 1;
	// TODO - sanity check input to ensure major lat field can only ever be 2 chars wide
	char buf[8];
	// Don't know whether to zero pad the below for single digits or not?
	//cout << d << ", " << (int)d << '\n';
	// 4 not 3 in size before for trailing \0
	int n = snprintf(buf, 4, "%i", (int)d);
	string s = buf;
	//cout << s << "... " << n << '\n';
	DrawText(s, field, px+(3-n), py);
	n = snprintf(buf, 7, "%05.2f'", ((double)(d - (int)d)) * 60.0f);
	s = buf;
	px += 3;
	DrawSpecialChar(0, field, px, py);	// Degrees symbol
	px++;
	DrawText(s, field, px, py);
}

void KLN89::DrawFreq(double d, int field, int px, int py) {
	if(d >= 1000) d /= 100.0f;
	char buf[8];
	snprintf(buf, 7, "%6.2f", d);
	string s = buf;
	DrawText(s, field, px, py);
}

void KLN89::DrawTime(double time, int field, int px, int py) {
	int hrs = (int)(time / 3600);
	int mins = (int)(ceil((time - (hrs * 3600)) / 60.0));
	char buf[10];
	int n;
	if(time >= 60.0) {
		// Draw hr:min
		n = snprintf(buf, 9, "%i:%02i", hrs, mins);
	} else {
		// Draw :secs
		n = snprintf(buf, 4, ":%02i", (int)time);
	}
	string s = buf;
	DrawText(s, field, px - n + 1, py);
}

void KLN89::DrawHeading(int h, int field, int px, int py) {
	char buf[4];
	snprintf(buf, 4, "%i", h);
	string s = buf;
	DrawText(s, field, px - s.size(), py);
	DrawSpecialChar(0, field, px, py);	// Degrees symbol
}

void KLN89::DrawDist(double d, int field, int px, int py) {
	d *= (_distUnits == GPS_DIST_UNITS_NM ? 1.0 : SG_NM_TO_METER * 0.001);
	char buf[10];
	snprintf(buf, 9, "%i", (int)(d + 0.5));
	string s = buf;
	s += (_distUnits == GPS_DIST_UNITS_NM ? "nm" : "Km");
	DrawText(s, field, px - s.size() + 1, py);
}

void KLN89::DrawSpeed(double v, int field, int px, int py, int decimal) {
	// TODO - implement variable decimal places
	v *= (_velUnits == GPS_VEL_UNITS_KT ? 1.0 : 0.51444444444 * 0.001 * 3600.0);
	char buf[10];
	snprintf(buf, 9, "%i", (int)(v + 0.5));
	string s = buf;
	if(_velUnits == GPS_VEL_UNITS_KT) {
		s += "kt";
		DrawText(s, field, px - s.size() + 1, py);
	} else {
		DrawText(s, field, px - s.size() - 1, py);
		DrawKPH(field, px - 1, py);
	}
}

void KLN89::DrawDirDistField(double lat, double lon, int field, int px, int py, bool to_flag, bool cursel) {
	DrawChar('>', field, px, py);
	char buf[8];
	double h;
	if(to_flag) {
		h = GetMagHeadingFromTo(_gpsLat, _gpsLon, lat, lon);
	} else {
		h = GetMagHeadingFromTo(lat, lon, _gpsLat, _gpsLon);
	}
	while(h < 0.0) h += 360.0;
	while(h > 360.0) h -= 360.0;
	snprintf(buf, 4, "%3i", (int)(h + 0.5));
	string s = buf;
	if(!(cursel && _blink)) { 
		DrawText(s, field, px + 4 - s.size(), py);
		DrawSpecialChar(0, field, px+4, py);
		DrawText((to_flag ? "To" : "Fr"), field, px+5, py);
		if(cursel) Underline(field, px + 1, py, 6);
	}
	//double d = GetHorizontalSeparation(_gpsLat, _gpsLon, lat, lon);
	//d *= (_distUnits == GPS_DIST_UNITS_NM ? SG_METER_TO_NM : 0.001);
	double d = GetGreatCircleDistance(_gpsLat, _gpsLon, lat, lon);
	d *= (_distUnits == GPS_DIST_UNITS_NM ? 1.0 : SG_NM_TO_METER * 0.001);
	if(d >= 100.0) {
		snprintf(buf, 7, "%5i", (int)(d + 0.5));
	} else {
		snprintf(buf, 7, "%4.1f", d);
	}
	s = buf;
	DrawText(s, field, px + 12 - s.size(), py);
	DrawText((_distUnits == GPS_DIST_UNITS_NM ? "nm" : "Km"), field, px + 12, py);
}

char KLN89::IncChar(char c, bool gap, bool wrap) {
	if(c == '9') return(wrap ? (gap ? ' ' : 'A') : '9');
	if(c == 'Z') return('0');
	if(c == ' ') return('A');
	return(c + 1);
}

char KLN89::DecChar(char c, bool gap, bool wrap) {
	if(c == 'A') return(wrap ? (gap ? ' ' : '9') : 'A');
	if(c == '0') return('Z');
	if(c == ' ') return('9');
	return(c - 1);
}
