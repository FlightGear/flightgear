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
#include <simgear/math/vector.hxx>

#include <Scenery/scenery.hxx>

#include "globals.hxx"
#include "viewer_rph.hxx"


// Constructor
FGViewerRPH::FGViewerRPH( void )
{
    set_reverse_view_offset(false);
#ifndef USE_FAST_VIEWROT
    // This never changes -- NHV
    LARC_TO_SSG[0][0] = 0.0; 
    LARC_TO_SSG[0][1] = 1.0; 
    LARC_TO_SSG[0][2] = -0.0; 
    LARC_TO_SSG[0][3] = 0.0; 

    LARC_TO_SSG[1][0] = 0.0; 
    LARC_TO_SSG[1][1] = 0.0; 
    LARC_TO_SSG[1][2] = 1.0; 
    LARC_TO_SSG[1][3] = 0.0;
	
    LARC_TO_SSG[2][0] = 1.0; 
    LARC_TO_SSG[2][1] = -0.0; 
    LARC_TO_SSG[2][2] = 0.0; 
    LARC_TO_SSG[2][3] = 0.0;
	
    LARC_TO_SSG[3][0] = 0.0; 
    LARC_TO_SSG[3][1] = 0.0; 
    LARC_TO_SSG[3][2] = 0.0; 
    LARC_TO_SSG[3][3] = 1.0; 
#endif // USE_FAST_VIEWROT
}


#define USE_FAST_VIEWROT
#ifdef USE_FAST_VIEWROT
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
#endif


#define USE_FAST_LOCAL
#ifdef USE_FAST_LOCAL
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
#endif


// convert sgMat4 to MAT3 and print
static void print_sgMat4( sgMat4 &in) {
    int i, j;
    for ( i = 0; i < 4; i++ ) {
	for ( j = 0; j < 4; j++ ) {
	    printf("%10.4f ", in[i][j]);
	}
	cout << endl;
    }
}


