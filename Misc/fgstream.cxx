// zlib input file stream wrapper.
//
// Written by Bernie Bright, 1998
//
// Copyright (C) 1998  Bernie Bright - bbright@c031.aone.net.au
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
// (Log is kept at end of this file)

#include <ctype.h> // isspace()
#include "fgstream.hxx"

//-----------------------------------------------------------------------------
//
// Open a possibly gzipped file for reading.
//
fg_gzifstream::fg_gzifstream( const string& name, int io_mode )
{
    open( name, io_mode );
}

//-----------------------------------------------------------------------------
//
// Attach a stream to an already opened file descriptor.
//
fg_gzifstream::fg_gzifstream( int fd, int io_mode )
    : gzstream( fd, io_mode )
{
}

//-----------------------------------------------------------------------------
//
// Open a possibly gzipped file for reading.
// If the initial open fails and the filename has a ".gz" extension then
// remove the extension and try again.
// If the initial open fails and the filename doesn't have a ".gz" extension
// then append ".gz" and try again.
//
void
fg_gzifstream::open( const string& name, int io_mode )
{
    gzstream.open( name.c_str(), io_mode );
    if ( gzstream.fail() )
    {
	string s = name;
	if ( s.substr( s.length() - 3, 3 ) == ".gz" )
	{
	    // remove ".gz" suffix
	    s.replace( s.length() - 3, 3, "" );
// 	    s.erase( s.length() - 3, 3 );
	}
	else
	{
	    // Append ".gz" suffix
	    s += ".gz";
	}

	// Try again.
	gzstream.open( s.c_str(), io_mode );
    }
}

//-----------------------------------------------------------------------------
//
// Remove whitespace characters from the stream.
//
istream&
fg_gzifstream::eat_whitespace()
{
    char c;
    while ( gzstream.get(c) )
    {
	if ( ! isspace( c ) )
	{
	    // put pack the non-space character
	    gzstream.putback(c);
	    break;
	}
    }
    return gzstream;
}

//-----------------------------------------------------------------------------
// 
// Remove whitspace chatacters and comment lines from a stream.
//
istream&
fg_gzifstream::eat_comments()
{
    for (;;)
    {
	// skip whitespace
	eat_whitespace();

	char c;
	gzstream.get( c );
	if ( c != '#' )
	{
	    // not a comment
	    gzstream.putback(c);
	    break;
	}

	// skip to end of line.
	while ( gzstream.get(c) && (c != '\n' && c != '\r') )
	    ;
    }
    return gzstream;
}

//
// Manipulators
//

istream&
skipeol( istream& in )
{
    char c = 0;
    // skip to end of line.
    while ( in.get(c) && (c != '\n' && c != '\r') )
	;

    // \r\n ?
    return in;
}

istream&
skipws( istream& in )
{
    char c;
    while ( in.get(c) )
    {
	if ( ! isspace( c ) )
	{
	    // put pack the non-space character
	    in.putback(c);
	    break;
	}
    }
    return in;
}

istream&
skipcomment( istream& in )
{
    while ( in )
    {
	// skip whitespace
	in >> skipws;

	char c;
	if ( in.get( c ) && c != '#' )
	{
	    // not a comment
	    in.putback(c);
	    break;
	}
	in >> skipeol;
    }
    return in;
}

// $Log$
// Revision 1.2  1998/09/24 15:22:17  curt
// Additional enhancements.
//
// Revision 1.1  1998/09/01 19:06:29  curt
// Initial revision.
//
