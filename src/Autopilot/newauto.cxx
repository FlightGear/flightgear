// newauto.cxx -- autopilot defines and prototypes (very alpha)
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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>		// sprintf()
#include <string.h>		// strcmp()

#include <simgear/constants.h>
#include <simgear/sg_inlines.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/math/sg_random.h>
#include <simgear/route/route.hxx>

#include <Cockpit/steam.hxx>
#include <Cockpit/radiostack.hxx>
#include <Controls/controls.hxx>
#include <FDM/flight.hxx>
#include <Main/globals.hxx>
#include <Scenery/scenery.hxx>

#include "newauto.hxx"


/// These statics will eventually go into the class
/// they are just here while I am experimenting -- NHV :-)
// AutoPilot Gain Adjuster members
static double MaxRollAdjust;        // MaxRollAdjust       = 2 * APData->MaxRoll;
static double RollOutAdjust;        // RollOutAdjust       = 2 * APData->RollOut;
static double MaxAileronAdjust;     // MaxAileronAdjust    = 2 * APData->MaxAileron;
static double RollOutSmoothAdjust;  // RollOutSmoothAdjust = 2 * APData->RollOutSmooth;

static char NewTgtAirportId[16];
// static char NewTgtAirportLabel[] = "Enter New TgtAirport ID"; 

extern char *coord_format_lat(float);
extern char *coord_format_lon(float);
			

// constructor
FGAutopilot::FGAutopilot()
{
}

// destructor
FGAutopilot::~FGAutopilot() {}


void FGAutopilot::MakeTargetLatLonStr( double lat, double lon ) {
    sprintf( TargetLatitudeStr , "%s", coord_format_lat(get_TargetLatitude()));
    sprintf( TargetLongitudeStr, "%s", coord_format_lon(get_TargetLongitude()));
    sprintf( TargetLatLonStr, "%s  %s", TargetLatitudeStr, TargetLongitudeStr );
}


void FGAutopilot::MakeTargetAltitudeStr( double altitude ) {
    if ( altitude_mode == FG_ALTITUDE_TERRAIN ) {
	sprintf( TargetAltitudeStr, "APAltitude  %6.0f+", altitude );
    } else {
	sprintf( TargetAltitudeStr, "APAltitude  %6.0f", altitude );
    }
}


void FGAutopilot::MakeTargetHeadingStr( double bearing ) {
    if ( bearing < 0. ) {
	bearing += 360.;
    } else if (bearing > 360. ) {
	bearing -= 360.;
    }
    sprintf( TargetHeadingStr, "APHeading  %6.1f", bearing );
}


static inline double get_speed( void ) {
    return( cur_fdm_state->get_V_equiv_kts() );
}

static inline double get_ground_speed() {
    // starts in ft/s so we convert to kts
    static const SGPropertyNode * speedup_node = fgGetNode("/sim/speed-up");

    double ft_s = cur_fdm_state->get_V_ground_speed() 
        * speedup_node->getIntValue();
    double kts = ft_s * SG_FEET_TO_METER * 3600 * SG_METER_TO_NM;

    return kts;
}


void FGAutopilot::MakeTargetWPStr( double distance ) {
    static time_t last_time = 0;
    time_t current_time = time(NULL);
    if ( last_time == current_time ) {
	return;
    }

    last_time = current_time;

    double accum = 0.0;

    int size = globals->get_route()->size();

    // start by wiping the strings
    TargetWP1Str[0] = 0;
    TargetWP2Str[0] = 0;
    TargetWP3Str[0] = 0;

    // current route
    if ( size > 0 ) {
	SGWayPoint wp1 = globals->get_route()->get_waypoint( 0 );
	accum += distance;
	double eta = accum * SG_METER_TO_NM / get_ground_speed();
	if ( eta >= 100.0 ) { eta = 99.999; }
	int major, minor;
	if ( eta < (1.0/6.0) ) {
	    // within 10 minutes, bump up to min/secs
	    eta *= 60.0;
	}
	major = (int)eta;
	minor = (int)((eta - (int)eta) * 60.0);
	sprintf( TargetWP1Str, "%s %.2f NM  ETA %d:%02d",
		 wp1.get_id().c_str(),
		 accum*SG_METER_TO_NM, major, minor );
    }

    // next route
    if ( size > 1 ) {
	SGWayPoint wp2 = globals->get_route()->get_waypoint( 1 );
	accum += wp2.get_distance();
	
	double eta = accum * SG_METER_TO_NM / get_ground_speed();
	if ( eta >= 100.0 ) { eta = 99.999; }
	int major, minor;
	if ( eta < (1.0/6.0) ) {
	    // within 10 minutes, bump up to min/secs
	    eta *= 60.0;
	}
	major = (int)eta;
	minor = (int)((eta - (int)eta) * 60.0);
	sprintf( TargetWP2Str, "%s %.2f NM  ETA %d:%02d",
		 wp2.get_id().c_str(),
		 accum*SG_METER_TO_NM, major, minor );
    }

    // next route
    if ( size > 2 ) {
	for ( int i = 2; i < size; ++i ) {
	    accum += globals->get_route()->get_waypoint( i ).get_distance();
	}
	
	SGWayPoint wpn = globals->get_route()->get_waypoint( size - 1 );

	double eta = accum * SG_METER_TO_NM / get_ground_speed();
	if ( eta >= 100.0 ) { eta = 99.999; }
	int major, minor;
	if ( eta < (1.0/6.0) ) {
	    // within 10 minutes, bump up to min/secs
	    eta *= 60.0;
	}
	major = (int)eta;
	minor = (int)((eta - (int)eta) * 60.0);
	sprintf( TargetWP3Str, "%s %.2f NM  ETA %d:%02d",
		 wpn.get_id().c_str(),
		 accum*SG_METER_TO_NM, major, minor );
    }
}


void FGAutopilot::update_old_control_values() {
    old_aileron = globals->get_controls()->get_aileron();
    old_elevator = globals->get_controls()->get_elevator();
    old_elevator_trim = globals->get_controls()->get_elevator_trim();
    old_rudder = globals->get_controls()->get_rudder();
}


// Initialize autopilot subsystem

