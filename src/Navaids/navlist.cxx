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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sgstream.hxx>
#include <simgear/math/sg_geodesy.hxx>

#include "navlist.hxx"


// Return true if the nav record matches the type
static bool isTypeMatch(const FGNavRecord* n, fg_nav_types type)
{
  if (type == FG_NAV_ANY) return true;
  return type == n->type();
}



// FGNavList ------------------------------------------------------------------

FGNavList::FGNavList( void )
{
}


FGNavList::~FGNavList( void )
{
    navaids_by_tile.erase( navaids_by_tile.begin(), navaids_by_tile.end() );
    nav_list_type navlist = navaids.begin()->second;
    navaids.erase( navaids.begin(), navaids.end() );
}


// load the navaids and build the map
bool FGNavList::init()
{
    // No need to delete the original navaid structures
    // since we're using an SGSharedPointer
    nav_list_type navlist = navaids.begin()->second;
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
static void tile_add( nav_map_type &navmap, FGNavRecord *n )
{
    double diff = 0;

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
bool FGNavList::add( FGNavRecord *n )
{
    navaids[n->get_freq()].push_back(n);
    ident_navaids[n->get_ident()].push_back(n);
    tile_add( navaids_by_tile, n );
    return true;
}


FGNavRecord *FGNavList::findByFreq( double freq, double lon, double lat, double elev )
{
    const nav_list_type& stations = navaids[(int)(freq*100.0 + 0.5)];

    SGGeod geod = SGGeod::fromRadM(lon, lat, elev);
    SGVec3d aircraft = SGVec3d::fromGeod(geod);
    SG_LOG( SG_INSTR, SG_DEBUG, "findbyFreq " << freq << " size " << stations.size()  );

    return findNavFromList( aircraft, stations );
}


FGNavRecord *FGNavList::findByIdent( const char* ident,
                               const double lon, const double lat )
{
    const nav_list_type& stations = ident_navaids[ident];
    SGGeod geod = SGGeod::fromRad(lon, lat);
    SGVec3d aircraft = SGVec3d::fromGeod(geod);
    return findNavFromList( aircraft, stations );
}


nav_list_type FGNavList::findFirstByIdent( const string& ident, fg_nav_types type, bool exact)
{
  nav_list_type n2;
  n2.clear();

  if ((type != FG_NAV_VOR) && (type != FG_NAV_NDB)) {
    return n2;
  }
  
    nav_ident_map_iterator it;
    if(exact) {
        it = ident_navaids.find(ident);
    } else {
        bool typeMatch = false;
        int safety_count = 0;
        it = ident_navaids.lower_bound(ident);
        while(!typeMatch) {
            nav_list_type n0 = it->second;
            // local copy, so we should be able to do anything with n0.
            // Remove the types that don't match request.
            for(nav_list_iterator it0 = n0.begin(); it0 != n0.end();) {
                FGNavRecord* nv = *it0;
                if(nv->type() == type) {
                    typeMatch = true;
                    ++it0;
                } else {
                    it0 = n0.erase(it0);
                }
            }
            if(typeMatch) {
                return(n0);
            }
            if(it == ident_navaids.begin()) {
                // We didn't find a match before reaching the beginning of the map
                n0.clear();
                return(n0);
            }
            safety_count++;
            if(safety_count == 1000000) {
                SG_LOG(SG_INSTR, SG_ALERT,
                       "safety_count triggered exit from while loop in findFirstByIdent!");
                break;
            }
            ++it;
            if(it == ident_navaids.end()) {
                n0.clear();
                return(n0);
            }
        }
    }
    if(it == ident_navaids.end()) {
        n2.clear();
        return(n2);
    } else {
        nav_list_type n1 = it->second;
        n2.clear();
        for(nav_list_iterator it2 = n1.begin(); it2 != n1.end(); ++it2) {
            FGNavRecord* nv = *it2;
            if(nv->type() == type) n2.push_back(nv);
        }
        return(n2);
    }
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
        nav_list_const_iterator it, end = stations.end();
        for ( it = stations.begin(); it != end; ++it ) {
            if ( f == (*it)->get_freq() ) {
                return (*it);
            }
        }
    } else if (!stations.empty()) {
        return stations[0];
    }

    return NULL;
}

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

static bool penaltyForNav(FGNavRecord* aNav, const SGVec3d &aPos)
{
  switch (aNav->type()) {
  case FGPositioned::ILS:
  case FGPositioned::LOC:
  case FGPositioned::GS:
// FIXME
//  case FGPositioned::DME: we can't get the heading for a DME transmitter, oops
    break;
  default:
    return false;
  }
  
  double hdg_deg = 0.0;
  if (aNav->type() == FGPositioned::GS) {
    int tmp = (int)(aNav->get_multiuse() / 1000.0);
    hdg_deg = aNav->get_multiuse() - (tmp * 1000);
  } else {    
    hdg_deg = aNav->get_multiuse();
  }
  
  double az1 = 0.0, az2 = 0.0, s = 0.0;
  SGGeod geod = SGGeod::fromCart(aPos);
  geo_inverse_wgs_84( geod, aNav->geod(), &az1, &az2, &s);
  az1 = az1 - hdg_deg;
  
  if ( az1 >  180.0) az1 -= 360.0;
  if ( az1 < -180.0) az1 += 360.0;
  
  return fabs(az1) > 90.0;
}

// Given a point and a list of stations, return the closest one to the
// specified point.
FGNavRecord *FGNavList::findNavFromList( const SGVec3d &aircraft,
                                         const nav_list_type &stations )
{
    FGNavRecord *nav = NULL;
    double d2;                  // in meters squared
    double min_dist
        = FG_NAV_MAX_RANGE*SG_NM_TO_METER*FG_NAV_MAX_RANGE*SG_NM_TO_METER;

    nav_list_const_iterator it;
    nav_list_const_iterator end = stations.end();
    // find the closest station within a sensible range (FG_NAV_MAX_RANGE)
    for ( it = stations.begin(); it != end; ++it ) {
        FGNavRecord *station = *it;
        // cout << "testing " << current->get_ident() << endl;
        d2 = distSqr(station->get_cart(), aircraft);
        if ( d2 < min_dist && penaltyForNav(station, aircraft))
        {
          double dist = sqrt(d2);
          d2 = (dist + 5000) * (dist + 5000);
        }
        
        if ( d2 < min_dist ) {
            min_dist = d2;
            nav = station;
        }
    }

    return nav;
}


// returns the closest entry to the give lon/lat/elev
FGNavRecord *FGNavList::findClosest( double lon_rad, double lat_rad,
                                     double elev_m, fg_nav_types type)
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

    const nav_list_type& navs = navaids_by_tile[ master_index ];
    // cout << "Master index = " << master_index << endl;
    // cout << "beacon search length = " << beacons.size() << endl;

    nav_list_const_iterator current = navs.begin();
    nav_list_const_iterator last = navs.end();

    SGGeod geod = SGGeod::fromRadM(lon_rad, lat_rad, elev_m);
    SGVec3d aircraft = SGVec3d::fromGeod(geod);

    double min_dist = 999999999.0;

    for ( ; current != last ; ++current ) {
        if(isTypeMatch(*current, type)) {
            // cout << "  testing " << (*current)->get_ident() << endl;

            double d = distSqr((*current)->get_cart(), aircraft);
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
    }

    // cout << "lon = " << lon << " lat = " << lat
    //      << "  closest beacon = " << sqrt( min_dist ) << endl;

    return result;
}

// Given a frequency, return the first matching station.
FGNavRecord *FGNavList::findStationByFreq( double freq )
{
    const nav_list_type& stations = navaids[(int)(freq*100.0 + 0.5)];

    SG_LOG( SG_INSTR, SG_DEBUG, "findStationByFreq " << freq << " size " << stations.size()  );

    if (!stations.empty()) {
        return stations[0];
    }
    return NULL;
}



// FGTACANList ----------------------------------------------------------------

FGTACANList::FGTACANList( void )
{
}


FGTACANList::~FGTACANList( void )
{
}


bool FGTACANList::init()
{
    return true;
}


// add an entry to the lists
bool FGTACANList::add( FGTACANRecord *c )
{
    ident_channels[c->get_channel()].push_back(c);
    return true;
}


// Given a TACAN Channel return the first matching frequency
FGTACANRecord *FGTACANList::findByChannel( const string& channel )
{
    const tacan_list_type& stations = ident_channels[channel];
    SG_LOG( SG_INSTR, SG_DEBUG, "findByChannel " << channel<< " size " << stations.size() );

    if (!stations.empty()) {
        return stations[0];
    }
    return NULL;
}


