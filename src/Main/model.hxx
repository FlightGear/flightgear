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

  struct Animation
  {
    enum Type {
      None,
      Spin
    };
    string name;
    Type type;
    ssgTransform * transform;
    sgMat4 matrix;
    SGPropertyNode * prop;
    float position;
    float center_x;
    float center_y;
    float center_z;
    float axis_x;
    float axis_y;
    float axis_z;
  };

  Animation read_animation (const SGPropertyNode * node);
  void do_animation (Animation &animation, long elapsed_ms);

  SGPropertyNode * _props;
  ssgEntity * _model;
  ssgSelector * _selector;
  ssgTransform * _position;

  SGTimeStamp _last_timestamp;
  SGTimeStamp _current_timestamp;

  vector<Animation> _animations;

};

extern FGAircraftModel current_model;

#endif // __MODEL_HXX

