/**************************************************************************
 * autopilot.cxx -- autopilot subsystem
 *
 * Written by Jeff Goeke-Smith, started April 1998.
 *
 * Copyright (C) 1998  Jeff Goeke-Smith, jgoeke@voyager.net
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
 *
 **************************************************************************/


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <assert.h>
#include <stdlib.h>

// #include <list>
// #include <Include/fg_stl_config.h>

#include <Scenery/scenery.hxx>

// #ifdef NEEDNAMESPACESTD
// using namespace std;
// #endif

#include "autopilot.hxx"

#include <Include/fg_constants.h>
#include <Debug/fg_debug.h>


// static list < double > alt_error_queue;


// The below routines were copied right from hud.c ( I hate reinventing
// the wheel more than necessary)

// The following routines obtain information concerntin the aircraft's
// current state and return it to calling instrument display routines.
// They should eventually be member functions of the aircraft.
//

static double get_throttleval( void )
{
	fgCONTROLS *pcontrols;

  pcontrols = current_aircraft.controls;
  return pcontrols->throttle[0];     // Hack limiting to one engine
}

static double get_aileronval( void )
{
	fgCONTROLS *pcontrols;

  pcontrols = current_aircraft.controls;
  return pcontrols->aileron;
}

static double get_elevatorval( void )
{
	fgCONTROLS *pcontrols;

  pcontrols = current_aircraft.controls;
  return pcontrols->elevator;
}

static double get_elev_trimval( void )
{
	fgCONTROLS *pcontrols;

  pcontrols = current_aircraft.controls;
  return pcontrols->elevator_trim;
}

static double get_rudderval( void )
{
	fgCONTROLS *pcontrols;

  pcontrols = current_aircraft.controls;
  return pcontrols->rudder;
}

static double get_speed( void )
{
	fgFLIGHT *f;

	f = current_aircraft.flight;
	return( FG_V_equiv_kts );    // Make an explicit function call.
}

static double get_aoa( void )
{
	fgFLIGHT *f;
              
	f = current_aircraft.flight;
	return( FG_Gamma_vert_rad * RAD_TO_DEG );
}

static double fgAPget_roll( void )
{
	fgFLIGHT *f;

	f = current_aircraft.flight;
	return( FG_Phi * RAD_TO_DEG );
}

static double get_pitch( void )
{
	fgFLIGHT *f;
              
	f = current_aircraft.flight;
	return( FG_Theta );
}

double fgAPget_heading( void )
{
	fgFLIGHT *f;

	f = current_aircraft.flight;
	return( FG_Psi * RAD_TO_DEG );
}

static double fgAPget_altitude( void )
{
	fgFLIGHT *f;

	f = current_aircraft.flight;

	return( FG_Altitude * FEET_TO_METER /* -rough_elev */ );
}

static double fgAPget_climb( void )
{
	fgFLIGHT *f;

	f = current_aircraft.flight;

	// return in meters per minute
	return( FG_Climb_Rate * FEET_TO_METER * 60 );
}

static double get_sideslip( void )
{
        fgFLIGHT *f;
        
        f = current_aircraft.flight;
        
        return( FG_Beta );
}

static double fgAPget_agl( void )
{
        fgFLIGHT *f;
        double agl;

        f = current_aircraft.flight;
        agl = FG_Altitude * FEET_TO_METER - scenery.cur_elev;

        return( agl );
}

// End of copied section.  ( thanks for the wheel :-)

// Local Prototype section

double LinearExtrapolate( double x,double x1, double y1, double x2, double y2);
double NormalizeDegrees( double Input);

// End Local ProtoTypes

fgAPDataPtr APDataGlobal; 	// global variable holding the AP info
				// I want this gone.  Data should be in aircraft structure



