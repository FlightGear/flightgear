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


#include <simgear/misc/props.hxx>
#include <simgear/route/waypoint.hxx>

#include <Main/fgfs.hxx>
#include <Main/fg_props.hxx>

// Structures
class FGAutopilot : public FGSubsystem
{

public:

    enum fgAutoHeadingMode {
        FG_DG_HEADING_LOCK = 0,   // follow bug on directional gryo (vacuum)
        FG_TC_HEADING_LOCK = 1,   // keep turn coordinator zero'd
        FG_TRUE_HEADING_LOCK = 2, // lock to true heading (i.e. a perfect world)
        FG_HEADING_NAV1 = 3,      // follow nav1 radial
        FG_HEADING_NAV2 = 4,      // follow nav2 radial
        FG_HEADING_WAYPOINT = 5   // track next waypoint
    };

    enum fgAutoAltitudeMode {
        FG_ALTITUDE_LOCK = 0,     // lock to a specific altitude (indicated)
        FG_ALTITUDE_TERRAIN = 1,  // try to maintain a specific AGL
        FG_ALTITUDE_GS1 = 2,      // follow glide slope 1
        FG_ALTITUDE_GS2 = 3,      // follow glide slope 2
        FG_ALTITUDE_ARM = 4,      // ascend to selected altitude
        FG_TRUE_ALTITUDE_LOCK = 5 // lock to a specific true altitude
                                  // (i.e. a perfect world)
    };


private:

    bool heading_hold;          // the current state of the heading hold
    bool altitude_hold;         // the current state of the altitude hold
    bool auto_throttle;         // the current state of the auto throttle

    fgAutoHeadingMode heading_mode;
    fgAutoAltitudeMode altitude_mode;

    SGWayPoint waypoint;        // the waypoint the AP should steer to.

    // double TargetLatitude;   // the latitude the AP should steer to.
    // double TargetLongitude;  // the longitude the AP should steer to.
    double TargetDistance;      // the distance to Target.
    double DGTargetHeading;     // the apparent DG heading to steer towards.
    double TargetHeading;       // the true heading the AP should steer to.
    double TargetAltitude;      // altitude to hold
    double TargetAGL;           // the terrain separation

    double TargetSpeed;         // speed to shoot for
    double alt_error_accum;     // altitude error accumulator
    double climb_error_accum;   // climb error accumulator (for GS)
    double speed_error_accum;   // speed error accumulator

    double TargetSlope;         // the glide slope hold value

    double MaxRoll ;            // the max the plane can roll for the turn
    double RollOut;             // when the plane should roll out
                                // measured from Heading
    double MaxAileron;          // how far to move the aleroin from center
    double RollOutSmooth;       // deg to use for smoothing Aileron Control
    double MaxElevator;         // the maximum elevator allowed
    double SlopeSmooth;         // smoothing angle for elevator

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
    char TargetWP1Str[64];
    char TargetWP2Str[64];
    char TargetWP3Str[64];
    char TargetHeadingStr[64];
    char TargetAltitudeStr[64];

    // property nodes
    SGPropertyNode *latitude_node;
    SGPropertyNode *longitude_node;
    SGPropertyNode *altitude_node;
    SGPropertyNode *altitude_agl_node;
    SGPropertyNode *vertical_speed_node;
    SGPropertyNode *heading_node;
    SGPropertyNode *dg_heading_node;
    SGPropertyNode *roll_node;
    SGPropertyNode *pitch_node;

    SGPropertyNode *min_climb;           // minimum climb speed
    SGPropertyNode *best_climb;          // best climb speed
    SGPropertyNode *elevator_adj_factor; // factor to optimize altitude hold adjustments
    SGPropertyNode *integral_contrib;    // amount of contribution of the integral
                                // component of the pid
    SGPropertyNode *zero_pitch_throttle; // amount of throttle at which the aircraft does not pitch up
    SGPropertyNode *zero_pitch_trim_full_throttle; // amount of trim required to level at full throttle
    SGPropertyNode *max_aileron_node; // maximum aileron setting range -1 ~ 1
    SGPropertyNode *max_roll_node; // maximum roll setting in degrees
    SGPropertyNode *roll_out_node; // start rollout offset from desired heading in degrees
    SGPropertyNode *roll_out_smooth_node; // rollout smoothing offset in degrees

    SGPropertyNode *TargetClimbRate;    // target climb rate
    SGPropertyNode *TargetDescentRate;  // target decent rate
    SGPropertyNode *current_throttle; // current throttle (engine 0)

public:

    // constructor
    FGAutopilot();

    // destructor
    ~FGAutopilot();


    ////////////////////////////////////////////////////////////////////
    // Implementation of FGSubsystem.
    ////////////////////////////////////////////////////////////////////

    void init ();
    void bind ();
    void unbind ();
    void update (double dt);

    // Reset the autopilot system
    void reset(void);

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

