// nav.hxx -- vor/dme/ndb class
//
// Written by Curtis Olson, started April 2000.
//
// Copyright (C) 2000  Curtis L. Olson - curt@flightgear.org
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


#ifndef _FG_NAV_HXX
#define _FG_NAV_HXX

#include <stdio.h>

#include <simgear/compiler.h>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/misc/fgstream.hxx>
#include <simgear/magvar/magvar.hxx>
#include <simgear/timing/sg_time.hxx>

#ifdef FG_HAVE_STD_INCLUDES
#  include <istream>
#elif defined( FG_HAVE_NATIVE_SGI_COMPILERS )
#  include <iostream.h>
#elif defined( __BORLANDC__ )
#  include <iostream>
#else
#  include <istream.h>
#endif

#if ! defined( FG_HAVE_NATIVE_SGI_COMPILERS )
FG_USING_STD(istream);
#endif


class FGNav {

    char type;
    double lon, lat;
    double elev;
    double x, y, z;
    int freq;
    int range;
    bool has_dme;
    string ident;		// to avoid a core dump with corrupt data
    double magvar;		// magvar from true north (negative = W)

public:

    inline FGNav(void) {}
    inline ~FGNav(void) {}

    inline char get_type() const { return type; }
    inline double get_lon() const { return lon; }
    inline double get_lat() const { return lat; }
    inline double get_elev() const { return elev; }
    inline double get_x() const { return x; }
    inline double get_y() const { return y; }
    inline double get_z() const { return z; }
    inline int get_freq() const { return freq; }
    inline int get_range() const { return range; }
    inline bool get_has_dme() const { return has_dme; }
    inline const char *get_ident() { return ident.c_str(); }
    inline double get_magvar () const { return magvar; }

    /* inline void set_type( char t ) { type = t; }
    inline void set_lon( double l ) { lon = l; }
    inline void set_lat( double l ) { lat = l; }
    inline void set_elev( double e ) { elev = e; }
    inline void set_freq( int f ) { freq = f; }
    inline void set_range( int r ) { range = r; }
    inline void set_dme( bool b ) { dme = b; }
    inline void set_ident( char *i ) { strncpy( ident, i, 5 ); } */

    friend istream& operator>> ( istream&, FGNav& );
};


inline istream&
operator >> ( istream& in, FGNav& n )
{
    double f;
    char c /* , magvar_dir */ ;
    string magvar_s;

    static SGTime time_params;
    static bool first_time = true;
    static double julian_date = 0;
    if ( first_time ) {
	time_params.update( 0.0, 0.0, 0 );
	julian_date = time_params.getJD();
	first_time = false;
    }

    in >> n.type >> n.lat >> n.lon >> n.elev >> f >> n.range 
       >> c >> n.ident >> magvar_s;

    n.freq = (int)(f*100.0 + 0.5);
    if ( c == 'Y' ) {
	n.has_dme = true;
    } else {
	n.has_dme = false;
    }

    // Calculate the magvar from true north.
    // cout << "Calculating magvar for navaid " << n.ident << endl;
    if (magvar_s == "XXX") {
	// default to mag var as of 1990-01-01 (Julian 2447892.5)
	// cout << "lat = " << n.lat << " lon = " << n.lon << " elev = " 
	//      << n.elev << " JD = " 
	//      << julian_date << endl;
	n.magvar = sgGetMagVar(n.lon * DEG_TO_RAD, n.lat * DEG_TO_RAD,
				n.elev * FEET_TO_METER,
				julian_date) * RAD_TO_DEG;
	// cout << "Default variation at " << n.lon << ',' << n.lat
	// 	<< " is " << var << endl;
#if 0
	// I don't know what this is for - CLO 1 Feb 2001
	if (var - int(var) >= 0.5)
	    n.magvar = int(var) + 1;
	else if (var - int(var) <= -0.5)
	    n.magvar = int(var) - 1;
	else
	    n.magvar = int(var);
#endif
	// cout << "Defaulted to magvar of " << n.magvar << endl;
    } else {
	char direction;
	int var;
	sscanf(magvar_s.c_str(), "%d%c", &var, &direction);
	n.magvar = var;
	if (direction == 'W')
	    n.magvar = -n.magvar;
	// cout << "Explicit magvar of " << n.magvar << endl;
    }
    // cout << n.ident << " " << n.magvar << endl;

    // generate cartesian coordinates
    Point3D geod( n.lon * DEG_TO_RAD, n.lat * DEG_TO_RAD, n.elev );
    Point3D cart = sgGeodToCart( geod );
    n.x = cart.x();
    n.y = cart.y();
    n.z = cart.z();

    return in >> skipeol;
}


#endif // _FG_NAV_HXX
