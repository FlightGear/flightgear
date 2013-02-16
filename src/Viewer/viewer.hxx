// viewer.hxx -- class for managing a viewer in the flightgear world.
//
// Written by Curtis Olson, started August 1997.
//                          overhaul started October 2000.
//   partially rewritten by Jim Wilson jim@kelcomaine.com using interface
//                          by David Megginson March 2002
//
// Copyright (C) 1997 - 2000  Curtis L. Olson  - http://www.flightgear.org/~curt
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$


#ifndef _VIEWER_HXX
#define _VIEWER_HXX                                

namespace flightgear
{
class CameraGroup;
}

#include <osg/ref_ptr>

#include <simgear/compiler.h>
#include <simgear/constants.h>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/math/SGMath.hxx>

#define FG_FOV_MIN 0.1
#define FG_FOV_MAX 179.9

enum fgViewType {
 FG_LOOKFROM = 0,
 FG_LOOKAT = 1
};

// Define a structure containing view information
class FGViewer : public SGSubsystem {

public:

    enum fgScalingType {  // nominal Field Of View actually applies to ...
	FG_SCALING_WIDTH,       // window width
	FG_SCALING_MAX          // max(width, height)
	// FG_SCALING_G_MEAN,      // geometric_mean(width, height)
	// FG_SCALING_INDEPENDENT  // whole screen
    };

    // Constructor
    FGViewer( fgViewType Type, bool from_model, int from_model_index,
              bool at_model, int at_model_index,
              double damp_roll, double damp_pitch, double damp_heading,
              double x_offset_m, double y_offset_m, double z_offset_m,
              double heading_offset_deg, double pitch_offset_deg,
              double roll_offset_deg,
              double fov_deg, double aspect_ratio_multiplier,
              double target_x_offset_m, double target_y_offset_m,
              double target_z_offset_m, double near_m, bool internal );

    // Destructor
    virtual ~FGViewer( void );

    //////////////////////////////////////////////////////////////////////
    // Part 1: standard SGSubsystem implementation.
    //////////////////////////////////////////////////////////////////////

    virtual void init ();
    virtual void bind ();
    virtual void unbind ();
    void update (double dt);


    //////////////////////////////////////////////////////////////////////
    // Part 2: user settings.
    //////////////////////////////////////////////////////////////////////

    virtual fgViewType getType() const { return _type; }
    virtual void setType( int type );

    virtual bool getInternal() const { return _internal; }
    virtual void setInternal( bool internal );

    // Reference geodetic position of view from position...
    //   These are the actual aircraft position (pilot in
    //   pilot view, model in model view).
    //   FIXME: the model view position (ie target positions) 
    //   should be in the model class.
    virtual void setPosition (double lon_deg, double lat_deg, double alt_ft);
    const SGGeod& getPosition() const { return _position; }

    // Reference geodetic target position...
    virtual void setTargetPosition (double lon_deg, double lat_deg, double alt_ft);
    const SGGeod& getTargetPosition() const { return _target; }



    // Position offsets from reference
    //   These offsets position they "eye" in the scene according to a given
    //   location.  For example in pilot view they are used to position the 
    //   head inside the aircraft.
    //   Note that in pilot view these are applied "before" the orientation 
    //   rotations (see below) so that the orientation rotations have the 
    //   effect of the pilot staying in his seat and "looking out" in 
    //   different directions.
    //   In chase view these are applied "after" the application of the 
    //   orientation rotations listed below.  This has the effect of the 
    //   eye moving around and "looking at" the object (model) from 
    //   different angles.
    virtual SGVec3d getOffset_m () const { return _offset_m; }
    virtual double getXOffset_m () const { return _offset_m.x(); }
    virtual double getYOffset_m () const { return _offset_m.y(); }
    virtual double getZOffset_m () const { return _offset_m.z(); }
    virtual double getTargetXOffset_m () const { return _target_offset_m.x(); }
    virtual double getTargetYOffset_m () const { return _target_offset_m.y(); }
    virtual double getTargetZOffset_m () const { return _target_offset_m.z(); }
    virtual void setXOffset_m (double x_offset_m);
    virtual void setYOffset_m (double y_offset_m);
    virtual void setZOffset_m (double z_offset_m);
    virtual void setTargetXOffset_m (double x_offset_m);
    virtual void setTargetYOffset_m (double y_offset_m);
    virtual void setTargetZOffset_m (double z_offset_m);
    virtual void setPositionOffsets (double x_offset_m,
				     double y_offset_m,
				     double z_offset_m);




    // Reference orientation rotations...
    //   These are rotations that represent the plane attitude effect on
    //   the view (in Pilot view).  IE The view frustrum rotates as the plane
    //   turns, pitches, and rolls.
    //   In model view (lookat/chaseview) these end up changing the angle that
    //   the eye is looking at the ojbect (ie the model).
    //   FIXME: the FGModel class should have its own version of these so that
    //   it can generate it's own model rotations.
    virtual double getRoll_deg () const { return _roll_deg; }
    virtual double getPitch_deg () const {return _pitch_deg; }
    virtual double getHeading_deg () const {return _heading_deg; }
    virtual void setRoll_deg (double roll_deg);
    virtual void setPitch_deg (double pitch_deg);
    virtual void setHeading_deg (double heading_deg);
    virtual void setOrientation (double roll_deg, double pitch_deg, double heading_deg);
    virtual double getTargetRoll_deg () const { return _target_roll_deg; }
    virtual double getTargetPitch_deg () const {return _target_pitch_deg; }
    virtual double getTargetHeading_deg () const {return _target_heading_deg; }
    virtual void setTargetRoll_deg (double roll_deg);
    virtual void setTargetPitch_deg (double pitch_deg);
    virtual void setTargetHeading_deg (double heading_deg);
    virtual void setTargetOrientation (double roll_deg, double pitch_deg, double heading_deg);




