// TimeManager.cxx -- simulation-wide time management
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "TimeManager.hxx"

#include <simgear/timing/sg_time.hxx>
#include <simgear/structure/event_mgr.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/timing/lowleveltime.h>
#include <simgear/structure/commands.hxx>

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Time/sunsolver.hxx>

using std::string;

static bool do_timeofday (const SGPropertyNode * arg)
{
    const string &offset_type = arg->getStringValue("timeofday", "noon");
    int offset = arg->getIntValue("offset", 0);
    TimeManager* self = (TimeManager*) globals->get_subsystem("time");
    if (offset_type == "real") {
    // without this, setting 'real' time is a no-op, since the current
    // wrap value (orig_warp) is retained in setTimeOffset. Ick.
        fgSetInt("/sim/time/warp", 0);
    }
    
    self->setTimeOffset(offset_type, offset);
    return true;
}

TimeManager::TimeManager() :
  _inited(false),
  _impl(NULL),
  _sceneryLoaded("sim/sceneryloaded"),
  _sceneryLoadOverride("sim/sceneryloaded-override"),
  _modelHz("sim/model-hz"),
  _timeDelta("sim/time/delta-realtime-sec"),
  _simTimeDelta("sim/time/delta-sec")
{
  SGCommandMgr::instance()->addCommand("timeofday", do_timeofday);
}

void TimeManager::init()
{
  if (_inited) {
    // time manager has to be initialised early, so needs to be defensive
    // about multiple initialisation 
    return; 
  }
  
  _firstUpdate = true;
  _inited = true;
  _dtRemainder = 0.0;
  _adjustWarpOnUnfreeze = false;
  
  _maxDtPerFrame = fgGetNode("/sim/max-simtime-per-frame", true);
  _clockFreeze = fgGetNode("/sim/freeze/clock", true);
  _timeOverride = fgGetNode("/sim/time/cur-time-override", true);
  _warp = fgGetNode("/sim/time/warp", true);
  _warp->addChangeListener(this);
  
  _warpDelta = fgGetNode("/sim/time/warp-delta", true);
  
  _longitudeDeg = fgGetNode("/position/longitude-deg", true);
  _latitudeDeg = fgGetNode("/position/latitude-deg", true);
  
  SGPath zone(globals->get_fg_root());
  zone.append("Timezone");
  double lon = _longitudeDeg->getDoubleValue() * SG_DEGREES_TO_RADIANS;
  double lat = _latitudeDeg->getDoubleValue() * SG_DEGREES_TO_RADIANS;
  
  _impl = new SGTime(lon, lat, zone.str(), _timeOverride->getLongValue());
  
  _warpDelta->setIntValue(0);
  
  globals->get_event_mgr()->addTask("updateLocalTime", this,
                            &TimeManager::updateLocalTime, 30*60 );
  updateLocalTime();
  
  _impl->update(lon, lat, _timeOverride->getLongValue(),
               _warp->getIntValue());
  globals->set_time_params(_impl);
    
  // frame-rate / worst-case latency / update-rate counters
  _frameRate = fgGetNode("/sim/frame-rate", true);
  _frameLatency = fgGetNode("/sim/frame-latency-max-ms", true);
  _frameRateWorst = fgGetNode("/sim/frame-rate-worst", true);
  _lastFrameTime = 0;
  _frameLatencyMax = 0.0;
  _frameCount = 0;
}

void TimeManager::postinit()
{
  initTimeOffset();
}

void TimeManager::reinit()
{
  shutdown();
  init();
  postinit();
}

void TimeManager::shutdown()
{
  _warp->removeChangeListener(this);
  
  globals->set_time_params(NULL);
  delete _impl;
  _impl = NULL;
  _inited = false;
  globals->get_event_mgr()->removeTask("updateLocalTime");
}

void TimeManager::valueChanged(SGPropertyNode* aProp)
{
  if (aProp == _warp) {
    if (_clockFreeze->getBoolValue()) {
    // if the warp is changed manually while frozen, don't modify it when
    // un-freezing - the user wants to unfreeze with exactly the warp
    // they specified.
      _adjustWarpOnUnfreeze = false;
    }
    
    double lon = _longitudeDeg->getDoubleValue() * SG_DEGREES_TO_RADIANS;
    double lat = _latitudeDeg->getDoubleValue() * SG_DEGREES_TO_RADIANS;
    _impl->update(lon, lat,
                   _timeOverride->getLongValue(),
                   _warp->getIntValue());
  }
}

