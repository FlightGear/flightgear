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
    sgSetVec3( pilot_offset, 0.0, 0.0, 0.0 );

    winWidth = current_options.get_xsize();
    winHeight = current_options.get_ysize();

    if ( ! current_options.get_panel_status() ) {
	current_view.set_win_ratio( (GLfloat) winWidth / (GLfloat) winHeight );
    } else {
	current_view.set_win_ratio( (GLfloat) winWidth / 
				    ((GLfloat) (winHeight)*0.4232) );
    }

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

    sgVec3 rollvec;
    sgSetVec3( rollvec, 0.0, 0.0, 1.0 );
    sgMat4 PHI;		// roll
    sgMakeRotMat4( PHI, f.get_Phi() * RAD_TO_DEG, rollvec );

    sgVec3 pitchvec;
    sgSetVec3( pitchvec, 0.0, 1.0, 0.0 );
    sgMat4 THETA;		// pitch
    sgMakeRotMat4( THETA, f.get_Theta() * RAD_TO_DEG, pitchvec );

    sgMat4 ROT;
    sgMultMat4( ROT, PHI, THETA );

    sgVec3 yawvec;
    sgSetVec3( yawvec, 1.0, 0.0, 0.0 );
    sgMat4 PSI;		// pitch
    sgMakeRotMat4( PSI, -f.get_Psi() * RAD_TO_DEG, yawvec );

    sgMultMat4( LOCAL, ROT, PSI );
    // cout << "LOCAL matrix" << endl;
    // print_sgMat4( LOCAL );
	
    sgMakeRotMat4( UP, 
		   f.get_Longitude() * RAD_TO_DEG,
		   0.0,
		   -f.get_Latitude() * RAD_TO_DEG );

    sgSetVec3( local_up, 1.0, 0.0, 0.0 );
    sgXformVec3( local_up, UP );
    // cout << "Local Up = " << local_up[0] << "," << local_up[1] << ","
    //      << local_up[2] << endl;
    
    // Alternative method to Derive local up vector based on
    // *geodetic* coordinates
    // alt_up = fgPolarToCart(FG_Longitude, FG_Latitude, 1.0);
    // printf( "    Alt Up = (%.4f, %.4f, %.4f)\n", 
    //         alt_up.x, alt_up.y, alt_up.z);

    sgMat4 TMP2;
    sgMultMat4( VIEWo, LOCAL, UP );
    // cout << "VIEWo matrix" << endl;
    // print_sgMat4( VIEWo );

    // generate the sg view up vector
    sgVec3 vec1;
    sgSetVec3( vec1, 1.0, 0.0, 0.0 );
    sgXformVec3( view_up, vec1, VIEWo );

    // generate the pilot offset vector in world coordinates
    sgVec3 pilot_offset_world;
    sgSetVec3( vec1, 
	       pilot_offset[2], pilot_offset[1], -pilot_offset[0] );
    sgXformVec3( pilot_offset_world, vec1, VIEWo );

    // generate the view offset matrix
    sgMakeRotMat4( VIEW_OFFSET, view_offset * RAD_TO_DEG, view_up );
    // cout << "VIEW_OFFSET matrix" << endl;
    // print_sgMat4( VIEW_OFFSET );
	
    sgMultMat4( TMP2, VIEWo, VIEW_OFFSET );
    sgMultMat4( VIEW_ROT, LARC_TO_SSG, TMP2 );
    // cout << "VIEW_ROT matrix" << endl;
    // print_sgMat4( VIEW_ROT );

    sgMakeTransMat4( TRANS, 
		     view_pos.x() + pilot_offset_world[0],
		     view_pos.y() + pilot_offset_world[1],
		     view_pos.z() + pilot_offset_world[2] );

    sgMultMat4( VIEW, VIEW_ROT, TRANS );

//!!!!!!!!!!!!!!!!!!!	
    // THIS IS THE EXPERIMENTAL VIEWING ANGLE SHIFTER
    // THE MAJORITY OF THE WORK IS DONE IN GUI.CXX
    // this in gui.cxx for now just testing
	extern float quat_mat[4][4];
	sgPreMultMat4( VIEW, quat_mat);
// !!!!!!!!!! testing	

    sgSetVec3( sgvec, 0.0, 0.0, 1.0 );
    sgXformVec3( forward, sgvec, VIEWo );
    // cout << "forward = " << forward[0] << ","
    //      << forward[1] << "," << forward[2] << endl;

    sgMakeRotMat4( TMP, view_offset * RAD_TO_DEG, view_up );
    sgXformVec3( view_forward, forward, TMP );
    // cout << "view_forward = " << view_forward[0] << ","
    //      << view_forward[1] << "," << view_forward[2] << endl;
    
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
    sgMakeRotMat4( TMP, FG_PI_2 * RAD_TO_DEG, view_up );
    // cout << "sgMat4 TMP" << endl;
    // print_sgMat4( TMP );
    sgXformVec3(surface_east, surface_south, TMP);
    // cout << "Surface direction directly east" << surface_east[0] << ","
    //      << surface_east[1] << "," << surface_east[2] << endl;
    // cout << "Should be close to zero = "
    //      << sgScalarProductVec3(surface_south, surface_east) << endl;
}


// Destructor
FGView::~FGView( void ) {
}
