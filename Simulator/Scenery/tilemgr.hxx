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


