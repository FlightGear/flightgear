// pidcontroller.hxx - implementation of PID controller
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
#ifndef __PIDCONTROLLER_HXX
#define __PIDCONTROLLER_HXX 1

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "analogcomponent.hxx"

#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>

namespace FGXMLAutopilot {

/**
 * Roy Ovesen's PID controller
 */
class PIDController : public AnalogComponent {

private:
    // Configuration values
    InputValueList Kp;          // proportional gain
    InputValueList Ti;          // Integrator time (sec)
    InputValueList Td;          // Derivator time (sec)

    double alpha;               // low pass filter weighing factor (usually 0.1)
    double beta;                // process value weighing factor for
                                // calculating proportional error
                                // (usually 1.0)
    double gamma;               // process value weighing factor for
                                // calculating derivative error
                                // (usually 0.0)

    // Previous state tracking values
    double ep_n_1;              // ep[n-1]  (prop error)
    double edf_n_1;             // edf[n-1] (derivative error)
    double edf_n_2;             // edf[n-2] (derivative error)
    double u_n_1;               // u[n-1]   (output)
    double desiredTs;            // desired sampling interval (sec)
    double elapsedTime;          // elapsed time (sec)

protected:
    bool configure( const std::string & nodeName, SGPropertyNode_ptr configNode);
public:
    PIDController();
    ~PIDController() {}

    void update( bool firstTime, double dt );
};

}
#endif // __PIDCONTROLLER_HXX
