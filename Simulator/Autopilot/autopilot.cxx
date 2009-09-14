// autopilot.cxx -- autopilot subsystem
//
// Written by Jeff Goeke-Smith, started April 1998.
//
// Copyright (C) 1998  Jeff Goeke-Smith, jgoeke@voyager.net
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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <assert.h>
#include <stdlib.h>

#include <Scenery/scenery.hxx>

#include "autopilot.hxx"

#include <Include/fg_constants.h>
#include <Debug/logstream.hxx>
#include <Main/options.hxx>


// The below routines were copied right from hud.c ( I hate reinventing
// the wheel more than necessary)

// The following routines obtain information concerntin the aircraft's
// current state and return it to calling instrument display routines.
// They should eventually be member functions of the aircraft.
//


static double get_speed( void )
{
    return( current_aircraft.fdm_state->get_V_equiv_kts() );
}

static double get_aoa( void )
{
    return( current_aircraft.fdm_state->get_Gamma_vert_rad() * RAD_TO_DEG );
}

static double fgAPget_roll( void )
{
    return( current_aircraft.fdm_state->get_Phi() * RAD_TO_DEG );
}

static double get_pitch( void )
{
    return( current_aircraft.fdm_state->get_Theta() );
}

double fgAPget_heading( void )
{
    return( current_aircraft.fdm_state->get_Psi() * RAD_TO_DEG );
}

static double fgAPget_altitude( void )
{
    return( current_aircraft.fdm_state->get_Altitude() * FEET_TO_METER );
}

static double fgAPget_climb( void )
{
    // return in meters per minute
    return( current_aircraft.fdm_state->get_Climb_Rate() * FEET_TO_METER * 60 );
}

static double get_sideslip( void )
{
    return( current_aircraft.fdm_state->get_Beta() );
}

static double fgAPget_agl( void )
{
        double agl;

        agl = current_aircraft.fdm_state->get_Altitude() * FEET_TO_METER
	    - scenery.cur_elev;

        return( agl );
}

// End of copied section.  ( thanks for the wheel :-)

// Local Prototype section

double LinearExtrapolate( double x,double x1, double y1, double x2, double y2);
double NormalizeDegrees( double Input);

// End Local ProtoTypes

fgAPDataPtr APDataGlobal; 	// global variable holding the AP info
				// I want this gone.  Data should be in aircraft structure


bool fgAPHeadingEnabled( void )
{
    fgAPDataPtr APData;
	
    APData = APDataGlobal;
	        
    // heading hold enabled?
    return APData->heading_hold;
}

bool fgAPAltitudeEnabled( void )
{
    fgAPDataPtr APData;
	
    APData = APDataGlobal;
	        
    // altitude hold or terrain follow enabled?
    return APData->altitude_hold || APData->terrain_follow ;
}

bool fgAPAutoThrottleEnabled( void )
{
    fgAPDataPtr APData;
	
    APData = APDataGlobal;

    // autothrottle enabled?
    return APData->auto_throttle;
}

void fgAPAltitudeAdjust( double inc )
{
    // Remove at a later date
    fgAPDataPtr APData;
    APData = APDataGlobal;
    // end section

    double target_alt, target_agl;

    if ( current_options.get_units() == fgOPTIONS::FG_UNITS_FEET ) {
	target_alt = APData->TargetAltitude * METER_TO_FEET;
	target_agl = APData->TargetAGL * METER_TO_FEET;
    } else {
	target_alt = APData->TargetAltitude;
	target_agl = APData->TargetAGL;
    }

    target_alt = (int)(target_alt / inc) * inc + inc;
    target_agl = (int)(target_agl / inc) * inc + inc;

    if ( current_options.get_units() == fgOPTIONS::FG_UNITS_FEET ) {
	target_alt *= FEET_TO_METER;
	target_agl *= FEET_TO_METER;
    }

    APData->TargetAltitude = target_alt;
    APData->TargetAGL = target_agl;
}

void fgAPHeadingAdjust( double inc )
{
    fgAPDataPtr APData;
    APData = APDataGlobal;

    double target = (int)(APData->TargetHeading / inc) * inc + inc;

    APData->TargetHeading = NormalizeDegrees(target);
}