void FGAutopilot::init ()
{
    SG_LOG( SG_AUTOPILOT, SG_INFO, "Init AutoPilot Subsystem" );

    // bind data input property nodes...
    latitude_node = fgGetNode("/position/latitude-deg", true);
    longitude_node = fgGetNode("/position/longitude-deg", true);
    altitude_node = fgGetNode("/position/altitude-ft", true);
    altitude_agl_node = fgGetNode("/position/altitude-agl-ft", true);
    vertical_speed_node = fgGetNode("/velocities/vertical-speed-fps", true);
    heading_node = fgGetNode("/orientation/heading-deg", true);
    roll_node = fgGetNode("/orientation/roll-deg", true);
    pitch_node = fgGetNode("/orientation/pitch-deg", true);



    // bind config property nodes...
    TargetClimbRate
        = fgGetNode("/autopilot/config/target-climb-rate-fpm", true);
    TargetDescentRate
        = fgGetNode("/autopilot/config/target-descent-rate-fpm", true);
    min_climb = fgGetNode("/autopilot/config/min-climb-speed-kt", true);
    best_climb = fgGetNode("/autopilot/config/best-climb-speed-kt", true);
    elevator_adj_factor
        = fgGetNode("/autopilot/config/elevator-adj-factor", true);
    integral_contrib
        = fgGetNode("/autopilot/config/integral-contribution", true);
    zero_pitch_throttle
        = fgGetNode("/autopilot/config/zero-pitch-throttle", true);
    zero_pitch_trim_full_throttle
        = fgGetNode("/autopilot/config/zero-pitch-trim-full-throttle", true);
    max_aileron_node = fgGetNode("/autopilot/config/max-aileron", true);
    max_roll_node = fgGetNode("/autopilot/config/max-roll-deg", true);
    roll_out_node = fgGetNode("/autopilot/config/roll-out-deg", true);
    roll_out_smooth_node = fgGetNode("/autopilot/config/roll-out-smooth-deg", true);

    current_throttle = fgGetNode("/controls/throttle");

    // initialize config properties with defaults (in case config isn't there)
    if ( TargetClimbRate->getFloatValue() < 1 )
        fgSetFloat( "/autopilot/config/target-climb-rate-fpm", 500);
    if ( TargetDescentRate->getFloatValue() < 1 )
        fgSetFloat( "/autopilot/config/target-descent-rate-fpm", 1000 );
    if ( min_climb->getFloatValue() < 1)
        fgSetFloat( "/autopilot/config/min-climb-speed-kt", 70 );
    if (best_climb->getFloatValue() < 1)
        fgSetFloat( "/autopilot/config/best-climb-speed-kt", 120 );
    if (elevator_adj_factor->getFloatValue() < 1)
        fgSetFloat( "/autopilot/config/elevator-adj-factor", 5000 );
    if ( integral_contrib->getFloatValue() < 0.0000001 )
        fgSetFloat( "/autopilot/config/integral-contribution", 0.01 );
    if ( zero_pitch_throttle->getFloatValue() < 0.0000001 )
        fgSetFloat( "/autopilot/config/zero-pitch-throttle", 0.60 );
    if ( zero_pitch_trim_full_throttle->getFloatValue() < 0.0000001 )
        fgSetFloat( "/autopilot/config/zero-pitch-trim-full-throttle", 0.15 );
    if ( max_aileron_node->getFloatValue() < 0.0000001 )
        fgSetFloat( "/autopilot/config/max-aileron", 0.2 );
    if ( max_roll_node->getFloatValue() < 0.0000001 )
        fgSetFloat( "/autopilot/config/max-roll-deg", 20 );
    if ( roll_out_node->getFloatValue() < 0.0000001 )
        fgSetFloat( "/autopilot/config/roll-out-deg", 20 );
    if ( roll_out_smooth_node->getFloatValue() < 0.0000001 )
        fgSetFloat( "/autopilot/config/roll-out-smooth-deg", 10 );

    /* set defaults */
    heading_hold = false ;      // turn the heading hold off
    altitude_hold = false ;     // turn the altitude hold off
    auto_throttle = false ;	// turn the auto throttle off
    heading_mode = DEFAULT_AP_HEADING_LOCK;
    altitude_mode = DEFAULT_AP_ALTITUDE_LOCK;

    DGTargetHeading = fgGetDouble("/autopilot/settings/heading-bug-deg");
    TargetHeading = fgGetDouble("/autopilot/settings/heading-bug-deg");
    TargetAltitude = fgGetDouble("/autopilot/settings/altitude-ft") * SG_FEET_TO_METER;

    // Initialize target location to startup location
    old_lat = latitude_node->getDoubleValue();
    old_lon = longitude_node->getDoubleValue();
    // set_WayPoint( old_lon, old_lat, 0.0, "default" );

    MakeTargetLatLonStr( get_TargetLatitude(), get_TargetLongitude() );
	
    alt_error_accum = 0.0;
    climb_error_accum = 0.0;

    MakeTargetAltitudeStr( TargetAltitude );
    MakeTargetHeadingStr( TargetHeading );
	
    // These eventually need to be read from current_aircaft somehow.

    // the maximum roll, in Deg
    MaxRoll = 20;

    // the deg from heading to start rolling out at, in Deg
    RollOut = 20;

    // Smoothing distance for alerion control
    RollOutSmooth = 10;

    // Hardwired for now should be in options
    // 25% max control variablilty  0.5 / 2.0
    disengage_threshold = 1.0;

    // set default aileron max deflection
    MaxAileron = 0.5;

#if !defined( USING_SLIDER_CLASS )
    MaxRollAdjust = 2 * MaxRoll;
    RollOutAdjust = 2 * RollOut;
    //MaxAileronAdjust = 2 * MaxAileron;
    RollOutSmoothAdjust = 2 * RollOutSmooth;
#endif  // !defined( USING_SLIDER_CLASS )

    update_old_control_values();
};

