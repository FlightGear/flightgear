// model.cxx - manage a 3D aircraft model.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain, and comes with no warranty.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <plib/sg.h>
#include <plib/ssg.h>

#include <simgear/compiler.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/exception.hxx>

#include "globals.hxx"
#include "fg_props.hxx"
#include "viewmgr.hxx"
#include "model.hxx"

extern unsigned long int fgSimTime; // FIXME: this is ugly

extern ssgRoot * scene;		// FIXME: from main.cxx

FGAircraftModel current_model;	// FIXME: add to globals


static ssgEntity *
find_named_node (ssgEntity * node, const string &name)
{
  char * node_name = node->getName();
  if (node_name != 0 && name == node_name)
    return node;
  else if (node->isAKindOf(ssgTypeBranch())) {
    int nKids = node->getNumKids();
    for (int i = 0; i < nKids; i++) {
      ssgEntity * result =
	find_named_node(((ssgBranch*)node)->getKid(i), name);
      if (result != 0)
	return result;
    }
  } 
  return 0;
}

FGAircraftModel::FGAircraftModel ()
  : _object(0),
    _selector(new ssgSelector),
    _position(new ssgTransform),
    _prop_position(0)
{
}

FGAircraftModel::~FGAircraftModel ()
{
  // since the nodes are attached to the scene graph, they'll be
  // deleted automatically
}

void 
FGAircraftModel::init ()
{
  // TODO: optionally load an XML file with a pointer to the 3D object
  // and placement and animation info

				// Load the 3D aircraft object itself
  SGPath path = globals->get_fg_root();
  path.append(fgGetString("/sim/model/path", "Models/Geometry/glider.ac"));
  ssgTexturePath((char *)path.dir().c_str());
  _object = ssgLoad((char *)path.c_str());
  if (_object == 0) {
    _object = ssgLoad((char *)"Models/Geometry/glider.ac");
    if (_object == 0)
      throw sg_exception("Failed to load an aircraft model");
  }

				// Find the propeller
  ssgEntity * prop_node = find_named_node(_object, "Propeller");
  if (prop_node != 0) {
    _prop_position = new ssgTransform;
    int nParents = prop_node->getNumParents();
    _prop_position->addKid(prop_node);
    for (int i = 0; i < nParents; i++) {
      ssgBranch * parent = prop_node->getParent(i);
      parent->replaceKid(prop_node, _prop_position);
    }
  }

				// Set up the alignment node
  ssgTransform * align = new ssgTransform;
  align->addKid(_object);
  sgMat4 rot_matrix;
  sgMat4 off_matrix;
  sgMat4 res_matrix;
  float h_rot = fgGetFloat("/sim/model/heading-offset-deg", 0.0);
  float p_rot = fgGetFloat("/sim/model/roll-offset-deg", 0.0);
  float r_rot = fgGetFloat("/sim/model/pitch-offset-deg", 0.0);
  float x_off = fgGetFloat("/sim/model/x-offset-m", 0.0);
  float y_off = fgGetFloat("/sim/model/y-offset-m", 0.0);
  float z_off = fgGetFloat("/sim/model/z-offset-m", 0.0);
  sgMakeRotMat4(rot_matrix, h_rot, p_rot, r_rot);
  sgMakeTransMat4(off_matrix, x_off, y_off, z_off);
  sgMultMat4(res_matrix, off_matrix, rot_matrix);
  align->setTransform(res_matrix);

				// Set up the position node
  _position->addKid(align);

				// Set up the selector node
  _selector->addKid(_position);
  _selector->clrTraversalMaskBits(SSGTRAV_HOT);
  scene->addKid(_selector);
}

void 
FGAircraftModel::bind ()
{
}

void 
FGAircraftModel::unbind ()
{
}

void
FGAircraftModel::update (int dt)
{
  // START TEMPORARY KLUDGE
  static float prop_rotation = 0;
  static sgMat4 prop_matrix;

  _current_timestamp.stamp();
  long ms = (_current_timestamp - _last_timestamp) / 1000;
  _last_timestamp.stamp();

  double rpms = fgGetDouble("/engines/engine[0]/rpm") / 60000.0;
  prop_rotation += (ms * rpms * 360);
  while (prop_rotation >= 360)
    prop_rotation -= 360;
  // END TEMPORARY KLUDGE

  if (globals->get_viewmgr()->get_current() == 0
      && !fgGetBool("/sim/model/enable-interior")) {
    _selector->select(false);
  } else {
				// TODO: use correct alignment in pilot
				// view.
    _selector->select(true);
    FGViewerRPH *pilot_view =
      (FGViewerRPH *)globals->get_viewmgr()->get_view( 0 );
    
    sgMat4 sgTRANS;
    sgMakeTransMat4( sgTRANS, pilot_view->get_view_pos() );
    
    sgVec3 ownship_up;
    sgSetVec3( ownship_up, 0.0, 0.0, 1.0);
    
    sgMat4 sgROT;
    sgMakeRotMat4( sgROT, -90.0, ownship_up );
    
    sgMat4 sgTUX;
    sgCopyMat4( sgTUX, sgROT );
    sgPostMultMat4( sgTUX, pilot_view->get_VIEW_ROT() );
    sgPostMultMat4( sgTUX, sgTRANS );
    
    sgCoord tuxpos;
    sgSetCoord( &tuxpos, sgTUX );
    _position->setTransform( &tuxpos );

    // START TEMPORARY KLUDGE
    if (_prop_position != 0) {
      double offset = fgGetDouble("/tmp/offset", -.75);
      sgMat4 tmp;
      sgMakeTransMat4(prop_matrix, 0, 0, offset);
      sgMakeRotMat4(tmp, 0, 0, prop_rotation);
      sgPostMultMat4(prop_matrix, tmp);
      sgMakeTransMat4(tmp, 0, 0, -offset);
      sgPostMultMat4(prop_matrix, tmp);
      _prop_position->setTransform(prop_matrix);
    }
    // END_TEMPORARY KLUDGE
  }
}

// end of model.cxx