    // Orientation offsets rotations from reference orientation.
    // Goal settings are for smooth transition from prior 
    // offset when changing view direction.
    //   These offsets are in ADDITION to the orientation rotations listed 
    //   above.
    //   In pilot view they are applied after the position offsets in order to
    //   give the effect of the pilot looking around.
    //   In lookat view they are applied before the position offsets so that
    //   the effect is the eye moving around looking at the object (ie the model)
    //   from different angles.
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

    const SGVec3d& get_view_pos() { if ( _dirty ) { recalc(); } return _absolute_view_pos; }
    const SGVec3d& getViewPosition() { if ( _dirty ) { recalc(); } return _absolute_view_pos; }
    const SGQuatd& getViewOrientation() { if ( _dirty ) { recalc(); } return mViewOrientation; }
    const SGQuatd& getViewOrientationOffset() { if ( _dirty ) { recalc(); } return mViewOffsetOr; }

    //////////////////////////////////////////////////////////////////////
    // Part 4: View and frustrum data setters and getters
    //////////////////////////////////////////////////////////////////////

    virtual void set_fov( double fov_deg ) {
	_fov_deg = fov_deg;
    }
    virtual double get_fov() const { return _fov_deg; }
    virtual double get_h_fov();    // Get horizontal fov, in degrees.
    virtual double get_v_fov();    // Get vertical fov, in degrees.

    virtual double get_aspect_ratio() const;

    virtual void set_aspect_ratio_multiplier( double m ) {
	_aspect_ratio_multiplier = m;
    }
    virtual double get_aspect_ratio_multiplier() const {
        return _aspect_ratio_multiplier;
    }

    virtual double getNear_m () const { return _ground_level_nearplane_m; }
    inline void setNear_m (double near_m) {
        _ground_level_nearplane_m = near_m;
    }

    //////////////////////////////////////////////////////////////////////
    // Part 5: misc setters and getters
    //////////////////////////////////////////////////////////////////////

    inline void set_dirty() { _dirty = true; }
    inline void set_clean() { _dirty = false; }
    
private:

    //////////////////////////////////////////////////////////////////
    // private data                                                 //
    //////////////////////////////////////////////////////////////////

    // flag forcing a recalc of derived view parameters
    bool _dirty;

    SGQuatd mViewOrientation;
    SGQuatd mViewOffsetOr;
    SGVec3d _absolute_view_pos;

    SGGeod _position;
    SGGeod _target;

    double _roll_deg;
    double _pitch_deg;
    double _heading_deg;
    double _target_roll_deg;
    double _target_pitch_deg;
    double _target_heading_deg;

    SGVec3d _dampTarget; ///< current target value we are damping towards
    SGVec3d _dampOutput; ///< current output of damping filter
    SGVec3d _dampFactor; ///< weighting of the damping filter
    
    // Position offsets from FDM origin.  The X axis is positive
    // out the tail, Y is out the right wing, and Z is positive up.
    // distance in meters
    SGVec3d _offset_m;

    // Target offsets from FDM origin (for "lookat" targets) The X
    // axis is positive out the tail, Y is out the right wing, and Z
    // is positive up.  distance in meters
    SGVec3d _target_offset_m;


    // orientation offsets from reference (_goal* are for smoothed transitions)
    double _roll_offset_deg;
    double _pitch_offset_deg;
    double _heading_offset_deg;
    double _goal_roll_offset_deg;
    double _goal_pitch_offset_deg;
    double _goal_heading_offset_deg;

    // used to set nearplane when at ground level for this view
    double _ground_level_nearplane_m;

    fgViewType _type;
    fgScalingType _scaling_type;

    // internal view (e.g. cockpit) flag
    bool _internal;

    // view is looking from a model
    bool _from_model;
    int _from_model_index;  // number of model (for multi model)

    // view is looking at a model
    bool _at_model;
    int _at_model_index;  // number of model (for multi model)

    // the nominal field of view (angle, in degrees)
    double _fov_deg;

    // default = 1.0, this value is user configurable and is
    // multiplied into the aspect_ratio to get the actual vertical fov
    double _aspect_ratio_multiplier;

    // camera group controled by this view
    osg::ref_ptr<flightgear::CameraGroup> _cameraGroup;
    //////////////////////////////////////////////////////////////////
    // private functions                                            //
    //////////////////////////////////////////////////////////////////

    void recalc ();
    void recalcLookFrom();
    void recalcLookAt();

    void setDampTarget(double h, double p, double r);
    void getDampOutput(double& roll, double& pitch, double& heading);
    
    void updateDampOutput(double dt);
    
    // add to _heading_offset_deg
    inline void incHeadingOffset_deg( double amt ) {
	set_dirty();
	_heading_offset_deg += amt;
    }

    // add to _pitch_offset_deg
    inline void incPitchOffset_deg( double amt ) {
	set_dirty();
	_pitch_offset_deg += amt;
    }

    // add to _roll_offset_deg
    inline void incRollOffset_deg( double amt ) {
	set_dirty();
	_roll_offset_deg += amt;
    }

};


#endif // _VIEWER_HXX
