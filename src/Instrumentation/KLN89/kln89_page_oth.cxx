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

#include "kln89_page_oth.hxx"

using std::string;

KLN89OthPage::KLN89OthPage(KLN89* parent)
: KLN89Page(parent) {
	_nSubPages = 12;
	_subPage = 0;
	_name = "OTH";
}

KLN89OthPage::~KLN89OthPage() {
}

void KLN89OthPage::Update(double dt) {
	// The OTH pages aren't terribly important at the moment, since we don't simulate
	// error and failure, but lets hardwire some representitive output anyway.
	if(_subPage == 0) {
		_kln89->DrawText("State", 2, 0, 3);
		_kln89->DrawText("GPS Alt", 2, 0, 2);
		_kln89->DrawText("Estimated Posn", 2, 0, 1);
		_kln89->DrawText("Error", 2, 1, 0);
		
		// FIXME - hardwired value.
		_kln89->DrawText("NAV D", 2, 9, 3);
		// TODO - add error physics to FG GPS where the alt value comes from.
		char buf[6];
		int n = snprintf(buf, 5, "%i", _kln89->_altUnits == GPS_ALT_UNITS_FT ? (int)_kln89->_alt : (int)(_kln89->_alt * SG_FEET_TO_METER));
		_kln89->DrawText((string)buf, 2, 13-n, 2);
		_kln89->DrawText(_kln89->_altUnits == GPS_ALT_UNITS_FT ? "ft" : "m", 2, 13, 2);
		// FIXME - hardwired values.
		// Note that a 5th digit if required is left padded one further at position 7.
		_kln89->DrawText(_kln89->_distUnits == GPS_DIST_UNITS_NM ? "0.02nm" : "0.03km", 2, 8, 0);
	}
	
	KLN89Page::Update(dt);
}
