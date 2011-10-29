// kln89_page.cxx - base class for the "pages" that
//                  are used in the KLN89 GPS unit simulation. 
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

#include "kln89_page.hxx"
#include <Main/fg_props.hxx>

using std::string;

KLN89Page::KLN89Page(KLN89* parent) {
	_kln89 = parent;
	_entInvert = false;
	_to_flag = true;
	_subPage = 0;
}

KLN89Page::~KLN89Page() {
}

void KLN89Page::Update(double dt) {
	bool crsr = (_kln89->_mode == KLN89_MODE_CRSR ? true : false);
	bool nav1 = (_name == "NAV" && _subPage == 0);
	bool nav4 = (_name == "NAV" && _subPage == 3);
	// The extra level of check for the ACT page is necessary since
	// ACT is implemented by using the other waypoint pages as
	// appropriate.
	bool act = (_kln89->_activePage->GetName() == "ACT");
	_kln89->DrawDivider();
	if(crsr) {
		if(!nav4) _kln89->DrawText("*CRSR*", 1, 0, 0);
		if(_uLinePos == 0) _kln89->Underline(1, 3, 1, 3);
	} else {
		if(!nav4) {
			if(act) {
				_kln89->DrawText("ACT", 1, 0, 0);
			} else {
				_kln89->DrawText(_name, 1, 0, 0);
			}
			if(_name == "DIR") {
				// Don't draw a subpage number
			} else if(_name == "USR" || _name == "FPL") {
				// Zero-based
				_kln89->DrawText(GPSitoa(_subPage), 1, 4, 0);
			} else {
				// One-based
				_kln89->DrawText(GPSitoa(_subPage+1), 1, 4, 0);
			}
		}
	}
	if(crsr && _uLinePos == 0 && _kln89->_blink) {
		// Don't draw
	} else {
		if(_kln89->_obsMode) {
			_kln89->DrawText(GPSitoa(_kln89->_obsHeading), 1, 3, 1);
		} else {
			_kln89->DrawText("Leg", 1, 3, 1);
		}
	}
	_kln89->DrawText((_kln89->GetDistVelUnitsSI() ? "km" : "nm"), 1, 4, 3);
	GPSWaypoint* awp = _kln89->GetActiveWaypoint();
	if(_kln89->_navFlagged) {
		_kln89->DrawText("--.-", 1, 0 ,3);
		// Only nav1 still gets speed drawn if nav is flagged - not ACT
		if(!nav1) _kln89->DrawText("------", 1, 0, 2);
	} else {
		char buf[8];
		float f = _kln89->GetDistToActiveWaypoint() * (_kln89->GetDistVelUnitsSI() ? 0.001 : SG_METER_TO_NM);
		snprintf(buf, 5, (f >= 100.0 ? "%4.0f" : "%4.1f"), f);
		string s = buf;
		_kln89->DrawText(s, 1, 4 - s.size(), 3, true);
		// Draw active waypoint ID, except for
		// nav1, act, and any waypoint pages matching 
		// active waypoint that need speed drawn instead.
		if(act || nav1 || (awp && awp->id == _id)) {
			_kln89->DrawSpeed(_kln89->_groundSpeed_kts, 1, 5, 2);
		} else {
			if(!(_kln89->_waypointAlert && _kln89->_blink)) _kln89->DrawText(awp->id, 1, 0, 2);
		}
	}
	/*
	if(_noNrst) {
		_kln89->DrawText("  No  ", 1, 0, 1, false, 99);
		_kln89->DrawText(" Nrst ", 1, 0, 0, false, 99);
	}
	*/
	if(_scratchpadMsg) {
		_kln89->DrawText(_scratchpadLine1, 1, 0, 1, false, 99);
		_kln89->DrawText(_scratchpadLine2, 1, 0, 0, false, 99);
		_scratchpadTimer += dt;
		if(_scratchpadTimer > 4.0) {
			_scratchpadMsg = false;
			_scratchpadTimer = 0.0;
		}
	}
}

void KLN89Page::ShowScratchpadMessage(const string& line1, const string& line2) {
	_scratchpadLine1 = line1;
	_scratchpadLine2 = line2;
	_scratchpadTimer = 0.0;
	_scratchpadMsg = true;
}	

void KLN89Page::Knob1Left1() {
	if(_kln89->_mode == KLN89_MODE_CRSR) {
		if(_uLinePos > 0) _uLinePos--;
	}
}

void KLN89Page::Knob1Right1() {
	if(_kln89->_mode == KLN89_MODE_CRSR) {
		if(_uLinePos < _maxULinePos) _uLinePos++;
	}
}

void KLN89Page::Knob2Left1() {
	if(_kln89->_mode != KLN89_MODE_CRSR && !fgGetBool("/instrumentation/kln89/scan-pull")) {
		_kln89->_activePage->LooseFocus();
		_subPage--;
		if(_subPage < 0) _subPage = _nSubPages - 1;
	} else {
		if(_uLinePos == 0 && _kln89->_obsMode) {
			_kln89->_obsHeading--;
			if(_kln89->_obsHeading < 0) {
				_kln89->_obsHeading += 360;
			}
			_kln89->SetOBSFromWaypoint();
		}
	}
}

void KLN89Page::Knob2Right1() {
	if(_kln89->_mode != KLN89_MODE_CRSR && !fgGetBool("/instrumentation/kln89/scan-pull")) {
		_kln89->_activePage->LooseFocus();
		_subPage++;
		if(_subPage >= _nSubPages) _subPage = 0;
	} else {
		if(_uLinePos == 0 && _kln89->_obsMode) {
			_kln89->_obsHeading++;
			if(_kln89->_obsHeading > 359) {
				_kln89->_obsHeading -= 360;
			}
			_kln89->SetOBSFromWaypoint();
		}
	}
}

void KLN89Page::CrsrPressed() {
	// Stick some sensible defaults in
	if(_kln89->_obsMode) {
		_uLinePos = 0;
	} else {
		_uLinePos = 1;
	}
	_maxULinePos = 1;
}

void KLN89Page::EntPressed() {}
void KLN89Page::ClrPressed() {}
void KLN89Page::DtoPressed() {}
void KLN89Page::NrstPressed() {}
void KLN89Page::AltPressed() {}

void KLN89Page::OBSPressed() {
	if(_kln89->_obsMode) {
		// If ORS2 and not slaved to gps
		_uLinePos = 0;
	} else {
		// Don't leave the cursor on in the leg position.
		if(_uLinePos == 0) {
			_kln89->_mode = KLN89_MODE_DISP;
		}
	}
}

void KLN89Page::MsgPressed() {}

void KLN89Page::CleanUp() {
	_kln89->_cleanUpPage = -1;
}

void KLN89Page::LooseFocus() {
	_entInvert = false;
}

void KLN89Page::SetId(const string& s) {
	_id = s;
}

void KLN89Page::SetSubPage(int n) {
	if(n < 0) n = 0;
	if(n >= _nSubPages) n = _nSubPages-1;
	_subPage = n;
}

const string& KLN89Page::GetId() {
	return(_id);
}

// TODO - this function probably shouldn't be here - FG almost certainly has better handling
// of this somewhere already.
string KLN89Page::GPSitoa(int n) {
	char buf[6];
	snprintf(buf, 6, "%i", n);
	string s = buf;
	return(s);
}
