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

#ifndef _FGSTREAM_HXX
#define _FGSTREAM_HXX

#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <string>

#include "Include/fg_stl_config.h"
_FG_USING_NAMESPACE(std);

#include "zfstream.hxx"

//-----------------------------------------------------------------------------
//
// Envelope class for gzifstream.
//
class fg_gzifstream
{
public:
    //
    fg_gzifstream();

    // Attempt to open a file with and without ".gz" extension.
    fg_gzifstream( const string& name,
		   int io_mode = ios::in|ios::binary );

    // 
    fg_gzifstream( int fd, int io_mode = ios::in|ios::binary );

    // Attempt to open a file with and without ".gz" extension.
    void open( const string& name,
	       int io_mode = ios::in|ios::binary );

    // Return the underlying stream.
    istream& stream() { return gzstream; }

    // Check stream state.
    bool operator ! () const { return !gzstream; }

    // Check for end of file.
    bool eof() const { return gzstream.eof(); }

    // Remove whitespace from stream.
    // Whitespace is as defined by isspace().
    istream& eat_whitespace();

    // Removes comments and whitespace from stream.
    // A comment is any line starting with '#'.
    istream& eat_comments();

    // Read one character from stream.
    istream& get( char& c ) { return gzstream.get(c); }

    // Put a character back into the input buffer.
    istream& putback( char c ) { return gzstream.putback(c); }

private:
    // The underlying compressed data stream.
    gzifstream gzstream;

private:
    // Not defined!
    fg_gzifstream( const fg_gzifstream& );    
};

// istream manipulator that skips to end of line.
istream& skipeol( istream& in );

// istream manipulator that skips over white space.
istream& skipws( istream& in );

// istream manipulator that skips comments and white space.
// A comment starts with '#'.
istream& skipcomment( istream& in );

#endif /* _FGSTREAM_HXX */

// $Log$
// Revision 1.3  1998/10/13 00:10:06  curt
// More portability changes to help with windoze compilation problems.
//
// Revision 1.2  1998/09/24 15:22:18  curt
// Additional enhancements.
//
// Revision 1.1  1998/09/01 19:06:29  curt
// Initial revision.
//
