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


#include <simgear/debug/logstream.hxx>

#include <Main/globals.hxx>

#include "navrecord.hxx"

#include "navdb.hxx"


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
