// acmodel.hxx - manage a 3D aircraft model.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain, and comes with no warranty.

#ifndef __ACMODEL_HXX
#define __ACMODEL_HXX 1

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
  virtual void reinit ();
  virtual void bind ();
  virtual void unbind ();
  virtual void update (double dt);
  virtual SGModelPlacement * get3DModel() { return _aircraft; }
  virtual SGVec3d& getVelocity() { return _velocity; }

private:
  void deinit ();
  
  SGModelPlacement * _aircraft;
  SGVec3d _velocity;
  SGSharedPtr<FGFX>  _fx;

  SGPropertyNode_ptr _lon;
  SGPropertyNode_ptr _lat;
  SGPropertyNode_ptr _alt;
  SGPropertyNode_ptr _pitch;
  SGPropertyNode_ptr _roll;
  SGPropertyNode_ptr _heading;
  SGPropertyNode_ptr _speed_n;
  SGPropertyNode_ptr _speed_e;
  SGPropertyNode_ptr _speed_d;
};

#endif // __ACMODEL_HXX