void
FGAutopilot::bind ()
{
    // Autopilot control property get/set bindings
    fgTie("/autopilot/locks/altitude", this,
	  &FGAutopilot::getAPAltitudeLock, &FGAutopilot::setAPAltitudeLock);
    fgSetArchivable("/autopilot/locks/altitude");
    fgTie("/autopilot/settings/altitude-ft", this,
	  &FGAutopilot::getAPAltitude, &FGAutopilot::setAPAltitude);
    fgSetArchivable("/autopilot/settings/altitude-ft");
    fgTie("/autopilot/locks/glide-slope", this,
	  &FGAutopilot::getAPGSLock, &FGAutopilot::setAPGSLock);
    fgSetArchivable("/autopilot/locks/glide-slope");
    fgSetDouble("/autopilot/settings/altitude-ft", 3000.0f);
    fgTie("/autopilot/locks/terrain", this,
	  &FGAutopilot::getAPTerrainLock, &FGAutopilot::setAPTerrainLock);
    fgSetArchivable("/autopilot/locks/terrain");
    fgTie("/autopilot/settings/climb-rate-fpm", this,
	  &FGAutopilot::getAPClimb, &FGAutopilot::setAPClimb, false);
    fgSetArchivable("/autopilot/settings/climb-rate-fpm");
    fgTie("/autopilot/locks/heading", this,
	  &FGAutopilot::getAPHeadingLock, &FGAutopilot::setAPHeadingLock);
    fgSetArchivable("/autopilot/locks/heading");
    fgTie("/autopilot/settings/heading-bug-deg", this,
	  &FGAutopilot::getAPHeadingBug, &FGAutopilot::setAPHeadingBug);
    fgSetArchivable("/autopilot/settings/heading-bug-deg");
    fgSetDouble("/autopilot/settings/heading-bug-deg", 0.0f);
    fgTie("/autopilot/locks/wing-leveler", this,
	  &FGAutopilot::getAPWingLeveler, &FGAutopilot::setAPWingLeveler);
    fgSetArchivable("/autopilot/locks/wing-leveler");
    fgTie("/autopilot/locks/nav[0]", this,
	  &FGAutopilot::getAPNAV1Lock, &FGAutopilot::setAPNAV1Lock);
    fgSetArchivable("/autopilot/locks/nav[0]");
    fgTie("/autopilot/locks/auto-throttle", this,
	  &FGAutopilot::getAPAutoThrottleLock,
	  &FGAutopilot::setAPAutoThrottleLock);
    fgSetArchivable("/autopilot/locks/auto-throttle");
    fgTie("/autopilot/control-overrides/rudder", this,
	  &FGAutopilot::getAPRudderControl,
	  &FGAutopilot::setAPRudderControl);
    fgSetArchivable("/autopilot/control-overrides/rudder");
    fgTie("/autopilot/control-overrides/elevator", this,
	  &FGAutopilot::getAPElevatorControl,
	  &FGAutopilot::setAPElevatorControl);
    fgSetArchivable("/autopilot/control-overrides/elevator");
    fgTie("/autopilot/control-overrides/throttle", this,
	  &FGAutopilot::getAPThrottleControl,
	  &FGAutopilot::setAPThrottleControl);
    fgSetArchivable("/autopilot/control-overrides/throttle");
}

void
FGAutopilot::unbind ()
{
}

// Reset the autopilot system
void FGAutopilot::reset() {

    heading_hold = false ;      // turn the heading hold off
    altitude_hold = false ;     // turn the altitude hold off
    auto_throttle = false ;	// turn the auto throttle off
    heading_mode = DEFAULT_AP_HEADING_LOCK;

    // TargetHeading = 0.0;	// default direction, due north
    MakeTargetHeadingStr( TargetHeading );			
	
    // TargetAltitude = 3000;   // default altitude in meters
    MakeTargetAltitudeStr( TargetAltitude );
	
    alt_error_accum = 0.0;
    climb_error_accum = 0.0;
	
    update_old_control_values();

    sprintf( NewTgtAirportId, "%s", fgGetString("/sim/startup/airport-id") );
	
    MakeTargetLatLonStr( get_TargetLatitude(), get_TargetLongitude() );
}


static double NormalizeDegrees( double Input ) {
    // normalize the input to the range (-180,180]
    // Input should not be greater than -360 to 360.
    // Current rules send the output to an undefined state.
    while ( Input > 180.0 ) { Input -= 360.0; }
    while ( Input <= -180.0 ) { Input += 360.0; }

    return Input;
};

static double LinearExtrapolate( double x, double x1, double y1, double x2, double y2 ) {
    // This procedure extrapolates the y value for the x posistion on a line defined by x1,y1; x2,y2
    //assert(x1 != x2); // Divide by zero error.  Cold abort for now

	// Could be
	// static double y = 0.0;
	// double dx = x2 -x1;
	// if( (dx < -SG_EPSILON ) || ( dx > SG_EPSILON ) )
	// {

    double m, b, y;          // the constants to find in y=mx+b
    // double m, b;

    m = ( y2 - y1 ) / ( x2 - x1 );   // calculate the m

    b = y1 - m * x1;       // calculate the b

    y = m * x + b;       // the final calculation

    // }

    return ( y );

};


