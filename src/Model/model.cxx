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
#include <simgear/props/condition.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/scene/model/animation.hxx>
#include <simgear/scene/model/location.hxx>

#include "panelnode.hxx"

#include "model.hxx"



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


static void
make_animation (ssgBranch * model,
                const char * name,
                vector<SGPropertyNode_ptr> &name_nodes,
                SGPropertyNode *prop_root,
                SGPropertyNode_ptr node,
                double sim_time_sec )
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
    animation = new SelectAnimation(prop_root, node);
  } else if (!strcmp("spin", type)) {
    animation = new SpinAnimation(prop_root, node, sim_time_sec );
  } else if (!strcmp("timed", type)) {
    animation = new TimedAnimation(node);
  } else if (!strcmp("rotate", type)) {
    animation = new RotateAnimation(prop_root, node);
  } else if (!strcmp("translate", type)) {
    animation = new TranslateAnimation(prop_root, node);
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

  for (unsigned int i = 1; i < name_nodes.size(); i++) {
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
fgLoad3DModel( const string &fg_root, const string &path,
               SGPropertyNode *prop_root,
               double sim_time_sec )
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
    xmlpath = fg_root;
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
  ssgTransform * alignmainmodel = new ssgTransform;
  alignmainmodel->addKid(model);
  sgMat4 res_matrix;
  make_offsets_matrix(&res_matrix,
                      props.getFloatValue("/offsets/heading-deg", 0.0),
                      props.getFloatValue("/offsets/roll-deg", 0.0),
                      props.getFloatValue("/offsets/pitch-deg", 0.0),
                      props.getFloatValue("/offsets/x-m", 0.0),
                      props.getFloatValue("/offsets/y-m", 0.0),
                      props.getFloatValue("/offsets/z-m", 0.0));
  alignmainmodel->setTransform(res_matrix);

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
    make_animation( model, name, name_nodes, prop_root, animation_nodes[i],
                    sim_time_sec);
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

    ssgBranch * kid = fgLoad3DModel( fg_root, node->getStringValue("path"),
                                     prop_root, sim_time_sec );
    align->addKid(kid);
    model->addKid(align);
  }

  return alignmainmodel;
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
FGModelPlacement::init( const string &fg_root,
                        const string &path,
                        SGPropertyNode *prop_root,
                        double sim_time_sec )
{
  ssgBranch * model = fgLoad3DModel( fg_root, path, prop_root, sim_time_sec );
  if (model != 0)
      _position->addKid(model);
  _selector->addKid(_position);
  _selector->clrTraversalMaskBits(SSGTRAV_HOT);
}

void
FGModelPlacement::update( const Point3D scenery_center )
{
  _location->setPosition( _lon_deg, _lat_deg, _elev_ft );
  _location->setOrientation( _roll_deg, _pitch_deg, _heading_deg );

  sgCopyMat4( POS, _location->getTransformMatrix(scenery_center) );

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
