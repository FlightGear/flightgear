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
// DCL 28/09/00 - Added estimate of engine and prop inertia and changed engine speed calculation to be calculated from Angular acceleration = Torque / Inertia.  
//		  Requires a timestep to be passed to FGNewEngine::init and currently assumes this timestep does not change.
//		  Could easily be altered to pass a variable timestep to FGNewEngine::update every step instead if required.
//
// DCL 27/10/00 - Added first stab at cylinder head temperature model
//		  See the comment block in the code for details
//
// DCL 02/11/00 - Modified EGT code to reduce values to those more representative of a sensor downstream 
//
// DCL 02/02/01 - Changed the prop model to one based on efficiency and co-efficient of power curves from McCormick instead of the
//		  blade element method we were using previously.  This works much better, and is similar to how Jon is doing it in JSBSim. 
//
// DCL 08/02/01 - Overhauled fuel consumption rate support.  
//
// DCL 22/03/01 - Added input of actual air pressure and temperature (and hence density) to the model.  Hence the power correlation
//                with pressure height and temperature is no longer required since the power is based on the actual manifold pressure.
//
// DCL 22/03/01 - based on Riley's post on the list (25 rpm gain at 1000 rpm as lever is pulled out from full rich)
//                I have reduced the sea level full rich mixture to thi = 1.3 
//////////////////////////////////////////////////////////////////////

#include <simgear/compiler.h>

#include <iostream>
#include <fstream>
#include <math.h>

#include "IO360.hxx"

FG_USING_STD(cout);

// Static utility functions

// Calculate Density Ratio
static float Density_Ratio ( float x )
{
    float y ;
    y = ((3E-10 * x * x) - (3E-05 * x) + 0.9998);
    return(y);
}


// Calculate Air Density - Rho, using the ideal gas equation
// Takes and returns SI values
static float Density ( float temperature, float pressure )
{
    // rho = P / RT
    // R = 287.3 for air

    float R = 287.3;
    float rho = pressure / (R * temperature);
    return(rho);
}


// Calculate Speed in FPS given Knots CAS
static float IAS_to_FPS (float x)
{
    float y;
    y = x * 1.68888888;
    return y;
}

// FGNewEngine member functions

float FGNewEngine::Lookup_Combustion_Efficiency(float thi_actual)
{
    const int NUM_ELEMENTS = 11;
    float thi[NUM_ELEMENTS] = {0.0, 0.9, 1.0, 1.05, 1.1, 1.15, 1.2, 1.3, 1.4, 1.5, 1.6};  //array of equivalence ratio values
    float neta_comb[NUM_ELEMENTS] = {0.98, 0.98, 0.97, 0.95, 0.9, 0.85, 0.79, 0.7, 0.63, 0.57, 0.525};  //corresponding array of combustion efficiency values
    //combustion efficiency values from Heywood, "Internal Combustion Engine Fundamentals", ISBN 0-07-100499-8
    float neta_comb_actual;
    float factor;

    int i;
    int j = NUM_ELEMENTS;  //This must be equal to the number of elements in the lookup table arrays

    for(i=0;i<j;i++)
    {
	if(i == (j-1)) {
	    // Assume linear extrapolation of the slope between the last two points beyond the last point
	    float dydx = (neta_comb[i] - neta_comb[i-1]) / (thi[i] - thi[i-1]);
	    neta_comb_actual = neta_comb[i] + dydx * (thi_actual - thi[i]);
	    return neta_comb_actual;
	}
	if(thi_actual == thi[i]) {
	    neta_comb_actual = neta_comb[i];
	    return neta_comb_actual;
	}
	if((thi_actual > thi[i]) && (thi_actual < thi[i + 1])) {
	    //do linear interpolation between the two points
	    factor = (thi_actual - thi[i]) / (thi[i+1] - thi[i]);
	    neta_comb_actual = (factor * (neta_comb[i+1] - neta_comb[i])) + neta_comb[i];
	    return neta_comb_actual;
	}
    }

    //if we get here something has gone badly wrong
    cout << "ERROR: error in FGNewEngine::Lookup_Combustion_Efficiency\n";
    return neta_comb_actual;
}

