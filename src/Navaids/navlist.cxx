// navlist.cxx -- navaids management class
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

        FGNav *n = new FGNav;
        in >> (*n);
        if ( n->get_type() == '[' ) {
            break;
        }

	/* cout << "id = " << n.get_ident() << endl;
	cout << " type = " << n.get_type() << endl;
	cout << " lon = " << n.get_lon() << endl;
	cout << " lat = " << n.get_lat() << endl;
	cout << " elev = " << n.get_elev() << endl;
	cout << " freq = " << n.get_freq() << endl;
 	cout << " range = " << n.get_range() << endl << endl; */

        navaids      [n->get_freq() ].push_back(n);
        ident_navaids[n->get_ident()].push_back(n);
		
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


// Query the database for the specified frequency.  It is assumed that
// there will be multiple stations with matching frequencies so a
// position must be specified.  Lon and lat are in degrees, elev is in
// meters.
FGNav *FGNavList::findByFreq( double freq, double lon, double lat, double elev )
{
    nav_list_type stations = navaids[(int)(freq*100.0 + 0.5)];
    Point3D aircraft = sgGeodToCart( Point3D(lon, lat, elev) );

    return findNavFromList( aircraft, stations );
}


FGNav *FGNavList::findByIdent( const char* ident,
                               const double lon, const double lat )
{
    nav_list_type stations = ident_navaids[ident];
    Point3D aircraft = sgGeodToCart( Point3D(lon, lat, 0.0) );

    return findNavFromList( aircraft, stations );
}


// Given an Ident and optional freqency, return the first matching
// station.
FGNav *FGNavList::findByIdentAndFreq( const char* ident, const double freq )
{
    nav_list_type stations = ident_navaids[ident];

    if ( freq > 0.0 ) {
        // sometimes there can be duplicated idents.  If a freq is
        // specified, use it to refine the search.
        int f = (int)(freq*100.0 + 0.5);
        for ( unsigned int i = 0; i < stations.size(); ++i ) {
            if ( f == stations[i]->get_freq() ) {
                return stations[i];
            }
        }
    } else {
        return stations[0];
    }

    return NULL;
}


// Given a point and a list of stations, return the closest one to the
// specified point.
FGNav *FGNavList::findNavFromList( const Point3D &aircraft, 
                                   const nav_list_type &stations )
{
    FGNav *nav = NULL;
    Point3D station;
    double d2;
    double min_dist;

    // prime the pump with info from stations[0]
    if ( stations.size() > 0 ) {
        nav = stations[0];
	station = Point3D( nav->get_x(), nav->get_y(), nav->get_z());
	min_dist = aircraft.distance3Dsquared( station );
    }

    // check if any of the remaining stations are closer
    for ( unsigned int i = 1; i < stations.size(); ++i ) {
	// cout << "testing " << current->get_ident() << endl;
	station = Point3D( stations[i]->get_x(), stations[i]->get_y(),
                           stations[i]->get_z());

	d2 = aircraft.distance3Dsquared( station );

	// cout << "  dist = " << sqrt(d)
	//      << "  range = " << current->get_range() * SG_NM_TO_METER
        //      << endl;

        if ( d2 < min_dist ) {
            min_dist = d2;
            nav = stations[i];
        }
    }

    return nav;
}
