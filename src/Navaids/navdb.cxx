// navdb.cxx -- top level navaids management routines
//
// Written by Curtis Olson, started May 2004.
//
// Copyright (C) 2004  Curtis L. Olson - curt@flightgear.org
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


#include <simgear/compiler.h>

#include STL_STRING

#include <simgear/debug/logstream.hxx>

#include <Airports/runways.hxx>
#include <Main/globals.hxx>

#include "navrecord.hxx"
#include "navdb.hxx"

SG_USING_STD( string );


// load and initialize the navigational databases
bool fgNavDBInit( FGNavList *navlist, FGNavList *loclist, FGNavList *gslist,
                  FGNavList *dmelist, FGNavList *mkrlist )
{
    SG_LOG(SG_GENERAL, SG_INFO, "Loading Navaid Databases");
    // SG_LOG(SG_GENERAL, SG_INFO, "  VOR/NDB");
    // SGPath p_nav( globals->get_fg_root() );
    // p_nav.append( "Navaids/default.nav" );
    // navlist->init( p_nav );

    // SG_LOG(SG_GENERAL, SG_INFO, "  ILS and Marker Beacons");
    // beacons->init();
    // SGPath p_ils( globals->get_fg_root() );
    // p_ils.append( "Navaids/default.ils" );
    // ilslist->init( p_ils );


    SGPath path( globals->get_fg_root() );
    path.append( "Navaids/nav.dat" );

    sg_gzifstream in( path.str() );
    if ( !in.is_open() ) {
        SG_LOG( SG_GENERAL, SG_ALERT, "Cannot open file: " << path.str() );
        exit(-1);
    }

    // skip first two lines
    in >> skipeol;
    in >> skipeol;


#ifdef __MWERKS__
    char c = 0;
    while ( in.get(c) && c != '\0'  ) {
        in.putback(c);
#else
    while ( ! in.eof() ) {
#endif

        FGNavRecord *r = new FGNavRecord;
        in >> (*r);
        if ( r->get_type() > 95 ) {
            break;
        }

	/* cout << "id = " << n.get_ident() << endl;
	cout << " type = " << n.get_type() << endl;
	cout << " lon = " << n.get_lon() << endl;
	cout << " lat = " << n.get_lat() << endl;
	cout << " elev = " << n.get_elev() << endl;
	cout << " freq = " << n.get_freq() << endl;
 	cout << " range = " << n.get_range() << endl << endl; */

        if ( r->get_type() == 2 || r->get_type() == 3 ) {
            // NDB=2, VOR=3
            navlist->add( r );
        } else if ( r->get_type() == 4 || r->get_type() == 5 ) {
            // ILS=4, LOC(only)=5
            loclist->add( r );
        } else if ( r->get_type() == 6 ) {
            // GS=6
            gslist->add( r );
        } else if ( r->get_type() == 7 || r->get_type() == 8
                    || r->get_type() == 9 )
        {
            // Marker Beacon = 7,8,9
            mkrlist->add( r );
        } else if ( r->get_type() == 12 ) {
            // DME=12
            dmelist->add( r );
        }
		
        in >> skipcomment;
    }

    // cout << "min freq = " << min << endl;
    // cout << "max freq = " << max << endl;

    return true;
}


// Given a localizer record and it's corresponding runway record,
// adjust the localizer position so it is in perfect alignment with
// the runway.
static void update_loc_position( FGNavRecord *loc, FGRunway *rwy ) {

    double hdg = rwy->heading;
    hdg += 180.0;
    if ( hdg > 360.0 ) {
        hdg -= 360.0;
    }

    // calculate runway threshold point
    double thresh_lat, thresh_lon, return_az;
    geo_direct_wgs_84 ( 0.0, rwy->lat, rwy->lon, hdg,
                        rwy->length/2.0 * SG_FEET_TO_METER,
                        &thresh_lat, &thresh_lon, &return_az );
    // cout << "Threshold = " << thresh_lat << "," << thresh_lon << endl;

    // calculate distance from threshold to localizer
    double az1, az2, dist_m;
    geo_inverse_wgs_84( 0.0, loc->get_lat(), loc->get_lon(),
                        thresh_lat, thresh_lon,
                        &az1, &az2, &dist_m );
    // cout << "Distance = " << dist_m << endl;

    // back project that distance along the runway center line
    double nloc_lat, nloc_lon;
    geo_direct_wgs_84 ( 0.0, thresh_lat, thresh_lon, hdg + 180.0, 
                        dist_m, &nloc_lat, &nloc_lon, &return_az );
    // printf("New localizer = %.6f %.6f\n", nloc_lat, nloc_lon );

    // sanity check, how far have we moved the localizer?
    geo_inverse_wgs_84( 0.0, loc->get_lat(), loc->get_lon(),
                        nloc_lat, nloc_lon,
                        &az1, &az2, &dist_m );
    // cout << "Distance moved = " << dist_m << endl;

    loc->set_lat( nloc_lat );
    loc->set_lon( nloc_lon );
    loc->set_multiuse( rwy->heading );

    // cout << "orig heading = " << loc->get_multiuse() << endl;
    // cout << "new heading = " << rwy->heading << endl;
}


// This routines traverses the localizer list and attempts to match
// each entry with it's corresponding runway.  When it is successful,
// it then "moves" the localizer and updates it's heading so it
// *perfectly* aligns with the runway, but is still the same distance
// from the runway threshold.
void fgNavDBAlignLOCwithRunway( FGRunwayList *runways, FGNavList *loclist ) {
    nav_map_type navmap = loclist->get_navaids();

    nav_map_iterator freq = navmap.begin();
    while ( freq != navmap.end() ) {
        nav_list_type locs = freq->second;
        nav_list_iterator loc = locs.begin();
        while ( loc != locs.end() ) {
            string name = (*loc)->get_name();
            string::size_type pos1 = name.find(" ");
            string id = name.substr(0, pos1);
            name = name.substr(pos1+1);
            string::size_type pos2 = name.find(" ");
            string rwy = name.substr(0, pos2);

            FGRunway r;
            if ( runways->search(id, rwy, &r) ) {
                update_loc_position( (*loc), &r );
            }
            ++loc;
        }
        ++freq;
    }
}
