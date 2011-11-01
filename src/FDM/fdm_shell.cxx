// fdm_shell.cxx -- encapsulate FDM implementations as well-behaved subsystems
//
// Written by James Turner, started June 2010.
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
//
// $Id$

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <cassert>
#include <simgear/structure/exception.hxx>

#include <FDM/fdm_shell.hxx>
#include <FDM/flight.hxx>
#include <Aircraft/replay.hxx>
#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <Scenery/scenery.hxx>

// all the FDMs, since we are the factory method
#ifdef ENABLE_SP_FDM
#include <FDM/SP/ADA.hxx>
#include <FDM/SP/ACMS.hxx>
#include <FDM/SP/MagicCarpet.hxx>
#include <FDM/SP/Balloon.h>
#endif
#include <FDM/ExternalNet/ExternalNet.hxx>
#include <FDM/ExternalPipe/ExternalPipe.hxx>

#ifdef ENABLE_JSBSIM
#include <FDM/JSBSim/JSBSim.hxx>
#endif

#ifdef ENABLE_LARCSIM
#include <FDM/LaRCsim/LaRCsim.hxx>
#endif

#include <FDM/UFO.hxx>
#include <FDM/NullFDM.hxx>

#ifdef ENABLE_YASIM
#include <FDM/YASim/YASim.hxx>
#endif

/*
 * Evil global variable required by Network/FGNative,
 * see that class for more information
 */
FGInterface* evil_global_fdm_state = NULL;

FDMShell::FDMShell() :
  _tankProperties( fgGetNode("/consumables/fuel", true) ),
  _impl(NULL),
  _dataLogging(false)
{
}

FDMShell::~FDMShell()
{
  delete _impl;
}

void FDMShell::init()
{
  _props = globals->get_props();
  fgSetBool("/sim/fdm-initialized", false);

  _wind_north       = _props->getNode("environment/wind-from-north-fps",    true);
  _wind_east        = _props->getNode("environment/wind-from-east-fps",     true);
  _wind_down        = _props->getNode("environment/wind-from-down-fps",     true);
  _control_fdm_atmo = _props->getNode("environment/params/control-fdm-atmosphere", true);
  _temp_degc        = _props->getNode("environment/temperature-degc",       true);
  _pressure_inhg    = _props->getNode("environment/pressure-inhg",          true);
  _density_slugft   = _props->getNode("environment/density-slugft3",        true);
  _data_logging     = _props->getNode("/sim/temp/fdm-data-logging",         true);
  _replay_master    = _props->getNode("/sim/freeze/replay-state",           true);

  createImplementation();
}

void FDMShell::reinit()
{
  if (_impl) {
    fgSetBool("/sim/fdm-initialized", false);
    evil_global_fdm_state = NULL;
    _impl->unbind();
    delete _impl;
    _impl = NULL;
  }
  
  init();
}

void FDMShell::bind()
{
  _tankProperties.bind();
  if (_impl && _impl->get_inited()) {
    if (_impl->get_bound()) {
      throw sg_exception("FDMShell::bind of bound FGInterface impl");
    }
    _impl->bind();
  }
}

void FDMShell::unbind()
{
  if( _impl ) _impl->unbind();
  _tankProperties.unbind();
}

void FDMShell::update(double dt)
{
  if (!_impl) {
    return;
  }
  
  if (!_impl->get_inited()) {
    // Check for scenery around the aircraft.
    double lon = fgGetDouble("/sim/presets/longitude-deg");
    double lat = fgGetDouble("/sim/presets/latitude-deg");
        
    double range = 1000.0; // in meters
    SGGeod geod = SGGeod::fromDeg(lon, lat);
    if (globals->get_scenery()->scenery_available(geod, range)) {
        SG_LOG(SG_FLIGHT, SG_INFO, "Scenery loaded, will init FDM");
        _impl->init();
        if (_impl->get_bound()) {
          _impl->unbind();
        }
        _impl->bind();
        
        evil_global_fdm_state = _impl;
        fgSetBool("/sim/fdm-initialized", true);
        fgSetBool("/sim/signals/fdm-initialized", true);
    }
  }

  if (!_impl->get_inited()) {
    return; // still waiting
  }

// pull environmental data in, since the FDMs are lazy
  _impl->set_Velocities_Local_Airmass(
          _wind_north->getDoubleValue(),
          _wind_east->getDoubleValue(),
          _wind_down->getDoubleValue());

  if (_control_fdm_atmo->getBoolValue()) {
    // convert from Rankine to Celsius
    double tempDegC = _temp_degc->getDoubleValue();
    _impl->set_Static_temperature((9.0/5.0) * (tempDegC + 273.15));
    
    // convert from inHG to PSF
    double pressureInHg = _pressure_inhg->getDoubleValue();
    _impl->set_Static_pressure(pressureInHg * 70.726566);
    // keep in slugs/ft^3
    _impl->set_Density(_density_slugft->getDoubleValue());
  }

  bool doLog = _data_logging->getBoolValue();
  if (doLog != _dataLogging) {
    _dataLogging = doLog;
    _impl->ToggleDataLogging(doLog);
  }

  switch(_replay_master->getIntValue())
  {
      case 0:
          // normal FDM operation
          _impl->update(dt);
          break;
      case 3:
          // resume FDM operation at current replay position
          _impl->reinit();
          break;
      default:
          // replay is active
          break;
  }
}