void fgAPAutoThrottleAdjust( double inc )
{
    fgAPDataPtr APData;
    APData = APDataGlobal;

    double target = (int)(APData->TargetSpeed / inc) * inc + inc;

    APData->TargetSpeed = target;
}

void fgAPInit( fgAIRCRAFT *current_aircraft )
{
	fgAPDataPtr APData ;

	FG_LOG( FG_AUTOPILOT, FG_INFO, "Init AutoPilot Subsystem" );

	APData  = (fgAPDataPtr)calloc(sizeof(fgAPData),1);
	
	if (APData == NULL) {
	    // I couldn't get the mem.  Dying
	    FG_LOG( FG_AUTOPILOT, FG_ALERT, "No ram for Autopilot. Dying.");
	    exit(-1);
	}
		
	APData->heading_hold = false ;     // turn the heading hold off
	APData->altitude_hold = false ;    // turn the altitude hold off

	APData->TargetHeading = 0.0;    // default direction, due north
	APData->TargetAltitude = 3000;  // default altitude in meters
	APData->alt_error_accum = 0.0;

	// These eventually need to be read from current_aircaft somehow.

#if 0
	// Original values
	// the maximum roll, in Deg
	APData->MaxRoll = 7;
	// the deg from heading to start rolling out at, in Deg
	APData->RollOut = 30;
	// how far can I move the aleron from center.
	APData->MaxAileron= .1;
	// Smoothing distance for alerion control
	APData->RollOutSmooth = 10;
#endif

	// the maximum roll, in Deg
	APData->MaxRoll = 20;

	// the deg from heading to start rolling out at, in Deg
	APData->RollOut = 20;

	// how far can I move the aleron from center.
	APData->MaxAileron= .2;

	// Smoothing distance for alerion control
	APData->RollOutSmooth = 10;

	//Remove at a later date
	APDataGlobal = APData;
	
};

