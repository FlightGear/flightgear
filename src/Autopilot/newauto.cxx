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

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/math/sg_random.h>

#include <Cockpit/steam.hxx>
#include <Cockpit/radiostack.hxx>
#include <Controls/controls.hxx>
#include <FDM/flight.hxx>
#include <Main/bfi.hxx>
#include <Main/globals.hxx>
#include <Scenery/scenery.hxx>

#include "newauto.hxx"


FGAutopilot *current_autopilot;


// Climb speed constants
const double min_climb = 70.0;	// kts
const double best_climb = 75.0;	// kts
// const double ideal_climb_rate = 500.0 * FEET_TO_METER; // fpm -> mpm
// const double ideal_decent_rate = 1000.0 * FEET_TO_METER; // fpm -> mpm

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
FGAutopilot::FGAutopilot():
TargetClimbRate(1000 * FEET_TO_METER)
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
    if( bearing < 0. ) {
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
    double ft_s = cur_fdm_state->get_V_ground_speed() 
      * fgGetInt("/sim/speed-up"); // FIXME: inefficient
    double kts = ft_s * FEET_TO_METER * 3600 * METER_TO_NM;

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
	double eta = accum * METER_TO_NM / get_ground_speed();
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
		 accum*METER_TO_NM, major, minor );
	// cout << "distance = " << distance*METER_TO_NM
	//      << "  gndsp = " << get_ground_speed() 
	//      << "  time = " << eta
	//      << "  major = " << major
	//      << "  minor = " << minor
	//      << endl;
    }

    // next route
    if ( size > 1 ) {
	SGWayPoint wp2 = globals->get_route()->get_waypoint( 1 );
	accum += wp2.get_distance();
	
	double eta = accum * METER_TO_NM / get_ground_speed();
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
		 accum*METER_TO_NM, major, minor );
    }

    // next route
    if ( size > 2 ) {
	for ( int i = 2; i < size; ++i ) {
	    accum += globals->get_route()->get_waypoint( i ).get_distance();
	}
	
	SGWayPoint wpn = globals->get_route()->get_waypoint( size - 1 );

	double eta = accum * METER_TO_NM / get_ground_speed();
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
		 accum*METER_TO_NM, major, minor );
    }
}


void FGAutopilot::update_old_control_values() {
    old_aileron = controls.get_aileron();
    old_elevator = controls.get_elevator();
    old_elevator_trim = controls.get_elevator_trim();
    old_rudder = controls.get_rudder();
}


// Initialize autopilot subsystem
void FGAutopilot::init() {
    FG_LOG( FG_AUTOPILOT, FG_INFO, "Init AutoPilot Subsystem" );

    heading_hold = false ;      // turn the heading hold off
    altitude_hold = false ;     // turn the altitude hold off
    auto_throttle = false ;	// turn the auto throttle off

    sg_srandom_time();
    DGTargetHeading = sg_random() * 360.0;

    // Initialize target location to startup location
    old_lat = FGBFI::getLatitude();
    old_lon = FGBFI::getLongitude();
    // set_WayPoint( old_lon, old_lat, 0.0, "default" );

    MakeTargetLatLonStr( get_TargetLatitude(), get_TargetLongitude() );
	
    TargetHeading = 0.0;	// default direction, due north
    TargetAltitude = 3000;	// default altitude in meters
    alt_error_accum = 0.0;
    climb_error_accum = 0.0;

    MakeTargetAltitudeStr( 3000.0);
    MakeTargetHeadingStr( 0.0 );
	
    // These eventually need to be read from current_aircaft somehow.

    // the maximum roll, in Deg
    MaxRoll = 20;

    // the deg from heading to start rolling out at, in Deg
    RollOut = 20;

    // how far can I move the aleron from center.
    MaxAileron = .2;

    // Smoothing distance for alerion control
    RollOutSmooth = 10;

    // Hardwired for now should be in options
    // 25% max control variablilty  0.5 / 2.0
    disengage_threshold = 1.0;

#if !defined( USING_SLIDER_CLASS )
    MaxRollAdjust = 2 * MaxRoll;
    RollOutAdjust = 2 * RollOut;
    MaxAileronAdjust = 2 * MaxAileron;
    RollOutSmoothAdjust = 2 * RollOutSmooth;
#endif  // !defined( USING_SLIDER_CLASS )

    update_old_control_values();
	
    // Initialize GUI components of autopilot
    // NewTgtAirportInit();
    // fgAPAdjustInit() ;
    // NewHeadingInit();
    // NewAltitudeInit();
};


