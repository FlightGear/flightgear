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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sgstream.hxx>
#include <simgear/math/sg_geodesy.hxx>

#include "mkrbeacons.hxx"
#include "ilslist.hxx"


FGILSList *current_ilslist;


// Constructor
FGILSList::FGILSList( void ) {
}


// Destructor
FGILSList::~FGILSList( void ) {
}


// load the navaids and build the map
bool FGILSList::init( SGPath path ) {

    ilslist.erase( ilslist.begin(), ilslist.end() );

    sg_gzifstream in( path.str() );
    if ( !in.is_open() ) {
        SG_LOG( SG_GENERAL, SG_ALERT, "Cannot open file: " << path.str() );
        exit(-1);
    }

    // read in each line of the file

    in >> skipeol;
    in >> skipcomment;

    // double min = 1000000.0;
    // double max = 0.0;

#ifdef __MWERKS__
    char c = 0;
    while ( in.get(c) && c != '\0' ) {
        in.putback(c);
#else
    while ( ! in.eof() ) {
#endif

        FGILS ils;
        in >> ils;
        if ( ils.get_ilstype() == '[' ) {
            break;
        }

        /* cout << "typename = " << ils.get_ilstypename() << endl;
        cout << " aptcode = " << ils.get_aptcode() << endl;
        cout << " twyno = " << ils.get_rwyno() << endl;
        cout << " locfreq = " << ils.get_locfreq() << endl;
        cout << " locident = " << ils.get_locident() << endl << endl; */

        ilslist[ils.get_locfreq()].push_back(ils);
        in >> skipcomment;

	/* if ( ils.get_locfreq() < min ) {
	    min = ils.get_locfreq();
	}
	if ( ils.get_locfreq() > max ) {
	    max = ils.get_locfreq();
	} */

	// update the marker beacon list
	if ( fabs(ils.get_omlon()) > SG_EPSILON ||
	     fabs(ils.get_omlat()) > SG_EPSILON ) {
	    current_beacons->add( ils.get_omlon(), ils.get_omlat(),
				  ils.get_gselev(), FGMkrBeacon::OUTER );
	}
	if ( fabs(ils.get_mmlon()) > SG_EPSILON ||
	     fabs(ils.get_mmlat()) > SG_EPSILON ) {
	    current_beacons->add( ils.get_mmlon(), ils.get_mmlat(),
				  ils.get_gselev(), FGMkrBeacon::MIDDLE );
	}
	if ( fabs(ils.get_imlon()) > SG_EPSILON ||
	     fabs(ils.get_imlat()) > SG_EPSILON ) {
	    current_beacons->add( ils.get_imlon(), ils.get_imlat(),
				  ils.get_gselev(), FGMkrBeacon::INNER );
	}
    }

    // cout << "min freq = " << min << endl;
    // cout << "max freq = " << max << endl;

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

    double best_angle = 362.0;
    bool found_one = false;

    // double az1, az2, s;
    Point3D aircraft = sgGeodToCart( Point3D(lon, lat, elev) );
    Point3D station;
    double d;
    for ( ; current != last ; ++current ) {
	// cout << "  testing " << current->get_locident() << endl;
	station = Point3D(current->get_x(), 
			  current->get_y(),
			  current->get_z());
	// cout << "    aircraft = " << aircraft << " station = " << station 
	//      << endl;

	d = aircraft.distance3Dsquared( station );
	// cout << "  distance = " << d << " (" 
	//      << FG_ILS_DEFAULT_RANGE * SG_NM_TO_METER 
	//         * FG_ILS_DEFAULT_RANGE * SG_NM_TO_METER
	//      << ")" << endl;

	// cout << "  dist = " << s << endl;

	// match up to twice the published range so we can model
	// reduced signal strength
	if ( d < (2* FG_ILS_DEFAULT_RANGE * SG_NM_TO_METER 
		  * 2 * FG_ILS_DEFAULT_RANGE * SG_NM_TO_METER) ) {

            found_one = true;

            // Get our bearing from this station.
            double reciprocal_bearing, dummy;
            double a_lat_deg = lat * SGD_RADIANS_TO_DEGREES;
            double a_lon_deg = lon * SGD_RADIANS_TO_DEGREES;
            // Locator beam direction
            double s_ils_deg = current->get_locheading() - 180.0;
            if ( s_ils_deg < 0.0 ) { s_ils_deg += 360.0; }
            double angle_to_beam_deg;

            // printf("**ALI geting geo_inverse_wgs_84 with elev = %.2f, a.lat = %.2f, a.lon = %.2f,
            // s.lat = %.2f, s.lon = %.2f\n", elev,a_lat_deg,a_lon_deg,current->get_loclat(),current->get_loclon());

            geo_inverse_wgs_84( elev, current->get_loclat(),
                                current->get_loclon(), a_lat_deg, a_lon_deg,
                                &reciprocal_bearing, &dummy, &dummy );
            angle_to_beam_deg = fabs(reciprocal_bearing - s_ils_deg);
            if ( angle_to_beam_deg <= best_angle ) {
                *ils = *current;
                best_angle = angle_to_beam_deg;
            }
	}
    }

    return found_one;
}
