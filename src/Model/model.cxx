// model.cxx - manage a 3D aircraft model.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain, and comes with no warranty.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string.h>             // for strcmp()

#include <plib/sg.h>
#include <plib/ssg.h>

#include <simgear/compiler.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/math/interpolater.hxx>
#include <simgear/math/point3d.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/misc/exception.hxx>
#include <simgear/misc/sg_path.hxx>

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Main/location.hxx>
#include <Scenery/scenery.hxx>

#include "model.hxx"



////////////////////////////////////////////////////////////////////////
// Static utility functions.
////////////////////////////////////////////////////////////////////////

/**
 * Locate a named SSG node in a branch.
 */
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


/**
 * Read an interpolation table from properties.
 */
static SGInterpTable *
read_interpolation_table (const SGPropertyNode * props)
{
  const SGPropertyNode * table_node = props->getNode("interpolation");
  if (table_node != 0) {
    SGInterpTable * table = new SGInterpTable();
    vector<const SGPropertyNode *> entries = table_node->getChildren("entry");
    for (int i = 0; i < entries.size(); i++)
      table->addEntry(entries[i]->getDoubleValue("ind", 0.0),
		      entries[i]->getDoubleValue("dep", 0.0));
    return table;
  } else {
    return 0;
  }
}



////////////////////////////////////////////////////////////////////////
// Implementation of FG3DModel
////////////////////////////////////////////////////////////////////////

FG3DModel::FG3DModel ()
  : _model(0),
    _selector(new ssgSelector),
    _position(new ssgTransform)
{
}

FG3DModel::~FG3DModel ()
{
  // since the nodes are attached to the scene graph, they'll be
  // deleted automatically

  int i;
  for (i = 0; i < _animations.size(); i++)
    delete _animations[i];
}

void 
FG3DModel::init (const string &path)
{
  SGPropertyNode props;

                                // Load the 3D aircraft object itself
  SGPath xmlpath = globals->get_fg_root();
  SGPath modelpath = path;
  xmlpath.append(modelpath.str());

                                // Check for an XML wrapper
  if (xmlpath.str().substr(xmlpath.str().size() - 4, 4) == ".xml") {
    readProperties(xmlpath.str(), &props);
    if (props.hasValue("/path")) {
      modelpath = modelpath.dir();
      modelpath.append(props.getStringValue("/path"));
    } else {
      throw sg_exception("No path for model");
    }
  }

                                // Assume that textures are in
                                // the same location as the XML file.
  ssgTexturePath((char *)xmlpath.dir().c_str());
  _model = (ssgBranch *)ssgLoad((char *)modelpath.c_str());
  if (_model == 0)
    throw sg_exception("Failed to load 3D model");

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

                                // Set up a location class
  _location = (FGLocation *) new FGLocation;

                                // Load animations
  vector<SGPropertyNode *> animation_nodes = props.getChildren("animation");
  unsigned int i;
  for (i = 0; i < animation_nodes.size(); i++) {
    vector<SGPropertyNode *> name_nodes =
      animation_nodes[i]->getChildren("object-name");
    if (name_nodes.size() < 1) {
      Animation * animation = make_animation(0, animation_nodes[i]);
    } else {
      for (unsigned int j = 0; j < name_nodes.size(); j++) {
        Animation * animation =
          make_animation(name_nodes[j]->getStringValue(), animation_nodes[i]);
        if (animation != 0)
          _animations.push_back(animation);
      }
    }
  }
}

void
FG3DModel::update (int dt)
{
  unsigned int i;

  for (i = 0; i < _animations.size(); i++)
    _animations[i]->update(dt);

  _location->setPosition( _lon_deg, _lat_deg, _elev_ft );
  _location->setOrientation( _roll_deg, _pitch_deg, _heading_deg );

  sgMat4 POS;
  sgCopyMat4(POS, _location->getTransformMatrix());
  
  sgVec3 trans;
  sgCopyVec3(trans, _location->get_view_pos());

  for(i = 0; i < 4; i++) {
    float tmp = POS[i][3];
    for( int j=0; j<3; j++ ) {
      POS[i][j] += (tmp * trans[j]);
    }
  }
  _position->setTransform(POS);
}

bool
FG3DModel::getVisible () const
{
  return _selector->getSelect();
}

void
FG3DModel::setVisible (bool visible)
{
  _selector->select(visible);
}

void
FG3DModel::setLongitudeDeg (double lon_deg)
{
  _lon_deg = lon_deg;
}

void
FG3DModel::setLatitudeDeg (double lat_deg)
{
  _lat_deg = lat_deg;
}

