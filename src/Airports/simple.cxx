//
// simple.cxx -- a really simplistic class to manage airport ID,
//               lat, lon of the center of one of it's runways, and 
//               elevation in feet.
//
// Written by Curtis Olson, started April 1998.
//
// Copyright (C) 1998  Curtis L. Olson  - curt@me.umn.edu
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

#include <math.h>

#include <simgear/compiler.h>

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sgstream.hxx>

#include STL_STRING
#include STL_IOSTREAM

#include "simple.hxx"

SG_USING_NAMESPACE(std);
SG_USING_STD(istream);


inline istream&
operator >> ( istream& in, FGAirport& a )
{
    string junk;
    in >> junk >> a.id >> a.latitude >> a.longitude >> a.elevation
       >> a.code;

    char name[256];             // should never be longer than this, right? :-)
    in.getline( name, 256 );
    a.name = name;

    // a.has_metar = true;         // assume true
    // only airports with four-letter codes can have metar stations
    a.has_metar = (isalpha(a.id[0]) && isalpha(a.id[1]) && isalpha(a.id[2])
        && isalpha(a.id[3]) && !a.id[4]);

    return in;
}


FGAirportList::FGAirportList( const string& file ) {
    SG_LOG( SG_GENERAL, SG_DEBUG, "Reading simple airport list: " << file );

    // open the specified file for reading
    sg_gzifstream in( file );
    if ( !in.is_open() ) {
        SG_LOG( SG_GENERAL, SG_ALERT, "Cannot open file: " << file );
	exit(-1);
    }

    // skip header line
    in >> skipeol;

    FGAirport a;
    int size = 0;
    while ( in ) {
        in >> a;
        airports_by_id[a.id] = a;
        airports_array.push_back( &airports_by_id[a.id] );
        size++;
    }
}


// search for the specified id
FGAirport FGAirportList::search( const string& id) {
    
    return airports_by_id[id];
}


// search for the airport nearest the specified position
FGAirport FGAirportList::search( double lon_deg, double lat_deg,
                                 bool with_metar ) {
    int closest = 0;
    double min_dist = 360.0;
    unsigned int i;
    for ( i = 0; i < airports_array.size(); ++i ) {
        // crude manhatten distance based on lat/lon difference
        double d = fabs(lon_deg - airports_array[i]->longitude)
            + fabs(lat_deg - airports_array[i]->latitude);
        if ( d < min_dist ) {
            if ( !with_metar || (with_metar && airports_array[i]->has_metar) ) {
                closest = i;
                min_dist = d;
            }
        }
    }

    return *airports_array[closest];
}


// Destructor
FGAirportList::~FGAirportList( void ) {
}

int
FGAirportList::size () const
{
    return airports_array.size();
}

const FGAirport *FGAirportList::getAirport( int index ) const
{
    return airports_array[index];
}


/**
 * Mark the specified airport record as not having metar
 */
void FGAirportList::no_metar( const string &id ) {
    airports_by_id[id].has_metar = false;
}
