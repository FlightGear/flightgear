//
// airports.cxx -- a really simplistic class to manage airport ID,
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
// (Log is kept at end of this file)


#include <string>

#include <Debug/logstream.hxx>
#include <Main/options.hxx>
#include <Misc/fgstream.hxx>

#include "simple.hxx"

#include "Include/fg_stl_config.h"
#include STL_FUNCTIONAL
#include STL_ALGORITHM


fgAIRPORTS::fgAIRPORTS() {
}


// load the data
int fgAIRPORTS::load( const string& file ) {
    fgAIRPORT a;

    // build the path name to the airport file
    string path = current_options.get_fg_root() + "/Airports/" + file;

    airports.erase( airports.begin(), airports.end() );

    fg_gzifstream in( path );
    if ( !in ) {
	FG_LOG( FG_GENERAL, FG_ALERT, "Cannot open file: " << path );
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
    in >> skipcomment;
    while ( ! in.eof() )
    {
	in >> a;
	airports.insert(a);
	in >> skipcomment;
    }

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


// $Log$
// Revision 1.9  1998/11/06 21:17:34  curt
// Converted to new logstream debugging facility.  This allows release
// builds with no messages at all (and no performance impact) by using
// the -DFG_NDEBUG flag.
//
// Revision 1.8  1998/11/06 14:47:01  curt
// Changes to track Bernie's updates to fgstream.
//
// Revision 1.7  1998/09/08 21:38:41  curt
// Changes by Bernie Bright.
//
// Revision 1.6  1998/09/03 21:25:02  curt
// tweaked in data file comment handling.
//
// Revision 1.5  1998/09/02 14:35:38  curt
// Rewrote simple airport loader so it can deal with comments and blank lines.
//
// Revision 1.4  1998/09/01 19:02:53  curt
// Changes contributed by Bernie Bright <bbright@c031.aone.net.au>
//  - The new classes in libmisc.tgz define a stream interface into zlib.
//    I've put these in a new directory, Lib/Misc.  Feel free to rename it
//    to something more appropriate.  However you'll have to change the
//    include directives in all the other files.  Additionally you'll have
//    add the library to Lib/Makefile.am and Simulator/Main/Makefile.am.
//
//    The StopWatch class in Lib/Misc requires a HAVE_GETRUSAGE autoconf
//    test so I've included the required changes in config.tgz.
//
//    There are a fair few changes to Simulator/Objects as I've moved
//    things around.  Loading tiles is quicker but thats not where the delay
//    is.  Tile loading takes a few tenths of a second per file on a P200
//    but it seems to be the post-processing that leads to a noticeable
//    blip in framerate.  I suppose its time to start profiling to see where
//    the delays are.
//
//    I've included a brief description of each archives contents.
//
// Lib/Misc/
//   zfstream.cxx
//   zfstream.hxx
//     C++ stream interface into zlib.
//     Taken from zlib-1.1.3/contrib/iostream/.
//     Minor mods for STL compatibility.
//     There's no copyright associated with these so I assume they're
//     covered by zlib's.
//
//   fgstream.cxx
//   fgstream.hxx
//     FlightGear input stream using gz_ifstream.  Tries to open the
//     given filename.  If that fails then filename is examined and a
//     ".gz" suffix is removed or appended and that file is opened.
//
//   stopwatch.hxx
//     A simple timer for benchmarking.  Not used in production code.
//     Taken from the Blitz++ project.  Covered by GPL.
//
//   strutils.cxx
//   strutils.hxx
//     Some simple string manipulation routines.
//
// Simulator/Airports/
//   Load airports database using fgstream.
//   Changed fgAIRPORTS to use set<> instead of map<>.
//   Added bool fgAIRPORTS::search() as a neater way doing the lookup.
//   Returns true if found.
//
// Simulator/Astro/
//   Modified fgStarsInit() to load stars database using fgstream.
//
// Simulator/Objects/
//   Modified fgObjLoad() to use fgstream.
//   Modified fgMATERIAL_MGR::load_lib() to use fgstream.
//   Many changes to fgMATERIAL.
//   Some changes to fgFRAGMENT but I forget what!
//
// Revision 1.3  1998/08/27 17:01:55  curt
// Contributions from Bernie Bright <bbright@c031.aone.net.au>
// - use strings for fg_root and airport_id and added methods to return
//   them as strings,
// - inlined all access methods,
// - made the parsing functions private methods,
// - deleted some unused functions.
// - propogated some of these changes out a bit further.
//
// Revision 1.2  1998/08/25 20:53:24  curt
// Shuffled $FG_ROOT file layout.
//
// Revision 1.1  1998/08/25 17:19:13  curt
// Moved from ../Main/
//
// Revision 1.8  1998/07/13 21:01:37  curt
// Wrote access functions for current fgOPTIONS.
//
// Revision 1.7  1998/06/03 22:01:07  curt
// Tweaking sound library usage.
//
// Revision 1.6  1998/06/03 00:47:13  curt
// Updated to compile in audio support if OSS available.
// Updated for new version of Steve's audio library.
// STL includes don't use .h
// Small view optimizations.
//
// Revision 1.5  1998/05/29 20:37:22  curt
// Tweaked material properties & lighting a bit in GLUTmain.cxx.
// Read airport list into a "map" STL for dynamic list sizing and fast tree
// based lookups.
//
// Revision 1.4  1998/05/13 18:26:25  curt
// Root path info moved to fgOPTIONS.
//
// Revision 1.3  1998/05/06 03:16:24  curt
// Added an averaged global frame rate counter.
// Added an option to control tile radius.
//
// Revision 1.2  1998/04/28 21:42:50  curt
// Wrapped zlib calls up so we can conditionally comment out zlib support.
//
// Revision 1.1  1998/04/25 15:11:11  curt
// Added an command line option to set starting position based on airport ID.
//
//
