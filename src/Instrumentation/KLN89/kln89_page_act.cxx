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

#include "kln89_page_act.hxx"
#include "kln89_page_apt.hxx"
#include "kln89_page_vor.hxx"
#include "kln89_page_ndb.hxx"
#include "kln89_page_int.hxx"
#include "kln89_page_usr.hxx"

KLN89ActPage::KLN89ActPage(KLN89* parent) 
: KLN89Page(parent) {
	_nSubPages = 1;
	_subPage = 0;
	_name = "ACT";
	_actWp = NULL;
	_actWpId = -1;
	_actPage = NULL;
	
	_aptPage = new KLN89AptPage(parent);
	_vorPage = new KLN89VorPage(parent);
	_ndbPage = new KLN89NDBPage(parent);
	_intPage = new KLN89IntPage(parent);
	_usrPage = new KLN89UsrPage(parent);
}

KLN89ActPage::~KLN89ActPage() {
	delete _aptPage;
	delete _vorPage;
	delete _ndbPage;
	delete _intPage;
	delete _usrPage;
}

void KLN89ActPage::Update(double dt) {
	if(!_actWp) {
		_actWp = _kln89->GetActiveWaypoint();
		_actWpId = _kln89->GetActiveWaypointIndex();
	}
	if(_actWp) {
		switch(_actWp->type) {
		case GPS_WP_APT: _actPage = _aptPage; break;
		case GPS_WP_VOR: _actPage = _vorPage; break;
		case GPS_WP_NDB: _actPage = _ndbPage; break;
		case GPS_WP_INT: _actPage = _intPage; break;
		case GPS_WP_USR: _actPage = _usrPage; break;
		default:
			_actPage = NULL;
			// ASSERT(0); // ie. we shouldn't ever get here.
		}
	}
	
	_id = _actWp->id; 
	if(_actPage) {
		_actPage->SetId(_actWp->id);
		_actPage->Update(dt);
	} else {
		KLN89Page::Update(dt);
	}
}

void KLN89ActPage::CrsrPressed() {
	if(_actPage) {
		_actPage->CrsrPressed();
	} else {
		KLN89Page::CrsrPressed();
	}
}

void KLN89ActPage::EntPressed() {
	if(_actPage) {
		_actPage->EntPressed();
	} else {
		KLN89Page::EntPressed();
	}
}

void KLN89ActPage::ClrPressed() {
	if(_actPage) {
		_actPage->ClrPressed();
	} else {
		KLN89Page::ClrPressed();
	}
}

void KLN89ActPage::Knob1Left1() {
	if(_actPage) {
		_actPage->Knob1Left1();
	}
}

void KLN89ActPage::Knob1Right1() {
	if(_actPage) {
		_actPage->Knob1Right1();
	}
}

void KLN89ActPage::Knob2Left1() {
	if((_kln89->_mode != KLN89_MODE_CRSR) && (_actPage)) {
		_actPage->Knob2Left1();
	}
}

void KLN89ActPage::Knob2Right1() {
	if((_kln89->_mode != KLN89_MODE_CRSR) && (_actPage)) {
		_actPage->Knob2Right1();
	}
}

void KLN89ActPage::LooseFocus() {
	// Setting to NULL and -1 is better than resetting to
	// active waypoint and index since we can't guarantee that
	// the fpl active waypoint won't change behind our backs
	// when we don't have focus.
	_actWp = NULL;
	_actWpId = -1;
}
