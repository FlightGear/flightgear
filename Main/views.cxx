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
// (Log is kept at end of this file)


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <Debug/fg_debug.h>
#include <Flight/flight.h>
#include <Include/fg_constants.h>
#include <Math/mat3.h>
#include <Math/polar3d.h>
#include <Math/vector.h>
#include <Scenery/scenery.hxx>
#include <Time/fg_time.hxx>

#include "options.hxx"
#include "views.hxx"


// This is a record containing current view parameters
fgVIEW current_view;


// Constructor
fgVIEW::fgVIEW( void ) {
}


// Initialize a view structure
void fgVIEW::Init( void ) {
    fgPrintf( FG_VIEW, FG_INFO, "Initializing View parameters\n");

    view_offset = 0.0;
    goal_view_offset = 0.0;
}


// Update the view parameters
void fgVIEW::Update( fgFLIGHT *f ) {
    fgOPTIONS *o;
    fgPolarPoint3d p;
    MAT3vec vec, forward, v0, minus_z;
    MAT3mat R, TMP, UP, LOCAL, VIEW;
    double theta_x, theta_y, ntmp;

    o = &current_options;

    scenery.center.x = scenery.next_center.x;
    scenery.center.y = scenery.next_center.y;
    scenery.center.z = scenery.next_center.z;

    printf("win_ratio = %.2f\n", win_ratio);

    // calculate sin() and cos() of fov / 2 in X direction;
    theta_x = FG_PI_2 - (o->fov * win_ratio * DEG_TO_RAD) / 2.0;
    printf("theta_x = %.2f\n", theta_x);
    sin_fov_x = sin(theta_x);
    cos_fov_x = cos(theta_x);
    slope_x = sin_fov_x / cos_fov_x;
    printf("slope_x = %.2f\n", slope_x);

    // calculate sin() and cos() of fov / 2 in Y direction;
    theta_y = FG_PI_2 - (o->fov * DEG_TO_RAD) / 2.0;
    printf("theta_y = %.2f\n", theta_y);
    sin_fov_y = sin(theta_y);
    cos_fov_y = cos(theta_y);
    slope_y = sin_fov_y / cos_fov_y;
    printf("slope_y = %.2f\n", slope_y);

    // calculate the cartesion coords of the current lat/lon/0 elev
    p.lon = FG_Longitude;
    p.lat = FG_Lat_geocentric;
    p.radius = FG_Sea_level_radius * FEET_TO_METER;

    cur_zero_elev = fgPolarToCart3d(p);

    cur_zero_elev.x -= scenery.center.x;
    cur_zero_elev.y -= scenery.center.y;
    cur_zero_elev.z -= scenery.center.z;

    // calculate view position in current FG view coordinate system
    // p.lon & p.lat are already defined earlier
    p.radius = FG_Radius_to_vehicle * FEET_TO_METER + 1.0;

    abs_view_pos = fgPolarToCart3d(p);

    view_pos.x = abs_view_pos.x - scenery.center.x;
    view_pos.y = abs_view_pos.y - scenery.center.y;
    view_pos.z = abs_view_pos.z - scenery.center.z;

    fgPrintf( FG_VIEW, FG_DEBUG, "Absolute view pos = %.4f, %.4f, %.4f\n", 
	   abs_view_pos.x, abs_view_pos.y, abs_view_pos.z);
    fgPrintf( FG_VIEW, FG_DEBUG, "Relative view pos = %.4f, %.4f, %.4f\n", 
	   view_pos.x, view_pos.y, view_pos.z);

    // Derive the LOCAL aircraft rotation matrix (roll, pitch, yaw)
    // from FG_T_local_to_body[3][3]

    // Question: Why is the LaRCsim matrix arranged so differently
    // than the one we need???
    LOCAL[0][0] = FG_T_local_to_body_33;
    LOCAL[0][1] = -FG_T_local_to_body_32;
    LOCAL[0][2] = -FG_T_local_to_body_31;
    LOCAL[0][3] = 0.0;
    LOCAL[1][0] = -FG_T_local_to_body_23;
    LOCAL[1][1] = FG_T_local_to_body_22;
    LOCAL[1][2] = FG_T_local_to_body_21;
    LOCAL[1][3] = 0.0;
    LOCAL[2][0] = -FG_T_local_to_body_13;
    LOCAL[2][1] = FG_T_local_to_body_12;
    LOCAL[2][2] = FG_T_local_to_body_11;
    LOCAL[2][3] = 0.0;
    LOCAL[3][0] = LOCAL[3][1] = LOCAL[3][2] = LOCAL[3][3] = 0.0;
    LOCAL[3][3] = 1.0;
    // printf("LaRCsim LOCAL matrix\n");
    // MAT3print(LOCAL, stdout);

#ifdef OLD_LOCAL_TO_BODY_CODE
        // old code to calculate LOCAL matrix calculated from Phi,
        // Theta, and Psi (roll, pitch, yaw)

        MAT3_SET_VEC(vec, 0.0, 0.0, 1.0);
	MAT3rotate(R, vec, FG_Phi);
	/* printf("Roll matrix\n"); */
	/* MAT3print(R, stdout); */

	MAT3_SET_VEC(vec, 0.0, 1.0, 0.0);
	/* MAT3mult_vec(vec, vec, R); */
	MAT3rotate(TMP, vec, FG_Theta);
	/* printf("Pitch matrix\n"); */
	/* MAT3print(TMP, stdout); */
	MAT3mult(R, R, TMP);

	MAT3_SET_VEC(vec, 1.0, 0.0, 0.0);
	/* MAT3mult_vec(vec, vec, R); */
	/* MAT3rotate(TMP, vec, FG_Psi - FG_PI_2); */
	MAT3rotate(TMP, vec, -FG_Psi);
	/* printf("Yaw matrix\n");
	   MAT3print(TMP, stdout); */
	MAT3mult(LOCAL, R, TMP);
	// printf("FG derived LOCAL matrix\n");
	// MAT3print(LOCAL, stdout);
#endif // OLD_LOCAL_TO_BODY_CODE

    // Derive the local UP transformation matrix based on *geodetic*
    // coordinates
    MAT3_SET_VEC(vec, 0.0, 0.0, 1.0);
    MAT3rotate(R, vec, FG_Longitude);     // R = rotate about Z axis
    // printf("Longitude matrix\n");
    // MAT3print(R, stdout);

    MAT3_SET_VEC(vec, 0.0, 1.0, 0.0);
    MAT3mult_vec(vec, vec, R);
    MAT3rotate(TMP, vec, -FG_Latitude);  // TMP = rotate about X axis
    // printf("Latitude matrix\n");
    // MAT3print(TMP, stdout);

    MAT3mult(UP, R, TMP);
    // printf("Local up matrix\n");
    // MAT3print(UP, stdout);

    MAT3_SET_VEC(local_up, 1.0, 0.0, 0.0);
    MAT3mult_vec(local_up, local_up, UP);

    // printf( "Local Up = (%.4f, %.4f, %.4f)\n",
    //         local_up[0], local_up[1], local_up[2]);
    
    // Alternative method to Derive local up vector based on
    // *geodetic* coordinates
    // alt_up = fgPolarToCart(FG_Longitude, FG_Latitude, 1.0);
    // printf( "    Alt Up = (%.4f, %.4f, %.4f)\n", 
    //         alt_up.x, alt_up.y, alt_up.z);

    // Calculate the VIEW matrix
    MAT3mult(VIEW, LOCAL, UP);
    // printf("VIEW matrix\n");
    // MAT3print(VIEW, stdout);

    // generate the current up, forward, and fwrd-view vectors
    MAT3_SET_VEC(vec, 1.0, 0.0, 0.0);
    MAT3mult_vec(view_up, vec, VIEW);

    MAT3_SET_VEC(vec, 0.0, 0.0, 1.0);
    MAT3mult_vec(forward, vec, VIEW);
    // printf( "Forward vector is (%.2f,%.2f,%.2f)\n", forward[0], forward[1], 
    //         forward[2]);

    MAT3rotate(TMP, view_up, view_offset);
    MAT3mult_vec(view_forward, forward, TMP);

    // make a vector to the current view position
    MAT3_SET_VEC(v0, view_pos.x, view_pos.y, view_pos.z);

    // Given a vector pointing straight down (-Z), map into onto the
    // local plane representing "horizontal".  This should give us the
    // local direction for moving "south".
    MAT3_SET_VEC(minus_z, 0.0, 0.0, -1.0);
    map_vec_onto_cur_surface_plane(local_up, v0, minus_z, surface_south);
    MAT3_NORMALIZE_VEC(surface_south, ntmp);
    // printf( "Surface direction directly south %.2f %.2f %.2f\n",
    //         surface_south[0], surface_south[1], surface_south[2]);

    // now calculate the surface east vector
    MAT3rotate(TMP, view_up, FG_PI_2);
    MAT3mult_vec(surface_east, surface_south, TMP);
    // printf( "Surface direction directly east %.2f %.2f %.2f\n",
    //         surface_east[0], surface_east[1], surface_east[2]);
    // printf( "Should be close to zero = %.2f\n", 
    //         MAT3_DOT_PRODUCT(surface_south, surface_east));
}