void
FG3DModel::setElevationFt (double elev_ft)
{
  _elev_ft = elev_ft;
}

void
FG3DModel::setPosition (double lon_deg, double lat_deg, double elev_ft)
{
  _lon_deg = lon_deg;
  _lat_deg = lat_deg;
  _elev_ft = elev_ft;
}

void
FG3DModel::setRollDeg (double roll_deg)
{
  _roll_deg = roll_deg;
}

void
FG3DModel::setPitchDeg (double pitch_deg)
{
  _pitch_deg = pitch_deg;
}

void
FG3DModel::setHeadingDeg (double heading_deg)
{
  _heading_deg = heading_deg;
}

void
FG3DModel::setOrientation (double roll_deg, double pitch_deg,
                           double heading_deg)
{
  _roll_deg = roll_deg;
  _pitch_deg = pitch_deg;
  _heading_deg = heading_deg;
}

FG3DModel::Animation *
FG3DModel::make_animation (const char * object_name,
			   SGPropertyNode * node)
{
  Animation * animation = 0;
  const char * type = node->getStringValue("type");
  if (!strcmp("none", type)) {
    animation = new NullAnimation();
  } else if (!strcmp("range", type)) {
    animation = new RangeAnimation();
  } else if (!strcmp("billboard", type)) {
    animation = new BillboardAnimation();
  } else if (!strcmp("select", type)) {
    animation = new SelectAnimation();
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

  ssgEntity * object;
  if (object_name != 0) {
    object = find_named_node(_model, object_name);
    if (object == 0) {
      SG_LOG(SG_INPUT, SG_WARN, "Object " << object_name << " not found");
      delete animation;
      animation = 0;
    }
  } else {
    object = _model;
  }

  animation->init(object, node);
  return animation;
}



////////////////////////////////////////////////////////////////////////
// Implementation of FG3DModel::Animation
////////////////////////////////////////////////////////////////////////

FG3DModel::Animation::Animation ()
{
}

FG3DModel::Animation::~Animation ()
{
}



////////////////////////////////////////////////////////////////////////
// Implementation of FG3DModel::NullAnimation
////////////////////////////////////////////////////////////////////////

FG3DModel::NullAnimation::NullAnimation ()
  : _branch(new ssgBranch)
{
}

FG3DModel::NullAnimation::~NullAnimation ()
{
  _branch = 0;
}

void
FG3DModel::NullAnimation::init (ssgEntity * object,
                                      SGPropertyNode * props)
{
  splice_branch(_branch, object);
  _branch->setName(props->getStringValue("name", 0));
}

void
FG3DModel::NullAnimation::update (int dt)
{
}



////////////////////////////////////////////////////////////////////////
// Implementation of FG3DModel::RangeAnimation
////////////////////////////////////////////////////////////////////////

FG3DModel::RangeAnimation::RangeAnimation ()
  : _branch(new ssgRangeSelector)
{
}

FG3DModel::RangeAnimation::~RangeAnimation ()
{
  _branch = 0;
}

void
FG3DModel::RangeAnimation::init (ssgEntity * object,
                                      SGPropertyNode * props)
{
  float ranges[2];
  splice_branch(_branch, object);
  _branch->setName(props->getStringValue("name", 0));
  ranges[0] = props->getFloatValue("min-m", 0);
  ranges[1] = props->getFloatValue("max-m", 5000);
  _branch->setRanges(ranges, 2);
}

void
FG3DModel::RangeAnimation::update (int dt)
{
}



////////////////////////////////////////////////////////////////////////
// Implementation of FG3DModel::BillboardAnimation
////////////////////////////////////////////////////////////////////////

FG3DModel::BillboardAnimation::BillboardAnimation ()
  : _branch(0)
{
  // Note: we cannot allocate the branch until we know whether
  // it can rotate around the x axis as well as the z axis.
}

FG3DModel::BillboardAnimation::~BillboardAnimation ()
{
  _branch = 0;
}

void
FG3DModel::BillboardAnimation::init (ssgEntity * object,
				     SGPropertyNode * props)
{
  _branch = new ssgCutout(props->getBoolValue("spherical", true));
  splice_branch(_branch, object);
  _branch->setName(props->getStringValue("name", 0));
}

void
FG3DModel::BillboardAnimation::update (int dt)
{
}



////////////////////////////////////////////////////////////////////////
// Implementation of FG3DModel::SelectAnimation
////////////////////////////////////////////////////////////////////////

FG3DModel::SelectAnimation::SelectAnimation ()
  : _condition(0),
    _selector(new ssgSelector)
{
}

FG3DModel::SelectAnimation::~SelectAnimation ()
{
  delete _condition;
  _selector = 0;
}

void
FG3DModel::SelectAnimation::init (ssgEntity * object,
                                      SGPropertyNode * props)
{
  splice_branch(_selector, object);
  _selector->setName(props->getStringValue("name", 0));
  SGPropertyNode * node = props->getChild("condition");
  if (node != 0) {
    _condition = fgReadCondition(node);
  }
}

void
FG3DModel::SelectAnimation::update (int dt)
{
  if (_condition != 0 && _condition->test()) 
    _selector->select(0xffff);
  else
    _selector->select(0x0000);
}



////////////////////////////////////////////////////////////////////////
// Implementation of FG3DModel::SpinAnimation
////////////////////////////////////////////////////////////////////////

FG3DModel::SpinAnimation::SpinAnimation ()
  : _prop(0),
    _factor(0),
    _position_deg(0),
    _transform(new ssgTransform)
{
}

FG3DModel::SpinAnimation::~SpinAnimation ()
{
  _transform = 0;
}

void
FG3DModel::SpinAnimation::init (ssgEntity * object,
                                      SGPropertyNode * props)
{
                                // Splice in the new transform node
  splice_branch(_transform, object);
  _transform->setName(props->getStringValue("name", 0));
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
FG3DModel::SpinAnimation::update (int dt)
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
// Implementation of FG3DModel::RotateAnimation
////////////////////////////////////////////////////////////////////////

FG3DModel::RotateAnimation::RotateAnimation ()
  : _prop(0),
    _offset_deg(0.0),
    _factor(1.0),
    _table(0),
    _has_min(false),
    _min_deg(0.0),
    _has_max(false),
    _max_deg(1.0),
    _position_deg(0.0),
    _transform(new ssgTransform)
{
}

FG3DModel::RotateAnimation::~RotateAnimation ()
{
  delete _table;
  _transform = 0;
}

void
FG3DModel::RotateAnimation::init (ssgEntity * object,
				  SGPropertyNode * props)
{
                                // Splice in the new transform node
  splice_branch(_transform, object);
  _transform->setName(props->getStringValue("name", 0));
  _prop = fgGetNode(props->getStringValue("property", "/null"), true);
  _offset_deg = props->getDoubleValue("offset-deg", 0.0);
  _factor = props->getDoubleValue("factor", 1.0);
  _table = read_interpolation_table(props);
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
FG3DModel::RotateAnimation::update (int dt)
{
  if (_table == 0) {
    _position_deg = (_prop->getDoubleValue() + _offset_deg) * _factor;
   if (_has_min && _position_deg < _min_deg)
     _position_deg = _min_deg;
   if (_has_max && _position_deg > _max_deg)
     _position_deg = _max_deg;
  } else {
    _position_deg = _table->interpolate(_prop->getDoubleValue());
  }
  set_rotation(_matrix, _position_deg, _center, _axis);
  _transform->setTransform(_matrix);
}



////////////////////////////////////////////////////////////////////////
// Implementation of FG3DModel::TranslateAnimation
////////////////////////////////////////////////////////////////////////

FG3DModel::TranslateAnimation::TranslateAnimation ()
  : _prop(0),
    _offset_m(0.0),
    _factor(1.0),
    _table(0),
    _has_min(false),
    _min_m(0.0),
    _has_max(false),
    _max_m(1.0),
    _position_m(0.0),
    _transform(new ssgTransform)
{
}

FG3DModel::TranslateAnimation::~TranslateAnimation ()
{
  delete _table;
  _transform = 0;
}

void
FG3DModel::TranslateAnimation::init (ssgEntity * object,
				     SGPropertyNode * props)
{
                                // Splice in the new transform node
  splice_branch(_transform, object);
  _transform->setName(props->getStringValue("name", 0));
  _prop = fgGetNode(props->getStringValue("property", "/null"), true);
  _offset_m = props->getDoubleValue("offset-m", 0.0);
  _factor = props->getDoubleValue("factor", 1.0);
  _table = read_interpolation_table(props);
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
FG3DModel::TranslateAnimation::update (int dt)
{
  if (_table == 0) {
    _position_m = (_prop->getDoubleValue() + _offset_m) * _factor;
    if (_has_min && _position_m < _min_m)
      _position_m = _min_m;
    if (_has_max && _position_m > _max_m)
      _position_m = _max_m;
  } else {
    _position_m = _table->interpolate(_prop->getDoubleValue());
  }
  set_translation(_matrix, _position_m, _axis);
  _transform->setTransform(_matrix);
}


// end of model.cxx
