// viewer_rph.cxx -- class for managing a Roll/Pitch/Heading viewer in
//                   the flightgear world.
//
// Written by Curtis Olson, started August 1997.
//                          overhaul started October 2000.
//
// Copyright (C) 1997 - 2000  Curtis L. Olson  - curt@flightgear.org
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
#include "viewer_rph.hxx"


// Constructor
FGViewerRPH::FGViewerRPH( void )
{
    set_reverse_view_offset(false);
}


// VIEW_ROT = LARC_TO_SSG * ( VIEWo * VIEW_OFFSET )
// This takes advantage of the fact that VIEWo and VIEW_OFFSET
// only have entries in the upper 3x3 block
// and that LARC_TO_SSG is just a shift of rows   NHV
inline static void fgMakeViewRot( sgMat4 dst, const sgMat4 m1, const sgMat4 m2 )
{
    for ( int j = 0 ; j < 3 ; j++ ) {
	dst[2][j] = m2[0][0] * m1[0][j] +
	    m2[0][1] * m1[1][j] +
	    m2[0][2] * m1[2][j];

	dst[0][j] = m2[1][0] * m1[0][j] +
	    m2[1][1] * m1[1][j] +
	    m2[1][2] * m1[2][j];

	dst[1][j] = m2[2][0] * m1[0][j] +
	    m2[2][1] * m1[1][j] +
	    m2[2][2] * m1[2][j];
    }
    dst[0][3] = 
	dst[1][3] = 
	dst[2][3] = 
	dst[3][0] = 
	dst[3][1] = 
	dst[3][2] = SG_ZERO;
    dst[3][3] = SG_ONE;
}


inline static void fgMakeLOCAL( sgMat4 dst, const double Theta,
				const double Phi, const double Psi)
{
    SGfloat cosTheta = (SGfloat) cos(Theta);
    SGfloat sinTheta = (SGfloat) sin(Theta);
    SGfloat cosPhi   = (SGfloat) cos(Phi);
    SGfloat sinPhi   = (SGfloat) sin(Phi);
    SGfloat sinPsi   = (SGfloat) sin(Psi) ;
    SGfloat cosPsi   = (SGfloat) cos(Psi) ;
	
    dst[0][0] = cosPhi * cosTheta;
    dst[0][1] =	sinPhi * cosPsi + cosPhi * -sinTheta * -sinPsi;
    dst[0][2] =	sinPhi * sinPsi + cosPhi * -sinTheta * cosPsi;
    dst[0][3] =	SG_ZERO;

    dst[1][0] = -sinPhi * cosTheta;
    dst[1][1] =	cosPhi * cosPsi + -sinPhi * -sinTheta * -sinPsi;
    dst[1][2] =	cosPhi * sinPsi + -sinPhi * -sinTheta * cosPsi;
    dst[1][3] = SG_ZERO ;
	
    dst[2][0] = sinTheta;
    dst[2][1] =	cosTheta * -sinPsi;
    dst[2][2] =	cosTheta * cosPsi;
    dst[2][3] = SG_ZERO;
	
    dst[3][0] = SG_ZERO;
    dst[3][1] = SG_ZERO;
    dst[3][2] = SG_ZERO;
    dst[3][3] = SG_ONE ;
}


