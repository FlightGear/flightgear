// Module:        10520c.c
//  Author:       Phil Schubert
//  Date started: 12/03/99
//  Purpose:      Models a Continental IO-520-M Engine
//  Called by:    FGSimExec
// 
//  Copyright (C) 1999  Philip L. Schubert (philip@zedley.com)
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
// 15/03/99	PLS	Added Oil Pressure
// 19/8/2000               PLS     Updated E-mail address - This version compiles
//  19/8/2000              PLS          Set Max Prop blade angle to prevent prop exeeding 90
// ------------------------------------------------------------------------
// INCLUDES
// ------------------------------------------------------------------------

#include <simgear/compiler.h>

#include <iostream>
#include <math.h>

SG_USING_STD(cout);
SG_USING_STD(endl);

// ------------------------------------------------------------------------
// CODE
// ------------------------------------------------------------------------

// prototype definitions
// These should be in a header file 10520c.h

float Density (float x);
void  ShowRho (float x);

float IAS_to_FPS (float x);
void ShowFPS(float x);

float Get_Throttle (float x);
void Show_Throttle (float x);

float Manifold_Pressure (float x, float z);
void Show_Manifold_Pressure (float x);

float CHT (float Fuel_Flow, float Mixture, float IAS);
void Show_CHT (float x);

float Oil_Temp (float x, float y);
void Show_Oil_Temp (float x);

float Oil_Press (float Oil_Temp, float Engine_RPM);
void Show_Oil_Press (float x);

int main()

