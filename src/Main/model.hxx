// model.hxx - manage a 3D aircraft model.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain, and comes with no warranty.

#ifndef __MODEL_HXX
#define __MODEL_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

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

  SGPropertyNode * _props;
  ssgEntity * _object;
  ssgSelector * _selector;
  ssgTransform * _position;

  SGTimeStamp _last_timestamp;
  SGTimeStamp _current_timestamp;

  ssgTransform * _prop_position;

};

extern FGAircraftModel current_model;

#endif // __MODEL_HXX

