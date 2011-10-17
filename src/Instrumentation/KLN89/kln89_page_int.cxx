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

#include "kln89_page_int.hxx"
#include <Navaids/fix.hxx>
#include <Navaids/navrecord.hxx>

using std::string;

KLN89IntPage::KLN89IntPage(KLN89* parent) 
: KLN89Page(parent) {
	_nSubPages = 2;
	_subPage = 0;
	_name = "INT";
	_int_id = "PORTE";
	_last_int_id = "";
	_fp = NULL;
	_nearestVor = NULL;
	_refNav = NULL;
}

KLN89IntPage::~KLN89IntPage() {
}

void KLN89IntPage::Update(double dt) {
	bool actPage = (_kln89->_activePage->GetName() == "ACT" ? true : false);
	bool multi;  // Not set by FindFirst...
	bool exact = false;
	if(_int_id.size() == 5) exact = true;
	if(_fp == NULL) {
		_fp = _kln89->FindFirstIntById(_int_id, multi, exact);
	} else if(_fp->get_ident() != _int_id) {
		_fp = _kln89->FindFirstIntById(_int_id, multi, exact);
	}
	
	if(_fp) {
		_int_id = _fp->get_ident();
		if(_kln89->GetActiveWaypoint()) {
			if(_int_id == _kln89->GetActiveWaypoint()->id) {
				if(!(_kln89->_waypointAlert && _kln89->_blink)) {
					// Active waypoint arrow
					_kln89->DrawSpecialChar(4, 2, 0, 3);
				}
			}
		}
		if(_int_id != _last_int_id) {
			_nearestVor = _kln89->FindClosestVor(_fp->get_lat() * SG_DEGREES_TO_RADIANS, _fp->get_lon() * SG_DEGREES_TO_RADIANS);
			if(_nearestVor) {
				_nvRadial = _kln89->GetMagHeadingFromTo(_nearestVor->get_lat() * SG_DEGREES_TO_RADIANS, _nearestVor->get_lon() * SG_DEGREES_TO_RADIANS,
			                                            _fp->get_lat() * SG_DEGREES_TO_RADIANS, _fp->get_lon() * SG_DEGREES_TO_RADIANS);
				_nvDist = _kln89->GetGreatCircleDistance(_nearestVor->get_lat() * SG_DEGREES_TO_RADIANS, _nearestVor->get_lon() * SG_DEGREES_TO_RADIANS,
			                                             _fp->get_lat() * SG_DEGREES_TO_RADIANS, _fp->get_lon() * SG_DEGREES_TO_RADIANS);
				_refNav = _nearestVor;	// TODO - check that this *always* holds - eg. when changing INT id after explicitly setting ref nav 
				    					// but with no loss of focus.
			} else {
				_refNav = NULL;
			}
			_last_int_id = _int_id;
		}
		if(_kln89->_mode != KLN89_MODE_CRSR) {
			if(!_entInvert) {
				if(!actPage) {
					_kln89->DrawText(_fp->get_ident(), 2, 1, 3);
				} else {
					// If it's the ACT page, The ID is shifted slightly right to make space for the waypoint index.
					_kln89->DrawText(_fp->get_ident(), 2, 4, 3);
					char buf[3];
					int n = snprintf(buf, 3, "%i", _kln89->GetActiveWaypointIndex() + 1);
					_kln89->DrawText((string)buf, 2, 3 - n, 3);
					// We also draw an I to differentiate INT from USR when it's the ACT page
					_kln89->DrawText("I", 2, 11, 3);
				}
			} else {
				if(!_kln89->_blink) {
					_kln89->DrawText(_fp->get_ident(), 2, 1, 3, false, 99);
					_kln89->DrawEnt();
				}
			}
		}
		if(_subPage == 0) {
			_kln89->DrawLatitude(_fp->get_lat(), 2, 3, 2);
			_kln89->DrawLongitude(_fp->get_lon(), 2, 3, 1);
			_kln89->DrawDirDistField(_fp->get_lat() * SG_DEGREES_TO_RADIANS, _fp->get_lon() * SG_DEGREES_TO_RADIANS, 2, 0, 0, 
			                         _to_flag, (_kln89->_mode == KLN89_MODE_CRSR && _uLinePos == 6 ? true : false));
		} else {
			_kln89->DrawText("Ref:", 2, 1, 2);
			_kln89->DrawText("Rad:", 2, 1, 1);
			_kln89->DrawText("Dis:", 2, 1, 0);
			if(_refNav) {
				_kln89->DrawText(_refNav->get_ident(), 2, 9, 2);	// TODO - flash and allow to change if under cursor
				//_kln89->DrawHeading(_nvRadial, 2, 11, 1);
				//_kln89->DrawDist(_nvDist, 2, 11, 0);
				// Currently our draw heading and draw dist functions don't do as many decimal points as we want here,
				// so draw it ourselves!
				// Heading
				char buf[10];
				snprintf(buf, 6, "%.1f", _nvRadial);
				string s = buf;
				_kln89->DrawText(s, 2, 13 - s.size(), 1);
				_kln89->DrawSpecialChar(0, 2, 13, 1);	// Degrees symbol
				// Dist
				double d = _nvDist;
				d *= (_kln89->_distUnits == GPS_DIST_UNITS_NM ? 1.0 : SG_NM_TO_METER * 0.001);
				snprintf(buf, 9, "%.1f", d);
				s = buf;
				s += (_kln89->_distUnits == GPS_DIST_UNITS_NM ? "nm" : "Km");
				_kln89->DrawText(s, 2, 14 - s.size(), 0);
			}
		}
	} else {
		// TODO - when we leave the page with invalid id and return it should
		// revert to showing the last valid id.  Same for vor/ndb/probably apt etc.
		if(_kln89->_mode != KLN89_MODE_CRSR) _kln89->DrawText(_int_id, 2, 1, 3);
		if(_subPage == 0) {
			_kln89->DrawText("- -- --.--'", 2, 3, 2);
			_kln89->DrawText("---- --.--'", 2, 3, 1);
			_kln89->DrawSpecialChar(0, 2, 7, 2);
			_kln89->DrawSpecialChar(0, 2, 7, 1);
			_kln89->DrawText(">---    ----", 2, 0, 0);
			_kln89->DrawSpecialChar(0, 2, 4, 0);
			_kln89->DrawText(_to_flag ? "To" : "Fr", 2, 5, 0);
			_kln89->DrawText(_kln89->_distUnits == GPS_DIST_UNITS_NM ? "nm" : "km", 2, 12, 0);
		}
	}
	
	if(_kln89->_mode == KLN89_MODE_CRSR) {
		if(_uLinePos > 0 && _uLinePos < 6) {
			// TODO - blink as well
			_kln89->Underline(2, _uLinePos, 3, 1);
		}
		for(unsigned int i = 0; i < _int_id.size(); ++i) {
			if(_uLinePos != (i + 1)) {
				_kln89->DrawChar(_int_id[i], 2, i + 1, 3);
			} else {
				if(!_kln89->_blink) _kln89->DrawChar(_int_id[i], 2, i + 1, 3);
			}
		}
	}
	
	// TODO - fix this duplication - use _id instead of _apt_id, _vor_id, _ndb_id, _int_id etc!
	_id = _int_id;
	
	KLN89Page::Update(dt);
}