{
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
    float IAS = 0; 
    cout << "Enter IAS  ";
    // cin  >> IAS;
    IAS = 85;
    cout << endl;
 

    // 0 = Closed, 100 = Fully Open
    float FGEng1_Throttle_Lever_Pos = 75;
    // 0 = Full Course 100 = Full Fine
    float FGEng1_Propeller_Lever_Pos = 75;	
    // 0 = Idle Cut Off 100 = Full Rich
    float FGEng1_Mixture_Lever_Pos = 100;

    // Environmental Variables

    // Temp Variation from ISA (Deg F)
    float FG_ISA_VAR = 0;
    // Pressure Altitude  1000's of Feet
    float FG_Pressure_Ht = 0;

    // Parameters that alter the operation of the engine.
    // Yes = 1. Is there Fuel Available. Calculated elsewhere
    int FGEng1_Fuel_Available = 1;
    // Off = 0. Reduces power by 3 % for same throttle setting
    int FGEng1_Alternate_Air_Pos =0;
    // 1 = On.   Reduces power by 5 % for same power lever settings
    int FGEng1_Magneto_Left = 1;
    // 1 = On.  Ditto, Both of the above though do not alter fuel flow
    int FGEng1_Magneto_Right = 1;

    // There needs to be a section in here to trap silly values, like
    // 0, otherwise they will crash the calculations

    //  Engine Specific Variables used by this program that have limits.
    //  Will be set in a parameter file to be read in to create
    //  and instance for each engine.
    float FGEng_Max_Manifold_Pressure = 29.50;
    float FGEng_Max_RPM = 2700;
    float FGEng_Min_RPM = 1000;
    float FGEng_Max_Fuel_Flow = 130;
    float FGEng_Mag_Derate_Percent = 5;
    float FGEng_MaxHP = 285;
    float FGEng_Gear_Ratio = 1;

    // Initialise Engine Variables used by this instance
    float FGEng1_Percentage_Power = 0;
    float FGEng1_Manifold_Pressure = 29.00;     // Inches
    float FGEng1_RPM = 500;
    float FGEng1_Fuel_Flow = 0;                 // lbs/hour
    float FGEng1_Torque = 0;
    float FGEng1_CHT = 370;
    float FGEng1_Mixture = 14;
    float FGEng1_Oil_Pressure = 0;		// PSI
    float FGEng1_Oil_Temp = 85;			// Deg C
    float FGEng1_HP = 0;
    float FGEng1_RPS = 0;
    float Torque_Imbalance = 0;
    float FGEng1_Desired_RPM = 0;

    // Initialise Propellor Variables used by this instance
    float FGProp1_Angular_V = 0;
    float FGProp1_Coef_Drag =  0.6;
    float FGProp1_Torque = 0;
    float FGProp1_Thrust = 0;
    float FGProp1_RPS = 0;
    float FGProp1_Coef_Lift = 0.1;
    float Alpha1 = 13.5;
    float FGProp1_Blade_Angle = 13.5;
    float FGProp_Fine_Pitch_Stop = 13.5;
    float FGProp_Course_Pitch_Stop = 55;

    // cout << "Enter Blade Angle  ";
    // cin  >> FGProp1_Blade_Angle;
    // cout << endl;

    cout << " Number of Iterations ";
    // cin >> num2;
    num2 = 100;
    cout << endl;

    cout << " Throttle % ";
    // cin >> FGEng1_Throttle_Lever_Pos;
    FGEng1_Throttle_Lever_Pos = 50;
    cout << endl;

    cout << " Prop % ";
    // cin >> FGEng1_Propeller_Lever_Pos;
    FGEng1_Propeller_Lever_Pos = 100;
    cout << endl;

    //==================================================================
    // Engine & Environmental Inputs from elsewhere

    // Calculate Air Density (Rho) - In FG this is calculated in 
    // FG_Atomoshere.cxx

    Rho = Density(FG_Pressure_Ht); // In FG FG_Pressure_Ht is "h"
    ShowRho(Rho);

  
    // Calculate Manifold Pressure (Engine 1) as set by throttle opening

    FGEng1_Manifold_Pressure = Manifold_Pressure(FGEng1_Throttle_Lever_Pos,
						 FGEng1_Manifold_Pressure );
    Show_Manifold_Pressure(FGEng1_Manifold_Pressure);

    // Calculate Desired RPM as set by Prop Lever Position.
    // Actual engine RPM may be different
    // The governed max RPM at 100% Prop Lever Position =  FGEng_MaxRPM
    // The governed minimum RPM at 0% Prop Lever Position = FGEng_Min_RPM
    // The actual minimum RPM of the engine can be < FGEng_Min_RPM if there is insufficient 
    //  engine torque to counter act the propeller torque at  FGProp_Fine_Pitch_Stop

    FGEng1_RPM = (FGEng1_Propeller_Lever_Pos * (FGEng_Max_RPM - FGEng_Min_RPM) /100)
	+ FGEng_Min_RPM ;

	// * ((FGEng_Max_RPM + FGEng_Min_RPM) / 100);
    
    if (FGEng1_RPM >= 2700) {
	FGEng1_RPM = 2700;
    }
    FGEng1_Desired_RPM = FGEng1_RPM;

    cout << "Desired RPM = " << FGEng1_Desired_RPM << endl;

    //==================================================================
    // Engine Power & Torque Calculations
    
    // Loop until stable - required for testing only
    for (num = 1; num < num2; num++) {
	cout << endl << "====================" << endl;
	cout << "MP Inches = " << FGEng1_Manifold_Pressure << "\t";
	cout << FGEng1_RPM << "  RPM" << "\t";

	// For a givem Manifold Pressure and RPM calculate the % Power
	// Multiply Manifold Pressure by RPM
	ManXRPM = FGEng1_Manifold_Pressure * FGEng1_RPM;
	cout << ManXRPM << endl;

	//  Calculate % Power
	FGEng1_Percentage_Power = (+ 7E-09 * ManXRPM * ManXRPM) 
	    + ( + 7E-04 * ManXRPM) - 0.1218;
	cout << "percent power = " << FGEng1_Percentage_Power <<  "%" << "\t";

	// Adjust for Temperature - Temperature above Standard decrease
	// power % by 7/120 per degree F increase, and incease power for
	// temps below at the same ratio
	FGEng1_Percentage_Power = FGEng1_Percentage_Power - (FG_ISA_VAR * 7 /120);
	cout << " adjusted T = " << FGEng1_Percentage_Power <<  "%" << "\t";
	
	// Adjust for Altitude. In this version a linear variation is
	// used. Decrease 1% for each 1000' increase in Altitde
	FGEng1_Percentage_Power = FGEng1_Percentage_Power 
	    + (FG_Pressure_Ht * 12/10000);	
	cout << " adjusted A = " << FGEng1_Percentage_Power <<  "%" << "\t";

	// Now Calculate Fuel Flow based on % Power Best Power Mixture
	FGEng1_Fuel_Flow = FGEng1_Percentage_Power
	    * FGEng_Max_Fuel_Flow / 100;
	// cout << FGEng1_Fuel_Flow << " lbs/hr"<< endl;
	
	// Now Derate engine for the effects of Bad/Switched off magnetos
	if (FGEng1_Magneto_Left == 0 && FGEng1_Magneto_Right == 0) {
	    // cout << "Both OFF\n";
	    FGEng1_Percentage_Power = 0;
	} else if (FGEng1_Magneto_Left && FGEng1_Magneto_Right) {
	    // cout << "Both On    ";
	} else if (FGEng1_Magneto_Left == 0 || FGEng1_Magneto_Right== 0) {
	    // cout << "1 Magneto Failed   ";
				
	    FGEng1_Percentage_Power = FGEng1_Percentage_Power * 
		((100 - FGEng_Mag_Derate_Percent)/100);
	    //  cout << FGEng1_Percentage_Power <<  "%" << "\t";
	}	

	// Calculate Engine Horsepower

	FGEng1_HP = FGEng1_Percentage_Power * FGEng_MaxHP/100;

	// Calculate Engine Torque

	FGEng1_Torque = FGEng1_HP * 5252 / FGEng1_RPM;
	cout << FGEng1_Torque << "Ft/lbs" << "\t";

	// Calculate Cylinder Head Temperature
	FGEng1_CHT = CHT (FGEng1_Fuel_Flow, FGEng1_Mixture, IAS);
	// Show_CHT (FGEng1_CHT);

	// Calculate Oil Pressure
	FGEng1_Oil_Pressure = Oil_Press (FGEng1_Oil_Temp, FGEng1_RPM);
	// Show_Oil_Press(FGEng1_Oil_Pressure);

	
	//==============================================================

	// Now do the Propellor Calculations

	// Revs per second
	FGProp1_RPS = FGEng1_RPM * FGEng_Gear_Ratio/60;
	// cout << FGProp1_RPS << " RPS" <<  endl;

	//Radial Flow Vector (V2) Ft/sec at Ref Blade Station (usually 30")
	FGProp1_Angular_V = FGProp1_RPS * 2 * PI * (Blade_Station / 12);
	cout << "Angular Velocity " << FGProp1_Angular_V << endl;

	// Axial Flow Vector (Vo) Ft/sec
	// Some further work required here to allow for inflow at low speeds
	// Vo = (IAS + 20) * 1.688888;
	Vo = IAS_to_FPS(IAS + 20);
	// ShowFPS ( Vo );

	// cout << Vo << "Axial Velocity" << endl;

	// Relative Velocity (V1)
	V1 = sqrt((FGProp1_Angular_V * FGProp1_Angular_V) +
		  (Vo * Vo));
	cout << "Relative Velocity " << V1 << endl;
    
	if ( FGProp1_Blade_Angle >= FGProp_Course_Pitch_Stop )  {
	    FGProp1_Blade_Angle = FGProp_Course_Pitch_Stop;
	}

	cout << FGProp1_Blade_Angle << " Prop Blade Angle" << endl;

	// Blade Angle of Attack (Alpha1)

	Alpha1 = FGProp1_Blade_Angle -(atan(Vo / FGProp1_Angular_V) * (180/PI));
	// cout << Alpha1 << " Alpha1" << endl;

	cout << "  Alpha1 = " << Alpha1
	     << "  Blade angle = " << FGProp1_Blade_Angle
	     << "  Vo = " << Vo
	     << "  FGProp1_Angular_V = " << FGProp1_Angular_V << endl;

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
	cout << "Prop Torque = " << FGProp1_Torque << endl;

	//  Calculate Prop Thrust
	FGProp1_Thrust = 0.5 * Rho * (V1 * V1) * FGProp_Area
	    * ((FGProp1_Coef_Lift * cos(Alpha1 * PI / 180))
	       - (FGProp1_Coef_Drag * sin(Alpha1 * PI / 180)));
	cout << " Prop Thrust = " << FGProp1_Thrust <<  endl;

	// End of Propeller Calculations   
	//==============================================================



	Torque_Imbalance = FGProp1_Torque - FGEng1_Torque; 
	//  cout <<  Torque_Imbalance << endl;

	if (Torque_Imbalance > 20) {
	    FGEng1_RPM -= 14.5;
	    // FGProp1_RPM -= 25;
	    FGProp1_Blade_Angle -= 0.75;
	}

	if (FGProp1_Blade_Angle < FGProp_Fine_Pitch_Stop) {
	    FGProp1_Blade_Angle = FGProp_Fine_Pitch_Stop;
	}
	if (Torque_Imbalance < -20) {
	    FGEng1_RPM += 14.5;
	    // FGProp1_RPM += 25;
	    FGProp1_Blade_Angle += 0.75;
	}

	if (FGEng1_RPM >= 2700) {
	    FGEng1_RPM = 2700;
	}


	// cout << FGEng1_RPM << " Blade_Angle  " << FGProp1_Blade_Angle << endl << endl;

    }


    return (0);
}




