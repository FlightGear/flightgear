// external.hxx -- the "external" flight model (driven from other
//                 external input)
//
// Written by Curtis Olson, started December 1998.
//
// Copyright (C) 1998  Curtis L. Olson  - curt@flightgear.org
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
//
// $Id$


#ifndef _EXTERNAL_HXX
#define _EXTERNAL_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#include <Time/fg_time.hxx>
#include <Time/timestamp.hxx>


/*
class fgFDM_EXTERNAL {

public:

    // Time Stamp

    // The time at which these values are correct (for extrapolating
    // later frames between position updates)
    FGTimeStamp t;

    // Positions

    // placement in geodetic coordinates
    double Latitude;
    double Longitude;
    double Altitude;

    // orientation in euler angles relative to local frame (or ground
    // position)
    double Phi;       // roll
    double Theta;     // pitch
    double Psi;       // heading

    // Velocities

    // velocities in geodetic coordinates
    double Latitude_dot;   // rad/sec
    double Longitude_dot;  // rad/sec
    double Altitude_dot;   // feet/sec

    // rotational rates
    double Phi_dot;
    double Theta_dot;
    double Psi_dot;
};
*/


// reset flight params to a specific position 
void fgExternalInit( FGInterface& f );


#endif // _EXTERNAL_HXX


