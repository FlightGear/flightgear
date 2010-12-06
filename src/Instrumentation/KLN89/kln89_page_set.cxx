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

#include "kln89_page_set.hxx"

#include <iostream>
using namespace std;

KLN89SetPage::KLN89SetPage(KLN89* parent) 
: KLN89Page(parent) {
	_nSubPages = 11;
	_subPage = 0;
	_name = "SET";
}

KLN89SetPage::~KLN89SetPage() {
}

void KLN89SetPage::Update(double dt) {
	string sBaro, sAlt, sVel;
	switch(_subPage+1) {
	case 1:
		_kln89->DrawText("INIT POS:", 2, 0, 3);
		break;
	case 2:
		_kln89->DrawText("DATE", 2, 0, 3);
		_kln89->DrawText("TIME", 2, 0, 2);
		_kln89->DrawText("Cord", 2, 0, 1);
		_kln89->DrawText("Mag Var:", 2, 0, 0);
		break;
	case 3:
		_kln89->DrawText("Update DB on", 2, 1, 3);
		_kln89->DrawText("ground only", 2, 1, 2);
		_kln89->DrawText("Key", 2, 0, 1);
		_kln89->DrawText("Update pub DB?", 2, 0, 0);
		break;
	case 4:
		//cout << "_uLinePos = " << _uLinePos << '\n';
		_kln89->DrawText("TURN", 2, 5, 3);
		_kln89->DrawText("ANTICIPATION", 2, 1, 2);
		if(_kln89->_mode == KLN89_MODE_CRSR && _uLinePos == 1) {
			if(!_kln89->_blink) {
				_kln89->DrawText((_kln89->GetTurnAnticipation() ? "ENABLED" : "DISABLED"), 2, 3, 1);
			}
			_kln89->Underline(2, 3, 1, 8);
		} else {
			_kln89->DrawText((_kln89->GetTurnAnticipation() ? "ENABLED" : "DISABLED"), 2, 3, 1);
		}
		break;
	case 5:
		_kln89->DrawText("Default First", 2, 0, 3);
		_kln89->DrawText("Character of", 2, 1, 2);
		_kln89->DrawText("Wpt identifier", 2, 0, 1);
		_kln89->DrawText("Entry:", 2, 3, 0);
		if(_kln89->_mode == KLN89_MODE_CRSR && _uLinePos == 1) {
			if(!_kln89->_blink) {
				_kln89->DrawChar(_kln89->_defaultFirstChar, 2, 10, 0);
			}
			_kln89->Underline(2, 10, 0, 1);
		} else {
			_kln89->DrawChar(_kln89->_defaultFirstChar, 2, 10, 0);
		}
		break;
	case 6:
		_kln89->DrawText("NEAREST APT", 2, 1, 3);
		_kln89->DrawText("CRITERIA", 2, 3, 2);
		_kln89->DrawText("Length:", 2, 0, 1);
		_kln89->DrawText("Surface:", 2, 0, 0);
		break;
	case 7:
		_kln89->DrawText("SUA ALERT", 2, 3, 3);
		if(_kln89->_mode == KLN89_MODE_CRSR && _uLinePos == 1) {
			if(!_kln89->_blink) {
				_kln89->DrawText((_kln89->GetSuaAlertEnabled() ? "ENABLED" : "DISABLED"), 2, 4, 2);
			}
			_kln89->Underline(2, 4, 2, 8);
		} else {
			_kln89->DrawText((_kln89->GetSuaAlertEnabled() ? "ENABLED" : "DISABLED"), 2, 4, 2);
		}
		if(_kln89->GetSuaAlertEnabled()) {
			_kln89->DrawText("Buffer:", 2, 0, 1);
			_kln89->DrawSpecialChar(5, 2, 7, 1);	// +- sign.
			if(_kln89->_mode == KLN89_MODE_CRSR && _uLinePos == 2) {
				if(!_kln89->_blink) {
					_kln89->DrawText("00300", 2, 8, 1);		// TODO - fix this hardwiring!!!!
				}
				_kln89->Underline(2, 8, 1, 5);
			} else {
				_kln89->DrawText("00300", 2, 8, 1);		// TODO - fix this hardwiring!!!!
			}
			_kln89->DrawText("ft", 2, 13, 1);	// TODO - fix this hardwiring!!!!
		}
		break;
	case 8:
		_kln89->DrawText("SET UNITS:", 2, 3, 3);
		_kln89->DrawText("Baro    :", 2, 0, 2);
		_kln89->DrawText("Alt-APT :", 2, 0, 1);
		_kln89->DrawText("Dist-Vel:", 2, 0, 0);
		switch(_kln89->_baroUnits) {
		case GPS_PRES_UNITS_IN:
			sBaro = "\"";
			break;
		case GPS_PRES_UNITS_MB:
			sBaro = "mB";
			break;
		case GPS_PRES_UNITS_HP:
			sBaro = "hP";
			break;
		}
		if(_kln89->_altUnits == GPS_ALT_UNITS_FT) sAlt = "ft";
		else sAlt = "m";
		if(_kln89->_distUnits == GPS_DIST_UNITS_NM) sVel = "nm-kt";
		else sVel = "km-";
			
		if(_kln89->_mode == KLN89_MODE_CRSR && _uLinePos == 1) {
			if(!_kln89->_blink) {
				_kln89->DrawText(sBaro, 2, 10, 2);
			}
			_kln89->Underline(2, 10, 2, 2);
		} else {
			_kln89->DrawText(sBaro, 2, 10, 2);
		}
		if(_kln89->_mode == KLN89_MODE_CRSR && _uLinePos == 2) {
			if(!_kln89->_blink) {
				_kln89->DrawText(sAlt, 2, 10, 1);
			}
			_kln89->Underline(2, 10, 1, 2);
		} else {
			_kln89->DrawText(sAlt, 2, 10, 1);
		}
		if(_kln89->_mode == KLN89_MODE_CRSR && _uLinePos == 3) {
			if(!_kln89->_blink) {
				_kln89->DrawText(sVel, 2, 10, 0);
				if(_kln89->_distUnits != GPS_DIST_UNITS_NM) _kln89->DrawKPH(2, 13, 0);
			}
			_kln89->Underline(2, 10, 0, 5);
		} else {
			_kln89->DrawText(sVel, 2, 10, 0);
			if(_kln89->_distUnits != GPS_DIST_UNITS_NM) _kln89->DrawKPH(2, 13, 0);
		}
		break;
	case 9:
		_kln89->DrawText("Altitude", 2, 3, 3);
		_kln89->DrawText("Alert:", 2, 1, 2);
		break;
	case 10:
		_kln89->DrawText("BUS MONITOR", 2, 2, 3);
		_kln89->DrawText("Bus Volt", 2, 0, 2);
		_kln89->DrawText("Alert Volt", 2, 0, 1);
		_kln89->DrawText("Alert Delay", 2, 0, 0);
		break;
	case 11:
		_kln89->DrawText("MIN DISPLAY", 2, 2, 3);
		_kln89->DrawText("BRIGHTNESS ADJ", 2, 1, 2);
		if(_kln89->_mode == KLN89_MODE_CRSR && _uLinePos == 1) {
			if(!_kln89->_blink) {
				_kln89->DrawChar('0' + _kln89->GetMinDisplayBrightness(), 2, 6, 0);
			}
			_kln89->Underline(2, 6, 0, 1);
		} else {
			_kln89->DrawChar('0' + _kln89->GetMinDisplayBrightness(), 2, 6, 0);
		}
		if(_kln89->GetMinDisplayBrightness() == 4) {
			_kln89->DrawText("Default", 2, 8, 0);
		}
		break;
	}
	
	KLN89Page::Update(dt);
}

