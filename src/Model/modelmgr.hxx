// model-mgr.hxx - manage user-specified 3D models.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain, and comes with no warranty.

#ifndef __MODELMGR_HXX
#define __MODELMGR_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#include <vector>

#include <plib/ssg.h>

#include <simgear/compiler.h>
#include <simgear/misc/props.hxx>

#include <Main/fgfs.hxx>

#include "model.hxx"

SG_USING_STD(vector);


/**
 * Manage a list of user-specified models.
 */
class FGModelMgr : public FGSubsystem
{
public:
  FGModelMgr ();
  virtual ~FGModelMgr ();

  virtual void init ();
  virtual void bind ();
  virtual void unbind ();
  virtual void update (int dt);

  virtual void draw ();

private:

  struct Instance
  {
    Instance ();
    virtual ~Instance ();
    FG3DModel * model;
    SGPropertyNode * lon_deg_node;
    SGPropertyNode * lat_deg_node;
    SGPropertyNode * elev_ft_node;
    SGPropertyNode * roll_deg_node;
    SGPropertyNode * pitch_deg_node;
    SGPropertyNode * heading_deg_node;
  };

  vector<Instance *> _instances;

  ssgRoot * _scene;
  float _nearplane;
  float _farplane;

};

#endif // __MODELMGR_HXX



