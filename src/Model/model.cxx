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
#include <plib/ul.h>

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
#include "panelnode.hxx"



////////////////////////////////////////////////////////////////////////
// Static utility functions.
////////////////////////////////////////////////////////////////////////

/**
 * Callback to update an animation.
 */
static int
animation_callback (ssgEntity * entity, int mask)
{
    ((Animation *)entity->getUserData())->update();
    return true;
}


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
 * Make an offset matrix from rotations and position offset.
 */
static void
make_offsets_matrix (sgMat4 * result, double h_rot, double p_rot, double r_rot,
                     double x_off, double y_off, double z_off)
{
  sgMat4 rot_matrix;
  sgMat4 pos_matrix;
  sgMakeRotMat4(rot_matrix, h_rot, p_rot, r_rot);
  sgMakeTransMat4(pos_matrix, x_off, y_off, z_off);
  sgMultMat4(*result, pos_matrix, rot_matrix);
}


/**
 * Read an interpolation table from properties.
 */
static SGInterpTable *
read_interpolation_table (SGPropertyNode_ptr props)
{
  SGPropertyNode_ptr table_node = props->getNode("interpolation");
  if (table_node != 0) {
    SGInterpTable * table = new SGInterpTable();
    vector<SGPropertyNode_ptr> entries = table_node->getChildren("entry");
    for (unsigned int i = 0; i < entries.size(); i++)
      table->addEntry(entries[i]->getDoubleValue("ind", 0.0),
                      entries[i]->getDoubleValue("dep", 0.0));
    return table;
  } else {
    return 0;
  }
}


static void
make_animation (ssgBranch * model,
                const char * name,
                vector<SGPropertyNode_ptr> &name_nodes,
                SGPropertyNode_ptr node)
{
  Animation * animation = 0;
  const char * type = node->getStringValue("type", "none");
  if (!strcmp("none", type)) {
    animation = new NullAnimation(node);
  } else if (!strcmp("range", type)) {
    animation = new RangeAnimation(node);
  } else if (!strcmp("billboard", type)) {
    animation = new BillboardAnimation(node);
  } else if (!strcmp("select", type)) {
    animation = new SelectAnimation(node);
  } else if (!strcmp("spin", type)) {
    animation = new SpinAnimation(node);
  } else if (!strcmp("timed", type)) {
    animation = new TimedAnimation(node);
  } else if (!strcmp("rotate", type)) {
    animation = new RotateAnimation(node);
  } else if (!strcmp("translate", type)) {
    animation = new TranslateAnimation(node);
  } else {
    animation = new NullAnimation(node);
    SG_LOG(SG_INPUT, SG_WARN, "Unknown animation type " << type);
  }

  if (name != 0)
      animation->setName((char *)name);

  ssgEntity * object;
  if (name_nodes.size() > 0) {
    object = find_named_node(model, name_nodes[0]->getStringValue());
    if (object == 0) {
      SG_LOG(SG_INPUT, SG_WARN, "Object " << name_nodes[0]->getStringValue()
             << " not found");
      delete animation;
      animation = 0;
    }
  } else {
    object = model;
  }
  
  ssgBranch * branch = animation->getBranch();
  splice_branch(branch, object);

  for (int i = 1; i < name_nodes.size(); i++) {
      const char * name = name_nodes[i]->getStringValue();
      object = find_named_node(model, name);
      if (object == 0) {
          SG_LOG(SG_INPUT, SG_WARN, "Object " << name << " not found");
          delete animation;
          animation = 0;
      }
      ssgBranch * oldParent = object->getParent(0);
      branch->addKid(object);
      oldParent->removeKid(object);
  }

  animation->init();
  branch->setUserData(animation);
  branch->setTravCallback(SSG_CALLBACK_PRETRAV, animation_callback);
}



////////////////////////////////////////////////////////////////////////
// Global functions.
////////////////////////////////////////////////////////////////////////

