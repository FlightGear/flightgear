// submodel.cxx - models a releasable submodel.
// Written by Dave Culp, started Aug 2004
//
// This file is in the Public Domain and comes with no warranty.

#include "submodel.hxx"

#include <simgear/structure/exception.hxx>
#include <simgear/misc/sg_path.hxx>

#include <Main/fg_props.hxx>
#include <Main/util.hxx>
#include <AIModel/AIManager.hxx>


SubmodelSystem::SubmodelSystem ()
{
  x_offset = y_offset = 0.0;
  z_offset = -4.0;
  pitch_offset = 2.0;
  yaw_offset = 0.0;
}

SubmodelSystem::~SubmodelSystem ()
{
}

void
SubmodelSystem::init ()
{
    load();
    _serviceable_node = fgGetNode("/sim/systems/submodels/serviceable", true);
	
    _user_lat_node = fgGetNode("/position/latitude-deg", true);
    _user_lon_node = fgGetNode("/position/longitude-deg", true);
    _user_alt_node = fgGetNode("/position/altitude-ft", true);

    _user_heading_node = fgGetNode("/orientation/heading-deg", true);
    _user_pitch_node =   fgGetNode("/orientation/pitch-deg", true);
    _user_roll_node =    fgGetNode("/orientation/roll-deg", true);
    _user_yaw_node =     fgGetNode("/orientation/yaw-deg", true);

    _user_speed_node = fgGetNode("/velocities/uBody-fps", true);

    ai = (FGAIManager*)globals->get_subsystem("ai_model");
}

void
SubmodelSystem::bind ()
{
}

void
SubmodelSystem::unbind ()
{
}

void
SubmodelSystem::update (double dt)
{
  if (!(_serviceable_node->getBoolValue())) return;

  submodel_iterator = submodels.begin();
  while(submodel_iterator != submodels.end()) {

    if ((*submodel_iterator)->trigger->getBoolValue()) {
        if ((*submodel_iterator)->count > 0) {
          release( (*submodel_iterator), dt);
        } 
    } 
    ++submodel_iterator;
  }
   
}

bool
SubmodelSystem::release (submodel* sm, double dt)
{
  sm->timer += dt;
  if (sm->timer < sm->delay) return false;
  sm->timer = 0.0;

  transform(sm);  // calculate submodel's initial conditions in world-coordinates

  int rval = ai->createBallistic( sm->model, IC.lat, IC.lon, IC.alt, IC.azimuth,
                                  IC.elevation, IC.speed );
  sm->count--; 
  return true;                    
}

void
SubmodelSystem::load ()
{
    int i;
    SGPropertyNode *path = fgGetNode("/sim/systems/submodels/path");
    SGPropertyNode root;

    if (path) {
      SGPath config( globals->get_fg_root() );
      config.append( path->getStringValue() );

      try {
        readProperties(config.str(), &root);
      } catch (const sg_exception &e) {
        SG_LOG(SG_GENERAL, SG_ALERT,
        "Unable to read submodels file: ");
        cout << config.str() << endl;
        return;
      }
    }

   int count = root.nChildren();
   for (i = 0; i < count; i++) { 
     // cout << "Reading submodel " << i << endl;        
     submodel* sm = new submodel;
     submodels.push_back( sm );
     SGPropertyNode * entry_node = root.getChild(i);
     sm->trigger        = fgGetNode(entry_node->getStringValue("trigger", "none"), true);
     sm->name           = entry_node->getStringValue("name", "none_defined");
     sm->model          = entry_node->getStringValue("model", "Models/Geometry/tracer.ac");
     sm->speed          = entry_node->getDoubleValue("speed", 0.0);
     sm->repeat         = entry_node->getBoolValue  ("repeat", false); 
     sm->delay          = entry_node->getDoubleValue("delay", 0.25); 
     sm->count          = entry_node->getIntValue   ("count", 1); 
     sm->slaved         = entry_node->getBoolValue  ("slaved", false); 
     sm->x_offset       = entry_node->getDoubleValue("x-offset", 0.0); 
     sm->y_offset       = entry_node->getDoubleValue("y_offset", 0.0); 
     sm->z_offset       = entry_node->getDoubleValue("z-offset", 0.0); 
     sm->yaw_offset     = entry_node->getDoubleValue("yaw-offset", 0.0); 
     sm->pitch_offset   = entry_node->getDoubleValue("pitch-offset", 0.0);

     sm->trigger->setBoolValue(false);
     sm->timer = 0.0;
   }


  submodel_iterator = submodels.begin();
  // cout << submodels.size() << " submodels read." << endl;
}

void
SubmodelSystem::transform( submodel* sm) 
{
IC.lat = _user_lat_node->getDoubleValue();
IC.lon = _user_lon_node->getDoubleValue();
IC.alt = _user_alt_node->getDoubleValue();
IC.azimuth = _user_heading_node->getDoubleValue() + sm->yaw_offset;
IC.elevation = _user_pitch_node->getDoubleValue() + sm->pitch_offset;
IC.speed = _user_speed_node->getDoubleValue() + sm->speed;
}

// end of submodel.cxx
