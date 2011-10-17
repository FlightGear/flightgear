// kln89_page_*.[ch]xx - this file is one of the "pages" that
//                       are used in the KLN89 GPS unit simulation. 
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "kln89_page_dir.hxx"
#include <Main/fg_props.hxx>

using std::string;

KLN89DirPage::KLN89DirPage(KLN89* parent)
: KLN89Page(parent) {
	_nSubPages = 1;
	_subPage = 0;
	_name = "DIR";
	_maxULinePos = 4;
	_DToWpDispMode = 2;
}

KLN89DirPage::~KLN89DirPage() {
}

void KLN89DirPage::Update(double dt) {
	// TODO - this can apparently be "ACTIVATE:" under some circumstances
	_kln89->DrawText("DIRECT TO:", 2, 2, 3);
	
	if(_kln89->_mode == KLN89_MODE_CRSR) {
		string s = _id;
		while(s.size() < 5) s += ' ';
		if(_DToWpDispMode == 0) {
			if(!_kln89->_blink) {
				_kln89->DrawText(s, 2, 4, 1, false, 99);
				_kln89->DrawEnt(1, 0, 1);
			}
		} else if(_DToWpDispMode == 1) {
			if(!_kln89->_blink) {
				_kln89->DrawText(s, 2, 4, 1, false, _uLinePos);
				_kln89->DrawEnt(1, 0, 1);
			}
			_kln89->Underline(2, 4, 1, 5);
		} else {
			if(!_kln89->_blink) _kln89->DrawText("_____", 2, 4, 1);
			_kln89->Underline(2, 4, 1, 5);
		}
	} else {
		_kln89->DrawText("_____", 2, 4, 1);
	}
	
	KLN89Page::Update(dt);
}

// This can only be called from the KLN89 when DTO is pressed from outside of the DIR page.
// DO NOT USE IT to set _id internally from the DIR page, since it initialises various state 
// based on the assumption that the DIR page is being first entered.
void KLN89DirPage::SetId(const string& s) {
	if(s.size()) {
		_id = s;
		_DToWpDispMode = 0;
		if(!_kln89->_activeFP->IsEmpty()) {
			_DToWpDispIndex = (int)_kln89->_activeFP->waypoints.size() - 1;
		}
	} else {
		_DToWpDispMode = 2;
	}
	_saveMasterMode = _kln89->_mode;
	_uLinePos = 1;	// Needed to stop Leg flashing
}

void KLN89DirPage::CrsrPressed() {
	// Pressing CRSR clears the ID field (from sim).
	_DToWpDispMode = 2;
}

void KLN89DirPage::ClrPressed() {
	if(_kln89->_mode == KLN89_MODE_CRSR) {
		if(_DToWpDispMode <= 1) {
			_DToWpDispMode = 2;
			_id.clear();
		} else {
			// Restore the original master mode
			_kln89->_mode = _saveMasterMode;
			// Stop displaying dir page
			_kln89->_activePage = _kln89->_pages[_kln89->_curPage];
		}
	} else {
		// TODO
	}
}

void KLN89DirPage::EntPressed() {
	// Trim any RH whitespace from _id
	while(!_id.empty()) {
		if(_id[_id.size()-1] == ' ') {
			_id = _id.substr(0, _id.size()-1);
		} else {
			// Important to break, since usr waypoint names may contain space.
			break;
		}
	}
	if(_DToWpDispMode == 2 || _id.empty()) {
		_kln89->DtoCancel();
	} else {
		if(_DToWpDispMode == 0) {
			// It's a waypoint from the active flightplan - these get processed without data page review.
			_kln89->DtoInitiate(_id);
		} else {
			// Display the appropriate data page for review (USR page if the ident is not currently valid)
			_kln89->_dtoReview = true;
			GPSWaypoint* wp = _kln89->FindFirstByExactId(_id);
			if(wp) {
				// Set the current page to be the appropriate data page
				_kln89->_curPage = wp->type;
				delete wp;
			} else {
				// Set the current page to be the user page
				_kln89->_curPage = 4;
			}
			// set the page ID and entInvert, and activate the current page.
			_kln89->_activePage = _kln89->_pages[_kln89->_curPage];
			_kln89->_activePage->SetId(_id);
			_kln89->_activePage->SetEntInvert(true);
		}
	}
}

