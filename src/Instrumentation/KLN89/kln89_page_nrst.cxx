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

#include "kln89_page_nrst.hxx"

KLN89NrstPage::KLN89NrstPage(KLN89* parent) 
: KLN89Page(parent) {
	_nSubPages = 2;
	_subPage = 0;
	_name = "NRST";
	_uLinePos = 1;
	_maxULinePos = 8;
}

KLN89NrstPage::~KLN89NrstPage() {
}

void KLN89NrstPage::Update(double dt) {
	// crsr is always on on nearest page
	bool blink = _kln89->_blink;
	
	_kln89->DrawText("NEAREST", 2, 3, 3);
	if(!(_uLinePos == 1 && blink)) _kln89->DrawText("APT?", 2, 0, 2);
	if(!(_uLinePos == 2 && blink)) _kln89->DrawText("VOR?", 2, 5, 2);
	if(!(_uLinePos == 3 && blink)) _kln89->DrawText("NDB?", 2, 10, 2);
	if(!(_uLinePos == 4 && blink)) _kln89->DrawText("INT?", 2, 0, 1);
	if(!(_uLinePos == 5 && blink)) _kln89->DrawText("USR?", 2, 5, 1);
	if(!(_uLinePos == 6 && blink)) _kln89->DrawText("SUA?", 2, 10, 1);
	if(!(_uLinePos == 7 && blink)) _kln89->DrawText("FSS?", 2, 0, 0);
	if(!(_uLinePos == 8 && blink)) _kln89->DrawText("CTR?", 2, 5, 0);
		
	switch(_uLinePos) {
	case 1: _kln89->Underline(2, 0, 2, 4); break;
	case 2: _kln89->Underline(2, 5, 2, 4); break;
	case 3: _kln89->Underline(2, 10, 2, 4); break;
	case 4: _kln89->Underline(2, 0, 1, 4); break;
	case 5: _kln89->Underline(2, 5, 1, 4); break;
	case 6: _kln89->Underline(2, 10, 1, 4); break;
	case 7: _kln89->Underline(2, 0, 0, 4); break;
	case 8: _kln89->Underline(2, 5, 0, 4); break;
	}
	
	// Actually, the kln89 sim from Bendix-King dosn't draw the 'ENT'
	// for 'APT?' if it was on 'Leg' (pos 0) immediately previously, but does if
	// it was not on 'Leg' immediately previously.  I think we can desist from
	// reproducing this probable bug.
	if(_uLinePos > 0) {
		if(!blink) _kln89->DrawEnt();
	}
	
	KLN89Page::Update(dt);
}

void KLN89NrstPage::CrsrPressed() {
}

void KLN89NrstPage::EntPressed() {
	if(_uLinePos > 4) {
		ShowScratchpadMessage("  No  ", " Nrst ");
	}
}

void KLN89NrstPage::LooseFocus() {
	_uLinePos = 1;
	_scratchpadMsg = false;
}
