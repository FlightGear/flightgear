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

#ifndef _KLN89_PAGE_INT_HXX
#define _KLN89_PAGE_INT_HXX

#include "kln89.hxx"

class FGFix;

class KLN89IntPage : public KLN89Page {

public:
	KLN89IntPage(KLN89* parent);
	~KLN89IntPage();
	
	void Update(double dt);
	
	void CrsrPressed();
	void ClrPressed();
	void EntPressed();
	void Knob2Left1();
	void Knob2Right1();
	
	void SetId(const std::string& s);
	
private:
	std::string _int_id;
	std::string _last_int_id;
	std::string _save_int_id;
	const FGFix* _fp;
	FGNavRecord* _nearestVor;
	FGNavRecord* _refNav;	// Will usually be the same as _nearestVor, and gets reset to _nearestVor when page looses focus.
	double _nvRadial;	// radial from nearest VOR
	double _nvDist;		// distance to nearest VOR
};

#endif	// _KLN89_PAGE_INT_HXX
