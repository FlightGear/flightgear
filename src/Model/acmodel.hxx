#// model.hxx - manage a 3D aircraft model.
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

SG_USING_STD(string);
SG_USING_STD(vector);

#include <osg/ref_ptr>
#include <osg/Group>
#include <osg/Switch>

#include <simgear/structure/subsystem_mgr.hxx>	// for SGSubsystem


// Don't pull in the headers, since we don't need them here.
class SGModelPlacement;


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
  void select( bool s ) { _selector->setValue( 0, s ); }

private:

  SGModelPlacement * _aircraft;
  osg::ref_ptr<osg::Switch> _selector;
  osg::ref_ptr<osg::Group> _scene;
  float _nearplane;
  float _farplane;

};

#endif // __ACMODEL_HXX
