// views.hxx -- data structures and routines for managing and view parameters.
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


#ifndef _VIEWS_HXX
#define _VIEWS_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   

#include <Include/compiler.h>

#include <list>

#include <sg.h>			// plib include

#include <FDM/flight.hxx>
#include <Math/mat3.h>
#include <Math/point3d.hxx>
#include <Time/fg_time.hxx>
#include <Time/light.hxx>

#include "options.hxx"

FG_USING_STD(list);


class FGMat4Wrapper {
public:
    sgMat4 m;
};

typedef list < FGMat4Wrapper > sgMat4_list;
typedef sgMat4_list::iterator sgMat4_list_iterator;
typedef sgMat4_list::const_iterator const_sgMat4_list_iterator;


// used in views.cxx and tilemgr.cxx
#define USE_FAST_FOV_CLIP 


// Define a structure containing view information
class FGView {

public:

    enum fgViewMode
    {
	FG_VIEW_FIRST_PERSON = 0,
	FG_VIEW_FOLLOW  = 1
    };

    // the current offset from forward for viewing
    double view_offset;

    // the goal view offset for viewing (used for smooth view changes)
    double goal_view_offset;

    // flag forcing update of fov related stuff
    bool update_fov;
	
    // fov of view is specified in the y direction, win_ratio is used to
    // calculate the fov in the X direction = width/height
    double win_ratio;

    // width & height of window
    int winWidth, winHeight;

    // sin and cos of (fov / 2) in Y axis
    double sin_fov_y, cos_fov_y;
    double sinlon, coslon;

    // slope of view frustum edge in eye space Y axis
    double slope_y;

    // sin and cos of (fov / 2) in X axis
    double sin_fov_x, cos_fov_x;

    // slope of view frustum edge in eye space X axis
    double slope_x;

#if defined( USE_FAST_FOV_CLIP )
    double fov_x_clip, fov_y_clip;
#endif // USE_FAST_FOV_CLIP

    // View frustum cull ratio (% of tiles culled ... used for
    // reporting purposes)
    double vfc_ratio;

    // Number of triangles rendered;
    int tris_rendered;
    int tris_culled;

    // absolute view position
    Point3D abs_view_pos;

    // view position translated to scenery.center
    Point3D view_pos;

    // cartesion coordinates of current lon/lat if at sea level
    // translated to scenery.center*/
    Point3D cur_zero_elev;

    // vector in cartesian coordinates from current position to the
    // postion on the earth's surface the sun is directly over
    MAT3vec to_sun;
    
    // surface direction to go to head towards sun
    MAT3vec surface_to_sun;

    // vector in cartesian coordinates from current position to the
    // postion on the earth's surface the moon is directly over
    MAT3vec to_moon;
  
    // surface direction to go to head towards moon
    MAT3vec surface_to_moon;

    // surface vector heading south
    MAT3vec surface_south;

    // surface vector heading east (used to unambiguously align sky
    // with sun)
    MAT3vec surface_east;

    // local up vector (normal to the plane tangent to the earth's
    // surface at the spot we are directly above
    MAT3vec local_up;

    // up vector for the view (usually point straight up through the
    // top of the aircraft
    MAT3vec view_up;

    // the vector pointing straight out the nose of the aircraft
    MAT3vec view_forward;

    // Transformation matrix for eye coordinates to aircraft coordinates
    MAT3mat AIRCRAFT;

    // Transformation matrix for the view direction offset relative to
    // the AIRCRAFT matrix
    MAT3mat VIEW_OFFSET;

    // Transformation matrix for aircraft coordinates to world
    // coordinates
    MAT3mat WORLD;

    // Combined transformation from eye coordinates to world coordinates
    MAT3mat EYE_TO_WORLD;

    // Inverse of EYE_TO_WORLD which is a transformation from world
    // coordinates to eye coordinates
    MAT3mat WORLD_TO_EYE;

    // Current model view matrix;
    GLfloat MODEL_VIEW[16];

    // view mode
    fgViewMode view_mode;

    // sg versions of our friendly matrices
    sgMat4 sgLOCAL, sgUP, sgVIEW_ROT, sgTRANS, sgVIEW, sgLARC_TO_SSG;

