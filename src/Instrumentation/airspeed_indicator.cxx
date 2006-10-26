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
    name(node->getStringValue("name", "airspeed-indicator")),
    num(node->getIntValue("number", 0)),
    pitot_port(node->getStringValue("pitot-port", "/systems/pitot")),
    static_port(node->getStringValue("static-port", "/systems/static"))
{
}

AirspeedIndicator::~AirspeedIndicator ()
{
}

void
AirspeedIndicator::init ()
{
    string branch;
    branch = "/instrumentation/" + name;
    pitot_port += "/total-pressure-inhg";
    static_port += "/pressure-inhg";

    SGPropertyNode *node = fgGetNode(branch.c_str(), num, true );
    _serviceable_node = node->getChild("serviceable", 0, true);
    _total_pressure_node = fgGetNode(pitot_port.c_str(), true);
    _static_pressure_node = fgGetNode(static_port.c_str(), true);
    _density_node = fgGetNode("/environment/density-slugft3", true);
    _speed_node = node->getChild("indicated-speed-kt", 0, true);
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
    if (_serviceable_node->getBoolValue()) {
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
        _speed_node->setDoubleValue(fgGetLowPass(last_speed_kt,
                                                 current_speed_kt,
                                                 dt * RESPONSIVENESS));
    }
}

// end of airspeed_indicator.cxx
