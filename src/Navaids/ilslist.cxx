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
    }

#endif

    return true;
}


// query the database for the specified frequency, lon and lat are in
// degrees, elev is in meters
bool FGILSList::query( double lon, double lat, double elev, double freq,
		       FGILS *ils, double *heading, double *dist )
{
    ils_list_type stations = ilslist[(int)(freq*100.0)];

    ils_list_iterator current = stations.begin();
    ils_list_iterator last = stations.end();

    double az1, az2, s;
    for ( ; current != last ; ++current ) {
	// cout << "testing " << current->get_ident() << endl;
	geo_inverse_wgs_84( elev, lat, lon, 
			    current->get_loclat(), current->get_loclon(),
			    &az1, &az2, &s );
	// cout << "  dist = " << s << endl;
	if ( s < ( FG_ILS_DEFAULT_RANGE * NM_TO_METER ) ) {
	    *ils = *current;
	    *heading = az2;
	    *dist = s;
	    return true;
	}
    }

    return false;
}