void KLN89SetPage::CrsrPressed() {
	if(_kln89->_mode == KLN89_MODE_DISP) return;
	if(_kln89->_obsMode) {
		_uLinePos = 0;
	} else {
		_uLinePos = 1;
	}
	switch(_subPage+1) {
	case 1:
		break;
	case 2:
		break;
	case 3:
		break;
	case 4:
		_maxULinePos = 1;
		break;
	case 5:
		_maxULinePos = 1;
		break;
	case 6:
		_maxULinePos = 2;
		break;
	case 7:
		if(_kln89->GetSuaAlertEnabled()) _maxULinePos = 2;
		else _maxULinePos = 1;
		break;
	case 8:
		_maxULinePos = 3;
		break;
	case 9:
		break;
	case 10:
		break;
	case 11:
		_maxULinePos = 1;
		break;
	}
}

void KLN89SetPage::Knob2Left1() {
	if(_kln89->_mode != KLN89_MODE_CRSR || _uLinePos == 0) {
		KLN89Page::Knob2Left1();
	} else {
		switch(_subPage+1) {
		case 1:
			break;
		case 2:
			break;
		case 3:
			break;
		case 4:
			if(_uLinePos == 1) {
				_kln89->SetTurnAnticipation(!_kln89->GetTurnAnticipation());
			}
			break;
		case 5:
			if(_uLinePos == 1) {
				_kln89->_defaultFirstChar = _kln89->DecChar(_kln89->_defaultFirstChar, false, true);
			}
			break;
		case 6:
			break;
		case 7:
			if(_uLinePos == 1) {
				_kln89->SetSuaAlertEnabled(!_kln89->GetSuaAlertEnabled());
				_maxULinePos = (_kln89->GetSuaAlertEnabled() ? 2 : 1);
			} else if(_uLinePos == 2) {
				// TODO - implement variable sua alert buffer
			}
			break;
		case 8:
			if(_uLinePos == 1) {  // baro units
				_kln89->SetBaroUnits(_kln89->GetBaroUnits() - 1, true);
			} else if(_uLinePos == 2) {
				_kln89->SetAltUnitsSI(!_kln89->GetAltUnitsSI());
			} else if(_uLinePos == 3) {
				_kln89->SetDistVelUnitsSI(!_kln89->GetDistVelUnitsSI());
			}
			break;
		case 11:
			if(_uLinePos == 1) {
				_kln89->DecrementMinDisplayBrightness();
			}
			break;
		}
	}
}