// Reset the autopilot system
void FGAutopilot::reset() {

    heading_hold = false ;      // turn the heading hold off
    altitude_hold = false ;     // turn the altitude hold off
    auto_throttle = false ;	// turn the auto throttle off

    TargetHeading = 0.0;	// default direction, due north
    MakeTargetHeadingStr( TargetHeading );			
	
    TargetAltitude = 3000;   // default altitude in meters
    MakeTargetAltitudeStr( TargetAltitude );
	
    alt_error_accum = 0.0;
    climb_error_accum = 0.0;
	
    update_old_control_values();

    sprintf( NewTgtAirportId, "%s", fgGetString("/sim/startup/airport-id").c_str() );
	
    // TargetLatitude = FGBFI::getLatitude();
    // TargetLongitude = FGBFI::getLongitude();
    // set_WayPoint( FGBFI::getLongitude(), FGBFI::getLatitude(), 0.0, "reset" );

    MakeTargetLatLonStr( get_TargetLatitude(), get_TargetLongitude() );
}


static double NormalizeDegrees( double Input ) {
    // normalize the input to the range (-180,180]
    // Input should not be greater than -360 to 360.
    // Current rules send the output to an undefined state.
    if ( Input > 180 )
	while(Input > 180 )
	    Input -= 360;
    else if ( Input <= -180 )
	while ( Input <= -180 )
	    Input += 360;
    return ( Input );
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


int FGAutopilot::run() {
    // Remove the following lines when the calling funcitons start
    // passing in the data pointer

    // get control settings 
    // double aileron = FGBFI::getAileron();
    // double elevator = FGBFI::getElevator();
    // double elevator_trim = FGBFI::getElevatorTrim();
    // double rudder = FGBFI::getRudder();
	
    double lat = FGBFI::getLatitude();
    double lon = FGBFI::getLongitude();
    double alt = FGBFI::getAltitude() * FEET_TO_METER;

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
	    // cout << "DG heading = " << FGSteam::get_DG_deg()
	    //      << " DG error = " << FGSteam::get_DG_err() << endl;

	    TargetHeading = DGTargetHeading + FGSteam::get_DG_err();
	    while ( TargetHeading <   0.0 ) { TargetHeading += 360.0; }
	    while ( TargetHeading > 360.0 ) { TargetHeading -= 360.0; }
	    MakeTargetHeadingStr( TargetHeading );
	} else if ( heading_mode == FG_TC_HEADING_LOCK ) {
	    // we don't set a specific target heading in
	    // TC_HEADING_LOCK mode, we instead try to keep the turn
	    // coordinator zero'd
	} else if ( heading_mode == FG_TRUE_HEADING_LOCK ) {
	    // leave "true" target heading as is
	} else if ( heading_mode == FG_HEADING_NAV1 ) {
	    // track the NAV1 heading needle deflection

	    // determine our current radial position relative to the
	    // navaid in "true" heading.
	    double cur_radial = current_radiostack->get_nav1_heading();
	    if ( current_radiostack->get_nav1_loc() ) {
		// ILS localizers radials are already "true" in our
		// database
	    } else {
		cur_radial += current_radiostack->get_nav1_magvar();
	    }
	    if ( current_radiostack->get_nav1_from_flag() ) {
		cur_radial += 180.0;
		while ( cur_radial >= 360.0 ) { cur_radial -= 360.0; }
	    }

	    // determine the target radial in "true" heading
	    double tgt_radial = current_radiostack->get_nav1_radial();
	    if ( current_radiostack->get_nav1_loc() ) {
		// ILS localizers radials are already "true" in our
		// database
	    } else {
		// VOR radials need to have that vor's offset added in
		tgt_radial += current_radiostack->get_nav1_magvar();
	    }

	    // determine the heading adjustment needed.
	    double adjustment = 
		current_radiostack->get_nav1_heading_needle_deflection()
		* (current_radiostack->get_nav1_loc_dist() * METER_TO_NM);
	    if ( adjustment < -30.0 ) { adjustment = -30.0; }
	    if ( adjustment >  30.0 ) { adjustment =  30.0; }

	    // determine the target heading to fly to intercept the
	    // tgt_radial
	    TargetHeading = tgt_radial + adjustment; 
	    while ( TargetHeading <   0.0 ) { TargetHeading += 360.0; }
	    while ( TargetHeading > 360.0 ) { TargetHeading -= 360.0; }

	    MakeTargetHeadingStr( TargetHeading );
	    // cout << "target course (true) = " << TargetHeading << endl;
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
		cout << "Reached waypoint within " << wp_distance << "meters"
		     << endl;

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
		    heading_mode = FG_TRUE_HEADING_LOCK;
		    // use current heading
		    TargetHeading = FGBFI::getHeading();
		}
	    }
	    MakeTargetHeadingStr( TargetHeading );
	    // Force this just in case
	    TargetDistance = wp_distance;
	    MakeTargetWPStr( wp_distance );
	}

	if ( heading_mode == FG_TC_HEADING_LOCK ) {
	    // drive the turn coordinator to zero
	    double turn = FGSteam::get_TC_std();
	    // cout << "turn rate = " << turn << endl;
	    double AileronSet = -turn / 2.0;
	    if ( AileronSet < -1.0 ) { AileronSet = -1.0; }
	    if ( AileronSet >  1.0 ) { AileronSet =  1.0; }
	    controls.set_aileron( AileronSet );
	    controls.set_rudder( AileronSet / 4.0 );
	} else {
	    // steer towards the target heading

	    double RelHeading;
	    double TargetRoll;
	    double RelRoll;
	    double AileronSet;

	    RelHeading
		= NormalizeDegrees( TargetHeading - FGBFI::getHeading() );
	    // figure out how far off we are from desired heading

	    // Now it is time to deterime how far we should be rolled.
	    FG_LOG( FG_AUTOPILOT, FG_DEBUG, "RelHeading: " << RelHeading );


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

	    FG_LOG( FG_COCKPIT, FG_BULK, "TargetRoll: " << TargetRoll );

	    RelRoll = NormalizeDegrees( TargetRoll - FGBFI::getRoll() );

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

	    controls.set_aileron( AileronSet );
	    controls.set_rudder( AileronSet / 4.0 );
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
	    // normal altitude hold
	    // cout << "TargetAltitude = " << TargetAltitude
	    //      << "Altitude = " << FGBFI::getAltitude() * FEET_TO_METER
	    //      << endl;
	    climb_rate =
		( TargetAltitude - FGSteam::get_ALT_ft() * FEET_TO_METER ) * 8.0;
	} else if ( altitude_mode == FG_ALTITUDE_GS1 ) {
	    double x = current_radiostack->get_nav1_gs_dist();
	    double y = (FGBFI::getAltitude() 
			- current_radiostack->get_nav1_elev()) * FEET_TO_METER;
	    double current_angle = atan2( y, x ) * SGD_RADIANS_TO_DEGREES;
	    // cout << "current angle = " << current_angle << endl;

	    double target_angle = current_radiostack->get_nav1_target_gs();
	    // cout << "target angle = " << target_angle << endl;

	    double gs_diff = target_angle - current_angle;
	    // cout << "difference from desired = " << gs_diff << endl;

	    // convert desired vertical path angle into a climb rate
	    double des_angle = current_angle - 10 * gs_diff;
	    // cout << "desired angle = " << des_angle << endl;

	    // convert to meter/min
	    // cout << "raw ground speed = " << cur_fdm_state->get_V_ground_speed() << endl;
	    double horiz_vel = cur_fdm_state->get_V_ground_speed()
		* FEET_TO_METER * 60.0;
	    // cout << "Horizontal vel = " << horiz_vel << endl;
	    climb_rate = -sin( des_angle * SGD_DEGREES_TO_RADIANS ) * horiz_vel;
	    // cout << "climb_rate = " << climb_rate << endl;
	    /* climb_error_accum += gs_diff * 2.0; */
	    /* climb_rate = gs_diff * 200.0 + climb_error_accum; */
	} else if ( altitude_mode == FG_ALTITUDE_TERRAIN ) {
	    // brain dead ground hugging with no look ahead
	    climb_rate =
		( TargetAGL - FGBFI::getAGL()*FEET_TO_METER ) * 16.0;
	    // cout << "target agl = " << TargetAGL 
	    //      << "  current agl = " << fgAPget_agl() 
	    //      << "  target climb rate = " << climb_rate 
	    //      << endl;
	} else {
	    // just try to zero out rate of climb ...
	    climb_rate = 0.0;
	}

	speed = get_speed();

	if ( speed < min_climb ) {
	    max_climb = 0.0;
	} else if ( speed < best_climb ) {
	    max_climb = ((best_climb - min_climb) - (best_climb - speed)) 
		* fabs(TargetClimbRate) 
		/ (best_climb - min_climb);
	} else {			
	    max_climb = ( speed - best_climb ) * 10.0 + fabs(TargetClimbRate);
	}

	// this first one could be optional if we wanted to allow
	// better climb performance assuming we have the airspeed to
	// support it.
	if ( climb_rate > fabs(TargetClimbRate) ) {
	    climb_rate = fabs(TargetClimbRate);
	}

	if ( climb_rate > max_climb ) {
	    climb_rate = max_climb;
	}

	if ( climb_rate < -fabs(TargetClimbRate) ) {
	    climb_rate = -fabs(TargetClimbRate);
	}
	// cout << "Target climb rate = " << TargetClimbRate << endl;
	// cout << "given our speed, modified desired climb rate = "
	//      << climb_rate * METER_TO_FEET 
	//      << " fpm" << endl;

	error = FGBFI::getVerticalSpeed() * FEET_TO_METER - climb_rate;
	// cout << "climb rate = " << FGBFI::getVerticalSpeed()
	//      << "  vsi rate = " << FGSteam::get_VSI_fps() << endl;

	// accumulate the error under the curve ... this really should
	// be *= delta t
	alt_error_accum += error;

	// calculate integral error, and adjustment amount
	int_error = alt_error_accum;
	// printf("error = %.2f  int_error = %.2f\n", error, int_error);
	int_adj = int_error / 20000.0;

	// caclulate proportional error
	prop_error = error;
	prop_adj = prop_error / 2000.0;

	total_adj = 0.9 * prop_adj + 0.1 * int_adj;
	// if ( total_adj > 0.6 ) {
	//     total_adj = 0.6;
	// } else if ( total_adj < -0.2 ) {
	//     total_adj = -0.2;
	// }
	if ( total_adj > 1.0 ) {
	    total_adj = 1.0;
	} else if ( total_adj < -1.0 ) {
	    total_adj = -1.0;
	}

	controls.set_elevator( total_adj );
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

	controls.set_throttle( FGControls::ALL_ENGINES, total_adj );
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
    old_aileron = controls.get_aileron();
    old_elevator = controls.get_elevator();
    old_elevator_trim = controls.get_elevator_trim();
    old_rudder = controls.get_rudder();

    // for cross track error
    old_lat = lat;
    old_lon = lon;
	
	// Ok, we are done
    return 0;
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
	TargetHeading = FGBFI::getHeading();
    } else if ( heading_mode == FG_HEADING_WAYPOINT ) {
	if ( globals->get_route()->size() ) {
	    double course, distance;

	    old_lat = FGBFI::getLatitude();
	    old_lon = FGBFI::getLongitude();

	    waypoint = globals->get_route()->get_first();
	    waypoint.CourseAndDistance( FGBFI::getLongitude(),
					FGBFI::getLatitude(),
					FGBFI::getLatitude() * FEET_TO_METER,
					&course, &distance );
	    TargetHeading = course;
	    TargetDistance = distance;
	    MakeTargetLatLonStr( waypoint.get_target_lat(),
				 waypoint.get_target_lon() );
	    MakeTargetWPStr( distance );

	    if ( waypoint.get_target_alt() > 0.0 ) {
		TargetAltitude = waypoint.get_target_alt();
		altitude_mode = FG_ALTITUDE_LOCK;
		set_AltitudeEnabled( true );
		MakeTargetAltitudeStr( TargetAltitude * METER_TO_FEET );
	    }

	    FG_LOG( FG_COCKPIT, FG_INFO, " set_HeadingMode: ( "
		    << get_TargetLatitude()  << " "
		    << get_TargetLongitude() << " ) "
		    );
	} else {
	    // no more way points, default to heading lock.
	    heading_mode = FG_TC_HEADING_LOCK;
	    // TargetHeading = FGBFI::getHeading();
	}
    }

    MakeTargetHeadingStr( TargetHeading );			
    update_old_control_values();
}