////////////////////////////////////////////////////////////////////////////////////////////
// Return the percentage of best mixture power available at a given mixture strength
//
// Based on data from "Technical Considerations for Catalysts for the European Market"
// by H S Gandi, published 1988 by IMechE
//
// Note that currently no attempt is made to set a lean limit on stable combustion
////////////////////////////////////////////////////////////////////////////////////////////
float FGNewEngine::Power_Mixture_Correlation(float thi_actual)
{
    float AFR_actual = 14.7 / thi_actual;
    // thi and thi_actual are equivalence ratio
    const int NUM_ELEMENTS = 13;
    // The lookup table is in AFR because the source data was.  I added the two end elements to make sure we are almost always in it.
    float AFR[NUM_ELEMENTS] = {(14.7/1.6), 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, (14.7/0.6)};  //array of equivalence ratio values
    float mixPerPow[NUM_ELEMENTS] = {78, 86, 93.5, 98, 100, 99, 96.4, 92.5, 88, 83, 78.5, 74, 58};  //corresponding array of combustion efficiency values
    float mixPerPow_actual;
    float factor;
    float dydx;

    int i;
    int j = NUM_ELEMENTS;  //This must be equal to the number of elements in the lookup table arrays

    for(i=0;i<j;i++)
    {
	if(i == (j-1)) {
	    // Assume linear extrapolation of the slope between the last two points beyond the last point
	    dydx = (mixPerPow[i] - mixPerPow[i-1]) / (AFR[i] - AFR[i-1]);
	    mixPerPow_actual = mixPerPow[i] + dydx * (AFR_actual - AFR[i]);
	    return mixPerPow_actual;
	}
	if((i == 0) && (AFR_actual < AFR[i])) {
	    // Assume linear extrapolation of the slope between the first two points for points before the first point
	    dydx = (mixPerPow[i] - mixPerPow[i-1]) / (AFR[i] - AFR[i-1]);
	    mixPerPow_actual = mixPerPow[i] + dydx * (AFR_actual - AFR[i]);
	    return mixPerPow_actual;
	}
	if(AFR_actual == AFR[i]) {
	    mixPerPow_actual = mixPerPow[i];
	    return mixPerPow_actual;
	}
	if((AFR_actual > AFR[i]) && (AFR_actual < AFR[i + 1])) {
	    //do linear interpolation between the two points
	    factor = (AFR_actual - AFR[i]) / (AFR[i+1] - AFR[i]);
	    mixPerPow_actual = (factor * (mixPerPow[i+1] - mixPerPow[i])) + mixPerPow[i];
	    return mixPerPow_actual;
	}
    }

    //if we get here something has gone badly wrong
    cout << "ERROR: error in FGNewEngine::Power_Mixture_Correlation\n";
    return mixPerPow_actual;
}



// Calculate Manifold Pressure based on Throttle lever Position
float FGNewEngine::Calc_Manifold_Pressure ( float LeverPosn, float MaxMan, float MinMan)
{
    float Inches;
    // if ( x < = 0 ) {
    //   x = 0.00001;
    // }

    //Note that setting the manifold pressure as a function of lever position only is not strictly accurate
    //MAP is also a function of engine speed. (and ambient pressure if we are going for an actual MAP model)
    Inches = MinMan + (LeverPosn * (MaxMan - MinMan) / 100);

    //allow for idle bypass valve or slightly open throttle stop
    if(Inches < MinMan)
	Inches = MinMan;

    return Inches;
}




// Calculate Oil Temperature
float FGNewEngine::Calc_Oil_Temp (float Fuel_Flow, float Mixture, float IAS)
{
    float Oil_Temp = 85;

    return (Oil_Temp);
}

// Calculate Oil Pressure
float FGNewEngine::Calc_Oil_Press (float Oil_Temp, float Engine_RPM)
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