int fgAPRun( void )
{
    // Remove the following lines when the calling funcitons start
    // passing in the data pointer

    fgAPDataPtr APData;
	
    APData = APDataGlobal;
    // end section
	        
    // heading hold enabled?
    if ( APData->heading_hold == true ) {
	double RelHeading;
	double TargetRoll;
	double RelRoll;
	double AileronSet;
		
	RelHeading =  
	    NormalizeDegrees( APData->TargetHeading - fgAPget_heading());
	// figure out how far off we are from desired heading
		  
	// Now it is time to deterime how far we should be rolled.
	FG_LOG( FG_AUTOPILOT, FG_DEBUG, "RelHeading: " << RelHeading );
		
		
	// Check if we are further from heading than the roll out point
	if ( fabs(RelHeading) > APData->RollOut ) {
	    // set Target Roll to Max in desired direction
	    if (RelHeading < 0 ) {
		TargetRoll = 0-APData->MaxRoll;
	    } else {
		TargetRoll = APData->MaxRoll;
	    }
	} else {
	    // We have to calculate the Target roll

	    // This calculation engine thinks that the Target roll
	    // should be a line from (RollOut,MaxRoll) to (-RollOut,
	    // -MaxRoll) I hope this works well.  If I get ambitious
	    // some day this might become a fancier curve or
	    // something.

	    TargetRoll = LinearExtrapolate( RelHeading, -APData->RollOut,
					    -APData->MaxRoll, APData->RollOut,
					    APData->MaxRoll );
	}
		
	// Target Roll has now been Found.

	// Compare Target roll to Current Roll, Generate Rel Roll

	FG_LOG( FG_COCKPIT, FG_BULK, "TargetRoll: " << TargetRoll );
		
	RelRoll = NormalizeDegrees(TargetRoll - fgAPget_roll());

	// Check if we are further from heading than the roll out smooth point
	if ( fabs(RelRoll) > APData->RollOutSmooth ) {
	    // set Target Roll to Max in desired direction
	    if (RelRoll < 0 ) {
		AileronSet = 0-APData->MaxAileron;
	    } else {
		AileronSet = APData->MaxAileron;
	    }
	} else {
	    AileronSet = LinearExtrapolate( RelRoll, -APData->RollOutSmooth,
					    -APData->MaxAileron,
					    APData->RollOutSmooth,
					    APData->MaxAileron );
	}
		
	controls.set_aileron( AileronSet );
	controls.set_rudder( 0.0 );
    }

    // altitude hold or terrain follow enabled?
    if ( APData->altitude_hold || APData->terrain_follow ) {
	double speed, max_climb, error;
	double prop_error, int_error;
	double prop_adj, int_adj, total_adj;

	if ( APData->altitude_hold ) {
	    // normal altitude hold
	    APData->TargetClimbRate = 
		(APData->TargetAltitude - fgAPget_altitude()) * 8.0;
	} else if ( APData->terrain_follow ) {
	    // brain dead ground hugging with no look ahead
	    APData->TargetClimbRate = 
		( APData->TargetAGL - fgAPget_agl() ) * 16.0;
	} else {
	    // just try to zero out rate of climb ...
	    APData->TargetClimbRate = 0.0;
	}

	speed = get_speed();

	if ( speed < 90.0 ) {
	    max_climb = 0.0;
	} else if ( speed < 100.0 ) {
	    max_climb = (speed - 90.0) * 20;
	} else {
	    max_climb = ( speed - 100.0 ) * 4.0 + 200.0;
	}

	if ( APData->TargetClimbRate > max_climb ) {
	    APData->TargetClimbRate = max_climb;
	}

	if ( APData->TargetClimbRate < -400.0 ) {
	    APData->TargetClimbRate = -400.0;
	}

	error = fgAPget_climb() - APData->TargetClimbRate;

	// accumulate the error under the curve ... this really should
	// be *= delta t
	APData->alt_error_accum += error;

	// calculate integral error, and adjustment amount
	int_error = APData->alt_error_accum;
	// printf("error = %.2f  int_error = %.2f\n", error, int_error);
	int_adj = int_error / 8000.0;
	
	// caclulate proportional error
	prop_error = error;
	prop_adj = prop_error / 2000.0;

	total_adj = 0.9 * prop_adj + 0.1 * int_adj;
	if ( total_adj >  0.6 ) { total_adj =  0.6; }
	if ( total_adj < -0.2 ) { total_adj = -0.2; }

	controls.set_elevator( total_adj );
    }

    // auto throttle enabled?
    if ( APData->auto_throttle ) {
	double error;
	double prop_error, int_error;
	double prop_adj, int_adj, total_adj;

	error = APData->TargetSpeed - get_speed();

	// accumulate the error under the curve ... this really should
	// be *= delta t
	APData->speed_error_accum += error;
	if ( APData->speed_error_accum > 2000.0 ) {
	    APData->speed_error_accum = 2000.0;
	}
	if ( APData->speed_error_accum < -2000.0 ) {
	    APData->speed_error_accum = -2000.0;
	}

	// calculate integral error, and adjustment amount
	int_error = APData->speed_error_accum;

	// printf("error = %.2f  int_error = %.2f\n", error, int_error);
	int_adj = int_error / 200.0;
	
	// caclulate proportional error
	prop_error = error;
	prop_adj = 0.5 + prop_error / 50.0;

	total_adj = 0.9 * prop_adj + 0.1 * int_adj;
	if ( total_adj > 1.0 ) { total_adj = 1.0; }
	if ( total_adj < 0.0 ) { total_adj = 0.0; }

	controls.set_throttle( FGControls::ALL_ENGINES, total_adj );
    }

     /*
    if (APData->Mode == 2) // Glide slope hold
    {
	double RelSlope;
	double RelElevator;
		
	// First, calculate Relative slope and normalize it
	RelSlope = NormalizeDegrees( APData->TargetSlope - get_pitch());
	
	// Now calculate the elevator offset from current angle
	if ( abs(RelSlope) > APData->SlopeSmooth )
	{
	    if ( RelSlope < 0 )		//  set RelElevator to max in the correct direction
		RelElevator = -APData->MaxElevator;
	    else
		RelElevator = APData->MaxElevator;
	}
		
	else
	    RelElevator = LinearExtrapolate(RelSlope,-APData->SlopeSmooth,-APData->MaxElevator,APData->SlopeSmooth,APData->MaxElevator);
		
	// set the elevator
	fgElevMove(RelElevator);
		
    }
    */
	
    // Ok, we are done
    return 0;

}

