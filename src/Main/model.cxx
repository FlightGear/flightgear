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
#include <simgear/misc/sg_path.hxx>

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
  : _props(new SGPropertyNode),
    _model(0),
    _selector(new ssgSelector),
    _position(new ssgTransform)
{
}

FGAircraftModel::~FGAircraftModel ()
{
  delete _props;
  // since the nodes are attached to the scene graph, they'll be
  // deleted automatically
}

void 
FGAircraftModel::init ()
{
  // TODO: optionally load an XML file with a pointer to the 3D object
  // and placement and animation info

  SG_LOG(SG_INPUT, SG_INFO, "Initializing aircraft 3D model");

				// Load the 3D aircraft object itself
  SGPath path = globals->get_fg_root();
  path.append(fgGetString("/sim/model/path", "Models/Geometry/glider.ac"));

  if (path.str().substr(path.str().size() - 4, 4) == ".xml") {
    readProperties(path.str(), _props);
    if (_props->hasValue("/path")) {
      path = path.dir();;
      path.append(_props->getStringValue("/path"));
    } else {
      path = globals->get_fg_root();
      path.append("Models/Geometry/glider.ac");
    }
  }

  ssgTexturePath((char *)path.dir().c_str());
  _model = ssgLoad((char *)path.c_str());
  if (_model == 0) {
    _model = ssgLoad((char *)"Models/Geometry/glider.ac");
    if (_model == 0)
      throw sg_exception("Failed to load an aircraft model");
  }

				// Load animations
  vector<SGPropertyNode *> animation_nodes =
    _props->getChildren("animation");
  for (int i = 0; i < animation_nodes.size(); i++) {
    _animations.push_back(read_animation(animation_nodes[i]));
  }

				// Set up the alignment node
  ssgTransform * align = new ssgTransform;
  align->addKid(_model);
  sgMat4 rot_matrix;
  sgMat4 off_matrix;
  sgMat4 res_matrix;
  float h_rot = _props->getFloatValue("/offsets/heading-deg", 0.0);
  float p_rot = _props->getFloatValue("/offsets/roll-deg", 0.0);
  float r_rot = _props->getFloatValue("/offsets/pitch-deg", 0.0);
  float x_off = _props->getFloatValue("/offsets/x-m", 0.0);
  float y_off = _props->getFloatValue("/offsets/y-m", 0.0);
  float z_off = _props->getFloatValue("/offsets/z-m", 0.0);
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
  _current_timestamp.stamp();
  long elapsed_ms = (_current_timestamp - _last_timestamp) / 1000;
  _last_timestamp.stamp();

  if (globals->get_viewmgr()->get_current() == 0) {
    _selector->select(false);
  } else {
    for (int i = 0; i < _animations.size(); i++)
      do_animation(_animations[i], elapsed_ms);

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
  }

}

FGAircraftModel::Animation
FGAircraftModel::read_animation (const SGPropertyNode * node)
{
  Animation animation;

				// Figure out the animation type
  string type_name = node->getStringValue("type");
  if (type_name == "spin") {
    SG_LOG(SG_INPUT, SG_INFO, "Reading spin animation");
    animation.type = Animation::Spin;
  } else {
    animation.type = Animation::None;
    SG_LOG(SG_INPUT, SG_ALERT, "Unknown animation type " << type_name);
    return animation;
  }

				// Find the object to be animated
  string object_name = node->getStringValue("object-name");
  ssgEntity * target = find_named_node(_model, object_name);
  if (target != 0) {
    SG_LOG(SG_INPUT, SG_INFO, "  Target object is " << object_name);
  } else {
    animation.type = Animation::None;
    SG_LOG(SG_INPUT, SG_ALERT, "Object " << object_name
	   << " not found in model");
    return animation;
  }

				// Splice a transform node into the tree
  animation.transform = new ssgTransform;
  int nParents = target->getNumParents();
  animation.transform->addKid(target);
  for (int i = 0; i < nParents; i++) {
    ssgBranch * parent = target->getParent(i);
    parent->replaceKid(target, animation.transform);
  }

				// Get the node
  animation.prop =
    fgGetNode(node->getStringValue("property", "/null"), true);

				// Get the center and axis
  animation.center_x = node->getFloatValue("center/x-m", 0);
  animation.center_y = node->getFloatValue("center/y-m", 0);
  animation.center_z = node->getFloatValue("center/z-m", 0);
  animation.axis_x = node->getFloatValue("axis/x", 0);
  animation.axis_y = node->getFloatValue("axis/y", 1);
  animation.axis_z = node->getFloatValue("axis/z", 0);

  return animation;
}

void
FGAircraftModel::do_animation (Animation &animation, long elapsed_ms)
{
  switch (animation.type) {
  case Animation::None:
    return;
  case Animation::Spin: {
    float velocity_rpms = animation.prop->getDoubleValue() / 60000.0;
    animation.position += (elapsed_ms * velocity_rpms * 360);
    while (animation.position >= 360)
      animation.position -= 360;
    sgMakeTransMat4(animation.matrix, -animation.center_x,
		    -animation.center_y, -animation.center_z);
    sgVec3 axis;
    sgSetVec3(axis, animation.axis_x, animation.axis_y, animation.axis_z);
    sgMat4 tmp;
    sgMakeRotMat4(tmp, animation.position, axis);
    sgPostMultMat4(animation.matrix, tmp);
    sgMakeTransMat4(tmp, animation.center_x,
		    animation.center_y, animation.center_z);
    sgPostMultMat4(animation.matrix, tmp);
    animation.transform->setTransform(animation.matrix);
    return;
  }
  default:
    return;
  }
}

// end of model.cxx
