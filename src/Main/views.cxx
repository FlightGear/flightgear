// views.cxx -- data structures and routines for managing and view
//               parameters.
//
// Written by Curtis Olson, started August 1997.
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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <ssg.h>		// plib include

#include <Aircraft/aircraft.hxx>
#include <Cockpit/panel.hxx>
#include <Debug/logstream.hxx>
#include <Include/fg_constants.h>
#include <Math/mat3.h>
#include <Math/point3d.hxx>
#include <Math/polar3d.hxx>
#include <Math/vector.hxx>
#include <Scenery/scenery.hxx>
#include <Time/fg_time.hxx>

#include "options.hxx"
#include "views.hxx"


// temporary (hopefully) hack
static int panel_hist = 0;


// This is a record containing current view parameters for the current
// aircraft position
FGView pilot_view;

// This is a record containing current view parameters for the current
// view position
FGView current_view;


// Constructor
FGView::FGView( void ) {
}


// Initialize a view structure
void FGView::Init( void ) {
    FG_LOG( FG_VIEW, FG_INFO, "Initializing View parameters" );

    view_offset = 0.0;
    goal_view_offset = 0.0;

    winWidth = current_options.get_xsize();
    winHeight = current_options.get_ysize();

    if ( ! current_options.get_panel_status() ) {
	current_view.set_win_ratio( (GLfloat) winWidth / (GLfloat) winHeight );
    } else {
	current_view.set_win_ratio( (GLfloat) winWidth / 
				    ((GLfloat) (winHeight)*0.4232) );
    }

    // This never changes -- NHV
    sgLARC_TO_SSG[0][0] = 0.0; 
    sgLARC_TO_SSG[0][1] = 1.0; 
    sgLARC_TO_SSG[0][2] = -0.0; 
    sgLARC_TO_SSG[0][3] = 0.0; 

    sgLARC_TO_SSG[1][0] = 0.0; 
    sgLARC_TO_SSG[1][1] = 0.0; 
    sgLARC_TO_SSG[1][2] = 1.0; 
    sgLARC_TO_SSG[1][3] = 0.0;
	
    sgLARC_TO_SSG[2][0] = 1.0; 
    sgLARC_TO_SSG[2][1] = -0.0; 
    sgLARC_TO_SSG[2][2] = 0.0; 
    sgLARC_TO_SSG[2][3] = 0.0;
	
    sgLARC_TO_SSG[3][0] = 0.0; 
    sgLARC_TO_SSG[3][1] = 0.0; 
    sgLARC_TO_SSG[3][2] = 0.0; 
    sgLARC_TO_SSG[3][3] = 1.0; 
	
    force_update_fov_math();
}

// Update the view volume, position, and orientation
void FGView::UpdateViewParams( const FGInterface& f ) {
    UpdateViewMath(f);
    
    if ((current_options.get_panel_status() != panel_hist) &&                          (current_options.get_panel_status()))
    {
	FGPanel::OurPanel->ReInit( 0, 0, 1024, 768);
    }

    if ( ! current_options.get_panel_status() ) {
	xglViewport(0, 0 , (GLint)(winWidth), (GLint)(winHeight) );
    } else {
	xglViewport(0, (GLint)((winHeight)*0.5768), (GLint)(winWidth), 
		    (GLint)((winHeight)*0.4232) );
    }

    panel_hist = current_options.get_panel_status();
}


// convert sgMat4 to MAT3 and print
static void print_sgMat4( sgMat4 &in) {
    MAT3mat print;
    int i;
    int j;
    for ( i = 0; i < 4; i++ ) {
	for ( j = 0; j < 4; j++ ) {
	    print[i][j] = in[i][j];
	}
    }
    MAT3print( print, stdout);
}


// convert convert MAT3 to sgMat4
static void MAT3mat_To_sgMat4( MAT3mat &in, sgMat4 &out ) {
    out[0][0] = in[0][0];
    out[0][1] = in[0][1];
    out[0][2] = in[0][2];
    out[0][3] = in[0][3];
    out[1][0] = in[1][0];
    out[1][1] = in[1][1];
    out[1][2] = in[1][2];
    out[1][3] = in[1][3];
    out[2][0] = in[2][0];
    out[2][1] = in[2][1];
    out[2][2] = in[2][2];
    out[2][3] = in[2][3];
    out[3][0] = in[3][0];
    out[3][1] = in[3][1];
    out[3][2] = in[3][2];
    out[3][3] = in[3][3];
}


