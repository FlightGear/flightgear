// kln89_page_*.[ch]xx - this file is one of the "pages" that
//                       are used in the KLN89 GPS unit simulation. 
//
// Written by David Luff, started 2010.
//
// Copyright (C) 2010 - David C Luff - daveluff AT ntlworld.com
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "kln89_page_alt.hxx"

using std::string;

KLN89AltPage::KLN89AltPage(KLN89* parent) 
: KLN89Page(parent) {
	_nSubPages = 2;
	_subPage = 0;
	_name = "ALT";
	_uLinePos = 1;
	_maxULinePos = 1;
}

KLN89AltPage::~KLN89AltPage() {
}

void KLN89AltPage::Update(double dt) {
	if(_subPage == 0) {
		_kln89->DrawText("BARO:", 2, 2, 3);
		if(_kln89->_baroUnits == GPS_PRES_UNITS_IN) {
			// If the units are not consistent with the setting, then convert to the correct
			// units.  We do it here instead of where the units are set in order to avoid any
			// possible value creep with multiple unit toggling.
			if(_kln89->_userBaroSetting >= 745 && _kln89->_userBaroSetting <= 1117) {
				_kln89->_userBaroSetting = (int)((float)_kln89->_userBaroSetting * 0.0295301 * 100 + 0.5);
			}
			char buf[6];
			snprintf(buf, 6, "%2i.%02i", _kln89->_userBaroSetting/100, _kln89->_userBaroSetting % 100);
			string s = buf;
			if(!(_kln89->_mode == KLN89_MODE_CRSR && _uLinePos == 1 && _kln89->_blink)) {
				_kln89->DrawText(s, 2, 7, 3);
			}
			if(_kln89->_mode == KLN89_MODE_CRSR && _uLinePos == 1) {
				_kln89->Underline(2, 7, 3, 5);
			}
			_kln89->DrawText("\"", 2, 12, 3);
		} else {
			// If the units are not consistent with the setting, then convert to the correct
			// units.  We do it here instead of where the units are set in order to avoid any
			// possible value creep with multiple unit toggling.
			if(_kln89->_userBaroSetting >= 2200 && _kln89->_userBaroSetting <= 3299) {
				_kln89->_userBaroSetting = (int)(((float)_kln89->_userBaroSetting / 100.0) * 33.8637526 + 0.5);
			}
			char buf[5];
			snprintf(buf, 5, "%4i", _kln89->_userBaroSetting);
			string s = buf;
			if(!(_kln89->_mode == KLN89_MODE_CRSR && _uLinePos == 1 && _kln89->_blink)) {
				_kln89->DrawText(s, 2, 8, 3);
			}
			if(_kln89->_mode == KLN89_MODE_CRSR && _uLinePos == 1) {
				_kln89->Underline(2, 8, 3, 4);
			}
			_kln89->DrawText(_kln89->_baroUnits == GPS_PRES_UNITS_MB ? "mB" : "hP", 2, 12, 3);
		}
				
		_kln89->DrawText("MSA", 2, 2, 1);
		_kln89->DrawText("ESA", 2, 2, 0);
		
		// At the moment we have no obstruction database, so dash out MSA & ESA
		_kln89->DrawText("----", 2, 8, 1);
		_kln89->DrawText("----", 2, 8, 0);
		if(_kln89->_altUnits == GPS_ALT_UNITS_FT) {
			_kln89->DrawText("ft", 2, 12, 1);
			_kln89->DrawText("ft", 2, 12, 0);
		} else {
			_kln89->DrawText("m", 2, 12, 1);
			_kln89->DrawText("m", 2, 12, 0);
		}
	} else {
		_kln89->DrawText("Vnv Inactive", 2, 0, 3);
	}

	KLN89Page::Update(dt);
}

void KLN89AltPage::CrsrPressed() {
}

void KLN89AltPage::EntPressed() {
}

void KLN89AltPage::Knob2Left1() {
	_kln89->_userBaroSetting--;
	if(_kln89->_baroUnits == GPS_PRES_UNITS_IN) {
		if(_kln89->_userBaroSetting < 2200) _kln89->_userBaroSetting = 3299;
	} else {
		if(_kln89->_userBaroSetting < 745) _kln89->_userBaroSetting = 1117;
	}
}

void KLN89AltPage::Knob2Right1() {
	_kln89->_userBaroSetting++;
	if(_kln89->_baroUnits == GPS_PRES_UNITS_IN) {
		if(_kln89->_userBaroSetting > 3299) _kln89->_userBaroSetting = 2200;
	} else {
		if(_kln89->_userBaroSetting > 1117) _kln89->_userBaroSetting = 745;
	}
}

void KLN89AltPage::LooseFocus() {
	_uLinePos = 1;
}
