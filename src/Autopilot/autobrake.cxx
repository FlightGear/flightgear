// autobrake.cxx - generic, configurable autobrake system
//
// Written by James Turner, started September 2009.
//
// Copyright (C) 2009  Curtis L. Olson
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$

#include "autobrake.hxx"

#include <Main/fg_props.hxx>

FGAutoBrake::FGAutoBrake() :
  _lastGroundspeedKts(0.0),
  _step(0),
  _rto(false),
  _armed(false),
  _rtoArmed(false),
  _engaged(false),
  _targetDecel(0.0),
  _fcsBrakeControl(0.0),
  _leftBrakeOutput(0.0),
  _rightBrakeOutput(0.0)
{
  // default config, close to Boeing 747-400 / 777 values
  _configNumSteps = 4;
  _configRTOStep = -1;
  _configMaxDecel = 11; // ft-sec-sec
  _configRTODecel = 11; // ft-sec-sec
  _configRTOSpeed = 85; // kts
}

FGAutoBrake::~FGAutoBrake()
{
}

void FGAutoBrake::init()
{
  _root = fgGetNode("/autopilot/autobrake", true);
  
  _root->tie("armed", SGRawValueMethods<FGAutoBrake, bool>
    (*this, &FGAutoBrake::getArmed, &FGAutoBrake::setArmed));
  
  _root->tie("step", SGRawValueMethods<FGAutoBrake, int>
    (*this, &FGAutoBrake::getStep, &FGAutoBrake::setStep));
    
  _root->tie("rto", SGRawValueMethods<FGAutoBrake, bool>
    (*this, &FGAutoBrake::getRTO, &FGAutoBrake::setRTO));
      
  _engineControlsNode = fgGetNode("/controls/engines");
  
  // brake position nodes
  _brakeInputs[0] = fgGetNode("/controls/gear/brake-left");
  _brakeInputs[1] = fgGetNode("/controls/gear/brake-right");
  _brakeInputs[2] = fgGetNode("/controls/gear/copilot-brake-left");
  _brakeInputs[3] = fgGetNode("/controls/gear/copilot-brake-right");
  
    // config
  SGPropertyNode_ptr config = _root->getChild("config", 0, true);
  config->tie("num-steps", SGRawValuePointer<int>(&_configNumSteps));
  config->tie("rto-step", SGRawValuePointer<int>(&_configRTOStep));
  config->tie("rto-decel-fts-sec", SGRawValuePointer<double>(&_configRTODecel));
  config->tie("max-decel-fts-sec", SGRawValuePointer<double>(&_configMaxDecel));
  config->tie("rto-engage-kts", SGRawValuePointer<double>(&_configRTOSpeed));
  
  // outputs
  _root->tie("target-decel-fts-sec", 
    SGRawValueMethods<FGAutoBrake,double>(*this, &FGAutoBrake::getTargetDecel));
  _root->tie("actual-decel-fts-sec", 
    SGRawValueMethods<FGAutoBrake,double>(*this, &FGAutoBrake::getActualDecel));
  _root->tie("fcs-brake", SGRawValuePointer<double>(&_fcsBrakeControl));
  _root->tie("brake-left-output", SGRawValuePointer<double>(&_leftBrakeOutput));
  _root->tie("brake-right-output", SGRawValuePointer<double>(&_rightBrakeOutput));
  
  _root->tie("engaged", SGRawValueMethods<FGAutoBrake, bool>(*this, &FGAutoBrake::getEngaged)); 
}

void FGAutoBrake::postinit()
{  
  _weightOnWheelsNode = fgGetNode("/gear/gear/wow");
  _groundspeedNode = fgGetNode("/velocities/groundspeed-kt");
  _lastWoW = _weightOnWheelsNode->getBoolValue();
}


void FGAutoBrake::bind()
{
}

void FGAutoBrake::unbind()
{
}

void FGAutoBrake::update(double dt)
{
  _leftBrakeInput = SGMiscd::max(_brakeInputs[0]->getDoubleValue(), 
      _brakeInputs[2]->getDoubleValue());
  _rightBrakeInput = SGMiscd::max(_brakeInputs[1]->getDoubleValue(), 
      _brakeInputs[3]->getDoubleValue());

  // various FDMs don't supply sufficent information for us to work,
  // so just be passive in those cases.
  if (!_weightOnWheelsNode || !_groundspeedNode || !_engineControlsNode) {
    // pass the values straight through
    _leftBrakeOutput = _leftBrakeInput; 
    _rightBrakeOutput = _rightBrakeInput; 
    return;
  }

  if (dt <= 0.0) {
    return; // paused
  }
  
  double gs = _groundspeedNode->getDoubleValue();
  _actualDecel = (_lastGroundspeedKts - gs) / dt;
  _lastGroundspeedKts = gs;
  
  if (!_armed || (_step == 0)) {
    // even if we're off or disarmed, the physical valve system must pass
    // pilot control values through.
    _leftBrakeOutput = _leftBrakeInput; 
    _rightBrakeOutput = _rightBrakeInput; 
    return;
  }
  
  if (_engaged) {
    updateEngaged(dt);
  } else {
    bool e = shouldEngage();
    if (e) {
      engage();
      updateEngaged(dt);
    }
  }
  
  // sum pilot inpiuts with auto-brake FCS input
  _leftBrakeOutput = SGMiscd::max(_leftBrakeInput, _fcsBrakeControl);
  _rightBrakeOutput = SGMiscd::max(_rightBrakeInput, _fcsBrakeControl);

  _lastWoW = _weightOnWheelsNode->getBoolValue();
}

