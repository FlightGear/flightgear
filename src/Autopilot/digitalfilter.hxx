// digitalfilter.hxx - a selection of digital filters
//
// Written by Torsten Dreyer
// Based heavily on work created by Curtis Olson, started January 2004.
//
// Copyright (C) 2004  Curtis L. Olson  - http://www.flightgear.org/~curt
// Copyright (C) 2010  Torsten Dreyer - Torsten (at) t3r (dot) de
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
#ifndef __DIGITALFILTER_HXX
#define __DIGITALFILTER_HXX 1

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "analogcomponent.hxx"

namespace FGXMLAutopilot {

/**
 * brief@ DigitalFilter - a selection of digital filters
 *
 */
class DigitalFilter : public AnalogComponent
{
private:
    SGSharedPtr<class DigitalFilterImplementation> _implementation;

    enum InitializeTo {
      INITIALIZE_OUTPUT,
      INITIALIZE_INPUT,
      INITIALIZE_NONE
    };

protected:
    bool configure( const std::string & nodeName, SGPropertyNode_ptr configNode);
    void update( bool firstTime, double dt);

    InputValueList _Tf;
    InputValueList _samples;
    InputValueList _rateOfChange;
    InputValueList _gain;
    InitializeTo _initializeTo;

public:
    DigitalFilter();
    ~DigitalFilter();

};

} // namespace FGXMLAutopilot
#endif
