// fixlist.cxx -- fix list management class
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


#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sgstream.hxx>
#include <simgear/math/sg_geodesy.hxx>

#include "fixlist.hxx"


FGFixList *current_fixlist;


// Constructor
FGFixList::FGFixList( void ) {
}


// Destructor
FGFixList::~FGFixList( void ) {
}


// load the navaids and build the map
bool FGFixList::init( SGPath path ) {
    FGFix fix;

    fixlist.erase( fixlist.begin(), fixlist.end() );

    sg_gzifstream in( path.str() );
    if ( !in.is_open() ) {
        SG_LOG( SG_GENERAL, SG_ALERT, "Cannot open file: " << path.str() );
        exit(-1);
    }

    // read in each line of the file

    in >> skipeol;
    in >> skipcomment;

#ifdef __MWERKS__

    char c = 0;
    while ( in.get(c) && c != '\0' && fix.get_ident() != (string)"[End]" ) {
        in.putback(c);
        in >> fix;
	if ( fix.get_ident() != (string)"[End]" ) {
	    fixlist[fix.get_ident()] = fix;
	}
        in >> skipcomment;
    }

#else

    while ( ! in.eof() && fix.get_ident() != (string)"[End]" ) {
        in >> fix;
	/* cout << "id = " << n.get_ident() << endl;
	cout << " type = " << n.get_type() << endl;
	cout << " lon = " << n.get_lon() << endl;
	cout << " lat = " << n.get_lat() << endl;
	cout << " elev = " << n.get_elev() << endl;
	cout << " freq = " << n.get_freq() << endl;
 	cout << " range = " << n.get_range() << endl; */
	if ( fix.get_ident() != (string)"[End]" ) {
	    fixlist[fix.get_ident()] = fix;
	}
        in >> skipcomment;
    }

#endif

    return true;
}


// query the database for the specified frequency, lon and lat are in
// degrees, elev is in meters
bool FGFixList::query( const string& ident, double lon, double lat, double elev,
		       FGFix *fix, double *heading, double *dist )
{
    *fix = fixlist[ident];
    if ( fix->get_ident() == "" ) {
	return false;
    }

    double az1, az2, s;
    geo_inverse_wgs_84( elev, lat, lon, 
			fix->get_lat(), fix->get_lon(),
			&az1, &az2, &s );
    // cout << "  dist = " << s << endl;
    *heading = az2;
    *dist = s;
    return true;
}
