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

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h> // for Sleep()
#else
#  include <unistd.h> // for usleep()
#endif

#include <simgear/timing/sg_time.hxx>
#include <simgear/structure/event_mgr.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/timing/lowleveltime.h>

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Time/sunsolver.hxx>

TimeManager::TimeManager() :
  _inited(false),
  _impl(NULL)
{
  
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
  
  _maxDtPerFrame = fgGetNode("/sim/max-simtime-per-frame", true);
  _clockFreeze = fgGetNode("/sim/freeze/clock", true);
  _timeOverride = fgGetNode("/sim/time/cur-time-override", true);
  
  _longitudeDeg = fgGetNode("/position/longitude-deg", true);
  _latitudeDeg = fgGetNode("/position/latitude-deg", true);
  
  SGPath zone(globals->get_fg_root());
  zone.append("Timezone");
  double lon = _longitudeDeg->getDoubleValue() * SG_DEGREES_TO_RADIANS;
  double lat = _latitudeDeg->getDoubleValue() * SG_DEGREES_TO_RADIANS;
  _impl = new SGTime(lon, lat, zone.str(), _timeOverride->getLongValue());
  
  globals->set_warp_delta(0);
  
  globals->get_event_mgr()->addTask("updateLocalTime", this,
                            &TimeManager::updateLocalTime, 30*60 );
  updateLocalTime();
  
  _impl->update(lon, lat, _timeOverride->getLongValue(),
               globals->get_warp());
  globals->set_time_params(_impl);
    
// frame/update-rate counters
  _frameRate = fgGetNode("/sim/frame-rate", true);
  _lastFrameTime = _impl->get_cur_time();
  _frameCount = 0;
}

void TimeManager::postinit()
{
  initTimeOffset();
}

void TimeManager::reinit()
{
  globals->set_time_params(NULL);
  delete _impl;
  _inited = false;
  globals->get_event_mgr()->removeTask("updateLocalTime");
  
  init();
  postinit();
}

void TimeManager::computeTimeDeltas(double& simDt, double& realDt)
{
  // Update the elapsed time.
  if (_firstUpdate) {
    _lastStamp.stamp();
    _firstUpdate = false;
    _lastClockFreeze = _clockFreeze->getBoolValue();
  }

  bool scenery_loaded = fgGetBool("sim/sceneryloaded");
  bool wait_for_scenery = !(scenery_loaded || fgGetBool("sim/sceneryloaded-override"));
  
  if (!wait_for_scenery) {
    throttleUpdateRate();
  }
  
  SGTimeStamp currentStamp;
  currentStamp.stamp();
  double dt = (currentStamp - _lastStamp).toSecs();
  
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
  
  int model_hz = fgGetInt("/sim/model-hz");
  
  SGSubsystemGroup* fdmGroup = 
    globals->get_subsystem_mgr()->get_group(SGSubsystemMgr::FDM);
  fdmGroup->set_fixed_update_time(1.0 / model_hz);
  
// round the real time down to a multiple of 1/model-hz.
// this way all systems are updated the _same_ amount of dt.
  dt += _dtRemainder;
  int multiLoop = long(floor(dt * model_hz));
  multiLoop = SGMisc<long>::max(0, multiLoop);
  _dtRemainder = dt - double(multiLoop)/double(model_hz);
  dt = double(multiLoop)/double(model_hz);

  realDt = dt;
  if (_clockFreeze->getBoolValue() || wait_for_scenery) {
    simDt = 0;
  } else {
    simDt = dt;
  }
  
  _lastStamp = currentStamp;
  globals->inc_sim_time_sec(simDt);

// These are useful, especially for Nasal scripts.
  fgSetDouble("/sim/time/delta-realtime-sec", realDt);
  fgSetDouble("/sim/time/delta-sec", simDt);
}

void TimeManager::update(double dt)
{
  bool freeze = _clockFreeze->getBoolValue();
  if (freeze) {
    // clock freeze requested
    if (_timeOverride->getLongValue() == 0) {
      fgSetLong( "/sim/time/cur-time-override", _impl->get_cur_time());
      globals->set_warp(0);
    }
  } else {
    // no clock freeze requested
    if (_lastClockFreeze) {
    // clock just unfroze, let's set warp as the difference
    // between frozen time and current time so we don't get a
    // time jump (and corresponding sky object and lighting
    // jump.)
      globals->set_warp(_timeOverride->getLongValue() - time(NULL));
      fgSetLong( "/sim/time/cur-time-override", 0 );
    }
    
    if ( globals->get_warp_delta() != 0 ) {
      globals->inc_warp( globals->get_warp_delta() );
    }
  }

  _lastClockFreeze = freeze;
  double lon = _longitudeDeg->getDoubleValue() * SG_DEGREES_TO_RADIANS;
  double lat = _latitudeDeg->getDoubleValue() * SG_DEGREES_TO_RADIANS;
  _impl->update(lon, lat,
               _timeOverride->getLongValue(),
               globals->get_warp());

  computeFrameRate();
}

