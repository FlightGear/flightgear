// model.hxx - manage a 3D aircraft model.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain, and comes with no warranty.

#ifndef __MODEL_HXX
#define __MODEL_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#include <vector>

SG_USING_STD(vector);

#include <plib/sg.h>
#include <plib/ssg.h>

#include <simgear/misc/props.hxx>


// Don't pull in the headers, since we don't need them here.
class ssgBranch;
class ssgCutout;
class ssgEntity;
class ssgRangeSelector;
class ssgSelector;
class ssgTransform;

class SGInterpTable;
class FGCondition;
class FGLocation;


// Has anyone done anything *really* stupid, like making min and max macros?
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif


/**
 * Load a 3D model with or without XML wrapper.
 *
 * If the path ends in ".xml", then it will be used as a property-
 * list wrapper to add animations to the model.
 *
 * Subsystems should not normally invoke this function directly;
 * instead, they should use the FGModelLoader declared in loader.hxx.
 */
ssgBranch * fgLoad3DModel (const string &path);



//////////////////////////////////////////////////////////////////////
// Animation classes
//////////////////////////////////////////////////////////////////////

/**
 * Abstract base class for all animations.
 */
class Animation :  public ssgBase
{
public:

  Animation (SGPropertyNode_ptr props, ssgBranch * branch);

  virtual ~Animation ();

  /**
   * Get the SSG branch holding the animation.
   */
  virtual ssgBranch * getBranch () { return _branch; }

  /**
   * Initialize the animation, after children have been added.
   */
  virtual void init ();

  /**
   * Update the animation.
   */
  virtual void update ();

protected:

  ssgBranch * _branch;

};


/**
 * A no-op animation.
 */
class NullAnimation : public Animation
{
public:
  NullAnimation (SGPropertyNode_ptr props);
  virtual ~NullAnimation ();
};


/**
 * A range, or level-of-detail (LOD) animation.
 */
class RangeAnimation : public Animation
{
public:
  RangeAnimation (SGPropertyNode_ptr props);
  virtual ~RangeAnimation ();
};


/**
 * Animation to turn and face the screen.
 */
class BillboardAnimation : public Animation
{
public:
  BillboardAnimation (SGPropertyNode_ptr props);
  virtual ~BillboardAnimation ();
};


/**
 * Animation to select alternative versions of the same object.
 */
class SelectAnimation : public Animation
{
public:
  SelectAnimation (SGPropertyNode_ptr props);
  virtual ~SelectAnimation ();
  virtual void update ();
private:
  FGCondition * _condition;
};


/**
 * Animation to spin an object around a center point.
 *
 * This animation rotates at a specific velocity.
 */
class SpinAnimation : public Animation
{
public:
  SpinAnimation (SGPropertyNode_ptr props);
  virtual ~SpinAnimation ();
  virtual void update ();
private:
  SGPropertyNode_ptr _prop;
  double _factor;
  double _position_deg;
  double _last_time_sec;
  sgMat4 _matrix;
  sgVec3 _center;
  sgVec3 _axis;
};


/**
 * Animation to draw objects for a specific amount of time each.
 */
class TimedAnimation : public Animation
{
public:
    TimedAnimation (SGPropertyNode_ptr props);
    virtual ~TimedAnimation ();
    virtual void update ();
private:
    double _duration_sec;
    double _last_time_sec;
    int _step;
};


/**
 * Animation to rotate an object around a center point.
 *
 * This animation rotates to a specific position.
 */
class RotateAnimation : public Animation
{
public:
  RotateAnimation (SGPropertyNode_ptr props);
  virtual ~RotateAnimation ();
  virtual void update ();
private:
  SGPropertyNode_ptr _prop;
  double _offset_deg;
  double _factor;
  SGInterpTable * _table;
  bool _has_min;
  double _min_deg;
  bool _has_max;
  double _max_deg;
  double _position_deg;
  sgMat4 _matrix;
  sgVec3 _center;
  sgVec3 _axis;
};


/**
 * Animation to slide along an axis.
 */
class TranslateAnimation : public Animation
{
public:
  TranslateAnimation (SGPropertyNode_ptr props);
  virtual ~TranslateAnimation ();
  virtual void update ();
private:
  SGPropertyNode_ptr _prop;
  double _offset_m;
  double _factor;
  SGInterpTable * _table;
  bool _has_min;
  double _min_m;
  bool _has_max;
  double _max_m;
  double _position_m;
  sgMat4 _matrix;
  sgVec3 _axis;
};



////////////////////////////////////////////////////////////////////////
// Model placement.
////////////////////////////////////////////////////////////////////////

/**
 * A wrapper for a model with a definite placement.
 */
class FGModelPlacement
{
public:

  FGModelPlacement ();
  virtual ~FGModelPlacement ();

  virtual void init (const string &path);
  virtual void update ();

  virtual ssgEntity * getSceneGraph () { return (ssgEntity *)_selector; }

  virtual FGLocation * getFGLocation () { return _location; }

  virtual bool getVisible () const;
  virtual void setVisible (bool visible);

  virtual double getLongitudeDeg () const { return _lon_deg; }
  virtual double getLatitudeDeg () const { return _lat_deg; }
  virtual double getElevationFt () const { return _elev_ft; }

  virtual void setLongitudeDeg (double lon_deg);
  virtual void setLatitudeDeg (double lat_deg);
  virtual void setElevationFt (double elev_ft);
  virtual void setPosition (double lon_deg, double lat_deg, double elev_ft);

  virtual double getRollDeg () const { return _roll_deg; }
  virtual double getPitchDeg () const { return _pitch_deg; }
  virtual double getHeadingDeg () const { return _heading_deg; }

  virtual void setRollDeg (double roll_deg);
  virtual void setPitchDeg (double pitch_deg);
  virtual void setHeadingDeg (double heading_deg);
  virtual void setOrientation (double roll_deg, double pitch_deg,
                               double heading_deg);
  
  // Addition by Diarmuid Tyson for Multiplayer Support
  // Allows multiplayer to get players position transform
  virtual const sgVec4 *get_POS() { return POS; }

private:

                                // Geodetic position
  double _lon_deg;
  double _lat_deg;
  double _elev_ft;

                                // Orientation
  double _roll_deg;
  double _pitch_deg;
  double _heading_deg;

  ssgSelector * _selector;
  ssgTransform * _position;

                                // Location
  FGLocation * _location;


  // Addition by Diarmuid Tyson for Multiplayer Support
  // Moved from update method
  // POS for transformation Matrix
  sgMat4 POS;

};

#endif // __MODEL_HXX