void FGAutopilot::set_AltitudeMode( fgAutoAltitudeMode mode ) {
    altitude_mode = mode;

    alt_error_accum = 0.0;

    if ( altitude_mode == FG_ALTITUDE_LOCK ) {
	if ( TargetAltitude < FGBFI::getAGL() * FEET_TO_METER ) {
	    // TargetAltitude = FGBFI::getAltitude() * FEET_TO_METER;
	}

	if ( fgGetString("/sim/startup/units") == "feet" ) {
	    MakeTargetAltitudeStr( TargetAltitude * METER_TO_FEET );
	} else {
	    MakeTargetAltitudeStr( TargetAltitude * METER_TO_FEET );
	}
    } else if ( altitude_mode == FG_ALTITUDE_GS1 ) {
	climb_error_accum = 0.0;

    } else if ( altitude_mode == FG_ALTITUDE_TERRAIN ) {
	TargetAGL = FGBFI::getAGL() * FEET_TO_METER;

	if ( fgGetString("/sim/startup/units") == "feet" ) {
	    MakeTargetAltitudeStr( TargetAGL * METER_TO_FEET );
	} else {
	    MakeTargetAltitudeStr( TargetAGL * METER_TO_FEET );
	}
    }
    
    update_old_control_values();
    FG_LOG( FG_COCKPIT, FG_INFO, " set_AltitudeMode():" );
}