ssgBranch *
fgLoad3DModel (const string &path)
{
  ssgBranch * model = 0;
  SGPropertyNode props;

                                // Load the 3D aircraft object itself
  SGPath xmlpath;
  SGPath modelpath = path;
  if ( ulIsAbsolutePathName( path.c_str() ) ) {
    xmlpath = modelpath;
  }
  else {
    xmlpath = globals->get_fg_root();
    xmlpath.append(modelpath.str());
  }

                                // Check for an XML wrapper
  if (xmlpath.str().substr(xmlpath.str().size() - 4, 4) == ".xml") {
    readProperties(xmlpath.str(), &props);
    if (props.hasValue("/path")) {
      modelpath = modelpath.dir();
      modelpath.append(props.getStringValue("/path"));
    } else {
      if (model == 0)
        model = new ssgBranch;
    }
  }

                                // Assume that textures are in
                                // the same location as the XML file.
  if (model == 0) {
    ssgTexturePath((char *)xmlpath.dir().c_str());
    model = (ssgBranch *)ssgLoad((char *)modelpath.c_str());
    if (model == 0)
      throw sg_exception("Failed to load 3D model");
  }

                                // Set up the alignment node
  ssgTransform * align = new ssgTransform;
  align->addKid(model);
  sgMat4 res_matrix;
  make_offsets_matrix(&res_matrix,
                      props.getFloatValue("/offsets/heading-deg", 0.0),
                      props.getFloatValue("/offsets/roll-deg", 0.0),
                      props.getFloatValue("/offsets/pitch-deg", 0.0),
                      props.getFloatValue("/offsets/x-m", 0.0),
                      props.getFloatValue("/offsets/y-m", 0.0),
                      props.getFloatValue("/offsets/z-m", 0.0));
  align->setTransform(res_matrix);

                                // Load panels
  unsigned int i;
  vector<SGPropertyNode_ptr> panel_nodes = props.getChildren("panel");
  for (i = 0; i < panel_nodes.size(); i++) {
    SG_LOG(SG_INPUT, SG_DEBUG, "Loading a panel");
    FGPanelNode * panel = new FGPanelNode(panel_nodes[i]);
    if (panel_nodes[i]->hasValue("name"))
        panel->setName((char *)panel_nodes[i]->getStringValue("name"));
    model->addKid(panel);
  }

                                // Load animations
  vector<SGPropertyNode_ptr> animation_nodes = props.getChildren("animation");
  for (i = 0; i < animation_nodes.size(); i++) {
    const char * name = animation_nodes[i]->getStringValue("name", 0);
    vector<SGPropertyNode_ptr> name_nodes =
      animation_nodes[i]->getChildren("object-name");
    make_animation(model, name, name_nodes, animation_nodes[i]);
  }

                                // Load sub-models
  vector<SGPropertyNode_ptr> model_nodes = props.getChildren("model");
  for (i = 0; i < model_nodes.size(); i++) {
    SGPropertyNode_ptr node = model_nodes[i];
    ssgTransform * align = new ssgTransform;
    sgMat4 res_matrix;
    make_offsets_matrix(&res_matrix,
                        node->getFloatValue("offsets/heading-deg", 0.0),
                        node->getFloatValue("offsets/roll-deg", 0.0),
                        node->getFloatValue("offsets/pitch-deg", 0.0),
                        node->getFloatValue("offsets/x-m", 0.0),
                        node->getFloatValue("offsets/y-m", 0.0),
                        node->getFloatValue("offsets/z-m", 0.0));
    align->setTransform(res_matrix);

    ssgBranch * kid = fgLoad3DModel(node->getStringValue("path"));
    align->addKid(kid);
    model->addKid(align);
  }

  return model;
}



////////////////////////////////////////////////////////////////////////
// Implementation of Animation
////////////////////////////////////////////////////////////////////////

Animation::Animation (SGPropertyNode_ptr props, ssgBranch * branch)
    : _branch(branch)
{
    _branch->setName(props->getStringValue("name", 0));
}

Animation::~Animation ()
{
}

void
Animation::init ()
{
}

void
Animation::update ()
{
}



////////////////////////////////////////////////////////////////////////
// Implementation of NullAnimation
////////////////////////////////////////////////////////////////////////

NullAnimation::NullAnimation (SGPropertyNode_ptr props)
  : Animation(props, new ssgBranch)
{
}

NullAnimation::~NullAnimation ()
{
}



////////////////////////////////////////////////////////////////////////
// Implementation of RangeAnimation
////////////////////////////////////////////////////////////////////////

RangeAnimation::RangeAnimation (SGPropertyNode_ptr props)
  : Animation(props, new ssgRangeSelector)
{
    float ranges[] = { props->getFloatValue("min-m", 0),
                       props->getFloatValue("max-m", 5000) };
    ((ssgRangeSelector *)_branch)->setRanges(ranges, 2);
                       
}

RangeAnimation::~RangeAnimation ()
{
}



////////////////////////////////////////////////////////////////////////
// Implementation of BillboardAnimation
////////////////////////////////////////////////////////////////////////

BillboardAnimation::BillboardAnimation (SGPropertyNode_ptr props)
    : Animation(props, new ssgCutout(props->getBoolValue("spherical", true)))
{
}

