// atislist.cxx -- navaids management class
//
// Written by David Luff, started October 2001.
// Based on navlist.cxx by Curtis Olson, started April 2000.
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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sgstream.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/math/sg_random.h>

#include "atislist.hxx"


FGATISList *current_atislist;


// Constructor
FGATISList::FGATISList( void ) {
}


// Destructor
FGATISList::~FGATISList( void ) {
}


// load the navaids and build the map
bool FGATISList::init( SGPath path ) {

    atislist.erase( atislist.begin(), atislist.end() );

    sg_gzifstream in( path.str() );
    if ( !in.is_open() ) {
        SG_LOG( SG_GENERAL, SG_ALERT, "Cannot open file: " << path.str() );
        exit(-1);
    }

    // read in each line of the file

    // in >> skipeol;
    in >> skipcomment;

    // double min = 100000;
    // double max = 0;

#ifdef __MWERKS__
    char c = 0;
    while ( in.get(c) && c != '\0' ) {
        in.putback(c);
#else
    while ( ! in.eof() ) {
#endif
    
        FGATIS a;
        in >> a;
	if ( a.get_type() == '[' ) {
            break;
        }
	
	/* cout << "id = " << a.GetIdent() << endl;
	cout << " type = " << a.get_type() << endl;
	cout << " lon = " << a.get_lon() << endl;
	cout << " lat = " << a.get_lat() << endl;
	cout << " elev = " << a.get_elev() << endl;
	cout << " freq = " << a.get_freq() << endl;
 	cout << " range = " << a.get_range() << endl << endl; */

        atislist[a.get_freq()].push_back(a);
        in >> skipcomment;

	/* if ( a.get_type() != 'N' ) {
	    if ( a.get_freq() < min ) {
		min = a.get_freq();
	    }
	    if ( a.get_freq() > max ) {
		max = a.get_freq();
	    }
	} */
    }

    // cout << "min freq = " << min << endl;
    // cout << "max freq = " << max << endl;

    return true;
}


// query the database for the specified frequency, lon and lat are in
// degrees, elev is in meters
bool FGATISList::query( double lon, double lat, double elev, double freq,
		       FGATIS *a )
{
    atis_list_type stations = atislist[(int)(freq*100.0 + 0.5)];

    atis_list_iterator current = stations.begin();
    atis_list_iterator last = stations.end();

    // double az1, az2, s;
    Point3D aircraft = sgGeodToCart( Point3D(lon, lat, elev) );
    Point3D station;
    double d;
    for ( ; current != last ; ++current ) {
	//cout << "testing " << current->get_ident() << endl;
	station = Point3D(current->get_x(), current->get_y(), current->get_z());
	//cout << "aircraft = " << aircraft << endl;
	//cout << "station = " << station << endl;

	d = aircraft.distance3Dsquared( station );

	//cout << "  dist = " << sqrt(d)
	//     << "  range = " << current->get_range() * SG_NM_TO_METER << endl;

	// match up to twice the published range so we can model
	// reduced signal strength
	if ( d < (2 * current->get_range() * SG_NM_TO_METER 
		  * 2 * current->get_range() * SG_NM_TO_METER ) ) {
	    //cout << "matched = " << current->get_ident() << endl;
	    *a = *current;
	    return true;
	}
    }

    return false;
}


int FGATISList::GetCallSign( string apt_id, int hours, int mins )
{
    atis_transmission_type tran;

    if(atislog.find(apt_id) == atislog.end()) {
	// This station has not transmitted yet - return a random identifier
	// and add the transmission to the log
	tran.hours = hours;
	tran.mins = mins;
	sg_srandom_time();
	tran.callsign = int(sg_random() * 25) + 1;	// This *should* give a random int between 1 and 26
	//atislog[apt_id].push_back(tran);
	atislog[apt_id] = tran;
    } else {
	// This station has transmitted - calculate the appropriate identifier
	// and add the transmission to the log if it has changed
	tran = atislog[apt_id];
	// This next bit assumes that no-one comes back to the same ATIS station
	// after running FlightGear for more than 24 hours !!
	if((tran.hours == hours) && (tran.mins == mins)) {
	    return(tran.callsign);
	} else {
	    if(tran.hours == hours) {
		// The minutes must have changed
		tran.mins = mins;
		tran.callsign++;
	    } else {
		if(hours < tran.hours) {
		    hours += 24;
		}
		tran.callsign += (hours - tran.hours);
		if(mins != 0) {
		    // Assume transmissions were made on every hour
		    tran.callsign++;
		}
		tran.hours = hours;
		tran.mins = mins;
	    }
	    // Wrap if we've exceeded Zulu
	    if(tran.callsign > 26) {
		tran.callsign -= 26;
	    }
	    // And write the new transmission to the log
	    atislog[apt_id] = tran;
	}
    }
    return(tran.callsign);
}
