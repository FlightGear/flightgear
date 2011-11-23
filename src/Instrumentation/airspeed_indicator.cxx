// airspeed_indicator.cxx - a regular pitot-static airspeed indicator.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain and comes with no warranty.

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <math.h>

#include <simgear/math/interpolater.hxx>

#include "airspeed_indicator.hxx"
#include <Main/fg_props.hxx>
#include <Main/util.hxx>
#include <Environment/environment_mgr.hxx>
#include <Environment/environment.hxx>


// A higher number means more responsive.
#define RESPONSIVENESS 50.0

AirspeedIndicator::AirspeedIndicator ( SGPropertyNode *node )
    :
    _name(node->getStringValue("name", "airspeed-indicator")),
    _num(node->getIntValue("number", 0)),
    _total_pressure(node->getStringValue("total-pressure", "/systems/pitot/total-pressure-inhg")),
    _static_pressure(node->getStringValue("static-pressure", "/systems/static/pressure-inhg")),
    _has_overspeed(node->getBoolValue("has-overspeed-indicator",false)),
    _pressure_alt_source(node->getStringValue("pressure-alt-source", "/instrumentation/altimeter/pressure-alt-ft")),
    _ias_limit(node->getDoubleValue("ias-limit", 248.0)),
    _mach_limit(node->getDoubleValue("mach-limit", 0.48)),
    _alt_threshold(node->getDoubleValue("alt-threshold", 13200))
{
  _environmentManager = NULL;
}

AirspeedIndicator::~AirspeedIndicator ()
{
}

void
AirspeedIndicator::init ()
{
    string branch;
    branch = "/instrumentation/" + _name;

    SGPropertyNode *node = fgGetNode(branch.c_str(), _num, true );
    _serviceable_node = node->getChild("serviceable", 0, true);
    _total_pressure_node = fgGetNode(_total_pressure.c_str(), true);
    _static_pressure_node = fgGetNode(_static_pressure.c_str(), true);
    _density_node = fgGetNode("/environment/density-slugft3", true);
    _speed_node = node->getChild("indicated-speed-kt", 0, true);
    _tas_node = node->getChild("true-speed-kt", 0, true);
    _mach_node = node->getChild("indicated-mach", 0, true);
    
  // overspeed-indicator properties
    if (_has_overspeed) {
        _ias_limit_node = node->getNode("ias-limit",0, true);
        _mach_limit_node = node->getNode("mach-limit",0, true);
        _alt_threshold_node = node->getNode("alt-threshold",0, true);
        
        if (!_ias_limit_node->hasValue()) {
          _ias_limit_node->setDoubleValue(_ias_limit);
        }

        if (!_mach_limit_node->hasValue()) {
          _mach_limit_node->setDoubleValue(_mach_limit);
        }

        if (!_alt_threshold_node->hasValue()) {
          _alt_threshold_node->setDoubleValue(_alt_threshold);
        }

        _airspeed_limit = node->getChild("airspeed-limit-kt", 0, true);
        _pressure_alt = fgGetNode(_pressure_alt_source.c_str(), true);
    }
    
    _environmentManager = (FGEnvironmentMgr*) globals->get_subsystem("environment");
}

#ifndef FPSTOKTS
# define FPSTOKTS 0.592484
#endif

#ifndef INHGTOPSF
# define INHGTOPSF (2116.217/29.9212)
#endif

void
AirspeedIndicator::update (double dt)
{
    if (!_serviceable_node->getBoolValue()) {
        return;
    }
    
    double pt = _total_pressure_node->getDoubleValue() * INHGTOPSF;
    double p = _static_pressure_node->getDoubleValue() * INHGTOPSF;
    double r = _density_node->getDoubleValue();
    double q = ( pt - p );  // dynamic pressure

    // Now, reverse the equation (normalize dynamic pressure to
    // avoid "nan" results from sqrt)
    if ( q < 0 ) { q = 0.0; }
    double v_fps = sqrt((2 * q) / r);

                            // Publish the indicated airspeed
    double last_speed_kt = _speed_node->getDoubleValue();
    double current_speed_kt = v_fps * FPSTOKTS;
    double filtered_speed = fgGetLowPass(last_speed_kt,
                                             current_speed_kt,
                                             dt * RESPONSIVENESS);
    _speed_node->setDoubleValue(filtered_speed);
    computeMach(filtered_speed);

    if (!_has_overspeed) {
        return;
    }
    
    double lmt = _ias_limit_node->getDoubleValue();
    if (_pressure_alt->getDoubleValue() > _alt_threshold_node->getDoubleValue()) {
        double mmo = _mach_limit_node->getDoubleValue();
        lmt = (filtered_speed/_mach_node->getDoubleValue())* mmo;
    }
    
    _airspeed_limit->setDoubleValue(lmt);
}

void
AirspeedIndicator::computeMach(double ias)
{
  if (!_environmentManager) {
    return;
  }
  
  FGEnvironment env(_environmentManager->getEnvironment());
  
  // derived from http://williams.best.vwh.net/avform.htm#Mach
  // names here are picked to be consistent with those formulae!

  double oatK = env.get_temperature_degc() + 273.15; // OAT in Kelvin
  double CS = 38.967854 * sqrt(oatK); // speed-of-sound in knots at altitude
  double CS_0 = 661.4786; // speed-of-sound in knots at sea-level
  double P_0 = env.get_pressure_sea_level_inhg();
  double P = _static_pressure_node->getDoubleValue();
  
  double DP = P_0 * (pow(1 + 0.2*pow(ias/CS_0, 2), 3.5) - 1);
  double mach = pow(5 * ( pow(DP/P + 1, 2.0/7.0) -1) , 0.5);
  
  // publish Mach and TAS
  _mach_node->setDoubleValue(mach);
  _tas_node->setDoubleValue(CS * mach);
}

// end of airspeed_indicator.cxx
