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
#include <simgear/math/point3d.hxx>
#include <simgear/timing/sg_time.hxx>

#include <list>

#include <plib/sg.h>		// plib include

#include <FDM/flight.hxx>
#include <Time/light.hxx>


FG_USING_STD(list);


#define FG_FOV_MIN 0.1
#define FG_FOV_MAX 179.9


// Define a structure containing view information
class FGViewer {

private:

    // flag forcing a recalc of derived view parameters
    bool dirty;
	
    // the current view offset angle from forward (rotated about the
    // view_up vector)
    double view_offset;

    // the goal view offset angle  (used for smooth view changes)
    double goal_view_offset;

    // geodetic view position
    Point3D geod_view_pos;

    // radius to sea level from center of the earth (m)
    double sea_level_radius;

    // absolute view position in earth coordinates
    Point3D abs_view_pos;

    // view position in opengl world coordinates (this is the
    // abs_view_pos translated to scenery.center)
    Point3D view_pos;

    // pilot offset from center of gravity.  The X axis is positive
    // out the tail, Y is out the right wing, and Z is positive up.
    // Distances in meters of course.
    sgVec3 pilot_offset;

    // view orientation (heading, pitch, roll)
    sgVec3 hpr;

    // cartesion coordinates of current lon/lat if at sea level
    // translated to scenery.center
    Point3D cur_zero_elev;

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

    // up vector for the view (usually point straight up through the
    // top of the aircraft
    sgVec3 view_up;

    // the vector pointing straight out the nose of the aircraft
    sgVec3 view_forward;

    // Transformation matrix for the view direction offset relative to
    // the AIRCRAFT matrix
    sgMat4 VIEW_OFFSET;

    // sg versions of our friendly matrices
    sgMat4 LOCAL, UP, VIEW_ROT, TRANS, VIEW, LARC_TO_SSG;

    // Update the view volume, position, and orientation
    void update();

public:

    // Constructor
    FGViewer( void );

    // Destructor
    ~FGViewer( void );

    // Initialize a view class
    void init( void );

    // Transform a vector from world coordinates to the local plane
    void CurrentNormalInLocalPlane(sgVec3 dst, sgVec3 src);

    //////////////////////////////////////////////////////////////////////
    // setter functions
    //////////////////////////////////////////////////////////////////////
    inline void set_geod_view_pos( double lon, double lat, double alt ) {
	// data should be in radians and meters asl
	dirty = true;
	// cout << "set_geod_view_pos = " << lon << ", " << lat << ", " << alt
	//      << endl;
	geod_view_pos = Point3D( lon, lat, alt );
    }
    inline void set_sea_level_radius( double r ) {
	// data should be in meters from the center of the earth
	dirty = true;
	sea_level_radius = r;
    }
    inline void set_hpr( double h, double p, double r ) {
	// data should be in radians
	dirty = true;
	sgSetVec3( hpr, h, p, r );
    }
    inline void set_pilot_offset( float x, float y, float z ) {
	dirty = true;
	sgSetVec3( pilot_offset, x, y, z );
    }
    inline void set_view_offset( double a ) {
	dirty = true;
	view_offset = a;
    }
    inline void inc_view_offset( double amt ) {
	dirty = true;
	view_offset += amt;
    }
    inline void set_goal_view_offset( double a) {
	dirty = true;
	goal_view_offset = a;
    }

    //////////////////////////////////////////////////////////////////////
    // accessor functions
    //////////////////////////////////////////////////////////////////////
    inline double get_view_offset() const { return view_offset; }
    inline double get_goal_view_offset() const { return goal_view_offset; }
    inline float *get_pilot_offset() { return pilot_offset; }
    inline double get_sea_level_radius() const { return sea_level_radius; }
    inline float *get_hpr() { return hpr; }

    //////////////////////////////////////////////////////////////////////
    // derived values accessor functions
    //////////////////////////////////////////////////////////////////////
    inline Point3D get_abs_view_pos() {
	if ( dirty ) { update(); }
	return abs_view_pos;
    }
    inline Point3D get_view_pos() {
	if ( dirty ) { update(); }
	return view_pos;
    }
    inline Point3D get_cur_zero_elev() {
	if ( dirty ) { update(); }
	return cur_zero_elev;
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
    inline float *get_view_forward() {
	if ( dirty ) { update(); }
	return view_forward;
    }
    inline const sgVec4 *get_VIEW() {
	if ( dirty ) { update(); }
	return VIEW;
    }
    inline const sgVec4 *get_VIEW_ROT() {
	if ( dirty ) { update(); }
	return VIEW_ROT;
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