void
FGAutopilot::update (double dt)
{
    // Remove the following lines when the calling funcitons start
    // passing in the data pointer

    // get control settings 
	
    double lat = latitude_node->getDoubleValue();
    double lon = longitude_node->getDoubleValue();
    double alt = altitude_node->getDoubleValue() * SG_FEET_TO_METER;

    // get config settings
    MaxAileron = max_aileron_node->getDoubleValue();
    MaxRoll = max_roll_node->getDoubleValue();
    RollOut = roll_out_node->getDoubleValue();
    RollOutSmooth = roll_out_smooth_node->getDoubleValue();

    SG_LOG( SG_ALL, SG_DEBUG, "FGAutopilot::run()  lat = " << lat <<
            "  lon = " << lon << "  alt = " << alt );
	
#ifdef FG_FORCE_AUTO_DISENGAGE
    // see if somebody else has changed them
    if( fabs(aileron - old_aileron) > disengage_threshold ||
	fabs(elevator - old_elevator) > disengage_threshold ||
	fabs(elevator_trim - old_elevator_trim) > 
	disengage_threshold ||		
	fabs(rudder - old_rudder) > disengage_threshold )
    {
	// if controls changed externally turn autopilot off
	waypoint_hold = false ;	  // turn the target hold off
	heading_hold = false ;	  // turn the heading hold off
	altitude_hold = false ;	  // turn the altitude hold off
	terrain_follow = false;	  // turn the terrain_follow hold off
	// auto_throttle = false; // turn the auto_throttle off

	// stash this runs control settings
	old_aileron = aileron;
	old_elevator = elevator;
	old_elevator_trim = elevator_trim;
	old_rudder = rudder;
	
	return 0;
    }
#endif
	
    // heading hold
    if ( heading_hold == true ) {
	if ( heading_mode == FG_DG_HEADING_LOCK ) {
	    TargetHeading = DGTargetHeading +
	      globals->get_steam()->get_DG_err();
	    while ( TargetHeading <   0.0 ) { TargetHeading += 360.0; }
	    while ( TargetHeading > 360.0 ) { TargetHeading -= 360.0; }
	    MakeTargetHeadingStr( TargetHeading );
	} else if ( heading_mode == FG_TC_HEADING_LOCK ) {
	    // we don't set a specific target heading in
	    // TC_HEADING_LOCK mode, we instead try to keep the turn
	    // coordinator zero'd
	} else if ( heading_mode == FG_TRUE_HEADING_LOCK ) {
	    // leave "true" target heading as is
            while ( TargetHeading <   0.0 ) { TargetHeading += 360.0; }
            while ( TargetHeading > 360.0 ) { TargetHeading -= 360.0; }
            MakeTargetHeadingStr( TargetHeading );
	} else if ( heading_mode == FG_HEADING_NAV1 ) {
	    // track the NAV1 heading needle deflection

	    // determine our current radial position relative to the
	    // navaid in "true" heading.
	    double cur_radial = current_radiostack->get_navcom1()->get_nav_heading();
	    if ( current_radiostack->get_navcom1()->get_nav_loc() ) {
		// ILS localizers radials are already "true" in our
		// database
	    } else {
		cur_radial += current_radiostack->get_navcom1()->get_nav_magvar();
	    }
	    if ( current_radiostack->get_navcom1()->get_nav_from_flag() ) {
		cur_radial += 180.0;
		while ( cur_radial >= 360.0 ) { cur_radial -= 360.0; }
	    }

	    // determine the target radial in "true" heading
	    double tgt_radial = current_radiostack->get_navcom1()->get_nav_radial();
	    if ( current_radiostack->get_navcom1()->get_nav_loc() ) {
		// ILS localizers radials are already "true" in our
		// database
	    } else {
		// VOR radials need to have that vor's offset added in
		tgt_radial += current_radiostack->get_navcom1()->get_nav_magvar();
	    }

	    // determine the heading adjustment needed.
	    double adjustment = 
		current_radiostack->get_navcom1()->get_nav_heading_needle_deflection()
		* (current_radiostack->get_navcom1()->get_nav_loc_dist() * SG_METER_TO_NM);
	    SG_CLAMP_RANGE( adjustment, -30.0, 30.0 );

            // clamp closer when inside cone when beyond 5km...
            if (current_radiostack->get_navcom1()->get_nav_loc_dist() > 5000) {
              double clamp_angle = fabs(current_radiostack->get_navcom1()->get_nav_heading_needle_deflection()) * 3;
              if (clamp_angle < 30)
                SG_CLAMP_RANGE( adjustment, -clamp_angle, clamp_angle);
            }

	    // determine the target heading to fly to intercept the
	    // tgt_radial
	    TargetHeading = tgt_radial + adjustment; 
	    while ( TargetHeading <   0.0 ) { TargetHeading += 360.0; }
	    while ( TargetHeading > 360.0 ) { TargetHeading -= 360.0; }

	    MakeTargetHeadingStr( TargetHeading );
	} else if ( heading_mode == FG_HEADING_WAYPOINT ) {
	    // update target heading to waypoint

	    double wp_course, wp_distance;

#ifdef DO_fgAP_CORRECTED_COURSE
	    // compute course made good
	    // this needs lots of special casing before use
	    double course, reverse, distance, corrected_course;
	    // need to test for iter
	    geo_inverse_wgs_84( 0, //fgAPget_altitude(),
				old_lat,
				old_lon,
				lat,
				lon,
				&course,
				&reverse,
				&distance );
#endif // DO_fgAP_CORRECTED_COURSE

	    // compute course to way_point
	    // need to test for iter
	    SGWayPoint wp = globals->get_route()->get_first();
	    wp.CourseAndDistance( lon, lat, alt, 
				  &wp_course, &wp_distance );

#ifdef DO_fgAP_CORRECTED_COURSE
	    corrected_course = course - wp_course;
	    if( fabs(corrected_course) > 0.1 )
		printf("fgAP: course %f  wp_course %f  %f  %f\n",
		       course, wp_course, fabs(corrected_course),
		       distance );
#endif // DO_fgAP_CORRECTED_COURSE
		
	    if ( wp_distance > 100 ) {
		// corrected_course = course - wp_course;
		TargetHeading = NormalizeDegrees(wp_course);
	    } else {
		// pop off this waypoint from the list
		if ( globals->get_route()->size() ) {
		    globals->get_route()->delete_first();
		}

		// see if there are more waypoints on the list
		if ( globals->get_route()->size() ) {
		    // more waypoints
		    set_HeadingMode( FG_HEADING_WAYPOINT );
		} else {
		    // end of the line
		    heading_mode = DEFAULT_AP_HEADING_LOCK;
		    // use current heading
		    TargetHeading = heading_node->getDoubleValue();
		}
	    }
	    MakeTargetHeadingStr( TargetHeading );
	    // Force this just in case
	    TargetDistance = wp_distance;
	    MakeTargetWPStr( wp_distance );
	}

	if ( heading_mode == FG_TC_HEADING_LOCK ) {
	    // drive the turn coordinator to zero
	    double turn = globals->get_steam()->get_TC_std();
	    double AileronSet = -turn / 2.0;
            SG_CLAMP_RANGE( AileronSet, -1.0, 1.0 );
	    globals->get_controls()->set_aileron( AileronSet );
	    globals->get_controls()->set_rudder( AileronSet / 4.0 );
	} else {
	    // steer towards the target heading

	    double RelHeading;
	    double TargetRoll;
	    double RelRoll;
	    double AileronSet;

	    RelHeading
		= NormalizeDegrees( TargetHeading
				    - heading_node->getDoubleValue() );
	    // figure out how far off we are from desired heading

	    // Now it is time to deterime how far we should be rolled.
	    SG_LOG( SG_AUTOPILOT, SG_DEBUG,
                    "Heading = " << heading_node->getDoubleValue() <<
                    " TargetHeading = " << TargetHeading <<
                    " RelHeading = " << RelHeading );

	    // Check if we are further from heading than the roll out point
	    if ( fabs( RelHeading ) > RollOut ) {
		// set Target Roll to Max in desired direction
		if ( RelHeading < 0 ) {
		    TargetRoll = 0 - MaxRoll;
		} else {
		    TargetRoll = MaxRoll;
		}
	    } else {
		// We have to calculate the Target roll

		// This calculation engine thinks that the Target roll
		// should be a line from (RollOut,MaxRoll) to (-RollOut,
		// -MaxRoll) I hope this works well.  If I get ambitious
		// some day this might become a fancier curve or
		// something.

		TargetRoll = LinearExtrapolate( RelHeading, -RollOut,
						-MaxRoll, RollOut,
						MaxRoll );
	    }

	    // Target Roll has now been Found.

	    // Compare Target roll to Current Roll, Generate Rel Roll

	    SG_LOG( SG_COCKPIT, SG_BULK, "TargetRoll: " << TargetRoll );

	    RelRoll = NormalizeDegrees( TargetRoll 
					- roll_node->getDoubleValue() );

	    // Check if we are further from heading than the roll out
	    // smooth point
	    if ( fabs( RelRoll ) > RollOutSmooth ) {
		// set Target Roll to Max in desired direction
		if ( RelRoll < 0 ) {
		AileronSet = 0 - MaxAileron;
		} else {
		    AileronSet = MaxAileron;
		}
	    } else {
		AileronSet = LinearExtrapolate( RelRoll, -RollOutSmooth,
						-MaxAileron,
						RollOutSmooth,
						MaxAileron );
	    }

	    globals->get_controls()->set_aileron( AileronSet );
	    globals->get_controls()->set_rudder( AileronSet / 4.0 );
	    // controls.set_rudder( 0.0 );
	}
    }

    // altitude hold
    if ( altitude_hold ) {
	double climb_rate;
	double speed, max_climb, error;
	double prop_error, int_error;
	double prop_adj, int_adj, total_adj;

	if ( altitude_mode == FG_ALTITUDE_LOCK ) {
	    climb_rate =
		( TargetAltitude -
		  globals->get_steam()->get_ALT_ft() * SG_FEET_TO_METER ) * 8.0;
        } else if ( altitude_mode == FG_TRUE_ALTITUDE_LOCK ) {
            climb_rate = ( TargetAltitude - alt ) * 8.0;
	} else if ( altitude_mode == FG_ALTITUDE_GS1 ) {
	    double x = current_radiostack->get_navcom1()->get_nav_gs_dist();
	    double y = (altitude_node->getDoubleValue()
			- current_radiostack->get_navcom1()->get_nav_elev()) * SG_FEET_TO_METER;
	    double current_angle = atan2( y, x ) * SGD_RADIANS_TO_DEGREES;

	    double target_angle = current_radiostack->get_navcom1()->get_nav_target_gs();

	    double gs_diff = target_angle - current_angle;

	    // convert desired vertical path angle into a climb rate
	    double des_angle = current_angle - 10 * gs_diff;

	    // convert to meter/min
	    double horiz_vel = cur_fdm_state->get_V_ground_speed()
		* SG_FEET_TO_METER * 60.0;
	    climb_rate = -sin( des_angle * SGD_DEGREES_TO_RADIANS ) * horiz_vel;
	    /* climb_error_accum += gs_diff * 2.0; */
	    /* climb_rate = gs_diff * 200.0 + climb_error_accum; */
	} else if ( altitude_mode == FG_ALTITUDE_TERRAIN ) {
	    // brain dead ground hugging with no look ahead
	    climb_rate =
		( TargetAGL - altitude_agl_node->getDoubleValue()
                  * SG_FEET_TO_METER ) * 16.0;
	} else {
	    // just try to zero out rate of climb ...
	    climb_rate = 0.0;
	}

	speed = get_speed();

	if ( speed < min_climb->getFloatValue() ) {
	    max_climb = 0.0;
	} else if ( speed < best_climb->getFloatValue() ) {
	    max_climb = ((best_climb->getFloatValue()
                          - min_climb->getFloatValue())
                         - (best_climb->getFloatValue() - speed)) 
		* fabs(TargetClimbRate->getFloatValue() * SG_FEET_TO_METER) 
		/ (best_climb->getFloatValue() - min_climb->getFloatValue());
	} else {			
	    max_climb = ( speed - best_climb->getFloatValue() ) * 10.0
                + fabs(TargetClimbRate->getFloatValue() * SG_FEET_TO_METER);
	}

	// this first one could be optional if we wanted to allow
	// better climb performance assuming we have the airspeed to
	// support it.
	if ( climb_rate >
             fabs(TargetClimbRate->getFloatValue() * SG_FEET_TO_METER) ) {
	    climb_rate
                = fabs(TargetClimbRate->getFloatValue() * SG_FEET_TO_METER);
	}

	if ( climb_rate > max_climb ) {
	    climb_rate = max_climb;
	}

	if ( climb_rate <
             -fabs(TargetDescentRate->getFloatValue() * SG_FEET_TO_METER) ) {
	    climb_rate
                = -fabs(TargetDescentRate->getFloatValue() * SG_FEET_TO_METER);
	}

	error = vertical_speed_node->getDoubleValue() * 60
            - climb_rate * SG_METER_TO_FEET;

	// accumulate the error under the curve ... this really should
	// be *= delta t
	alt_error_accum += error;

	// calculate integral error, and adjustment amount
	int_error = alt_error_accum;
	// printf("error = %.2f  int_error = %.2f\n", error, int_error);
        
        // scale elev_adj_factor by speed of aircraft in relation to min climb 
	double elev_adj_factor = elevator_adj_factor->getFloatValue();
        elev_adj_factor *=
	  pow(float(speed / min_climb->getFloatValue()), 3.0f);

	int_adj = int_error / elev_adj_factor;

	// caclulate proportional error
	prop_error = error;
	prop_adj = prop_error / elev_adj_factor;

	total_adj = ((double) 1.0 - (double) integral_contrib->getFloatValue()) * prop_adj
            + (double) integral_contrib->getFloatValue() * int_adj;

        // stop on autopilot trim at 30% +/-
//	if ( total_adj > 0.3 ) {
//	     total_adj = 0.3;
//	 } else if ( total_adj < -0.3 ) {
//	     total_adj = -0.3;
//	 }

        // adjust for throttle pitch gain
        total_adj += ((current_throttle->getFloatValue() - zero_pitch_throttle->getFloatValue())
                     / (1 - zero_pitch_throttle->getFloatValue()))
                     * zero_pitch_trim_full_throttle->getFloatValue();

	globals->get_controls()->set_elevator_trim( total_adj );
    }

    // auto throttle
    if ( auto_throttle ) {
	double error;
	double prop_error, int_error;
	double prop_adj, int_adj, total_adj;

	error = TargetSpeed - get_speed();

	// accumulate the error under the curve ... this really should
	// be *= delta t
	speed_error_accum += error;
	if ( speed_error_accum > 2000.0 ) {
	    speed_error_accum = 2000.0;
	}
	else if ( speed_error_accum < -2000.0 ) {
	    speed_error_accum = -2000.0;
	}

	// calculate integral error, and adjustment amount
	int_error = speed_error_accum;

	// printf("error = %.2f  int_error = %.2f\n", error, int_error);
	int_adj = int_error / 200.0;

	// caclulate proportional error
	prop_error = error;
	prop_adj = 0.5 + prop_error / 50.0;

	total_adj = 0.9 * prop_adj + 0.1 * int_adj;
	if ( total_adj > 1.0 ) {
	    total_adj = 1.0;
	}
	else if ( total_adj < 0.0 ) {
	    total_adj = 0.0;
	}

	globals->get_controls()->set_throttle( FGControls::ALL_ENGINES,
					       total_adj );
    }

#ifdef THIS_CODE_IS_NOT_USED
    if (Mode == 2) // Glide slope hold
	{
	    double RelSlope;
	    double RelElevator;

	    // First, calculate Relative slope and normalize it
	    RelSlope = NormalizeDegrees( TargetSlope - get_pitch());

	    // Now calculate the elevator offset from current angle
	    if ( abs(RelSlope) > SlopeSmooth )
		{
		    if ( RelSlope < 0 )     //  set RelElevator to max in the correct direction
			RelElevator = -MaxElevator;
		    else
			RelElevator = MaxElevator;
		}

	    else
		RelElevator = LinearExtrapolate(RelSlope,-SlopeSmooth,-MaxElevator,SlopeSmooth,MaxElevator);

	    // set the elevator
	    fgElevMove(RelElevator);

	}
#endif // THIS_CODE_IS_NOT_USED

    // stash this runs control settings
    //	update_old_control_values();
    old_aileron = globals->get_controls()->get_aileron();
    old_elevator = globals->get_controls()->get_elevator();
    old_elevator_trim = globals->get_controls()->get_elevator_trim();
    old_rudder = globals->get_controls()->get_rudder();

    // for cross track error
    old_lat = lat;
    old_lon = lon;
	
    // Ok, we are done
    SG_LOG( SG_ALL, SG_DEBUG, "FGAutopilot::run( returns )" );
}


