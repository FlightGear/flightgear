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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


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
	
	char type;
	double lon, lat, elev;
	double x, y, z;
	int freq;
	int range;
	bool display;		// Flag to indicate whether we should be outputting to the ATC display.
	bool displaying;		// Flag to indicate whether we are outputting to the ATC display.
	string ident;		// Code of the airport its at.
	string name;		// Name transmitted in the broadcast.
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
	void Update(void);
	
	//Indicate that this instance should be outputting to the ATC display
	inline void SetDisplay(void) {display = true;}
	
	//Indicate that this instance should not be outputting to the ATC display
	inline void SetNoDisplay(void) {display = false;}
	
	inline char get_type() const { return type; }
	inline double get_lon() const { return lon; }
	inline double get_lat() const { return lat; }
	inline double get_elev() const { return elev; }
	inline double get_x() const { return x; }
	inline double get_y() const { return y; }
	inline double get_z() const { return z; }
	inline int get_freq() const { return freq; }
	inline int get_range() const { return range; }
	inline const char* GetIdent() { return ident.c_str(); }
	inline string get_trans_ident() { return trans_ident; }
	inline atc_type GetType() { return ATIS; }
	inline void set_refname(string r) { refname = r; } 
	
	private:
	
	string refname;		// Holds the refname of a transmission in progress
	
	//Update the transmission string
	void UpdateTransmission(void);
	
	/* inline void set_type( char t ) { type = t; }
	inline void set_lon( double l ) { lon = l; }
	inline void set_lat( double l ) { lat = l; }
	inline void set_elev( double e ) { elev = e; }
	inline void set_freq( int f ) { freq = f; }
	inline void set_range( int r ) { range = r; }
	inline void set_dme( bool b ) { dme = b; }
	inline void set_ident( char *i ) { strncpy( ident, i, 5 ); } */
	
	friend istream& operator>> ( istream&, FGATIS& );
};


inline istream&
operator >> ( istream& in, FGATIS& a )
{
	double f;
	char ch;
	
	static bool first_time = true;
	static double julian_date = 0;
	static const double MJD0    = 2415020.0;
	if ( first_time ) {
		julian_date = sgTimeCurrentMJD(0, 0) + MJD0;
		first_time = false;
	}
	
	in >> a.type;
	
	if ( a.type == '[' )
		return in >> skipeol;
	
	in >> a.lat >> a.lon >> a.elev >> f >> a.range 
	>> a.ident;
	
	a.name = "";
	in >> ch;
	a.name += ch;
	while(1) {
		//in >> noskipws
		in.unsetf(ios::skipws);
		in >> ch;
		a.name += ch;
		if((ch == '"') || (ch == 0x0A)) {
			break;
		}   // we shouldn't need the 0x0A but it makes a nice safely in case someone leaves off the "
	}
	in.setf(ios::skipws);
	//cout << "atis.name = " << a.name << '\n';
	
	a.freq = (int)(f*100.0 + 0.5);
	
	// cout << a.ident << endl;
	
	// generate cartesian coordinates
	Point3D geod( a.lon * SGD_DEGREES_TO_RADIANS, a.lat * SGD_DEGREES_TO_RADIANS, a.elev );
	Point3D cart = sgGeodToCart( geod );
	a.x = cart.x();
	a.y = cart.y();
	a.z = cart.z();
	
	a.trans_ident = a.ident;
	a.atis_failed = false;
	
	return in >> skipeol;
}

#endif // _FG_ATIS_HXX
