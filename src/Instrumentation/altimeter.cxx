// altimeter.cxx - an altimeter tied to the static port.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain and comes with no warranty.

#include <simgear/math/interpolater.hxx>

#include "altimeter.hxx"
#include <Main/fg_props.hxx>
#include <Main/util.hxx>


// A higher number means more responsive
#define RESPONSIVENESS 10.0


// Altitude based on pressure difference from sea level.
// pressure difference inHG, altitude ft
static double altitude_data[][2] = {
 { -8.41, -8858.27 },
 { 0.00, 0.00 },
 { 3.05, 2952.76 },
 { 5.86, 5905.51 },
 { 8.41, 8858.27 },
 { 10.74, 11811.02 },
 { 12.87, 14763.78 },
 { 14.78, 17716.54 },
 { 16.55, 20669.29 },
 { 18.13, 23622.05 },
 { 19.62, 26574.80 },
 { 20.82, 29527.56 },
 { 21.96, 32480.31 },
 { 23.01, 35433.07 },
 { 23.91, 38385.83 },
 { 24.71, 41338.58 },
 { 25.40, 44291.34 },
 { 26.00, 47244.09 },
 { 26.51, 50196.85 },
 { 26.96, 53149.61 },
 { 27.35, 56102.36 },
 { 27.68, 59055.12 },
 { 27.98, 62007.87 },
 { 29.62, 100000.00 },            // just to fill it in
 { -1, -1 }
};


Altimeter::Altimeter ( SGPropertyNode *node )
    : _altitude_table(new SGInterpTable),
      name("altimeter"),
      num(0),
      static_port("/systems/static")
{
    int i;
    for (i = 0; altitude_data[i][0] != -1; i++)
        _altitude_table->addEntry(altitude_data[i][0], altitude_data[i][1]);

    for ( i = 0; i < node->nChildren(); ++i ) {
        SGPropertyNode *child = node->getChild(i);
        string cname = child->getName();
        string cval = child->getStringValue();
        if ( cname == "name" ) {
            name = cval;
        } else if ( cname == "number" ) {
            num = child->getIntValue();
        } else if ( cname == "static-port" ) {
            static_port = cval;
        } else {
            SG_LOG( SG_AUTOPILOT, SG_WARN, "Error in altimeter config logic" );
            if ( name.length() ) {
                SG_LOG( SG_AUTOPILOT, SG_WARN, "Section = " << name );
            }
        }
    }
}

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
    string branch;
    branch = "/instrumentation/" + name;
    static_port += "/pressure-inhg";

    SGPropertyNode *node = fgGetNode(branch.c_str(), num, true );

    _serviceable_node = node->getChild("serviceable", 0, true);
    _setting_node = node->getChild("setting-inhg", 0, true);
    _pressure_node = fgGetNode(static_port.c_str(), true);
    _altitude_node = node->getChild("indicated-altitude-ft", 0, true);

    _serviceable_node->setBoolValue(true);
    _setting_node->setDoubleValue(29.92);
}

void
Altimeter::update (double dt)
{
    if (_serviceable_node->getBoolValue()) {
        double pressure = _pressure_node->getDoubleValue();
        double setting = _setting_node->getDoubleValue();

                                // Move towards the current setting
        double last_altitude = _altitude_node->getDoubleValue();
        double current_altitude =
            _altitude_table->interpolate(setting - pressure);
        _altitude_node->setDoubleValue(fgGetLowPass(last_altitude,
                                                    current_altitude,
                                                    dt * RESPONSIVENESS));
    }
}

// end of altimeter.cxx
