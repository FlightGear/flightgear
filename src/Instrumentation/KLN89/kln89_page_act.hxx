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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$

#ifndef _KLN89_PAGE_ACT_HXX
#define _KLN89_PAGE_ACT_HXX

#include "kln89.hxx"

class KLN89ActPage : public KLN89Page {

public:
	KLN89ActPage(KLN89* parent);
	~KLN89ActPage();

	void Update(double dt);
	
	void CrsrPressed();
	void EntPressed();
	void ClrPressed();
	void Knob2Left1();
	void Knob2Right1();
	
	void LooseFocus();
	
private:
	// Position of the currently displayed waypoint within the active flightplan.
	// -1 indicates no active flightplan.
	int _actWpId;
	
	GPSWaypoint* _actWp;
	
	// The actual ACT page that gets displayed...
	GPSPage* _actPage;
	// ...which points to one of the below.
	GPSPage* _aptPage;
	GPSPage* _vorPage;
	GPSPage* _ndbPage;
	GPSPage* _intPage;
	GPSPage* _usrPage;
};

#endif	// _KLN89_PAGE_ACT_HXX
