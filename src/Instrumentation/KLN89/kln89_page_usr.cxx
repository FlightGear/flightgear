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

#include "kln89_page_usr.hxx"

KLN89UsrPage::KLN89UsrPage(KLN89* parent) 
: KLN89Page(parent) {
	_nSubPages = 4;
	_subPage = 0;
	_name = "USR";
}

KLN89UsrPage::~KLN89UsrPage() {
}

void KLN89UsrPage::Update(double dt) {
        // bool actPage = (_kln89->_activePage->GetName() == "ACT" ? true : false);
	bool crsr = (_kln89->_mode == KLN89_MODE_CRSR);
	bool blink = _kln89->_blink;
	
	if(_subPage == 0) {
		// Hardwire no-waypoint output for now
		_kln89->DrawText("0", 2, 1, 3);
		_kln89->DrawText("USR at:", 2, 7, 3);
		if(!(crsr && _uLinePos == 6 && blink)) _kln89->DrawText("User Pos L/L?", 2, 1, 2);
		if(!(crsr && _uLinePos == 7 && blink)) _kln89->DrawText("User Pos R/D?", 2, 1, 1);
		if(!(crsr && _uLinePos == 8 && blink)) _kln89->DrawText("Present Pos?", 2, 1, 0);
	}
	
	KLN89Page::Update(dt);
}
