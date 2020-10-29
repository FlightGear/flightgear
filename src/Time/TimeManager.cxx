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
#include <simgear/math/SGMath.hxx>

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Time/bodysolver.hxx>

using std::string;

static bool do_timeofday (const SGPropertyNode * arg, SGPropertyNode * root)
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
  _impl(NULL)
{
  globals->get_commands()->addCommand("timeofday", do_timeofday);
}

TimeManager::~TimeManager()
{
   globals->get_commands()->removeCommand("timeofday");
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
  _mpProtocolClock = _steadyClock = 0.0;
  _adjustWarpOnUnfreeze = false;
  
  _maxDtPerFrame = fgGetNode("/sim/max-simtime-per-frame", true);
  _clockFreeze = fgGetNode("/sim/freeze/clock", true);
  _timeOverride = fgGetNode("/sim/time/cur-time-override", true);
  _warp = fgGetNode("/sim/time/warp", true);
  _warp->addChangeListener(this);
  _maxFrameRate  = fgGetNode("/sim/frame-rate-throttle-hz", true);

  _warpDelta = fgGetNode("/sim/time/warp-delta", true);
  
  SGPath zone(globals->get_fg_root());
  zone.append("Timezone");
  
  _impl = new SGTime(globals->get_aircraft_position(), zone, _timeOverride->getLongValue());
  
  _warpDelta->setDoubleValue(0.0);
  
  globals->get_event_mgr()->addTask("updateLocalTime", this,
                            &TimeManager::updateLocalTime, 30*60 );
  updateLocalTime();
  
  _impl->update(globals->get_aircraft_position(), _timeOverride->getLongValue(),
               _warp->getIntValue());
  globals->set_time_params(_impl);
    
  // frame-rate / worst-case latency / update-rate counters
  _frameRate = fgGetNode("/sim/frame-rate", true);
  _frameLatency = fgGetNode("/sim/frame-latency-max-ms", true);
  _frameRateWorst = fgGetNode("/sim/frame-rate-worst", true);
  _lastFrameTime = 0;
  _frameLatencyMax = 0.0;
  _frameCount = 0;
    
    _sceneryLoaded = fgGetNode("sim/sceneryloaded", true);
    _modelHz = fgGetNode("sim/model-hz", true);
    _timeDelta = fgGetNode("sim/time/delta-realtime-sec", true);
    _simTimeDelta = fgGetNode("sim/time/delta-sec", true);
    _mpProtocolClockNode = fgGetNode("sim/time/mp-clock-sec", true);
    _steadyClockNode = fgGetNode("sim/time/steady-clock-sec", true);
    _mpClockOffset = fgGetNode("sim/time/mp-clock-offset-sec", true);
    _steadyClockDrift = fgGetNode("sim/time/steady-clock-drift-ms", true);
    _computeDrift = fgGetNode("sim/time/compute-clock-drift", true);
    _frameWait = fgGetNode("sim/time/frame-wait-ms", true);
    _simTimeFactor = fgGetNode("/sim/speed-up", true);
    // use pre-set value but ensure we get a sane default
    if (!_simTimeDelta->hasValue()) {
        _simTimeFactor->setDoubleValue(1.0);
    }
    if (!_mpClockOffset->hasValue()) {
        _mpClockOffset->setDoubleValue(0.0);
    }
    _computeDrift->setBoolValue(true);
}

void TimeManager::unbind()
{
    _maxDtPerFrame.clear();
    _clockFreeze.clear();
    _timeOverride.clear();
    _warp.clear();
    _warpDelta.clear();
    _frameRate.clear();
    _frameLatency.clear();
    _frameRateWorst.clear();
    _frameWait.clear();
    _maxFrameRate.clear();

    _sceneryLoaded.clear();
    _modelHz.clear();
    _timeDelta.clear();
    _simTimeDelta.clear();
    _mpProtocolClockNode.clear();
    _steadyClockNode.clear();
    _mpClockOffset.clear();
    _steadyClockDrift.clear();
    _computeDrift.clear();
    _simTimeFactor.clear();
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
    
    _impl->update(globals->get_aircraft_position(),
                   _timeOverride->getLongValue(),
                   _warp->getIntValue());
  }
}