// Update the view parameters
void FGViewerRPH::update() {
    Point3D tmp;
    sgVec3 minus_z, forward;
    sgMat4 VIEWo;

    // calculate the cartesion coords of the current lat/lon/0 elev
    Point3D p = Point3D( geod_view_pos[0], 
			 geod_view_pos[1], 
			 sea_level_radius );

    tmp = sgPolarToCart3d(p) - scenery.center;
    sgSetVec3( zero_elev, tmp[0], tmp[1], tmp[2] );

    // calculate view position in current FG view coordinate system
    // p.lon & p.lat are already defined earlier, p.radius was set to
    // the sea level radius, so now we add in our altitude.
    if ( geod_view_pos[2] > (scenery.cur_elev + 0.5 * METER_TO_FEET) ) {
	p.setz( p.radius() + geod_view_pos[2] );
    } else {
	p.setz( p.radius() + scenery.cur_elev + 0.5 * METER_TO_FEET );
    }

    tmp = sgPolarToCart3d(p);
    sgdSetVec3( abs_view_pos, tmp[0], tmp[1], tmp[2] );

    // view_pos = abs_view_pos - scenery.center;
    sgdVec3 sc;
    sgdSetVec3( sc, scenery.center.x(), scenery.center.y(), scenery.center.z());
    sgdVec3 vp;
    sgdSubVec3( vp, abs_view_pos, sc );
    sgSetVec3( view_pos, vp );

    FG_LOG( FG_VIEW, FG_DEBUG, "sea level radius = " << sea_level_radius );
    FG_LOG( FG_VIEW, FG_DEBUG, "Polar view pos = " << p );
    FG_LOG( FG_VIEW, FG_DEBUG, "Absolute view pos = "
	    << abs_view_pos[0] << ","
	    << abs_view_pos[1] << ","
	    << abs_view_pos[2] );
    FG_LOG( FG_VIEW, FG_DEBUG, "(RPH) Relative view pos = "
	    << view_pos[0] << "," << view_pos[1] << "," << view_pos[2] );

    // code to calculate LOCAL matrix calculated from Phi, Theta, and
    // Psi (roll, pitch, yaw) in case we aren't running LaRCsim as our
    // flight model
	
#ifdef USE_FAST_LOCAL
	
    fgMakeLOCAL( LOCAL, rph[1], rph[0], -rph[2] );
	
#else // USE_TEXT_BOOK_METHOD
	
    sgVec3 rollvec;
    sgSetVec3( rollvec, 0.0, 0.0, 1.0 );
    sgMat4 PHI;		// roll
    sgMakeRotMat4( PHI, rph[0] * SGD_RADIANS_TO_DEGREES, rollvec );

    sgVec3 pitchvec;
    sgSetVec3( pitchvec, 0.0, 1.0, 0.0 );
    sgMat4 THETA;		// pitch
    sgMakeRotMat4( THETA, rph[1] * SGD_RADIANS_TO_DEGREES, pitchvec );

    // ROT = PHI * THETA
    sgMat4 ROT;
    // sgMultMat4( ROT, PHI, THETA );
    sgCopyMat4( ROT, PHI );
    sgPostMultMat4( ROT, THETA );

    sgVec3 yawvec;
    sgSetVec3( yawvec, 1.0, 0.0, 0.0 );
    sgMat4 PSI;		// heading
    sgMakeRotMat4( PSI, -rph[2] * SGD_RADIANS_TO_DEGREES, yawvec );

    // LOCAL = ROT * PSI
    // sgMultMat4( LOCAL, ROT, PSI );
    sgCopyMat4( LOCAL, ROT );
    sgPostMultMat4( LOCAL, PSI );

#endif // USE_FAST_LOCAL
	
    // cout << "LOCAL matrix" << endl;
    // print_sgMat4( LOCAL );
	
    sgMakeRotMat4( UP, 
		   geod_view_pos[0] * SGD_RADIANS_TO_DEGREES,
		   0.0,
		   -geod_view_pos[1] * SGD_RADIANS_TO_DEGREES );

    sgSetVec3( world_up, UP[0][0], UP[0][1], UP[0][2] );
    // sgXformVec3( world_up, UP );
    // cout << "World Up = " << world_up[0] << "," << world_up[1] << ","
    //      << world_up[2] << endl;
    
    // Alternative method to Derive world up vector based on
    // *geodetic* coordinates
    // alt_up = sgPolarToCart(FG_Longitude, FG_Latitude, 1.0);
    // printf( "    Alt Up = (%.4f, %.4f, %.4f)\n", 
    //         alt_up.x, alt_up.y, alt_up.z);

    // VIEWo = LOCAL * UP
    // sgMultMat4( VIEWo, LOCAL, UP );
    sgCopyMat4( VIEWo, LOCAL );
    sgPostMultMat4( VIEWo, UP );
    // cout << "VIEWo matrix" << endl;
    // print_sgMat4( VIEWo );

    // generate the sg view up and forward vectors
    sgSetVec3( view_up, VIEWo[0][0], VIEWo[0][1], VIEWo[0][2] );
    // cout << "view = " << view[0] << ","
    //      << view[1] << "," << view[2] << endl;
    sgSetVec3( forward, VIEWo[2][0], VIEWo[2][1], VIEWo[2][2] );
    // cout << "forward = " << forward[0] << ","
    //      << forward[1] << "," << forward[2] << endl;

    // generate the pilot offset vector in world coordinates
    sgVec3 pilot_offset_world;
    sgSetVec3( pilot_offset_world, 
	       pilot_offset[2], pilot_offset[1], -pilot_offset[0] );
    sgXformVec3( pilot_offset_world, pilot_offset_world, VIEWo );

    // generate the view offset matrix
    sgMakeRotMat4( VIEW_OFFSET, view_offset * SGD_RADIANS_TO_DEGREES, view_up );
    // cout << "VIEW_OFFSET matrix" << endl;
    // print_sgMat4( VIEW_OFFSET );
    sgXformVec3( view_forward, forward, VIEW_OFFSET );
    FG_LOG( FG_VIEW, FG_DEBUG, "(RPH) view forward = "
	    << view_forward[0] << "," << view_forward[1] << ","
	    << view_forward[2] );
	
    // VIEW_ROT = LARC_TO_SSG * ( VIEWo * VIEW_OFFSET )
#ifdef USE_FAST_VIEWROT
    fgMakeViewRot( VIEW_ROT, VIEW_OFFSET, VIEWo );
#else
    // sgMultMat4( VIEW_ROT, VIEW_OFFSET, VIEWo );
    // sgPreMultMat4( VIEW_ROT, LARC_TO_SSG );
    sgCopyMat4( VIEW_ROT, VIEWo );
    sgPostMultMat4( VIEW_ROT, VIEW_OFFSET );
    sgPreMultMat4( VIEW_ROT, LARC_TO_SSG );
#endif
    // cout << "VIEW_ROT matrix" << endl;
    // print_sgMat4( VIEW_ROT );

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
#define USE_FAST_SURFACE_EAST
#ifdef USE_FAST_SURFACE_EAST
    sgVec3 world_down;
    sgNegateVec3(world_down, world_up);
    sgVectorProductVec3(surface_east, surface_south, world_down);
#else
    sgMakeRotMat4( TMP, SGD_PI_2 * SGD_RADIANS_TO_DEGREES, world_up );
    // cout << "sgMat4 TMP" << endl;
    // print_sgMat4( TMP );
    sgXformVec3(surface_east, surface_south, TMP);
#endif //  USE_FAST_SURFACE_EAST
    // cout << "Surface direction directly east " << surface_east[0] << ","
    //      << surface_east[1] << "," << surface_east[2] << endl;
    // cout << "Should be close to zero = "
    //      << sgScalarProductVec3(surface_south, surface_east) << endl;

    set_clean();
}


// Destructor
FGViewerRPH::~FGViewerRPH( void ) {
}
