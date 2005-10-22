// navlist.cxx -- navaids management class
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

FGTACANList::FGTACANList( void ){
}


// Destructor
FGNavList::~FGNavList( void ) {
}

FGTACANList::~FGTACANList( void ){
}

// load the navaids and build the map
bool FGNavList::init() {

    // FIXME: leaves all the individual navaid entries leaked
    navaids.erase( navaids.begin(), navaids.end() );
    navaids_by_tile.erase( navaids_by_tile.begin(), navaids_by_tile.end() );
    ident_navaids.erase( ident_navaids.begin(), ident_navaids.end() );

    return true;
}

bool FGTACANList::init() {
    
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

// add an entry to the lists
bool FGTACANList::add( FGTACANRecord *c ) {
    ident_channels[c->get_channel()].push_back(c);
    return true;
}

FGNavRecord *FGNavList::findByFreq( double freq, double lon, double lat, double elev )
{
    nav_list_type stations = navaids[(int)(freq*100.0 + 0.5)];
    Point3D aircraft = sgGeodToCart( Point3D(lon, lat, elev) );
    SG_LOG( SG_INSTR, SG_DEBUG, "findbyFreq " << freq << " size " << stations.size()  );

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
    SG_LOG( SG_INSTR, SG_DEBUG, "findByIdent " << ident<< " size " << stations.size()  );
    if ( freq > 0.0 ) {
        // sometimes there can be duplicated idents.  If a freq is
        // specified, use it to refine the search.
        int f = (int)(freq*100.0 + 0.5);
        for ( unsigned int i = 0; i < stations.size(); ++i ) {
            if ( f == stations[i]->get_freq() ) {
                return stations[i];
            }
        }
    } else if (stations.size()) {
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
    double d2;                  // in meters squared
    double min_dist
        = FG_NAV_MAX_RANGE*SG_NM_TO_METER*FG_NAV_MAX_RANGE*SG_NM_TO_METER;

    // find the closest station within a sensible range (FG_NAV_MAX_RANGE)
    for ( unsigned int i = 0; i < stations.size(); ++i ) {
	// cout << "testing " << current->get_ident() << endl;
	station = Point3D( stations[i]->get_x(),
                           stations[i]->get_y(),
                           stations[i]->get_z() );

	d2 = aircraft.distance3Dsquared( station );

	// cout << "  dist = " << sqrt(d)
	//      << "  range = " << current->get_range() * SG_NM_TO_METER
        //      << endl;

        // LOC, ILS, GS, and DME antenna's could potentially be
        // installed at the opposite end of the runway.  So it's not
        // enough to simply find the closest antenna with the right
        // frequency.  We need the closest antenna with the right
        // frequency that is most oriented towards us.  (We penalize
        // stations that are facing away from us by adding 5000 meters
        // which is further than matching stations would ever be
        // placed from each other.  (Do the expensive check only for
        // directional atennas and only when there is a chance it is
        // the closest station.)
	if ( d2 < min_dist &&
             (stations[i]->get_type() == 4 || stations[i]->get_type() == 5 ||
              stations[i]->get_type() == 6 || stations[i]->get_type() == 12) )
        {
            double hdg_deg = 0.0;
            if ( stations[i]->get_type() == 4 || stations[i]->get_type() == 5 ){
                hdg_deg = stations[i]->get_multiuse();
            } else if ( stations[i]->get_type() == 6 ) {
                int tmp = (int)(stations[i]->get_multiuse() / 1000.0);
                hdg_deg = stations[i]->get_multiuse() - (tmp * 1000);
            } else if ( stations[i]->get_type() == 12 ) {
                // oops, Robin's data format doesn't give us the
                // needed information to compute a heading for a DME
                // transmitter.  FIXME Robin!
            }

            double az1 = 0.0, az2 = 0.0, s = 0.0;
            double elev_m = 0.0, lat_rad = 0.0, lon_rad = 0.0;
            double xyz[3] = { aircraft.x(), aircraft.y(), aircraft.z() };
            sgCartToGeod( xyz, &lat_rad, &lon_rad, &elev_m );
            geo_inverse_wgs_84( elev_m,
                                lat_rad * SG_RADIANS_TO_DEGREES,
                                lon_rad * SG_RADIANS_TO_DEGREES,
                                stations[i]->get_lat(), stations[i]->get_lon(),
                                &az1, &az2, &s);
            az1 = az1 - stations[i]->get_multiuse();
            if ( az1 >  180.0) az1 -= 360.0;
            if ( az1 < -180.0) az1 += 360.0;
            // penalize opposite facing stations by adding 5000 meters
            // (squared) which is further than matching stations would
            // ever be placed from each other.
            if ( fabs(az1) > 90.0 ) {
                double dist = sqrt(d2);
                d2 = (dist + 5000) * (dist + 5000);
            }
        }

        if ( d2 < min_dist ) {
            min_dist = d2;
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

// Given a TACAN Channel return the first matching frequency
FGTACANRecord *FGTACANList::findByChannel( string channel )
{
    tacan_list_type stations = ident_channels[channel];
    SG_LOG( SG_INSTR, SG_DEBUG, "findByChannel " << channel<< " size " << stations.size()  );
    
    if (stations.size()) {
        return stations[0];
    }    
    return NULL;
}

// Given a frequency, return the first matching station.
FGNavRecord *FGNavList::findStationByFreq( double freq )
{
    nav_list_type stations = navaids[(int)(freq*100.0 + 0.5)];
   
    SG_LOG( SG_INSTR, SG_DEBUG, "findStationByFreq " << freq << " size " << stations.size()  );
    
    if (stations.size()) {
        return stations[0];
    }    
    return NULL;
}
