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
                       
                       
#ifndef _AUTOPILOT_HXX
#define _AUTOPILOT_HXX

#include <Include/compiler.h>
                       
#include STL_STRING

#include <string.h>

#include <Aircraft/aircraft.hxx>
#include <FDM/flight.hxx>
#include <Controls/controls.hxx>
                       
FG_USING_STD(string);

                  
// Structures
typedef struct {
    bool waypoint_hold;       // the current state of the target hold
    bool heading_hold;        // the current state of the heading hold
    bool altitude_hold;       // the current state of the altitude hold
    bool terrain_follow;      // the current state of the terrain follower
    bool auto_throttle;       // the current state of the auto throttle

    double TargetLatitude;    // the latitude the AP should steer to.
    double TargetLongitude;   // the longitude the AP should steer to.
    double TargetDistance;    // the distance to Target.
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
	
	// following for testing disengagement of autopilot
	// apon pilot interaction with controls
    double old_aileron;
    double old_elevator;
    double old_elevator_trim;
    double old_rudder;
	
    // manual controls override beyond this value
    double disengage_threshold; 

    // For future cross track error adjust
    double old_lat;
    double old_lon;

    // keeping these locally to save work inside main loop
    char TargetLatitudeStr[64];
    char TargetLongitudeStr[64];
    char TargetLatLonStr[64];
    char TargetDistanceStr[64];
    char TargetHeadingStr[64];
    char TargetAltitudeStr[64];
    //	char jnk[32];
    // using current_options.airport_id for now
    //	string tgt_airport_id;  // ID of initial starting airport    
} fgAPData, *fgAPDataPtr ;
		

// Defines
#define AP_CURRENT_HEADING -1


// prototypes
void fgAPInit( fgAIRCRAFT *current_aircraft );
int fgAPRun( void );

void fgAPToggleWayPoint( void );
void fgAPToggleHeading( void );
void fgAPToggleAltitude( void );
void fgAPToggleTerrainFollow( void );
void fgAPToggleAutoThrottle( void );

bool fgAPTerrainFollowEnabled( void );
bool fgAPAltitudeEnabled( void );
bool fgAPHeadingEnabled( void );
bool fgAPWayPointEnabled( void );
bool fgAPAutoThrottleEnabled( void );

void fgAPAltitudeAdjust( double inc );
void fgAPHeadingAdjust( double inc );
void fgAPAutoThrottleAdjust( double inc );

void fgAPHeadingSet( double value );

double fgAPget_TargetLatitude( void );
double fgAPget_TargetLongitude( void );
double fgAPget_TargetHeading( void );
double fgAPget_TargetDistance( void );
double fgAPget_TargetAltitude( void );

char *fgAPget_TargetLatitudeStr( void );
char *fgAPget_TargetLongitudeStr( void );
char *fgAPget_TargetDistanceStr( void );
char *fgAPget_TargetHeadingStr( void );
char *fgAPget_TargetAltitudeStr( void );
char *fgAPget_TargetLatLonStr( void );

//void fgAPset_tgt_airport_id( const string );
//string fgAPget_tgt_airport_id( void );

void fgAPReset(void);

int geo_inverse_wgs_84( double alt,
						double lat1, double lon1,
						double lat2, double lon2,
						double *az1, double *az2,
						double *s );

int geo_direct_wgs_84(  double alt,
						double lat1, double lon1,
						double az1, double s,
						double *lat2, double *lon2,
						double *az2 );

class puObject;
void fgAPAdjust( puObject * );

#endif // _AUTOPILOT_HXX
