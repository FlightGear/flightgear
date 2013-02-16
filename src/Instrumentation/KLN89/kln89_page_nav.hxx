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

#include "kln89.hxx"

class KLN89NavPage : public KLN89Page {

public:
	KLN89NavPage(KLN89* parent);
	~KLN89NavPage();
	
	void Update(double dt);
	
	void CrsrPressed();
	void EntPressed();
	void ClrPressed();
	void Knob1Left1();
	void Knob1Right1();
	void Knob2Left1();
	void Knob2Right1();
	
	void LooseFocus();
	
	// Returns the id string of the selected waypoint on NAV4 if valid, else returns an empty string.
	std::string GetNav4WpId();
	
private:
	int _posFormat;		// 0 => lat,lon; 1 => ref to wp.
	
	int _vnv;	// 0 => To, 1 => Fr, 2 => off.
	
	// The data snippet to be displayed in field 1 when the moving map is active (NAV 4)
	int _nav4DataSnippet;	// 0 => DTK, 1 => groundspeed, 2 => ETE, 3 => cross-track correction.
	
	// Format to draw in the CDI field.
	// 0 => CDI, 1 => Cross track correction (eg " Fly R  2.15nm"), 2 => cdi scale (eg "CDI Scale:5.0nm")
	int _cdiFormat;
	
	// Drawing of apt, vor and sua on the moving map can be temporarily suspended
	// Note that this should be cleared when page focus is lost, or when the menu is displayed. 
	bool _suspendAVS;
	
	// NAV 4 menu stuff
	bool _menuActive;
	int _menuPos;
	
	// NAV 4 waypoint scan drawing housekeeping.
	bool _scanWpSet;
	int _scanWpIndex;
};
