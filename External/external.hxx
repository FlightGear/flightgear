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
// (Log is kept at end of this file)


#ifndef _EXTERNAL_HXX
#define _EXTERNAL_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#include <Time/fg_time.hxx>


class fgFDM_EXTERNAL {

public:

    // Time Stamp

    // The time at which these values are correct (for extrapolating
    // later frames between position updates)
    fgTIMESTAMP t;

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

    // velocity relative to the local aircraft's coordinate system
    double V_north;
    double V_east;
    double V_down;

    // rotational rates relative to the coordinate system the body lives in
    double P_body;
    double Q_body;
    double R_body;

    // Accelerations
    
    // acceleration relative to the local aircraft's coordinate system
    double V_dot_north;
    double V_dot_east;
    double V_dot_down;

    // rotational acceleration relative to the coordinate system the
    // body lives in
    double P_dot_body;
    double Q_dot_body;
    double R_dot_body;
};


// reset flight params to a specific position 
void fgExternalInit(fgFLIGHT& f, double dt);

// update position based on inputs, positions, velocities, etc.
void fgExternalUpdate( fgFLIGHT& f, int multiloop );


#endif // _EXTERNAL_HXX


// $Log$
// Revision 1.1  1998/12/04 01:28:49  curt
// Initial revision.
//