/*
void fgAPSetMode( int mode)
{
    //Remove the following line when the calling funcitons start passing in the data pointer
    fgAPDataPtr APData;
	 
    APData = APDataGlobal;
    // end section
	 
    fgPrintf( FG_COCKPIT, FG_INFO, "APSetMode : %d\n", mode );
	 
    APData->Mode = mode;  // set the new mode
}
*/

void fgAPToggleHeading( void )
{
    // Remove at a later date
    fgAPDataPtr APData;

    APData = APDataGlobal;
    // end section

    if ( APData->heading_hold ) {
	// turn off heading hold
	APData->heading_hold = false;
    } else {
	// turn on heading hold, lock at current heading
	APData->heading_hold = true;
	APData->TargetHeading = fgAPget_heading();
    }

    FG_LOG( FG_COCKPIT, FG_INFO, " fgAPSetHeading: (" 
	    << APData->heading_hold << ") " << APData->TargetHeading );
}
	         

void fgAPToggleAltitude( void )
{
    // Remove at a later date
    fgAPDataPtr APData;

    APData = APDataGlobal;
    // end section

    if ( APData->altitude_hold ) {
	// turn off altitude hold
	APData->altitude_hold = false;
    } else {
	// turn on altitude hold, lock at current altitude
	APData->altitude_hold = true;
	APData->terrain_follow = false;
	APData->TargetAltitude = fgAPget_altitude();
	APData->alt_error_accum = 0.0;
	// alt_error_queue.erase( alt_error_queue.begin(), 
	//                        alt_error_queue.end() );
    }

    FG_LOG( FG_COCKPIT, FG_INFO, " fgAPSetAltitude: (" 
	    << APData->altitude_hold << ") " << APData->TargetAltitude );
}
	         

void fgAPToggleAutoThrottle ( void )
{
    // Remove at a later date
    fgAPDataPtr APData;

    APData = APDataGlobal;
    // end section

    if ( APData->auto_throttle ) {
	// turn off altitude hold
	APData->auto_throttle = false;
    } else {
	// turn on terrain follow, lock at current agl
	APData->auto_throttle = true;
	APData->TargetSpeed = get_speed();
	APData->speed_error_accum = 0.0;
    }

    FG_LOG( FG_COCKPIT, FG_INFO, " fgAPSetAutoThrottle: (" 
	    << APData->auto_throttle << ") " << APData->TargetSpeed );
}

void fgAPToggleTerrainFollow( void )
{
    // Remove at a later date
    fgAPDataPtr APData;

    APData = APDataGlobal;
    // end section

    if ( APData->terrain_follow ) {
	// turn off altitude hold
	APData->terrain_follow = false;
    } else {
	// turn on terrain follow, lock at current agl
	APData->terrain_follow = true;
	APData->altitude_hold = false;
	APData->TargetAGL = fgAPget_agl();
	APData->alt_error_accum = 0.0;
    }

    FG_LOG( FG_COCKPIT, FG_INFO, " fgAPSetTerrainFollow: ("
	    << APData->terrain_follow << ") " << APData->TargetAGL );
}

double LinearExtrapolate( double x,double x1,double y1,double x2,double y2)
{
    // This procedure extrapolates the y value for the x posistion on a line defined by x1,y1; x2,y2
    //assert(x1 != x2); // Divide by zero error.  Cold abort for now
    
    double m, b, y; 		// the constants to find in y=mx+b
	
    m=(y2-y1)/(x2-x1);	// calculate the m
	
    b= y1- m * x1;		// calculate the b
	
    y = m * x + b;		// the final calculation
	
    return (y);
	
};

double NormalizeDegrees(double Input)
{
    // normalize the input to the range (-180,180]
    // Input should not be greater than -360 to 360.  Current rules send the output to an undefined state.
    if (Input > 180)
	Input -= 360;
    if (Input <= -180)
	Input += 360;
    
    return (Input);
};


// $Log$
// Revision 1.15  1999/02/12 23:22:35  curt
// Allow auto-throttle adjustment while active.
//
// Revision 1.14  1999/02/12 22:17:14  curt
// Changes contributed by Norman Vine to allow adjustment of the autopilot
// while it is activated.
//
