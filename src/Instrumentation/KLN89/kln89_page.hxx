// kln89_page.hxx - base class for the "pages" that
//                  are used in the KLN89 GPS unit simulation. 
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

#ifndef _KLN89_PAGE_HXX
#define _KLN89_PAGE_HXX

#include <Instrumentation/dclgps.hxx>
#include "kln89.hxx"

class KLN89;

class KLN89Page {

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
	
	// Sometimes a page needs to maintain state for some return paths,
	// but change it for others.  The CleanUp function can be used for
	// changing state for non-ENT return  paths in conjunction with
	// GPS::_cleanUpPage
	virtual void CleanUp();
	
	// The LooseFocus function is called when a page or subpage looses focus
	// and allows pages to clean up state that is maintained whilst focus is
	// retained, but lost on return.
	virtual void LooseFocus();
	
	inline void SetEntInvert(bool b) { _entInvert = b; }
	
	// Get / Set a waypoint id, NOT the page name!
	virtual void SetId(const std::string& s);
	virtual const std::string& GetId();
	
	inline int GetSubPage() { return(_subPage); }
	void SetSubPage(int n);
	
	inline int GetNSubPages() { return(_nSubPages); }
	
	inline const std::string& GetName() { return(_name); }
	
protected:

	KLN89* _kln89;
	
	std::string _name;	// eg. "APT", "NAV" etc
	int _nSubPages;
	// _subpage is zero based
	int _subPage;	// The subpage gets remembered when other pages are displayed
	
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
	
	std::string _id;		// The ID of the waypoint that the page is displaying.
					// Doesn't make sense for all pages, but does for all the data pages.
					
	void ShowScratchpadMessage(const std::string& line1, const std::string& line2);
					
	bool _scratchpadMsg;		// Set true when there is a scratchpad message to display
	double _scratchpadTimer;	// Used for displaying the scratchpad messages for the right amount of time.
	std::string _scratchpadLine1;
	std::string _scratchpadLine2;
	
	// TODO - remove this function from this class and use a built in method instead.
	std::string GPSitoa(int n);
};

#endif	// _KLN89_PAGE_HXX
