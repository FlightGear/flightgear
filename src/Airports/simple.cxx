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


#include <simgear/compiler.h>

#include <simgear/logstream.hxx>
#include <simgear/fgpath.hxx>
#include <simgear/fgstream.hxx>

#include <Main/options.hxx>

#include STL_STRING
#include STL_FUNCTIONAL
#include STL_ALGORITHM

#include "simple.hxx"


fgAIRPORTS::fgAIRPORTS() {
}


// load the data
int fgAIRPORTS::load( const string& file ) {
    fgAIRPORT a;

    // build the path name to the airport file
    FGPath path( current_options.get_fg_root() );
    path.append( "Airports" );
    path.append( file );

    airports.erase( airports.begin(), airports.end() );

    fg_gzifstream in( path.str() );
    if ( !in.is_open() ) {
	FG_LOG( FG_GENERAL, FG_ALERT, "Cannot open file: " << path.str() );
	exit(-1);
    }

    /*
    // We can use the STL copy algorithm because the input
    // file doesn't contain and comments or blank lines.
    copy( istream_iterator<fgAIRPORT,ptrdiff_t>(in.stream()),
	  istream_iterator<fgAIRPORT,ptrdiff_t>(),
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


// search for the specified id
bool
fgAIRPORTS::search( const string& id, fgAIRPORT* a ) const
{
    const_iterator it = airports.find( fgAIRPORT(id) );
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


fgAIRPORT
fgAIRPORTS::search( const string& id ) const
{
    fgAIRPORT a;
    this->search( id, &a );
    return a;
}


// Destructor
fgAIRPORTS::~fgAIRPORTS( void ) {
}


