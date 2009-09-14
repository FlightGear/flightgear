// autopilot.hxx -- autopilot defines and prototypes (very alpha)
//
// Written by Jeff Goeke-Smith, started April 1998.
//
// Copyright (C) 1998 Jeff Goeke-Smith  - jgoeke@voyager.net
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
                       
                       
#ifndef _AUTOPILOT_HXX
#define _AUTOPILOT_HXX
                       

#include <Aircraft/aircraft.hxx>
#include <FDM/flight.hxx>
#include <Controls/controls.hxx>
                       
                       
// Structures
typedef struct {
    bool heading_hold;   // the current state of the heading hold
    bool altitude_hold;  // the current state of the altitude hold
    bool terrain_follow; // the current state of the terrain follower
    bool auto_throttle;  // the current state of the auto throttle

    double TargetHeading;     // the heading the AP should steer to.
    double TargetAltitude;    // altitude to hold
    double TargetAGL;         // the terrain separation
    double TargetClimbRate;   // climb rate to shoot for
    double TargetSpeed;       // speed to shoot for
    double alt_error_accum;   // altitude error accumulator
    double speed_error_accum; // speed error accumulator

    double TargetSlope; // the glide slope hold value
    
    double MaxRoll ; // the max the plane can roll for the turn
    double RollOut;  // when the plane should roll out
    // measured from Heading
    double MaxAileron; // how far to move the aleroin from center
    double RollOutSmooth; // deg to use for smoothing Aileron Control
    double MaxElevator; // the maximum elevator allowed
    double SlopeSmooth; // smoothing angle for elevator
    
} fgAPData, *fgAPDataPtr ;
		

// Defines
#define AP_CURRENT_HEADING -1


// prototypes
void fgAPInit( fgAIRCRAFT *current_aircraft );
int fgAPRun( void );
void fgAPToggleHeading( void );
void fgAPToggleAltitude( void );
void fgAPToggleTerrainFollow( void );
void fgAPToggleAutoThrottle( void );

bool fgAPAltitudeEnabled( void );
bool fgAPHeadingEnabled( void );
bool fgAPAutoThrottleEnabled( void );
void fgAPAltitudeAdjust( double inc );
void fgAPHeadingAdjust( double inc );
void fgAPAutoThrottleAdjust( double inc );


#endif // _AUTOPILOT_HXX


// $Log$
// Revision 1.9  1999/02/12 23:22:36  curt
// Allow auto-throttle adjustment while active.
//
// Revision 1.8  1999/02/12 22:17:15  curt
// Changes contributed by Norman Vine to allow adjustment of the autopilot
// while it is activated.
//