#if 0
static inline double get_aoa( void ) {
    return( cur_fdm_state->get_Gamma_vert_rad() * SGD_RADIANS_TO_DEGREES );
}

static inline double fgAPget_latitude( void ) {
    return( cur_fdm_state->get_Latitude() * SGD_RADIANS_TO_DEGREES );
}

static inline double fgAPget_longitude( void ) {
    return( cur_fdm_state->get_Longitude() * SGD_RADIANS_TO_DEGREES );
}

static inline double fgAPget_roll( void ) {
    return( cur_fdm_state->get_Phi() * SGD_RADIANS_TO_DEGREES );
}

static inline double get_pitch( void ) {
    return( cur_fdm_state->get_Theta() );
}

double fgAPget_heading( void ) {
    return( cur_fdm_state->get_Psi() * SGD_RADIANS_TO_DEGREES );
}

static inline double fgAPget_altitude( void ) {
    return( cur_fdm_state->get_Altitude() * FEET_TO_METER );
}

static inline double fgAPget_climb( void ) {
    // return in meters per minute
    return( cur_fdm_state->get_Climb_Rate() * FEET_TO_METER * 60 );
}

static inline double get_sideslip( void ) {
    return( cur_fdm_state->get_Beta() );
}

static inline double fgAPget_agl( void ) {
    double agl;

    agl = cur_fdm_state->get_Altitude() * FEET_TO_METER
	- scenery.cur_elev;

    return( agl );
}
#endif


