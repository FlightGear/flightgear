// navaids.cxx -- navaids management class
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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sgstream.hxx>
#include <simgear/math/sg_geodesy.hxx>

#include "navlist.hxx"


FGNavList *current_navlist;


// Constructor
FGNavList::FGNavList( void ) {
}


// Destructor
FGNavList::~FGNavList( void ) {
}


// load the navaids and build the map
bool FGNavList::init( SGPath path ) {

    navaids.erase( navaids.begin(), navaids.end() );

    sg_gzifstream in( path.str() );
    if ( !in.is_open() ) {
        SG_LOG( SG_GENERAL, SG_ALERT, "Cannot open file: " << path.str() );
        exit(-1);
    }

    // read in each line of the file

    in >> skipeol;
    in >> skipcomment;

    // double min = 100000;
    // double max = 0;

#ifdef __MWERKS__
    char c = 0;
    while ( in.get(c) && c != '\0'  ) {
        in.putback(c);
#else
    while ( ! in.eof() ) {
#endif

        FGNav n;
        in >> n;
        if ( n.get_type() == '[' ) {
            break;
        }

	/* cout << "id = " << n.get_ident() << endl;
	cout << " type = " << n.get_type() << endl;
	cout << " lon = " << n.get_lon() << endl;
	cout << " lat = " << n.get_lat() << endl;
	cout << " elev = " << n.get_elev() << endl;
	cout << " freq = " << n.get_freq() << endl;
 	cout << " range = " << n.get_range() << endl << endl; */

        navaids[n.get_freq()].push_back(n);
        in >> skipcomment;

	/* if ( n.get_type() != 'N' ) {
	    if ( n.get_freq() < min ) {
		min = n.get_freq();
	    }
	    if ( n.get_freq() > max ) {
		max = n.get_freq();
	    }
	} */
    }

    // cout << "min freq = " << min << endl;
    // cout << "max freq = " << max << endl;

    return true;
}


// query the database for the specified frequency, lon and lat are in
// degrees, elev is in meters
bool FGNavList::query( double lon, double lat, double elev, double freq,
		       FGNav *n )
{
    nav_list_type stations = navaids[(int)(freq*100.0 + 0.5)];

    nav_list_iterator current = stations.begin();
    nav_list_iterator last = stations.end();

    // double az1, az2, s;
    Point3D aircraft = sgGeodToCart( Point3D(lon, lat, elev) );
    Point3D station;
    double d;
    for ( ; current != last ; ++current ) {
	// cout << "testing " << current->get_ident() << endl;
	station = Point3D(current->get_x(), current->get_y(), current->get_z());

	d = aircraft.distance3Dsquared( station );

	// cout << "  dist = " << sqrt(d)
	//      << "  range = " << current->get_range() * SG_NM_TO_METER << endl;

	// match up to twice the published range so we can model
	// reduced signal strength
	if ( d < (2 * current->get_range() * SG_NM_TO_METER 
		  * 2 * current->get_range() * SG_NM_TO_METER ) ) {
	    // cout << "matched = " << current->get_ident() << endl;
	    *n = *current;
	    return true;
	}
    }

    return false;
}