void TimeManager::computeTimeDeltas(double& simDt, double& realDt)
{
  // Update the elapsed time.
  if (_firstUpdate) {
    _lastStamp.stamp();

    // we initialise the mp protocol clock  with the system clock.
    _systemStamp.systemClockHoursAndMinutes();
    _steadyClock = _systemStamp.toSecs();

    _firstUpdate = false;
    _lastClockFreeze = _clockFreeze->getBoolValue();
  }

  bool wait_for_scenery = !_sceneryLoaded->getBoolValue();
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

  // if asked, we compute the drift between the steady clock and the system clock

  if (_computeDrift->getBoolValue()) {
      _systemStamp.systemClockHoursAndMinutes();
      double clockdrift = _steadyClock + (currentStamp - _lastStamp).toSecs()
        + _dtRemainder - _systemStamp.toSecs();
      _steadyClockDrift->setDoubleValue(clockdrift * 1000.0);
      _computeDrift->setBoolValue(false);
  }

  // this dt will be clamped by the max sim time by frame.
  double dt = (currentStamp - _lastStamp).toSecs();

  // here we have a true real dt for a clock "real time".
  double mpProtocolDt = dt;

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
  double modelHz = _modelHz->getDoubleValue();
  fdmGroup->set_fixed_update_time(1.0 / modelHz);

// round the real time down to a multiple of 1/model-hz.
// this way all systems are updated the _same_ amount of dt.
  dt += _dtRemainder;

  // we keep the mp clock sync with the sim time, as it's used as timestamp
  // in fdm state,
  mpProtocolDt += _dtRemainder;
  int multiLoop = long(floor(dt * modelHz));
  multiLoop = SGMisc<long>::max(0, multiLoop);
  _dtRemainder = dt - double(multiLoop)/modelHz;
  dt = double(multiLoop)/modelHz;
  mpProtocolDt -= _dtRemainder;

  realDt = dt;
  if (_clockFreeze->getBoolValue() || wait_for_scenery) {
    simDt = 0;
  } else {
      // sim time can be scaled
      simDt = dt * _simTimeFactor->getDoubleValue();
  }

  _lastStamp = currentStamp;
  globals->inc_sim_time_sec(simDt);
  _steadyClock += mpProtocolDt;
  _mpProtocolClock = _steadyClock + _mpClockOffset->getDoubleValue();

  _steadyClockNode->setDoubleValue(_steadyClock);
  _mpProtocolClockNode->setDoubleValue(_mpProtocolClock);

// These are useful, especially for Nasal scripts.
  _timeDelta->setDoubleValue(realDt);
  _simTimeDelta->setDoubleValue(simDt);
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
        SG_LOG(SG_GENERAL, SG_DEBUG, "adjusting on un-freeze:" << adjust);
        _warp->setIntValue(_warp->getIntValue() + adjust);
      }
      _timeOverride->setLongValue(0);
    }

      // account for speed-up in warp value. This implies when speed-up is not
      // 1.0 we need to continually adjust warp, either forwards for speed-up,
      // or backwards for a slow-down. Eg for a speed up of 4x, we want to
      // incease warp by 3 additional seconds per elapsed real second.
      // for a 1/2x factor, we want to decrease warp by half a second per
      // elapsed real second.
      double speedUp = _simTimeFactor->getDoubleValue() - 1.0;
      if (speedUp != 0.0) {
          double realDt = _timeDelta->getDoubleValue();
          double speedUpOffset = speedUp * realDt;
          _warp->setDoubleValue(_warp->getDoubleValue() + speedUpOffset);
      }
  } // of sim not frozen

    // scale warp-delta by real-dt, so rate is constant with frame-rate,
    // but warping works while paused
    int warpDelta = _warpDelta->getIntValue();
    if (warpDelta) {
        _adjustWarpOnUnfreeze = false;
        double warpOffset = warpDelta * _timeDelta->getDoubleValue();
        _warp->setDoubleValue(_warp->getDoubleValue() + warpOffset);
    }

  _lastClockFreeze = freeze;
  _impl->update(globals->get_aircraft_position(),
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
  double throttle_hz = _maxFrameRate->getDoubleValue();

  // no delay required.
  if (throttle_hz <= 0)
  {
      _frameWait->setDoubleValue(0);
      return;
  }
  SGTimeStamp frameWaitStart = SGTimeStamp::now();

  // sleep for exactly 1/hz seconds relative to the past valid timestamp
  SGTimeStamp::sleepUntil(_lastStamp + SGTimeStamp::fromSec(1 / throttle_hz));
  _frameWait->setDoubleValue(frameWaitStart.elapsedMSec());
}

// periodic time updater wrapper
void TimeManager::updateLocalTime() 
{
  _impl->updateLocal(globals->get_aircraft_position(), globals->get_fg_root() / "Timezone");
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
  SGGeod loc = globals->get_aircraft_position();
  int warp = 0;
  
  if ( offset_type == "real" ) {
      warp = 0;
  } else if ( offset_type == "dawn" ) {
      warp = fgTimeSecondsUntilBodyAngle( cur_time, loc, 90.0, true, true );
  } else if ( offset_type == "morning" ) {
     warp = fgTimeSecondsUntilBodyAngle( cur_time, loc, 75.0, true, true );
  } else if ( offset_type == "noon" ) {
     warp = fgTimeSecondsUntilBodyAngle( cur_time, loc, 0.0, true, true );
  } else if ( offset_type == "afternoon" ) {
    warp = fgTimeSecondsUntilBodyAngle( cur_time, loc, 75.0, false, true );
  } else if ( offset_type == "dusk" ) {
    warp = fgTimeSecondsUntilBodyAngle( cur_time, loc, 90.0, false, true );
  } else if ( offset_type == "evening" ) {
    warp = fgTimeSecondsUntilBodyAngle( cur_time, loc, 100.0, false, true );
  } else if ( offset_type == "midnight" ) {
    warp = fgTimeSecondsUntilBodyAngle( cur_time, loc, 180.0, false, true );
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
  
  if( fgGetBool("/sim/time/warp-easing", false) && !fgGetBool("/devices/status/keyboard/ctrl", false)) {
    double duration = fgGetDouble("/sim/time/warp-easing-duration-secs", 5.0 );
    const string easing = fgGetString("/sim/time/warp-easing-method", "swing" );
    SGPropertyNode n;
    n.setDoubleValue( orig_warp + warp );
    _warp->interpolate( "numeric", n, duration, easing );
  } else {
    _warp->setIntValue( orig_warp + warp );
  }

  SG_LOG( SG_GENERAL, SG_INFO, "After TimeManager::setTimeOffset(): warp = " 
            << _warp->getIntValue() );
}

double TimeManager::getSimSpeedUpFactor() const
{
    return _simTimeFactor->getDoubleValue();
}

// Register the subsystem.
SGSubsystemMgr::Registrant<TimeManager> registrantTimeManager(
    SGSubsystemMgr::INIT,
    {{"FDM", SGSubsystemMgr::Dependency::HARD}});
