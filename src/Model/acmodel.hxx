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

private:

  FG3DModel * _aircraft;

};

extern FGAircraftModel current_model;

#endif // __ACMODEL_HXX

