//  A C++ I/O streams interface to the zlib gz* functions
//
// Written by Bernie Bright, 1998
// Based on zlib/contrib/iostream/ by Kevin Ruland <kevin@rodin.wustl.edu>
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

#ifndef _zfstream_hxx
#define _zfstream_hxx

#include "zlib/zlib.h"
#include "Include/compiler.h"

#ifdef FG_HAVE_STD_INCLUDES

#  include <streambuf>
#  include <istream>

#  define ios_openmode ios_base::openmode
#  define ios_in       ios_base::in
#  define ios_out      ios_base::out
#  define ios_app      ios_base::app
#  define ios_binary   ios_base::binary

#  define ios_seekdir  ios_base::seekdir

#  define ios_badbit   ios_base::badbit
#  define ios_failbit  ios_base::failbit

FG_USING_STD(streambuf);
FG_USING_STD(ios_base);
FG_USING_STD(streampos);
FG_USING_STD(streamoff);

#else

#  ifdef FG_HAVE_STREAMBUF
#    include <streambuf.h>
#    include <istream.h>
#  else
#    include <iostream.h>
#  endif

//#  define ios_openmode ios::open_mode
#  define ios_openmode int
#  define ios_in       ios::in
#  define ios_out      ios::out
#  define ios_app      ios::app

#if defined(__GNUC__) && __GNUC_MINOR__ < 8
#  define ios_binary   ios::bin
#elif defined( FG_HAVE_NATIVE_SGI_COMPILERS )
#  define ios_binary   0
#else
#  define ios_binary   ios::binary
#endif

#  define ios_seekdir  ios::seek_dir

#  define ios_badbit   ios::badbit
#  define ios_failbit  ios::failbit

#  include "Include/fg_traits.hxx"

#endif // FG_HAVE_STD_INCLUDES

//-----------------------------------------------------------------------------
//
//
//
class gzfilebuf : public streambuf
{
public:

#ifndef FG_HAVE_STD_INCLUDES
    typedef char_traits<char>           traits_type;
    typedef char_traits<char>::int_type int_type;
    typedef char_traits<char>::pos_type pos_type;
    typedef char_traits<char>::off_type off_type;
#endif

    gzfilebuf();
    virtual ~gzfilebuf();

    gzfilebuf* open( const char* name, ios_openmode io_mode );
    gzfilebuf* attach( int file_descriptor, ios_openmode io_mode );
    gzfilebuf* close();

//     int setcompressionlevel( int comp_level );
//     int setcompressionstrategy( int comp_strategy );
    bool is_open() const { return (file != NULL); }
    virtual streampos seekoff( streamoff off, ios_seekdir way, int which );
    virtual int sync();

protected:

    virtual int_type underflow();
    virtual int_type overflow( int_type c = traits_type::eof() );

private:

    int_type flushbuf();
    int fillbuf();

    // Convert io_mode to "rwab" string.
    void cvt_iomode( char* mode_str, ios_openmode io_mode );

private:

    gzFile file;
    ios_openmode mode;
    bool own_file_descriptor;

    // Get (input) buffer.
    int ibuf_size;
    char* ibuffer;

    enum { page_size = 4096 };

private:
    // Not defined
    gzfilebuf( const gzfilebuf& );
    void operator= ( const gzfilebuf& );
};

//-----------------------------------------------------------------------------
//
// 
//
struct gzifstream_base
{
    gzifstream_base() {}

    gzfilebuf gzbuf;
};

#endif // _zfstream_hxx

// $Log$
// Revision 1.9  1999/03/08 22:00:12  curt
// Tweak for native SGI compilers.
//
// Revision 1.8  1999/02/26 22:08:10  curt
// Added initial support for native SGI compilers.
//
// Revision 1.7  1999/01/19 20:41:49  curt
// Portability updates contributed by Bernie Bright.
//
// Revision 1.6  1998/12/07 21:10:26  curt
// Portability improvements.
//
// Revision 1.5  1998/11/06 21:17:29  curt
// Converted to new logstream debugging facility.  This allows release
// builds with no messages at all (and no performance impact) by using
// the -DFG_NDEBUG flag.
//
// Revision 1.4  1998/11/06 14:05:16  curt
// More portability improvements by Bernie Bright.
//

