// FGTower - a class to provide tower control at towered airports.
//
// Written by David Luff, started March 2002.
//
// Copyright (C) 2002  David C. Luff - david.luff@nottingham.ac.uk
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

#ifndef _FG_TOWER_HXX
#define _FG_TOWER_HXX

#include <simgear/compiler.h>
#include <simgear/math/point3d.hxx>
#include <simgear/misc/sgstream.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <plib/sg.h>

#include STL_IOSTREAM
#include STL_STRING

SG_USING_STD(string);
#ifndef SG_HAVE_NATIVE_SGI_COMPILERS
SG_USING_STD(ios);
#endif

#include "ATC.hxx"

//DCL - a complete guess for now.
#define FG_TOWER_DEFAULT_RANGE 30

// somewhere in the ATC/AI system we are going to have defined something like
// struct plane_rec
// list <PlaneRec> plane_rec_list_type

class FGTower : public FGATC {

private:

    // Need a data structure to hold details of the various active planes
    // or possibly another data structure with the positions of the inactive planes.
    // Need a data structure to hold outstanding communications from aircraft.


public:

    FGTower();
    ~FGTower();

    void Init();

    void Update();

    inline void SetDisplay() {display = true;}
    inline void SetNoDisplay() {display = false;}

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
    inline atc_type GetType() { return TOWER; }

    // Make a request of tower control
    //void Request(tower_request request);

private:

    char type;
    double lon, lat;
    double elev;
    double x, y, z;
    int freq;
    int range;
    bool display;		// Flag to indicate whether we should be outputting to the ATC display.
    bool displaying;		// Flag to indicate whether we are outputting to the ATC display.
    string ident;		// Code of the airport its at.
    string name;		// Name generally used in transmissions.

    // for failure modeling
    string trans_ident;		// transmitted ident
    bool tower_failed;		// tower failed?

    friend istream& operator>> ( istream&, FGTower& );
};


inline istream&
operator >> ( istream& in, FGTower& t )
{
    double f;
    char ch;

    in >> t.type;
    
    if ( t.type == '[' )
      return in >> skipeol;

    in >> t.lat >> t.lon >> t.elev >> f >> t.range 
       >> t.ident;

    t.name = "";
    in >> ch;
    t.name += ch;
    while(1) {
	//in >> noskipws
	in.unsetf(ios::skipws);
	in >> ch;
	t.name += ch;
	if((ch == '"') || (ch == 0x0A)) {
	    break;
	}   // we shouldn't need the 0x0A but it makes a nice safely in case someone leaves off the "
    }
    in.setf(ios::skipws);
    //cout << "tower.name = " << t.name << '\n';

    t.freq = (int)(f*100.0 + 0.5);

    // cout << t.ident << endl;

    // generate cartesian coordinates
    Point3D geod( t.lon * SGD_DEGREES_TO_RADIANS, t.lat * SGD_DEGREES_TO_RADIANS, t.elev );
    Point3D cart = sgGeodToCart( geod );
    t.x = cart.x();
    t.y = cart.y();
    t.z = cart.z();

    t.trans_ident = t.ident;
    t.tower_failed = false;

    return in >> skipeol;
}


#endif  //_FG_TOWER_HXX