void FGAutopilot::set_HeadingMode( fgAutoHeadingMode mode ) {
    heading_mode = mode;

    if ( heading_mode == FG_DG_HEADING_LOCK ) {
	// set heading hold to current heading (as read from DG)
	// ... no, leave target heading along ... just use the current
	// heading bug value
        //  DGTargetHeading = FGSteam::get_DG_deg();
    } else if ( heading_mode == FG_TC_HEADING_LOCK ) {
	// set autopilot to hold a zero turn (as reported by the TC)
    } else if ( heading_mode == FG_TRUE_HEADING_LOCK ) {
	// set heading hold to current heading
	TargetHeading = heading_node->getDoubleValue();
    } else if ( heading_mode == FG_HEADING_WAYPOINT ) {
	if ( globals->get_route()->size() ) {
	    double course, distance;

	    old_lat = latitude_node->getDoubleValue();
	    old_lon = longitude_node->getDoubleValue();

	    waypoint = globals->get_route()->get_first();
	    waypoint.CourseAndDistance( longitude_node->getDoubleValue(),
					latitude_node->getDoubleValue(),
					altitude_node->getDoubleValue()
                                        * SG_FEET_TO_METER,
					&course, &distance );
	    TargetHeading = course;
	    TargetDistance = distance;
	    MakeTargetLatLonStr( waypoint.get_target_lat(),
				 waypoint.get_target_lon() );
	    MakeTargetWPStr( distance );

	    if ( waypoint.get_target_alt() > 0.0 ) {
		TargetAltitude = waypoint.get_target_alt();
		altitude_mode = DEFAULT_AP_ALTITUDE_LOCK;
		set_AltitudeEnabled( true );
		MakeTargetAltitudeStr( TargetAltitude * SG_METER_TO_FEET );
	    }

	    SG_LOG( SG_COCKPIT, SG_INFO, " set_HeadingMode: ( "
		    << get_TargetLatitude()  << " "
		    << get_TargetLongitude() << " ) "
		    );
	} else {
	    // no more way points, default to heading lock.
	    heading_mode = FG_TC_HEADING_LOCK;
	}
    }

    MakeTargetHeadingStr( TargetHeading );			
    update_old_control_values();
}


