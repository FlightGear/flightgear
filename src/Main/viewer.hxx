// viewer.hxx -- class for managing a viewer in the flightgear world.
//
// Written by Curtis Olson, started August 1997.
//                          overhaul started October 2000.
//   partially rewritten by Jim Wilson jim@kelcomaine.com using interface
//                          by David Megginson March 2002
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
#include <simgear/constants.h>

#include <plib/sg.h>		// plib include

#include "fgfs.hxx"


#define FG_FOV_MIN 0.1
#define FG_FOV_MAX 179.9


// Define a structure containing view information
class FGViewer : public FGSubsystem {

public:

    enum fgViewType {
	FG_RPH = 0,
	FG_LOOKAT = 1,
	FG_HPR = 2
    };

    enum fgScalingType {  // nominal Field Of View actually applies to ...
	FG_SCALING_WIDTH,       // window width
	FG_SCALING_MAX,         // max(width, height)
	// FG_SCALING_G_MEAN,      // geometric_mean(width, height)
	// FG_SCALING_INDEPENDENT  // whole screen
    };

    // Constructor
    FGViewer( void );

    // Destructor
    virtual ~FGViewer( void );

    //////////////////////////////////////////////////////////////////////
    // Part 1: standard FGSubsystem implementation.
    //////////////////////////////////////////////////////////////////////

    virtual void init ();
    virtual void bind ();
    virtual void unbind ();
    void update (int dt);


    //////////////////////////////////////////////////////////////////////
    // Part 2: user settings.
    //////////////////////////////////////////////////////////////////////

    virtual fgViewType getType() const { return _type; }
    virtual void setType( int type );

		    // Reference geodetic position of view from position...
    virtual double getLongitude_deg () const { return _lon_deg; }
    virtual double getLatitude_deg () const { return _lat_deg; }
    virtual double getAltitudeASL_ft () const { return _alt_ft; }
		    // Set individual coordinates for the view point position.
    virtual void setLongitude_deg (double lon_deg);
    virtual void setLatitude_deg (double lat_deg);
    virtual void setAltitude_ft (double alt_ft);
		    // Set the geodetic position for the view point.
    virtual void setPosition (double lon_deg, double lat_deg, double alt_ft);

		    // Reference geodetic target position...
    virtual double getTargetLongitude_deg () const { return _target_lon_deg; }
    virtual double getTargetLatitude_deg () const { return _target_lat_deg; }
    virtual double getTargetAltitudeASL_ft () const { return _target_alt_ft; }
		    // Set individual coordinates for the Target point position.
    virtual void setTargetLongitude_deg (double lon_deg);
    virtual void setTargetLatitude_deg (double lat_deg);
    virtual void setTargetAltitude_ft (double alt_ft);
		    // Set the geodetic position for the Target point.
    virtual void setTargetPosition (double lon_deg, double lat_deg, double alt_ft);

		    // Refence orientation...
    virtual double getRoll_deg () const { return _roll_deg; }
    virtual double getPitch_deg () const {return _pitch_deg; }
    virtual double getHeading_deg () const {return _heading_deg; }
    virtual void setRoll_deg (double roll_deg);
    virtual void setPitch_deg (double pitch_deg);
    virtual void setHeading_deg (double heading_deg);
    virtual void setOrientation (double roll_deg, double pitch_deg, double heading_deg);

		    // Position offsets from reference
    virtual double getXOffset_m () const { return _x_offset_m; }
    virtual double getYOffset_m () const { return _y_offset_m; }
    virtual double getZOffset_m () const { return _z_offset_m; }
    virtual void setXOffset_m (double x_offset_m);
    virtual void setYOffset_m (double y_offset_m);
    virtual void setZOffset_m (double z_offset_m);
    virtual void setPositionOffsets (double x_offset_m,
				     double y_offset_m,
				     double z_offset_m);