// Update the view parameters
void FGView::UpdateViewMath( const FGInterface& f ) {
    Point3D p;
    sgVec3 v0, minus_z;
    MAT3vec vec, forward;
    MAT3mat R, TMP, UP, LOCAL, VIEW;
    sgMat4 sgTMP;

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

    cur_zero_elev = fgPolarToCart3d(p) - scenery.center;

    // calculate view position in current FG view coordinate system
    // p.lon & p.lat are already defined earlier, p.radius was set to
    // the sea level radius, so now we add in our altitude.
    if ( f.get_Altitude() * FEET_TO_METER > 
	 (scenery.cur_elev + 0.5 * METER_TO_FEET) ) {
	p.setz( p.radius() + f.get_Altitude() * FEET_TO_METER );
    } else {
	p.setz( p.radius() + scenery.cur_elev + 0.5 * METER_TO_FEET );
    }

    abs_view_pos = fgPolarToCart3d(p);
	
    view_pos = abs_view_pos - scenery.center;

    FG_LOG( FG_VIEW, FG_DEBUG, "Polar view pos = " << p );
    FG_LOG( FG_VIEW, FG_DEBUG, "Absolute view pos = " << abs_view_pos );
    FG_LOG( FG_VIEW, FG_DEBUG, "Relative view pos = " << view_pos );

    // code to calculate LOCAL matrix calculated from Phi, Theta, and
    // Psi (roll, pitch, yaw) in case we aren't running LaRCsim as our
    // flight model

    MAT3_SET_VEC(vec, 0.0, 0.0, 1.0);
    MAT3rotate(R, vec, f.get_Phi());
    // cout << "Roll matrix" << endl;
    // MAT3print(R, stdout);

    sgVec3 sgrollvec;
    sgSetVec3( sgrollvec, 0.0, 0.0, 1.0 );
    sgMat4 sgPHI;		// roll
    sgMakeRotMat4( sgPHI, f.get_Phi() * RAD_TO_DEG, sgrollvec );

    MAT3_SET_VEC(vec, 0.0, 1.0, 0.0);
    MAT3rotate(TMP, vec, f.get_Theta());
    // cout << "Pitch matrix" << endl;;
    // MAT3print(TMP, stdout);
    MAT3mult(R, R, TMP);
    // cout << "tmp rotation matrix, R:" << endl;;
    // MAT3print(R, stdout);

    sgVec3 sgpitchvec;
    sgSetVec3( sgpitchvec, 0.0, 1.0, 0.0 );
    sgMat4 sgTHETA;		// pitch
    sgMakeRotMat4( sgTHETA, f.get_Theta() * RAD_TO_DEG,
		   sgpitchvec );

    sgMat4 sgROT;
    sgMultMat4( sgROT, sgPHI, sgTHETA );

    MAT3_SET_VEC(vec, 1.0, 0.0, 0.0);
    MAT3rotate(TMP, vec, -f.get_Psi());
    // cout << "Yaw matrix" << endl;
    // MAT3print(TMP, stdout);
    MAT3mult(LOCAL, R, TMP);
    // cout << "LOCAL matrix:" << endl;
    // MAT3print(LOCAL, stdout);

    sgVec3 sgyawvec;
    sgSetVec3( sgyawvec, 1.0, 0.0, 0.0 );
    sgMat4 sgPSI;		// pitch
    sgMakeRotMat4( sgPSI, -f.get_Psi() * RAD_TO_DEG, sgyawvec );

    sgMultMat4( sgLOCAL, sgROT, sgPSI );
    // cout << "sgLOCAL matrix" << endl;
    // print_sgMat4( sgLOCAL );
	
    // Derive the local UP transformation matrix based on *geodetic*
    // coordinates
    MAT3_SET_VEC(vec, 0.0, 0.0, 1.0);
    MAT3rotate(R, vec, f.get_Longitude());     // R = rotate about Z axis
    // printf("Longitude matrix\n");
    // MAT3print(R, stdout);

    MAT3_SET_VEC(vec, 0.0, 1.0, 0.0);
    MAT3mult_vec(vec, vec, R);
    MAT3rotate(TMP, vec, -f.get_Latitude());  // TMP = rotate about X axis
    // printf("Latitude matrix\n");
    // MAT3print(TMP, stdout);

    MAT3mult(UP, R, TMP);
    // cout << "Local up matrix" << endl;;
    // MAT3print(UP, stdout);

    sgMakeRotMat4( sgUP, 
		   f.get_Longitude() * RAD_TO_DEG,
		   0.0,
		   -f.get_Latitude() * RAD_TO_DEG );
    /*
      cout << "FG derived UP matrix using sg routines" << endl;
    MAT3mat print;
    int i;
    int j;
    for ( i = 0; i < 4; i++ ) {
	for ( j = 0; j < 4; j++ ) {
	print[i][j] = sgUP[i][j];
	}
	}
    MAT3print( print, stdout);
    */

    sgSetVec3( local_up, 1.0, 0.0, 0.0 );
    sgXformVec3( local_up, sgUP );
    // cout << "Local Up = " << local_up[0] << "," << local_up[1] << ","
    //      << local_up[2] << endl;
    
    // Alternative method to Derive local up vector based on
    // *geodetic* coordinates
    // alt_up = fgPolarToCart(FG_Longitude, FG_Latitude, 1.0);
    // printf( "    Alt Up = (%.4f, %.4f, %.4f)\n", 
    //         alt_up.x, alt_up.y, alt_up.z);

    // Calculate the VIEW matrix
    MAT3mult(VIEW, LOCAL, UP);
    // cout << "VIEW matrix" << endl;;
    // MAT3print(VIEW, stdout);

    sgMat4 sgTMP2;
    sgMultMat4( sgTMP, sgLOCAL, sgUP );

    // generate the sg view up vector
    sgVec3 vec1;
    sgSetVec3( vec1, 1.0, 0.0, 0.0 );
    sgXformVec3( sgview_up, vec1, sgTMP );

    // generate the view offset matrix
    sgMakeRotMat4( sgVIEW_OFFSET, view_offset * RAD_TO_DEG, sgview_up );
    // cout << "sgVIEW_OFFSET matrix" << endl;
    // print_sgMat4( sgVIEW_OFFSET );
	
    sgMultMat4( sgTMP2, sgTMP, sgVIEW_OFFSET );
    sgMultMat4( sgVIEW_ROT, sgLARC_TO_SSG, sgTMP2 );

    sgMakeTransMat4( sgTRANS, view_pos.x(), view_pos.y(), view_pos.z() );

    sgMultMat4( sgVIEW, sgVIEW_ROT, sgTRANS );

    // FGMat4Wrapper tmp;
    // sgCopyMat4( tmp.m, sgVIEW );
    // follow.push_back( tmp );

    // generate the current up, forward, and fwrd-view vectors
    MAT3_SET_VEC(vec, 1.0, 0.0, 0.0);
    MAT3mult_vec(view_up, vec, VIEW);

    /*
    cout << "FG derived VIEW matrix using sg routines" << endl;
    MAT3mat print;
    int i;
    int j;
    for ( i = 0; i < 4; i++ ) {
	for ( j = 0; j < 4; j++ ) {
	    print[i][j] = sgVIEW[i][j];
	}
    }
    MAT3print( print, stdout);
    */

    MAT3_SET_VEC(vec, 0.0, 0.0, 1.0);
    MAT3mult_vec(forward, vec, VIEW);
    // printf( "Forward vector is (%.2f,%.2f,%.2f)\n", forward[0], forward[1], 
    //         forward[2]);

    MAT3rotate(TMP, view_up, view_offset);
    MAT3mult_vec(view_forward, forward, TMP);

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
    sgMakeRotMat4( sgTMP, FG_PI_2 * RAD_TO_DEG, sgview_up );
    // cout << "sgMat4 sgTMP" << endl;
    // print_sgMat4( sgTMP );
    sgXformVec3(surface_east, surface_south, sgTMP);
    // cout << "Surface direction directly east" << surface_east[0] << ","
    //      << surface_east[1] << "," << surface_east[2] << endl;
    // cout << "Should be close to zero = "
    //      << sgScalarProductVec3(surface_south, surface_east) << endl;
}


// Destructor
FGView::~FGView( void ) {
}