void FGAutopilot::set_AltitudeMode( fgAutoAltitudeMode mode ) {
    altitude_mode = mode;

    alt_error_accum = 0.0;


    if ( altitude_mode == DEFAULT_AP_ALTITUDE_LOCK ) {
	if ( TargetAltitude < altitude_agl_node->getDoubleValue()
             * SG_FEET_TO_METER ) {
	}

	if ( !strcmp(fgGetString("/sim/startup/units"), "feet") ) {
	    MakeTargetAltitudeStr( TargetAltitude * SG_METER_TO_FEET );
	} else {
	    MakeTargetAltitudeStr( TargetAltitude * SG_METER_TO_FEET );
	}
    } else if ( altitude_mode == FG_ALTITUDE_GS1 ) {
	climb_error_accum = 0.0;

    } else if ( altitude_mode == FG_ALTITUDE_TERRAIN ) {
	TargetAGL = altitude_agl_node->getDoubleValue() * SG_FEET_TO_METER;

	if ( !strcmp(fgGetString("/sim/startup/units"), "feet") ) {
	    MakeTargetAltitudeStr( TargetAGL * SG_METER_TO_FEET );
	} else {
	    MakeTargetAltitudeStr( TargetAGL * SG_METER_TO_FEET );
	}
    }
    
    update_old_control_values();
    SG_LOG( SG_COCKPIT, SG_INFO, " set_AltitudeMode():" );
}