		    // Orientation offsets from reference
                    //  Goal settings are for smooth transition from prior 
                    //  offset when changing view direction.
    virtual double getRollOffset_deg () const { return _roll_offset_deg; }
    virtual double getPitchOffset_deg () const { return _pitch_offset_deg; }
    virtual double getHeadingOffset_deg () const { return _heading_offset_deg; }
    virtual double getGoalRollOffset_deg () const { return _goal_roll_offset_deg; }
    virtual double getGoalPitchOffset_deg () const { return _goal_pitch_offset_deg; }
    virtual double getGoalHeadingOffset_deg () const {return _goal_heading_offset_deg; }
    virtual void setRollOffset_deg (double roll_offset_deg);
    virtual void setPitchOffset_deg (double pitch_offset_deg);
    virtual void setHeadingOffset_deg (double heading_offset_deg);
    virtual void setGoalRollOffset_deg (double goal_roll_offset_deg);
    virtual void setGoalPitchOffset_deg (double goal_pitch_offset_deg);
    virtual void setGoalHeadingOffset_deg (double goal_heading_offset_deg);
    virtual void setOrientationOffsets (double roll_offset_deg,
				     double heading_offset_deg,
				     double pitch_offset_deg);



    //////////////////////////////////////////////////////////////////////
    // Part 3: output vectors and matrices in FlightGear coordinates.
    //////////////////////////////////////////////////////////////////////

    // Vectors and positions...

	    // Get zero view_pos
    virtual float * get_view_pos() {if ( _dirty ) { recalc(); }	return view_pos; }
	    // Get the absolute view position in fgfs coordinates.
    virtual double * get_absolute_view_pos ();
	    // Get zero elev
    virtual float * get_zero_elev() {if ( _dirty ) { recalc(); } return zero_elev; }
	    // Get world up vector
    virtual float *get_world_up() {if ( _dirty ) { recalc(); } return world_up; }
	    // Get the relative (to scenery center) view position in fgfs coordinates.
    virtual float * getRelativeViewPos ();
	    // Get the absolute zero-elevation view position in fgfs coordinates.
    virtual float * getZeroElevViewPos ();
	    // Get surface east vector
    virtual float *get_surface_east() {	if ( _dirty ) { recalc(); } return surface_east; }
	    // Get surface south vector
    virtual float *get_surface_south() {if ( _dirty ) { recalc(); } return surface_south; }

	    // Matrices...
    virtual const sgVec4 *get_VIEW() { if ( _dirty ) { recalc(); } return VIEW; }
    virtual const sgVec4 *get_VIEW_ROT() { if ( _dirty ) { recalc(); }	return VIEW_ROT; }
    virtual const sgVec4 *get_UP() { if ( _dirty ) { recalc(); } return UP; }
	    // (future?)
	    // virtual double get_ground_elev() { if ( _dirty ) { recalc(); } return ground_elev; }


private:

    // flag forcing a recalc of derived view parameters
    bool _dirty;

    void recalc ();
    void recalcPositionVectors (double lon_deg, double lat_deg, double alt_ft) const;

    mutable sgdVec3 _absolute_view_pos;
    mutable sgVec3 _relative_view_pos;
    mutable sgVec3 _zero_elev_view_pos;

    double _lon_deg;
    double _lat_deg;
    double _alt_ft;
    double _target_lon_deg;
    double _target_lat_deg;
    double _target_alt_ft;

    double _roll_deg;
    double _pitch_deg;
    double _heading_deg;

    // Position offsets from center of gravity.  The X axis is positive
    // out the tail, Y is out the right wing, and Z is positive up.
    // distance in meters
    double _x_offset_m;
    double _y_offset_m;
    double _z_offset_m;

    // orientation offsets from reference (_goal* are for smoothed transitions)
    double _roll_offset_deg;
    double _pitch_offset_deg;
    double _heading_offset_deg;
    double _goal_roll_offset_deg;
    double _goal_pitch_offset_deg;
    double _goal_heading_offset_deg;

protected:

    fgViewType _type;
    fgScalingType scalingType;

