// newauto.hxx -- autopilot defines and prototypes (very alpha)
// 
// Started April 1998  Copyright (C) 1998
//
// Contributions by Jeff Goeke-Smith <jgoeke@voyager.net>
//                  Norman Vine <nhv@cape.com>
//                  Curtis Olson <curt@flightgear.org>
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
                       
                       
#ifndef _NEWAUTO_HXX
#define _NEWAUTO_HXX


// Structures
class FGAutopilot {

public:

    enum fgAutoHeadingMode {
	FG_HEADING_LOCK = 0,
	FG_HEADING_NAV1 = 1,
	FG_HEADING_NAV2 = 2,
        FG_HEADING_WAYPOINT = 3
    };

    enum fgAutoAltitudeMode {
	FG_ALTITUDE_LOCK = 0,
	FG_ALTITUDE_TERRAIN = 1,
	FG_ALTITUDE_GS1 = 2,
	FG_ALTITUDE_GS2 = 3
    };

private:

    bool heading_hold;		// the current state of the heading hold
    bool altitude_hold;		// the current state of the altitude hold
    bool auto_throttle;		// the current state of the auto throttle

    fgAutoHeadingMode heading_mode;
    fgAutoAltitudeMode altitude_mode;

    double TargetLatitude;	// the latitude the AP should steer to.
    double TargetLongitude;	// the longitude the AP should steer to.
    double TargetDistance;	// the distance to Target.
    double TargetHeading;	// the heading the AP should steer to.
    double TargetAltitude;	// altitude to hold
    double TargetAGL;		// the terrain separation
    double TargetClimbRate;	// climb rate to shoot for
    double TargetSpeed;		// speed to shoot for
    double alt_error_accum;	// altitude error accumulator
    double climb_error_accum;	// climb error accumulator (for GS)
    double speed_error_accum;	// speed error accumulator

    double TargetSlope;		// the glide slope hold value
    
    double MaxRoll ;		// the max the plane can roll for the turn
    double RollOut;		// when the plane should roll out
				// measured from Heading
    double MaxAileron;		// how far to move the aleroin from center
    double RollOutSmooth;	// deg to use for smoothing Aileron Control
    double MaxElevator;		// the maximum elevator allowed
    double SlopeSmooth;		// smoothing angle for elevator
	
    // following for testing disengagement of autopilot upon pilot
    // interaction with controls
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

public:

    // Initialize autopilot system
    void init();

    // Reset the autopilot system
    void reset(void);

    // run an interation of the autopilot (updates control positions)
    int run();

    void AltitudeSet( double new_altitude );
    void AltitudeAdjust( double inc );
    void HeadingAdjust( double inc );
    void AutoThrottleAdjust( double inc );

    void HeadingSet( double value );

    inline bool get_HeadingEnabled() const { return heading_hold; }
    inline void set_HeadingEnabled( bool value ) { heading_hold = value; }
    inline fgAutoHeadingMode get_HeadingMode() const { return heading_mode; }
    void set_HeadingMode( fgAutoHeadingMode mode );

    inline bool get_AltitudeEnabled() const { return altitude_hold; }
    inline void set_AltitudeEnabled( bool value ) { altitude_hold = value; }
    inline fgAutoAltitudeMode get_AltitudeMode() const { return altitude_mode;}
    void set_AltitudeMode( fgAutoAltitudeMode mode );

    inline bool get_AutoThrottleEnabled() const { return auto_throttle; }
    void set_AutoThrottleEnabled( bool value );

    inline double get_TargetLatitude() const { return TargetLatitude; }
    inline void set_TargetLatitude( double val ) { TargetLatitude = val; }
    inline void set_old_lat( double val ) { old_lat = val; }
    inline double get_TargetLongitude() const { return TargetLongitude; }
    inline void set_TargetLongitude( double val ) { TargetLongitude = val; }
    inline void set_old_lon( double val ) { old_lon = val; }
    inline double get_TargetHeading() const { return TargetHeading; }
    inline void set_TargetHeading( double val ) { TargetHeading = val; }
    inline double get_TargetDistance() const { return TargetDistance; }
    inline void set_TargetDistance( double val ) { TargetDistance = val; }
    inline double get_TargetAltitude() const { return TargetAltitude; }
    inline void set_TargetAltitude( double val ) { TargetAltitude = val; }

    inline char *get_TargetLatitudeStr() { return TargetLatitudeStr; }
    inline char *get_TargetLongitudeStr() { return TargetLongitudeStr; }
    inline char *get_TargetDistanceStr() { return TargetDistanceStr; }
    inline char *get_TargetHeadingStr() { return TargetHeadingStr; }
    inline char *get_TargetAltitudeStr() { return TargetAltitudeStr; }
    inline char *get_TargetLatLonStr() { return TargetLatLonStr; }

    // utility functions
    void MakeTargetLatLonStr( double lat, double lon );
    void MakeTargetAltitudeStr( double altitude );
    void MakeTargetHeadingStr( double bearing );
    void MakeTargetDistanceStr( double distance );
    void update_old_control_values();

    // accessors
    inline double get_MaxRoll() const { return MaxRoll; }
    inline void set_MaxRoll( double val ) { MaxRoll = val; }
    inline double get_RollOut() const { return RollOut; }
    inline void set_RollOut( double val ) { RollOut = val; }

    inline double get_MaxAileron() const { return MaxAileron; }
    inline void set_MaxAileron( double val ) { MaxAileron = val; }
    inline double get_RollOutSmooth() const { return RollOutSmooth; }
    inline void set_RollOutSmooth( double val ) { RollOutSmooth = val; }

};


extern FGAutopilot *current_autopilot;


#endif // _NEWAUTO_HXX