void FGAutopilot::AltitudeSet( double new_altitude ) {
    double target_alt = new_altitude;
    altitude_mode = DEFAULT_AP_ALTITUDE_LOCK;

    if ( !strcmp(fgGetString("/sim/startup/units"), "feet") ) {
	target_alt = new_altitude * SG_FEET_TO_METER;
    }

    if( target_alt < globals->get_scenery()->get_cur_elev() ) {
	target_alt = globals->get_scenery()->get_cur_elev();
    }

    TargetAltitude = target_alt;

    if ( !strcmp(fgGetString("/sim/startup/units"), "feet") ) {
	target_alt *= SG_METER_TO_FEET;
    }
    // ApAltitudeDialogInput->setValue((float)target_alt);
    MakeTargetAltitudeStr( target_alt );
	
    update_old_control_values();
}


void FGAutopilot::AltitudeAdjust( double inc )
{
    double target_alt, target_agl;

    if ( !strcmp(fgGetString("/sim/startup/units"), "feet") ) {
	target_alt = TargetAltitude * SG_METER_TO_FEET;
	target_agl = TargetAGL * SG_METER_TO_FEET;
    } else {
	target_alt = TargetAltitude;
	target_agl = TargetAGL;
    }

    if ( fabs((int)(target_alt / inc) * inc - target_alt) < SG_EPSILON ) {
	target_alt += inc;
    } else {
	target_alt = ( int ) ( target_alt / inc ) * inc + inc;
    }

    if ( fabs((int)(target_agl / inc) * inc - target_agl) < SG_EPSILON ) {
	target_agl += inc;
    } else {
	target_agl = ( int ) ( target_agl / inc ) * inc + inc;
    }

    if ( !strcmp(fgGetString("/sim/startup/units"), "feet") ) {
	target_alt *= SG_FEET_TO_METER;
	target_agl *= SG_FEET_TO_METER;
    }

    TargetAltitude = target_alt;
    TargetAGL = target_agl;
	
    if ( !strcmp(fgGetString("/sim/startup/units"), "feet") )
	target_alt *= SG_METER_TO_FEET;
    if ( !strcmp(fgGetString("/sim/startup/units"), "feet") )
	target_agl *= SG_METER_TO_FEET;

    if ( altitude_mode == DEFAULT_AP_ALTITUDE_LOCK ) {
	MakeTargetAltitudeStr( target_alt );
    } else if ( altitude_mode == FG_ALTITUDE_TERRAIN ) {
	MakeTargetAltitudeStr( target_agl );
    }

    update_old_control_values();
}


void FGAutopilot::HeadingAdjust( double inc ) {
    if ( heading_mode != FG_DG_HEADING_LOCK
	 && heading_mode != FG_TRUE_HEADING_LOCK )
    {
	heading_mode = DEFAULT_AP_HEADING_LOCK;
    }

    if ( heading_mode == FG_DG_HEADING_LOCK ) {
	double target = ( int ) ( DGTargetHeading / inc ) * inc + inc;
	DGTargetHeading = NormalizeDegrees( target );
    } else {
	double target = ( int ) ( TargetHeading / inc ) * inc + inc;
	TargetHeading = NormalizeDegrees( target );
    }

    update_old_control_values();
}


void FGAutopilot::HeadingSet( double new_heading ) {
    heading_mode = DEFAULT_AP_HEADING_LOCK;
    if( heading_mode == FG_TRUE_HEADING_LOCK ) {
        new_heading = NormalizeDegrees( new_heading );
        TargetHeading = new_heading;
        MakeTargetHeadingStr( TargetHeading );
    } else {
        heading_mode = FG_DG_HEADING_LOCK;

        new_heading = NormalizeDegrees( new_heading );
        DGTargetHeading = new_heading;
        // following cast needed ambiguous plib
        // ApHeadingDialogInput -> setValue ((float)APData->TargetHeading );
        MakeTargetHeadingStr( DGTargetHeading );
    }
    update_old_control_values();
}

void FGAutopilot::AutoThrottleAdjust( double inc ) {
    double target = ( int ) ( TargetSpeed / inc ) * inc + inc;

    TargetSpeed = target;
}


void FGAutopilot::set_AutoThrottleEnabled( bool value ) {
    auto_throttle = value;

    if ( auto_throttle == true ) {
        TargetSpeed = fgGetDouble("/velocities/airspeed-kt");
	speed_error_accum = 0.0;
    }

    update_old_control_values();
    SG_LOG( SG_COCKPIT, SG_INFO, " fgAPSetAutoThrottle: ("
	    << auto_throttle << ") " << TargetSpeed );
}




////////////////////////////////////////////////////////////////////////
// Kludged methods for tying to properties.
//
// These should change eventually; they all used to be static
// functions.
////////////////////////////////////////////////////////////////////////

/**
 * Get the autopilot altitude lock (true=on).
 */
bool
FGAutopilot::getAPAltitudeLock () const
{
    return (get_AltitudeEnabled() &&
	    get_AltitudeMode()
	    == DEFAULT_AP_ALTITUDE_LOCK);
}


/**
 * Set the autopilot altitude lock (true=on).
 */
void
FGAutopilot::setAPAltitudeLock (bool lock)
{
  if (lock)
    set_AltitudeMode(DEFAULT_AP_ALTITUDE_LOCK);
  if (get_AltitudeMode() == DEFAULT_AP_ALTITUDE_LOCK)
    set_AltitudeEnabled(lock);
}


/**
 * Get the autopilot target altitude in feet.
 */
double
FGAutopilot::getAPAltitude () const
{
  return get_TargetAltitude() * SG_METER_TO_FEET;
}


/**
 * Set the autopilot target altitude in feet.
 */
void
FGAutopilot::setAPAltitude (double altitude)
{
  set_TargetAltitude( altitude * SG_FEET_TO_METER );
}

