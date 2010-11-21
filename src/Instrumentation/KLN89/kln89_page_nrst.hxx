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

#ifndef _KLN89_PAGE_NRST
#define _KLN89_PAGE_NRST

#include "kln89.hxx"

class KLN89NrstPage : public KLN89Page {

public:
	KLN89NrstPage(KLN89* parent);
	~KLN89NrstPage();
	
	void Update(double dt);
	
	void CrsrPressed();
	void EntPressed();
	//void ClrPressed();
	//void Knob1Left1();
	//void Knob1Right1();
	//void Knob2Left1();
	//void Knob2Right1();
	
	void LooseFocus();
};

#endif	// _KLN89_PAGE_NRST
