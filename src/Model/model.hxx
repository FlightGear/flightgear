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


// Don't pull in the headers, since we don't need them here.
class ssgBranch;
class ssgCutout;
class ssgEntity;
class ssgRangeSelector;
class ssgSelector;
class ssgTransform;

class SGPropertyNode;
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

class FG3DModel
{
public:

  FG3DModel ();
  virtual ~FG3DModel ();

  virtual void init (const string &path);
  virtual void update (double dt);

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

  virtual ssgEntity * getSceneGraph () const { return (ssgEntity *)_selector; }

  virtual FGLocation * getFGLocation () const { return _location; }

private:

  class Animation;
  Animation * make_animation (const char * object_name, SGPropertyNode * node);

				// Geodetic position
  double _lon_deg;
  double _lat_deg;
  double _elev_ft;

				// Orientation
  double _roll_deg;
  double _pitch_deg;
  double _heading_deg;

				// Animations
  vector <Animation *> _animations;

				// Scene graph
  ssgBranch * _model;
  ssgSelector * _selector;
  ssgTransform * _position;

				// Location
  FGLocation * _location;


  
  //////////////////////////////////////////////////////////////////////
  // Internal classes for individual animations.
  //////////////////////////////////////////////////////////////////////

  /**
   * Abstract base class for all animations.
   */
  class Animation
  {
  public:

    Animation ();

    virtual ~Animation ();

    /**
     * Initialize the animation.
     *
     * @param object The object to animate.
     * @param props The property node with configuration information.
     */
    virtual void init (ssgEntity * object, SGPropertyNode * props) = 0;


    /**
     * Update the animation.
     *
     * @param dt The elapsed time in seconds since the last call.
     */
    virtual void update (double dt) = 0;

  };


  /**
   * A no-op animation.
   */
  class NullAnimation : public Animation
  {
  public:
    NullAnimation ();
    virtual ~NullAnimation ();
    virtual void init (ssgEntity * object, SGPropertyNode * props);
    virtual void update (double dt);
  private:
    ssgBranch * _branch;
  };

  
  /**
   * A range, or level-of-detail (LOD) animation.
   */
  class RangeAnimation : public Animation
  {
  public:
    RangeAnimation ();
    virtual ~RangeAnimation ();
    virtual void init (ssgEntity * object, SGPropertyNode * props);
    virtual void update (double dt);
  private:
    ssgRangeSelector * _branch;
  };


  /**
   * Animation to turn and face the screen.
   */
  class BillboardAnimation : public Animation
  {
  public:
    BillboardAnimation ();
    virtual ~BillboardAnimation ();
    virtual void init (ssgEntity * object, SGPropertyNode * props);
    virtual void update (double dt);
  private:
    ssgCutout * _branch;
  };


  /**
   * Animation to select alternative versions of the same object.
   */
  class SelectAnimation : public Animation
  {
  public:
    SelectAnimation ();
    virtual ~SelectAnimation ();
    virtual void init (ssgEntity * object, SGPropertyNode * props);
    virtual void update (double dt);
  private:
    FGCondition * _condition;
    ssgSelector * _selector;
  };


  /**
   * Animation to spin an object around a center point.
   *
   * This animation rotates at a specific velocity.
   */
  class SpinAnimation : public Animation
  {
  public:
    SpinAnimation ();
    virtual ~SpinAnimation ();
    virtual void init (ssgEntity * object, SGPropertyNode * props);
    virtual void update (double dt);
  private:
    SGPropertyNode * _prop;
    double _factor;
    double _position_deg;
    sgMat4 _matrix;
    sgVec3 _center;
    sgVec3 _axis;
    ssgTransform * _transform;
  };


  /**
   * Animation to rotate an object around a center point.
   *
   * This animation rotates to a specific position.
   */
  class RotateAnimation : public Animation
  {
  public:
    RotateAnimation ();
    virtual ~RotateAnimation ();
    virtual void init (ssgEntity * object, SGPropertyNode * props);
    virtual void update (double dt);
  private:
    SGPropertyNode * _prop;
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
    ssgTransform * _transform;
  };


  /**
   * Animation to slide along an axis.
   */
  class TranslateAnimation : public Animation
  {
  public:
    TranslateAnimation ();
    virtual ~TranslateAnimation ();
    virtual void init (ssgEntity * object, SGPropertyNode * props);
    virtual void update (double dt);
  private:
    SGPropertyNode * _prop;
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
    ssgTransform * _transform;
  };


};

#endif // __MODEL_HXX



