// views.cxx -- data structures and routines for managing and view
//               parameters.
//
// Written by Curtis Olson, started August 1997.
//
// Copyright (C) 1997  Curtis L. Olson  - curt@flightgear.org
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

#include <Aircraft/aircraft.hxx>
#include <Cockpit/panel.hxx>
#include <Scenery/scenery.hxx>

#include "options.hxx"
#include "views.hxx"


// This is a record containing current view parameters for the current
// aircraft position
FGView pilot_view;

// This is a record containing current view parameters for the current
// view position
FGView current_view;


// Constructor
FGView::FGView( void ) {
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

// Initialize a view structure
void FGView::Init( void ) {
    FG_LOG( FG_VIEW, FG_INFO, "Initializing View parameters" );

    view_offset = 0.0;
    goal_view_offset = 0.0;
    sgSetVec3( pilot_offset, 0.0, 0.0, 0.0 );

    winWidth = current_options.get_xsize();
    winHeight = current_options.get_ysize();

    set_win_ratio( winHeight / winWidth );

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

    force_update_fov_math();
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


// Update the view volume, position, and orientation
void FGView::UpdateViewParams( const FGInterface& f ) {
    UpdateViewMath(f);
    
    if ( ! current_options.get_panel_status() ) {
	xglViewport(0, 0 , (GLint)(winWidth), (GLint)(winHeight) );
    } else {
        int view_h =
	  int((current_panel->getViewHeight() - current_panel->getYOffset())
	      * (winHeight / 768.0));
	glViewport(0, (GLint)(winHeight - view_h),
		   (GLint)(winWidth), (GLint)(view_h) );
    }
}


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
void FGView::UpdateViewMath( const FGInterface& f ) {

    Point3D p;
    sgVec3 v0, minus_z, sgvec, forward;
    sgMat4 VIEWo, TMP;

    if ( update_fov ) {
	ssgSetFOV( current_options.get_fov(), 
		   current_options.get_fov() * win_ratio );
	update_fov = false;
    }
		
    scenery.center = scenery.next_center;

    // printf("scenery center = %.2f %.2f %.2f\n", scenery.center.x,
    //        scenery.center.y, scenery.center.z);

    // calculate the cartesion coords of the current lat/lon/0 elev
    p = Point3D( f.get_Longitude(), 
		 f.get_Lat_geocentric(), 
		 f.get_Sea_level_radius() * FEET_TO_METER );

    cur_zero_elev = sgPolarToCart3d(p) - scenery.center;

    // calculate view position in current FG view coordinate system
    // p.lon & p.lat are already defined earlier, p.radius was set to
    // the sea level radius, so now we add in our altitude.
    if ( f.get_Altitude() * FEET_TO_METER > 
	 (scenery.cur_elev + 0.5 * METER_TO_FEET) ) {
	p.setz( p.radius() + f.get_Altitude() * FEET_TO_METER );
    } else {
	p.setz( p.radius() + scenery.cur_elev + 0.5 * METER_TO_FEET );
    }

    abs_view_pos = sgPolarToCart3d(p);
	
    view_pos = abs_view_pos - scenery.center;

    FG_LOG( FG_VIEW, FG_DEBUG, "Polar view pos = " << p );
    FG_LOG( FG_VIEW, FG_DEBUG, "Absolute view pos = " << abs_view_pos );
    FG_LOG( FG_VIEW, FG_DEBUG, "Relative view pos = " << view_pos );

    // code to calculate LOCAL matrix calculated from Phi, Theta, and
    // Psi (roll, pitch, yaw) in case we aren't running LaRCsim as our
    // flight model
	
#ifdef USE_FAST_LOCAL
	
    fgMakeLOCAL( LOCAL, f.get_Theta(), f.get_Phi(), -f.get_Psi() );
	
#else // USE_TEXT_BOOK_METHOD
	
    sgVec3 rollvec;
    sgSetVec3( rollvec, 0.0, 0.0, 1.0 );
    sgMat4 PHI;		// roll
    sgMakeRotMat4( PHI, f.get_Phi() * RAD_TO_DEG, rollvec );

    sgVec3 pitchvec;
    sgSetVec3( pitchvec, 0.0, 1.0, 0.0 );
    sgMat4 THETA;		// pitch
    sgMakeRotMat4( THETA, f.get_Theta() * RAD_TO_DEG, pitchvec );

    // ROT = PHI * THETA
    sgMat4 ROT;
    // sgMultMat4( ROT, PHI, THETA );
    sgCopyMat4( ROT, PHI );
    sgPostMultMat4( ROT, THETA );

    sgVec3 yawvec;
    sgSetVec3( yawvec, 1.0, 0.0, 0.0 );
    sgMat4 PSI;		// pitch
    sgMakeRotMat4( PSI, -f.get_Psi() * RAD_TO_DEG, yawvec );

    // LOCAL = ROT * PSI
    // sgMultMat4( LOCAL, ROT, PSI );
    sgCopyMat4( LOCAL, ROT );
    sgPostMultMat4( LOCAL, PSI );

#endif // YIKES
	
    // cout << "LOCAL matrix" << endl;
    // print_sgMat4( LOCAL );
	
    sgMakeRotMat4( UP, 
		   f.get_Longitude() * RAD_TO_DEG,
		   0.0,
		   -f.get_Latitude() * RAD_TO_DEG );

    sgSetVec3( local_up, UP[0][0], UP[0][1], UP[0][2] );
    // sgXformVec3( local_up, UP );
    // cout << "Local Up = " << local_up[0] << "," << local_up[1] << ","
    //      << local_up[2] << endl;
    
    // Alternative method to Derive local up vector based on
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
    sgMakeRotMat4( VIEW_OFFSET, view_offset * RAD_TO_DEG, view_up );
    // cout << "VIEW_OFFSET matrix" << endl;
    // print_sgMat4( VIEW_OFFSET );
    sgXformVec3( view_forward, forward, VIEW_OFFSET );
    // cout << "view_forward = " << view_forward[0] << ","
    //      << view_forward[1] << "," << view_forward[2] << endl;
	
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
    sgSetVec3( trans_vec, 
	       view_pos.x() + pilot_offset_world[0],
	       view_pos.y() + pilot_offset_world[1],
	       view_pos.z() + pilot_offset_world[2] );

    // VIEW = VIEW_ROT * TRANS
    sgCopyMat4( VIEW, VIEW_ROT );
    sgPostMultMat4ByTransMat4( VIEW, trans_vec );

    //!!!!!!!!!!!!!!!!!!!	
    // THIS IS THE EXPERIMENTAL VIEWING ANGLE SHIFTER
    // THE MAJORITY OF THE WORK IS DONE IN GUI.CXX
    // this in gui.cxx for now just testing
    extern float quat_mat[4][4];
    sgPreMultMat4( VIEW, quat_mat);
    // !!!!!!!!!! testing	

    // make a vector to the current view position
    sgSetVec3( v0, view_pos.x(), view_pos.y(), view_pos.z() );

    // Given a vector pointing straight down (-Z), map into onto the
    // local plane representing "horizontal".  This should give us the
    // local direction for moving "south".
    sgSetVec3( minus_z, 0.0, 0.0, -1.0 );

    sgmap_vec_onto_cur_surface_plane(local_up, v0, minus_z, surface_south);
    sgNormalizeVec3(surface_south);
    // cout << "Surface direction directly south " << surface_south[0] << ","
    //      << surface_south[1] << "," << surface_south[2] << endl;

    // now calculate the surface east vector
#define USE_FAST_SURFACE_EAST
#ifdef USE_FAST_SURFACE_EAST
    sgVec3 local_down;
    sgNegateVec3(local_down, local_up);
    sgVectorProductVec3(surface_east, surface_south, local_down);
#else
#define USE_LOCAL_UP
#ifdef USE_LOCAL_UP
    sgMakeRotMat4( TMP, FG_PI_2 * RAD_TO_DEG, local_up );
#else
    sgMakeRotMat4( TMP, FG_PI_2 * RAD_TO_DEG, view_up );
#endif // USE_LOCAL_UP
    // cout << "sgMat4 TMP" << endl;
    // print_sgMat4( TMP );
    sgXformVec3(surface_east, surface_south, TMP);
#endif //  USE_FAST_SURFACE_EAST
    // cout << "Surface direction directly east " << surface_east[0] << ","
    //      << surface_east[1] << "," << surface_east[2] << endl;
    // cout << "Should be close to zero = "
    //      << sgScalarProductVec3(surface_south, surface_east) << endl;
}


void FGView::CurrentNormalInLocalPlane(sgVec3 dst, sgVec3 src) {
    sgVec3 tmp;
    sgSetVec3(tmp, src[0], src[1], src[2] );
    sgMat4 TMP;
    sgTransposeNegateMat4 ( TMP, UP ) ;
    sgXformVec3(tmp, tmp, TMP);
    sgSetVec3(dst, tmp[2], tmp[1], tmp[0] );
}


// Destructor
FGView::~FGView( void ) {
}