void KLN89IntPage::SetId(const string& s) {
	_last_int_id = _int_id;
	_save_int_id = _int_id;
	_int_id = s;
	_fp = NULL;
}

void KLN89IntPage::CrsrPressed() {
	if(_kln89->_mode == KLN89_MODE_DISP) return;
	if(_kln89->_obsMode) {
		_uLinePos = 0;
	} else {
		_uLinePos = 1;
	}
	if(_subPage == 0) {
		_maxULinePos = 6;
	} else {
		_maxULinePos = 6;
	}
}

void KLN89IntPage::ClrPressed() {
	if(_subPage == 0 && _uLinePos == 6) {
		_to_flag = !_to_flag;
	}
}

void KLN89IntPage::EntPressed() {
	if(_entInvert) {
		_entInvert = false;
		_entInvert = false;
		if(_kln89->_dtoReview) {
			_kln89->DtoInitiate(_int_id);
		} else {
			_last_int_id = _int_id;
			_int_id = _save_int_id;
		}
	}
}

void KLN89IntPage::Knob2Left1() {
	if(_kln89->_mode != KLN89_MODE_CRSR || _uLinePos == 0) {
		KLN89Page::Knob2Left1();
	} else {
		if(_uLinePos < 6) {
			// Same logic for both pages - set the ID
			_int_id = _int_id.substr(0, _uLinePos);
			// ASSERT(_uLinePos > 0);
			if(_uLinePos == (_int_id.size() + 1)) {
				_int_id += '9';
			} else {
				_int_id[_uLinePos - 1] = _kln89->DecChar(_int_id[_uLinePos - 1], (_uLinePos == 1 ? false : true));
			}
		} else {
			if(_subPage == 0) {
				// NO-OP - from/to field is switched by clr button, not inner knob.
			} else {
				// TODO - LNR type field.
			}
		}
	}
}

void KLN89IntPage::Knob2Right1() {
	if(_kln89->_mode != KLN89_MODE_CRSR || _uLinePos == 0) {
		KLN89Page::Knob2Right1();
	} else {
		if(_uLinePos < 6) {
			// Same logic for both pages - set the ID
			_int_id = _int_id.substr(0, _uLinePos);
			// ASSERT(_uLinePos > 0);
			if(_uLinePos == (_int_id.size() + 1)) {
				_int_id += 'A';
			} else {
				_int_id[_uLinePos - 1] = _kln89->IncChar(_int_id[_uLinePos - 1], (_uLinePos == 1 ? false : true));
			}
		} else {
			if(_subPage == 0) {
				// NO-OP - from/to field is switched by clr button, not inner knob.
			} else {
				// TODO - LNR type field.
			}
		}
	}
}

