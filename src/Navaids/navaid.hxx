// navaid.hxx -- navaid class
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


#ifndef _FG_NAVAID_HXX
#define _FG_NAVAID_HXX


#include <simgear/compiler.h>
#include <simgear/misc/fgstream.hxx>

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


class FGNavAid {

    char type;
    double lon, lat;
    double elev;
    int freq;
    int range;
    bool dme;
    char ident[5];

public:

    inline FGNavAid(void) {}
    inline ~FGNavAid(void) {}

    inline char get_type() const { return type; }
    inline double get_lon() const { return lon; }
    inline double get_lat() const { return lat; }
    inline double get_elev() const { return elev; }
    inline int get_freq() const { return freq; }
    inline int get_range() const { return range; }
    inline bool get_dme() const { return dme; }
    inline char *get_ident() { return ident; }

    inline void set_type( char t ) { type = t; }
    inline void set_lon( double l ) { lon = l; }
    inline void set_lat( double l ) { lat = l; }
    inline void set_elev( double e ) { elev = e; }
    inline void set_freq( int f ) { freq = f; }
    inline void set_range( int r ) { range = r; }
    inline void set_dme( bool b ) { dme = b; }
    inline void set_ident( char *i ) { strncpy( ident, i, 5 ); }

    friend istream& operator>> ( istream&, FGNavAid& );
};


inline istream&
operator >> ( istream& in, FGNavAid& n )
{
    double f;
    char c;
    in >> n.type >> n.lon >> n.lat >> n.elev >> f >> n.range 
       >> c >> n.ident;

    n.freq = (int)(f * 100.0);
    if ( c == 'Y' ) {
	n.dme = true;
    } else {
	n.dme = false;
    }

    return in >> skipeol;
}


#endif // _FG_NAVAID_HXX