// Update the view parameters
void FGViewerRPH::update() {
    sgVec3 minus_z, right, forward, tilt;
    sgMat4 VIEWo;

    view_point.setPosition(geod_view_pos[0] * SGD_RADIANS_TO_DEGREES,
			   geod_view_pos[1] * SGD_RADIANS_TO_DEGREES,
			   geod_view_pos[2] * SG_METER_TO_FEET);
    sgCopyVec3(zero_elev, view_point.getZeroElevViewPos());
    sgdCopyVec3(abs_view_pos, view_point.getAbsoluteViewPos());
    sgCopyVec3(view_pos, view_point.getRelativeViewPos());

    // code to calculate LOCAL matrix calculated from Phi, Theta, and
    // Psi (roll, pitch, yaw) in case we aren't running LaRCsim as our
    // flight model
	
    fgMakeLOCAL( LOCAL, rph[1], rph[0], -rph[2] );
	
    sgMakeRotMat4( UP, 
		   geod_view_pos[0] * SGD_RADIANS_TO_DEGREES,
		   0.0,
		   -geod_view_pos[1] * SGD_RADIANS_TO_DEGREES );

    sgSetVec3( world_up, UP[0][0], UP[0][1], UP[0][2] );
    sgCopyMat4( VIEWo, LOCAL );
    sgPostMultMat4( VIEWo, UP );

    // generate the sg view up and forward vectors
    sgSetVec3( view_up, VIEWo[0][0], VIEWo[0][1], VIEWo[0][2] );
    sgSetVec3( right, VIEWo[1][0], VIEWo[1][1], VIEWo[1][2] );
    sgSetVec3( forward, VIEWo[2][0], VIEWo[2][1], VIEWo[2][2] );

    // generate the pilot offset vector in world coordinates
    sgVec3 pilot_offset_world;
    sgSetVec3( pilot_offset_world, 
	       pilot_offset[2], pilot_offset[1], -pilot_offset[0] );
    sgXformVec3( pilot_offset_world, pilot_offset_world, VIEWo );

    // generate the view offset matrix
    sgMakeRotMat4( VIEW_OFFSET, view_offset * SGD_RADIANS_TO_DEGREES, view_up );

    sgMat4 VIEW_TILT;
    sgMakeRotMat4( VIEW_TILT, view_tilt * SGD_RADIANS_TO_DEGREES, right );
    sgPreMultMat4(VIEW_OFFSET, VIEW_TILT);
    // cout << "VIEW_OFFSET matrix" << endl;
    // print_sgMat4( VIEW_OFFSET );
    sgXformVec3( view_forward, forward, VIEW_OFFSET );
    SG_LOG( SG_VIEW, SG_DEBUG, "(RPH) view forward = "
	    << view_forward[0] << "," << view_forward[1] << ","
	    << view_forward[2] );
	
    // VIEW_ROT = LARC_TO_SSG * ( VIEWo * VIEW_OFFSET )
    fgMakeViewRot( VIEW_ROT, VIEW_OFFSET, VIEWo );

    sgVec3 trans_vec;
    sgAddVec3( trans_vec, view_pos, pilot_offset_world );

    // VIEW = VIEW_ROT * TRANS
    sgCopyMat4( VIEW, VIEW_ROT );
    sgPostMultMat4ByTransMat4( VIEW, trans_vec );

    //!!!!!!!!!!!!!!!!!!!	
    // THIS IS THE EXPERIMENTAL VIEWING ANGLE SHIFTER
    // THE MAJORITY OF THE WORK IS DONE IN GUI.CXX
    // this in gui.cxx for now just testing
    extern float GuiQuat_mat[4][4];
    sgPreMultMat4( VIEW, GuiQuat_mat);
    // !!!!!!!!!! testing	

    // Given a vector pointing straight down (-Z), map into onto the
    // local plane representing "horizontal".  This should give us the
    // local direction for moving "south".
    sgSetVec3( minus_z, 0.0, 0.0, -1.0 );

    sgmap_vec_onto_cur_surface_plane(world_up, view_pos, minus_z,
				     surface_south);
    sgNormalizeVec3(surface_south);
    // cout << "Surface direction directly south " << surface_south[0] << ","
    //      << surface_south[1] << "," << surface_south[2] << endl;

    // now calculate the surface east vector
    sgVec3 world_down;
    sgNegateVec3(world_down, world_up);
    sgVectorProductVec3(surface_east, surface_south, world_down);

    set_clean();
}


// Destructor
FGViewerRPH::~FGViewerRPH( void ) {
}
