// atis.hxx -- ATIS class
//
// Written by David Luff, started October 2001.
// Based on nav.hxx by Curtis Olson, started April 2000.
//
// Copyright (C) 2001  David C. Luff - david.luff@nottingham.ac.uk
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


#ifndef _FG_ATIS_HXX
#define _FG_ATIS_HXX

#include <stdio.h>
#include <string>

#include <simgear/compiler.h>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/misc/sgstream.hxx>
#include <simgear/magvar/magvar.hxx>
#include <simgear/timing/sg_time.hxx>

#ifdef SG_HAVE_STD_INCLUDES
#  include <istream>
#  include <iomanip>
#elif defined( __BORLANDC__ ) || (__APPLE__)
#  include <iostream>
#else
#  include <istream.h>
#  include <iomanip.h>
#endif

SG_USING_STD(istream);
SG_USING_STD(string);

#include "ATC.hxx"

//DCL - a complete guess for now.
#define FG_ATIS_DEFAULT_RANGE 30
	
class FGATIS : public FGATC {
	
	//atc_type type;
	string transmission;	// The actual ATIS transmission
	// This is not stored in default.atis but is generated
	// from the prevailing conditions when required.
	
	// for failure modeling
	string trans_ident;		// transmitted ident
	bool atis_failed;		// atis failed?
	
	// Aircraft position
	// ATIS is actually a special case in that unlike other ATC eg.tower it doesn't actually know about
	// or the whereabouts of the aircraft it is transmitting to.  However, to ensure consistancy of
	// operation with the other ATC classes the ATIS class must calculate range to the aircraft in order
	// to decide whether to render the transmission - hence the users plane details must be stored.
	//SGPropertyNode *airplane_lon_node; 
	//SGPropertyNode *airplane_lat_node;
	//SGPropertyNode *airplane_elev_node; 
	
	public:
	
	FGATIS(void);
	~FGATIS(void);
	
	//run the ATIS instance
	void Update(double dt);
	
	//inline void set_type(const atc_type tp) {type = tp;}
	inline const string& get_trans_ident() { return trans_ident; }
	inline void set_refname(const string& r) { refname = r; } 
	
	private:
	
	string refname;		// Holds the refname of a transmission in progress
	
	//Update the transmission string
	void UpdateTransmission(void);
	
	friend istream& operator>> ( istream&, FGATIS& );
};

#endif // _FG_ATIS_HXX