void TimeManager::computeTimeDeltas(double& simDt, double& realDt)
{
  // Update the elapsed time.
  if (_firstUpdate) {
    _lastStamp.stamp();
    _firstUpdate = false;
    _lastClockFreeze = _clockFreeze->getBoolValue();
  }

  bool wait_for_scenery = !(_sceneryLoaded || _sceneryLoadOverride);
  if (!wait_for_scenery) {
    throttleUpdateRate();
  }
  else
  {
      // suppress framerate while initial scenery isn't loaded yet (splash screen still active) 
      _lastFrameTime=0;
      _frameCount = 0;
  }
  
  SGTimeStamp currentStamp;
  currentStamp.stamp();
  double dt = (currentStamp - _lastStamp).toSecs();
  if (dt > _frameLatencyMax)
      _frameLatencyMax = dt;

// Limit the time we need to spend in simulation loops
// That means, if the /sim/max-simtime-per-frame value is strictly positive
// you can limit the maximum amount of time you will do simulations for
// one frame to display. The cpu time spent in simulations code is roughly
// at least O(real_delta_time_sec). If this is (due to running debug
// builds or valgrind or something different blowing up execution times)
// larger than the real time you will no longer get any response
// from flightgear. This limits that effect. Just set to property from
// your .fgfsrc or commandline ...
  double dtMax = _maxDtPerFrame->getDoubleValue();
  if (0 < dtMax && dtMax < dt) {
    dt = dtMax;
  }
    
  SGSubsystemGroup* fdmGroup = 
    globals->get_subsystem_mgr()->get_group(SGSubsystemMgr::FDM);
  fdmGroup->set_fixed_update_time(1.0 / _modelHz);
  
// round the real time down to a multiple of 1/model-hz.
// this way all systems are updated the _same_ amount of dt.
  dt += _dtRemainder;
  int multiLoop = long(floor(dt * _modelHz));
  multiLoop = SGMisc<long>::max(0, multiLoop);
  _dtRemainder = dt - double(multiLoop)/double(_modelHz);
  dt = double(multiLoop)/double(_modelHz);

  realDt = dt;
  if (_clockFreeze->getBoolValue() || wait_for_scenery) {
    simDt = 0;
  } else {
    simDt = dt;
  }
  
  _lastStamp = currentStamp;
  globals->inc_sim_time_sec(simDt);

// These are useful, especially for Nasal scripts.
  _timeDelta = realDt;
  _simTimeDelta = simDt;
}

void TimeManager::update(double dt)
{
  bool freeze = _clockFreeze->getBoolValue();
  time_t now = time(NULL);
  
  if (freeze) {
    // clock freeze requested
    if (_timeOverride->getLongValue() == 0) {
      _timeOverride->setLongValue(now);
      _adjustWarpOnUnfreeze = true;
    }
  } else {
    // no clock freeze requested
    if (_lastClockFreeze) {
      if (_adjustWarpOnUnfreeze) {
      // clock just unfroze, let's set warp as the difference
      // between frozen time and current time so we don't get a
      // time jump (and corresponding sky object and lighting
      // jump.)
        int adjust = _timeOverride->getLongValue() - now;
        SG_LOG(SG_GENERAL, SG_INFO, "adjusting on un-freeze:" << adjust);
        _warp->setIntValue(_warp->getIntValue() + adjust);
      }
      _timeOverride->setLongValue(0);
    }
    
    int warpDelta = _warpDelta->getIntValue();
    if (warpDelta != 0) {
      _warp->setIntValue(_warp->getIntValue() + warpDelta);
    }
  }

  _lastClockFreeze = freeze;
  double lon = _longitudeDeg->getDoubleValue() * SG_DEGREES_TO_RADIANS;
  double lat = _latitudeDeg->getDoubleValue() * SG_DEGREES_TO_RADIANS;
  _impl->update(lon, lat,
               _timeOverride->getLongValue(),
               _warp->getIntValue());

  computeFrameRate();
}

void TimeManager::computeFrameRate()
{
  // Calculate frame rate average
  if ((_impl->get_cur_time() != _lastFrameTime)) {
    _frameRate->setIntValue(_frameCount);
    _frameLatency->setDoubleValue(_frameLatencyMax*1000);
    if (_frameLatencyMax>0)
        _frameRateWorst->setIntValue(1/_frameLatencyMax);
    _frameCount = 0;
    _frameLatencyMax = 0.0;
  }
  
  _lastFrameTime = _impl->get_cur_time();
  ++_frameCount;
}

