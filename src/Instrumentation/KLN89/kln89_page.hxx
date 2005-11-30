// kln89_page.hxx - base class for the "pages" that
//                  are used in the KLN89 GPS unit simulation. 
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

#ifndef _KLN89_PAGE_HXX
#define _KLN89_PAGE_HXX

#include <Instrumentation/dclgps.hxx>
#include "kln89.hxx"

class KLN89;

class KLN89Page : public GPSPage {

public:
	KLN89Page(KLN89* parent);
	virtual ~KLN89Page();
	virtual void Update(double dt);
	virtual void Knob1Left1();
	virtual void Knob1Right1();
	virtual void Knob2Left1();
	virtual void Knob2Right1();
	virtual void CrsrPressed();
	virtual void EntPressed();
	virtual void ClrPressed();
	// Even though some/all of the buttons below aren't processed directly by the current page,
	// the current page often needs to save or change some state when they are pressed, and 
	// hence should provide a function to handle them.
	virtual void DtoPressed();
	virtual void NrstPressed();
	virtual void AltPressed();
	virtual void OBSPressed();
	virtual void MsgPressed();
	
	// See base class comments for this.
	virtual void CleanUp();
	
	// ditto
	virtual void LooseFocus();
	
	inline void SetEntInvert(bool b) { _entInvert = b; }
	
	// Get / Set a waypoint id, NOT the page name!
	virtual void SetId(string s);
	virtual string GetId();
	
protected:
	KLN89* _kln89;
	
	// Underline position in cursor mode is not persistant when subpage is changed - hence we only need one variable per page for it.
	// Note that pos 0 is special - this is the leg pos in field 1, so pos will normally be set to 1 when crsr is pressed.
	// Also note that in general it doesn't seem to wrap.
	unsigned int _uLinePos;
	unsigned int _maxULinePos;
	
	// This is NOT the main gps to/from flag - derived page classes can use this flag
	// for any purpose, typically whether a radial bearing should be displayed to or from.
	bool _to_flag;	// true for TO, false for FROM
	
	// Invert ID and display ENT in field 1
	bool _entInvert;
	
	string _id;		// The ID of the waypoint that the page is displaying.
					// Doesn't make sense for all pages, but does for all the data pages.
					
	void ShowScratchpadMessage(string line1, string line2);
					
	bool _scratchpadMsg;		// Set true when there is a scratchpad message to display
	double _scratchpadTimer;	// Used for displaying the scratchpad messages for the right amount of time.
	string _scratchpadLine1;
	string _scratchpadLine2;
};

#endif	// _KLN89_PAGE_HXX
