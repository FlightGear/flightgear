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

#include <Aircraft/aircraft.hxx>
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


// This is a record containing current view parameters
fgVIEW current_view;


// Constructor
fgVIEW::fgVIEW( void ) {
}


// Initialize a view structure
void fgVIEW::Init( void ) {
    FG_LOG( FG_VIEW, FG_INFO, "Initializing View parameters" );

    view_offset = 0.0;
    goal_view_offset = 0.0;

    winWidth = 640;  // FG_DEFAULT_WIN_WIDTH
    winHeight = 480; // FG_DEFAULT_WIN_HEIGHT
    win_ratio = (double) winWidth / (double) winHeight;
    update_fov = true;
}


// Update the field of view parameters
void fgVIEW::UpdateFOV( fgOPTIONS *o ) {
    double fov, theta_x, theta_y;

    fov = o->get_fov();
	
    // printf("win_ratio = %.2f\n", win_ratio);
    // calculate sin() and cos() of fov / 2 in X direction;
    theta_x = (fov * win_ratio * DEG_TO_RAD) / 2.0;
    // printf("theta_x = %.2f\n", theta_x);
    sin_fov_x = sin(theta_x);
    cos_fov_x = cos(theta_x);
    slope_x =  -cos_fov_x / sin_fov_x;
    // printf("slope_x = %.2f\n", slope_x);

#if defined( USE_FAST_FOV_CLIP )
    fov_x_clip = slope_x*cos_fov_x - sin_fov_x;
#endif // defined( USE_FAST_FOV_CLIP )

    // calculate sin() and cos() of fov / 2 in Y direction;
    theta_y = (fov * DEG_TO_RAD) / 2.0;
    // printf("theta_y = %.2f\n", theta_y);
    sin_fov_y = sin(theta_y);
    cos_fov_y = cos(theta_y);
    slope_y = cos_fov_y / sin_fov_y;
    // printf("slope_y = %.2f\n", slope_y);

#if defined( USE_FAST_FOV_CLIP )
    fov_y_clip = -(slope_y*cos_fov_y + sin_fov_y);	
#endif // defined( USE_FAST_FOV_CLIP )
}


// Basically, this is a modified version of the Mesa gluLookAt()
// function that's been modified slightly so we can capture the
// result before sending it off to OpenGL land.
void fgVIEW::LookAt( GLdouble eyex, GLdouble eyey, GLdouble eyez,
		     GLdouble centerx, GLdouble centery, GLdouble centerz,
		     GLdouble upx, GLdouble upy, GLdouble upz ) {
    GLdouble *m;
    GLdouble x[3], y[3], z[3];
    GLdouble mag;

    m = current_view.MODEL_VIEW;

    /* Make rotation matrix */

    /* Z vector */
    z[0] = eyex - centerx;
    z[1] = eyey - centery;
    z[2] = eyez - centerz;
    mag = sqrt( z[0]*z[0] + z[1]*z[1] + z[2]*z[2] );
    if (mag) {  /* mpichler, 19950515 */
	z[0] /= mag;
	z[1] /= mag;
	z[2] /= mag;
    }

    /* Y vector */
    y[0] = upx;
    y[1] = upy;
    y[2] = upz;

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

#define M(row,col)  m[col*4+row]
    M(0,0) = x[0];  M(0,1) = x[1];  M(0,2) = x[2];  M(0,3) = 0.0;
    M(1,0) = y[0];  M(1,1) = y[1];  M(1,2) = y[2];  M(1,3) = 0.0;
    M(2,0) = z[0];  M(2,1) = z[1];  M(2,2) = z[2];  M(2,3) = 0.0;
    // the following is part of the original gluLookAt(), but we are
    // commenting it out because we know we are going to be doing a
    // translation below which will set these values anyways
    // M(3,0) = 0.0;   M(3,1) = 0.0;   M(3,2) = 0.0;   M(3,3) = 1.0;
#undef M

    // Translate Eye to Origin
    // replaces: glTranslated( -eyex, -eyey, -eyez );

    // this has been slightly modified from the original glTranslate()
    // code because we know that coming into this m[12] = m[13] =
    // m[14] = 0.0, and m[15] = 1.0;
    m[12] = m[0] * -eyex + m[4] * -eyey + m[8]  * -eyez /* + m[12] */;
    m[13] = m[1] * -eyex + m[5] * -eyey + m[9]  * -eyez /* + m[13] */;
    m[14] = m[2] * -eyex + m[6] * -eyey + m[10] * -eyez /* + m[14] */;
    m[15] = 1.0 /* m[3] * -eyex + m[7] * -eyey + m[11] * -eyez + m[15] */;

    // xglMultMatrixd( m );
    xglLoadMatrixd( m );
}


