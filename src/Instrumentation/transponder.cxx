// transponder.cxx -- class to impliment a transponder
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

#include "transponder.hxx"

Transponder::Transponder(SGPropertyNode *node)
    :
    name("transponder"),
    num(0),
    encoder("/instrumentation/encoder")
{
    int i;
    for ( i = 0; i < node->nChildren(); ++i ) {
        SGPropertyNode *child = node->getChild(i);
        string cname = child->getName();
        string cval = child->getStringValue();
        if ( cname == "name" ) {
            name = cval;
        } else if ( cname == "number" ) {
            num = child->getIntValue();
        } else if ( cname == "encoder" ) {
            encoder = cval;
        } else {
            SG_LOG( SG_INSTR, SG_WARN, 
                    "Error in transponder config logic" );
            if ( name.length() ) {
                SG_LOG( SG_INSTR, SG_WARN, "Section = " << name );
            }
        }
    }
}


Transponder::~Transponder()
{
}


void Transponder::init()
{
    string branch;
    branch = "/instrumentation/" + name;
    encoder += "/mode-c-alt-ft";

    SGPropertyNode *node = fgGetNode(branch.c_str(), num, true );
    // Inputs
    pressureAltitudeNode = fgGetNode(encoder.c_str(), true);
    busPowerNode = fgGetNode("/systems/electrical/outputs/transponder", true);
    serviceableNode = node->getChild("serviceable", 0, true);
    // Outputs
    idCodeNode = node->getChild("id-code", 0, true);
    flightLevelNode = node->getChild("flight-level", 0, true);
}


void Transponder::update(double dt)
{
    if (serviceableNode->getBoolValue())
    {
        int idCode = idCodeNode->getIntValue();
	if (idCode < 0)
	    idCode = 0;
	int firstDigit  = idCode % 10;
	int secondDigit = (idCode/10) % 10;
	int thirdDigit  = (idCode/100) % 10;
	int fourthDigit = (idCode/1000) % 10;

	if (firstDigit-7 > 0)
	    idCode -= firstDigit-7;
	if (secondDigit-7 > 0)
	    idCode -= (secondDigit-7) * 10;
	if (thirdDigit-7 > 0)
	    idCode -= (thirdDigit-7) * 100;
	if (fourthDigit-7 > 0)
	    idCode -= (fourthDigit-7) * 1000;

        if (idCode > 7777)
            idCode = 7777;
        else if (idCode < 0)
            idCode = 0;

        idCodeNode->setIntValue(idCode);

        int pressureAltitude = pressureAltitudeNode->getIntValue();
        int flightLevel = pressureAltitude / 100;
        flightLevelNode->setIntValue(flightLevel);
    }
}
