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


#ifndef _OBJ_HXX
#define _OBJ_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

// #ifdef HAVE_WINDOWS_H
// #  include <windows.h>
// #endif

// #include <GL/glut.h>

#include STL_STRING

#include <plib/ssg.h>		// plib include

#include <simgear/math/sg_types.hxx>

#include <Scenery/tileentry.hxx>

SG_USING_STD(string);


// duplicated from the TerraGear tools
#define FG_MAX_NODES 4000


// Load a binary object file
ssgBranch *fgBinObjLoad(const string& path, FGTileEntry *tile,
			ssgVertexArray *lights, const bool is_base);

// Load an ascii object file
ssgBranch *fgAsciiObjLoad(const string& path, FGTileEntry *tile,
			  ssgVertexArray *lights, const bool is_base);

// Generate an ocean tile
ssgBranch *fgGenTile( const string& path, FGTileEntry *t);


// Create an ssg leaf
ssgLeaf *gen_leaf( const string& path,
		   const GLenum ty, const string& material,
		   const point_list& nodes, const point_list& normals,
		   const point_list& texcoords,
		   const int_list node_index,
		   const int_list& tex_index,
		   const bool calc_lights, ssgVertexArray *lights );


#endif // _OBJ_HXX