// Update the "World to Eye" transformation matrix
// This is most useful for view frustum culling
void fgVIEW::UpdateWorldToEye( fgFLIGHT *f ) {
    MAT3mat R_Phi, R_Theta, R_Psi, R_Lat, R_Lon, T_view;
    MAT3mat TMP;
    MAT3hvec vec;

    // Roll Matrix
    MAT3_SET_HVEC(vec, 0.0, 0.0, -1.0, 1.0);
    MAT3rotate(R_Phi, vec, FG_Phi);
    // printf("Roll matrix (Phi)\n");
    // MAT3print(R_Phi, stdout);

    // Pitch Matrix
    MAT3_SET_HVEC(vec, 1.0, 0.0, 0.0, 1.0);
    MAT3rotate(R_Theta, vec, FG_Theta);
    // printf("\nPitch matrix (Theta)\n");
    // MAT3print(R_Theta, stdout);

    // Yaw Matrix
    MAT3_SET_HVEC(vec, 0.0, -1.0, 0.0, 1.0);
    MAT3rotate(R_Psi, vec, FG_Psi + FG_PI - view_offset );
    // printf("\nYaw matrix (Psi)\n");
    // MAT3print(R_Psi, stdout);

    // Latitude
    MAT3_SET_HVEC(vec, 1.0, 0.0, 0.0, 1.0);
    // R_Lat = rotate about X axis
    MAT3rotate(R_Lat, vec, FG_Latitude);
    // printf("\nLatitude matrix\n");
    // MAT3print(R_Lat, stdout);

    // Longitude
    MAT3_SET_HVEC(vec, 0.0, 0.0, 1.0, 1.0);
    // R_Lon = rotate about Z axis
    MAT3rotate(R_Lon, vec, FG_Longitude - FG_PI_2 );
    // printf("\nLongitude matrix\n");
    // MAT3print(R_Lon, stdout);

    // View position in scenery centered coordinates
    MAT3_SET_HVEC(vec, view_pos.x, view_pos.y, view_pos.z, 1.0);
    MAT3translate(T_view, vec);
    // printf("\nTranslation matrix\n");
    // MAT3print(T_view, stdout);

    // aircraft roll/pitch/yaw
    MAT3mult(TMP, R_Phi, R_Theta);
    MAT3mult(AIRCRAFT, TMP, R_Psi);
    // printf("\naircraft roll pitch yaw\n");
    // MAT3print(AIRCRAFT, stdout);

    // lon/lat
    MAT3mult(WORLD, R_Lat, R_Lon);
    // printf("\nworld\n");
    // MAT3print(WORLD, stdout);

    MAT3mult(EYE_TO_WORLD, AIRCRAFT, WORLD);
    MAT3mult(EYE_TO_WORLD, EYE_TO_WORLD, T_view);
    // printf("\nEye to world\n");
    // MAT3print(EYE_TO_WORLD, stdout);

    MAT3invert(WORLD_TO_EYE, EYE_TO_WORLD);
    // printf("\nWorld to eye\n");
    // MAT3print(WORLD_TO_EYE, stdout);

    // printf( "\nview_pos = %.2f %.2f %.2f\n", 
    //         view_pos.x, view_pos.y, view_pos.z );

    // MAT3_SET_HVEC(eye, 0.0, 0.0, 0.0, 1.0);
    // MAT3mult_vec(vec, eye, EYE_TO_WORLD);
    // printf("\neye -> world = %.2f %.2f %.2f\n", vec[0], vec[1], vec[2]);

    // MAT3_SET_HVEC(vec1, view_pos.x, view_pos.y, view_pos.z, 1.0);
    // MAT3mult_vec(vec, vec1, WORLD_TO_EYE);
    // printf( "\nabs_view_pos -> eye = %.2f %.2f %.2f\n", 
    //         vec[0], vec[1], vec[2]);
}


