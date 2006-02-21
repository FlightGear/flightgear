// fixlist.cxx -- fix list management class
//
// Written by Curtis Olson, started April 2000.
//
// Copyright (C) 2000  Curtis L. Olson - http://www.flightgear.org/~curt
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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sgstream.hxx>
#include <simgear/math/sg_geodesy.hxx>

#include "fixlist.hxx"


// Constructor
FGFixList::FGFixList( void ) {
}


// Destructor
FGFixList::~FGFixList( void ) {
}


// load the navaids and build the map
bool FGFixList::init( SGPath path ) {
    fixlist.erase( fixlist.begin(), fixlist.end() );

    sg_gzifstream in( path.str() );
    if ( !in.is_open() ) {
        SG_LOG( SG_GENERAL, SG_ALERT, "Cannot open file: " << path.str() );
        exit(-1);
    }

    // toss the first two lines of the file
    in >> skipeol;
    in >> skipeol;

    // read in each remaining line of the file

#ifdef __MWERKS__
    char c = 0;
    while ( in.get(c) && c != '\0' ) {
        in.putback(c);
#else	
    while ( ! in.eof() ) {
#endif

        FGFix fix;
        in >> fix;
        if ( fix.get_lat() > 95.0 ) {
            break;
        }

        /* cout << "ident=" << fix.get_ident()
             << ", lat=" << fix.get_lat()
             << ", lon=" << fix.get_lon() << endl; */

        fixlist[fix.get_ident()] = fix;
        in >> skipcomment;
    }
    return true;
}


// query the database for the specified fix, lon and lat are in
// degrees, elev is in meters
bool FGFixList::query( const string& ident, FGFix *fix ) {
    *fix = fixlist[ident];
    if ( ! fix->get_ident().empty() ) {
	return true;
    } else {
        return false;
    }
}


// query the database for the specified fix, lon and lat are in
// degrees, elev is in meters
bool FGFixList::query_and_offset( const string& ident, double lon, double lat,
                                  double elev, FGFix *fix, double *heading,
                                  double *dist )
{
    *fix = fixlist[ident];
    if ( fix->get_ident().empty() ) {
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

const FGFix* FGFixList::findFirstByIdent( const string& ident, bool exact)
{
    fix_map_iterator itr;
    if(exact) {
        itr = fixlist.find(ident);
    } else {
        itr = fixlist.lower_bound(ident);
    }
    if(itr == fixlist.end()) {
        return(NULL);
    } else {
        return(&(itr->second));
    }
}
