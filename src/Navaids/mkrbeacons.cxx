// mkrbeacons.cxx -- marker beacon management class
//
// Written by Curtis Olson, started March 2001.
//
// Copyright (C) 2001  Curtis L. Olson - curt@flightgear.org
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


#include "mkrbeacons.hxx"


// constructor
FGMkrBeacon::FGMkrBeacon() {
    FGMkrBeacon( 0, 0, 0, NOBEACON );
}

FGMkrBeacon::FGMkrBeacon( double _lon, double _lat, double _elev,
		    fgMkrBeacType _type ) {
    lon = _lon;
    lat = _lat;
    elev = _elev;
    type = _type;

    Point3D pos = sgGeodToCart(Point3D(lon * SGD_DEGREES_TO_RADIANS, lat * SGD_DEGREES_TO_RADIANS, 0));
    // cout << "pos = " << pos << endl;
    x = pos.x();
    y = pos.y();
    z = pos.z();
}

// destructor
FGMkrBeacon::~FGMkrBeacon() {
}


// constructor
FGMarkerBeacons::FGMarkerBeacons() {
}

// destructor
FGMarkerBeacons::~FGMarkerBeacons() {
}


// initialize the structures
bool FGMarkerBeacons::init() {
    // erase the beacon map
    beacon_map.clear();

    return true;
}


// real add a marker beacon
bool FGMarkerBeacons::real_add( const int master_index, const FGMkrBeacon& b ) {
    // cout << "Master index = " << master_index << endl;
    beacon_map[master_index].push_back( b );
    
    return true;
}


// front end for add a marker beacon
bool FGMarkerBeacons::add( double lon, double lat, double elev,
			   FGMkrBeacon::fgMkrBeacType type ) {
    double diff;

    int lonidx = (int)lon;
    diff = lon - (double)lonidx;
    if ( (lon < 0.0) && (fabs(diff) > SG_EPSILON) ) {
	lonidx -= 1;
    }
    double lonfrac = lon - (double)lonidx;
    lonidx += 180;

    int latidx = (int)lat;
    diff = lat - (double)latidx;
    if ( (lat < 0.0) && (fabs(diff) > SG_EPSILON) ) {
	latidx -= 1;
    }
    double latfrac = lat - (double)latidx;
    latidx += 90;

    int master_index = lonidx * 1000 + latidx;
    FGMkrBeacon b( lon, lat, elev, type );

    // add to the actual bucket
    real_add( master_index, b );

    // if we are close to the edge, add to adjacent buckets so we only
    // have to search one bucket at run time

    // there are 8 cases since there are 8 adjacent tiles

    if ( lonfrac < 0.2 ) {
	real_add( master_index - 1000, b );
	if ( latfrac < 0.2 ) {
	    real_add( master_index - 1000 - 1, b );
	} else if ( latfrac > 0.8 ) {
	    real_add( master_index - 1000 + 1, b );
	}
    } else if ( lonfrac > 0.8 ) {
	real_add( master_index + 1000, b );
	if ( latfrac < 0.2 ) {
	    real_add( master_index + 1000 - 1, b );
	} else if ( latfrac > 0.8 ) {
	    real_add( master_index+- 1000 + 1, b );
	}
    } else if ( latfrac < 0.2 ) {
	real_add( master_index - 1, b );
    } else if ( latfrac > 0.8 ) {
	real_add( master_index + 1, b );
    }

    return true;
}


// returns marker beacon type if we are over a marker beacon, NOBEACON
// otherwise
FGMkrBeacon::fgMkrBeacType FGMarkerBeacons::query( double lon, double lat,
                                                   double elev )
{
    double diff;

    int lonidx = (int)lon;
    diff = lon - (double)lonidx;
    if ( (lon < 0.0) && (fabs(diff) > SG_EPSILON) ) {
	lonidx -= 1;
    }
    lonidx += 180;

    int latidx = (int)lat;
    diff = lat - (double)latidx;
    if ( (lat < 0.0) && (fabs(diff) > SG_EPSILON) ) {
	latidx -= 1;
    }
    latidx += 90;

    int master_index = lonidx * 1000 + latidx;

    beacon_list_type beacons = beacon_map[ master_index ];
    // cout << "Master index = " << master_index << endl;
    // cout << "beacon search length = " << beacons.size() << endl;

    beacon_list_iterator current = beacons.begin();
    beacon_list_iterator last = beacons.end();

    Point3D aircraft = sgGeodToCart( Point3D(lon*SGD_DEGREES_TO_RADIANS,
                                             lat*SGD_DEGREES_TO_RADIANS, 0) );

    double min_dist = 999999999.0;

    for ( ; current != last ; ++current ) {
	// cout << "  testing " << current->get_locident() << endl;
	Point3D station = Point3D( current->get_x(),
				   current->get_y(),
				   current->get_z() );
	// cout << "    aircraft = " << aircraft << " station = " << station 
	//      << endl;

	double d = aircraft.distance3Dsquared( station ); // meters^2
	// cout << "  distance = " << d << " (" 
	//      << FG_ILS_DEFAULT_RANGE * SG_NM_TO_METER 
	//         * FG_ILS_DEFAULT_RANGE * SG_NM_TO_METER
	//      << ")" << endl;

	// cout << "  range = " << sqrt(d) << endl;

	if ( d < min_dist ) {
	    min_dist = d;
	}

	// cout << "elev = " << elev * SG_METER_TO_FEET
	//      << " current->get_elev() = " << current->get_elev() << endl;
	double delev = elev * SG_METER_TO_FEET - current->get_elev();

	// max range is the area under r = 2.4 * alt or r^2 = 4000^2 - alt^2
	// whichever is smaller.  The intersection point is 1538 ...
	double maxrange2;	// feet^2
	if ( delev < 1538.0 ) {
	    maxrange2 = 2.4 * 2.4 * delev * delev;
	} else if ( delev < 4000.0 ) {
	    maxrange2 = 4000 * 4000 - delev * delev;
	} else {
	    maxrange2 = 0.0;
	}
	maxrange2 *= SG_FEET_TO_METER * SG_FEET_TO_METER; // convert to meter^2
	// cout << "delev = " << delev << " maxrange = " << maxrange << endl;

	// match up to twice the published range so we can model
	// reduced signal strength
	if ( d < maxrange2 ) {
	    // cout << "lon = " << lon << " lat = " << lat
	    //      << "  closest beacon = " << sqrt( min_dist ) << endl;
	    return current->get_type();
	}
    }

    // cout << "lon = " << lon << " lat = " << lat
    //      << "  closest beacon = " << sqrt( min_dist ) << endl;

    return FGMkrBeacon::NOBEACON;
}


FGMarkerBeacons *current_beacons;