// Functions

// Calculate Air Density - Rho
float Density ( float x )
{
    float y ;
    y = ((9E-08 * x * x) - (7E-08 * x) + 0.0024);
    return(y);
}

// Show Air Density Calculations
void ShowRho (float x)
{
    cout << "Rho = ";
    cout << x << endl;
}





// Calculate Speed in FPS given Knots CAS
float IAS_to_FPS (float x)
{
    float y;
    y = x * 1.68888888;
    return (y);
}

// Show Feet per Second
void ShowFPS (float x)
{
    cout << "Feet/sec = ";
    cout << x << endl;
}



// Calculate Manifold Pressure based on Throttle lever Position

float Manifold_Pressure ( float x, float z)
{
    float y;  
    // if ( x < = 0 )
    //   {
    //	x = 0.00001;
    //	}
    y = x * z / 100;
    return (y);
}

// Show Manifold Pressure
void  Show_Manifold_Pressure (float x)
{
    cout << "Manifold Pressure  = ";
    cout << x << endl;
}

// Calculate Oil Temperature

float Oil_Temp (float Fuel_Flow, float Mixture, float IAS)
{
    float Oil_Temp = 85;
	
    return (Oil_Temp);
}

// Show Oil Temperature

void Show_Oil_Temp (float x)
{
    cout << "Oil Temperature (F) = ";
    cout << x << endl;
}


// Calculate Oil Pressure

float Oil_Press (float Oil_Temp, float Engine_RPM)
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
	
    return (Oil_Pressure);
}

// Show Oil Pressure
void Show_Oil_Press (float x)
{
    cout << "Oil Pressure (PSI) = ";
    cout << x << endl;
}



// Calculate Cylinder Head Temperature

float CHT (float Fuel_Flow, float Mixture, float IAS)
{
    float CHT = 350;
	
    return (CHT);
}

// Show Cyl Head Temperature

void Show_CHT (float x)
{
    cout << "CHT (F) = ";
    cout << x << endl;
}
