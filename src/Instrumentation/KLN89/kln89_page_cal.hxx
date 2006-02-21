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

#ifndef _KLN89_PAGE_CAL
#define _KLN89_PAGE_CAL

#include "kln89.hxx"

class KLN89CalPage : public KLN89Page {

public:
	KLN89CalPage(KLN89* parent);
	~KLN89CalPage();
	
	void Update(double dt);
	
private:
	unsigned int _nFp0;	// flightplan no. displayed on page 1 (_subPage == 0).
	double _ground_speed_ms;	// Assumed ground speed for ete calc on page 1 (user can alter this).
};

#endif	// _KLN89_PAGE_CAL