BillboardAnimation::~BillboardAnimation ()
{
}



////////////////////////////////////////////////////////////////////////
// Implementation of SelectAnimation
////////////////////////////////////////////////////////////////////////

SelectAnimation::SelectAnimation (SGPropertyNode_ptr props)
  : Animation(props, new ssgSelector),
    _condition(0)
{
  SGPropertyNode_ptr node = props->getChild("condition");
  if (node != 0)
    _condition = fgReadCondition(node);
}

SelectAnimation::~SelectAnimation ()
{
  delete _condition;
}

void
SelectAnimation::update ()
{
  if (_condition != 0 && _condition->test()) 
      ((ssgSelector *)_branch)->select(0xffff);
  else
      ((ssgSelector *)_branch)->select(0x0000);
}



////////////////////////////////////////////////////////////////////////
// Implementation of SpinAnimation
////////////////////////////////////////////////////////////////////////

SpinAnimation::SpinAnimation (SGPropertyNode_ptr props)
  : Animation(props, new ssgTransform),
    _prop(fgGetNode(props->getStringValue("property", "/null"), true)),
    _factor(props->getDoubleValue("factor", 1.0)),
    _position_deg(props->getDoubleValue("starting-position-deg", 0)),
    _last_time_sec(globals->get_sim_time_sec())
{
    _center[0] = props->getFloatValue("center/x-m", 0);
    _center[1] = props->getFloatValue("center/y-m", 0);
    _center[2] = props->getFloatValue("center/z-m", 0);
    _axis[0] = props->getFloatValue("axis/x", 0);
    _axis[1] = props->getFloatValue("axis/y", 0);
    _axis[2] = props->getFloatValue("axis/z", 0);
    sgNormalizeVec3(_axis);
}

SpinAnimation::~SpinAnimation ()
{
}

void
SpinAnimation::update ()
{
  double sim_time = globals->get_sim_time_sec();
  double dt = sim_time - _last_time_sec;
  _last_time_sec = sim_time;

  float velocity_rpms = (_prop->getDoubleValue() * _factor / 60.0);
  _position_deg += (dt * velocity_rpms * 360);
  while (_position_deg < 0)
    _position_deg += 360.0;
  while (_position_deg >= 360.0)
    _position_deg -= 360.0;
  set_rotation(_matrix, _position_deg, _center, _axis);
  ((ssgTransform *)_branch)->setTransform(_matrix);
}



////////////////////////////////////////////////////////////////////////
// Implementation of TimedAnimation
////////////////////////////////////////////////////////////////////////

TimedAnimation::TimedAnimation (SGPropertyNode_ptr props)
  : Animation(props, new ssgSelector),
    _duration_sec(props->getDoubleValue("duration-sec", 1.0)),
    _last_time_sec(0),
    _step(-1)
{
}

TimedAnimation::~TimedAnimation ()
{
}

void
TimedAnimation::update ()
{
    float sim_time_sec = globals->get_sim_time_sec();
    if ((sim_time_sec - _last_time_sec) >= _duration_sec) {
        _last_time_sec = sim_time_sec;
        _step++;
        if (_step >= getBranch()->getNumKids())
            _step = 0;
        ((ssgSelector *)getBranch())->selectStep(_step);
    }
}



////////////////////////////////////////////////////////////////////////
// Implementation of RotateAnimation
////////////////////////////////////////////////////////////////////////

RotateAnimation::RotateAnimation (SGPropertyNode_ptr props)
    : Animation(props, new ssgTransform),
      _prop(fgGetNode(props->getStringValue("property", "/null"), true)),
      _offset_deg(props->getDoubleValue("offset-deg", 0.0)),
      _factor(props->getDoubleValue("factor", 1.0)),
      _table(read_interpolation_table(props)),
      _has_min(props->hasValue("min-deg")),
      _min_deg(props->getDoubleValue("min-deg")),
      _has_max(props->hasValue("max-deg")),
      _max_deg(props->getDoubleValue("max-deg")),
      _position_deg(props->getDoubleValue("starting-position-deg", 0))
{
  _center[0] = props->getFloatValue("center/x-m", 0);
  _center[1] = props->getFloatValue("center/y-m", 0);
  _center[2] = props->getFloatValue("center/z-m", 0);
  _axis[0] = props->getFloatValue("axis/x", 0);
  _axis[1] = props->getFloatValue("axis/y", 0);
  _axis[2] = props->getFloatValue("axis/z", 0);
  sgNormalizeVec3(_axis);
}

RotateAnimation::~RotateAnimation ()
{
  delete _table;
}

