// Module:        10520c.c
//  Author:       Phil Schubert
//  Date started: 12/03/99
//  Purpose:      Models a Continental IO-520-M Engine
//  Called by:    FGSimExec
// 
//  Copyright (C) 1999  Philip L. Schubert (philings@ozemail.com.au)
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
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA.
//
// Further information about the GNU General Public License can also
// be found on the world wide web at http://www.gnu.org.
//
// FUNCTIONAL DESCRIPTION
// ------------------------------------------------------------------------
// Models a Continental IO-520-M engine. This engine is used in Cessna
// 210, 310, Beechcraft Bonaza and Baron C55. The equations used below
// were determined by a first and second order curve fits using Excel. 
// The data is from the Cessna Aircraft Corporations Engine and Flight
// Computer for C310. Part Number D3500-13
// 
// ARGUMENTS
// ------------------------------------------------------------------------
// 
// 
// HISTORY
// ------------------------------------------------------------------------
// 12/03/99   	PLS	Created
// 07/03/99	PLS	Added Calculation of Density, and Prop_Torque
// 07/03/99	PLS	Restructered Variables to allow easier implementation
//			of Classes
// 15/03/99	PLS	Added Oil Pressure, Oil Temperature and CH Temp
// ------------------------------------------------------------------------
// INCLUDES
// ------------------------------------------------------------------------

#include <simgear/compiler.h>

#include <iostream>
#include <math.h>

#include "10520d.hxx"

SG_USING_STD(cout);
SG_USING_STD(endl);

// ------------------------------------------------------------------------
// CODE
// ------------------------------------------------------------------------


// Calculate Engine RPM based on Propellor Lever Position
float FGEngine::Calc_Engine_RPM (float LeverPosition)
{
    // Calculate RPM as set by Prop Lever Position. Assumes engine
    // will run at 1000 RPM at full course
    
    float RPM = LeverPosition * (Max_RPM - Min_RPM) /100 + Min_RPM ;
    
    if ( RPM >= Max_RPM ) {
	RPM = Max_RPM;
    }

    return RPM;
}


// Calculate Manifold Pressure based on Throttle lever Position
static float Calc_Manifold_Pressure ( float LeverPosn, float MaxMan)
{
    float Inches;  
    // if ( x < = 0 ) {
    //   x = 0.00001;
    // }
    Inches = LeverPosn * MaxMan / 100;
    return Inches;
}


// set initial default values
void FGEngine::init() {
    // Control and environment inputs
    IAS = 0;
    Throttle_Lever_Pos = 75;
    Propeller_Lever_Pos = 75;	
    Mixture_Lever_Pos = 100;

    // Engine Specific Variables used by this program that have limits.
    // Will be set in a parameter file to be read in to create
    // and instance for each engine.
    Max_Manifold_Pressure = 29.50;
    Max_RPM = 2700;
    Min_RPM = 1000;
    Max_Fuel_Flow = 130;
    Mag_Derate_Percent = 5;
    MaxHP = 285;
    Gear_Ratio = 1;

    // Initialise Engine Variables used by this instance
    Percentage_Power = 0;
    Manifold_Pressure = 29.00; // Inches
    RPM = 500;
    Fuel_Flow = 0;	// lbs/hour
    Torque = 0;
    CHT = 370;
    Mixture = 14;
    Oil_Pressure = 0;	// PSI
    Oil_Temp = 85;	// Deg C
    HP = 0;
    RPS = 0;
    Torque_Imbalance = 0;
    Desired_RPM = 0;

    // Initialise Propellor Variables used by this instance
    FGProp1_Angular_V = 0;
    FGProp1_Coef_Drag =  0.6;
    FGProp1_Torque = 0;
    FGProp1_Thrust = 0;
    FGProp1_RPS = 0;
    FGProp1_Coef_Lift = 0.1;
    Alpha1 = 13.5;
    FGProp1_Blade_Angle = 13.5;
    FGProp_Fine_Pitch_Stop = 13.5;
    FGProp_Course_Pitch_Stop = 55;

    // Other internal values
    Rho = 0.002378;
}


