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
//
//
/////////////////////////////////////////////////////////////////////
//
// Modified by Dave Luff (david.luff@nottingham.ac.uk) September 2000
//
// Altered manifold pressure range to add a minimum value at idle to simulate the throttle stop / idle bypass valve,
// and to reduce the maximum value whilst the engine is running to slightly below ambient to account for CdA losses across the throttle
//
// Altered it a bit to model an IO360 from C172 - 360 cubic inches, 180 HP max, fixed pitch prop
// Added a simple fixed pitch prop model by Nev Harbor - this is not intended as a final model but simply a hack to get it running for now
// I used Phil's ManXRPM correlation for power rather than do a new one for the C172 for now, but altered it a bit to reduce power at the low end
//
// Added EGT model based on combustion efficiency and an energy balance with the exhaust gases
//
// Added a mixture - power correlation based on a curve in the IO360 operating manual
//
// I've tried to match the prop and engine model to give roughly 600 RPM idle and 180 HP at 2700 RPM
// but it is by no means currently at a completed stage - DCL 15/9/00
//
// DCL 28/9/00 - Added estimate of engine and prop inertia and changed engine speed calculation to be calculated from Angular acceleration = Torque / Inertia.  
//		 Requires a timestep to be passed to FGNewEngine::init and currently assumes this timestep does not change.
//		 Could easily be altered to pass a variable timestep to FGNewEngine::update every step instead if required.
//
// DCL 27/10/00 - Added first stab at cylinder head temperature model
//		  See the comment block in the code for details
//
// DCL 02/11/00 - Modified EGT code to reduce values to those more representative of a sensor downstream  
//
//////////////////////////////////////////////////////////////////////

#include <simgear/compiler.h>

#include <iostream>
#include <fstream>
#include <math.h>

#include "IO360.hxx"

FG_USING_STD(cout);

// ------------------------------------------------------------------------
// CODE
// ------------------------------------------------------------------------

/*
// Calculate Engine RPM based on Propellor Lever Position
float FGNewEngine::Calc_Engine_RPM (float LeverPosition)
{
    // Calculate RPM as set by Prop Lever Position. Assumes engine
    // will run at 1000 RPM at full course
    
    float RPM;
    RPM = LeverPosition * Max_RPM / 100.0;
    // * ((FGEng_Max_RPM + FGEng_Min_RPM) / 100);
    
    if ( RPM >= Max_RPM ) {
	RPM = Max_RPM;
    }

    return RPM;
}
*/

float FGNewEngine::Lookup_Combustion_Efficiency(float thi_actual)
{
    float thi[11];  //array of equivalence ratio values
    float neta_comb[11];  //corresponding array of combustion efficiency values
    float neta_comb_actual;
    float factor;

    //thi = (0.0,0.9,1.0,1.05,1.1,1.15,1.2,1.3,1.4,1.5,1.6);
    thi[0] = 0.0;
    thi[1] = 0.9;
    thi[2] = 1.0;
    thi[3] = 1.05;	//There must be an easier way of doing this !!!!!!!!
    thi[4] = 1.1;
    thi[5] = 1.15;
    thi[6] = 1.2;
    thi[7] = 1.3;
    thi[8] = 1.4;
    thi[9] = 1.5;
    thi[10] = 1.6;
    //neta_comb = (0.98,0.98,0.97,0.95,0.9,0.85,0.79,0.7,0.63,0.57,0.525);
    neta_comb[0] = 0.98;
    neta_comb[1] = 0.98;
    neta_comb[2] = 0.97;
    neta_comb[3] = 0.95;
    neta_comb[4] = 0.9;
    neta_comb[5] = 0.85;
    neta_comb[6] = 0.79;
    neta_comb[7] = 0.7;
    neta_comb[8] = 0.63;
    neta_comb[9] = 0.57;
    neta_comb[10] = 0.525;
    //combustion efficiency values from Heywood: ISBN 0-07-100499-8

    int i;
    int j;
    j = 11;  //This must be equal to the number of elements in the lookup table arrays

    for(i=0;i<j;i++)
    {
	if(i == (j-1))
	{
	    //this is just to avoid crashing the routine is we are bigger than the last element - for now just return the last element
	    //but at some point we will have to extrapolate further
	    neta_comb_actual = neta_comb[i];
	    return neta_comb_actual;
	}
	if(thi_actual == thi[i])
	{
	    neta_comb_actual = neta_comb[i];
	    return neta_comb_actual;
	}
	if((thi_actual > thi[i]) && (thi_actual < thi[i + 1]))
	{
	    //do linear interpolation between the two points
	    factor = (thi_actual - thi[i]) / (thi[i+1] - thi[i]);
	    neta_comb_actual = (factor * (neta_comb[i+1] - neta_comb[i])) + neta_comb[i];
	    return neta_comb_actual;
	}
    }

    //if we get here something has gone badly wrong
    cout << "ERROR: error in FGNewEngine::Lookup_Combustion_Efficiency\n";
    //exit(-1);
    return neta_comb_actual;  //keeps the compiler happy
}
/*
float FGNewEngine::Calculate_Delta_T_Exhaust(void)
{
	float dT_exhaust;
	heat_capacity_exhaust = (Cp_air * m_dot_air) + (Cp_fuel * m_dot_fuel);
	dT_exhaust = enthalpy_exhaust / heat_capacity_exhaust;

	return(dT_exhaust);
}
*/

