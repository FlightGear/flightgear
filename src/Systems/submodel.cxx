// submodel.cxx - models a releasable submodel.
// Written by Dave Culp, started Aug 2004
//
// This file is in the Public Domain and comes with no warranty.

#include "submodel.hxx"
#include <Main/fg_props.hxx>
#include <Main/util.hxx>
#include <AIModel/AIManager.hxx>


SubmodelSystem::SubmodelSystem ()
{
  firing = false;
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
    _serviceable_node = fgGetNode("/systems/submodel/serviceable", true);
	
    _trigger_node = fgGetNode("/systems/submodel/trigger", true);
    _trigger_node->setBoolValue(false);
	
    _amount_node = fgGetNode("/systems/submodel/amount", true);
	 
    _user_lat_node = fgGetNode("/position/latitude-deg", true);
    _user_lon_node = fgGetNode("/position/longitude-deg", true);
    _user_alt_node = fgGetNode("/position/altitude-ft", true);

    _user_heading_node = fgGetNode("/orientation/heading-deg", true);
    _user_pitch_node =   fgGetNode("/orientation/pitch-deg", true);
    _user_roll_node =    fgGetNode("/orientation/roll-deg", true);
    _user_yaw_node =     fgGetNode("/orientation/yaw-deg", true);

    _user_speed_node = fgGetNode("/velocities/uBody-fps", true);

    elapsed_time = 0.0;
    initial_velocity = 2750.0;  // feet per second, .50 caliber

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
  if (_trigger_node->getBoolValue()) {
    if (_serviceable_node->getBoolValue()) {
      if (_amount_node->getIntValue() > 0) {
        firing = true;
        release(dt);
      } 
    }
  } else {
    if (firing){
      firing = false;
      elapsed_time = 0.0;
    }
  }
}

bool
SubmodelSystem::release (double dt)
{
  // releases a submodel every 0.25 seconds
  elapsed_time += dt;
  if (elapsed_time < 0.25) return false;
  elapsed_time = 0.0;

  int rval = ai->createBallistic( "Models/Geometry/tracer.ac",
        _user_lat_node->getDoubleValue(),
        _user_lon_node->getDoubleValue(),
        _user_alt_node->getDoubleValue() + z_offset,
        _user_heading_node->getDoubleValue() + yaw_offset,
        _user_pitch_node->getDoubleValue() + pitch_offset,
        _user_speed_node->getDoubleValue() + initial_velocity );

  _amount_node->setIntValue( _amount_node->getIntValue() - 1); 
  return true;                    
}

// end of submodel.cxx
