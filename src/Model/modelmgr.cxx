// modelmgr.cxx - manage a collection of 3D models.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain, and comes with no warranty.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#include <vector>

#include <plib/ssg.h>

#include <simgear/scene/model/placement.hxx>
#include <simgear/scene/model/modellib.hxx>
#include <simgear/scene/model/shadowvolume.hxx>
#include <simgear/structure/exception.hxx>

#include <Main/fg_props.hxx>
#include <Scenery/scenery.hxx>


#include "modelmgr.hxx"

SG_USING_STD(vector);

extern SGShadowVolume *shadows;


FGModelMgr::FGModelMgr ()
  : _models(fgGetNode("/models", true)),
    _listener(new Listener(this))

{
  _models->addChangeListener(_listener);
}

FGModelMgr::~FGModelMgr ()
{
  _models->removeChangeListener(_listener);
  delete _listener;

  for (unsigned int i = 0; i < _instances.size(); i++) {
    globals->get_scenery()->get_scene_graph()
      ->removeKid(_instances[i]->model->getSceneGraph());
    delete _instances[i];
  }
}

void
FGModelMgr::init ()
{
  vector<SGPropertyNode_ptr> model_nodes = _models->getChildren("model");

  for (unsigned int i = 0; i < model_nodes.size(); i++) {
    try {
      add_model(model_nodes[i]);
    } catch (const sg_throwable& t) {
      SG_LOG(SG_GENERAL, SG_ALERT, t.getFormattedMessage() << t.getOrigin());
    }
  }
}

void
FGModelMgr::add_model (SGPropertyNode * node)
{
  SG_LOG(SG_GENERAL, SG_INFO,
         "Adding model " << node->getStringValue("name", "[unnamed]"));
  Instance * instance = new Instance;
  SGModelPlacement *model = new SGModelPlacement;
  instance->model = model;
  instance->node = node;
  SGModelLib *model_lib = globals->get_model_lib();
  ssgBranch *object = (ssgBranch *)model_lib->load_model(
      globals->get_fg_root(),
      node->getStringValue("path",
                           "Models/Geometry/glider.ac"),
      globals->get_props(),
      globals->get_sim_time_sec(), /*cache_object=*/false);

  model->init( object );
  if (shadows)
      shadows->addOccluder((ssgBranch *)object, SGShadowVolume::occluderTypeTileObject);

				// Set position and orientation either
				// indirectly through property refs
				// or directly with static values.
  SGPropertyNode * child = node->getChild("longitude-deg-prop");
  if (child != 0)
    instance->lon_deg_node = fgGetNode(child->getStringValue(), true);
  else
    model->setLongitudeDeg(node->getDoubleValue("longitude-deg"));

  child = node->getChild("latitude-deg-prop");
  if (child != 0)
    instance->lat_deg_node = fgGetNode(child->getStringValue(), true);
  else
    model->setLatitudeDeg(node->getDoubleValue("latitude-deg"));

  child = node->getChild("elevation-ft-prop");
  if (child != 0)
    instance->elev_ft_node = fgGetNode(child->getStringValue(), true);
  else
    model->setElevationFt(node->getDoubleValue("elevation-ft"));

  child = node->getChild("roll-deg-prop");
  if (child != 0)
    instance->roll_deg_node = fgGetNode(child->getStringValue(), true);
  else
    model->setRollDeg(node->getDoubleValue("roll-deg"));

  child = node->getChild("pitch-deg-prop");
  if (child != 0)
    instance->pitch_deg_node = fgGetNode(child->getStringValue(), true);
  else
    model->setPitchDeg(node->getDoubleValue("pitch-deg"));

  child = node->getChild("heading-deg-prop");
  if (child != 0)
    instance->heading_deg_node = fgGetNode(child->getStringValue(), true);
  else
    model->setHeadingDeg(node->getDoubleValue("heading-deg"));

      			// Add this model to the global scene graph
  globals->get_scenery()->get_scene_graph()->addKid(model->getSceneGraph());

  // Register that one at the scenery manager
  globals->get_scenery()->register_placement_transform(model->getTransform());


      			// Save this instance for updating
  add_instance(instance);
}

void
FGModelMgr::bind ()
{
}

void
FGModelMgr::unbind ()
{
}

void
FGModelMgr::update (double dt)
{
  for (unsigned int i = 0; i < _instances.size(); i++) {
    Instance * instance = _instances[i];
    SGModelPlacement * model = instance->model;

				// Optionally set position from properties
    if (instance->lon_deg_node != 0)
      model->setLongitudeDeg(instance->lon_deg_node->getDoubleValue());
    if (instance->lat_deg_node != 0)
      model->setLatitudeDeg(instance->lat_deg_node->getDoubleValue());
    if (instance->elev_ft_node != 0)
      model->setElevationFt(instance->elev_ft_node->getDoubleValue());

				// Optionally set orientation from properties
    if (instance->roll_deg_node != 0)
      model->setRollDeg(instance->roll_deg_node->getDoubleValue());
    if (instance->pitch_deg_node != 0)
      model->setPitchDeg(instance->pitch_deg_node->getDoubleValue());
    if (instance->heading_deg_node != 0)
      model->setHeadingDeg(instance->heading_deg_node->getDoubleValue());

    instance->model->update();
  }
}

void
FGModelMgr::add_instance (Instance * instance)
{
    _instances.push_back(instance);
}

void
FGModelMgr::remove_instance (Instance * instance)
{
    vector<Instance *>::iterator it;
    for (it = _instances.begin(); it != _instances.end(); it++) {
        if (*it == instance) {
            _instances.erase(it);
            delete instance;
            return;
        }
    }
}

void
FGModelMgr::draw ()
{
//   ssgSetNearFar(_nearplane, _farplane);
//   ssgCullAndDraw(_scene);
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGModelMgr::Instance
////////////////////////////////////////////////////////////////////////

FGModelMgr::Instance::Instance ()
  : model(0),
    node(0),
    lon_deg_node(0),
    lat_deg_node(0),
    elev_ft_node(0),
    roll_deg_node(0),
    pitch_deg_node(0),
    heading_deg_node(0)
{
}

FGModelMgr::Instance::~Instance ()
{
  // Unregister that one at the scenery manager
  globals->get_scenery()->unregister_placement_transform(model->getTransform());

  delete model;
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGModelMgr::Listener
////////////////////////////////////////////////////////////////////////

void
FGModelMgr::Listener::childAdded(SGPropertyNode * parent, SGPropertyNode * child)
{
  if (strcmp(parent->getName(), "model") || strcmp(child->getName(), "load"))
    return;

  try {
    _mgr->add_model(parent);
  } catch (const sg_throwable& t) {
    SG_LOG(SG_GENERAL, SG_ALERT, t.getFormattedMessage() << t.getOrigin());
  }
}

void
FGModelMgr::Listener::childRemoved(SGPropertyNode * parent, SGPropertyNode * child)
{
  if (strcmp(parent->getName(), "models") || strcmp(child->getName(), "model"))
    return;

  // search instance by node and remove it from scenegraph
  vector<Instance *>::iterator it = _mgr->_instances.begin();
  vector<Instance *>::iterator end = _mgr->_instances.end();

  for (; it != end; ++it) {
    Instance *instance = *it;
    if (instance->node != child)
      continue;

    _mgr->_instances.erase(it);
    ssgBranch *branch = (ssgBranch *)instance->model->getSceneGraph();
    if (shadows)
        shadows->deleteOccluder(branch);
    globals->get_scenery()->get_scene_graph()->removeKid(branch);

    delete instance;
    break;
  }
}

// end of modelmgr.cxx
