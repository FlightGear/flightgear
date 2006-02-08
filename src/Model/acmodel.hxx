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

SG_USING_STD(string);
SG_USING_STD(vector);

#include <simgear/structure/subsystem_mgr.hxx>	// for SGSubsystem
#include <simgear/structure/ssgSharedPtr.hxx>


// Don't pull in the headers, since we don't need them here.
class ssgRoot;
class ssgSelector;
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
  virtual void draw ();
  virtual SGModelPlacement * get3DModel() { return _aircraft; }
  void select( bool s ) { _selector->select( s ? 0xffffffff : 0 ); }

private:

  SGModelPlacement * _aircraft;
  ssgSharedPtr<ssgSelector> _selector;
  ssgSharedPtr<ssgRoot> _scene;
  float _nearplane;
  float _farplane;

};

#endif // __ACMODEL_HXX
