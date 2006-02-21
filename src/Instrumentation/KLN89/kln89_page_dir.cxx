// kln89_page_*.[ch]xx - this file is one of the "pages" that
//                       are used in the KLN89 GPS unit simulation. 
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "kln89_page_dir.hxx"

KLN89DirPage::KLN89DirPage(KLN89* parent)
: KLN89Page(parent) {
	_nSubPages = 1;
	_subPage = 0;
	_name = "DIR";
	_DToWpDispMode = 2;
}

KLN89DirPage::~KLN89DirPage() {
}

void KLN89DirPage::Update(double dt) {
	// TODO - this can apparently be "ACTIVATE:" under some circumstances
	_kln89->DrawText("DIRECT TO:", 2, 2, 3);
	
	if(_kln89->_mode == KLN89_MODE_CRSR) {
		if(_DToWpDispMode == 0) {
			string s = _id;
			while(s.size() < 5) s += ' ';
			if(!_kln89->_blink) {
				_kln89->DrawText(s, 2, 4, 1, false, 99);
				_kln89->DrawEnt(1, 0, 1);
			}
		} else if(_DToWpDispMode == 1) {
			if(!_kln89->_blink) {
				// TODO
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

void KLN89DirPage::SetId(const string& s) {
	if(s.size()) {
		_id = s;
		// TODO - fill in lat, lon, type
		// or just pass in waypoints (probably better!)
		_DToWpDispMode = 0;
		// TODO - this (above) should probably be dependent on whether s is a *valid* waypoint!
	} else {
		_DToWpDispMode = 2;
	}
	_saveMasterMode = _kln89->_mode;
	_uLinePos = 1;	// Needed to stop Leg flashing
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
	//cout << "DTO ENT Pressed()\n";
	if(_id.empty()) {
		_kln89->DtoCancel();
	} else {
		_kln89->DtoInitiate(_id);
	}
}
