// model.cxx - manage a 3D aircraft model.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain, and comes with no warranty.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string.h>		// for strcmp()

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

extern ssgRoot * cockpit;		// FIXME: from main.cxx

FGAircraftModel current_model;	// FIXME: add to globals



////////////////////////////////////////////////////////////////////////
// Static utility functions.
////////////////////////////////////////////////////////////////////////

static ssgEntity *
find_named_node (ssgEntity * node, const char * name)
{
  char * node_name = node->getName();
  if (node_name != 0 && !strcmp(name, node_name))
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

/**
 * Splice a branch in between all child nodes and their parents.
 */
static void
splice_branch (ssgBranch * branch, ssgEntity * child)
{
  int nParents = child->getNumParents();
  branch->addKid(child);
  for (int i = 0; i < nParents; i++) {
    ssgBranch * parent = child->getParent(i);
    parent->replaceKid(child, branch);
  }
}

/**
 * Set up the transform matrix for a spin or rotation.
 */
static void
set_rotation (sgMat4 &matrix, double position_deg,
	      sgVec3 &center, sgVec3 &axis)
{
 float temp_angle = -position_deg * SG_DEGREES_TO_RADIANS ;
 
 float s = (float) sin ( temp_angle ) ;
 float c = (float) cos ( temp_angle ) ;
 float t = SG_ONE - c ;

 // axis was normalized at load time 
 // hint to the compiler to put these into FP registers
 float x = axis[0];
 float y = axis[1];
 float z = axis[2];

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
}

/**
 * Set up the transform matrix for a translation.
 */
static void
set_translation (sgMat4 &matrix, double position_m, sgVec3 &axis)
{
  sgVec3 xyz;
  sgScaleVec3(xyz, axis, position_m);
  sgMakeTransMat4(matrix, xyz);
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGAircraftModel
////////////////////////////////////////////////////////////////////////

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

  for (int i = 0; i < _animations.size(); i++) {
    Animation * tmp = _animations[i];
    _animations[i] = 0;
    delete tmp;
  }

}

void 
FGAircraftModel::init ()
{
  // TODO: optionally load an XML file with a pointer to the 3D object
  // and placement and animation info

  SGPropertyNode props;

  SG_LOG(SG_INPUT, SG_INFO, "Initializing aircraft 3D model");

				// Load the 3D aircraft object itself
  // DCL - the xml parser requires the full path but the ssgLoader doesn't
  // so lets have two paths.
  SGPath xmlpath = globals->get_fg_root();
  SGPath modelpath = (string)fgGetString("/sim/model/path", "Models/Geometry/glider.ac");
  xmlpath.append(modelpath.str());

  if (xmlpath.str().substr(xmlpath.str().size() - 4, 4) == ".xml") {
    readProperties(xmlpath.str(), &props);
    if (props.hasValue("/path")) {
      modelpath = modelpath.dir();
      modelpath.append(props.getStringValue("/path"));
    } else {
      modelpath = "Models/Geometry/glider.ac";
    }
  }

  ssgTexturePath((char *)xmlpath.dir().c_str());
  _model = ssgLoad((char *)modelpath.c_str());
  if (_model == 0) {
    _model = ssgLoad((char *)"Models/Geometry/glider.ac");
    if (_model == 0)
      throw sg_exception("Failed to load an aircraft model");
  }

				// Load animations
  vector<SGPropertyNode *> animation_nodes = props.getChildren("animation");
  for (unsigned int i = 0; i < animation_nodes.size(); i++) {
    vector<SGPropertyNode *> name_nodes =
      animation_nodes[i]->getChildren("object-name");
    if (name_nodes.size() < 1) {
      SG_LOG(SG_INPUT, SG_ALERT, "No object-name given for transformation");
    } else {
      for (unsigned int j = 0; j < name_nodes.size(); j++) {
	Animation * animation =
	  make_animation(name_nodes[j]->getStringValue(), animation_nodes[i]);
	if (animation != 0)
	  _animations.push_back(animation);
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
  cockpit->addKid(_selector);
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
  sgMat4 MODEL_ROT, LOCAL;
  sgMat4 sgTRANS;

  int view_number = globals->get_viewmgr()->get_current();

  if (view_number == 0 && !fgGetBool("/sim/view/internal")) {
    _selector->select(false);
  } else {
    for (unsigned int i = 0; i < _animations.size(); i++)
      _animations[i]->update(dt);

    _selector->select(true);
    FGViewer *current_view = 
      (FGViewer *)globals->get_viewmgr()->get_view( view_number );
    
    // FIXME: this class needs to be unlinked from the viewer
    // get transform for current position in the world...
    sgMakeTransMat4( sgTRANS, current_view->getRelativeViewPos() );

    // get a copy of the LOCAL rotation from the current view...
    sgCopyMat4( LOCAL, current_view->get_LOCAL_ROT() );

    // Make the MODEL Rotation (just reordering the LOCAL matrix
    //  and flipping the model over on its feet)...
    MODEL_ROT[0][0] = -LOCAL[2][0];
    MODEL_ROT[0][1] = -LOCAL[2][1];
    MODEL_ROT[0][2] = -LOCAL[2][2];
    MODEL_ROT[0][3] = SG_ZERO;
    MODEL_ROT[1][0] = LOCAL[1][0];
    MODEL_ROT[1][1] = LOCAL[1][1];
    MODEL_ROT[1][2] = LOCAL[1][2];
    MODEL_ROT[1][3] = SG_ZERO;
    MODEL_ROT[2][0] = LOCAL[0][0];
    MODEL_ROT[2][1] = LOCAL[0][1];
    MODEL_ROT[2][2] = LOCAL[0][2];
    MODEL_ROT[2][3] = SG_ZERO;

    // add the position data to the matrix
    MODEL_ROT[3][0] = SG_ZERO;
    MODEL_ROT[3][1] = SG_ZERO;
    MODEL_ROT[3][2] = SG_ZERO;
    MODEL_ROT[3][3] = SG_ONE;

    sgPostMultMat4( MODEL_ROT, sgTRANS );

    sgCoord tuxpos;
    sgSetCoord( &tuxpos, MODEL_ROT );
    _position->setTransform( &tuxpos );
  }
}

FGAircraftModel::Animation *
FGAircraftModel::make_animation (const char * object_name,
				 SGPropertyNode * node)
{
  Animation * animation = 0;
  const char * type = node->getStringValue("type");
  if (!strcmp("none", type)) {
    animation = new NullAnimation();
  } else if (!strcmp("spin", type)) {
    animation = new SpinAnimation();
  } else if (!strcmp("rotate", type)) {
    animation = new RotateAnimation();
  } else if (!strcmp("translate", type)) {
    animation = new TranslateAnimation();
  } else {
    animation = new NullAnimation();
    SG_LOG(SG_INPUT, SG_WARN, "Unknown animation type " << type);
  }

  ssgEntity * object = find_named_node(_model, object_name);
  if (object == 0) {
    SG_LOG(SG_INPUT, SG_WARN, "Object " << object_name << " not found");
    delete animation;
    animation = 0;
  } else {
    animation->init(object, node);
  }

  return animation;
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGAircraftModel::Animation
////////////////////////////////////////////////////////////////////////

FGAircraftModel::Animation::Animation ()
{
}

FGAircraftModel::Animation::~Animation ()
{
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGAircraftModel::NullAnimation
////////////////////////////////////////////////////////////////////////

FGAircraftModel::NullAnimation::NullAnimation ()
{
}

FGAircraftModel::NullAnimation::~NullAnimation ()
{
}

void
FGAircraftModel::NullAnimation::init (ssgEntity * object,
				      SGPropertyNode * node)
{
}

void
FGAircraftModel::NullAnimation::update (int dt)
{
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGAircraftModel::SpinAnimation
////////////////////////////////////////////////////////////////////////

FGAircraftModel::SpinAnimation::SpinAnimation ()
  : _prop(0),
    _factor(0),
    _position_deg(0),
    _transform(new ssgTransform)
{
}

FGAircraftModel::SpinAnimation::~SpinAnimation ()
{
  _transform = 0;
}

void
FGAircraftModel::SpinAnimation::init (ssgEntity * object,
				      SGPropertyNode * props)
{
				// Splice in the new transform node
  splice_branch(_transform, object);
  _prop = fgGetNode(props->getStringValue("property", "/null"), true);
  _factor = props->getDoubleValue("factor", 1.0);
  _position_deg = props->getDoubleValue("starting-position-deg", 0);
  _center[0] = props->getFloatValue("center/x-m", 0);
  _center[1] = props->getFloatValue("center/y-m", 0);
  _center[2] = props->getFloatValue("center/z-m", 0);
  _axis[0] = props->getFloatValue("axis/x", 0);
  _axis[1] = props->getFloatValue("axis/y", 0);
  _axis[2] = props->getFloatValue("axis/z", 0);
  sgNormalizeVec3(_axis);
}

void
FGAircraftModel::SpinAnimation::update (int dt)
{
  float velocity_rpms = (_prop->getDoubleValue() * _factor / 60000.0);
  _position_deg += (dt * velocity_rpms * 360);
  while (_position_deg < 0)
    _position_deg += 360.0;
  while (_position_deg >= 360.0)
    _position_deg -= 360.0;
  set_rotation(_matrix, _position_deg, _center, _axis);
  _transform->setTransform(_matrix);
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGAircraftModel::RotateAnimation
////////////////////////////////////////////////////////////////////////

FGAircraftModel::RotateAnimation::RotateAnimation ()
  : _prop(0),
    _offset_deg(0.0),
    _factor(1.0),
    _has_min(false),
    _min_deg(0.0),
    _has_max(false),
    _max_deg(1.0),
    _position_deg(0.0),
    _transform(new ssgTransform)
{
}

FGAircraftModel::RotateAnimation::~RotateAnimation ()
{
  _transform = 0;
}

void
FGAircraftModel::RotateAnimation::init (ssgEntity * object,
					SGPropertyNode * props)
{
				// Splice in the new transform node
  splice_branch(_transform, object);
  _prop = fgGetNode(props->getStringValue("property", "/null"), true);
  _offset_deg = props->getDoubleValue("offset-deg", 0.0);
  _factor = props->getDoubleValue("factor", 1.0);
  if (props->hasValue("min-deg")) {
    _has_min = true;
    _min_deg = props->getDoubleValue("min-deg");
  }
  if (props->hasValue("max-deg")) {
    _has_max = true;
    _max_deg = props->getDoubleValue("max-deg");
  }
  _position_deg = props->getDoubleValue("starting-position-deg", 0);
  _center[0] = props->getFloatValue("center/x-m", 0);
  _center[1] = props->getFloatValue("center/y-m", 0);
  _center[2] = props->getFloatValue("center/z-m", 0);
  _axis[0] = props->getFloatValue("axis/x", 0);
  _axis[1] = props->getFloatValue("axis/y", 0);
  _axis[2] = props->getFloatValue("axis/z", 0);
  sgNormalizeVec3(_axis);
}

void
FGAircraftModel::RotateAnimation::update (int dt)
{
  _position_deg = ((_prop->getDoubleValue() + _offset_deg) * _factor);
  if (_has_min && _position_deg < _min_deg)
    _position_deg = _min_deg;
  if (_has_max && _position_deg > _max_deg)
    _position_deg = _max_deg;
  set_rotation(_matrix, _position_deg, _center, _axis);
  _transform->setTransform(_matrix);
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGAircraftModel::TranslateAnimation
////////////////////////////////////////////////////////////////////////

FGAircraftModel::TranslateAnimation::TranslateAnimation ()
  : _prop(0),
    _offset_m(0.0),
    _factor(1.0),
    _has_min(false),
    _min_m(0.0),
    _has_max(false),
    _max_m(1.0),
    _position_m(0.0),
    _transform(new ssgTransform)
{
}

FGAircraftModel::TranslateAnimation::~TranslateAnimation ()
{
  _transform = 0;
}

void
FGAircraftModel::TranslateAnimation::init (ssgEntity * object,
					SGPropertyNode * props)
{
				// Splice in the new transform node
  splice_branch(_transform, object);
  _prop = fgGetNode(props->getStringValue("property", "/null"), true);
  _offset_m = props->getDoubleValue("offset-m", 0.0);
  _factor = props->getDoubleValue("factor", 1.0);
  if (props->hasValue("min-m")) {
    _has_min = true;
    _min_m = props->getDoubleValue("min-m");
  }
  if (props->hasValue("max-m")) {
    _has_max = true;
    _max_m = props->getDoubleValue("max-m");
  }
  _position_m = props->getDoubleValue("starting-position-m", 0);
  _axis[0] = props->getFloatValue("axis/x", 0);
  _axis[1] = props->getFloatValue("axis/y", 0);
  _axis[2] = props->getFloatValue("axis/z", 0);
  sgNormalizeVec3(_axis);
}

void
FGAircraftModel::TranslateAnimation::update (int dt)
{
  _position_m = ((_prop->getDoubleValue() + _offset_m) * _factor);
  if (_has_min && _position_m < _min_m)
    _position_m = _min_m;
  if (_has_max && _position_m > _max_m)
    _position_m = _max_m;
  set_translation(_matrix, _position_m, _axis);
  _transform->setTransform(_matrix);
}


// end of model.cxx




