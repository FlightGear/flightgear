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


AirspeedIndicator::AirspeedIndicator ()
{
}

AirspeedIndicator::~AirspeedIndicator ()
{
}

void
AirspeedIndicator::init ()
{
    _serviceable_node =
        fgGetNode("/instrumentation/airspeed-indicator/serviceable",
                  true);
    _total_pressure_node =
        fgGetNode("/systems/pitot/total-pressure-inhg", true);
    _static_pressure_node =
        fgGetNode("/systems/static/pressure-inhg", true);
    _speed_node =
        fgGetNode("/instrumentation/airspeed-indicator/indicated-speed-kt",
                  true);
}

#ifndef SEA_LEVEL_DENSITY_SLUGFG3
# define SEA_LEVEL_DENSITY_SLUGFT3 0.002378
#endif

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
        double pt = _total_pressure_node->getDoubleValue();
        double p = _static_pressure_node->getDoubleValue();
        double q = ( pt - p ) * INHGTOPSF;      // dynamic pressure

        // Now, reverse the equation (normalize dynamic pressure to
        // avoid "nan" results from sqrt)
        if ( q < 0 ) { q = 0.0; }
        double v_fps = sqrt((2 * q) / SEA_LEVEL_DENSITY_SLUGFT3);

                                // Publish the indicated airspeed
        double last_speed_kt = _speed_node->getDoubleValue();
        double current_speed_kt = v_fps * FPSTOKTS;
        _speed_node->setDoubleValue(fgGetLowPass(last_speed_kt,
                                                 current_speed_kt,
                                                 dt * RESPONSIVENESS));
    }
}

// end of airspeed_indicator.cxx
