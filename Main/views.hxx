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
// (Log is kept at end of this file)


#ifndef _VIEWS_HXX
#define _VIEWS_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#include <FDM/flight.hxx>
#include <Math/mat3.h>
#include <Math/point3d.hxx>
#include <Time/fg_time.hxx>
#include <Time/light.hxx>

#include "options.hxx"


// used in views.cxx and tilemgr.cxx
#define USE_FAST_FOV_CLIP 


// Define a structure containing view information
class FGView {

public:

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
    GLdouble MODEL_VIEW[16];

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
    void UpdateViewMath( FGState *f );

    // Update the "World to Eye" transformation matrix
    void UpdateWorldToEye( FGState *f );

    // Update the field of view coefficients
    void UpdateFOV( const fgOPTIONS& o );

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
    inline double *get_surface_south() { return surface_south; }
    inline double *get_surface_east() { return surface_east; }
    inline double *get_local_up() { return local_up; }
    inline const MAT3mat *get_WORLD_TO_EYE() const { return &WORLD_TO_EYE; }
    inline GLdouble *get_MODEL_VIEW() { return MODEL_VIEW; }
};


extern FGView current_view;


#endif // _VIEWS_HXX


// $Log$
// Revision 1.20  1999/02/02 20:13:38  curt
// MSVC++ portability changes by Bernie Bright:
//
// Lib/Serial/serial.[ch]xx: Initial Windows support - incomplete.
// Simulator/Astro/stars.cxx: typo? included <stdio> instead of <cstdio>
// Simulator/Cockpit/hud.cxx: Added Standard headers
// Simulator/Cockpit/panel.cxx: Redefinition of default parameter
// Simulator/Flight/flight.cxx: Replaced cout with FG_LOG.  Deleted <stdio.h>
// Simulator/Main/fg_init.cxx:
// Simulator/Main/GLUTmain.cxx:
// Simulator/Main/options.hxx: Shuffled <fg_serial.hxx> dependency
// Simulator/Objects/material.hxx:
// Simulator/Time/timestamp.hxx: VC++ friend kludge
// Simulator/Scenery/tile.[ch]xx: Fixed using std::X declarations
// Simulator/Main/views.hxx: Added a constant
//
// Revision 1.19  1999/02/01 21:33:36  curt
// Renamed FlightGear/Simulator/Flight to FlightGear/Simulator/FDM since
// Jon accepted my offer to do this and thought it was a good idea.
//
// Revision 1.18  1998/12/11 20:26:30  curt
// Fixed view frustum culling accuracy bug so we can look out the sides and
// back without tri-stripes dropping out.
//
// Revision 1.17  1998/12/09 18:50:29  curt
// Converted "class fgVIEW" to "class FGView" and updated to make data
// members private and make required accessor functions.
//
// Revision 1.16  1998/12/05 15:54:25  curt
// Renamed class fgFLIGHT to class FGState as per request by JSB.
//
// Revision 1.15  1998/10/16 23:27:56  curt
// C++-ifying.
//
// Revision 1.14  1998/10/16 00:54:04  curt
// Converted to Point3D class.
//
// Revision 1.13  1998/09/08 15:04:36  curt
// Optimizations by Norman Vine.
//
// Revision 1.12  1998/08/24 20:11:15  curt
// Added i/I to toggle full vs. minimal HUD.
// Added a --hud-tris vs --hud-culled option.
// Moved options accessor funtions to options.hxx.
//
// Revision 1.11  1998/08/20 20:32:35  curt
// Reshuffled some of the code in and around views.[ch]xx
//
// Revision 1.10  1998/07/08 14:45:09  curt
// polar3d.h renamed to polar3d.hxx
// vector.h renamed to vector.hxx
// updated audio support so it waits to create audio classes (and tie up
//   /dev/dsp) until the mpg123 player is finished.
//
// Revision 1.9  1998/07/04 00:52:27  curt
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
// Revision 1.8  1998/05/27 02:24:06  curt
// View optimizations by Norman Vine.
//
// Revision 1.7  1998/05/17 16:59:04  curt
// First pass at view frustum culling now operational.
//
// Revision 1.6  1998/05/16 13:08:37  curt
// C++ - ified views.[ch]xx
// Shuffled some additional view parameters into the fgVIEW class.
// Changed tile-radius to tile-diameter because it is a much better
//   name.
// Added a WORLD_TO_EYE transformation to views.cxx.  This allows us
//  to transform world space to eye space for view frustum culling.
//
// Revision 1.5  1998/05/02 01:51:02  curt
// Updated polartocart conversion routine.
//
// Revision 1.4  1998/04/28 01:20:24  curt
// Type-ified fgTIME and fgVIEW.
// Added a command line option to disable textures.
//
// Revision 1.3  1998/04/25 22:06:31  curt
// Edited cvs log messages in source files ... bad bad bad!
//
// Revision 1.2  1998/04/24 00:49:22  curt
// Wrapped "#include <config.h>" in "#ifdef HAVE_CONFIG_H"
// Trying out some different option parsing code.
// Some code reorganization.
//
// Revision 1.1  1998/04/22 13:25:46  curt
// C++ - ifing the code.
// Starting a bit of reorganization of lighting code.
//
// Revision 1.11  1998/04/21 17:02:42  curt
// Prepairing for C++ integration.
//
// Revision 1.10  1998/02/07 15:29:45  curt
// Incorporated HUD changes and struct/typedef changes from Charlie Hotchkiss
// <chotchkiss@namg.us.anritsu.com>
//
// Revision 1.9  1998/01/29 00:50:29  curt
// Added a view record field for absolute x, y, z position.
//
// Revision 1.8  1998/01/27 00:47:58  curt
// Incorporated Paul Bleisch's <pbleisch@acm.org> new debug message
// system and commandline/config file processing code.
//
// Revision 1.7  1998/01/22 02:59:38  curt
// Changed #ifdef FILE_H to #ifdef _FILE_H
//
// Revision 1.6  1998/01/19 19:27:10  curt
// Merged in make system changes from Bob Kuehne <rpk@sgi.com>
// This should simplify things tremendously.
//
// Revision 1.5  1997/12/22 04:14:32  curt
// Aligned sky with sun so dusk/dawn effects can be correct relative to the sun.
//
// Revision 1.4  1997/12/17 23:13:36  curt
// Began working on rendering a sky.
//
// Revision 1.3  1997/12/15 23:54:51  curt
// Add xgl wrappers for debugging.
// Generate terrain normals on the fly.
//
// Revision 1.2  1997/12/10 22:37:48  curt
// Prepended "fg" on the name of all global structures that didn't have it yet.
// i.e. "struct WEATHER {}" became "struct fgWEATHER {}"
//
// Revision 1.1  1997/08/27 21:31:18  curt
// Initial revision.
//