/**
 * Get the autopilot altitude lock (true=on).
 */
bool
FGAutopilot::getAPGSLock () const
{
    return (get_AltitudeEnabled() &&
	    (get_AltitudeMode()
	     == FGAutopilot::FG_ALTITUDE_GS1));
}


/**
 * Set the autopilot altitude lock (true=on).
 */
void
FGAutopilot::setAPGSLock (bool lock)
{
  if (lock)
    set_AltitudeMode(FGAutopilot::FG_ALTITUDE_GS1);
  if (get_AltitudeMode() == FGAutopilot::FG_ALTITUDE_GS1)
    set_AltitudeEnabled(lock);
}


/**
 * Get the autopilot terrain lock (true=on).
 */
bool
FGAutopilot::getAPTerrainLock () const
{
    return (get_AltitudeEnabled() &&
	    (get_AltitudeMode()
	     == FGAutopilot::FG_ALTITUDE_TERRAIN));
}


/**
 * Set the autopilot terrain lock (true=on).
 */
void
FGAutopilot::setAPTerrainLock (bool lock)
{
  if (lock) {
    set_AltitudeMode(FGAutopilot::FG_ALTITUDE_TERRAIN);
    set_TargetAGL(fgGetFloat("/position/altitude-agl-ft") *
		  SG_FEET_TO_METER);
  }
  if (get_AltitudeMode() == FGAutopilot::FG_ALTITUDE_TERRAIN)
    set_AltitudeEnabled(lock);
}


/**
 * Get the autopilot target altitude in feet.
 */
double
FGAutopilot::getAPClimb () const
{
  return get_TargetClimbRate() * SG_METER_TO_FEET;
}


/**
 * Set the autopilot target altitude in feet.
 */
void
FGAutopilot::setAPClimb (double rate)
{
    set_TargetClimbRate( rate * SG_FEET_TO_METER );
}


/**
 * Get the autopilot heading lock (true=on).
 */
bool
FGAutopilot::getAPHeadingLock () const
{
    return
      (get_HeadingEnabled() &&
       get_HeadingMode() == DEFAULT_AP_HEADING_LOCK);
}


/**
 * Set the autopilot heading lock (true=on).
 */
void
FGAutopilot::setAPHeadingLock (bool lock)
{
  if (lock)
    set_HeadingMode(DEFAULT_AP_HEADING_LOCK);
  if (get_HeadingMode() == DEFAULT_AP_HEADING_LOCK)
    set_HeadingEnabled(lock);
}


/**
 * Get the autopilot heading bug in degrees.
 */
double
FGAutopilot::getAPHeadingBug () const
{
  return get_DGTargetHeading();
}


/**
 * Set the autopilot heading bug in degrees.
 */
void
FGAutopilot::setAPHeadingBug (double heading)
{
  set_DGTargetHeading( heading );
}


/**
 * Get the autopilot wing leveler lock (true=on).
 */
bool
FGAutopilot::getAPWingLeveler () const
{
    return
      (get_HeadingEnabled() &&
       get_HeadingMode() == FGAutopilot::FG_TC_HEADING_LOCK);
}


/**
 * Set the autopilot wing leveler lock (true=on).
 */
void
FGAutopilot::setAPWingLeveler (bool lock)
{
    if (lock)
	set_HeadingMode(FGAutopilot::FG_TC_HEADING_LOCK);
    if (get_HeadingMode() == FGAutopilot::FG_TC_HEADING_LOCK)
      set_HeadingEnabled(lock);
}

/**
 * Return true if the autopilot is locked to NAV1.
 */
bool
FGAutopilot::getAPNAV1Lock () const
{
  return
    (get_HeadingEnabled() &&
     get_HeadingMode() == FGAutopilot::FG_HEADING_NAV1);
}


/**
 * Set the autopilot NAV1 lock.
 */
void
FGAutopilot::setAPNAV1Lock (bool lock)
{
  if (lock)
    set_HeadingMode(FGAutopilot::FG_HEADING_NAV1);
  if (get_HeadingMode() == FGAutopilot::FG_HEADING_NAV1)
    set_HeadingEnabled(lock);
}

/**
 * Get the autopilot autothrottle lock.
 */
bool
FGAutopilot::getAPAutoThrottleLock () const
{
  return get_AutoThrottleEnabled();
}


/**
 * Set the autothrottle lock.
 */
void
FGAutopilot::setAPAutoThrottleLock (bool lock)
{
  set_AutoThrottleEnabled(lock);
}


// kludge
double
FGAutopilot::getAPRudderControl () const
{
    if (getAPHeadingLock())
        return get_TargetHeading();
    else
        return globals->get_controls()->get_rudder();
}

// kludge
void
FGAutopilot::setAPRudderControl (double value)
{
    if (getAPHeadingLock()) {
        SG_LOG(SG_GENERAL, SG_DEBUG, "setAPRudderControl " << value );
        value -= get_TargetHeading();
        HeadingAdjust(value < 0.0 ? -1.0 : 1.0);
    } else {
        globals->get_controls()->set_rudder(value);
    }
}

// kludge
double
FGAutopilot::getAPElevatorControl () const
{
  if (getAPAltitudeLock())
      return get_TargetAltitude();
  else
    return globals->get_controls()->get_elevator();
}

// kludge
void
FGAutopilot::setAPElevatorControl (double value)
{
    if (value != 0 && getAPAltitudeLock()) {
        SG_LOG(SG_GENERAL, SG_DEBUG, "setAPElevatorControl " << value );
        value -= get_TargetAltitude();
        AltitudeAdjust(value < 0.0 ? 100.0 : -100.0);
    } else {
        globals->get_controls()->set_elevator(value);
    }
}

// kludge
double
FGAutopilot::getAPThrottleControl () const
{
  if (getAPAutoThrottleLock())
    return 0.0;			// always resets
  else
    return globals->get_controls()->get_throttle(0);
}

// kludge
void
FGAutopilot::setAPThrottleControl (double value)
{
  if (getAPAutoThrottleLock())
    AutoThrottleAdjust(value < 0.0 ? -0.01 : 0.01);
  else
    globals->get_controls()->set_throttle(FGControls::ALL_ENGINES, value);
}

// end of newauto.cxx

