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
  : _model(0),
    _selector(new ssgSelector),
    _position(new ssgTransform)
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

  SGPropertyNode props;

  SG_LOG(SG_INPUT, SG_INFO, "Initializing aircraft 3D model");

				// Load the 3D aircraft object itself
  SGPath path = globals->get_fg_root();
  path.append(fgGetString("/sim/model/path", "Models/Geometry/glider.ac"));

  if (path.str().substr(path.str().size() - 4, 4) == ".xml") {
    readProperties(path.str(), &props);
    if (props.hasValue("/path")) {
      path = path.dir();;
      path.append(props.getStringValue("/path"));
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
    props.getChildren("animation");
  for (unsigned int i = 0; i < animation_nodes.size(); i++) {
    vector<SGPropertyNode *> name_nodes =
      animation_nodes[i]->getChildren("object-name");
    if (name_nodes.size() < 1) {
      SG_LOG(SG_INPUT, SG_ALERT, "No object-name given for transformation");
    } else {
      for (unsigned int j = 0; j < name_nodes.size(); j++) {
	_animations.push_back(read_animation(name_nodes[j]->getStringValue(),
					     animation_nodes[i]));
      }
    }
  }

				// Set up the alignment node
  ssgTransform * align = new ssgTransform;
  align->addKid(_model);
  sgMat4 rot_matrix;
  sgMat4 off_matrix;
  sgMat4 res_matrix;
  float h_rot = props.getFloatValue("/offsets/heading-deg", 0.0);
  float p_rot = props.getFloatValue("/offsets/roll-deg", 0.0);
  float r_rot = props.getFloatValue("/offsets/pitch-deg", 0.0);
  float x_off = props.getFloatValue("/offsets/x-m", 0.0);
  float y_off = props.getFloatValue("/offsets/y-m", 0.0);
  float z_off = props.getFloatValue("/offsets/z-m", 0.0);
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

  int view_number = globals->get_viewmgr()->get_current();

  if (view_number == 0 && !fgGetBool("/sim/view/internal")) {
    _selector->select(false);
  } else {
    for (unsigned int i = 0; i < _animations.size(); i++)
      do_animation(_animations[i], elapsed_ms);

    _selector->select(true);
    FGViewer *pilot_view =
      (FGViewer *)globals->get_viewmgr()->get_view( 0 );
    
    sgMat4 sgTRANS;
    sgMakeTransMat4( sgTRANS, pilot_view->get_view_pos() );
    
    sgVec3 ownship_up;
    sgSetVec3( ownship_up, 0.0, 0.0, 1.0);
    
    sgMat4 sgROT;
    sgMakeRotMat4( sgROT, -90.0, ownship_up );
    
    sgMat4 sgTUX;
    sgCopyMat4( sgTUX, sgROT );
    sgMat4 VIEW_ROT;
    sgCopyMat4( VIEW_ROT, pilot_view->get_VIEW_ROT());
    if (view_number == 0) {
				// FIXME: orientation is not applied
				// correctly when view is not forward
      sgMakeRotMat4( sgROT, -pilot_view->getHeadingOffset_deg(), 
         pilot_view->get_world_up() );

      /* Warning lame hack from Wilson ahead */
      /* get the pitch value */
      /* double it to counter the value already in the VIEW_ROT */
      float pitch = pilot_view->getPitch_deg() * SGD_DEGREES_TO_RADIANS * 2;
      /* make a ROT matrix 
         with the values waited by the X coordinate from the offset 
         rotation see sgROT above
      */
      sgMat4 PunROT;
      PunROT[0][0] = SG_ONE;
      PunROT[0][1] = SG_ZERO;
      PunROT[0][2] = SG_ZERO;
      PunROT[0][3] = SG_ZERO;
      PunROT[1][0] = SG_ZERO;
      PunROT[1][1] = cos((1 - sgROT[0][0]) * -pitch);
      PunROT[1][2] = -sin((1 - sgROT[0][0]) * -pitch);
      PunROT[1][3] = SG_ZERO;
      PunROT[2][0] = SG_ZERO;
      PunROT[2][1] = sin((1 - sgROT[0][0]) * -pitch);
      PunROT[2][2] = cos((1 - sgROT[0][0]) * -pitch);
      PunROT[2][3] = SG_ZERO;
      PunROT[3][0] = SG_ZERO;
      PunROT[3][1] = SG_ZERO;
      PunROT[3][2] = SG_ZERO;
      PunROT[3][3] = SG_ONE;

      sgPostMultMat4( sgTUX, PunROT );
      sgPostMultMat4( sgTUX, VIEW_ROT );
      sgPostMultMat4( sgTUX, sgROT );
      sgPostMultMat4( sgTUX, sgTRANS );
      /* end lame hack */

    } else {
      sgPostMultMat4( sgTUX, VIEW_ROT );
      sgPostMultMat4( sgTUX, sgTRANS );
    }

    sgCoord tuxpos;
    sgSetCoord( &tuxpos, sgTUX );
    _position->setTransform( &tuxpos );
  }
}

FGAircraftModel::Animation
FGAircraftModel::read_animation (const string &object_name,
				 const SGPropertyNode * node)
{
  Animation animation;

				// Find the object to be animated
  ssgEntity * target = find_named_node(_model, object_name);
  if (target != 0) {
    SG_LOG(SG_INPUT, SG_INFO, "  Target object is " << object_name);
  } else {
    animation.type = Animation::None;
    SG_LOG(SG_INPUT, SG_ALERT, "Object " << object_name
	   << " not found in model");
    return animation;
  }

				// Figure out the animation type
  string type_name = node->getStringValue("type");
  if (type_name == "spin") {
    SG_LOG(SG_INPUT, SG_INFO, "Reading spin animation");
    animation.type = Animation::Spin;
  } else if (type_name == "rotate") {
    SG_LOG(SG_INPUT, SG_INFO, "Reading rotate animation");
    animation.type = Animation::Rotate;
  } else if (type_name == "none") {
    SG_LOG(SG_INPUT, SG_INFO, "Reading disabled animation");
    animation.type = Animation::None;
    return animation;
  } else {
    animation.type = Animation::None;
    SG_LOG(SG_INPUT, SG_ALERT, "Unknown animation type " << type_name);
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

  animation.position = node->getFloatValue("initial-position", 0);
  animation.offset = node->getFloatValue("offset", 0);
  if (node->hasValue("min")) {
    animation.has_min = true;
    animation.min = node->getFloatValue("min");
  } else {
    animation.has_min = false;
  }
  if (node->hasValue("max")) {
    animation.has_max = true;
    animation.max = node->getFloatValue("max");
  } else {
    animation.has_max = false;
  }
  animation.factor = node->getFloatValue("factor", 1);

				// Get the center and axis
  animation.center[0] = node->getFloatValue("center/x-m", 0);
  animation.center[1] = node->getFloatValue("center/y-m", 0);
  animation.center[2] = node->getFloatValue("center/z-m", 0);
  animation.axis[0] = node->getFloatValue("axis/x", 0);
  animation.axis[1] = node->getFloatValue("axis/y", 0);
  animation.axis[2] = node->getFloatValue("axis/z", 0);

  sgNormalizeVec3(animation.axis);

  return animation;
}

void
FGAircraftModel::do_animation (Animation &animation, long elapsed_ms)
{
  switch (animation.type) {
  case Animation::None:
    return;
  case Animation::Spin:
  {
    float velocity_rpms = (animation.prop->getDoubleValue()
			   * animation.factor / 60000.0);
    animation.position += (elapsed_ms * velocity_rpms * 360);
    animation.setRotation();
    return;
  }
  case Animation::Rotate: {
    animation.position = ((animation.prop->getFloatValue()
			   + animation.offset)
			  * animation.factor);
    if (animation.has_min && animation.position < animation.min)
      animation.position = animation.min;
    if (animation.has_max && animation.position > animation.max)
      animation.position = animation.max;
    animation.setRotation();
    return;
  }
  default:
    return;
  }
}

/* 
 * Transform to rotate an object around its local axis
 * from a relative frame of reference at center -- NHV
 */
void
FGAircraftModel::Animation::setRotation()
{
 float temp_angle = -position * SG_DEGREES_TO_RADIANS ;
 
 float s = (float) sin ( temp_angle ) ;
 float c = (float) cos ( temp_angle ) ;
 float t = SG_ONE - c ;

 // axis was normalized at load time 
 // hint to the compiler to put these into FP registers
 float x = axis[0];
 float y = axis[1];
 float z = axis[2];

 sgMat4 matrix;
 matrix[0][0] = t * x * x + c ;
 matrix[0][1] = t * y * x - s * z ;
 matrix[0][2] = t * z * x + s * y ;
 matrix[0][3] = SG_ZERO;
 
 matrix[1][0] = t * x * y + s * z ;
 matrix[1][1] = t * y * y + c ;
 matrix[1][2] = t * z * y - s * x ;
 matrix[1][3] = SG_ZERO;
 
 matrix[2][0] = t * x * z - s * y ;
 matrix[2][1] = t * y * z + s * x ;
 matrix[2][2] = t * z * z + c ;
 matrix[2][3] = SG_ZERO;

  // hint to the compiler to put these into FP registers
 x = center[0];
 y = center[1];
 z = center[2];
 
 matrix[3][0] = x - x*matrix[0][0] - y*matrix[1][0] - z*matrix[2][0];
 matrix[3][1] = y - x*matrix[0][1] - y*matrix[1][1] - z*matrix[2][1];
 matrix[3][2] = z - x*matrix[0][2] - y*matrix[1][2] - z*matrix[2][2];
 matrix[3][3] = SG_ONE;
 
 transform->setTransform(matrix);
}


// end of model.cxx




