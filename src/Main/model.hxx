// model.hxx - manage a 3D aircraft model.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain, and comes with no warranty.

#ifndef __MODEL_HXX
#define __MODEL_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#include <string>
#include <vector>

SG_USING_STD(string);
SG_USING_STD(vector);

#include "fgfs.hxx"
#include <simgear/misc/props.hxx>
#include <simgear/timing/timestamp.hxx>

// Has anyone done anything *really* stupid, like making min and max macros?
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

class FGAircraftModel : public FGSubsystem
{
public:

  FGAircraftModel ();
  virtual ~FGAircraftModel ();

  virtual void init ();
  virtual void bind ();
  virtual void unbind ();
  virtual void update (int dt);

private:

  class Animation;

  Animation * make_animation (const char * object_name, SGPropertyNode * node);

  ssgEntity * _model;
  ssgSelector * _selector;
  ssgTransform * _position;

  vector <Animation *> _animations;


  
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
     * @param dt The elapsed time in milliseconds since the last call.
     */
    virtual void update (int dt) = 0;

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
    virtual void update (int dt);
  private:
    ssgBranch * _branch;
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
    virtual void update (int dt);
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
    virtual void update (int dt);
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
    virtual void update (int dt);
  private:
    SGPropertyNode * _prop;
    double _offset_deg;
    double _factor;
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
    virtual void update (int dt);
  private:
    SGPropertyNode * _prop;
    double _offset_m;
    double _factor;
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

extern FGAircraftModel current_model;

#endif // __MODEL_HXX

