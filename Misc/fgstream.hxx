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
#  include "Include/config.h"
#endif

#include <Include/compiler.h>

#if defined( FG_HAVE_STD_INCLUDES )
#  include <istream>
#elif defined ( FG_HAVE_NATIVE_SGI_COMPILERS )
#  include <CC/stream.h>
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

// $Log$
// Revision 1.8  1999/03/02 01:01:55  curt
// Tweaks for compiling with native SGI compilers.
//
// Revision 1.7  1999/02/26 22:08:08  curt
// Added initial support for native SGI compilers.
//
// Revision 1.6  1999/01/19 20:41:46  curt
// Portability updates contributed by Bernie Bright.
//
// Revision 1.5  1998/11/06 14:05:13  curt
// More portability improvements by Bernie Bright.
//
// Revision 1.4  1998/10/16 00:50:56  curt
// Remove leading _ from a couple defines.
//
// Revision 1.3  1998/10/13 00:10:06  curt
// More portability changes to help with windoze compilation problems.
//
// Revision 1.2  1998/09/24 15:22:18  curt
// Additional enhancements.
//
// Revision 1.1  1998/09/01 19:06:29  curt
// Initial revision.
//
