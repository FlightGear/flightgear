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

#include <simgear/compiler.h>

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sgstream.hxx>

#include STL_STRING
#include STL_FUNCTIONAL
#include STL_ALGORITHM
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

    while ( in ) {
        FGAirport a;
        in >> a;
        airports[a.id] = a;
    }
}


// search for the specified id
FGAirport FGAirportList::search( const string& id) {
    return airports[id];
}


// Destructor
FGAirportList::~FGAirportList( void ) {
}
