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

#include <Main/globals.hxx>

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
        
        FGILS *ils = new FGILS;
        in >> (*ils);
        if ( ils->get_ilstype() == '[' ) {
            break;
        }

        /* cout << "typename = " << ils.get_ilstypename() << endl;
        cout << " aptcode = " << ils.get_aptcode() << endl;
        cout << " twyno = " << ils.get_rwyno() << endl;
        cout << " locfreq = " << ils.get_locfreq() << endl;
        cout << " locident = " << ils.get_locident() << endl << endl; */

        ilslist[ils->get_locfreq()].push_back(ils);
        in >> skipcomment;

	/* if ( ils.get_locfreq() < min ) {
	    min = ils.get_locfreq();
	}
	if ( ils.get_locfreq() > max ) {
	    max = ils.get_locfreq();
	} */

	// update the marker beacon list
	if ( fabs(ils->get_omlon()) > SG_EPSILON ||
	     fabs(ils->get_omlat()) > SG_EPSILON ) {
	    globals->get_beacons()->add( ils->get_omlon(), ils->get_omlat(),
                                         ils->get_gselev(),
                                         FGMkrBeacon::OUTER );
	}
	if ( fabs(ils->get_mmlon()) > SG_EPSILON ||
	     fabs(ils->get_mmlat()) > SG_EPSILON ) {
	    globals->get_beacons()->add( ils->get_mmlon(), ils->get_mmlat(),
                                         ils->get_gselev(),
                                         FGMkrBeacon::MIDDLE );
	}
	if ( fabs(ils->get_imlon()) > SG_EPSILON ||
	     fabs(ils->get_imlat()) > SG_EPSILON ) {
	    globals->get_beacons()->add( ils->get_imlon(), ils->get_imlat(),
                                         ils->get_gselev(),
                                         FGMkrBeacon::INNER );
	}
    }

    // cout << "min freq = " << min << endl;
    // cout << "max freq = " << max << endl;

    return true;
}


// Query the database for the specified frequency.  It is assumed that
// there will be multiple stations with matching frequencies so a
// position must be specified.  Lon and lat are in degrees, elev is in
// meters.
FGILS *FGILSList::findByFreq( double freq,
                              double lon, double lat, double elev )
{
    FGILS *ils = NULL;

    ils_list_type stations = ilslist[(int)(freq*100.0 + 0.5)];

    double best_angle = 362.0;

    // double az1, az2, s;
    Point3D aircraft = sgGeodToCart( Point3D(lon, lat, elev) );
    Point3D station;
    double d2;
    for ( unsigned int i = 0; i < stations.size(); ++i ) {
	// cout << "  testing " << current->get_locident() << endl;
	station = Point3D(stations[i]->get_x(), 
			  stations[i]->get_y(),
			  stations[i]->get_z());
	// cout << "    aircraft = " << aircraft << " station = " << station 
	//      << endl;

	d2 = aircraft.distance3Dsquared( station );
	// cout << "  distance = " << d << " (" 
	//      << FG_ILS_DEFAULT_RANGE * SG_NM_TO_METER 
	//         * FG_ILS_DEFAULT_RANGE * SG_NM_TO_METER
	//      << ")" << endl;

	// cout << "  dist = " << s << endl;

	// match up to twice the published range so we can model
	// reduced signal strength.  The assumption is that there will
	// be maximum of two possbile stations of a particular
	// frequency in range.  If two exist, one will be associated
	// with each end of the runway.  In this case, pick the
	// station pointing most directly at us.
	if ( d2 < (2* FG_ILS_DEFAULT_RANGE * SG_NM_TO_METER 
                   * 2 * FG_ILS_DEFAULT_RANGE * SG_NM_TO_METER) ) {

            // Get our bearing from this station.
            double reciprocal_bearing, dummy;
            double a_lat_deg = lat * SGD_RADIANS_TO_DEGREES;
            double a_lon_deg = lon * SGD_RADIANS_TO_DEGREES;
            // Locator beam direction
            double s_ils_deg = stations[i]->get_locheading() - 180.0;
            if ( s_ils_deg < 0.0 ) { s_ils_deg += 360.0; }
            double angle_to_beam_deg;

            // printf("**ALI geting geo_inverse_wgs_84 with elev = %.2f, a.lat = %.2f, a.lon = %.2f,
            // s.lat = %.2f, s.lon = %.2f\n", elev,a_lat_deg,a_lon_deg,stations[i]->get_loclat(),stations[i]->get_loclon());

            geo_inverse_wgs_84( elev, stations[i]->get_loclat(),
                                stations[i]->get_loclon(), a_lat_deg, a_lon_deg,
                                &reciprocal_bearing, &dummy, &dummy );
            angle_to_beam_deg = fabs(reciprocal_bearing - s_ils_deg);
            if ( angle_to_beam_deg <= best_angle ) {
                ils = stations[i];
                best_angle = angle_to_beam_deg;
            }
	}
    }

    return ils;
}