// Calculate Manifold Pressure based on Throttle lever Position
static float Calc_Manifold_Pressure ( float LeverPosn, float MaxMan, float MinMan)
{
    float Inches;  
    // if ( x < = 0 ) {
    //   x = 0.00001;
    // }

    //Note that setting the manifold pressure as a function of lever position only is not strictly accurate
    //MAP is also a function of engine speed.
    Inches = MinMan + (LeverPosn * (MaxMan - MinMan) / 100);

    //allow for idle bypass valve or slightly open throttle stop
    if(Inches < MinMan)
	Inches = MinMan;

    return Inches;
}


// set initial default values
void FGNewEngine::init(double dt) {

    CONVERT_CUBIC_INCHES_TO_METERS_CUBED = 1.638706e-5;
    // Control and environment inputs
    IAS = 0;
    Throttle_Lever_Pos = 75;
    Propeller_Lever_Pos = 75;	
    Mixture_Lever_Pos = 100;
    Cp_air = 1005;	// J/KgK
    Cp_fuel = 1700;	// J/KgK
    calorific_value_fuel = 47.3e6;  // W/Kg  Note that this is only an approximate value
    R_air = 287.3;
    time_step = dt;

    // Engine Specific Variables used by this program that have limits.
    // Will be set in a parameter file to be read in to create
    // and instance for each engine.
    Max_Manifold_Pressure = 28.50;  //Inches Hg. An approximation - should be able to find it in the engine performance data
    Min_Manifold_Pressure = 6.5;    //Inches Hg. This is a guess corresponding to approx 0.24 bar MAP (7 in Hg) - need to find some proper data for this
    Max_RPM = 2700;
    Min_RPM = 600;		    //Recommended idle from Continental data sheet
    Max_Fuel_Flow = 130;
    Mag_Derate_Percent = 5;
//    MaxHP = 285;    //Continental IO520-M
    MaxHP = 180;    //Lycoming IO360
//  displacement = 520;  //Continental IO520-M
    displacement = 360;	 //Lycoming IO360   
    engine_inertia = 0.2;  //kgm^2 - value taken from a popular family saloon car engine - need to find an aeroengine value !!!!!
    prop_inertia = 0.03;  //kgm^2 - this value is a total guess - dcl
    displacement_SI = displacement * CONVERT_CUBIC_INCHES_TO_METERS_CUBED;

    Gear_Ratio = 1;
    started = true;
    cranking = false;

    CONVERT_HP_TO_WATTS = 745.6999;
//    ofstream outfile;
 //   outfile.open(ios::out|ios::trunc);

    // Initialise Engine Variables used by this instance
    Percentage_Power = 0;
    Manifold_Pressure = 29.00; // Inches
    RPM = 600;
    Fuel_Flow = 0;	// lbs/hour
    Torque = 0;
    CHT = 298.0;	//deg Kelvin
    CHT_degF = (CHT * 1.8) - 459.67;  //deg Fahrenheit
    Mixture = 14;
    Oil_Pressure = 0;	// PSI
    Oil_Temp = 85;	// Deg C
    HP = 0;
    RPS = 0;
    Torque_Imbalance = 0;
    Desired_RPM = 2500;	    //Recommended cruise RPM from Continental datasheet

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


/*
// Calculate Cylinder Head Temperature
static float Calc_CHT (float Fuel_Flow, float Mixture, float IAS, float rhoair, float tamb)
{
    float CHT = 350;
	
    return CHT;
}
*/

/*
//Calculate Exhaust Gas Temperature
//For now we will simply adjust this as a function of mixture
//It may be necessary to consider fuel flow rates and CHT in the calculation in the future
static float Calc_EGT (float Mixture)
{
    float EGT = 1000;   //off the top of my head !!!!
    //Now adjust for mixture strength

    return EGT;
}*/


// Calculate Density Ratio
static float Density_Ratio ( float x )
{
    float y ;
    y = ((3E-10 * x * x) - (3E-05 * x) + 0.9998);
    return(y);
}


// Calculate Air Density - Rho
static float Density ( float x )
{
    float y ;
    y = ((9E-08 * x * x) - (7E-08 * x) + 0.0024);
    return(y);
}


// Calculate Speed in FPS given Knots CAS
static float IAS_to_FPS (float x)
{
    float y;
    y = x * 1.68888888;
    return y;
}


//*****************************************************************************
//*****************************************************************************
// update the engine model based on current control positions
void FGNewEngine::update() {
    // Declare local variables
//    int num = 0;
    // const int num2 = 500;	// default is 100, number if iterations to run
//    const int num2 = 5;	// default is 100, number if iterations to run
    float ManXRPM = 0;
    float Vo = 0;
    float V1 = 0;


    // Set up the new variables
    float Blade_Station = 30;
    float FGProp_Area = 1.405/3;
    float PI = 3.1428571;

    // Input Variables

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
    int Fuel_Available = 1;	// Yes = 1. Is there Fuel Available. Calculated elsewhere
    int Alternate_Air_Pos =0;	// Off = 0. Reduces power by 3 % for same throttle setting
    int Magneto_Left = 1;	// 1 = On.   Reduces power by 5 % for same power lever settings
    int Magneto_Right = 1;	// 1 = On.  Ditto, Both of the above though do not alter fuel flow


    //==================================================================
    // Engine & Environmental Inputs from elsewhere

    // Calculate Air Density (Rho) - In FG this is calculated in 
    // FG_Atomoshere.cxx

    Rho = Density(FG_Pressure_Ht); // In FG FG_Pressure_Ht is "h"
    // cout << "Rho = " << Rho << endl;

    // Calculate Manifold Pressure (Engine 1) as set by throttle opening

    Manifold_Pressure = 
	Calc_Manifold_Pressure( Throttle_Lever_Pos, Max_Manifold_Pressure, Min_Manifold_Pressure );
    // cout << "manifold pressure = " << Manifold_Pressure << endl;

//**************************FIXME*******************************************
    //DCL - hack for testing - fly at sea level
    T_amb = 298.0;
    p_amb = 101325;
    p_amb_sea_level = 101325;

    //DCL - next calculate m_dot_air and m_dot_fuel into engine

    //calculate actual ambient pressure and temperature from altitude
    //Then find the actual manifold pressure (the calculated one is the sea level pressure)
    True_Manifold_Pressure = Manifold_Pressure * p_amb / p_amb_sea_level;

    //    RPM = Calc_Engine_RPM(Propeller_Lever_Pos);
    // RPM = 600;
    // cout << "Initial engine RPM = " << RPM << endl;

//    Desired_RPM = RPM;

//**************	
	
    //DCL - calculate mass air flow into engine based on speed and load - separate this out into a function eventually
    //t_amb is actual temperature calculated from altitude
    //calculate density from ideal gas equation
    rho_air = p_amb / ( R_air * T_amb );
    rho_air_manifold = rho_air * Manifold_Pressure / 29.6;
    //calculate ideal engine volume inducted per second
    swept_volume = (displacement_SI * (RPM / 60)) / 2;  //This equation is only valid for a four stroke engine
    //calculate volumetric efficiency - for now we will just use 0.8, but actually it is a function of engine speed and the exhaust to manifold pressure ratio
    volumetric_efficiency = 0.8;
    //Now use volumetric efficiency to calculate actual air volume inducted per second
    v_dot_air = swept_volume * volumetric_efficiency;
    //Now calculate mass flow rate of air into engine
    m_dot_air = v_dot_air * rho_air_manifold;

    // cout << "rho air manifold " << rho_air_manifold << '\n';
    // cout << "Swept volume " << swept_volume << '\n';

//**************

    //DCL - now calculate fuel flow into engine based on air flow and mixture lever position
    //assume lever runs from no flow at fully out to thi = 1.6 at fully in at sea level
    //also assume that the injector linkage is ideal - hence the set mixture is maintained at a given altitude throughout the speed and load range
    thi_sea_level = 1.6 * ( Mixture_Lever_Pos / 100.0 );
    equivalence_ratio = thi_sea_level * p_amb_sea_level / p_amb; //ie as we go higher the mixture gets richer for a given lever position
    m_dot_fuel = m_dot_air / 14.7 * equivalence_ratio;

    // cout << "fuel " << m_dot_fuel;
    // cout << " air " << m_dot_air << '\n';

//***********************************************************************
//Calculate percentage power

    // For a given Manifold Pressure and RPM calculate the % Power
    // Multiply Manifold Pressure by RPM
    ManXRPM = Manifold_Pressure * RPM;
    //	cout << ManXRPM;
    // cout << endl;

/*
//  Phil's %power correlation
    //  Calculate % Power
    Percentage_Power = (+ 7E-09 * ManXRPM * ManXRPM) + ( + 7E-04 * ManXRPM) - 0.1218;
    // cout << Percentage_Power <<  "%" << "\t";   
*/

// DCL %power correlation - basically Phil's correlation modified to give slighty less power at the low end
// might need some adjustment as the prop model is adjusted
// My aim is to match the prop model and engine model at the low end to give the manufacturer's recommended idle speed with the throttle closed - 600rpm for the Continental IO520
    //  Calculate % Power
    Percentage_Power = (+ 6E-09 * ManXRPM * ManXRPM) + ( + 8E-04 * ManXRPM) - 1.8524;
    // cout << Percentage_Power <<  "%" << "\t";
    
    
    // Adjust for Temperature - Temperature above Standard decrease
    // power % by 7/120 per degree F increase, and incease power for
    // temps below at the same ratio
    Percentage_Power = Percentage_Power - (FG_ISA_VAR * 7 /120);
    // cout << Percentage_Power <<  "%" << "\t";

//******DCL - this bit will need altering or removing if I go to true altitude adjusted manifold pressure
    // Adjust for Altitude. In this version a linear variation is
    // used. Decrease 1% for each 1000' increase in Altitde
    Percentage_Power = Percentage_Power + (FG_Pressure_Ht * 12/10000);	
    // cout << Percentage_Power <<  "%" << "\t";
    
    
    //DCL - now adjust power to compensate for mixture
    //uses a curve fit to the data in the IO360 / O360 operating manual
    //due to the shape of the curve I had to use a 6th order fit - I am sure it must be possible to reduce this in future,
    //possibly by using separate fits for rich and lean of best power mixture
    //first adjust actual mixture to abstract mixture - this is a temporary hack in order to account for the fact that the data I have
    //dosn't specify actual mixtures and I want to be able to change what I think they are without redoing the curve fit each time.
    //y=10x-12 for now
    abstract_mixture = 10.0 * equivalence_ratio - 12.0;
    float m = abstract_mixture;  //to simplify writing the next equation
    Percentage_of_best_power_mixture_power = ((-0.0012*m*m*m*m*m*m) + (0.021*m*m*m*m*m) + (-0.1425*m*m*m*m) + (0.4395*m*m*m) + (-0.8909*m*m) + (-0.5155*m) + 100.03);
    Percentage_Power = Percentage_Power * Percentage_of_best_power_mixture_power / 100.0;
    
    //cout << " %POWER = " << Percentage_Power << '\n';
    
//***DCL - FIXME - this needs altering - for instance going richer than full power mixture decreases the %power but increases the fuel flow    
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
	//  cout << FGEng1_Percentage_Power <<  "%" << "\t";
    }	



//**********************************************************************
//Calculate Exhaust gas temperature

    // cout << "Thi = " << equivalence_ratio << '\n'; 

    combustion_efficiency = Lookup_Combustion_Efficiency(equivalence_ratio);  //The combustion efficiency basically tells us what proportion of the fuels calorific value is released

    // cout << "Combustion efficiency = " << combustion_efficiency << '\n';

    //now calculate energy release to exhaust
    //We will assume a three way split of fuel energy between useful work, the coolant system and the exhaust system
    //This is a reasonable first suck of the thumb estimate for a water cooled automotive engine - whether it holds for an air cooled aero engine is probably open to question
    //Regardless - it won't affect the variation of EGT with mixture, and we con always put a multiplier on EGT to get a reasonable peak value.
    enthalpy_exhaust = m_dot_fuel * calorific_value_fuel * combustion_efficiency * 0.33;
    heat_capacity_exhaust = (Cp_air * m_dot_air) + (Cp_fuel * m_dot_fuel);
    delta_T_exhaust = enthalpy_exhaust / heat_capacity_exhaust;	
//  delta_T_exhaust = Calculate_Delta_T_Exhaust();

    // cout << "T_amb " << T_amb;
    // cout << " dT exhaust = " << delta_T_exhaust;

    EGT = T_amb + delta_T_exhaust;

    //The above gives the exhaust temperature immediately prior to leaving the combustion chamber
    //Now derate to give a more realistic figure as measured downstream
    //For now we will aim for a peak of around 400 degC (750 degF)

    EGT *= 0.444 + ((0.544 - 0.444) * Percentage_Power / 100.0);

    EGT_degF = (EGT * 1.8) - 459.67;

    //cout << " EGT = " << EGT << " degK " << EGT_degF << " degF";// << '\n';

//***************************************************************************************
// Calculate Cylinder Head Temperature

/* DCL 27/10/00  

This is a somewhat rough first attempt at modelling cylinder head temperature.  The cylinder head
is assumed to be at uniform temperature.  Obviously this is incorrect, but it simplifies things a 
lot, and we're just looking for the behaviour of CHT to be correct.  Energy transfer to the cylinder
head is assumed to be one third of the energy released by combustion at all conditions.  This is a
reasonable estimate, although obviously in real life it varies with different conditions and possibly
with CHT itself.  I've split energy transfer from the cylinder head into 2 terms - free convection - 
ie convection to stationary air, and forced convection, ie convection into flowing air.  The basic 
free convection equation is: dqdt = -hAdT   Since we don't know A and are going to set h quite arbitarily
anyway I've knocked A out and just wrapped it up in h - the only real significance is that the units
of h will be different but that dosn't really matter to us anyway.  In addition, we have the problem
that the prop model I'm currently using dosn't model the backwash from the prop which will add to the
velocity of the cooling air when the prop is turning, so I've added an extra term to try and cope 
with this.

In real life, forced convection equations are genarally empirically derived, and are quite complicated 
and generally contain such things as the Reynolds and Nusselt numbers to various powers.  The best 
course of action would probably to find an empirical correlation from the literature for a similar
situation and try and get it to fit well.  However, for now I am using my own made up very simple 
correlation for the energy transfer from the cylinder head:

dqdt = -(h1.dT) -(h2.m_dot.dT) -(h3.rpm.dT)

where dT is the temperature different between the cylinder head and the surrounding air, m_dot is the
mass flow rate of cooling air through an arbitary volume, rpm is the engine speed in rpm (this is the
backwash term), and h1, h2, h3 are co-efficients which we can play with to attempt to get the CHT 
behaviour to match real life.

In order to change the values of CHT that the engine settles down at at various conditions,
have a play with h1, h2 and h3.  In order to change the rate of heating/cooling without affecting
equilibrium values alter the cylinder head mass, which is really quite arbitary.  Bear in mind that
altering h1, h2 and h3 will also alter the rate of heating or cooling as well as equilibrium values,
but altering the cylinder head mass will only alter the rate.  It would I suppose be better to read 
the values from file to avoid the necessity for re-compilation every time I change them.
		    
*/
    //CHT = Calc_CHT( Fuel_Flow, Mixture, IAS);
    // cout << "Cylinder Head Temp (F) = " << CHT << endl;
    float h1 = -95.0;   //co-efficient for free convection
    float h2 = -3.95;   //co-efficient for forced convection
    float h3 = -0.05;	//co-efficient for forced convection due to prop backwash
    float v_apparent;	//air velocity over cylinder head in m/s
    float v_dot_cooling_air;
    float m_dot_cooling_air;
    float temperature_difference;
    float arbitary_area = 1.0;
    float dqdt_from_combustion;
    float dqdt_forced;	    //Rate of energy transfer to/from cylinder head due to forced convection (Joules) (sign convention: to cylinder head is +ve)
    float dqdt_free;	    //Rate of energy transfer to/from cylinder head due to free convection (Joules) (sign convention: to cylinder head is +ve)
    float dqdt_cylinder_head;	    //Overall energy change in cylinder head
    float CpCylinderHead = 800.0;   //FIXME - this is a guess - I need to look up the correct value
    float MassCylinderHead = 8.0;   //Kg - this is a guess - it dosn't have to be absolutely accurate but can be varied to alter the warm-up rate
    float HeatCapacityCylinderHead;
    float dCHTdt;

    temperature_difference = CHT - T_amb; 

    v_apparent = IAS * 0.5144444;  //convert from knots to m/s
    v_dot_cooling_air = arbitary_area * v_apparent;
    m_dot_cooling_air = v_dot_cooling_air * rho_air;

    //Calculate rate of energy transfer to cylinder head from combustion
    dqdt_from_combustion = m_dot_fuel * calorific_value_fuel * combustion_efficiency * 0.33;
    
    //Calculate rate of energy transfer from cylinder head due to cooling  NOTE is calculated as rate to but negative
    dqdt_forced = (h2 * m_dot_cooling_air * temperature_difference) + (h3 * RPM * temperature_difference);
    dqdt_free = h1 * temperature_difference;

    //Calculate net rate of energy transfer to or from cylinder head
    dqdt_cylinder_head = dqdt_from_combustion + dqdt_forced + dqdt_free;

    HeatCapacityCylinderHead = CpCylinderHead * MassCylinderHead;

    dCHTdt = dqdt_cylinder_head / HeatCapacityCylinderHead;

    CHT += (dCHTdt * time_step);

    CHT_degF = (CHT * 1.8) - 459.67;

    //cout << " CHT = " << CHT_degF << " degF\n";


// End calculate Cylinder Head Temperature

 
//***************************************************************************************
// Engine Power & Torque Calculations




	// Calculate Engine Horsepower

	HP = Percentage_Power * MaxHP / 100.0;

	Power_SI = HP * CONVERT_HP_TO_WATTS;

	// Calculate Engine Torque

	Torque = HP * 5252 / RPM;
	// cout << Torque << "Ft/lbs" << "\t";

	Torque_SI = (Power_SI * 60.0) / (2.0 * PI * RPM);  //Torque = power / angular velocity
	// cout << Torque << " Nm\n";


	// Calculate Oil Pressure
	Oil_Pressure = Oil_Press( Oil_Temp, RPM );
	// cout << "Oil Pressure (PSI) = " << Oil_Pressure << endl;
	
	//==============================================================

	// Now do the Propellor Calculations

#ifdef PHILS_PROP_MODEL

	// Revs per second
	FGProp1_RPS = RPM * Gear_Ratio / 60.0;
	// cout << FGProp1_RPS << " RPS" <<  endl;

	//Radial Flow Vector (V2) Ft/sec at Ref Blade Station (usually 30")
	FGProp1_Angular_V = FGProp1_RPS * 2 * PI * (Blade_Station / 12);
	//  cout << FGProp1_Angular_V << "Angular Velocity "  << endl;

	// Axial Flow Vector (Vo) Ft/sec
	// Some further work required here to allow for inflow at low speeds
	// Vo = (IAS + 20) * 1.688888;
	Vo = IAS_to_FPS(IAS + 20);
	// cout << "Feet/sec = " << Vo << endl;

	// cout << Vo << "Axial Velocity" << endl;

	// Relative Velocity (V1)
	V1 = sqrt((FGProp1_Angular_V * FGProp1_Angular_V) +
		  (Vo * Vo));
	// cout << V1 << "Relative Velocity " << endl;

	// cout << FGProp1_Blade_Angle << " Prop Blade Angle" << endl;

	// Blade Angle of Attack (Alpha1)

/*	cout << "  Alpha1 = " << Alpha1
	     << "  Blade angle = " << FGProp1_Blade_Angle
	     << "  Vo = " << Vo
	     << "  FGProp1_Angular_V = " << FGProp1_Angular_V << endl;*/
	Alpha1 = FGProp1_Blade_Angle -(atan(Vo / FGProp1_Angular_V) * (180/PI));
	// cout << Alpha1 << " Alpha1" << endl;

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
	// cout <<  FGProp1_Torque << " Prop Torque" << endl;

	//  Calculate Prop Thrust
	// cout << "  V1 = " << V1 << "  Alpha1 = " << Alpha1 << endl;
	FGProp1_Thrust = 0.5 * Rho * (V1 * V1) * FGProp_Area
	    * ((FGProp1_Coef_Lift * cos(Alpha1 * PI / 180))
	       - (FGProp1_Coef_Drag * sin(Alpha1 * PI / 180)));
	// cout << FGProp1_Thrust << " Prop Thrust " <<  endl;

	// End of Propeller Calculations   
	//==============================================================

#endif  //PHILS_PROP_MODEL

#ifdef NEVS_PROP_MODEL

	// Nev's prop model

	num_elements = 6.0;
	number_of_blades = 2.0;
	blade_length = 0.95;
	allowance_for_spinner = blade_length / 12.0;
	prop_fudge_factor = 1.453401525;
	forward_velocity = IAS;

	theta[0] = 25.0;
	theta[1] = 20.0;
	theta[2] = 15.0;
	theta[3] = 10.0;
	theta[4] = 5.0;
	theta[5] = 0.0;

	angular_velocity_SI = 2.0 * PI * RPM / 60.0;

	allowance_for_spinner = blade_length / 12.0;
	//Calculate thrust and torque by summing the contributions from each of the blade elements
	//Assumes equal length elements with numbered 1 inboard -> num_elements outboard
	prop_torque = 0.0;
	prop_thrust = 0.0;
	int i;
//	outfile << "Rho = " << Rho << '\n\n';
//	outfile << "Drag = ";
	for(i=1;i<=num_elements;i++)
	{
	    element = float(i);
	    distance = (blade_length * (element / num_elements)) + allowance_for_spinner; 
	    element_drag = 0.5 * rho_air * ((distance * angular_velocity_SI)*(distance * angular_velocity_SI)) * (0.000833 * ((theta[int(element-1)] - (atan(forward_velocity/(distance * angular_velocity_SI))))*(theta[int(element-1)] - (atan(forward_velocity/(distance * angular_velocity_SI))))))
			    * (0.1 * (blade_length / element)) * number_of_blades;

	    element_lift = 0.5 * rho_air * ((distance * angular_velocity_SI)*(distance * angular_velocity_SI)) * (0.036 * (theta[int(element-1)] - (atan(forward_velocity/(distance * angular_velocity_SI))))+0.4)
			    * (0.1 * (blade_length / element)) * number_of_blades;
	    element_torque = element_drag * distance;
	    prop_torque += element_torque;
	    prop_thrust += element_lift;
//	    outfile << "Drag = " << element_drag << " n = " << element << '\n';
	}

//	outfile << '\n';

//	outfile << "Angular velocity = " << angular_velocity_SI << " rad/s\n";

	// cout << "Thrust = " << prop_thrust << '\n';
	prop_thrust *= prop_fudge_factor;
	prop_torque *= prop_fudge_factor;
	prop_power_consumed_SI = prop_torque * angular_velocity_SI;
	prop_power_consumed_HP = prop_power_consumed_SI / 745.699;


#endif //NEVS_PROP_MODEL


//#if 0
#ifdef PHILS_PROP_MODEL  //Do Torque calculations in Ft/lbs - yuk :-(((
	Torque_Imbalance = FGProp1_Torque - Torque; 

	if (Torque_Imbalance > 5) {
	    RPM -= 14.5;
	    // FGProp1_RPM -= 25;
//	    FGProp1_Blade_Angle -= 0.75;
	}

	if (Torque_Imbalance < -5) {
	    RPM += 14.5;
	    // FGProp1_RPM += 25;
//	    FGProp1_Blade_Angle += 0.75;
	}
#endif


#ifdef NEVS_PROP_MODEL	    //use proper units - Nm
	Torque_Imbalance = Torque_SI - prop_torque;  //This gives a +ve value when the engine torque exeeds the prop torque

	angular_acceleration = Torque_Imbalance / (engine_inertia + prop_inertia);
	angular_velocity_SI += (angular_acceleration * time_step);
	RPM = (angular_velocity_SI * 60) / (2.0 * PI);
#endif



/*
	if( RPM > (Desired_RPM + 2)) {
	    FGProp1_Blade_Angle += 0.75;  //This value could be altered depending on how far from the desired RPM we are
	}

	if( RPM < (Desired_RPM - 2)) {
	    FGProp1_Blade_Angle -= 0.75;
	}

	if (FGProp1_Blade_Angle < FGProp_Fine_Pitch_Stop) {
	    FGProp1_Blade_Angle = FGProp_Fine_Pitch_Stop;
	}

	if (RPM >= 2700) {
	    RPM = 2700;
	}
*/
	//end constant speed prop
//#endif

	//DCL - stall the engine if RPM drops below 550 - this is possible if mixture lever is pulled right out
	if(RPM < 550)
	    RPM = 0;

//	outfile << "RPM = " << RPM << " Blade angle = " << FGProp1_Blade_Angle << " Engine torque = " << Torque << " Prop torque = " << FGProp1_Torque << '\n';
//	outfile << "RPM = " << RPM << " Engine torque = " << Torque_SI << " Prop torque = " << prop_torque << '\n';    

	// cout << FGEng1_RPM << " Blade_Angle  " << FGProp1_Blade_Angle << endl << endl;



    // cout << "Final engine RPM = " << RPM << '\n';
}




// Functions

// Calculate Oil Temperature

static float Oil_Temp (float Fuel_Flow, float Mixture, float IAS)
{
    float Oil_Temp = 85;
	
    return (Oil_Temp);
}