    /* inline void set_WayPoint( const double lon, const double lat, 
                              const double alt, const string s ) {
        waypoint = SGWayPoint( lon, lat, alt, SGWayPoint::WGS84, "Current WP" );
    } */
    inline double get_TargetLatitude() const {
        return waypoint.get_target_lat();
    }
    inline double get_TargetLongitude() const {
        return waypoint.get_target_lon();
    }
    inline void set_old_lat( double val ) { old_lat = val; }
    inline void set_old_lon( double val ) { old_lon = val; }
    inline double get_TargetHeading() const { return TargetHeading; }
    inline void set_TargetHeading( double val ) { TargetHeading = val; }
    inline double get_DGTargetHeading() const { return DGTargetHeading; }
    inline void set_DGTargetHeading( double val ) { DGTargetHeading = val; }
    inline double get_TargetDistance() const { return TargetDistance; }
    inline void set_TargetDistance( double val ) { TargetDistance = val; }
    inline double get_TargetAltitude() const { return TargetAltitude; }
    inline void set_TargetAltitude( double val ) { TargetAltitude = val; }
    inline double get_TargetAGL() const { return TargetAGL; }
    inline void set_TargetAGL( double val ) { TargetAGL = val; }
    inline double get_TargetClimbRate() const {
        return TargetClimbRate->getFloatValue();
    }
    inline void set_TargetClimbRate( double val ) {
        fgSetFloat( "/autopilot/config/target-climb-rate-fpm",  val);
    }

    inline char *get_TargetLatitudeStr() { return TargetLatitudeStr; }
    inline char *get_TargetLongitudeStr() { return TargetLongitudeStr; }
    inline char *get_TargetWP1Str() { return TargetWP1Str; }
    inline char *get_TargetWP2Str() { return TargetWP2Str; }
    inline char *get_TargetWP3Str() { return TargetWP3Str; }
    inline char *get_TargetHeadingStr() { return TargetHeadingStr; }
    inline char *get_TargetAltitudeStr() { return TargetAltitudeStr; }
    inline char *get_TargetLatLonStr() { return TargetLatLonStr; }

    // utility functions
    void MakeTargetLatLonStr( double lat, double lon );
    void MakeTargetAltitudeStr( double altitude );
    void MakeTargetHeadingStr( double bearing );
    void MakeTargetWPStr( double distance );
    void update_old_control_values();

    // accessors
    inline double get_MaxRoll() {
        return fgGetFloat( "/autopilot/config/max-roll-deg" );
    }
    inline double get_RollOut() {
        return fgGetFloat( "/autopilot/config/roll-out-deg" );
    }
    inline double get_MaxAileron() {
        return fgGetFloat( "/autopilot/config/max-aileron" );
    }
    inline double get_RollOutSmooth() {
        return fgGetFloat( "/autopilot/config/roll-out-smooth-deg" );
    }
    inline void set_MaxRoll( double val ) {
        fgSetFloat( "/autopilot/config/max-roll-deg", val );
    }
    inline void set_RollOut( double val ) {
        fgSetFloat( "/autopilot/config/roll-out-deg", val );
    }
    inline void set_MaxAileron( double val ) {
        fgSetFloat( "/autopilot/config/max-aileron", val );
    }
    inline void set_RollOutSmooth( double val ) {
        fgSetFloat( "/autopilot/config/roll-out-smooth-deg", val );
    }

private:

    bool getAPAltitudeLock () const;
    void setAPAltitudeLock (bool lock);
    double getAPAltitude () const;
    void setAPAltitude (double altitude);
    bool getAPGSLock () const;
    void setAPGSLock (bool lock);
    bool getAPTerrainLock () const;
    void setAPTerrainLock (bool lock);
    double getAPClimb () const;
    void setAPClimb (double rate);
    bool getAPHeadingLock () const;
    void setAPHeadingLock (bool lock);
    double getAPHeadingBug () const;
    void setAPHeadingBug (double heading);
    bool getAPWingLeveler () const;
    void setAPWingLeveler (bool lock);
    bool getAPNAV1Lock () const;
    void setAPNAV1Lock (bool lock);
    bool getAPAutoThrottleLock () const;
    void setAPAutoThrottleLock (bool lock);
    double getAPRudderControl () const;
    void setAPRudderControl (double value);
    double getAPElevatorControl () const;
    void setAPElevatorControl (double value);
    double getAPThrottleControl () const;
    void setAPThrottleControl (double value);

};


#define DEFAULT_AP_HEADING_LOCK FGAutopilot::FG_DG_HEADING_LOCK
#define DEFAULT_AP_ALTITUDE_LOCK FGAutopilot::FG_ALTITUDE_LOCK
//#define DEFAULT_AP_HEADING_LOCK FGAutopilot::FG_TRUE_HEADING_LOCK
//#define DEFAULT_AP_ALTITUDE_LOCK FGAutopilot::FG_TRUE_ALTITUDE_LOCK

/**
 * static functions for autopilot properties
 */

#endif // _NEWAUTO_HXX

