// modelmgr.cxx - manage a collection of 3D models.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain, and comes with no warranty.

#include "modelmgr.hxx"

#include <Main/fg_props.hxx>


FGModelMgr::FGModelMgr ()
  : _scene(new ssgRoot),
    _nearplane(0.5f),
    _farplane(120000.0f)
{
}

FGModelMgr::~FGModelMgr ()
{
  for (int i = 0; i < _instances.size(); i++) {
    delete _instances[i];
  }
  delete _scene;
}

void
FGModelMgr::init ()
{
  vector<SGPropertyNode *> model_nodes =
    fgGetNode("/models", true)->getChildren("model");
  for (int i = 0; i < model_nodes.size(); i++) {
    SGPropertyNode * node = model_nodes[i];
    SG_LOG(SG_GENERAL, SG_INFO,
	   "Adding model " << node->getStringValue("name", "[unnamed]"));
    Instance * instance = new Instance;
    FG3DModel * model = new FG3DModel;
    instance->model = model;
    model->init(node->getStringValue("path", "Models/Geometry/glider.ac"));

				// Set position and orientation either
				// indirectly through property refs
				// or directly with static values.
    SGPropertyNode * child = node->getChild("longitude-deg-prop");
    if (child != 0)
      instance->lon_deg_node = child;
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

				// Add this model to the scene graph
    _scene->addKid(model->getSceneGraph());

				// Save this instance for updating
    _instances.push_back(instance);
  }
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
FGModelMgr::update (int dt)
{
  for (int i = 0; i < _instances.size(); i++) {
    Instance * instance = _instances[i];
    FG3DModel * model = instance->model;

    instance->model->update(dt);

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

  }
}

void
FGModelMgr::draw ()
{
  ssgSetNearFar(_nearplane, _farplane);
  ssgCullAndDraw(_scene);
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGModelMgr::Instance
////////////////////////////////////////////////////////////////////////

FGModelMgr::Instance::Instance ()
  : model(0),
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
  delete model;
}

// end of modelmgr.cxx
