// altimeter.cxx - an altimeter tied to the static port.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain and comes with no warranty.

#include <simgear/math/interpolater.hxx>

#include "altimeter.hxx"
#include <Main/fg_props.hxx>


// Altitude based on pressure difference from sea level.
// pressure difference inHG, altitude ft
static double altitude_data[][2] = {
    -8.41, -8858.27,
    0.00, 0.00,
    3.05, 2952.76,
    5.86, 5905.51,
    8.41, 8858.27,
    10.74, 11811.02,
    12.87, 14763.78,
    14.78, 17716.54,
    16.55, 20669.29,
    18.13, 23622.05,
    19.62, 26574.80,
    20.82, 29527.56,
    21.96, 32480.31,
    23.01, 35433.07,
    23.91, 38385.83,
    24.71, 41338.58,
    25.40, 44291.34,
    26.00, 47244.09,
    26.51, 50196.85,
    26.96, 53149.61,
    27.35, 56102.36,
    27.68, 59055.12,
    27.98, 62007.87,
    29.62, 100000.00            // just to fill it in
    -1, -1,
};


Altimeter::Altimeter ()
    : _altitude_table(new SGInterpTable)
{

    for (int i = 0; altitude_data[i][0] != -1; i++)
        _altitude_table->addEntry(altitude_data[i][0], altitude_data[i][1]);
}

Altimeter::~Altimeter ()
{
    delete _altitude_table;
}

void
Altimeter::init ()
{
    _serviceable_node =
        fgGetNode("/instrumentation/altimeter/serviceable", true);
    _setting_node =
        fgGetNode("/instrumentation/altimeter/setting-inhg", true);
    _pressure_node =
        fgGetNode("/systems/static/pressure-inhg", true);
    _altitude_node =
        fgGetNode("/instrumentation/altimeter/indicated-altitude-ft", true);
}

void
Altimeter::bind ()
{
}

void
Altimeter::unbind ()
{
}

void
Altimeter::update (double dt)
{
    if (_serviceable_node->getBoolValue()) {
        double pressure = _pressure_node->getDoubleValue();
        double setting = _setting_node->getDoubleValue();
        _altitude_node
            ->setDoubleValue(_altitude_table->interpolate(setting-pressure));
    }
}

// end of altimeter.cxx