void TimeManager::computeFrameRate()
{
  // Calculate frame rate average
  if ((_impl->get_cur_time() != _lastFrameTime) && (_lastFrameTime > 0)) {
    _frameRate->setIntValue(_frameCount);
    _frameCount = 0;
  }
  
  _lastFrameTime = _impl->get_cur_time();
  ++_frameCount;
}

void TimeManager::throttleUpdateRate()
{
  double throttle_hz = fgGetDouble("/sim/frame-rate-throttle-hz", 0.0);
  SGTimeStamp currentStamp;
  
  // common case, no throttle requested
  if (throttle_hz <= 0.0) {
    return; // no-op
  }
  
  double frame_us = 1000000.0 / throttle_hz;
#define FG_SLEEP_BASED_TIMING 1
#if defined(FG_SLEEP_BASED_TIMING)
  // sleep based timing loop.
  //
  // Calling sleep, even usleep() on linux is less accurate than
  // we like, but it does free up the cpu for other tasks during
  // the sleep so it is desirable.  Because of the way sleep()
  // is implemented in consumer operating systems like windows
  // and linux, you almost always sleep a little longer than the
  // requested amount.
  //
  // To combat the problem of sleeping too long, we calculate the
  // desired wait time and shorten it by 2000us (2ms) to avoid
  // [hopefully] over-sleep'ing.  The 2ms value was arrived at
  // via experimentation.  We follow this up at the end with a
  // simple busy-wait loop to get the final pause timing exactly
  // right.
  //
  // Assuming we don't oversleep by more than 2000us, this
  // should be a reasonable compromise between sleep based
  // waiting, and busy waiting.

  // sleep() will always overshoot by a bit so undersleep by
  // 2000us in the hopes of never oversleeping.
  frame_us -= 2000.0;
  if ( frame_us < 0.0 ) {
      frame_us = 0.0;
  }
  
  currentStamp.stamp();

  double elapsed_us = (currentStamp - _lastStamp).toUSecs();
  if ( elapsed_us < frame_us ) {
    double requested_us = frame_us - elapsed_us;
 
#ifdef _WIN32
    Sleep ((int)(requested_us / 1000.0)) ;
#else
    usleep(requested_us) ;
#endif
  }
#endif

  // busy wait timing loop.
  //
  // This yields the most accurate timing.  If the previous
  // ulMilliSecondSleep() call is omitted this will peg the cpu
  // (which is just fine if FG is the only app you care about.)
  currentStamp.stamp();
  SGTimeStamp next_time_stamp = _lastStamp;
  next_time_stamp += SGTimeStamp::fromSec(1e-6*frame_us);
  while ( currentStamp < next_time_stamp ) {
      currentStamp.stamp();
  }
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
  // Handle potential user specified time offsets
  int orig_warp = globals->get_warp();
  time_t cur_time = _impl->get_cur_time();
  time_t currGMT = sgTimeGetGMT( gmtime(&cur_time) );
  time_t systemLocalTime = sgTimeGetGMT( localtime(&cur_time) );
  time_t aircraftLocalTime = 
      sgTimeGetGMT( fgLocaltime(&cur_time, _impl->get_zonename() ) );
    
  // Okay, we now have several possible scenarios
  int offset = fgGetInt("/sim/startup/time-offset");
  string offset_type = fgGetString("/sim/startup/time-offset-type");
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
    warp = fgTimeSecondsUntilSunAngle( cur_time, lon, lat, 60.0, false );  
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
          "TimeManager::initTimeOffset: unsupported offset: " << offset_type );
     warp = 0;
  }
  
  globals->set_warp( orig_warp + warp );
  _impl->update(lon, lat, _timeOverride->getLongValue(),
               globals->get_warp() );

  SG_LOG( SG_GENERAL, SG_INFO, "After fgInitTimeOffset(): warp = " 
            << globals->get_warp() );
}