void FGAutoBrake::engage()
{
  SG_LOG(SG_AUTOPILOT, SG_INFO, "FGAutoBrake: engaging");
  _engaged = true;
}

void FGAutoBrake::updateEngaged(double dt)
{
  // check for pilot braking input cancelling engage
  const double BRAKE_INPUT_THRESHOLD = 0.02; // 2% application triggers
  if ((_leftBrakeInput > BRAKE_INPUT_THRESHOLD) || (_rightBrakeInput > BRAKE_INPUT_THRESHOLD)) {
    SG_LOG(SG_AUTOPILOT, SG_INFO, "auto-brake, detected pilot brake input, disengaing");
    disengage();
    return;
  }

  // update the target deceleration; note step can be changed while engaged
  if (_rto) {
    _targetDecel = _configRTODecel;
  } else {
    double f = (double) _step / _configNumSteps;
    _targetDecel = _configMaxDecel * f;
  }
}

bool FGAutoBrake::shouldEngage()
{
  if (_rto) {
    return shouldEngageRTO();
  }

  if (_lastWoW || !_weightOnWheelsNode->getBoolValue()) {
    return false;
  }

  SG_LOG(SG_AUTOPILOT, SG_INFO, "Autobrake, should enage");
  return true;
}

bool FGAutoBrake::shouldEngageRTO()
{
  double speed = _groundspeedNode->getDoubleValue();

  if (_rtoArmed) {
    if (speed < _configRTOSpeed) {
      SG_LOG(SG_AUTOPILOT, SG_INFO, "FGAutoBrake: RTO armed, but speed is now below arming speed");
      _rtoArmed = false;
      return false;
    }
    
    if (!_weightOnWheelsNode->getBoolValue()) {
      return false;
    }
    
    return throttlesAtIdle();
  } else {
    // RTO arming case
    if (speed > _configRTOSpeed) {
      SG_LOG(SG_AUTOPILOT, SG_INFO, "Autobrake RTO: passed " << _configRTOSpeed << " knots, arming RTO mode");
      _rtoArmed = true;
    }
  }
  
  return false;
}

void FGAutoBrake::disengage()
{
  if (!_engaged) {
    return;
  }
  
  SG_LOG(SG_AUTOPILOT, SG_INFO, "FGAutoBrake: disengage");
  _engaged = false;
  _armed = false;
  _rtoArmed = false;
  _targetDecel = 0.0;
}

bool FGAutoBrake::throttlesAtIdle()
{
  SGPropertyNode_ptr e;
  const double IDLE_THROTTLE = 0.05; // 5% margin for idle setting
  
  for (int index=0; (e = _engineControlsNode->getChild("engine", index)) != NULL; ++index) {
    if ((e->getDoubleValue("throttle") > IDLE_THROTTLE) && !e->getBoolValue("reverser")) {
      return false;
    }
  } // of engines iteration
  
  SG_LOG(SG_AUTOPILOT, SG_INFO, "FGAutoBrake: throttles at idle");
  return true;
}

void FGAutoBrake::setArmed(bool aArmed)
{
  if (aArmed == _armed) {
    return;
  }
    
  disengage();
  _armed = aArmed;
}

void FGAutoBrake::setRTO(bool aRTO)
{
  if (aRTO) {
    // ensure we meet RTO selection criteria: 
    if (!_weightOnWheelsNode->getBoolValue()) {
      SG_LOG(SG_AUTOPILOT, SG_WARN, "auto-brake: cannot select RTO mode, no weight on wheels");
      return;
    }
    
    _rtoArmed = false;
    _rto = true;
    _step = _configRTOStep;
    setArmed(true);
    SG_LOG(SG_AUTOPILOT, SG_INFO, "RTO mode set");
  } else {
    _rto = false;
    _rtoArmed = false;
    // and if we're enaged?
    disengage(); 
  }
}

void FGAutoBrake::setStep(int aStep)
{
  if (aStep == _step) {
    return;
  }
  
  SG_LOG(SG_AUTOPILOT, SG_INFO, "Autobrake step now=" << aStep);
  _step = aStep;
  bool validStep = false;
  
  if (aStep == _configRTOStep) {
    setRTO(true);
    validStep = true;
  } else {
    _rto = false;
    validStep = (_step > 0) && (_step <= _configNumSteps);
  }
  
  setArmed(validStep); // selecting a valid step arms the system
}


