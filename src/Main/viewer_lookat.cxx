// viewer_lookat.hxx -- class for managing a "look at" viewer in
//                      the flightgear world.
//
// Written by Curtis Olson, started October 2000.
//
// Copyright (C) 2000  Curtis L. Olson  - curt@flightgear.org
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


#include <simgear/compiler.h>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <plib/ssg.h>		// plib include

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/math/point3d.hxx>
#include <simgear/math/polar3d.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/math/vector.hxx>

#include <Scenery/scenery.hxx>

#include "globals.hxx"
#include "viewer_lookat.hxx"


// Constructor
FGViewerLookAt::FGViewerLookAt( void )
{
    set_reverse_view_offset(true);
}


void fgMakeLookAtMat4 ( sgMat4 dst, const sgVec3 eye, const sgVec3 center,
			const sgVec3 up )
{
  // Caveats:
  // 1) In order to compute the line of sight, the eye point must not be equal
  //    to the center point.
  // 2) The up vector must not be parallel to the line of sight from the eye
  //    to the center point.

  /* Compute the direction vectors */
  sgVec3 x,y,z;

  /* Y vector = center - eye */
  sgSubVec3 ( y, center, eye ) ;

  /* Z vector = up */
  sgCopyVec3 ( z, up ) ;

  /* X vector = Y cross Z */
  sgVectorProductVec3 ( x, y, z ) ;

  /* Recompute Z = X cross Y */
  sgVectorProductVec3 ( z, x, y ) ;

  /* Normalize everything */
  sgNormaliseVec3 ( x ) ;
  sgNormaliseVec3 ( y ) ;
  sgNormaliseVec3 ( z ) ;

  /* Build the matrix */
#define M(row,col)  dst[row][col]
  M(0,0) = x[0];    M(0,1) = x[1];    M(0,2) = x[2];    M(0,3) = 0.0;
  M(1,0) = y[0];    M(1,1) = y[1];    M(1,2) = y[2];    M(1,3) = 0.0;
  M(2,0) = z[0];    M(2,1) = z[1];    M(2,2) = z[2];    M(2,3) = 0.0;
  M(3,0) = eye[0];  M(3,1) = eye[1];  M(3,2) = eye[2];  M(3,3) = 1.0;
#undef M
}


// Update the view parameters
void FGViewerLookAt::update() {
    sgVec3 minus_z;

    view_point.setPosition(geod_view_pos[0] * SGD_RADIANS_TO_DEGREES,
			   geod_view_pos[1] * SGD_RADIANS_TO_DEGREES,
			   geod_view_pos[2] * SG_METER_TO_FEET);
    sgCopyVec3(zero_elev, view_point.getZeroElevViewPos());
    sgdCopyVec3(abs_view_pos, view_point.getAbsoluteViewPos());
    sgCopyVec3(view_pos, view_point.getRelativeViewPos());

    sgVec3 tmp_offset;
    sgCopyVec3( tmp_offset, pilot_offset );
    SG_LOG( SG_VIEW, SG_DEBUG, "tmp offset = "
            << tmp_offset[0] << "," << tmp_offset[1] << ","
            << tmp_offset[2] );
	
    //!!!!!!!!!!!!!!!!!!!	
    // THIS IS THE EXPERIMENTAL VIEWING ANGLE SHIFTER
    // THE MAJORITY OF THE WORK IS DONE IN GUI.CXX
    extern float GuiQuat_mat[4][4];
    sgXformPnt3( tmp_offset, tmp_offset, GuiQuat_mat );
    SG_LOG( SG_VIEW, SG_DEBUG, "tmp_offset = "
            << tmp_offset[0] << "," << tmp_offset[1] << ","
            << tmp_offset[2] );
	
    sgAddVec3( view_pos, tmp_offset );
    // !!!!!!!!!! testing
	
    // Make the VIEW matrix.
    fgMakeLookAtMat4( VIEW, view_pos, view_forward, view_up );

    // the VIEW matrix includes both rotation and translation.  Let's
    // knock out the translation part to make the VIEW_ROT matrix
    sgCopyMat4( VIEW_ROT, VIEW );
    VIEW_ROT[3][0] = VIEW_ROT[3][1] = VIEW_ROT[3][2] = 0.0;

    // Make the world up rotation matrix
    sgMakeRotMat4( UP, 
		   geod_view_pos[0] * SGD_RADIANS_TO_DEGREES,
		   0.0,
		   -geod_view_pos[1] * SGD_RADIANS_TO_DEGREES );

    // use a clever observation into the nature of our tranformation
    // matrix to grab the world_up vector
    sgSetVec3( world_up, UP[0][0], UP[0][1], UP[0][2] );

    // Given a vector pointing straight down (-Z), map into onto the
    // local plane representing "horizontal".  This should give us the
    // local direction for moving "south".
    sgSetVec3( minus_z, 0.0, 0.0, -1.0 );

    sgmap_vec_onto_cur_surface_plane(world_up, view_pos, minus_z,
				     surface_south);
    sgNormalizeVec3(surface_south);

    // now calculate the surface east vector
    sgVec3 world_down;
    sgNegateVec3(world_down, world_up);
    sgVectorProductVec3(surface_east, surface_south, world_down);

    set_clean();
}


// Destructor
FGViewerLookAt::~FGViewerLookAt( void ) {
}
