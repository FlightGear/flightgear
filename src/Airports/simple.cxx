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

#include <sys/types.h>		// for gdbm open flags
#include <sys/stat.h>		// for gdbm open flags

#ifdef HAVE_GDBM
#  include <gdbm.h>
#else
#  include <simgear/gdbm/gdbm.h>
#endif

#include <simgear/compiler.h>

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/fgstream.hxx>

#include <Main/options.hxx>

#include STL_STRING
#include STL_FUNCTIONAL
#include STL_ALGORITHM

#include "simple.hxx"


FGAirports::FGAirports( const string& file ) {
    dbf = gdbm_open( (char *)file.c_str(), 0, GDBM_READER, 0, NULL );
    if ( dbf == NULL ) {
	cout << "Error opening " << file << endl;
	exit(-1);
    } else {
	cout << "successfully opened " << file << endl;
    }
}


// search for the specified id
bool
FGAirports::search( const string& id, FGAirport* a ) const
{
    FGAirport *tmp;
    datum content;
    datum key;

    key.dptr = (char *)id.c_str();
    key.dsize = id.length();

    content = gdbm_fetch( dbf, key );

    cout << "gdbm_fetch() finished" << endl;

    if ( content.dptr != NULL ) {
	tmp = (FGAirport *)content.dptr;

	// a->id = tmp->id;
	a->longitude = tmp->longitude;
	a->latitude = tmp->latitude;
	a->elevation = tmp->elevation;

	free( content.dptr );

    } else {
	return false;
    }

    return true;
}


FGAirport
FGAirports::search( const string& id ) const
{
    FGAirport a, *tmp;
    datum content;
    datum key;

    key.dptr = (char *)id.c_str();
    key.dsize = id.length();

    content = gdbm_fetch( dbf, key );

    if ( content.dptr != NULL ) {
	tmp = (FGAirport *)content.dptr;
	a = *tmp;
    }

    return a;
}


// Destructor
FGAirports::~FGAirports( void ) {
    gdbm_close( dbf );
}


// Constructor
FGAirportsUtil::FGAirportsUtil() {
}


// load the data
int FGAirportsUtil::load( const string& file ) {
    FGAirport a;

    airports.erase( airports.begin(), airports.end() );

    fg_gzifstream in( file );
    if ( !in.is_open() ) {
	FG_LOG( FG_GENERAL, FG_ALERT, "Cannot open file: " << file );
	exit(-1);
    }

    /*
    // We can use the STL copy algorithm because the input
    // file doesn't contain and comments or blank lines.
    copy( istream_iterator<FGAirport,ptrdiff_t>(in.stream()),
	  istream_iterator<FGAirport,ptrdiff_t>(),
 	  inserter( airports, airports.begin() ) );
    */

    // read in each line of the file

#ifdef __MWERKS__

    in >> skipcomment;
    char c = 0;
    while ( in.get(c) && c != '\0' ) {
	in.putback(c);
	in >> a;
	airports.insert(a);
	in >> skipcomment;
    }

#else

    in >> skipcomment;
    while ( ! in.eof() ) {
	in >> a;
	airports.insert(a);
	in >> skipcomment;
    }

#endif

    return 1;
}


// save the data in gdbm format
bool FGAirportsUtil::dump_gdbm( const string& file ) {

    GDBM_FILE dbf;

#if !defined( MACOS )
    dbf = gdbm_open( (char *)file.c_str(), 0, GDBM_NEWDB | GDBM_FAST, 
		     S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH,
		     NULL );
#else
    dbf = gdbm_open( (char *)file.c_str(), 0, GDBM_NEWDB | GDBM_FAST,
		     NULL, NULL );
#endif

    if ( dbf == NULL ) {
	cout << "Error opening " << file << endl;
	exit(-1);
    } else {
	cout << "successfully opened " << file << endl;
    }

    iterator current = airports.begin();
    const_iterator end = airports.end();
    while ( current != end ) {
	datum key;
	key.dptr = (char *)current->id.c_str();
	key.dsize = current->id.length();

	datum content;
	FGAirport tmp = *current;
	content.dptr = (char *)(& tmp);
	content.dsize = sizeof( *current );

	gdbm_store( dbf, key, content, GDBM_REPLACE );

	++current;
    }

    gdbm_close( dbf );

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


