/**************************************************************************
 * autopilot.hxx -- autopilot defines and prototypes (very alpha)
 *
 * Written by Jeff Goeke-Smith, started April 1998.
 *
 * Copyright (C) 1998 Jeff Goeke-Smith  - jgoeke@voyager.net
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * 
 **************************************************************************/
                       
                       
#ifndef _AUTOPILOT_H
#define _AUTOPILOT_H
                       

#include <Aircraft/aircraft.hxx>
#include <Flight/flight.hxx>
#include <Controls/controls.h>
                       
                       
#ifdef __cplusplus                                                          
extern "C" {                            
#endif                                   


// Structures
typedef struct {
    int heading_hold;   // the current state of the heading hold
    int altitude_hold;  // the current state of the altitude hold
    int terrain_follow; // the current state of the terrain follower
    int auto_throttle;  // the current state of the auto throttle

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


#ifdef __cplusplus
}
#endif


#endif // _AUTOPILOT_H