void fgAPInit( fgAIRCRAFT *current_aircraft )
{
	fgAPDataPtr APData ;

	fgPrintf( FG_AUTOPILOT, FG_INFO, "Init AutoPilot Subsystem\n" );

	APData  = (fgAPDataPtr)calloc(sizeof(fgAPData),1);
	
	if (APData == NULL) // I couldn't get the mem.  Dying
		fgPrintf( FG_AUTOPILOT, FG_EXIT,"No ram for Autopilot. Dying.\n");
		
	APData->heading_hold = 0 ;     // turn the heading hold off
	APData->altitude_hold = 0 ;    // turn the altitude hold off

	APData->TargetHeading = 0.0;    // default direction, due north
	APData->TargetAltitude = 3000;  // default altitude in meters
	APData->alt_error_accum = 0.0;

	// These eventually need to be read from current_aircaft somehow.
	
	APData->MaxRoll = 7;		// the maximum roll, in Deg
	APData->RollOut = 30;		// the deg from heading to start rolling out at, in Deg
	APData->MaxAileron= .1;		// how far can I move the aleron from center.
	APData->RollOutSmooth = 10;	// Smoothing distance for alerion control
	
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
    if ( APData->heading_hold == 1 ) {
	double RelHeading;
	double TargetRoll;
	double RelRoll;
	double AileronSet;
		
	RelHeading =  
	    NormalizeDegrees( APData->TargetHeading - fgAPget_heading());
	// figure out how far off we are from desired heading
		  
	// Now it is time to deterime how far we should be rolled.
	fgPrintf( FG_AUTOPILOT, FG_DEBUG, "RelHeading: %f\n", RelHeading);
		
		
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

	fgPrintf( FG_COCKPIT, FG_BULK, "TargetRoll: %f\n", TargetRoll);
		
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
		
	fgAileronSet(AileronSet);
	fgRudderSet(0.0);
    }

    // altitude hold enabled?
    if ( APData->altitude_hold == 1 ) {
	double error;
	double prop_error, int_error;
	double prop_adj, int_adj, total_adj;

	// normal altitude hold
	APData->TargetClimbRate = 
	    (APData->TargetAltitude - fgAPget_altitude()) * 8.0;
	
	// brain dead ground hugging with no look ahead
	// APData->TargetClimbRate = ( 250 - fgAPget_agl() ) * 8.0;

	// just try to zero out rate of climb ...
	// APData->TargetClimbRate = 0.0;

	if ( APData->TargetClimbRate > 200.0 ) {
	    APData->TargetClimbRate = 200.0;
	}
	if ( APData->TargetClimbRate < -200.0 ) {
	    APData->TargetClimbRate = -200.0;
	}

	error = fgAPget_climb() - APData->TargetClimbRate;

	// push current error onto queue and add into accumulator
	// alt_error_queue.push_back(error);
	APData->alt_error_accum += error;

	// if queue size larger than 60 ... pop front and subtract
	// from accumulator
	// size = alt_error_queue.size();
	// if ( size > 300 ) {
	    // APData->alt_error_accum -= alt_error_queue.front();
	    // alt_error_queue.pop_front();
	    // size--;
	// }

	// calculate integral error, and adjustment amount
	int_error = APData->alt_error_accum;
	// printf("error = %.2f  int_error = %.2f\n", error, int_error);
	int_adj = int_error / 8000.0;
	
	// caclulate proportional error
	prop_error = error;
	prop_adj = prop_error / 2000.0;

	total_adj = 0.9 * prop_adj + 0.1 * int_adj;
	if ( total_adj >  0.4 ) { total_adj =  0.4; }
	if ( total_adj < -0.3 ) { total_adj = -0.3; }

	fgElevSet( total_adj );
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
	APData->heading_hold = 0;
    } else {
	// turn on heading hold, lock at current heading
	APData->heading_hold = 1;
	APData->TargetHeading = fgAPget_heading();
    }

    fgPrintf( FG_COCKPIT, FG_INFO, " fgAPSetHeading: (%d) %.2f\n",
	      APData->heading_hold,
	      APData->TargetHeading);
}
	         

void fgAPToggleAltitude( void )
{
    // Remove at a later date
    fgAPDataPtr APData;

    APData = APDataGlobal;
    // end section

    if ( APData->altitude_hold ) {
	// turn off altitude hold
	APData->altitude_hold = 0;
    } else {
	// turn on altitude hold, lock at current altitude
	APData->altitude_hold = 1;
	APData->TargetAltitude = fgAPget_altitude();
	APData->alt_error_accum = 0.0;
	// alt_error_queue.erase( alt_error_queue.begin(), 
	//                        alt_error_queue.end() );
    }

    fgPrintf( FG_COCKPIT, FG_INFO, " fgAPSetAltitude: (%d) %.2f\n",
	      APData->altitude_hold,
	      APData->TargetAltitude);
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
