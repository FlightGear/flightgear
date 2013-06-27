// airspeed_indicator.cxx - a regular pitot-static airspeed indicator.
// Written by David Megginson, started 2002.
// Last modified by Eric van den Berg, 09 Dec 2012
//
// This file is in the Public Domain and comes with no warranty.

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <math.h>

#include <simgear/constants.h>
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
    std::string branch;
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

void
AirspeedIndicator::reinit ()
{
    _speed_node->setDoubleValue(0.0);
}

void
AirspeedIndicator::update (double dt)
{
    if (!_serviceable_node->getBoolValue()) {
        return;
    }
    
    double pt = _total_pressure_node->getDoubleValue() ;
    double p = _static_pressure_node->getDoubleValue() ;
    double qc = ( pt - p ) * SG_INHG_TO_PA ;  // Impact pressure in Pa, _not_ to be confused with dynamic pressure!!!

    // Now, reverse the equation (normalize impact pressure to
    // avoid "nan" results from sqrt)
    qc = std::max( qc , 0.0 );
    // Calibrated airspeed (using compressible aerodynamics) based on impact pressure qc in m/s
    // Using calibrated airspeed as indicated airspeed, neglecting any airspeed indicator errors.
    double v_cal = sqrt( 7 * SG_p0_Pa/SG_rho0_kg_p_m3 * ( pow( 1 + qc/SG_p0_Pa  , 1/3.5 )  -1 ) );

                            // Publish the indicated airspeed
    double last_speed_kt = _speed_node->getDoubleValue();
    double current_speed_kt = v_cal * SG_MPS_TO_KT;
    double filtered_speed = fgGetLowPass(last_speed_kt,
                                             current_speed_kt,
                                             dt * RESPONSIVENESS);
    _speed_node->setDoubleValue(filtered_speed);
    computeMach();

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
AirspeedIndicator::computeMach()
{
  if (!_environmentManager) {
    return;
  }
  
  FGEnvironment env(_environmentManager->getEnvironment());
  
  double oatK = env.get_temperature_degc() + SG_T0_K - 15.0 ;         // OAT in Kelvin
  oatK = std::max( oatK , 0.001 );                                // should never happen, but just in case someone flies into space...
  double c = sqrt(SG_gamma * SG_R_m2_p_s2_p_K * oatK);                 // speed-of-sound in m/s at aircraft position
  double pt = _total_pressure_node->getDoubleValue() * SG_INHG_TO_PA;  // total pressure in Pa
  double p = _static_pressure_node->getDoubleValue() * SG_INHG_TO_PA;  // static pressure in Pa
  p = std::max( p , 0.001 );                                           // should never happen, but just in case someone flies into space...
  double rho = _density_node->getDoubleValue() * SG_SLUGFT3_TO_KGPM3;  // air density in kg/m3
  rho = std::max( rho , 0.001 );                                      // should never happen, but just in case someone flies into space...
  
  // true airspeed in m/s
  pt = std::max( pt , p );
  double V_true = sqrt( 7 * p/rho * (pow( 1 + (pt-p)/p , 1/3.5 ) -1 ) );
  // Mach number; _see notes in systems/pitot.cxx_
  double mach = V_true / c;
  
  // publish Mach and TAS
  _mach_node->setDoubleValue(mach);
  _tas_node->setDoubleValue(V_true * SG_MPS_TO_KT );
}

// end of airspeed_indicator.cxx