void KLN89SetPage::Knob2Right1() {
	if(_kln89->_mode != KLN89_MODE_CRSR || _uLinePos == 0) {
		KLN89Page::Knob2Right1();
	} else {
		switch(_subPage+1) {
		case 1:
			break;
		case 2:
			break;
		case 3:
			break;
		case 4:
			if(_uLinePos == 1) {   // Which it should be!
				_kln89->SetTurnAnticipation(!_kln89->GetTurnAnticipation());
			}
			break;
		case 5:
			if(_uLinePos == 1) {
				_kln89->_defaultFirstChar = _kln89->IncChar(_kln89->_defaultFirstChar, false, true);
			}
			break;
		case 6:
			break;
		case 7:
			if(_uLinePos == 1) {
				_kln89->SetSuaAlertEnabled(!_kln89->GetSuaAlertEnabled());
				_maxULinePos = (_kln89->GetSuaAlertEnabled() ? 2 : 1);
			} else if(_uLinePos == 2) {
				// TODO - implement variable sua alert buffer
			}
			break;
		case 8:
			if(_uLinePos == 1) {  // baro units
				_kln89->SetBaroUnits(_kln89->GetBaroUnits() + 1, true);
			} else if(_uLinePos == 2) {
				_kln89->SetAltUnitsSI(!_kln89->GetAltUnitsSI());
			} else if(_uLinePos == 3) {
				_kln89->SetDistVelUnitsSI(!_kln89->GetDistVelUnitsSI());
			}
			break;
		case 11:
			if(_uLinePos == 1) {
				_kln89->IncrementMinDisplayBrightness();
			}
			break;
		}
	}
}