// Update the view volume, position, and orientation
void fgVIEW::UpdateViewParams( void ) {
    fgFLIGHT *f;
    fgLIGHT *l;

    f = current_aircraft.flight;
    l = &cur_light_params;

    UpdateViewMath(f);
    UpdateWorldToEye(f);

    // if (!o->panel_status) {
    // xglViewport( 0, (GLint)((winHeight) / 2 ) , 
    // (GLint)(winWidth), (GLint)(winHeight) / 2 );
    // Tell GL we are about to modify the projection parameters
    // xglMatrixMode(GL_PROJECTION);
    // xglLoadIdentity();
    // gluPerspective(o->fov, win_ratio / 2.0, 1.0, 100000.0);
    // } else {
    xglViewport(0, 0 , (GLint)(winWidth), (GLint)(winHeight) );
    // Tell GL we are about to modify the projection parameters
    xglMatrixMode(GL_PROJECTION);
    xglLoadIdentity();
    if ( FG_Altitude * FEET_TO_METER - scenery.cur_elev > 10.0 ) {
	gluPerspective(current_options.get_fov(), win_ratio, 10.0, 100000.0);
    } else {
	gluPerspective(current_options.get_fov(), win_ratio, 0.5, 100000.0);
	// printf("Near ground, minimizing near clip plane\n");
    }
    // }

    xglMatrixMode(GL_MODELVIEW);
    xglLoadIdentity();
    
    // set up our view volume (default)
    LookAt(view_pos.x(), view_pos.y(), view_pos.z(),
	   view_pos.x() + view_forward[0], 
	       view_pos.y() + view_forward[1], 
	       view_pos.z() + view_forward[2],
	       view_up[0], view_up[1], view_up[2]);

    // look almost straight up (testing and eclipse watching)
    /* LookAt(view_pos.x(), view_pos.y(), view_pos.z(),
	       view_pos.x() + view_up[0] + .001, 
	       view_pos.y() + view_up[1] + .001, 
	       view_pos.z() + view_up[2] + .001,
	       view_up[0], view_up[1], view_up[2]); */

    // lock view horizontally towards sun (testing)
    /* LookAt(view_pos.x(), view_pos.y(), view_pos.z(),
	       view_pos.x() + surface_to_sun[0], 
	       view_pos.y() + surface_to_sun[1], 
	       view_pos.z() + surface_to_sun[2],
	       view_up[0], view_up[1], view_up[2]); */

    // lock view horizontally towards south (testing)
    /* LookAt(view_pos.x(), view_pos.y(), view_pos.z(),
	       view_pos.x() + surface_south[0], 
	       view_pos.y() + surface_south[1], 
	       view_pos.z() + surface_south[2],
	       view_up[0], view_up[1], view_up[2]); */

    // set the sun position
    xglLightfv( GL_LIGHT0, GL_POSITION, l->sun_vec );
}


