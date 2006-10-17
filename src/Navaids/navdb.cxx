// navdb.cxx -- top level navaids management routines
//
// Written by Curtis Olson, started May 2004.
//
// Copyright (C) 2004  Curtis L. Olson - http://www.flightgear.org/~curt
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
#  include "config.h"
#endif

#include <simgear/compiler.h>

#include STL_STRING

#include <simgear/debug/logstream.hxx>

#include <Airports/runways.hxx>
#include <Airports/simple.hxx>
#include <Main/globals.hxx>

#include "navrecord.hxx"
#include "navdb.hxx"

SG_USING_STD( string );


// load and initialize the navigational databases
bool fgNavDBInit( FGAirportList *airports,
                  FGNavList *navlist, FGNavList *loclist, FGNavList *gslist,
                  FGNavList *dmelist, FGNavList *mkrlist, 
                  FGNavList *tacanlist, FGNavList *carrierlist,
                  FGTACANList *channellist)
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

	/*cout << "id = " << r->get_ident() << endl;
	cout << " type = " << r->get_type() << endl;
	cout << " lon = " << r->get_lon() << endl;
	cout << " lat = " << r->get_lat() << endl;
	cout << " elev = " <<r->get_elev_ft() << endl;
	cout << " freq = " << r->get_freq() << endl;
	cout << " range = " << r->get_range() << endl;
 	cout << " name = " << r->get_name() << endl << endl; */

        // fudge elevation to the field elevation if it's not specified
        if ( fabs(r->get_elev_ft()) < 0.01 && r->get_apt_id().length() ) {
            // cout << r->get_type() << " " << r->get_apt_id() << " zero elev"
            //      << endl;
            const FGAirport* a = airports->search( r->get_apt_id() );
            if ( a ) {
                r->set_elev_ft( a->getElevation() );
                // cout << "  setting to " << a.elevation << endl;
            }
        }

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
        } else if ( r->get_type() == 12 || r->get_type() == 13) {
            // DME with ILS=12; standalone DME=13
            string str1( r->get_name() );
            string::size_type loc1= str1.find( "TACAN", 0 );
            string::size_type loc2 = str1.find( "VORTAC", 0 );
                       
            if( loc1 != string::npos || loc2 != string::npos ){
                 //cout << " name = " << r->get_name() ;
                 //cout << " freq = " << r->get_freq() ;
                 tacanlist->add( r );
                 }
                
            dmelist->add( r );
            
        }
		
        in >> skipcomment;
    }

// load the carrier navaids file
    
    string file, name;
    path = "";
    path = globals->get_fg_root() ;
    path.append( "Navaids/carrier_nav.dat" );
    
    file = path.str();
    SG_LOG( SG_GENERAL, SG_INFO, "opening file: " << path.str() );
    
    sg_gzifstream incarrier( path.str() );
    
    if ( !incarrier.is_open() ) {
        SG_LOG( SG_GENERAL, SG_ALERT, "Cannot open file: " << path.str() );
        exit(-1);
    }
    
    // skip first two lines
    //incarrier >> skipeol;
    //incarrier >> skipeol;
    
#ifdef __MWERKS__
    char c = 0;
    while ( incarrier.get(c) && c != '\0'  ) {
        incarrier.putback(c);
#else
    while ( ! incarrier.eof() ) {
#endif
        
        FGNavRecord *r = new FGNavRecord;
        incarrier >> (*r);
        carrierlist->add ( r );
        /*cout << " carrier lon: "<<  r->get_lon() ;
        cout << " carrier lat: "<<  r->get_lat() ;
        cout << " freq: " << r->get_freq() ;
        cout << " carrier name: "<<  r->get_name() << endl;*/
                
        //if ( r->get_type() > 95 ) {
        //   break;}
    } // end while

// end loading the carrier navaids file

// load the channel/freqency file
    string channel, freq;
    path="";
    path = globals->get_fg_root();
    path.append( "Navaids/TACAN_freq.dat" );
    
    SG_LOG( SG_GENERAL, SG_INFO, "opening file: " << path.str() );
        
    sg_gzifstream inchannel( path.str() );
    
    if ( !inchannel.is_open() ) {
        SG_LOG( SG_GENERAL, SG_ALERT, "Cannot open file: " << path.str() );
        exit(-1);
    }
    
    // skip first line
    inchannel >> skipeol;
    
#ifdef __MWERKS__
    char c = 0;
    while ( inchannel.get(c) && c != '\0'  ) {
        in.putback(c);
#else
    while ( ! inchannel.eof() ) {
#endif

        FGTACANRecord *r = new FGTACANRecord;
        inchannel >> (*r);
        channellist->add ( r );
       	//cout << "channel = " << r->get_channel() ;
       	//cout << " freq = " << r->get_freq() << endl;
 	
    } // end while

 
 // end ReadChanFile


    return true;
}


// Given a localizer record and it's corresponding runway record,
// adjust the localizer position so it is in perfect alignment with
// the runway.
static void update_loc_position( FGNavRecord *loc, FGRunway *rwy,
                                 double threshold )
{
    double hdg = rwy->_heading;
    hdg += 180.0;
    if ( hdg > 360.0 ) {
        hdg -= 360.0;
    }

    // calculate runway threshold point
    double thresh_lat, thresh_lon, return_az;
    geo_direct_wgs_84 ( 0.0, rwy->_lat, rwy->_lon, hdg,
                        rwy->_length/2.0 * SG_FEET_TO_METER,
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

    // cout << "orig heading = " << loc->get_multiuse() << endl;
    // cout << "new heading = " << rwy->_heading << endl;

    double hdg_diff = loc->get_multiuse() - rwy->_heading;

    // clamp to [-180.0 ... 180.0]
    if ( hdg_diff < -180.0 ) {
        hdg_diff += 360.0;
    } else if ( hdg_diff > 180.0 ) {
        hdg_diff -= 360.0;
    }

    if ( fabs(hdg_diff) <= threshold ) {
        loc->set_lat( nloc_lat );
        loc->set_lon( nloc_lon );
        loc->set_multiuse( rwy->_heading );
    }
}


// This routines traverses the localizer list and attempts to match
// each entry with it's corresponding runway.  When it is successful,
// it then "moves" the localizer and updates it's heading so it
// *perfectly* aligns with the runway, but is still the same distance
// from the runway threshold.
void fgNavDBAlignLOCwithRunway( FGRunwayList *runways, FGNavList *loclist,
                                double threshold ) {
    nav_map_type navmap = loclist->get_navaids();

    nav_map_const_iterator freq = navmap.begin();
    while ( freq != navmap.end() ) {
        nav_list_type locs = freq->second;
        nav_list_const_iterator loc = locs.begin();
        while ( loc != locs.end() ) {
            string name = (*loc)->get_name();
            string::size_type pos1 = name.find(" ");
            string id = name.substr(0, pos1);
            name = name.substr(pos1+1);
            string::size_type pos2 = name.find(" ");
            string rwy = name.substr(0, pos2);

            FGRunway r;
            if ( runways->search(id, rwy, &r) ) {
                update_loc_position( (*loc), &r, threshold );
            }
            ++loc;
        }
        ++freq;
    }
}

