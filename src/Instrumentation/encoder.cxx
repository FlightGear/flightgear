// encoder.cxx -- class to impliment an altitude encoder
//
// Written by Roy Vegard Ovesen, started September 2004.
//
// Copyright (C) 2004  Roy Vegard Ovesen - rvovesen@tiscali.no
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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "encoder.hxx"


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

int round (double value, int nearest=1)
{
    return ((int) (value/nearest + 0.5)) * nearest;
}

Encoder::Encoder(SGPropertyNode *node)
    :
    altitudeTable(new SGInterpTable),
    name("encoder"),
    num(0),
    staticPort("/systems/static")
{
    for (int i = 0; altitude_data[i][0] != -1; i++)
        altitudeTable->addEntry(altitude_data[i][0], altitude_data[i][1]);

    int i;
    for ( i = 0; i < node->nChildren(); ++i ) {
        SGPropertyNode *child = node->getChild(i);
        string cname = child->getName();
        string cval = child->getStringValue();
        if ( cname == "name" ) {
            name = cval;
        } else if ( cname == "number" ) {
            num = child->getIntValue();
        } else if ( cname == "static-port" ) {
            staticPort = cval;
        } else {
            SG_LOG( SG_INSTR, SG_WARN, 
                    "Error in encoder config logic" );
            if ( name.length() ) {
                SG_LOG( SG_INSTR, SG_WARN, "Section = " << name );
            }
        }
    }
}


Encoder::~Encoder()
{
    delete altitudeTable;
}


void Encoder::init()
{
    string branch;
    branch = "/instrumentation/" + name;
    staticPort += "/pressure-inhg";

    SGPropertyNode *node = fgGetNode(branch.c_str(), num, true );
    // Inputs
    staticPressureNode = fgGetNode(staticPort.c_str(), true);
    busPowerNode = fgGetNode("/systems/electrical/outputs/encoder", true);
    serviceableNode = node->getChild("serviceable", 0, true);
    // Outputs
    pressureAltitudeNode = node->getChild("pressure-alt-ft", 0, true);
    modeCAltitudeNode = node->getChild("mode-c-alt-ft", 0, true);
    // Init
    serviceableNode->setBoolValue(true);
}


void Encoder::update(double dt)
{
    if (serviceableNode->getBoolValue())
    {
        double staticPressure = staticPressureNode->getDoubleValue();
        double pressureAltitude =
            altitudeTable->interpolate(29.92 - staticPressure);
        int pressureAlt = round(pressureAltitude, 10);
        int modeCAltitude = round(pressureAltitude, 100);

        pressureAltitudeNode->setIntValue(pressureAlt);
        modeCAltitudeNode->setIntValue(modeCAltitude);
    }
}
