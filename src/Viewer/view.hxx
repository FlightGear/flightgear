// view.hxx -- class for managing a view in the flightgear world.
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


#include <simgear/compiler.h>
#include <simgear/constants.h>

#include <simgear/props/props.hxx>
#include <simgear/props/tiedpropertylist.hxx>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/math/SGMath.hxx>

#define FG_FOV_MIN 0.1
#define FG_FOV_MAX 179.9

namespace flightgear
{

// Define a structure containing view information
class View : public SGSubsystem
{
public:
    enum ScalingType {  // nominal Field Of View actually applies to ...
        FG_SCALING_WIDTH,       // window width
        FG_SCALING_MAX          // max(width, height)
        // FG_SCALING_G_MEAN,      // geometric_mean(width, height)
        // FG_SCALING_INDEPENDENT  // whole screen
    };

    enum ViewType {
        FG_LOOKFROM = 0,
        FG_LOOKAT = 1
    };

    // view_index is to allow us to look up corresponding view information for
    // multiplayer aircraft.
    static View* createFromProperties(SGPropertyNode_ptr props, int view_index=-1);

    // Destructor
    virtual ~View();

    //////////////////////////////////////////////////////////////////////
    // Part 1: standard SGSubsystem implementation.
    //////////////////////////////////////////////////////////////////////

    void init() override;
    void bind() override;
    void unbind() override;
    void update(double dt) override;

    // Subsystem identification.
    static const char* staticSubsystemClassId() { return "view"; }

    //////////////////////////////////////////////////////////////////////
    // Part 2: user settings.
    //////////////////////////////////////////////////////////////////////

    void resetOffsetsAndFOV();

    ViewType getType() const { return _type; }
    void setType( int type );

    bool getInternal() const { return _internal; }
    void setInternal( bool internal );

    const std::string& getName() const { return _name; }

    // Reference geodetic position of view from position...
    //   These are the actual aircraft position (pilot in
    //   pilot view, model in model view).
    //   FIXME: the model view position (ie target positions)
    //   should be in the model class.


    const SGGeod& getPosition() const { return _position; }

    // Reference geodetic target position...
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
    SGVec3d getOffset_m () const { return _offset_m; }
    double getXOffset_m () const { return _offset_m.x(); }
    double getYOffset_m () const { return _offset_m.y(); }
    double getZOffset_m () const { return _offset_m.z(); }
    double getTargetXOffset_m () const { return _target_offset_m.x(); }
    double getTargetYOffset_m () const { return _target_offset_m.y(); }
    double getTargetZOffset_m () const { return _target_offset_m.z(); }
    void setXOffset_m (double x_offset_m);
    void setYOffset_m (double y_offset_m);
    void setZOffset_m (double z_offset_m);
    void setTargetXOffset_m (double x_offset_m);
    void setTargetYOffset_m (double y_offset_m);
    void setTargetZOffset_m (double z_offset_m);
    void setPositionOffsets (double x_offset_m,
                             double y_offset_m,
                             double z_offset_m);
    double getAdjustXOffset_m () const { return _adjust_offset_m.x(); }
    double getAdjustYOffset_m () const { return _adjust_offset_m.y(); }
    double getAdjustZOffset_m () const { return _adjust_offset_m.z(); }
    void setAdjustXOffset_m (double x_adjust_offset_m);
    void setAdjustYOffset_m (double y_adjust_offset_m);
    void setAdjustZOffset_m (double z_adjust_offset_m);

    // Reference orientation rotations...
    //   These are rotations that represent the plane attitude effect on
    //   the view (in Pilot view).  IE The view frustrum rotates as the plane
    //   turns, pitches, and rolls.
    //   In model view (lookat/chaseview) these end up changing the angle that
    //   the eye is looking at the ojbect (ie the model).
    //   FIXME: the FGModel class should have its own version of these so that
    //   it can generate it's own model rotations.
    double getHeading_deg () const {return _heading_deg; }


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
    double getRollOffset_deg () const { return _roll_offset_deg; }
    double getPitchOffset_deg () const { return _pitch_offset_deg; }
    double getHeadingOffset_deg () const { return _heading_offset_deg; }

