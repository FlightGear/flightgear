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

#ifndef _KLN89_PAGE_DIR
#define _KLN89_PAGE_DIR

#include "kln89.hxx"

class KLN89DirPage : public KLN89Page {

public:
	KLN89DirPage(KLN89* parent);
	~KLN89DirPage();
	
	void Update(double dt);
	
	void SetId(const std::string& s);
	
	void CrsrPressed();
	void ClrPressed();
	void EntPressed();
	void Knob2Left1();
	void Knob2Right1();
	
private:
	// Waypoint display mode.
	// There are a number of ways that the waypoint can be displayed in the DIR page:
	// 0 => Whole candidate waypoint displayed, entirely inverted.  This is normally how the page is initially displayed, unless a candidate waypoint cannot be determined.
	// 1 => Waypoint being entered, with a corresponding cursor position, and only the cursor position inverted.
	// 2 => Blanks.  These can be displayed flashing when the cursor is active (eg. when CLR is pressed) and are always displayed if the cursor is turned off.
	int _DToWpDispMode;
	
	// Position of the list in the mode that scans through the active flight plan.
	// This should be initialised to point at the final waypoint of the active flight plan when we enter mode zero above.
	int _DToWpDispIndex;
	
	// We need to save the mode when DTO gets pressed, since potentially this class handles page exit via. the CLR event handler
	KLN89Mode _saveMasterMode;
};

#endif	// _KLN89_PAGE_DIR