void FDMShell::createImplementation()
{
  assert(!_impl);
  
  double dt = 1.0 / fgGetInt("/sim/model-hz");
  string model = fgGetString("/sim/flight-model");

  if ( model == "ufo" ) {
    _impl = new FGUFO( dt );
  } else if ( model == "external" ) {
    // external is a synonym for "--fdm=null" and is
    // maintained here for backwards compatibility
    _impl = new FGNullFDM( dt );
  } else if ( model.find("network") == 0 ) {
    string host = "localhost";
    int port1 = 5501;
    int port2 = 5502;
    int port3 = 5503;
    string net_options = model.substr(8);
    string::size_type begin, end;
    begin = 0;
    // host
    end = net_options.find( ",", begin );
    if ( end != string::npos ) {
      host = net_options.substr(begin, end - begin);
      begin = end + 1;
    }
    // port1
    end = net_options.find( ",", begin );
    if ( end != string::npos ) {
      port1 = atoi( net_options.substr(begin, end - begin).c_str() );
      begin = end + 1;
    }
    // port2
    end = net_options.find( ",", begin );
    if ( end != string::npos ) {
      port2 = atoi( net_options.substr(begin, end - begin).c_str() );
      begin = end + 1;
    }
    // port3
    end = net_options.find( ",", begin );
    if ( end != string::npos ) {
      port3 = atoi( net_options.substr(begin, end - begin).c_str() );
      begin = end + 1;
    }
    _impl = new FGExternalNet( dt, host, port1, port2, port3 );
  } else if ( model.find("pipe") == 0 ) {
    // /* old */ string pipe_path = model.substr(5);
    // /* old */ _impl = new FGExternalPipe( dt, pipe_path );
    string pipe_path = "";
    string pipe_protocol = "";
    string pipe_options = model.substr(5);
    string::size_type begin, end;
    begin = 0;
    // pipe file path
    end = pipe_options.find( ",", begin );
    if ( end != string::npos ) {
      pipe_path = pipe_options.substr(begin, end - begin);
      begin = end + 1;
    }
    // protocol (last option)
    pipe_protocol = pipe_options.substr(begin);
    _impl = new FGExternalPipe( dt, pipe_path, pipe_protocol );
  } else if ( model == "null" ) {
    _impl = new FGNullFDM( dt );
  } 
#ifdef ENABLE_LARCSIM
    else if ( model == "larcsim" ) {
        _impl = new FGLaRCsim( dt );
    } 
#endif
#ifdef ENABLE_JSBSIM
    else if ( model == "jsb" ) {
        _impl = new FGJSBsim( dt );
    } 
#endif
#ifdef ENABLE_SP_FDM
    else if ( model == "ada" ) {
        _impl = new FGADA( dt );
    } else if ( model == "acms" ) {
        _impl = new FGACMS( dt );
    } else if ( model == "balloon" ) {
        _impl = new FGBalloonSim( dt );
    } else if ( model == "magic" ) {
        _impl = new FGMagicCarpet( dt );
    }
#endif
#ifdef ENABLE_YASIM
    else if ( model == "yasim" ) {
        _impl = new YASim( dt );
    } 
#endif
    else {
        throw sg_exception(string("Unrecognized flight model '") + model
               + "', cannot init flight dynamics model.");
    }

}