// Destructor
fgVIEW::~fgVIEW( void ) {
}


// $Log$
// Revision 1.9  1998/05/16 13:08:37  curt
// C++ - ified views.[ch]xx
// Shuffled some additional view parameters into the fgVIEW class.
// Changed tile-radius to tile-diameter because it is a much better
//   name.
// Added a WORLD_TO_EYE transformation to views.cxx.  This allows us
//  to transform world space to eye space for view frustum culling.
//
// Revision 1.8  1998/05/02 01:51:01  curt
// Updated polartocart conversion routine.
//
// Revision 1.7  1998/04/30 12:34:20  curt
// Added command line rendering options:
//   enable/disable fog/haze
//   specify smooth/flat shading
//   disable sky blending and just use a solid color
//   enable wireframe drawing mode
//
// Revision 1.6  1998/04/28 01:20:23  curt
// Type-ified fgTIME and fgVIEW.
// Added a command line option to disable textures.
//
// Revision 1.5  1998/04/26 05:10:04  curt
// "struct fgLIGHT" -> "fgLIGHT" because fgLIGHT is typedef'd.
//
// Revision 1.4  1998/04/25 22:04:53  curt
// Use already calculated LaRCsim values to create the roll/pitch/yaw
// transformation matrix (we call it LOCAL)
//
// Revision 1.3  1998/04/25 20:24:02  curt
// Cleaned up initialization sequence to eliminate interdependencies
// between sun position, lighting, and view position.  This creates a
// valid single pass initialization path.
//
// Revision 1.2  1998/04/24 00:49:22  curt
// Wrapped "#include <config.h>" in "#ifdef HAVE_CONFIG_H"
// Trying out some different option parsing code.
// Some code reorganization.
//
// Revision 1.1  1998/04/22 13:25:45  curt
// C++ - ifing the code.
// Starting a bit of reorganization of lighting code.
//
// Revision 1.16  1998/04/18 04:11:29  curt
// Moved fg_debug to it's own library, added zlib support.
//
// Revision 1.15  1998/02/20 00:16:24  curt
// Thursday's tweaks.
//
// Revision 1.14  1998/02/09 15:07:50  curt
// Minor tweaks.
//
// Revision 1.13  1998/02/07 15:29:45  curt
// Incorporated HUD changes and struct/typedef changes from Charlie Hotchkiss
// <chotchkiss@namg.us.anritsu.com>
//
// Revision 1.12  1998/01/29 00:50:28  curt
// Added a view record field for absolute x, y, z position.
//
// Revision 1.11  1998/01/27 00:47:58  curt
// Incorporated Paul Bleisch's <pbleisch@acm.org> new debug message
// system and commandline/config file processing code.
//
// Revision 1.10  1998/01/19 19:27:09  curt
// Merged in make system changes from Bob Kuehne <rpk@sgi.com>
// This should simplify things tremendously.
//
// Revision 1.9  1998/01/13 00:23:09  curt
// Initial changes to support loading and management of scenery tiles.  Note,
// there's still a fair amount of work left to be done.
//
// Revision 1.8  1997/12/30 22:22:33  curt
// Further integration of event manager.
//
// Revision 1.7  1997/12/30 20:47:45  curt
// Integrated new event manager with subsystem initializations.
//
// Revision 1.6  1997/12/22 04:14:32  curt
// Aligned sky with sun so dusk/dawn effects can be correct relative to the sun.
//
// Revision 1.5  1997/12/18 04:07:02  curt
// Worked on properly translating and positioning the sky dome.
//
// Revision 1.4  1997/12/17 23:13:36  curt
// Began working on rendering a sky.
//
// Revision 1.3  1997/12/15 23:54:50  curt
// Add xgl wrappers for debugging.
// Generate terrain normals on the fly.
//
// Revision 1.2  1997/12/10 22:37:48  curt
// Prepended "fg" on the name of all global structures that didn't have it yet.
// i.e. "struct WEATHER {}" became "struct fgWEATHER {}"
//
// Revision 1.1  1997/08/27 21:31:17  curt
// Initial revision.
//