// Update the view parameters
void fgVIEW::UpdateViewMath( fgFLIGHT *f ) {
    Point3D p;
    MAT3vec vec, forward, v0, minus_z;
    MAT3mat R, TMP, UP, LOCAL, VIEW;
    double ntmp;

    if(update_fov == true) {
	// printf("Updating fov\n");
	UpdateFOV(&current_options);
	update_fov = false;
    }
		
    scenery.center = scenery.next_center;

    // printf("scenery center = %.2f %.2f %.2f\n", scenery.center.x,
    //        scenery.center.y, scenery.center.z);

    // calculate the cartesion coords of the current lat/lon/0 elev
    p = Point3D( FG_Longitude, 
		 FG_Lat_geocentric, 
		 FG_Sea_level_radius * FEET_TO_METER );

    cur_zero_elev = fgPolarToCart3d(p) - scenery.center;

    // calculate view position in current FG view coordinate system
    // p.lon & p.lat are already defined earlier, p.radius was set to
    // the sea level radius, so now we add in our altitude.
    if ( FG_Altitude * FEET_TO_METER > 
	 (scenery.cur_elev + 0.5 * METER_TO_FEET) ) {
	p.setz( p.radius() + FG_Altitude * FEET_TO_METER );
    } else {
	p.setz( p.radius() + scenery.cur_elev + 0.5 * METER_TO_FEET );
    }

    abs_view_pos = fgPolarToCart3d(p);
    view_pos = abs_view_pos - scenery.center;

    FG_LOG( FG_VIEW, FG_DEBUG, "Absolute view pos = "
	    << abs_view_pos.x() << ", " 
	    << abs_view_pos.y() << ", " 
	    << abs_view_pos.z() );
    FG_LOG( FG_VIEW, FG_DEBUG, "Relative view pos = "
	    << view_pos.x() << ", " << view_pos.y() << ", " << view_pos.z() );

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
    MAT3_SET_VEC(v0, view_pos.x(), view_pos.y(), view_pos.z());

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

    // if we have a view offset use slow way for now
    if(fabs(view_offset)>FG_EPSILON){ 
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

	// aircraft roll/pitch/yaw
	MAT3mult(TMP, R_Phi, R_Theta);
	MAT3mult(AIRCRAFT, TMP, R_Psi);

    } else { // JUST USE LOCAL_TO_BODY  NHV 5/25/98
	// hey this is even different then LOCAL[][] above ??
	 
	AIRCRAFT[0][0] = -FG_T_local_to_body_22;
	AIRCRAFT[0][1] = -FG_T_local_to_body_23;
	AIRCRAFT[0][2] = FG_T_local_to_body_21;
	AIRCRAFT[0][3] = 0.0;
	AIRCRAFT[1][0] = FG_T_local_to_body_32;
	AIRCRAFT[1][1] = FG_T_local_to_body_33;
	AIRCRAFT[1][2] = -FG_T_local_to_body_31;
	AIRCRAFT[1][3] = 0.0;
	AIRCRAFT[2][0] = FG_T_local_to_body_12;
	AIRCRAFT[2][1] = FG_T_local_to_body_13;
	AIRCRAFT[2][2] = -FG_T_local_to_body_11;
	AIRCRAFT[2][3] = 0.0;
	AIRCRAFT[3][0] = AIRCRAFT[3][1] = AIRCRAFT[3][2] = AIRCRAFT[3][3] = 0.0;
	AIRCRAFT[3][3] = 1.0;

	// ??? SOMETHING LIKE THIS SHOULD WORK    NHV
	// Rotate about LOCAL_UP  (AIRCRAFT[2][])
	// MAT3_SET_HVEC(vec, AIRCRAFT[2][0], AIRCRAFT[2][1],
	//			  AIRCRAFT[2][2], AIRCRAFT[2][3]);
	// MAT3rotate(TMP, vec, FG_PI - view_offset );
	// MAT3mult(AIRCRAFT, AIRCRAFT, TMP);
    }
    // printf("\naircraft roll pitch yaw\n");
    // MAT3print(AIRCRAFT, stdout);

    // View position in scenery centered coordinates
    MAT3_SET_HVEC(vec, view_pos.x(), view_pos.y(), view_pos.z(), 1.0);
    MAT3translate(T_view, vec);
    // printf("\nTranslation matrix\n");
    // MAT3print(T_view, stdout);

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

#ifdef THIS_IS_OLD_CODE
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
#endif THIS_IS_OLD_CODE

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


#if 0
// Reject non viewable spheres from current View Frustrum by Curt
// Olson curt@me.umn.edu and Norman Vine nhv@yahoo.com with 'gentle
// guidance' from Steve Baker sbaker@link.com
int
fgVIEW::SphereClip( const Point3D& cp, const double radius )
{
    double x1, y1;

    MAT3vec eye;	
    double *mat;
    double x, y, z;

    x = cp->x;
    y = cp->y;
    z = cp->z;
	
    mat = (double *)(WORLD_TO_EYE);
	
    eye[2] =  x*mat[2] + y*mat[6] + z*mat[10] + mat[14];
	
    // Check near and far clip plane
    if( ( eye[2] > radius ) ||
	( eye[2] + radius + current_weather.visibility < 0) )
	// ( eye[2] + radius + far_plane < 0) )
    {
	return 1;
    }
	
    // check right and left clip plane (from eye perspective)
    x1 = radius * fov_x_clip;
    eye[0] = (x*mat[0] + y*mat[4] + z*mat[8] + mat[12]) * slope_x;
    if( (eye[2] > -(eye[0]+x1)) || (eye[2] > (eye[0]-x1)) ) {
	return(1);
    }
	
    // check bottom and top clip plane (from eye perspective)
    y1 = radius * fov_y_clip;
    eye[1] = (x*mat[1] + y*mat[5] + z*mat[9] + mat[13]) * slope_y; 
    if( (eye[2] > -(eye[1]+y1)) || (eye[2] > (eye[1]-y1)) ) {
	return 1;
    }

    return 0;
}
#endif


// Destructor
fgVIEW::~fgVIEW( void ) {
}


// $Log$
// Revision 1.25  1998/11/06 21:18:15  curt
// Converted to new logstream debugging facility.  This allows release
// builds with no messages at all (and no performance impact) by using
// the -DFG_NDEBUG flag.
//
// Revision 1.24  1998/10/18 01:17:19  curt
// Point3D tweaks.
//
// Revision 1.23  1998/10/17 01:34:26  curt
// C++ ifying ...
//
// Revision 1.22  1998/10/16 00:54:03  curt
// Converted to Point3D class.
//
// Revision 1.21  1998/09/17 18:35:33  curt
// Added F8 to toggle fog and F9 to toggle texturing.
//
// Revision 1.20  1998/09/08 15:04:35  curt
// Optimizations by Norman Vine.
//
// Revision 1.19  1998/08/20 20:32:34  curt
// Reshuffled some of the code in and around views.[ch]xx
//
// Revision 1.18  1998/07/24 21:57:02  curt
// Set near clip plane to 0.5 meters when close to the ground.  Also, let the view get a bit closer to the ground before hitting the hard limit.
//
// Revision 1.17  1998/07/24 21:39:12  curt
// Debugging output tweaks.
// Cast glGetString to (char *) to avoid compiler errors.
// Optimizations to fgGluLookAt() by Norman Vine.
//
// Revision 1.16  1998/07/13 21:01:41  curt
// Wrote access functions for current fgOPTIONS.
//
// Revision 1.15  1998/07/12 03:14:43  curt
// Added ground collision detection.
// Did some serious horsing around to be able to "hug" the ground properly
//   and still be able to take off.
// Set the near clip plane to 1.0 meters when less than 10 meters above the
//   ground.
// Did some serious horsing around getting the initial airplane position to be
//   correct based on rendered terrain elevation.
// Added a little cheat/hack that will prevent the view position from ever
//   dropping below the terrain, even when the flight model doesn't quite
//   put you as high as you'd like.
//
// Revision 1.14  1998/07/08 14:45:08  curt
// polar3d.h renamed to polar3d.hxx
// vector.h renamed to vector.hxx
// updated audio support so it waits to create audio classes (and tie up
//   /dev/dsp) until the mpg123 player is finished.
//
// Revision 1.13  1998/07/04 00:52:27  curt
// Add my own version of gluLookAt() (which is nearly identical to the
// Mesa/glu version.)  But, by calculating the Model View matrix our selves
// we can save this matrix without having to read it back in from the video
// card.  This hopefully allows us to save a few cpu cycles when rendering
// out the fragments because we can just use glLoadMatrixd() with the
// precalculated matrix for each tile rather than doing a push(), translate(),
// pop() for every fragment.
//
// Panel status defaults to off for now until it gets a bit more developed.
//
// Extract OpenGL driver info on initialization.
//
// Revision 1.12  1998/06/03 00:47:15  curt
// Updated to compile in audio support if OSS available.
// Updated for new version of Steve's audio library.
// STL includes don't use .h
// Small view optimizations.
//
// Revision 1.11  1998/05/27 02:24:05  curt
// View optimizations by Norman Vine.
//
// Revision 1.10  1998/05/17 16:59:03  curt
// First pass at view frustum culling now operational.
//
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
