// TimeManager.hxx -- simulation-wide time management
//
// Written by James Turner, started July 2010.
//
// Copyright (C) 2010  Curtis L. Olson  - http://www.flightgear.org/~curt
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

#ifndef FG_TIME_TIMEMANAGER_HXX
#define FG_TIME_TIMEMANAGER_HXX

#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/props/props.hxx>
#include <simgear/props/propertyObject.hxx>

// forward decls
class SGTime;

class TimeManager : public SGSubsystem, public SGPropertyChangeListener
{
public:
  TimeManager();
  
  void computeTimeDeltas(double& simDt, double& realDt);
  
  virtual void init();
  virtual void reinit();
  virtual void postinit();
  virtual void shutdown();
  
  void update(double dt);
  
// SGPropertyChangeListener overrides
  virtual void valueChanged(SGPropertyNode *);
  
  void setTimeOffset(const std::string& offset_type, long int offset);
private:
  
  /**
   * Ensure a consistent update-rate using a combination of
   * sleep()-ing and busy-waiting.  
   */
  void throttleUpdateRate();
  
  /**
   * Compute frame (update) rate and write it to a property
   */
  void computeFrameRate();
  
  void updateLocalTime();
  
  // set up a time offset (aka warp) if one is specified
  void initTimeOffset();
  
  bool _inited;
  SGTime* _impl;
  SGTimeStamp _lastStamp;
  bool _firstUpdate;
  double _dtRemainder;
  SGPropertyNode_ptr _maxDtPerFrame;
  SGPropertyNode_ptr _clockFreeze;
  SGPropertyNode_ptr _timeOverride;
  SGPropertyNode_ptr _warp;
  SGPropertyNode_ptr _warpDelta;
  
  bool _lastClockFreeze;
  bool _adjustWarpOnUnfreeze;
  
  SGPropertyNode_ptr _longitudeDeg;
  SGPropertyNode_ptr _latitudeDeg;
  
  // frame-rate / worst-case latency / update-rate counters
  SGPropertyNode_ptr _frameRate;
  SGPropertyNode_ptr _frameRateWorst;
  SGPropertyNode_ptr _frameLatency;
  time_t _lastFrameTime;
  double _frameLatencyMax;
  int _frameCount;
  
  SGPropObjBool _sceneryLoaded, 
    _sceneryLoadOverride;
  SGPropObjInt _modelHz;
  SGPropObjDouble _timeDelta, _simTimeDelta;
};

#endif // of FG_TIME_TIMEMANAGER_HXX
