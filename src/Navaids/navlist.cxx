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


// Constructor
FGNavList::FGNavList( void ) {
}


// Destructor
FGNavList::~FGNavList( void ) {
}


// load the navaids and build the map
bool FGNavList::init() {

    // FIXME: leaves all the individual navaid entries leaked
    navaids.erase( navaids.begin(), navaids.end() );
    navaids_by_tile.erase( navaids_by_tile.begin(), navaids_by_tile.end() );
    ident_navaids.erase( ident_navaids.begin(), ident_navaids.end() );

    return true;
}


// real add a marker beacon
static void real_add( nav_map_type &navmap, const int master_index,
                      FGNavRecord *n )
{
    navmap[master_index].push_back( n );
}


// front end for add a marker beacon
static void tile_add( nav_map_type &navmap, FGNavRecord *n ) {
    double diff;

    double lon = n->get_lon();
    double lat = n->get_lat();

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
    // cout << "lonidx = " << lonidx << " latidx = " << latidx << "  ";
    // cout << "Master index = " << master_index << endl;

    // add to the actual bucket
    real_add( navmap, master_index, n );

    // if we are close to the edge, add to adjacent buckets so we only
    // have to search one bucket at run time

    // there are 8 cases since there are 8 adjacent tiles

    if ( lonfrac < 0.2 ) {
	real_add( navmap, master_index - 1000, n );
	if ( latfrac < 0.2 ) {
	    real_add( navmap, master_index - 1000 - 1, n );
	} else if ( latfrac > 0.8 ) {
	    real_add( navmap, master_index - 1000 + 1, n );
	}
    } else if ( lonfrac > 0.8 ) {
	real_add( navmap, master_index + 1000, n );
	if ( latfrac < 0.2 ) {
	    real_add( navmap, master_index + 1000 - 1, n );
	} else if ( latfrac > 0.8 ) {
	    real_add( navmap, master_index + 1000 + 1, n );
	}
    } else if ( latfrac < 0.2 ) {
	real_add( navmap, master_index - 1, n );
    } else if ( latfrac > 0.8 ) {
	real_add( navmap, master_index + 1, n );
    }
}



// add an entry to the lists
bool FGNavList::add( FGNavRecord *n ) {
    navaids[n->get_freq()].push_back(n);
    ident_navaids[n->get_ident()].push_back(n);
    tile_add( navaids_by_tile, n );
    return true;
}


// Query the database for the specified frequency.  It is assumed that
// there will be multiple stations with matching frequencies so a
// position must be specified.  Lon and lat are in degrees, elev is in
// meters.
FGNavRecord *FGNavList::findByFreq( double freq, double lon, double lat, double elev )
{
    nav_list_type stations = navaids[(int)(freq*100.0 + 0.5)];
    Point3D aircraft = sgGeodToCart( Point3D(lon, lat, elev) );

    return findNavFromList( aircraft, stations );
}


FGNavRecord *FGNavList::findByIdent( const char* ident,
                               const double lon, const double lat )
{
    nav_list_type stations = ident_navaids[ident];
    Point3D aircraft = sgGeodToCart( Point3D(lon, lat, 0.0) );

    return findNavFromList( aircraft, stations );
}


// Given an Ident and optional freqency, return the first matching
// station.
FGNavRecord *FGNavList::findByIdentAndFreq( const char* ident, const double freq )
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
FGNavRecord *FGNavList::findNavFromList( const Point3D &aircraft, 
                                         const nav_list_type &stations )
{
    FGNavRecord *nav = NULL;
    Point3D station;
    double dist;
    double min_dist = FG_NAV_MAX_RANGE * SG_NM_TO_METER;

    // find the closest station within a sensible range (FG_NAV_MAX_RANGE)
    for ( unsigned int i = 0; i < stations.size(); ++i ) {
	// cout << "testing " << current->get_ident() << endl;
	station = Point3D( stations[i]->get_x(),
                           stations[i]->get_y(),
                           stations[i]->get_z() );

	dist = aircraft.distance3D( station );

	// cout << "  dist = " << sqrt(d)
	//      << "  range = " << current->get_range() * SG_NM_TO_METER
        //      << endl;

        if ( dist < min_dist ) {
            min_dist = dist;
            nav = stations[i];
        }
    }

    return nav;
}


// returns the closest entry to the give lon/lat/elev
FGNavRecord *FGNavList::findClosest( double lon_rad, double lat_rad,
                                     double elev_m )
{
    FGNavRecord *result = NULL;
    double diff;

    double lon_deg = lon_rad * SG_RADIANS_TO_DEGREES;
    double lat_deg = lat_rad * SG_RADIANS_TO_DEGREES;
    int lonidx = (int)lon_deg;
    diff = lon_deg - (double)lonidx;
    if ( (lon_deg < 0.0) && (fabs(diff) > SG_EPSILON) ) {
	lonidx -= 1;
    }
    lonidx += 180;

    int latidx = (int)lat_deg;
    diff = lat_deg - (double)latidx;
    if ( (lat_deg < 0.0) && (fabs(diff) > SG_EPSILON) ) {
	latidx -= 1;
    }
    latidx += 90;

    int master_index = lonidx * 1000 + latidx;
    
    nav_list_type navs = navaids_by_tile[ master_index ];
    // cout << "Master index = " << master_index << endl;
    // cout << "beacon search length = " << beacons.size() << endl;

    nav_list_iterator current = navs.begin();
    nav_list_iterator last = navs.end();

    Point3D aircraft = sgGeodToCart( Point3D(lon_rad,
                                             lat_rad,
                                             elev_m) );

    double min_dist = 999999999.0;

    for ( ; current != last ; ++current ) {
	// cout << "  testing " << (*current)->get_ident() << endl;
	Point3D station = Point3D( (*current)->get_x(),
				   (*current)->get_y(),
				   (*current)->get_z() );
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
            result = (*current);
	}
    }

    // cout << "lon = " << lon << " lat = " << lat
    //      << "  closest beacon = " << sqrt( min_dist ) << endl;

    return result;
}
