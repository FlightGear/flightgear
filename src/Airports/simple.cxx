//
// simple.cxx -- a really simplistic class to manage airport ID,
//                 lat, lon of the center of one of it's runways, and 
//                 elevation in feet.
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

#include "simple.hxx"

SG_USING_NAMESPACE(std);

#ifndef _MSC_VER
#   define NDEBUG			// she don't work without it.
#endif
#include <mk4.h>
#include <mk4str.h>
#ifndef _MSC_VER
#  undef NDEBUG
#endif

#ifdef SG_HAVE_STD_INCLUDES
#  include <istream>
#elif defined( __BORLANDC__ ) || defined (__APPLE__)
#  include <iostream>
#else
#  include <istream.h>
#endif
SG_USING_STD(istream);


inline istream&
operator >> ( istream& in, FGAirport& a )
{
    return in >> a.id >> a.latitude >> a.longitude >> a.elevation;
}


FGAirports::FGAirports( const string& file ) {
    // open the specified database readonly
    storage = new c4_Storage( file.c_str(), false );

    if ( !storage->Strategy().IsValid() ) {
	SG_LOG( SG_GENERAL, SG_ALERT, "Cannot open file: " << file );
	exit(-1);
    }

    vAirport = new c4_View;
    *vAirport = 
	storage->GetAs("airport[ID:S,Longitude:F,Latitude:F,Elevation:F]");
}


// search for the specified id
bool
FGAirports::search( const string& id, FGAirport* a ) const
{
    c4_StringProp pID ("ID");
    c4_FloatProp pLon ("Longitude");
    c4_FloatProp pLat ("Latitude");
    c4_FloatProp pElev ("Elevation");

    int idx = vAirport->Find(pID[id.c_str()]);
    cout << "idx = " << idx << endl;

    if ( idx == -1 ) {
	return false;
    }

    c4_RowRef r  = vAirport->GetAt(idx);
    a->id        = (const char *) pID(r); /// NHV fix wrong case crash
    a->longitude = (double) pLon(r);
    a->latitude  =  (double) pLat(r);
    a->elevation = (double) pElev(r);

    return true;
}


FGAirport
FGAirports::search( const string& id ) const
{
    FGAirport a;
    search( id, &a );
    return a;
}


// Destructor
FGAirports::~FGAirports( void ) {
    delete storage;
}


// Constructor
FGAirportsUtil::FGAirportsUtil() {
}


// load the data
int FGAirportsUtil::load( const string& file ) {
    FGAirport a;

    airports.erase( airports.begin(), airports.end() );

    sg_gzifstream in( file );
    if ( !in.is_open() ) {
	SG_LOG( SG_GENERAL, SG_ALERT, "Cannot open file: " << file );
	exit(-1);
    } else {
	SG_LOG( SG_GENERAL, SG_ALERT, "opened: " << file );
    }

    // skip first line of file
    char tmp[2048];
    in.getline( tmp, 2048 );

    // read in each line of the file

#ifdef __MWERKS__

    in >> ::skipws;
    char c = 0;
    while ( in.get(c) && c != '\0' ) {
	if ( c == 'A' ) {
	    in >> a;
            SG_LOG( SG_GENERAL, SG_INFO, a.id );
	    in >> skipeol;
	    airports.insert(a);
	} else if ( c == 'R' ) {
	    in >> skipeol;
	} else {
	    in >> skipeol;
	}
	in >> ::skipws;
    }

#else

    in >> ::skipws;
    string token;
    while ( ! in.eof() ) {
 	in >> token;
	if ( token == "A" ) {
	    in >> a;
            SG_LOG( SG_GENERAL, SG_INFO, "in <- " << a.id );
	    in >> skipeol;
	    airports.insert(a);
	} else if ( token == "R" ) {
	    in >> skipeol;
	} else {
	    in >> skipeol;
	}
	in >> ::skipws;
    }

#endif

    return 1;
}


// save the data in gdbm format
bool FGAirportsUtil::dump_mk4( const string& file ) {

    // open database for writing
    c4_Storage storage( file.c_str(), true );

    // need to do something about error handling here!

    // define the properties
    c4_StringProp pID ("ID");
    c4_FloatProp pLon ("Longitude");
    c4_FloatProp pLat ("Latitude");
    c4_FloatProp pElev ("Elevation");

    // Start with an empty view of the proper structure.
    c4_View vAirport =
	storage.GetAs("airport[ID:S,Longitude:F,Latitude:F,Elevation:F]");

    c4_Row row;

    const_iterator current = airports.begin();
    const_iterator end = airports.end();
    while ( current != end ) {
	// add each airport record
	cout << "out -> " << current->id << endl;
	pID (row) = current->id.c_str();
	pLon (row) = current->longitude;
	pLat (row) = current->latitude;
	pElev (row) = current->elevation;
	vAirport.Add(row);

	++current;
    }

    // commit our changes
    storage.Commit();

    return true;
}


// search for the specified id
bool
FGAirportsUtil::search( const string& id, FGAirport* a ) const
{
    const_iterator it = airports.find( FGAirport(id) );
    if ( it != airports.end() )
    {
	*a = *it;
	return true;
    }
    else
    {
	return false;
    }
}


FGAirport
FGAirportsUtil::search( const string& id ) const
{
    FGAirport a;
    this->search( id, &a );
    return a;
}


// Destructor
FGAirportsUtil::~FGAirportsUtil( void ) {
}