    void setGoalHeadingOffset_deg (double goal_heading_offset_deg);
    void setHeadingOffset_deg (double heading_offset_deg);

    //////////////////////////////////////////////////////////////////////
    // Part 3: output vectors and matrices in FlightGear coordinates.
    //////////////////////////////////////////////////////////////////////

    // Vectors and positions...

    const SGVec3d& getViewPosition() { if ( _dirty ) { recalc(); } return _absolute_view_pos; }
    const SGQuatd& getViewOrientation() { if ( _dirty ) { recalc(); } return mViewOrientation; }
    const SGQuatd& getViewOrientationOffset() { if ( _dirty ) { recalc(); } return mViewOffsetOr; }

    //////////////////////////////////////////////////////////////////////
    // Part 4: View and frustrum data setters and getters
    //////////////////////////////////////////////////////////////////////

    double get_fov() const { return _fov_deg; }
    double get_h_fov();    // Get horizontal fov, in degrees.
    double get_v_fov();    // Get vertical fov, in degrees.

    // this is currently just a wrapper for the default CameraGroups' aspect
    double get_aspect_ratio() const;

    //////////////////////////////////////////////////////////////////////
    // Part 5: misc setters and getters
    //////////////////////////////////////////////////////////////////////

    void set_dirty() { _dirty = true; }

private:
    // Constructor
    View( ViewType Type, bool from_model, int from_model_index,
         bool at_model, int at_model_index,
         double damp_roll, double damp_pitch, double damp_heading,
         double x_offset_m, double y_offset_m, double z_offset_m,
         double heading_offset_deg, double pitch_offset_deg,
         double roll_offset_deg,
         double fov_deg, double aspect_ratio_multiplier,
         double target_x_offset_m, double target_y_offset_m,
         double target_z_offset_m, double near_m, bool internal,
         bool lookat_agl, double lookat_agl_damping, int view_index );

    void set_clean() { _dirty = false; }

    void setHeadingOffset_deg_property (double heading_offset_deg);
    void setPitchOffset_deg_property(double pitch_offset_deg);
    void setRollOffset_deg_property(double roll_offset_deg);

    void setPosition (const SGGeod& geod);
    void setTargetPosition (const SGGeod& geod);

    double getAbsolutePosition_x() const;
    double getAbsolutePosition_y() const;
    double getAbsolutePosition_z() const;

    double getRawOrientation_w() const;
    double getRawOrientation_x() const;
    double getRawOrientation_y() const;
    double getRawOrientation_z() const;

    // quaternion accessors, for debugging:
    double getFrame_w() const;
    double getFrame_x() const;
    double getFrame_y() const;
    double getFrame_z() const;

    double getOrientation_w() const;
    double getOrientation_x() const;
    double getOrientation_y() const;
    double getOrientation_z() const;

    double getOrOffset_w() const;
    double getOrOffset_x() const;
    double getOrOffset_y() const;
    double getOrOffset_z() const;

    double getLon_deg() const;
    double getLat_deg() const;
    double getElev_ft() const;

    // Reference orientation rotations...
    //   These are rotations that represent the plane attitude effect on
    //   the view (in Pilot view).  IE The view frustrum rotates as the plane
    //   turns, pitches, and rolls.
    //   In model view (lookat/chaseview) these end up changing the angle that
    //   the eye is looking at the ojbect (ie the model).
    //   FIXME: the FGModel class should have its own version of these so that
    //   it can generate it's own model rotations.
    double getRoll_deg () const { return _roll_deg; }
    double getPitch_deg () const {return _pitch_deg; }
    void setRoll_deg (double roll_deg);
    void setPitch_deg (double pitch_deg);
    void setHeading_deg (double heading_deg);
    void setOrientation (double roll_deg, double pitch_deg, double heading_deg);
    double getTargetRoll_deg () const { return _target_roll_deg; }
    double getTargetPitch_deg () const {return _target_pitch_deg; }
    double getTargetHeading_deg () const {return _target_heading_deg; }
    void setTargetRoll_deg (double roll_deg);
    void setTargetPitch_deg (double pitch_deg);
    void setTargetHeading_deg (double heading_deg);
    void setTargetOrientation (double roll_deg, double pitch_deg, double heading_deg);

