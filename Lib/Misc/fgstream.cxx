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

#include <ctype.h> // isspace()
#include <Misc/fgstream.hxx>

fg_gzifstream::fg_gzifstream()
    : istream(&gzbuf)
{
}

//-----------------------------------------------------------------------------
//
// Open a possibly gzipped file for reading.
//
fg_gzifstream::fg_gzifstream( const string& name, ios_openmode io_mode )
    : istream(&gzbuf)
{
    this->open( name, io_mode );
}

//-----------------------------------------------------------------------------
//
// Attach a stream to an already opened file descriptor.
//
fg_gzifstream::fg_gzifstream( int fd, ios_openmode io_mode )
    : istream(&gzbuf)
{
    gzbuf.attach( fd, io_mode );
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
fg_gzifstream::open( const string& name, ios_openmode io_mode )
{
    gzbuf.open( name.c_str(), io_mode );
    if ( ! gzbuf.is_open() )
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
	gzbuf.open( s.c_str(), io_mode );
    }
}

void
fg_gzifstream::attach( int fd, ios_openmode io_mode )
{
    gzbuf.attach( fd, io_mode );
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

    #ifdef __MWERKS // -dw- need to strip line ending!
    in >> skipws;
    #endif

    return in;
}

istream&
skipws( istream& in ) {
    char c;
    while ( in.get(c) ) {
	#ifdef __MWERKS__ // -dw- for unix file compatibility
	if ( ! (isspace( c ) ) || c != '\0' || c!='\n' || c != '\r' ) {
	#else
	if ( ! isspace( c ) ) {
	#endif
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

