// obj.hxx -- routines to handle loading scenery and building the plib
//            scene graph.
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


#ifndef _OBJ_HXX
#define _OBJ_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#include <simgear/compiler.h>

#include STL_STRING

#include <plib/ssg.h>           // plib include

#include <simgear/math/sg_types.hxx>

SG_USING_STD(string);

class SGBucket;
class SGMaterialLib;


// Load a Binary obj file
bool fgBinObjLoad( const string& path, const bool is_base,
                   Point3D *center,
                   double *bounding_radius,
                   SGMaterialLib *matlib,
                   bool use_random_objects,
                   ssgBranch* geometry,
                   ssgBranch* rwy_lights,
                   ssgBranch* taxi_lights,
                   ssgVertexArray *ground_lights );

// Generate an ocean tile
bool fgGenTile( const string& path, SGBucket b,
                Point3D *center, double *bounding_radius,
                SGMaterialLib *matlib, ssgBranch* geometry );


#endif // _OBJ_HXX
