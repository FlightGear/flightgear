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

SG_USING_STD(string);
SG_USING_STD(vector);

#include <simgear/misc/props.hxx>

#include <Main/fgfs.hxx>

#include "model.hxx"


class FGAircraftModel : public FGSubsystem
{
public:

  FGAircraftModel ();
  virtual ~FGAircraftModel ();

  virtual void init ();
  virtual void bind ();
  virtual void unbind ();
  virtual void update (int dt);
  virtual void draw ();
  virtual FG3DModel * get3DModel() { return _aircraft; }

private:

  FG3DModel * _aircraft;
  ssgRoot * _scene;
  float _nearplane;
  float _farplane;

};

#endif // __ACMODEL_HXX


