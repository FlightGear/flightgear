/**************************************************************************
 * autopilot.c -- autopilot subsystem
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

#include "autopilot.h"

#include <Include/fg_constants.h>
#include <Debug/fg_debug.h>


// The below routines were copied right from hud.c ( I hate reinventing
// the wheel more than necessary)
//// The following routines obtain information concerntin the aircraft's
//// current state and return it to calling instrument display routines.
//// They should eventually be member functions of the aircraft.
////

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

static double get_altitude( void )
{
	fgFLIGHT *f;
	// double rough_elev;

	f = current_aircraft.flight;
	// rough_elev = mesh_altitude(FG_Longitude * RAD_TO_ARCSEC,
	//		                   FG_Latitude  * RAD_TO_ARCSEC);

	return( FG_Altitude * FEET_TO_METER /* -rough_elev */ );
}

static double get_sideslip( void )
{
        fgFLIGHT *f;
        
        f = current_aircraft.flight;
        
        return( FG_Beta );
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
		
	APData->Mode = 0 ; 		// turn the AP off
	APData->Heading = 0.0; 		// default direction, due north
	
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
	
	//Remove the following lines when the calling funcitons start passing in the data pointer
	fgAPDataPtr APData;
	
	APData = APDataGlobal;
	// end section
	        
	if (APData->Mode == 0) // the autopilot is shut off
		return 0 ;
		
	if (APData->Mode == 1) // heading hold mode
		{
		double RelHeading;
		double TargetRoll;
		double RelRoll;
		double AileronSet;
		
		RelHeading =  NormalizeDegrees( APData->Heading - fgAPget_heading());
		  // figure out how far off we are from desired heading
		  
		// Now it is time to deterime how far we should be rolled.
		fgPrintf( FG_AUTOPILOT, FG_DEBUG, "RelHeading: %f\n", RelHeading);
		
		
		if ( abs(RelHeading) > APData->RollOut )  // We are further from heading than the roll out point
			{
			if (RelHeading < 0 )		  // set Target Roll to Max in desired direction
				TargetRoll = 0-APData->MaxRoll;
			else
				TargetRoll = APData->MaxRoll;
			}
		else						// We have to calculate the Target roll
			{
			/*
			* This calculation engine thinks that the Target roll should be a line from (RollOut,MaxRoll) to 
			* (-RollOut, -MaxRoll)  I hope this works well.  If I get ambitious some day this might become a 
			* fancier curve or something.
			*/
			TargetRoll = LinearExtrapolate(RelHeading,-APData->RollOut,-APData->MaxRoll,APData->RollOut,APData->MaxRoll);
			};
		
		// Target Roll has now been Found.
		
		// Compare Target roll to Current Roll, Generate Rel Roll
		fgPrintf( FG_COCKPIT, FG_BULK, "TargetRoll: %f\n", TargetRoll);
		
		RelRoll = NormalizeDegrees(TargetRoll - fgAPget_roll());
		                                                                 
		if ( abs(RelRoll) > APData->RollOutSmooth )  // We are further from heading than the roll out smooth point
		{
			if (RelRoll < 0 )              // set Target Roll to Max in desired direction
				AileronSet = 0-APData->MaxAileron;
			else
				AileronSet = APData->MaxAileron;
		}
		
		else
			AileronSet = LinearExtrapolate(RelRoll,-APData->RollOutSmooth,-APData->MaxAileron,APData->RollOutSmooth,APData->MaxAileron);
		
		fgAileronSet(AileronSet);
		
		//Cool, it is done.
		return 0;
		}
	
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
	
	//every thing else failed.  Not in a valid autopilot mode
	return -1;

}

void fgAPSetMode( int mode)
{
	 //Remove the following line when the calling funcitons start passing in the data pointer
	 fgAPDataPtr APData;
	 
	 APData = APDataGlobal;
	 // end section
	 
	 fgPrintf( FG_COCKPIT, FG_INFO, "APSetMode : %d\n", mode );
	 
	 APData->Mode = mode;  // set the new mode
	 
}

void fgAPSetHeading( double Heading)
{
	// Set the heading for the autopilot subsystem
	// Special Magic Number:
	//			-1= Set Heading To Current Heading, Define equivilent AP_CURRENT_HEADING
	
	// Remove at a later date
	fgAPDataPtr APData;

	APData = APDataGlobal;
	// end section
	
	if (Heading == AP_CURRENT_HEADING) {
	    APData->Heading = fgAPget_heading();
	} else {
	    APData->Heading = Heading;
	}

	fgPrintf( FG_COCKPIT, FG_INFO, " fgAPSetHeading : %f\n", 
		  APData->Heading);
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