    // queue of view matrices so we can have a follow view
    sgMat4_list follow;

public:

    // Constructor
    FGView( void );

    // Destructor
    ~FGView( void );

    // Initialize a view class
    void Init( void );

    // Basically, this is a modified version of the Mesa gluLookAt()
    // function that's been modified slightly so we can capture the
    // result (and use it later) otherwise this all gets calculated in
    // OpenGL land and we don't have access to the results.
    void LookAt( GLdouble eyex, GLdouble eyey, GLdouble eyez,
		 GLdouble centerx, GLdouble centery, GLdouble centerz,
		 GLdouble upx, GLdouble upy, GLdouble upz );

    // Update the view volume, position, and orientation
    void UpdateViewParams( void );

    // Flag to request that UpdateFOV() be called next time
    // UpdateViewMath() is run.
    inline void force_update_fov_math() { update_fov = true; }

    // Update the view parameters
    void UpdateViewMath( FGInterface *f );

    // Update the "World to Eye" transformation matrix
    void UpdateWorldToEye( FGInterface *f );

    // Update the field of view coefficients
    void UpdateFOV( const fgOPTIONS& o );

    // Cycle view mode
    void cycle_view_mode();

    // accessor functions
    inline double get_view_offset() const { return view_offset; }
    inline void set_view_offset( double a ) { view_offset = a; }
    inline void inc_view_offset( double amt ) { view_offset += amt; }
    inline double get_goal_view_offset() const { return goal_view_offset; }
    inline void set_goal_view_offset( double a) { goal_view_offset = a; }
    inline double get_win_ratio() const { return win_ratio; }
    inline void set_win_ratio( double r ) { win_ratio = r; }
    inline int get_winWidth() const { return winWidth; }
    inline void set_winWidth( int w ) { winWidth = w; }
    inline int get_winHeight() const { return winHeight; }
    inline void set_winHeight( int h ) { winHeight = h; }
    inline double get_slope_y() const { return slope_y; }
    inline double get_slope_x() const { return slope_x; }
#if defined( USE_FAST_FOV_CLIP )
    inline double get_fov_x_clip() const { return fov_x_clip; }
    inline double get_fov_y_clip() const { return fov_y_clip; }
#endif // USE_FAST_FOV_CLIP
    inline double get_vfc_ratio() const { return vfc_ratio; }
    inline void set_vfc_ratio(double r) { vfc_ratio = r; }
    inline int get_tris_rendered() const { return tris_rendered; }
    inline void set_tris_rendered( int tris) { tris_rendered = tris; }
    inline int get_tris_culled() const { return tris_culled; }
    inline void set_tris_culled( int tris) { tris_culled = tris; }
    inline Point3D get_abs_view_pos() const { return abs_view_pos; }
    inline Point3D get_view_pos() const { return view_pos; }
    inline Point3D get_cur_zero_elev() const { return cur_zero_elev; }
    inline double *get_to_sun() { return to_sun; }
    inline void set_to_sun( double x, double y, double z) {
	to_sun[0] = x;
	to_sun[1] = y;
	to_sun[2] = z;
    }
    inline double *get_surface_to_sun() { return surface_to_sun; }
    inline void set_surface_to_sun( double x, double y, double z) {
	surface_to_sun[0] = x;
	surface_to_sun[1] = y;
	surface_to_sun[2] = z;
    }
    inline double *get_to_moon() { return to_moon; }
    inline void set_to_moon( double x, double y, double z) {
	to_moon[0] = x;
	to_moon[1] = y;
	to_moon[2] = z;
    }
    inline double *get_surface_to_moon() { return surface_to_moon; }
    inline void set_surface_to_moon( double x, double y, double z) {
	surface_to_moon[0] = x;
	surface_to_moon[1] = y;
	surface_to_moon[2] = z;
    }
    inline double *get_surface_south() { return surface_south; }
    inline double *get_surface_east() { return surface_east; }
    inline double *get_local_up() { return local_up; }
    inline double *get_view_forward() { return view_forward; }
    inline const MAT3mat *get_WORLD_TO_EYE() const { return &WORLD_TO_EYE; }
    inline GLfloat *get_MODEL_VIEW() { return MODEL_VIEW; }
};


extern FGView current_view;


#endif // _VIEWS_HXX


