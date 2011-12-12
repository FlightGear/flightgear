// modelmgr.cxx - manage a collection of 3D models.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain, and comes with no warranty.

#ifdef _MSC_VER
#  pragma warning( disable: 4355 )
#endif

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#include <algorithm>
#include <functional>
#include <vector>
#include <cstring>

#include <osg/Math>

#include <simgear/scene/model/placement.hxx>
#include <simgear/scene/model/modellib.hxx>
#include <simgear/structure/exception.hxx>

#include <Main/fg_props.hxx>
#include <Scenery/scenery.hxx>


#include "modelmgr.hxx"

using std::vector;

using namespace simgear;

// OSGFIXME
// extern SGShadowVolume *shadows;

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
      ->removeChild(_instances[i]->model->getSceneGraph());
    delete _instances[i];
  }
}

void
FGModelMgr::init ()
{
  vector<SGPropertyNode_ptr> model_nodes = _models->getChildren("model");

  for (unsigned int i = 0; i < model_nodes.size(); i++)
      add_model(model_nodes[i]);
}

void
FGModelMgr::add_model (SGPropertyNode * node)
{
  SG_LOG(SG_AIRCRAFT, SG_INFO,
         "Adding model " << node->getStringValue("name", "[unnamed]"));

  const char *path = node->getStringValue("path", "Models/Geometry/glider.ac");
  osg::Node *object;

  try {
      object = SGModelLib::loadDeferredModel(path, globals->get_props());
  } catch (const sg_throwable& t) {
    SG_LOG(SG_AIRCRAFT, SG_ALERT, "Error loading " << path << ":\n  "
        << t.getFormattedMessage() << t.getOrigin());
    return;
  }
  
  Instance * instance = new Instance;
  SGModelPlacement *model = new SGModelPlacement;
  instance->model = model;
  instance->node = node;

  model->init( object );

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
  globals->get_scenery()->get_scene_graph()->addChild(model->getSceneGraph());


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

namespace
{
double testNan(double val) throw (sg_range_exception)
{
    if (osg::isNaN(val))
        throw sg_range_exception("value is nan");
    return val;
}

struct UpdateFunctor : public std::unary_function<FGModelMgr::Instance*, void>
{
    void operator()(FGModelMgr::Instance* instance) const
    {
        SGModelPlacement* model = instance->model;
        double lon, lat, elev, roll, pitch, heading;

        try {
            // Optionally set position from properties
            if (instance->lon_deg_node != 0)
                lon = testNan(instance->lon_deg_node->getDoubleValue());
            if (instance->lat_deg_node != 0)
                lat = testNan(instance->lat_deg_node->getDoubleValue());
            if (instance->elev_ft_node != 0)
                elev = testNan(instance->elev_ft_node->getDoubleValue());

            // Optionally set orientation from properties
            if (instance->roll_deg_node != 0)
                roll = testNan(instance->roll_deg_node->getDoubleValue());
            if (instance->pitch_deg_node != 0)
                pitch = testNan(instance->pitch_deg_node->getDoubleValue());
            if (instance->heading_deg_node != 0)
                heading = testNan(instance->heading_deg_node->getDoubleValue());
        } catch (const sg_range_exception&) {
            const char *path = instance->node->getStringValue("path",
                                                              "unknown");
            SG_LOG(SG_AIRCRAFT, SG_INFO, "Instance of model " << path
                   << " has invalid values");
            return;
        }
        // Optionally set position from properties
        if (instance->lon_deg_node != 0)
            model->setLongitudeDeg(lon);
        if (instance->lat_deg_node != 0)
            model->setLatitudeDeg(lat);
        if (instance->elev_ft_node != 0)
            model->setElevationFt(elev);

        // Optionally set orientation from properties
        if (instance->roll_deg_node != 0)
            model->setRollDeg(roll);
        if (instance->pitch_deg_node != 0)
            model->setPitchDeg(pitch);
        if (instance->heading_deg_node != 0)
            model->setHeadingDeg(heading);

        instance->model->update();
    }
};
}

void
FGModelMgr::update (double dt)
{
    std::for_each(_instances.begin(), _instances.end(), UpdateFunctor());
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
    heading_deg_node(0),
    shadow(false)
{
}

FGModelMgr::Instance::~Instance ()
{
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

  _mgr->add_model(parent);
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
    osg::Node *branch = instance->model->getSceneGraph();
    // OSGFIXME
//     if (shadows && instance->shadow)
//         shadows->deleteOccluder(branch);
    globals->get_scenery()->get_scene_graph()->removeChild(branch);

    delete instance;
    break;
  }
}

// end of modelmgr.cxx