void KLN89DirPage::Knob2Left1() {
	if(_kln89->_mode == KLN89_MODE_CRSR) {
		if(fgGetBool("/instrumentation/kln89/scan-pull")) {
			if(_DToWpDispMode == 2) {
				if(!_kln89->_activeFP->IsEmpty()) {
					// Switch to mode 0, set the position to the end of the active flightplan *and* run the mode 0 case.
					_DToWpDispMode = 0;
					_DToWpDispIndex = (int)_kln89->_activeFP->waypoints.size() - 1;
				}
			}
			if(_DToWpDispMode == 0) {
				// If the knob is pulled out, then the unit cycles through the waypoints of the active flight plan
				// (This is deduced from the Bendix-King sim, I haven't found it documented in the pilot guide).
				// If the active flight plan is empty it clears the field (this is possible, e.g. if a data page was
				// active when DTO was pressed).
				if(!_kln89->_activeFP->IsEmpty()) {
					if(_DToWpDispIndex == 0) {
						_DToWpDispIndex = (int)_kln89->_activeFP->waypoints.size() - 1;
					} else {
						_DToWpDispIndex--;
					}
					_id = _kln89->_activeFP->waypoints[_DToWpDispIndex]->id;
				} else {
					_DToWpDispMode = 2;
				}
			}
			// _DToWpDispMode == 1 is a NO-OP when the knob is out.
		} else {
			if(_DToWpDispMode == 0) {
				// If the knob is not pulled out, then turning it transitions the DIR page to the waypoint selection mode
				// and sets the waypoint to the first beginning with '9'
				_id = "9";
				GPSWaypoint* wp = _kln89->FindFirstById(_id);
				if(wp) {
					_id = wp->id;
					delete wp;
				}
				_uLinePos = 0;
				_DToWpDispMode = 1;
			} else if(_DToWpDispMode == 1) {
				while(_id.size() < (_uLinePos + 1)) {
					_id += ' ';
				}
				char ch = _id[_uLinePos];
				if(ch == ' ') {
					ch = '9';
				} else if(ch == '0') {
					ch = 'Z';
				} else if(ch == 'A') {
					// It seems that blanks are allowed within the name, but not for the first character
					if(_uLinePos == 0) {
						ch = '9';
					} else {
						ch = ' ';
					}
				} else {
					ch--;
				}
				_id[_uLinePos] = ch;
				GPSWaypoint* wp = _kln89->FindFirstById(_id.substr(0, _uLinePos+1));
				if(wp) {
					_id = wp->id;
					delete wp;
				}
			} else {
				_id = "9";
				GPSWaypoint* wp = _kln89->FindFirstById(_id);
				if(wp) {
					_id = wp->id;
					delete wp;
				}
				_uLinePos = 0;
				_DToWpDispMode = 1;
			}
		}
	} else {
		// If the cursor is not displayed, then we return to the page that was displayed prior to DTO being pressed,
		// and pass the knob turn to that page, whether pulled out or not.
		_kln89->_activePage = _kln89->_pages[_kln89->_curPage];
		_kln89->_activePage->Knob2Left1();
	}
}

void KLN89DirPage::Knob2Right1() {
	if(_kln89->_mode == KLN89_MODE_CRSR) {
		if(fgGetBool("/instrumentation/kln89/scan-pull")) {
			if(_DToWpDispMode == 2) {
				if(!_kln89->_activeFP->IsEmpty()) {
					// Switch to mode 0, set the position to the end of the active flightplan *and* run the mode 0 case.
					_DToWpDispMode = 0;
					_DToWpDispIndex = (int)_kln89->_activeFP->waypoints.size() - 1;
				}
			}
			if(_DToWpDispMode == 0) {
				// If the knob is pulled out, then the unit cycles through the waypoints of the active flight plan
				// (This is deduced from the Bendix-King sim, I haven't found it documented in the pilot guide).
				// If the active flight plan is empty it clears the field (this is possible, e.g. if a data page was
				// active when DTO was pressed).
				if(!_kln89->_activeFP->IsEmpty()) {
					if(_DToWpDispIndex == (int)_kln89->_activeFP->waypoints.size() - 1) {
						_DToWpDispIndex = 0;
					} else {
						_DToWpDispIndex++;
					}
					_id = _kln89->_activeFP->waypoints[_DToWpDispIndex]->id;
				} else {
					_DToWpDispMode = 2;
				}
			}
			// _DToWpDispMode == 1 is a NO-OP when the knob is out.
		} else {
			if(_DToWpDispMode == 0) {
				// If the knob is not pulled out, then turning it transitions the DIR page to the waypoint selection mode
				// and sets the waypoint to the first beginning with 'A'
				_id = "A";
				GPSWaypoint* wp = _kln89->FindFirstById(_id);
				if(wp) {
					_id = wp->id;
					delete wp;
				}
				_uLinePos = 0;
				_DToWpDispMode = 1;
			} else if(_DToWpDispMode == 1) {
				while(_id.size() < (_uLinePos + 1)) {
					_id += ' ';
				}
				char ch = _id[_uLinePos];
				if(ch == ' ') {
					ch = 'A';
				} else if(ch == 'Z') {
					ch = '0';
				} else if(ch == '9') {
					// It seems that blanks are allowed within the name, but not for the first character
					if(_uLinePos == 0) {
						ch = 'A';
					} else {
						ch = ' ';
					}
				} else {
					ch++;
				}
				_id[_uLinePos] = ch;
				GPSWaypoint* wp = _kln89->FindFirstById(_id.substr(0, _uLinePos+1));
				if(wp) {
					_id = wp->id;
					delete wp;
				}
			} else {
				_id = "A";
				GPSWaypoint* wp = _kln89->FindFirstById(_id);
				if(wp) {
					_id = wp->id;
					delete wp;
				}
				_uLinePos = 0;
				_DToWpDispMode = 1;
			}
		}
	} else {
		// If the cursor is not displayed, then we return to the page that was displayed prior to DTO being pressed,
		// and pass the knob turn to that page, whether pulled out or not.
		_kln89->_activePage = _kln89->_pages[_kln89->_curPage];
		_kln89->_activePage->Knob2Right1();
	}
}
