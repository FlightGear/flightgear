// model.hxx - manage a 3D aircraft model.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain, and comes with no warranty.

#ifndef __ACMODEL_HXX
#define __ACMODEL_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#include <vector>
#include <string>

using std::string;
using std::vector;

#include <osg/ref_ptr>
#include <osg/Group>
#include <osg/Switch>

#include <simgear/structure/subsystem_mgr.hxx>	// for SGSubsystem


// Don't pull in the headers, since we don't need them here.
class SGModelPlacement;
class FGFX;

class FGAircraftModel : public SGSubsystem
{
public:

  FGAircraftModel ();
  virtual ~FGAircraftModel ();

  virtual void init ();
  virtual void bind ();
  virtual void unbind ();
  virtual void update (double dt);
  virtual SGModelPlacement * get3DModel() { return _aircraft; }
  virtual SGVec3f getVelocity() { return _velocity; }

private:

  SGModelPlacement * _aircraft;
  SGVec3f _velocity;
  FGFX * _fx;

  SGPropertyNode * _lon;
  SGPropertyNode * _lat;
  SGPropertyNode * _alt;
  SGPropertyNode * _pitch;
  SGPropertyNode * _roll;
  SGPropertyNode * _heading;
  SGPropertyNode * _speed;

};

#endif // __ACMODEL_HXX
