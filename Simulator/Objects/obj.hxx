// obj.hxx -- routines to handle WaveFront .obj-ish format files.
//
// Written by Curtis Olson, started October 1997.
//
// Copyright (C) 1997  Curtis L. Olson  - curt@infoplane.com
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


#ifndef _OBJ_HXX
#define _OBJ_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <GL/glut.h>

#include <string>

#include <Scenery/tile.hxx>


// Load a .obj file and build the GL fragment list
int fgObjLoad(const string& path, fgTILE *tile);


#endif // _OBJ_HXX


// $Log$
// Revision 1.1  1999/04/05 21:32:49  curt
// Initial revision
//
// Revision 1.3  1998/10/16 00:54:41  curt
// Converted to Point3D class.
//
// Revision 1.2  1998/09/01 19:03:10  curt
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
// Revision 1.1  1998/08/25 16:51:26  curt
// Moved from ../Scenery
//
// Revision 1.4  1998/05/24 02:49:10  curt
// Implimented fragment level view frustum culling.
//
// Revision 1.3  1998/05/23 14:09:21  curt
// Added tile.cxx and tile.hxx.
// Working on rewriting the tile management system so a tile is just a list
// fragments, and the fragment record contains the display list for that
// fragment. 
//
// Revision 1.2  1998/05/02 01:52:15  curt
// Playing around with texture coordinates.
//
// Revision 1.1  1998/04/30 12:35:29  curt
// Added a command line rendering option specify smooth/flat shading.
//
// Revision 1.11  1998/04/25 22:06:31  curt
// Edited cvs log messages in source files ... bad bad bad!
//
// Revision 1.10  1998/04/24 00:51:07  curt
// Wrapped "#include <config.h>" in "#ifdef HAVE_CONFIG_H"
// Tweaked the scenery file extentions to be "file.obj" (uncompressed)
// or "file.obz" (compressed.)
//
// Revision 1.9  1998/04/22 13:22:45  curt
// C++ - ifing the code a bit.
//
// Revision 1.8  1998/04/21 17:02:43  curt
// Prepairing for C++ integration.
//
// Revision 1.7  1998/04/03 22:11:37  curt
// Converting to Gnu autoconf system.
//
// Revision 1.6  1998/01/31 00:43:25  curt
// Added MetroWorks patches from Carmen Volpe.
//
// Revision 1.5  1998/01/27 00:48:03  curt
// Incorporated Paul Bleisch's <pbleisch@acm.org> new debug message
// system and commandline/config file processing code.
//
// Revision 1.4  1998/01/22 02:59:41  curt
// Changed #ifdef FILE_H to #ifdef _FILE_H
//
// Revision 1.3  1998/01/19 19:27:17  curt
// Merged in make system changes from Bob Kuehne <rpk@sgi.com>
// This should simplify things tremendously.
//
// Revision 1.2  1998/01/13 00:23:10  curt
// Initial changes to support loading and management of scenery tiles.  Note,
// there's still a fair amount of work left to be done.
//
// Revision 1.1  1997/10/28 21:14:55  curt
// Initial revision.

