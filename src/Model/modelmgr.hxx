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

#include <simgear/compiler.h>	// for SG_USING_STD

#include <Main/fgfs.hxx>	// for FGSubsystem

SG_USING_STD(vector);

// Don't pull in headers, since we don't need them here.
class ssgSelector;
class SGPropertyNode;
class FGModelPlacement;


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
  virtual void update (double dt);

  virtual void draw ();

private:

  struct Instance
  {
    Instance ();
    virtual ~Instance ();
    FGModelPlacement * model;
    SGPropertyNode * lon_deg_node;
    SGPropertyNode * lat_deg_node;
    SGPropertyNode * elev_ft_node;
    SGPropertyNode * roll_deg_node;
    SGPropertyNode * pitch_deg_node;
    SGPropertyNode * heading_deg_node;
  };

  vector<Instance *> _instances;

  ssgSelector * _selector;

};

#endif // __MODELMGR_HXX
