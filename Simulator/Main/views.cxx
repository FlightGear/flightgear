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


// Define following to extract various vectors directly
// from matrices we have allready computed
// rather then performing 'textbook algebra' to rederive them
// Norman Vine -- nhv@yahoo.com
// #define FG_VIEW_INLINE_OPTIMIZATIONS

// temporary (hopefully) hack
static int panel_hist = 0;


// specify code paths ... these are done as variable rather than
// #define's because down the road we may want to choose between them
// on the fly for different flight models ... this way magic carpet
// and external modes wouldn't need to recreate the LaRCsim matrices
// themselves.

static const bool use_larcsim_local_to_body = false;


// This is a record containing current view parameters
FGView current_view;


// Constructor
FGView::FGView( void ) {
    MAT3identity(WORLD);
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

    force_update_fov_math();
}


// Update the field of view coefficients
void FGView::UpdateFOV( const fgOPTIONS& o ) {
    double fov, theta_x, theta_y;

    fov = o.get_fov();
	
    // printf("win_ratio = %.2f\n", win_ratio);
    // calculate sin() and cos() of fov / 2 in X direction;
    theta_x = (fov * win_ratio * DEG_TO_RAD) / 2.0;
    // printf("theta_x = %.2f\n", theta_x);
    sin_fov_x = sin(theta_x);
    cos_fov_x = cos(theta_x);
    slope_x =  -cos_fov_x / sin_fov_x;
    // printf("slope_x = %.2f\n", slope_x);

    // fov_x_clip and fov_y_clip convoluted algebraic simplification
    // see code executed in tilemgr.cxx when USE_FAST_FOV_CLIP not
    // defined Norman Vine -- nhv@yahoo.com
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
void FGView::LookAt( GLdouble eyex, GLdouble eyey, GLdouble eyez,
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
void FGView::UpdateViewParams( void ) {
    FGInterface *f = current_aircraft.fdm_state;

    UpdateViewMath(f);
    UpdateWorldToEye(f);
    
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

    // Tell GL we are about to modify the projection parameters
    xglMatrixMode(GL_PROJECTION);
    xglLoadIdentity();
    if ( f->get_Altitude() * FEET_TO_METER - scenery.cur_elev > 10.0 ) {
	gluPerspective(current_options.get_fov(), win_ratio, 10.0, 100000.0);
    } else {
	gluPerspective(current_options.get_fov(), win_ratio, 0.5, 100000.0);
	// printf("Near ground, minimizing near clip plane\n");
    }
    // }

    xglMatrixMode(GL_MODELVIEW);
    xglLoadIdentity();
    
    // set up our view volume (default)
#if !defined(FG_VIEW_INLINE_OPTIMIZATIONS)
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

#else // defined(FG_VIEW_INLINE_OPTIMIZATIONS)
    //void FGView::LookAt( GLdouble eyex, GLdouble eyey, GLdouble eyez,
    //		     GLdouble centerx, GLdouble centery, GLdouble centerz,
    //		     GLdouble upx, GLdouble upy, GLdouble upz )
    {
	GLdouble *m;
	GLdouble x[3], y[3], z[3];
	//    GLdouble mag;

	m = current_view.MODEL_VIEW;

	/* Make rotation matrix */

	/* Z vector */
	z[0] = -view_forward[0]; //eyex - centerx;
	z[1] = -view_forward[1]; //eyey - centery;
	z[2] = -view_forward[2]; //eyez - centerz;
	
	// In our case this is a unit vector  NHV
	
	//    mag = sqrt( z[0]*z[0] + z[1]*z[1] + z[2]*z[2] );
	//    if (mag) {  /* mpichler, 19950515 */
	//		mag = 1.0/mag;
	//		printf("mag(%f)  ", mag);
	//	z[0] *= mag;
	//	z[1] *= mag;
	//	z[2] *= mag;
	//    }

	/* Y vector */
	y[0] = view_up[0]; //upx;
	y[1] = view_up[1]; //upy;
	y[2] = view_up[2]; //upz;

	/* X vector = Y cross Z */
	x[0] =  y[1]*z[2] - y[2]*z[1];
	x[1] = -y[0]*z[2] + y[2]*z[0];
	x[2] =  y[0]*z[1] - y[1]*z[0];

	//	printf(" %f %f %f  ", y[0], y[1], y[2]);
    
	/* Recompute Y = Z cross X */
	//    y[0] =  z[1]*x[2] - z[2]*x[1];
	//    y[1] = -z[0]*x[2] + z[2]*x[0];
	//    y[2] =  z[0]*x[1] - z[1]*x[0];

	//	printf(" %f %f %f\n", y[0], y[1], y[2]);
	
	// In our case these are unit vectors  NHV

	/* mpichler, 19950515 */
	/* cross product gives area of parallelogram, which is < 1.0 for
	 * non-perpendicular unit-length vectors; so normalize x, y here
	 */

	//    mag = sqrt( x[0]*x[0] + x[1]*x[1] + x[2]*x[2] );
	//    if (mag) {
	//		mag = 1.0/mag;
	//		printf("mag2(%f) ", mag);
	//	x[0] *= mag;
	//	x[1] *= mag;
	//	x[2] *= mag;
	//    }

	//    mag = sqrt( y[0]*y[0] + y[1]*y[1] + y[2]*y[2] );
	//    if (mag) {
	//		mag = 1.0/mag;
	//		printf("mag3(%f)\n", mag);
	//	y[0] *= mag;
	//	y[1] *= mag;
	//	y[2] *= mag;
	//    }

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
	m[12] = m[0] * -view_pos.x() + m[4] * -view_pos.y() + m[8]  * -view_pos.z() /* + m[12] */;
	m[13] = m[1] * -view_pos.x() + m[5] * -view_pos.y() + m[9]  * -view_pos.z() /* + m[13] */;
	m[14] = m[2] * -view_pos.x() + m[6] * -view_pos.y() + m[10] * -view_pos.z() /* + m[14] */;
	m[15] = 1.0 /* m[3] * -view_pos.x() + m[7] * -view_pos.y() + m[11] * -view_pos.z() + m[15] */;

	// xglMultMatrixd( m );
	xglLoadMatrixd( m );
    }
#endif // FG_VIEW_INLINE_OPTIMIZATIONS
	

    panel_hist = current_options.get_panel_status();
}


void getRotMatrix(double* out, MAT3vec vec, double radians)
{
    /* This function contributed by Erich Boleyn (erich@uruk.org) */
    /* This function used from the Mesa OpenGL code (matrix.c)  */
    double s, c; // mag,
    double vx, vy, vz, xy, yz, zx, xs, ys, zs, one_c; //, xx, yy, zz
  
    MAT3identity(out);
    s = sin(radians);
    c = cos(radians);
  
    //  mag = getMagnitude();
  
    vx = vec[0];
    vy = vec[1];
    vz = vec[2];
  
#define M(row,col)  out[row*4 + col]
  
    /*
     *     Arbitrary axis rotation matrix.
     *
     *  This is composed of 5 matrices, Rz, Ry, T, Ry', Rz', multiplied
     *  like so:  Rz * Ry * T * Ry' * Rz'.  T is the final rotation
     *  (which is about the X-axis), and the two composite transforms
     *  Ry' * Rz' and Rz * Ry are (respectively) the rotations necessary
     *  from the arbitrary axis to the X-axis then back.  They are
     *  all elementary rotations.
     *
     *  Rz' is a rotation about the Z-axis, to bring the axis vector
     *  into the x-z plane.  Then Ry' is applied, rotating about the
     *  Y-axis to bring the axis vector parallel with the X-axis.  The
     *  rotation about the X-axis is then performed.  Ry and Rz are
     *  simply the respective inverse transforms to bring the arbitrary
     *  axis back to it's original orientation.  The first transforms
     *  Rz' and Ry' are considered inverses, since the data from the
     *  arbitrary axis gives you info on how to get to it, not how
     *  to get away from it, and an inverse must be applied.
     *
     *  The basic calculation used is to recognize that the arbitrary
     *  axis vector (x, y, z), since it is of unit length, actually
     *  represents the sines and cosines of the angles to rotate the
     *  X-axis to the same orientation, with theta being the angle about
     *  Z and phi the angle about Y (in the order described above)
     *  as follows:
     *
     *  cos ( theta ) = x / sqrt ( 1 - z^2 )
     *  sin ( theta ) = y / sqrt ( 1 - z^2 )
     *
     *  cos ( phi ) = sqrt ( 1 - z^2 )
     *  sin ( phi ) = z
     *
     *  Note that cos ( phi ) can further be inserted to the above
     *  formulas:
     *
     *  cos ( theta ) = x / cos ( phi )
     *  sin ( theta ) = y / cos ( phi )
     *
     *  ...etc.  Because of those relations and the standard trigonometric
     *  relations, it is pssible to reduce the transforms down to what
     *  is used below.  It may be that any primary axis chosen will give the
     *  same results (modulo a sign convention) using thie method.
     *
     *  Particularly nice is to notice that all divisions that might
     *  have caused trouble when parallel to certain planes or
     *  axis go away with care paid to reducing the expressions.
     *  After checking, it does perform correctly under all cases, since
     *  in all the cases of division where the denominator would have
     *  been zero, the numerator would have been zero as well, giving
     *  the expected result.
     */
    
    one_c = 1.0F - c;
    
    //  xx = vx * vx;
    //  yy = vy * vy;
    //  zz = vz * vz;
  
    //  xy = vx * vy;
    //  yz = vy * vz;
    //  zx = vz * vx;
  
  
    M(0,0) = (one_c * vx * vx) + c;  
    xs = vx * s;
    yz = vy * vz * one_c;
    M(1,2) = yz + xs;
    M(2,1) = yz - xs;

    M(1,1) = (one_c * vy * vy) + c;
    ys = vy * s;
    zx = vz * vx * one_c;
    M(0,2) = zx - ys;
    M(2,0) = zx + ys;
  
    M(2,2) = (one_c * vz *vz) + c;
    zs = vz * s;
    xy = vx * vy * one_c;
    M(0,1) = xy + zs;
    M(1,0) = xy - zs;
  
    //  M(0,0) = (one_c * xx) + c;
    //  M(1,0) = (one_c * xy) - zs;
    //  M(2,0) = (one_c * zx) + ys;
  
    //  M(0,1) = (one_c * xy) + zs;
    //  M(1,1) = (one_c * yy) + c;
    //  M(2,1) = (one_c * yz) - xs;
  
    //  M(0,2) = (one_c * zx) - ys;
    //  M(1,2) = (one_c * yz) + xs;
    //  M(2,2) = (one_c * zz) + c;
  
#undef M
}


// Update the view parameters
void FGView::UpdateViewMath( FGInterface *f ) {
    Point3D p;
    MAT3vec vec, forward, v0, minus_z;
    MAT3mat R, TMP, UP, LOCAL, VIEW;
    double ntmp;

    if ( update_fov ) {
	// printf("Updating fov\n");
	UpdateFOV( current_options );
	update_fov = false;
    }
		
    scenery.center = scenery.next_center;

#if !defined(FG_VIEW_INLINE_OPTIMIZATIONS)
    // printf("scenery center = %.2f %.2f %.2f\n", scenery.center.x,
    //        scenery.center.y, scenery.center.z);

    // calculate the cartesion coords of the current lat/lon/0 elev
    p = Point3D( f->get_Longitude(), 
		 f->get_Lat_geocentric(), 
		 f->get_Sea_level_radius() * FEET_TO_METER );

    cur_zero_elev = fgPolarToCart3d(p) - scenery.center;

    // calculate view position in current FG view coordinate system
    // p.lon & p.lat are already defined earlier, p.radius was set to
    // the sea level radius, so now we add in our altitude.
    if ( f->get_Altitude() * FEET_TO_METER > 
	 (scenery.cur_elev + 0.5 * METER_TO_FEET) ) {
	p.setz( p.radius() + f->get_Altitude() * FEET_TO_METER );
    } else {
	p.setz( p.radius() + scenery.cur_elev + 0.5 * METER_TO_FEET );
    }

    abs_view_pos = fgPolarToCart3d(p);
	
#else // FG_VIEW_INLINE_OPTIMIZATIONS
	
    double tmp_radius = f->get_Sea_level_radius() * FEET_TO_METER;
    double tmp = f->get_cos_lat_geocentric() * tmp_radius;
	
    cur_zero_elev.setx(f->get_cos_longitude()*tmp - scenery.center.x());
    cur_zero_elev.sety(f->get_sin_longitude()*tmp - scenery.center.y());
    cur_zero_elev.setz(f->get_sin_lat_geocentric()*tmp_radius - scenery.center.z());

    // calculate view position in current FG view coordinate system
    // p.lon & p.lat are already defined earlier, p.radius was set to
    // the sea level radius, so now we add in our altitude.
    if ( f->get_Altitude() * FEET_TO_METER > 
	 (scenery.cur_elev + 0.5 * METER_TO_FEET) ) {
	tmp_radius += f->get_Altitude() * FEET_TO_METER;
    } else {
	tmp_radius += scenery.cur_elev + 0.5 * METER_TO_FEET ;
    }
    tmp = f->get_cos_lat_geocentric() * tmp_radius;
    abs_view_pos.setx(f->get_cos_longitude()*tmp);
    abs_view_pos.sety(f->get_sin_longitude()*tmp);
    abs_view_pos.setz(f->get_sin_lat_geocentric()*tmp_radius);
	
#endif // FG_VIEW_INLINE_OPTIMIZATIONS
	
    view_pos = abs_view_pos - scenery.center;

    FG_LOG( FG_VIEW, FG_DEBUG, "Polar view pos = " << p );
    FG_LOG( FG_VIEW, FG_DEBUG, "Absolute view pos = " << abs_view_pos );
    FG_LOG( FG_VIEW, FG_DEBUG, "Relative view pos = " << view_pos );

    // Derive the LOCAL aircraft rotation matrix (roll, pitch, yaw)
    // from FG_T_local_to_body[3][3]

    if ( use_larcsim_local_to_body ) {

	// Question: Why is the LaRCsim matrix arranged so differently
	// than the one we need???

	// Answer (I think): The LaRCsim matrix is generated in a
	// different reference frame than we've set up for our world

	LOCAL[0][0] = f->get_T_local_to_body_33();
	LOCAL[0][1] = -f->get_T_local_to_body_32();
	LOCAL[0][2] = -f->get_T_local_to_body_31();
	LOCAL[0][3] = 0.0;
	LOCAL[1][0] = -f->get_T_local_to_body_23();
	LOCAL[1][1] = f->get_T_local_to_body_22();
	LOCAL[1][2] = f->get_T_local_to_body_21();
	LOCAL[1][3] = 0.0;
	LOCAL[2][0] = -f->get_T_local_to_body_13();
	LOCAL[2][1] = f->get_T_local_to_body_12();
	LOCAL[2][2] = f->get_T_local_to_body_11();
	LOCAL[2][3] = 0.0;
	LOCAL[3][0] = LOCAL[3][1] = LOCAL[3][2] = LOCAL[3][3] = 0.0;
	LOCAL[3][3] = 1.0;

	// printf("LaRCsim LOCAL matrix\n");
	// MAT3print(LOCAL, stdout);

    } else {

	// code to calculate LOCAL matrix calculated from Phi, Theta, and
	// Psi (roll, pitch, yaw) in case we aren't running LaRCsim as our
	// flight model

	MAT3_SET_VEC(vec, 0.0, 0.0, 1.0);
	MAT3rotate(R, vec, f->get_Phi());
	/* printf("Roll matrix\n"); */
	/* MAT3print(R, stdout); */

	MAT3_SET_VEC(vec, 0.0, 1.0, 0.0);
	/* MAT3mult_vec(vec, vec, R); */
	MAT3rotate(TMP, vec, f->get_Theta());
	/* printf("Pitch matrix\n"); */
	/* MAT3print(TMP, stdout); */
	MAT3mult(R, R, TMP);

	MAT3_SET_VEC(vec, 1.0, 0.0, 0.0);
	/* MAT3mult_vec(vec, vec, R); */
	/* MAT3rotate(TMP, vec, FG_Psi - FG_PI_2); */
	MAT3rotate(TMP, vec, -f->get_Psi());
	/* printf("Yaw matrix\n");
	   MAT3print(TMP, stdout); */
	MAT3mult(LOCAL, R, TMP);
	// printf("FG derived LOCAL matrix\n");
	// MAT3print(LOCAL, stdout);

    } // if ( use_larcsim_local_to_body ) 

#if !defined(FG_VIEW_INLINE_OPTIMIZATIONS)
	
    // Derive the local UP transformation matrix based on *geodetic*
    // coordinates
    MAT3_SET_VEC(vec, 0.0, 0.0, 1.0);
    MAT3rotate(R, vec, f->get_Longitude());     // R = rotate about Z axis
    // printf("Longitude matrix\n");
    // MAT3print(R, stdout);

    MAT3_SET_VEC(vec, 0.0, 1.0, 0.0);
    MAT3mult_vec(vec, vec, R);
    MAT3rotate(TMP, vec, -f->get_Latitude());  // TMP = rotate about X axis
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
	
#else // FG_VIEW_INLINE_OPTIMIZATIONS
	 
    //	// Build spherical to cartesian transform matrix directly
    double cos_lat = f->get_cos_latitude(); // cos(-f->get_Latitude());
    double sin_lat = -f->get_sin_latitude(); // sin(-f->get_Latitude());
    double cos_lon = f->get_cos_longitude(); //cos(f->get_Longitude());
    double sin_lon = f->get_sin_longitude(); //sin(f->get_Longitude());

    double *mat = (double *)UP;
	
    mat[0] =  cos_lat*cos_lon;
    mat[1] =  cos_lat*sin_lon;
    mat[2] = -sin_lat;
    mat[3] =  0.0;
    mat[4] =  -sin_lon;
    mat[5] =  cos_lon;
    mat[6] =  0.0;
    mat[7] =  0.0;
    mat[8]  =  sin_lat*cos_lon;
    mat[9]  =  sin_lat*sin_lon;
    mat[10] =  cos_lat;
    mat[11] =  mat[12] = mat[13] = mat[14] = 0.0;
    mat[15] =  1.0;

    MAT3mult(VIEW, LOCAL, UP);
	
    // THESE COULD JUST BE POINTERS !!!
    MAT3_SET_VEC(local_up, mat[0],     mat[1],     mat[2]);
    MAT3_SET_VEC(view_up,  VIEW[0][0], VIEW[0][1], VIEW[0][2]);
    MAT3_SET_VEC(forward,  VIEW[2][0], VIEW[2][1], VIEW[2][2]);

    getRotMatrix((double *)TMP, view_up, view_offset);
    MAT3mult_vec(view_forward, forward, TMP);

    // make a vector to the current view position
    MAT3_SET_VEC(v0, view_pos.x(), view_pos.y(), view_pos.z());

    // Given a vector pointing straight down (-Z), map into onto the
    // local plane representing "horizontal".  This should give us the
    // local direction for moving "south".
    MAT3_SET_VEC(minus_z, 0.0, 0.0, -1.0);
    map_vec_onto_cur_surface_plane(local_up, v0, minus_z, surface_south);

    MAT3_NORMALIZE_VEC(surface_south, ntmp);
    // printf( "Surface direction directly south %.6f %.6f %.6f\n",
    //         surface_south[0], surface_south[1], surface_south[2]);

    // now calculate the surface east vector
    getRotMatrix((double *)TMP, view_up, FG_PI_2);
    MAT3mult_vec(surface_east, surface_south, TMP);
    // printf( "Surface direction directly east %.6f %.6f %.6f\n",
    //         surface_east[0], surface_east[1], surface_east[2]);
    // printf( "Should be close to zero = %.6f\n", 
    //         MAT3_DOT_PRODUCT(surface_south, surface_east));
#endif // !defined(FG_VIEW_INLINE_OPTIMIZATIONS)
}


// Update the "World to Eye" transformation matrix
// This is most useful for view frustum culling
void FGView::UpdateWorldToEye( FGInterface *f ) {
    MAT3mat R_Phi, R_Theta, R_Psi, R_Lat, R_Lon, T_view;
    MAT3mat TMP;
    MAT3hvec vec;

    if ( use_larcsim_local_to_body ) {

	// Question: hey this is even different then LOCAL[][] above??
	// Answer: yet another coordinate system, this time the
	// coordinate system in which we do our view frustum culling.

	AIRCRAFT[0][0] = -f->get_T_local_to_body_22();
	AIRCRAFT[0][1] = -f->get_T_local_to_body_23();
	AIRCRAFT[0][2] = f->get_T_local_to_body_21();
	AIRCRAFT[0][3] = 0.0;
	AIRCRAFT[1][0] = f->get_T_local_to_body_32();
	AIRCRAFT[1][1] = f->get_T_local_to_body_33();
	AIRCRAFT[1][2] = -f->get_T_local_to_body_31();
	AIRCRAFT[1][3] = 0.0;
	AIRCRAFT[2][0] = f->get_T_local_to_body_12();
	AIRCRAFT[2][1] = f->get_T_local_to_body_13();
	AIRCRAFT[2][2] = -f->get_T_local_to_body_11();
	AIRCRAFT[2][3] = 0.0;
	AIRCRAFT[3][0] = AIRCRAFT[3][1] = AIRCRAFT[3][2] = AIRCRAFT[3][3] = 0.0;
	AIRCRAFT[3][3] = 1.0;

    } else {

	// Roll Matrix
	MAT3_SET_HVEC(vec, 0.0, 0.0, -1.0, 1.0);
	MAT3rotate(R_Phi, vec, f->get_Phi());
	// printf("Roll matrix (Phi)\n");
	// MAT3print(R_Phi, stdout);

	// Pitch Matrix
	MAT3_SET_HVEC(vec, 1.0, 0.0, 0.0, 1.0);
	MAT3rotate(R_Theta, vec, f->get_Theta());
	// printf("\nPitch matrix (Theta)\n");
	// MAT3print(R_Theta, stdout);

	// Yaw Matrix
	MAT3_SET_HVEC(vec, 0.0, -1.0, 0.0, 1.0);
	MAT3rotate(R_Psi, vec, f->get_Psi() + FG_PI /* - view_offset */ );
	// MAT3rotate(R_Psi, vec, f->get_Psi() + FG_PI - view_offset );
	// printf("\nYaw matrix (Psi)\n");
	// MAT3print(R_Psi, stdout);

	// aircraft roll/pitch/yaw
	MAT3mult(TMP, R_Phi, R_Theta);
	MAT3mult(AIRCRAFT, TMP, R_Psi);

    } // if ( use_larcsim_local_to_body )

#if !defined(FG_VIEW_INLINE_OPTIMIZATIONS)
	
    // printf("AIRCRAFT matrix\n");
    // MAT3print(AIRCRAFT, stdout);

    // View rotation matrix relative to current aircraft orientation
    MAT3_SET_HVEC(vec, 0.0, -1.0, 0.0, 1.0);
    MAT3mult_vec(vec, vec, AIRCRAFT);
    // printf("aircraft up vector = %.2f %.2f %.2f\n", 
    //        vec[0], vec[1], vec[2]);
    MAT3rotate(TMP, vec, -view_offset );
    MAT3mult(VIEW_OFFSET, AIRCRAFT, TMP);
    // printf("VIEW_OFFSET matrix\n");
    // MAT3print(VIEW_OFFSET, stdout);

    // View position in scenery centered coordinates
    MAT3_SET_HVEC(vec, view_pos.x(), view_pos.y(), view_pos.z(), 1.0);
    MAT3translate(T_view, vec);
    // printf("\nTranslation matrix\n");
    // MAT3print(T_view, stdout);

    // Latitude
    MAT3_SET_HVEC(vec, 1.0, 0.0, 0.0, 1.0);
    // R_Lat = rotate about X axis
    MAT3rotate(R_Lat, vec, f->get_Latitude());
    // printf("\nLatitude matrix\n");
    // MAT3print(R_Lat, stdout);

    // Longitude
    MAT3_SET_HVEC(vec, 0.0, 0.0, 1.0, 1.0);
    // R_Lon = rotate about Z axis
    MAT3rotate(R_Lon, vec, f->get_Longitude() - FG_PI_2 );
    // printf("\nLongitude matrix\n");
    // MAT3print(R_Lon, stdout);

    // lon/lat
    MAT3mult(WORLD, R_Lat, R_Lon);
    // printf("\nworld\n");
    // MAT3print(WORLD, stdout);

    MAT3mult(EYE_TO_WORLD, VIEW_OFFSET, WORLD);
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
#else  // FG_VIEW_INLINE_OPTIMIZATIONS
	
    MAT3_SET_HVEC(vec, -AIRCRAFT[1][0], -AIRCRAFT[1][1], -AIRCRAFT[1][2], -AIRCRAFT[1][3]);
    getRotMatrix((double *)TMP, vec, -view_offset );
    MAT3mult(VIEW_OFFSET, AIRCRAFT, TMP);
    // MAT3print_formatted(VIEW_OFFSET, stdout, "VIEW_OFFSET matrix:\n",
    //					 NULL, "%#8.6f  ", "\n");

    // Build spherical to cartesian transform matrix directly
    double *mat = (double *)WORLD; //T_view; //WORLD;
    double cos_lat = f->get_cos_latitude(); //cos(f->get_Latitude());
    double sin_lat = f->get_sin_latitude(); //sin(f->get_Latitude());
    // using trig identities  this:
    //	mat[0]  =  cos(f->get_Longitude() - FG_PI_2);//cos_lon;
    //	mat[1]  =  sin(f->get_Longitude() - FG_PI_2);//sin_lon;
    // becomes this: :-)
    mat[0]  =  f->get_sin_longitude(); //cos_lon;
    mat[1]  = -f->get_cos_longitude(); //sin_lon;
    mat[4]  = -cos_lat*mat[1]; //mat[1]=sin_lon;
    mat[5]  =  cos_lat*mat[0]; //mat[0]=cos_lon;
    mat[6]  =  sin_lat;
    mat[8]  =  sin_lat*mat[1]; //mat[1]=sin_lon;
    mat[9]  = -sin_lat*mat[0]; //mat[0]=cos_lon;
    mat[10] =  cos_lat;

    // BUILD EYE_TO_WORLD = AIRCRAFT * WORLD
    // and WORLD_TO_EYE = Inverse( EYE_TO_WORLD) concurrently
    // by Transposing the 3x3 rotation sub-matrix
    WORLD_TO_EYE[0][0] = EYE_TO_WORLD[0][0] =
	VIEW_OFFSET[0][0]*mat[0] + VIEW_OFFSET[0][1]*mat[4] + VIEW_OFFSET[0][2]*mat[8];
	
    WORLD_TO_EYE[1][0] = EYE_TO_WORLD[0][1] =
	VIEW_OFFSET[0][0]*mat[1] + VIEW_OFFSET[0][1]*mat[5] + VIEW_OFFSET[0][2]*mat[9];
	
    WORLD_TO_EYE[2][0] = EYE_TO_WORLD[0][2] =
	VIEW_OFFSET[0][1]*mat[6] + VIEW_OFFSET[0][2]*mat[10];
	
    WORLD_TO_EYE[0][1] = EYE_TO_WORLD[1][0] =
	VIEW_OFFSET[1][0]*mat[0] + VIEW_OFFSET[1][1]*mat[4] + VIEW_OFFSET[1][2]*mat[8];
	
    WORLD_TO_EYE[1][1] = EYE_TO_WORLD[1][1] =
	VIEW_OFFSET[1][0]*mat[1] + VIEW_OFFSET[1][1]*mat[5] + VIEW_OFFSET[1][2]*mat[9];
	
    WORLD_TO_EYE[2][1] = EYE_TO_WORLD[1][2] =
	VIEW_OFFSET[1][1]*mat[6] + VIEW_OFFSET[1][2]*mat[10];
	
    WORLD_TO_EYE[0][2] = EYE_TO_WORLD[2][0] =
	VIEW_OFFSET[2][0]*mat[0] + VIEW_OFFSET[2][1]*mat[4] + VIEW_OFFSET[2][2]*mat[8];
	
    WORLD_TO_EYE[1][2] = EYE_TO_WORLD[2][1] =
	VIEW_OFFSET[2][0]*mat[1] + VIEW_OFFSET[2][1]*mat[5] + VIEW_OFFSET[2][2]*mat[9];
	
    WORLD_TO_EYE[2][2] = EYE_TO_WORLD[2][2] =
	VIEW_OFFSET[2][1]*mat[6] + VIEW_OFFSET[2][2]*mat[10];
	
    // TRANSLATE TO VIEW POSITION
    EYE_TO_WORLD[3][0] = view_pos.x();
    EYE_TO_WORLD[3][1] = view_pos.y();
    EYE_TO_WORLD[3][2] = view_pos.z();
	
    // FILL 0 ENTRIES
    WORLD_TO_EYE[0][3] = WORLD_TO_EYE[1][3] = WORLD_TO_EYE[2][3] = 
	EYE_TO_WORLD[0][3] = EYE_TO_WORLD[1][3] = EYE_TO_WORLD[2][3] = 0.0;

    // FILL UNITY ENTRIES
    WORLD_TO_EYE[3][3] = EYE_TO_WORLD[3][3] = 1.0;
	
    /* MAKE THE INVERTED TRANSLATIONS */
    mat = (double *)EYE_TO_WORLD;
    WORLD_TO_EYE[3][0] = -mat[12]*mat[0]
	-mat[13]*mat[1]
	-mat[14]*mat[2];
	
    WORLD_TO_EYE[3][1] = -mat[12]*mat[4]
	-mat[13]*mat[5]
	-mat[14]*mat[6];
	
    WORLD_TO_EYE[3][2] = -mat[12]*mat[8]
	-mat[13]*mat[9]
	-mat[14]*mat[10];
	
    // MAT3print_formatted(EYE_TO_WORLD, stdout, "EYE_TO_WORLD matrix:\n",
    //					 NULL, "%#8.6f  ", "\n");

    // MAT3print_formatted(WORLD_TO_EYE, stdout, "WORLD_TO_EYE matrix:\n",
    //					 NULL, "%#8.6f  ", "\n");

#endif // defined(FG_VIEW_INLINE_OPTIMIZATIONS)
}


#if 0
// Reject non viewable spheres from current View Frustrum by Curt
// Olson curt@me.umn.edu and Norman Vine nhv@yahoo.com with 'gentle
// guidance' from Steve Baker sbaker@link.com
int
FGView::SphereClip( const Point3D& cp, const double radius )
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
FGView::~FGView( void ) {
}


// $Log$
// Revision 1.1  1999/04/05 21:32:47  curt
// Initial revision
//
// Revision 1.35  1999/04/03 04:21:04  curt
// Integration of Steve's plib conglomeration.
// Optimizations (tm) by Norman Vine.
//
// Revision 1.34  1999/03/08 21:56:41  curt
// Added panel changes sent in by Friedemann.
// Added a splash screen randomization since we have several nice splash screens.
//
// Revision 1.33  1999/02/05 21:29:14  curt
// Modifications to incorporate Jon S. Berndts flight model code.
//
// Revision 1.32  1999/01/07 20:25:12  curt
// Updated struct fgGENERAL to class FGGeneral.
//
// Revision 1.31  1998/12/11 20:26:28  curt
// Fixed view frustum culling accuracy bug so we can look out the sides and
// back without tri-stripes dropping out.
//
// Revision 1.30  1998/12/09 18:50:28  curt
// Converted "class fgVIEW" to "class FGView" and updated to make data
// members private and make required accessor functions.
//
// Revision 1.29  1998/12/05 15:54:24  curt
// Renamed class fgFLIGHT to class FGState as per request by JSB.
//
// Revision 1.28  1998/12/03 01:17:20  curt
// Converted fgFLIGHT to a class.
//
// Revision 1.27  1998/11/16 14:00:06  curt
// Added pow() macro bug work around.
// Added support for starting FGFS at various resolutions.
// Added some initial serial port support.
// Specify default log levels in main().
//
// Revision 1.26  1998/11/09 23:39:25  curt
// Tweaks for the instrument panel.
//
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
