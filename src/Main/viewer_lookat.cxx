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
#include <simgear/math/vector.hxx>

#include <Scenery/scenery.hxx>

#include "globals.hxx"
#include "viewer_lookat.hxx"


// Constructor
FGViewerLookAt::FGViewerLookAt( void )
{
}


static void fgLookAt( sgVec3 eye, sgVec3 center, sgVec3 up, sgMat4 &m ) {
    double x[3], y[3], z[3];
    double mag;

    /* Make rotation matrix */

    /* Z vector */
    z[0] = eye[0] - center[0];
    z[1] = eye[1] - center[1];
    z[2] = eye[2] - center[2];
    mag = sqrt( z[0]*z[0] + z[1]*z[1] + z[2]*z[2] );
    if (mag) {  /* mpichler, 19950515 */
        z[0] /= mag;
        z[1] /= mag;
        z[2] /= mag;
    }

    /* Y vector */
    y[0] = up[0];
    y[1] = up[1];
    y[2] = up[2];

    /* X vector = Y cross Z */
    x[0] =  y[1]*z[2] - y[2]*z[1];
    x[1] = -y[0]*z[2] + y[2]*z[0];
    x[2] =  y[0]*z[1] - y[1]*z[0];

    /* Recompute Y = Z cross X */
    y[0] =  z[1]*x[2] - z[2]*x[1];
    y[1] = -z[0]*x[2] + z[2]*x[0];
    y[2] =  z[0]*x[1] - z[1]*x[0];

    /* mpichler, 19950515 */
    /* cross product gives area of parallelogram, which is < 1.0 for
     * non-perpendicular unit-length vectors; so normalize x, y here
     */

    mag = sqrt( x[0]*x[0] + x[1]*x[1] + x[2]*x[2] );
    if (mag) {
        x[0] /= mag;
        x[1] /= mag;
        x[2] /= mag;
    }

    mag = sqrt( y[0]*y[0] + y[1]*y[1] + y[2]*y[2] );
    if (mag) {
        y[0] /= mag;
        y[1] /= mag;
        y[2] /= mag;
    }

#define M(row,col)  m[row][col]
    M(0,0) = x[0];  M(0,1) = x[1];  M(0,2) = x[2];  M(0,3) = 0.0;
    M(1,0) = y[0];  M(1,1) = y[1];  M(1,2) = y[2];  M(1,3) = 0.0;
    M(2,0) = z[0];  M(2,1) = z[1];  M(2,2) = z[2];  M(2,3) = 0.0;
    M(3,0) = -eye[0]; M(3,1) = -eye[1]; M(3,2) = -eye[2]; M(3,3) = 1.0;
#undef M
}


// Initialize a view structure
void FGViewerLookAt::init( void ) {
    set_dirty();

    FG_LOG( FG_VIEW, FG_INFO, "Initializing View parameters" );

    view_offset = goal_view_offset =
	globals->get_options()->get_default_view_offset();
    sgSetVec3( pilot_offset, 0.0, 0.0, 0.0 );

    globals->get_options()->set_win_ratio( globals->get_options()->get_xsize() /
					   globals->get_options()->get_ysize()
					   );
}


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
void FGViewerLookAt::update() {
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
    FG_LOG( FG_VIEW, FG_DEBUG, "Relative view pos = "
	    << view_pos[0] << "," << view_pos[1] << "," << view_pos[2] );
    FG_LOG( FG_VIEW, FG_DEBUG, "view forward = "
	    << view_forward[0] << "," << view_forward[1] << ","
	    << view_forward[2] );
    FG_LOG( FG_VIEW, FG_DEBUG, "view up = "
	    << view_up[0] << "," << view_up[1] << ","
	    << view_up[2] );

    // Make the VIEW matrix.
    fgLookAt( view_pos, view_forward, view_up, VIEW );
    // cout << "VIEW matrix" << endl;
    // print_sgMat4( VIEW );

    // the VIEW matrix includes both rotation and translation.  Let's
    // knock out the translation part to make the VIEW_ROT matrix
    sgCopyMat4( VIEW_ROT, VIEW );
    VIEW_ROT[3][0] = VIEW_ROT[3][1] = VIEW_ROT[3][2] = 0.0;

    // Make the world up rotation matrix
    sgMakeRotMat4( UP, 
		   geod_view_pos[0] * RAD_TO_DEG,
		   0.0,
		   -geod_view_pos[1] * RAD_TO_DEG );

    // use a clever observation into the nature of our tranformation
    // matrix to grab the world_up vector
    sgSetVec3( world_up, UP[0][0], UP[0][1], UP[0][2] );
    // cout << "World Up = " << world_up[0] << "," << world_up[1] << ","
    //      << world_up[2] << endl;
    

    //!!!!!!!!!!!!!!!!!!!	
    // THIS IS THE EXPERIMENTAL VIEWING ANGLE SHIFTER
    // THE MAJORITY OF THE WORK IS DONE IN GUI.CXX
    // this in gui.cxx for now just testing
    extern float quat_mat[4][4];
    sgPreMultMat4( VIEW, quat_mat);
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
    sgMakeRotMat4( TMP, FG_PI_2 * RAD_TO_DEG, world_up );
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
FGViewerLookAt::~FGViewerLookAt( void ) {
}