    void handleAGL();

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
    double getGoalRollOffset_deg () const { return _goal_roll_offset_deg; }
    double getGoalPitchOffset_deg () const { return _goal_pitch_offset_deg; }
    double getGoalHeadingOffset_deg () const {return _goal_heading_offset_deg; }
    void setRollOffset_deg (double roll_offset_deg);
    void setPitchOffset_deg (double pitch_offset_deg);
    void setGoalRollOffset_deg (double goal_roll_offset_deg);
    void setGoalPitchOffset_deg (double goal_pitch_offset_deg);
    void setOrientationOffsets (double roll_offset_deg,
                                double heading_offset_deg,
                                double pitch_offset_deg);

    void set_aspect_ratio_multiplier( double m ) {
        _aspect_ratio_multiplier = m;
    }
    double get_aspect_ratio_multiplier() const {
        return _aspect_ratio_multiplier;
    }

    double getNear_m () const { return _ground_level_nearplane_m; }
    void setNear_m (double near_m) {
        _ground_level_nearplane_m = near_m;
    }

    void set_fov( double fov_deg ) {
        _fov_deg = fov_deg;
    }

    double get_fov_user() const { return _fov_user_deg; }
    void set_fov_user( double fov_deg ) { _fov_user_deg = fov_deg; }
    
    //////////////////////////////////////////////////////////////////
    // private data                                                 //
    //////////////////////////////////////////////////////////////////

    SGPropertyNode_ptr _config;
    std::string _name, _typeString;

    // flag forcing a recalc of derived view parameters
    bool _dirty;

    simgear::TiedPropertyList _tiedProperties;

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

    double _configRollOffsetDeg,
        _configHeadingOffsetDeg,
        _configPitchOffsetDeg;

    SGVec3d _dampTarget; ///< current target value we are damping towards
    SGVec3d _dampOutput; ///< current output of damping filter
    SGVec3d _dampFactor; ///< weighting of the damping filter
    
    /* Generic damping support. */
    struct Damping {
    
       Damping(double factor, double min, double max);
       void     setTarget(double target);
       void     update(double dt, void* id);
       double   get();
       void     updateTarget(double& io);
       void     reset(double target);

       private:
           void*    _id;
           double   _min;
           double   _max;
           double   _target;
           double   _factor;
           double   _current;
    };
    
    Damping _lookat_agl_damping;
    double  _lookat_agl_ground_altitude;
    
    // Position offsets from FDM origin.  The X axis is positive
    // out the tail, Y is out the right wing, and Z is positive up.
    // distance in meters
    SGVec3d _offset_m;
    SGVec3d _configOffset_m;
    
    SGVec3d _adjust_offset_m;

    // Target offsets from FDM origin (for "lookat" targets) The X
    // axis is positive out the tail, Y is out the right wing, and Z
    // is positive up.  distance in meters
    SGVec3d _target_offset_m;
    SGVec3d _configTargetOffset_m;

    // orientation offsets from reference (_goal* are for smoothed transitions)
    double _roll_offset_deg;
    double _pitch_offset_deg;
    double _heading_offset_deg;
    double _goal_roll_offset_deg;
    double _goal_pitch_offset_deg;
    double _goal_heading_offset_deg;

    // used to set nearplane when at ground level for this view
    double _ground_level_nearplane_m;

    ViewType _type;
    ScalingType _scaling_type;

    // internal view (e.g. cockpit) flag
    bool _internal;
    
    // Dynamically update view angle and field of view so that we always
    // include the target and the ground below it.
    bool _lookat_agl;
    
    int _view_index;

    // view is looking from a model
    bool _from_model;
    int _from_model_index;  // number of model (for multi model)

    // view is looking at a model
    bool _at_model;
    int _at_model_index;  // number of model (for multi model)
    
    // Field of view as requested by user. Usually copied directly into the
    // actual field of view, except for Tower AGL view.
    double _fov_user_deg;

    // the nominal field of view (angle, in degrees)
    double _fov_deg;
    double _configFOV_deg;
    // default = 1.0, this value is user configurable and is
    // multiplied into the aspect_ratio to get the actual vertical fov
    double _aspect_ratio_multiplier;

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
}; // of class View

} // of namespace flightgear

#endif // _VIEWER_HXX