//*************************************************************************************
// Initialise the engine model
void FGNewEngine::init(double dt) {

    // These constants should probably be moved eventually
    CONVERT_CUBIC_INCHES_TO_METERS_CUBED = 1.638706e-5;
    CONVERT_HP_TO_WATTS = 745.6999;

    // Properties of working fluids
    Cp_air = 1005;	// J/KgK
    Cp_fuel = 1700;	// J/KgK
    calorific_value_fuel = 47.3e6;  // W/Kg  Note that this is only an approximate value
    rho_fuel = 800;	// kg/m^3 - an estimate for now
    R_air = 287.3;
    p_amb_sea_level = 101325;

    // Control and environment inputs
    IAS = 0;
    Throttle_Lever_Pos = 75;
    Propeller_Lever_Pos = 75;
    Mixture_Lever_Pos = 100;

    time_step = dt;

    // Engine Specific Variables.
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
    displacement_SI = displacement * CONVERT_CUBIC_INCHES_TO_METERS_CUBED;
    engine_inertia = 0.2;  //kgm^2 - value taken from a popular family saloon car engine - need to find an aeroengine value !!!!!
    prop_inertia = 0.03;  //kgm^2 - this value is a total guess - dcl
    Gear_Ratio = 1;

    started = true;
    cranking = false;


    // Initialise Engine Variables used by this instance
    Percentage_Power = 0;
    Manifold_Pressure = 29.00; // Inches
    RPM = 600;
    Fuel_Flow_gals_hr = 0;
    Torque = 0;
    Torque_SI = 0;
    CHT = 298.0;	//deg Kelvin
    CHT_degF = (CHT * 1.8) - 459.67;  //deg Fahrenheit
    Mixture = 14;
    Oil_Pressure = 0;	// PSI
    Oil_Temp = 85;	// Deg C
    HP = 0;
    RPS = 0;
    Torque_Imbalance = 0;

    // Initialise Propellor Variables used by this instance
    FGProp1_Thrust = 0;
    FGProp1_RPS = 0;
    FGProp1_Blade_Angle = 13.5;
    prop_diameter = 1.8;         // meters
    blade_angle = 23.0;          // degrees
}