void TimeManager::throttleUpdateRate()
{
  // common case, no throttle requested
  double throttle_hz = fgGetDouble("/sim/frame-rate-throttle-hz", 0.0);
  if (throttle_hz <= 0)
    return; // no-op

  // sleep for exactly 1/hz seconds relative to the past valid timestamp
  SGTimeStamp::sleepUntil(_lastStamp + SGTimeStamp::fromSec(1/throttle_hz));
}

// periodic time updater wrapper
void TimeManager::updateLocalTime() 
{
  SGPath zone(globals->get_fg_root());
  zone.append("Timezone");
  
  double lon = _longitudeDeg->getDoubleValue() * SG_DEGREES_TO_RADIANS;
  double lat = _latitudeDeg->getDoubleValue() * SG_DEGREES_TO_RADIANS;
  
  SG_LOG(SG_GENERAL, SG_INFO, "updateLocal(" << lon << ", " << lat << ", " << zone.str() << ")");
  _impl->updateLocal(lon, lat, zone.str());
}

void TimeManager::initTimeOffset()
{

  long int offset = fgGetLong("/sim/startup/time-offset");
  string offset_type = fgGetString("/sim/startup/time-offset-type");
  setTimeOffset(offset_type, offset);
}

void TimeManager::setTimeOffset(const std::string& offset_type, long int offset)
{
  // Handle potential user specified time offsets
  int orig_warp = _warp->getIntValue();
  time_t cur_time = _impl->get_cur_time();
  time_t currGMT = sgTimeGetGMT( gmtime(&cur_time) );
  time_t systemLocalTime = sgTimeGetGMT( localtime(&cur_time) );
  time_t aircraftLocalTime = 
      sgTimeGetGMT( fgLocaltime(&cur_time, _impl->get_zonename() ) );
    
  // Okay, we now have several possible scenarios
  double lon = _longitudeDeg->getDoubleValue() * SG_DEGREES_TO_RADIANS;
  double lat = _latitudeDeg->getDoubleValue() * SG_DEGREES_TO_RADIANS;
  int warp = 0;
  
  if ( offset_type == "real" ) {
      warp = 0;
  } else if ( offset_type == "dawn" ) {
      warp = fgTimeSecondsUntilSunAngle( cur_time, lon, lat, 90.0, true ); 
  } else if ( offset_type == "morning" ) {
     warp = fgTimeSecondsUntilSunAngle( cur_time, lon, lat, 75.0, true ); 
  } else if ( offset_type == "noon" ) {
     warp = fgTimeSecondsUntilSunAngle( cur_time, lon, lat, 0.0, true ); 
  } else if ( offset_type == "afternoon" ) {
    warp = fgTimeSecondsUntilSunAngle( cur_time, lon, lat, 75.0, false );  
  } else if ( offset_type == "dusk" ) {
    warp = fgTimeSecondsUntilSunAngle( cur_time, lon, lat, 90.0, false );
  } else if ( offset_type == "evening" ) {
    warp = fgTimeSecondsUntilSunAngle( cur_time, lon, lat, 100.0, false );
  } else if ( offset_type == "midnight" ) {
    warp = fgTimeSecondsUntilSunAngle( cur_time, lon, lat, 180.0, false );
  } else if ( offset_type == "system-offset" ) {
    warp = offset;
    orig_warp = 0;
  } else if ( offset_type == "gmt-offset" ) {
    warp = offset - (currGMT - systemLocalTime);
    orig_warp = 0;
  } else if ( offset_type == "latitude-offset" ) {
    warp = offset - (aircraftLocalTime - systemLocalTime);
    orig_warp = 0;
  } else if ( offset_type == "system" ) {
    warp = offset - (systemLocalTime - currGMT) - cur_time;
  } else if ( offset_type == "gmt" ) {
      warp = offset - cur_time;
  } else if ( offset_type == "latitude" ) {
      warp = offset - (aircraftLocalTime - currGMT)- cur_time; 
  } else {
    SG_LOG( SG_GENERAL, SG_ALERT,
          "TimeManager::setTimeOffset: unsupported offset: " << offset_type );
     warp = 0;
  }
  
  _warp->setIntValue( orig_warp + warp );

  SG_LOG( SG_GENERAL, SG_INFO, "After TimeManager::setTimeOffset(): warp = " 
            << _warp->getIntValue() );
}
