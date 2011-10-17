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

#ifndef _KLN89_PAGE_FPL_HXX
#define _KLN89_PAGE_FPL_HXX

#include "kln89.hxx"

class KLN89FplPage : public KLN89Page {

public:
	KLN89FplPage(KLN89* parent);
	~KLN89FplPage();
	
	void Update(double dt);
	
	void CrsrPressed();
	void EntPressed();
	void ClrPressed();
	void Knob1Left1();
	void Knob1Right1();
	void Knob2Left1();
	void Knob2Right1();
	
	void CleanUp();
	void LooseFocus();
	
	// Override the base class GetId function to return the waypoint ID under the cursor
	// on FPL0 page, if there is one and the cursor is on.
	// Otherwise return an empty string.
	inline const std::string& GetId() { return(_fp0SelWpId); } 
	
private:
	int _fpMode;	// 0 = Dis, 1 = Dtk
	int _actFpMode;	// 0 = Dis, 1 = ETE, 2 = ETA, 3 = Dtk/OBS
	
	bool _bEntWp;	// set true when a waypoint is being entered
	bool _bEntExp;	// Set true when ent is expected to set the currently entered waypoint as entered.
	std::string _entWpStr;  // The currently entered wp ID (need not be valid) 
	GPSWaypoint* _entWp;	// Waypoint being currently entered
	
	// The position of the cursor in a waypoint being entered
	unsigned int _wLinePos;
	
	unsigned int _fplPos;	// The position of the start of the FP (NOT the active one) (zero-based).
							// since this is reset when subpage changes we only need 1, not an array of 25!
	bool _resetFplPos0;		// Set true when a recalculation of _fplPos for the active flightplan page is required.
	
	int _hdrPos;
	int _fencePos;

    // Get the waypoint (returned thru the pointer) at the zero-based position (pos)
    // in the waypoint display of the current page.
    // Returns 0 if no waypoint, 1 if sucessfull, 2 if appr header, 3 if approach fence.
    //int GetFPWaypoint(GPSWaypoint* wp, int pos);
	
	bool _delFP;	// Set true when the delete FP? dialogue is being displayed
	bool _delWp;	// The position of the waypoint to delete is given by _uLinePos
	bool _delAppr;	// Set true when the delete approach dialogue is being displayed
	bool _changeAppr;
	
	void DrawFpMode(int ypos);
	void Calc();
	
	// The ID of the waypoint under the cursor in fpl0, if those conditions exist!
	std::string _fp0SelWpId;
	
  std::vector<std::string> _params;
};

#endif	// _KLN89_PAGE_FPL_HXX
