//
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


#include <Include/fg_types.h>
#include <Flight/flight.h>
#include <Math/mat3.h>
#include <Time/fg_time.hxx>
#include <Time/light.hxx>

#include "options.hxx"

#ifndef BOOL
#define BOOL int
#endif

#ifndef TRUE
#define FALSE 0
#define TRUE 1
#endif

// Define a structure containing view information
class fgVIEW {

public:

    // the current offset from forward for viewing
    double view_offset;

    // the goal view offset for viewing (used for smooth view changes)
    double goal_view_offset;

    // flag forcing update of fov related stuff
    BOOL update_fov;
	
    // fov of view is specified in the y direction, win_ratio is used to
    // calculate the fov in the X direction = width/height
    double win_ratio;

    // width & height of window
    int winWidth, winHeight;

    // sin and cos of (fov / 2) in Y axis
    double sin_fov_y, cos_fov_y;

    // slope of view frustum edge in eye space Y axis
    double slope_y;

    // sin and cos of (fov / 2) in X axis
    double sin_fov_x, cos_fov_x;

    // slope of view frustum edge in eye space X axis
    double slope_x;

    // View frustum cull ratio (% of tiles culled ... used for
    // reporting purposes)
    double vfc_ratio;

    // absolute view position
    fgPoint3d abs_view_pos;

    // view position translated to scenery.center
    fgPoint3d view_pos;

    // cartesion coordinates of current lon/lat if at sea level
    // translated to scenery.center*/
    fgPoint3d cur_zero_elev;

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

    // Constructor
    fgVIEW( void );

    // Initialize a view class
    void Init( void );

    // Update the view parameters
    void Update( fgFLIGHT *f );

    // Update the "World to Eye" transformation matrix
    void UpdateWorldToEye(  fgFLIGHT *f );

    // Update the field of view parameters
    void UpdateFOV( fgOPTIONS *o );
	
    // Destructor
    ~fgVIEW( void );
};


extern fgVIEW current_view;


// Basically, this is a modified version of the Mesa gluLookAt()
// function that's been modified slightly so we can capture the result
// before sending it off to OpenGL land.
void fg_gluLookAt( GLdouble eyex, GLdouble eyey, GLdouble eyez,
		   GLdouble centerx, GLdouble centery, GLdouble centerz,
		   GLdouble upx, GLdouble upy, GLdouble upz );


#endif // _VIEWS_HXX


// $Log$
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
