// tilemgr.hxx -- routines to handle dynamic management of scenery tiles
//
// Written by Curtis Olson, started January 1998.
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


#ifndef _TILEMGR_HXX
#define _TILEMGR_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#include <Bucket/newbucket.hxx>


// Initialize the Tile Manager subsystem
int fgTileMgrInit( void );


// given the current lon/lat, fill in the array of local chunks.  If
// the chunk isn't already in the cache, then read it from disk.
int fgTileMgrUpdate( void );


// Determine scenery altitude.  Normally this just happens when we
// render the scene, but we'd also like to be able to do this
// explicitely.  lat & lon are in radians.  abs_view_pos in meters.
// Returns result in meters.
double fgTileMgrCurElevNEW( const FGBucket& p );
double fgTileMgrCurElev( double lon, double lat, const Point3D& abs_view_pos );


// Render the local tiles --- hack, hack, hack
void fgTileMgrRender( void );


#endif // _TILEMGR_HXX


// $Log$
// Revision 1.1  1999/04/05 21:32:49  curt
// Initial revision
//
// Revision 1.8  1999/03/25 19:03:29  curt
// Converted to use new bucket routines.
//
// Revision 1.7  1999/01/27 04:49:49  curt
// Fixes so that the sim can start out at an airport below sea level.
//
// Revision 1.6  1998/12/03 01:18:19  curt
// Converted fgFLIGHT to a class.
// Tweaks for Sun Portability.
// Tweaked current terrain elevation code as per NHV.
//
// Revision 1.5  1998/10/16 00:55:52  curt
// Converted to Point3D class.
//
// Revision 1.4  1998/09/09 20:58:11  curt
// Tweaks to loop constructs with STL usage.
//
// Revision 1.3  1998/08/22  14:49:59  curt
// Attempting to iron out seg faults and crashes.
// Did some shuffling to fix a initialization order problem between view
// position, scenery elevation.
//
// Revision 1.2  1998/05/20 20:53:56  curt
// Moved global ref point and radius (bounding sphere info, and offset) to
// data file rather than calculating it on the fly.
// Fixed polygon winding problem in scenery generation stage rather than
// compensating for it on the fly.
// Made a fgTILECACHE class.
//
// Revision 1.1  1998/04/22 13:22:49  curt
// C++ - ifing the code a bit.
//
// Revision 1.6  1998/04/21 17:02:45  curt
// Prepairing for C++ integration.
//
// Revision 1.5  1998/02/12 21:59:53  curt
// Incorporated code changes contributed by Charlie Hotchkiss
// <chotchkiss@namg.us.anritsu.com>
//
// Revision 1.4  1998/01/22 02:59:42  curt
// Changed #ifdef FILE_H to #ifdef _FILE_H
//
// Revision 1.3  1998/01/19 18:40:38  curt
// Tons of little changes to clean up the code and to remove fatal errors
// when building with the c++ compiler.
//
// Revision 1.2  1998/01/13 00:23:11  curt
// Initial changes to support loading and management of scenery tiles.  Note,
// there's still a fair amount of work left to be done.
//
// Revision 1.1  1998/01/07 23:50:51  curt
// "area" renamed to "tile"
