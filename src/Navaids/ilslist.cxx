// ilslist.cxx -- ils management class
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
#include <simgear/misc/fgstream.hxx>
#include <simgear/math/fg_geodesy.hxx>

#include "ilslist.hxx"


FGILSList *current_ilslist;


// Constructor
FGILSList::FGILSList( void ) {
}


// Destructor
FGILSList::~FGILSList( void ) {
}


// load the navaids and build the map
bool FGILSList::init( FGPath path ) {
    FGILS ils;

    ilslist.erase( ilslist.begin(), ilslist.end() );

    fg_gzifstream in( path.str() );
    if ( !in.is_open() ) {
        FG_LOG( FG_GENERAL, FG_ALERT, "Cannot open file: " << path.str() );
        exit(-1);
    }

    // read in each line of the file

    in >> skipeol;
    in >> skipcomment;

#ifdef __MWERKS__

    char c = 0;
    while ( in.get(c) && c != '\0' && n.get_ilstype() != '[' ) {
        in.putback(c);
        in >> ils;
	if ( ils.get_type() != '[' ) {
	    ilslist[ils.get_locfreq()].push_back(ils);
	}
        in >> skipcomment;
    }

#else

    double min = 1000000.0;
    double max = 0.0;

    while ( ! in.eof() && ils.get_ilstype() != '[' ) {
        in >> ils;
	/* cout << "id = " << n.get_ident() << endl;
	cout << " type = " << n.get_type() << endl;
	cout << " lon = " << n.get_lon() << endl;
	cout << " lat = " << n.get_lat() << endl;
	cout << " elev = " << n.get_elev() << endl;
	cout << " freq = " << n.get_freq() << endl;
 	cout << " range = " << n.get_range() << endl; */
	if ( ils.get_ilstype() != '[' ) {
	    ilslist[ils.get_locfreq()].push_back(ils);
	}
        in >> skipcomment;

	if ( ils.get_locfreq() < min ) {
	    min = ils.get_locfreq();
	}
	if ( ils.get_locfreq() > max ) {
	    max = ils.get_locfreq();
	}
    }

    // cout << "min freq = " << min << endl;
    // cout << "max freq = " << max << endl;

#endif

    return true;
}


// query the database for the specified frequency, lon and lat are in
// degrees, elev is in meters
bool FGILSList::query( double lon, double lat, double elev, double freq,
		       FGILS *ils )
{
    ils_list_type stations = ilslist[(int)(freq*100.0 + 0.5)];

    ils_list_iterator current = stations.begin();
    ils_list_iterator last = stations.end();

    // double az1, az2, s;
    Point3D aircraft = fgGeodToCart( Point3D(lon, lat, elev) );
    Point3D station;
    double d;
    for ( ; current != last ; ++current ) {
	// cout << "  testing " << current->get_locident() << endl;
	station = Point3D(current->get_x(), current->get_y(), current->get_z());
	// cout << "    aircraft = " << aircraft << " station = " << station 
	//      << endl;

	d = aircraft.distance3Dsquared( station );
	// cout << "  distance = " << d << " (" 
	//      << FG_ILS_DEFAULT_RANGE * NM_TO_METER 
	//         * FG_ILS_DEFAULT_RANGE * NM_TO_METER
	//      << ")" << endl;

	// cout << "  dist = " << s << endl;
	if ( d < (FG_ILS_DEFAULT_RANGE * NM_TO_METER 
		  * FG_ILS_DEFAULT_RANGE * NM_TO_METER) ) {
	    *ils = *current;
	    return true;
	}
    }

    return false;
}
