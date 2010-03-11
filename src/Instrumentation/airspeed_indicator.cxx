// airspeed_indicator.cxx - a regular pitot-static airspeed indicator.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain and comes with no warranty.

#include <math.h>

#include <simgear/math/interpolater.hxx>

#include "airspeed_indicator.hxx"
#include <Main/fg_props.hxx>
#include <Main/util.hxx>


// A higher number means more responsive.
#define RESPONSIVENESS 50.0

AirspeedIndicator::AirspeedIndicator ( SGPropertyNode *node )
    :
    _name(node->getStringValue("name", "airspeed-indicator")),
    _num(node->getIntValue("number", 0)),
    _total_pressure(node->getStringValue("total-pressure", "/systems/pitot/total-pressure-inhg")),
    _static_pressure(node->getStringValue("static-pressure", "/systems/static/pressure-inhg")),
    _has_barber_pole(node->getBoolValue("has-barber-pole",false))
{
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
    
  // barber-pole properties
    _ias_limit_node = node->getNode("ias-limit",0, true);
    _mach_limit_node = node->getNode("mach-limit",0, true);
    _alt_threshold_node = node->getNode("alt-threshold",0, true);
    _airspeed_limit = node->getChild("airspeed-limit-kt", 0, true);
    
    string paSource = node->getStringValue("pressure-alt-source",
      "/instrumentation/altimeter/pressure-alt-ft");
    _pressure_alt = fgGetNode(paSource.c_str(), true);
    _mach = fgGetNode("/velocities/mach", true);
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

    if (!_has_barber_pole) {
        return;
    }
    
    double lmt = _ias_limit_node->getDoubleValue();
    if (_pressure_alt->getDoubleValue() > _alt_threshold_node->getDoubleValue()) {
        double mmo = _mach_limit_node->getDoubleValue();
        lmt = (filtered_speed/_mach->getDoubleValue())* mmo;
    }
    
    _airspeed_limit->setDoubleValue(lmt);
}

// end of airspeed_indicator.cxx
