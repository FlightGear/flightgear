// viewer.hxx -- class for managing a viewer in the flightgear world.
//
// Written by Curtis Olson, started August 1997.
//                          overhaul started October 2000.
//
// Copyright (C) 1997 - 2000  Curtis L. Olson  - curt@flightgear.org
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


#ifndef _VIEWER_HXX
#define _VIEWER_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   

#include <simgear/compiler.h>
#include <simgear/timing/sg_time.hxx>

#include <plib/sg.h>		// plib include


// Define a structure containing view information
class FGViewer {

private:

    // flag forcing a recalc of derived view parameters
    bool dirty;

protected:

    // the current view offset angle from forward (rotated about the
    // view_up vector)
    double view_offset;

    // the goal view offset angle  (used for smooth view changes)
    double goal_view_offset;

    // absolute view position in earth coordinates
    sgdVec3 abs_view_pos;

    // view position in opengl world coordinates (this is the
    // abs_view_pos translated to scenery.center)
    sgVec3 view_pos;

    // radius to sea level from center of the earth (m)
    double sea_level_radius;

    // cartesion coordinates of current lon/lat if at sea level
    // translated to scenery.center
    sgVec3 zero_elev;

    // pilot offset from center of gravity.  The X axis is positive
    // out the tail, Y is out the right wing, and Z is positive up.
    // Distances in meters of course.
    sgVec3 pilot_offset;

    // vector in cartesian coordinates from current position to the
    // postion on the earth's surface the sun is directly over
    sgVec3 to_sun;

    // surface direction to go to head towards sun
    sgVec3 surface_to_sun;

    // vector in cartesian coordinates from current position to the
    // postion on the earth's surface the moon is directly over
    sgVec3 to_moon;
  
    // surface direction to go to head towards moon
    sgVec3 surface_to_moon;

    // surface vector heading south
    sgVec3 surface_south;

    // surface vector heading east (used to unambiguously align sky
    // with sun)
    sgVec3 surface_east;

    // local up vector (normal to the plane tangent to the earth's
    // surface at the spot we are directly above
    sgVec3 local_up;

    // sg versions of our friendly matrices
    sgMat4 VIEW, VIEW_ROT, UP;

    inline void set_dirty() { dirty = true; }
    inline void set_clean() { dirty = false; }

    // Update the view volume, position, and orientation
    virtual void update();

public:

    // Constructor
    FGViewer( void );

    // Destructor
    virtual ~FGViewer( void );

    // Initialize a view class
    virtual void init( void );

    //////////////////////////////////////////////////////////////////////
    // setter functions
    //////////////////////////////////////////////////////////////////////
    inline void set_view_offset( double a ) {
	set_dirty();
	view_offset = a;
    }
    inline void inc_view_offset( double amt ) {
	set_dirty();
	view_offset += amt;
    }
    inline void set_goal_view_offset( double a) {
	set_dirty();
	goal_view_offset = a;
    }
    inline void set_pilot_offset( float x, float y, float z ) {
	set_dirty();
	sgSetVec3( pilot_offset, x, y, z );
    }
    inline void set_sea_level_radius( double r ) {
	// data should be in meters from the center of the earth
	set_dirty();
	sea_level_radius = r;
    }

    //////////////////////////////////////////////////////////////////////
    // accessor functions
    //////////////////////////////////////////////////////////////////////
    inline bool is_dirty() const { return dirty; }
    inline double get_view_offset() const { return view_offset; }
    inline double get_goal_view_offset() const { return goal_view_offset; }
    inline float *get_pilot_offset() { return pilot_offset; }
    inline double get_sea_level_radius() const { return sea_level_radius; }

    //////////////////////////////////////////////////////////////////////
    // derived values accessor functions
    //////////////////////////////////////////////////////////////////////
    inline double *get_abs_view_pos() {
	if ( dirty ) { update(); }
	return abs_view_pos;
    }
    inline float *get_view_pos() {
	if ( dirty ) { update(); }
	return view_pos;
    }
    inline float *get_zero_elev() {
	if ( dirty ) { update(); }
	return zero_elev;
    }
    inline float *get_surface_south() {
	if ( dirty ) { update(); }
	return surface_south;
    }
    inline float *get_surface_east() {
	if ( dirty ) { update(); }
	return surface_east;
    }
    inline float *get_local_up() {
	if ( dirty ) { update(); }
	return local_up;
    }
    inline const sgVec4 *get_VIEW() {
	if ( dirty ) { update(); }
	return VIEW;
    }
    inline const sgVec4 *get_VIEW_ROT() {
	if ( dirty ) { update(); }
	return VIEW_ROT;
    }
    inline const sgVec4 *get_UP() {
	if ( dirty ) { update(); }
	return UP;
    }

    //////////////////////////////////////////////////////////////////////
    // need to fix these
    //////////////////////////////////////////////////////////////////////
    inline float *get_to_sun() { return to_sun; }
    inline void set_to_sun( float x, float y, float z ) {
	sgSetVec3( to_sun, x, y, z );
    }
    inline float *get_surface_to_sun() { return surface_to_sun; }
    inline void set_surface_to_sun( float x, float y, float z) {
	sgSetVec3( surface_to_sun, x, y, z );
    }
    inline float *get_to_moon() { return to_moon; }
    inline void set_to_moon( float x, float y, float z) {
	sgSetVec3( to_moon, x, y, z );
    }
    inline float *get_surface_to_moon() { return surface_to_moon; }
    inline void set_surface_to_moon( float x, float y, float z) {
	sgSetVec3( surface_to_moon, x, y, z );
    }
};


#endif // _VIEWER_HXX