void FGAutopilot::AltitudeSet( double new_altitude ) {
    double target_alt = new_altitude;

    // cout << "new altitude = " << new_altitude << endl;

    if ( fgGetString("/sim/startup/units") == "feet" ) {
	target_alt = new_altitude * FEET_TO_METER;
    }

    if( target_alt < scenery.cur_elev ) {
	target_alt = scenery.cur_elev;
    }

    TargetAltitude = target_alt;
    altitude_mode = FG_ALTITUDE_LOCK;

    // cout << "TargetAltitude = " << TargetAltitude << endl;

    if ( fgGetString("/sim/startup/units") == "feet" ) {
	target_alt *= METER_TO_FEET;
    }
    // ApAltitudeDialogInput->setValue((float)target_alt);
    MakeTargetAltitudeStr( target_alt );
	
    update_old_control_values();
}


void FGAutopilot::AltitudeAdjust( double inc )
{
    double target_alt, target_agl;

    if ( fgGetString("/sim/startup/units") == "feet" ) {
	target_alt = TargetAltitude * METER_TO_FEET;
	target_agl = TargetAGL * METER_TO_FEET;
    } else {
	target_alt = TargetAltitude;
	target_agl = TargetAGL;
    }

    // cout << "target_agl = " << target_agl << endl;
    // cout << "target_agl / inc = " << target_agl / inc << endl;
    // cout << "(int)(target_agl / inc) = " << (int)(target_agl / inc) << endl;

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

    if ( fgGetString("/sim/startup/units") == "feet" ) {
	target_alt *= FEET_TO_METER;
	target_agl *= FEET_TO_METER;
    }

    TargetAltitude = target_alt;
    TargetAGL = target_agl;
	
    if ( fgGetString("/sim/startup/units") == "feet" )
	target_alt *= METER_TO_FEET;
    if ( fgGetString("/sim/startup/units") == "feet" )
	target_agl *= METER_TO_FEET;

    if ( altitude_mode == FG_ALTITUDE_LOCK ) {
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
	heading_mode = FG_DG_HEADING_LOCK;
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
    heading_mode = FG_DG_HEADING_LOCK;
	
    new_heading = NormalizeDegrees( new_heading );
    DGTargetHeading = new_heading;
    // following cast needed ambiguous plib
    // ApHeadingDialogInput -> setValue ((float)APData->TargetHeading );
    MakeTargetHeadingStr( DGTargetHeading );			
    update_old_control_values();
}

void FGAutopilot::AutoThrottleAdjust( double inc ) {
    double target = ( int ) ( TargetSpeed / inc ) * inc + inc;

    TargetSpeed = target;
}


void FGAutopilot::set_AutoThrottleEnabled( bool value ) {
    auto_throttle = value;

    if ( auto_throttle == true ) {
	TargetSpeed = FGBFI::getAirspeed();
	speed_error_accum = 0.0;
    }

    update_old_control_values();
    FG_LOG( FG_COCKPIT, FG_INFO, " fgAPSetAutoThrottle: ("
	    << auto_throttle << ") " << TargetSpeed );
}