//*****************************************************************************
//*****************************************************************************
// update the engine model based on current control positions
void FGNewEngine::update() {

/*
    // Hack for testing - should output every 5 seconds
    static int count1 = 0;
    if(count1 == 0) {
	cout << "P_atmos = " << p_amb << "  T_atmos = " << T_amb << '\n';
	cout << "Manifold pressure = " << Manifold_Pressure << "  True_Manifold_Pressure = " << True_Manifold_Pressure << '\n';
	cout << "p_amb_sea_level = " << p_amb_sea_level << '\n';
	cout << "equivalence_ratio = " << equivalence_ratio << '\n';
	cout << "combustion_efficiency = " << combustion_efficiency << '\n';
	cout << "AFR = " << 14.7 / equivalence_ratio << '\n';
	cout << "Mixture lever = " << Mixture_Lever_Pos << '\n';
	cout << "n = " << RPM << " rpm\n";
    }
    count1++;
    if(count1 == 600)
	count1 = 0;
*/

    float ManXRPM = 0;
    float Vo = 0;
    float V1 = 0;

    // Set up the new variables
    float PI = 3.1428571;

    // Parameters that alter the operation of the engine.
    int Fuel_Available = 1;	// Yes = 1. Is there Fuel Available. Calculated elsewhere
    int Alternate_Air_Pos =0;	// Off = 0. Reduces power by 3 % for same throttle setting
    int Magneto_Left = 1;	// 1 = On.   Reduces power by 5 % for same power lever settings
    int Magneto_Right = 1;	// 1 = On.  Ditto, Both of the above though do not alter fuel flow


    // Calculate Sea Level Manifold Pressure
    Manifold_Pressure = Calc_Manifold_Pressure( Throttle_Lever_Pos, Max_Manifold_Pressure, Min_Manifold_Pressure );
    // cout << "manifold pressure = " << Manifold_Pressure << endl;

    //Then find the actual manifold pressure (the calculated one is the sea level pressure)
    True_Manifold_Pressure = Manifold_Pressure * p_amb / p_amb_sea_level;

//*************
//DCL - next calculate m_dot_air and m_dot_fuel into engine

    //DCL - calculate mass air flow into engine based on speed and load - separate this out into a function eventually
    //t_amb is actual temperature calculated from altitude
    //calculate density from ideal gas equation
    rho_air = p_amb / ( R_air * T_amb );
    rho_air_manifold = rho_air * Manifold_Pressure / 29.6;  //This is a bit of a roundabout way of calculating this but it works !!  If we put manifold pressure into SI units we could do it simpler.
    //calculate ideal engine volume inducted per second
    swept_volume = (displacement_SI * (RPM / 60)) / 2;  //This equation is only valid for a four stroke engine
    //calculate volumetric efficiency - for now we will just use 0.8, but actually it is a function of engine speed and the exhaust to manifold pressure ratio
    //Note that this is cylinder vol eff - the throttle drop is already accounted for in the MAP calculation
    volumetric_efficiency = 0.8;
    //Now use volumetric efficiency to calculate actual air volume inducted per second
    v_dot_air = swept_volume * volumetric_efficiency;
    //Now calculate mass flow rate of air into engine
    m_dot_air = v_dot_air * rho_air_manifold;

//**************

    //DCL - now calculate fuel flow into engine based on air flow and mixture lever position
    //assume lever runs from no flow at fully out to thi = 1.3 at fully in at sea level
    //also assume that the injector linkage is ideal - hence the set mixture is maintained at a given altitude throughout the speed and load range
    thi_sea_level = 1.3 * ( Mixture_Lever_Pos / 100.0 );
    equivalence_ratio = thi_sea_level * p_amb_sea_level / p_amb; //ie as we go higher the mixture gets richer for a given lever position
    m_dot_fuel = m_dot_air / 14.7 * equivalence_ratio;
    Fuel_Flow_gals_hr = (m_dot_fuel / rho_fuel) * 264.172 * 3600.0;  // Note this assumes US gallons

//***********************************************************************
//Engine power and torque calculations

    // For a given Manifold Pressure and RPM calculate the % Power
    // Multiply Manifold Pressure by RPM
    ManXRPM = True_Manifold_Pressure * RPM;
//    ManXRPM = Manifold_Pressure * RPM;
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
    //  Calculate % Power for Nev's prop model
    //Percentage_Power = (+ 6E-09 * ManXRPM * ManXRPM) + ( + 8E-04 * ManXRPM) - 1.8524;

    // Calculate %power for DCL prop model
    Percentage_Power = (7e-9 * ManXRPM * ManXRPM) + (7e-4 * ManXRPM) - 1.0;

    // Power de-rating for altitude has been removed now that we are basing the power
    // on the actual manifold pressure, which takes air pressure into account.  However - this fails to
    // take the temperature into account - this is TODO.

    //DCL - now adjust power to compensate for mixture
    Percentage_of_best_power_mixture_power = Power_Mixture_Correlation(equivalence_ratio);
    Percentage_Power = Percentage_Power * Percentage_of_best_power_mixture_power / 100.0;

    // Now Derate engine for the effects of Bad/Switched off magnetos
    if (Magneto_Left == 0 && Magneto_Right == 0) {
	// cout << "Both OFF\n";
	Percentage_Power = 0;
    } else if (Magneto_Left && Magneto_Right) {
	// cout << "Both On    ";
    } else if (Magneto_Left == 0 || Magneto_Right== 0) {
	// cout << "1 Magneto Failed   ";
	Percentage_Power = Percentage_Power * ((100.0 - Mag_Derate_Percent)/100.0);
	//  cout << FGEng1_Percentage_Power <<  "%" << "\t";
    }

    HP = Percentage_Power * MaxHP / 100.0;

    Power_SI = HP * CONVERT_HP_TO_WATTS;

    // Calculate Engine Torque. Check for div by zero since percentage power correlation does not guarantee zero power at zero rpm.
    if(RPM == 0) {
        Torque_SI = 0;
    }
    else {
        Torque_SI = (Power_SI * 60.0) / (2.0 * PI * RPM);  //Torque = power / angular velocity
	// cout << Torque << " Nm\n";
    }

//**********************************************************************
//Calculate Exhaust gas temperature

    // cout << "Thi = " << equivalence_ratio << '\n';

    combustion_efficiency = Lookup_Combustion_Efficiency(equivalence_ratio);  //The combustion efficiency basically tells us what proportion of the fuels calorific value is released

    // cout << "Combustion efficiency = " << combustion_efficiency << '\n';

    //now calculate energy release to exhaust
    //We will assume a three way split of fuel energy between useful work, the coolant system and the exhaust system
    //This is a reasonable first suck of the thumb estimate for a water cooled automotive engine - whether it holds for an air cooled aero engine is probably open to question
    //Regardless - it won't affect the variation of EGT with mixture, and we can always put a multiplier on EGT to get a reasonable peak value.
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
// Oil pressure calculation
    
    // Calculate Oil Pressure
    Oil_Pressure = Calc_Oil_Press( Oil_Temp, RPM );
    // cout << "Oil Pressure (PSI) = " << Oil_Pressure << endl;
    
//**************************************************************************************
// Now do the Propeller Calculations
    
    Gear_Ratio = 1.0;
    FGProp1_RPS = RPM * Gear_Ratio / 60.0;  // Borrow this variable from Phils model for now !!
    angular_velocity_SI = 2.0 * PI * RPM / 60.0;
    forward_velocity = IAS * 0.514444444444;        // Convert to m/s
    
    //cout << "Gear_Ratio = " << Gear_Ratio << '\n';
    //cout << "IAS = " << IAS << '\n';
    //cout << "forward_velocity = " << forward_velocity << '\n';
    //cout << "FGProp1_RPS = " << FGProp1_RPS << '\n';
    //cout << "prop_diameter = " << prop_diameter << '\n';
    if(FGProp1_RPS == 0)
        J = 0;
    else
        J = forward_velocity / (FGProp1_RPS * prop_diameter);
    //cout << "advance_ratio = " << J << '\n';
    
    //Cp correlations based on data from McCormick
    Cp_20 = 0.0342*J*J*J*J - 0.1102*J*J*J + 0.0365*J*J - 0.0133*J + 0.064;
    Cp_25 = 0.0119*J*J*J*J - 0.0652*J*J*J + 0.018*J*J - 0.0077*J + 0.0921;
    
    //cout << "Cp_20 = " << Cp_20 << '\n';
    //cout << "Cp_25 = " << Cp_25 << '\n';
    
    //Assume that the blade angle is between 20 and 25 deg - reasonable for fixed pitch prop but won't hold for variable one !!!
    Cp = Cp_20 + ( (Cp_25 - Cp_20) * ((blade_angle - 20)/(25 - 20)) );
    //cout << "Cp = " << Cp << '\n';
    //cout << "RPM = " << RPM << '\n';
    //cout << "angular_velocity_SI = " << angular_velocity_SI << '\n';
    
    prop_power_consumed_SI = Cp * rho_air * pow(FGProp1_RPS,3.0) * pow(prop_diameter,5.0);
    //cout << "prop HP consumed = " << prop_power_consumed_SI / 745.699 << '\n';
    if(angular_velocity_SI == 0)
        prop_torque = 0;
    else
        prop_torque = prop_power_consumed_SI / angular_velocity_SI;
    
    // calculate neta_prop here
    neta_prop_20 = 0.1328*J*J*J*J - 1.3073*J*J*J + 0.3525*J*J + 1.5591*J + 0.0007;
    neta_prop_25 = -0.3121*J*J*J*J + 0.4234*J*J*J - 0.7686*J*J + 1.5237*J - 0.0004;
    neta_prop = neta_prop_20 + ( (neta_prop_25 - neta_prop_20) * ((blade_angle - 20)/(25 - 20)) );
    
    //FIXME - need to check for zero forward velocity to avoid divide by zero
    if(forward_velocity < 0.0001)
        prop_thrust = 0.0;
    else
        prop_thrust = neta_prop * prop_power_consumed_SI / forward_velocity;       //TODO - rename forward_velocity to IAS_SI
    //cout << "prop_thrust = " << prop_thrust << '\n';
    
//******************************************************************************
// Now do the engine - prop torque balance to calculate final RPM
    
    //Calculate new RPM from torque balance and inertia.
    Torque_Imbalance = Torque_SI - prop_torque;  //This gives a +ve value when the engine torque exeeds the prop torque
    
    angular_acceleration = Torque_Imbalance / (engine_inertia + prop_inertia);
    angular_velocity_SI += (angular_acceleration * time_step);
    RPM = (angular_velocity_SI * 60) / (2.0 * PI);
    
    //DCL - stall the engine if RPM drops below 500 - this is possible if mixture lever is pulled right out
    if(RPM < 500)
        RPM = 0;
    
}

