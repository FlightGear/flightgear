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

#ifndef _FGSTREAM_HXX
#define _FGSTREAM_HXX

#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   

#ifdef HAVE_CONFIG_H
#  include "Include/config.h"
#endif

#include <Include/compiler.h>

#if defined( FG_HAVE_STD_INCLUDES )
#  include <istream>
#elif defined ( FG_HAVE_NATIVE_SGI_COMPILERS )
#  include <CC/stream.h>
#elif defined ( __BORLANDC__ )
#  include <iostream>
#else
#  include <istream.h>
#endif

#include STL_STRING

#include "zfstream.hxx"

FG_USING_STD(string);

#ifndef FG_HAVE_NATIVE_SGI_COMPILERS
FG_USING_STD(istream);
#endif


//-----------------------------------------------------------------------------
//
// Envelope class for gzifstream.
//
class fg_gzifstream : private gzifstream_base, public istream
{
public:
    //
    fg_gzifstream();

    // Attempt to open a file with and without ".gz" extension.
    fg_gzifstream( const string& name,
		   ios_openmode io_mode = ios_in | ios_binary );

    // 
    fg_gzifstream( int fd, ios_openmode io_mode = ios_in|ios_binary );

    // Attempt to open a file with and without ".gz" extension.
    void open( const string& name,
	       ios_openmode io_mode = ios_in|ios_binary );

    void attach( int fd, ios_openmode io_mode = ios_in|ios_binary );

    void close() { gzbuf.close(); }

    bool is_open() { return gzbuf.is_open(); }

private:
    // Not defined!
    fg_gzifstream( const fg_gzifstream& );    
    void operator= ( const fg_gzifstream& );    
};

// istream manipulator that skips to end of line.
istream& skipeol( istream& in );

// istream manipulator that skips over white space.
istream& skipws( istream& in );

// istream manipulator that skips comments and white space.
// A comment starts with '#'.
istream& skipcomment( istream& in );


#endif /* _FGSTREAM_HXX */

