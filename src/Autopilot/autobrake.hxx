// autobrake.hxx - generic, configurable autobrake system
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

#ifndef FG_INSTR_AUTOBRAKE_HXX
#define FG_INSTR_AUTOBRAKE_HXX

#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>

// forward decls

class FGAutoBrake : public SGSubsystem
{
public:
  FGAutoBrake();
  virtual ~FGAutoBrake();

  virtual void init ();
  virtual void postinit ();
  virtual void bind ();
  virtual void unbind ();
  virtual void update (double dt);
    
private:
  
  void engage();
  void disengage();
  
  void updateEngaged(double dt);
  
  bool shouldEngage();
  bool shouldEngageRTO();
  
  /**
   * Helper to determine if all throttles are at idle
   * (or have reverse thrust engaged)
   */
  bool throttlesAtIdle() const;
  
  /**
   * Helper to determine if we're airbone, i.e weight off all wheels
   */
  bool airborne() const;
  
// accessors, mostly for SGRawValueMethods use
  void setArmed(bool aArmed);
  bool getArmed() const { return _armed; }
  
  void setRTO(bool aRTO);
  bool getRTO() const { return _rto; }
  
  void setStep(int aStep);
  int getStep() const { return _step; }
  
  bool getEngaged() const { return _engaged;}
  double getTargetDecel() const { return _targetDecel; }
  double getActualDecel() const { return _actualDecel; }

// members
  double _lastGroundspeedKts;
  int _step;
  bool _rto; // true if in Rejected TakeOff mode
  bool _armed;
  bool _rtoArmed; ///< true if we have met arming criteria for RTO mode
  bool _engaged; ///< true if auto-braking is currently active
  double _targetDecel; // target deceleration ft-sec^2
  double _actualDecel; // measured current deceleration in ft-sec^2
  double _fcsBrakeControl;
  bool _lastWoW;
  double _leftBrakeInput; // summed pilot and co-pilot left brake input
  double _rightBrakeInput; // summed pilot and co-pilot right brake input
  double _leftBrakeOutput;
  double _rightBrakeOutput;
    
  SGPropertyNode_ptr _root;
  SGPropertyNode_ptr _brakeInputs[4];
  SGPropertyNode_ptr _weightOnWheelsNode;
  SGPropertyNode_ptr _engineControlsNode;
  SGPropertyNode_ptr _groundspeedNode;
  
  int _configNumSteps;
  int _configRTOStep;
  int _configDisengageStep;
  double _configMaxDecel; ///< deceleration (in ft-sec^2) corresponding to step=numSteps
  double _configRTODecel; ///< deceleration (in ft-sec^2) to use in RTO mode
  double _configRTOSpeed; ///< speed (in kts) for RTO mode to arm
};

#endif // of FG_INSTR_AUTOBRAKE_HXX