void
RotateAnimation::update ()
{
  if (_table == 0) {
   _position_deg = _prop->getDoubleValue() * _factor + _offset_deg;
   if (_has_min && _position_deg < _min_deg)
     _position_deg = _min_deg;
   if (_has_max && _position_deg > _max_deg)
     _position_deg = _max_deg;
  } else {
    _position_deg = _table->interpolate(_prop->getDoubleValue());
  }
  set_rotation(_matrix, _position_deg, _center, _axis);
  ((ssgTransform *)_branch)->setTransform(_matrix);
}



////////////////////////////////////////////////////////////////////////
// Implementation of TranslateAnimation
////////////////////////////////////////////////////////////////////////

TranslateAnimation::TranslateAnimation (SGPropertyNode_ptr props)
  : Animation(props, new ssgTransform),
    _prop(fgGetNode(props->getStringValue("property", "/null"), true)),
    _offset_m(props->getDoubleValue("offset-m", 0.0)),
    _factor(props->getDoubleValue("factor", 1.0)),
    _table(read_interpolation_table(props)),
    _has_min(props->hasValue("min-m")),
    _min_m(props->getDoubleValue("min-m")),
    _has_max(props->hasValue("max-m")),
    _max_m(props->getDoubleValue("max-m")),
    _position_m(props->getDoubleValue("starting-position-m", 0))
{
  _axis[0] = props->getFloatValue("axis/x", 0);
  _axis[1] = props->getFloatValue("axis/y", 0);
  _axis[2] = props->getFloatValue("axis/z", 0);
  sgNormalizeVec3(_axis);
}

TranslateAnimation::~TranslateAnimation ()
{
  delete _table;
}

void
TranslateAnimation::update ()
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
  ((ssgTransform *)_branch)->setTransform(_matrix);
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGModelPlacement.
////////////////////////////////////////////////////////////////////////

FGModelPlacement::FGModelPlacement ()
  : _lon_deg(0),
    _lat_deg(0),
    _elev_ft(0),
    _roll_deg(0),
    _pitch_deg(0),
    _heading_deg(0),
    _selector(new ssgSelector),
    _position(new ssgTransform),
    _location(new FGLocation)
{
}

FGModelPlacement::~FGModelPlacement ()
{
}

void
FGModelPlacement::init (const string &path)
{
  ssgBranch * model = fgLoad3DModel(path);
  if (model != 0)
      _position->addKid(model);
  _selector->addKid(_position);
  _selector->clrTraversalMaskBits(SSGTRAV_HOT);
}

void
FGModelPlacement::update ()
{
  _location->setPosition( _lon_deg, _lat_deg, _elev_ft );
  _location->setOrientation( _roll_deg, _pitch_deg, _heading_deg );

  sgMat4 POS;
  sgCopyMat4(POS, _location->getTransformMatrix());
  
  sgVec3 trans;
  sgCopyVec3(trans, _location->get_view_pos());

  for(int i = 0; i < 4; i++) {
    float tmp = POS[i][3];
    for( int j=0; j<3; j++ ) {
      POS[i][j] += (tmp * trans[j]);
    }
  }
  _position->setTransform(POS);
}

bool
FGModelPlacement::getVisible () const
{
  return (_selector->getSelect() != 0);
}

void
FGModelPlacement::setVisible (bool visible)
{
  _selector->select(visible);
}

void
FGModelPlacement::setLongitudeDeg (double lon_deg)
{
  _lon_deg = lon_deg;
}

void
FGModelPlacement::setLatitudeDeg (double lat_deg)
{
  _lat_deg = lat_deg;
}

void
FGModelPlacement::setElevationFt (double elev_ft)
{
  _elev_ft = elev_ft;
}

void
FGModelPlacement::setPosition (double lon_deg, double lat_deg, double elev_ft)
{
  _lon_deg = lon_deg;
  _lat_deg = lat_deg;
  _elev_ft = elev_ft;
}

void
FGModelPlacement::setRollDeg (double roll_deg)
{
  _roll_deg = roll_deg;
}

void
FGModelPlacement::setPitchDeg (double pitch_deg)
{
  _pitch_deg = pitch_deg;
}

void
FGModelPlacement::setHeadingDeg (double heading_deg)
{
  _heading_deg = heading_deg;
}

void
FGModelPlacement::setOrientation (double roll_deg, double pitch_deg,
                                  double heading_deg)
{
  _roll_deg = roll_deg;
  _pitch_deg = pitch_deg;
  _heading_deg = heading_deg;
}

// end of model.cxx