    // the nominal field of view (angle, in degrees)
    double fov; 

    // ratio of window width and height; height = width * aspect_ratio
    double aspect_ratio;

    bool reverse_view_offset;

    // view position in opengl world coordinates (this is the
    // abs_view_pos translated to scenery.center)
    sgVec3 view_pos;

    // radius to sea level from center of the earth (m)
    double sea_level_radius;

    // cartesion coordinates of current lon/lat if at sea level
    // translated to scenery.center
    sgVec3 zero_elev;

    // height ASL of the terrain for our current view position
    // (future?) double ground_elev;

    // surface vector heading south
    sgVec3 surface_south;

    // surface vector heading east (used to unambiguously align sky
    // with sun)
    sgVec3 surface_east;

    // world up vector (normal to the plane tangent to the earth's
    // surface at the spot we are directly above
    sgVec3 world_up;

    // sg versions of our friendly matrices
    sgMat4 VIEW, VIEW_ROT, UP;

    // up vector for the view (usually point straight up through the
    // top of the aircraft
    sgVec3 view_up;

    // the vector pointing straight out the nose of the aircraft
    sgVec3 view_forward;

    // Transformation matrix for the view direction offset relative to
    // the AIRCRAFT matrix
    sgMat4 VIEW_OFFSET;

    // sg versions of our friendly matrices (from lookat)
    sgMat4 LOCAL, TRANS, LARC_TO_SSG;

    inline void set_dirty() { _dirty = true; }
    inline void set_clean() { _dirty = false; }

    // from lookat
    void fgMakeLookAtMat4 ( sgMat4 dst, const sgVec3 eye, const sgVec3 center,
			const sgVec3 up );

    // from rph
    void fgMakeViewRot( sgMat4 dst, const sgMat4 m1, const sgMat4 m2 );
    void fgMakeLOCAL( sgMat4 dst, const double Theta,
				const double Phi, const double Psi);


public:


    //////////////////////////////////////////////////////////////////////
    // setter functions
    //////////////////////////////////////////////////////////////////////

    inline void set_fov( double fov_deg ) {
	fov = fov_deg;
    }

    inline void set_aspect_ratio( double r ) {
	aspect_ratio = r;
    }
    inline void inc_view_offset( double amt ) {
	set_dirty();
	_heading_offset_deg += amt;
    }
    inline void set_reverse_view_offset( bool val ) {
	reverse_view_offset = val;
    }
    inline void inc_view_tilt( double amt ) {
	set_dirty();
	_pitch_offset_deg += amt;
    }
    inline void set_sea_level_radius( double r ) {
	// data should be in meters from the center of the earth
	set_dirty();
	sea_level_radius = r;
    }

    /* from lookat */
    inline void set_view_forward( sgVec3 vf ) {
	set_dirty();
	sgCopyVec3( view_forward, vf );
    }
    inline void set_view_up( sgVec3 vf ) {
	set_dirty();
	sgCopyVec3( view_up, vf );
    }
    /* end from lookat */


    //////////////////////////////////////////////////////////////////////
    // accessor functions
    //////////////////////////////////////////////////////////////////////
    inline int get_type() const { return _type ; }
    inline int is_a( int t ) const { return get_type() == t ; }
    inline bool is_dirty() const { return _dirty; }
    inline double get_fov() const { return fov; }
    inline double get_aspect_ratio() const { return aspect_ratio; }
    inline bool get_reverse_view_offset() const { return reverse_view_offset; }
    inline double get_sea_level_radius() const { return sea_level_radius; }
    // Get horizontal field of view angle, in degrees.
    double get_h_fov();
    // Get vertical field of view angle, in degrees.
    double get_v_fov();

    /* from lookat */
    inline float *get_view_forward() { return view_forward; }
    inline float *get_view_up() { return view_up; }
    /* end from lookat */

    //////////////////////////////////////////////////////////////////////
    // derived values accessor functions
    //////////////////////////////////////////////////////////////////////

};


#endif // _VIEWER_HXX



