//
// simple.hxx -- a really simplistic class to manage airport ID,
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


#ifndef _AIRPORTS_HXX
#define _AIRPORTS_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#include <Include/compiler.h>
#ifdef FG_HAVE_STD_INCLUDES
#  include <istream>
#else
#  include <istream.h>
#endif

#include STL_STRING
#include <set>

FG_USING_STD(string);
FG_USING_STD(set);
FG_USING_STD(istream);


class fgAIRPORT {
public:
    fgAIRPORT( const string& name = "",
	       double lon = 0.0,
	       double lat = 0.0,
	       double ele = 0.0 )
	: id(name), longitude(lon), latitude(lat), elevation(ele) {}

    bool operator < ( const fgAIRPORT& a ) const {
	return id < a.id;
    }

public:
    string id;
    double longitude;
    double latitude;
    double elevation;
};

inline istream&
operator >> ( istream& in, fgAIRPORT& a )
{
    return in >> a.id >> a.longitude >> a.latitude >> a.elevation;
}

class fgAIRPORTS {
public:
#ifdef FG_NO_DEFAULT_TEMPLATE_ARGS
    typedef set< fgAIRPORT, less< fgAIRPORT > > container;
#else
    typedef set< fgAIRPORT > container;
#endif
    typedef container::iterator iterator;
    typedef container::const_iterator const_iterator;

private:
    container airports;

public:

    // Constructor
    fgAIRPORTS();

    // Destructor
    ~fgAIRPORTS();

    // load the data
    int load( const string& file );

    // search for the specified id.
    // Returns true if successful, otherwise returns false.
    // On success, airport data is returned thru "airport" pointer.
    // "airport" is not changed if "id" is not found.
    bool search( const string& id, fgAIRPORT* airport ) const;
    fgAIRPORT search( const string& id ) const;
};


#endif /* _AIRPORTS_HXX */


// $Log$
// Revision 1.1  1999/04/05 21:32:47  curt
// Initial revision
//
// Revision 1.8  1999/03/15 17:58:57  curt
// MSVC++ portability tweaks contributed by Bernie Bright.
//   Added using std::istream declaration.
//
// Revision 1.7  1999/03/02 01:02:33  curt
// Tweaks for building with native SGI compilers.
//
// Revision 1.6  1999/02/26 22:08:36  curt
// Added initial support for native SGI compilers.
//
// Revision 1.5  1998/11/02 18:25:34  curt
// Check for __CYGWIN__ (b20) as well as __CYGWIN32__ (pre b20 compilers)
// Other misc. tweaks.
//
// Revision 1.4  1998/09/08 21:38:43  curt
// Changes by Bernie Bright.
//
// Revision 1.3  1998/09/01 19:02:54  curt
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
// Revision 1.2  1998/08/27 17:01:56  curt
// Contributions from Bernie Bright <bbright@c031.aone.net.au>
// - use strings for fg_root and airport_id and added methods to return
//   them as strings,
// - inlined all access methods,
// - made the parsing functions private methods,
// - deleted some unused functions.
// - propogated some of these changes out a bit further.
//
// Revision 1.1  1998/08/25 17:19:14  curt
// Moved from ../Main/
//
// Revision 1.7  1998/07/24 21:39:09  curt
// Debugging output tweaks.
// Cast glGetString to (char *) to avoid compiler errors.
// Optimizations to fgGluLookAt() by Norman Vine.
//
// Revision 1.6  1998/07/06 21:34:19  curt
// Added an enable/disable splash screen option.
// Added an enable/disable intro music option.
// Added an enable/disable instrument panel option.
// Added an enable/disable mouse pointer option.
// Added using namespace std for compilers that support this.
//
// Revision 1.5  1998/06/17 21:35:11  curt
// Refined conditional audio support compilation.
// Moved texture parameter setup calls to ../Scenery/materials.cxx
// #include <string.h> before various STL includes.
// Make HUD default state be enabled.
//
// Revision 1.4  1998/06/03 00:47:14  curt
// Updated to compile in audio support if OSS available.
// Updated for new version of Steve's audio library.
// STL includes don't use .h
// Small view optimizations.
//
// Revision 1.3  1998/06/01 17:54:42  curt
// Added Linux audio support.
// avoid glClear( COLOR_BUFFER_BIT ) when not using it to set the sky color.
// map stl tweaks.
//
// Revision 1.2  1998/05/29 20:37:22  curt
// Tweaked material properties & lighting a bit in GLUTmain.cxx.
// Read airport list into a "map" STL for dynamic list sizing and fast tree
// based lookups.
//
// Revision 1.1  1998/04/25 15:11:11  curt
// Added an command line option to set starting position based on airport ID.
//
//