// Calculate Oil Pressure
static float Oil_Press (float Oil_Temp, float Engine_RPM)
{
    float Oil_Pressure = 0;			//PSI
    float Oil_Press_Relief_Valve = 60;	//PSI
    float Oil_Press_RPM_Max = 1800;
    float Design_Oil_Temp = 85;		//Celsius
    float Oil_Viscosity_Index = 0.25;	// PSI/Deg C
    float Temp_Deviation = 0;		// Deg C

    Oil_Pressure = (Oil_Press_Relief_Valve / Oil_Press_RPM_Max) * Engine_RPM;
	
    // Pressure relief valve opens at Oil_Press_Relief_Valve PSI setting
    if (Oil_Pressure >= Oil_Press_Relief_Valve)	
	{
	    Oil_Pressure = Oil_Press_Relief_Valve;
	}
	
    // Now adjust pressure according to Temp which affects the viscosity
	
    Oil_Pressure += (Design_Oil_Temp - Oil_Temp) * Oil_Viscosity_Index;	
	
    return Oil_Pressure;
}


// Calculate Cylinder Head Temperature
static float Calc_CHT (float Fuel_Flow, float Mixture, float IAS)
{
    float CHT = 350;
	
    return CHT;
}


// Calculate Density Ratio
static float Density_Ratio ( float x )
{
    float y = ((3E-10 * x * x) - (3E-05 * x) + 0.9998);
    return y;
}


// Calculate Air Density - Rho
static float Density ( float x )
{
    float y = ((9E-08 * x * x) - (7E-08 * x) + 0.0024);
    return y;
}


// Calculate Speed in FPS given Knots CAS
static float IAS_to_FPS (float ias)
{
    return ias * 1.68888888;
}


