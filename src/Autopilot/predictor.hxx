// predictor.hxx - predict future values
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
#ifndef __PREDICTOR_HXX
#define __PREDICTOR_HXX 1

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "analogcomponent.hxx"

#include <simgear/props/props.hxx>

namespace FGXMLAutopilot {

/**
 * @brief Simple moving average filter converts input value to predicted value "seconds".
 *
 * Smoothing as described by Curt Olson:
 *   gain would be valid in the range of 0 - 1.0
 *   1.0 would mean no filtering.
 *   0.0 would mean no input.
 *   0.5 would mean (1 part past value + 1 part current value) / 2
 *   0.1 would mean (9 parts past value + 1 part current value) / 10
 *   0.25 would mean (3 parts past value + 1 part current value) / 4
 */
class Predictor : public AnalogComponent {

private:
    double _last_value;
    double _average;
    InputValueList _seconds;
    InputValueList _filter_gain;

protected:
  bool configure(const std::string& nodeName, SGPropertyNode_ptr configNode );

public:
    Predictor();
    ~Predictor() {}

    void update( bool firstTime, double dt );
};

} // namespace FGXMLAutopilot

#endif