// update the engine model based on current control positions
void FGEngine::update() {
    // Declare local variables
    int num = 0; // Not used. Counting variables
    int num2 = 100;  // Not used.
    float ManXRPM = 0;
    float Vo = 0;
    float V1 = 0;

    // Set up the new variables
    float Blade_Station = 30;
    float Rho =	0.002378;
    float FGProp_Area = 1.405/3;
    float PI = 3.1428571;

    // Input Variables
    // float IAS = 0; 

    // 0 = Closed, 100 = Fully Open
    // float Throttle_Lever_Pos = 75;
    // 0 = Full Course 100 = Full Fine
    // float Propeller_Lever_Pos = 75;	
    // 0 = Idle Cut Off 100 = Full Rich
    // float Mixture_Lever_Pos = 100;

    // Environmental Variables

   // Temp Variation from ISA (Deg F)
    float FG_ISA_VAR = 0;
    // Pressure Altitude  1000's of Feet
    float FG_Pressure_Ht = 0;

    // Parameters that alter the operation of the engine.
    // Yes = 1. Is there Fuel Available. Calculated elsewhere
    int Fuel_Available = 1;
    // Off = 0. Reduces power by 3 % for same throttle setting
    int Alternate_Air_Pos =0;
    // 1 = On.   Reduces power by 5 % for same power lever settings
    int Magneto_Left = 1;
    // 1 = On.  Ditto, Both of the above though do not alter fuel flow
    int Magneto_Right = 1;

    // There needs to be a section in here to trap silly values, like
    // 0, otherwise they will crash the calculations

    // cout << " Number of Iterations ";
    // cin >> num2;
    // cout << endl;

    // cout << " Throttle % ";
    // cin >> Throttle_Lever_Pos;
    // cout << endl;

    // cout << " Prop % ";
    // cin >> Propeller_Lever_Pos;
    // cout << endl;

    //==================================================================
    // Engine & Environmental Inputs from elsewhere

    // Calculate Air Density (Rho) - In FG this is calculated in 
    // FG_Atomoshere.cxx

    Rho = Density(FG_Pressure_Ht); // In FG FG_Pressure_Ht is "h"
    // cout << "Rho = " << Rho << endl;

    // Calculate Manifold Pressure (Engine 1) as set by throttle opening

    Manifold_Pressure = 
	Calc_Manifold_Pressure( Throttle_Lever_Pos, Max_Manifold_Pressure );
    cout << "manifold pressure = " << Manifold_Pressure << endl;

 
    RPM = Calc_Engine_RPM(Propeller_Lever_Pos);
    // cout << "Engine RPM = " << RPM << endl;

    Desired_RPM = RPM;
    cout << "Desired RPM = " << Desired_RPM << endl;

    //==================================================================
    // Engine Power & Torque Calculations

    // Loop until stable - required for testing only
    for (num = 0; num < num2; num++) {
	// cout << endl << "====================" << endl;
	// cout << "MP Inches = " << Manifold_Pressure << "\t";
	// cout << "  RPM = " << RPM << "\t";

	// For a given Manifold Pressure and RPM calculate the % Power
	// Multiply Manifold Pressure by RPM
	ManXRPM = Manifold_Pressure * RPM;
	// cout << ManXRPM << endl;

	//  Calculate % Power
	Percentage_Power = (+ 7E-09 * ManXRPM * ManXRPM) 
	    + ( + 7E-04 * ManXRPM) - 0.1218;
	// cout << "percent power = " << Percentage_Power <<  "%" << "\t";

	// Adjust for Temperature - Temperature above Standard decrease
	// power % by 7/120 per degree F increase, and incease power for
	// temps below at the same ratio
	Percentage_Power = Percentage_Power - (FG_ISA_VAR * 7 /120);
	// cout << " adjusted T = " << Percentage_Power <<  "%" << "\t";
    
	// Adjust for Altitude. In this version a linear variation is
	// used. Decrease 1% for each 1000' increase in Altitde
	Percentage_Power = Percentage_Power + (FG_Pressure_Ht * 12/10000);	
	// cout << " adjusted A = " << Percentage_Power <<  "%" << "\t";

	// Now Calculate Fuel Flow based on % Power Best Power Mixture
	Fuel_Flow = Percentage_Power * Max_Fuel_Flow / 100.0;
	// cout << Fuel_Flow << " lbs/hr"<< endl;
	
	// Now Derate engine for the effects of Bad/Switched off magnetos
	if (Magneto_Left == 0 && Magneto_Right == 0) {
	    // cout << "Both OFF\n";
	    Percentage_Power = 0;
	} else if (Magneto_Left && Magneto_Right) {
	    // cout << "Both On    ";
	} else if (Magneto_Left == 0 || Magneto_Right== 0) {
	    // cout << "1 Magneto Failed   ";

	    Percentage_Power = Percentage_Power * 
		((100.0 - Mag_Derate_Percent)/100.0);
	}	
	// cout << "Final engine % power = " <<  Percentage_Power << "%" << endl;

	// Calculate Engine Horsepower

	HP = Percentage_Power * MaxHP / 100.0;

	// Calculate Engine Torque

	Torque = HP * 5252 / RPM;
	// cout << Torque << "Ft/lbs" << "\t";

	// Calculate Cylinder Head Temperature
	CHT = Calc_CHT( Fuel_Flow, Mixture, IAS);
	// cout << "Cylinder Head Temp (F) = " << CHT << endl;

	// Calculate Oil Pressure
	Oil_Pressure = Oil_Press( Oil_Temp, RPM );
	// cout << "Oil Pressure (PSI) = " << Oil_Pressure << endl;
	
	//==============================================================

	// Now do the Propellor Calculations

	// Revs per second
	FGProp1_RPS = RPM * Gear_Ratio / 60.0;
	// cout << FGProp1_RPS << " RPS" <<  endl;

	//Radial Flow Vector (V2) Ft/sec at Ref Blade Station (usually 30")
	FGProp1_Angular_V = FGProp1_RPS * 2 * PI * (Blade_Station / 12);
	// cout << "Angular Velocity " << FGProp1_Angular_V << endl;

	// Axial Flow Vector (Vo) Ft/sec
	// Some further work required here to allow for inflow at low speeds
	// Vo = (IAS + 20) * 1.688888;
	Vo = IAS_to_FPS(IAS + 20);
	// cout << "Feet/sec = " << Vo << endl;

	// cout << Vo << "Axial Velocity" << endl;

	// Relative Velocity (V1)
	V1 = sqrt((FGProp1_Angular_V * FGProp1_Angular_V) +
		  (Vo * Vo));
	// cout << "Relative Velocity " << V1 << endl;

	if ( FGProp1_Blade_Angle >= FGProp_Course_Pitch_Stop )  {
	    FGProp1_Blade_Angle = FGProp_Course_Pitch_Stop;
	}

	// cout << FGProp1_Blade_Angle << " Prop Blade Angle" << endl;

	// Blade Angle of Attack (Alpha1)

	Alpha1 = FGProp1_Blade_Angle -(atan(Vo / FGProp1_Angular_V) * (180/PI));
	// cout << Alpha1 << " Alpha1" << endl;

	// cout << "  Alpha1 = " << Alpha1
	//      << "  Blade angle = " << FGProp1_Blade_Angle
	//      << "  Vo = " << Vo
	//      << "  FGProp1_Angular_V = " << FGProp1_Angular_V << endl;

	// Calculate Coefficient of Drag at Alpha1
	FGProp1_Coef_Drag = (0.0005 * (Alpha1 * Alpha1)) + (0.0003 * Alpha1)
	    + 0.0094;
	//	cout << FGProp1_Coef_Drag << " Coef Drag" << endl;

	// Calculate Coefficient of Lift at Alpha1
	FGProp1_Coef_Lift = -(0.0026 * (Alpha1 * Alpha1)) + (0.1027 * Alpha1)
	    + 0.2295;
	// cout << FGProp1_Coef_Lift << " Coef Lift " << endl;

	// Covert Alplha1 to Radians
	// Alpha1 = Alpha1 * PI / 180;

	//  Calculate Prop Torque
	FGProp1_Torque = (0.5 * Rho * (V1 * V1) * FGProp_Area
			  * ((FGProp1_Coef_Lift * sin(Alpha1 * PI / 180))
			     + (FGProp1_Coef_Drag * cos(Alpha1 * PI / 180))))
	    * (Blade_Station/12);
	// cout << "Prop Torque = " << FGProp1_Torque << endl;

	//  Calculate Prop Thrust
	// cout << "  V1 = " << V1 << "  Alpha1 = " << Alpha1 << endl;
	FGProp1_Thrust = 0.5 * Rho * (V1 * V1) * FGProp_Area
	    * ((FGProp1_Coef_Lift * cos(Alpha1 * PI / 180))
	       - (FGProp1_Coef_Drag * sin(Alpha1 * PI / 180)));
	// cout << " Prop Thrust = " << FGProp1_Thrust <<  endl;

	// End of Propeller Calculations   
	//==============================================================



	Torque_Imbalance = FGProp1_Torque - Torque; 
	//  cout <<  Torque_Imbalance << endl;

	if (Torque_Imbalance > 20) {
	    RPM -= 14.5;
	    // FGProp1_RPM -= 25;
	    FGProp1_Blade_Angle -= 0.75;
	}

	if (FGProp1_Blade_Angle < FGProp_Fine_Pitch_Stop) {
	    FGProp1_Blade_Angle = FGProp_Fine_Pitch_Stop;
	}
	if (Torque_Imbalance < -20) {
	    RPM += 14.5;
	    // FGProp1_RPM += 25;
	    FGProp1_Blade_Angle += 0.75;
	}

	if (RPM >= 2700) {
	    RPM = 2700;
	}


	// cout << FGEng1_RPM << " Blade_Angle  " << FGProp1_Blade_Angle << endl << endl;

    }
}




// Functions

// Calculate Oil Temperature

static float Oil_Temp (float Fuel_Flow, float Mixture, float IAS)
{
    float Oil_Temp = 85;
	
    return (Oil_Temp);
}
