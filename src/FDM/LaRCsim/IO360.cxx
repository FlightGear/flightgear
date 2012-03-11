// IO360.cxx - a piston engine model currently for the IO360 engine fitted to the C172
//             but with the potential to model other naturally aspirated piston engines
//             given appropriate config input.
//
// Written by David Luff, started 2000.
// Based on code by Phil Schubert, started 1999.
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <simgear/compiler.h>

#include <math.h>

#include <fstream>
#include <iostream>

#include <Main/fg_props.hxx>

#include "IO360.hxx"
#include "ls_constants.h"


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

    // environment inputs
    p_amb_sea_level = 101325;	// Pascals		

    // Control inputs  - ARE THESE NEEDED HERE???
    Throttle_Lever_Pos = 75;
    Propeller_Lever_Pos = 75;
    Mixture_Lever_Pos = 100;

    //misc
    IAS = 0;
    time_step = dt;

    // Engine Specific Variables that should be read in from a config file
    MaxHP = 200;    //Lycoming IO360 -A-C-D series
//  MaxHP = 180;    //Current Lycoming IO360 ?
//  displacement = 520;  //Continental IO520-M
    displacement = 360;	 //Lycoming IO360
    displacement_SI = displacement * CONVERT_CUBIC_INCHES_TO_METERS_CUBED;
    engine_inertia = 0.2;  //kgm^2 - value taken from a popular family saloon car engine - need to find an aeroengine value !!!!!
    prop_inertia = 0.05;  //kgm^2 - this value is a total guess - dcl
    Max_Fuel_Flow = 130;  // Units??? Do we need this variable any more??

    // Engine specific variables that maybe should be read in from config but are pretty generic and won't vary much for a naturally aspirated piston engine.
    Max_Manifold_Pressure = 28.50;  //Inches Hg. An approximation - should be able to find it in the engine performance data
    Min_Manifold_Pressure = 6.5;    //Inches Hg. This is a guess corresponding to approx 0.24 bar MAP (7 in Hg) - need to find some proper data for this
    Max_RPM = 2700;
    Min_RPM = 600;		    //Recommended idle from Continental data sheet
    Mag_Derate_Percent = 5;
    Gear_Ratio = 1;
    n_R = 2;         // Number of crank revolutions per power cycle - 2 for a 4 stroke engine.

    // Various bits of housekeeping describing the engines initial state.
    running = false;
    cranking = false;
    crank_counter = false;
    starter = false;

    // Initialise Engine Variables used by this instance
    if(running)
	RPM = 600;
    else
	RPM = 0;
    Percentage_Power = 0;
    Manifold_Pressure = 29.96; // Inches
    Fuel_Flow_gals_hr = 0;
//    Torque = 0;
    Torque_SI = 0;
    CHT = 298.0;			//deg Kelvin
    CHT_degF = (CHT * 1.8) - 459.67;	//deg Fahrenheit
    Mixture = 14;
    Oil_Pressure = 0;	// PSI
    Oil_Temp = 85;	// Deg C
    current_oil_temp = 298.0;	//deg Kelvin
    /**** one of these is superfluous !!!!***/
    HP = 0;
    RPS = 0;
    Torque_Imbalance = 0;

    // Initialise Propellor Variables used by this instance
    FGProp1_RPS = 0;
    // Hardcode propellor for now - the following two should be read from config eventually
    prop_diameter = 1.8;         // meters
    blade_angle = 23.0;          // degrees
}

//*****************************************************************************
// update the engine model based on current control positions
void FGNewEngine::update() {

/*
    // Hack for testing - should output every 5 seconds
    static int count1 = 0;
    if(count1 == 0) {
//	cout << "P_atmos = " << p_amb << "  T_atmos = " << T_amb << '\n';
//	cout << "Manifold pressure = " << Manifold_Pressure << "  True_Manifold_Pressure = " << True_Manifold_Pressure << '\n';
//	cout << "p_amb_sea_level = " << p_amb_sea_level << '\n';
//	cout << "equivalence_ratio = " << equivalence_ratio << '\n';
//	cout << "combustion_efficiency = " << combustion_efficiency << '\n';
//	cout << "AFR = " << 14.7 / equivalence_ratio << '\n';
//	cout << "Mixture lever = " << Mixture_Lever_Pos << '\n';
//	cout << "n = " << RPM << " rpm\n";
//      cout << "T_amb = " << T_amb << '\n';
//	cout << "running = " << running << '\n';
//	cout << "fuel = " << fgGetFloat("/consumables/fuel/tank[0]/level-gal_us") << '\n';
//	cout << "Percentage_Power = " << Percentage_Power << '\n';
//	cout << "current_oil_temp = " << current_oil_temp << '\n';
//	cout << "EGT = " << EGT << '\n';
    }
    count1++;
    if(count1 == 100)
	count1 = 0;
*/

    // Check parameters that may alter the operating state of the engine. 
    // (spark, fuel, starter motor etc)

    // Check for spark
    bool Magneto_Left = false;
    bool Magneto_Right = false;
    // Magneto positions:
    // 0 -> off
    // 1 -> left only
    // 2 -> right only
    // 3 -> both
    if(mag_pos != 0) {
	spark = true;
    } else {
	spark = false;
    }  // neglects battery voltage, master on switch, etc for now.
    if((mag_pos == 1) || (mag_pos > 2)) 
	Magneto_Left = true;
    if(mag_pos > 1)
	Magneto_Right = true;
 
    // crude check for fuel
    if((fgGetFloat("/consumables/fuel/tank[0]/level-gal_us") > 0) || (fgGetFloat("/consumables/fuel/tank[1]/level-gal_us") > 0)) {
	fuel = true;
    } else {
	fuel = false;
    }  // Need to make this better, eg position of fuel selector switch.

    // Check if we are turning the starter motor
    if(cranking != starter) {
	// This check saves .../cranking from getting updated every loop - they only update when changed.
	cranking = starter;
	crank_counter = 0;
    }
    // Note that although /engines/engine[0]/starter and /engines/engine[0]/cranking might appear to be duplication it is
    // not since the starter may be engaged with the battery voltage too low for cranking to occur (or perhaps the master 
    // switch just left off) and the sound manager will read .../cranking to determine wether to play a cranking sound.
    // For now though none of that is implemented so cranking can be set equal to .../starter without further checks.

//    int Alternate_Air_Pos =0;	// Off = 0. Reduces power by 3 % for same throttle setting
    // DCL - don't know what this Alternate_Air_Pos is - this is a leftover from the Schubert code.

    //Check mode of engine operation
    if(cranking) {
	crank_counter++;
	if(RPM <= 480) {
	    RPM += 100;
	    if(RPM > 480)
		RPM = 480;
	} else {
	    // consider making a horrible noise if the starter is engaged with the engine running
	}
    }
    if((!running) && (spark) && (fuel) && (crank_counter > 120)) {
	// start the engine if revs high enough
	if(RPM > 450) {
	    // For now just instantaneously start but later we should maybe crank for a bit
	    running = true;
//	    RPM = 600;
	}
    }
    if( (running) && ((!spark)||(!fuel)) ) {
	// Cut the engine
	// note that we only cut the power - the engine may continue to spin if the prop is in a moving airstream
	running = false;
    }

    // Now we've ascertained whether the engine is running or not we can start to do the engine calculations 'proper'

    // Calculate Sea Level Manifold Pressure
    Manifold_Pressure = Calc_Manifold_Pressure( Throttle_Lever_Pos, Max_Manifold_Pressure, Min_Manifold_Pressure );
    // cout << "manifold pressure = " << Manifold_Pressure << endl;

    //Then find the actual manifold pressure (the calculated one is the sea level pressure)
    True_Manifold_Pressure = Manifold_Pressure * p_amb / p_amb_sea_level;

    //Do the fuel flow calculations
    Calc_Fuel_Flow_Gals_Hr();

    //Calculate engine power
    Calc_Percentage_Power(Magneto_Left, Magneto_Right);
    HP = Percentage_Power * MaxHP / 100.0;
    Power_SI = HP * CONVERT_HP_TO_WATTS;

    // FMEP calculation.  For now we will just use this during motored operation.
    // Eventually we will calculate IMEP and use the FMEP all the time to give BMEP (maybe!)
    if(!running) {
        // This FMEP data is from the Patton paper, assumes fully warm conditions.
        FMEP = 1e-12*pow(RPM,4) - 1e-8*pow(RPM,3) + 5e-5*pow(RPM,2) - 0.0722*RPM + 154.85;
        // Gives FMEP in kPa - now convert to Pa
        FMEP *= 1000;
    } else {
        FMEP = 0.0;
    }
    // Is this total FMEP or friction FMEP ???

    Torque_FMEP = (FMEP * displacement_SI) / (2.0 * LS_PI * n_R);

    // Calculate Engine Torque. Check for div by zero since percentage power correlation does not guarantee zero power at zero rpm.
    // However this is problematical since there is a resistance to movement even at rest
    // Ie this is a dynamics equation not a statics one.  This can be solved by going over to MEP based torque calculations.
    if(RPM == 0) {
        Torque_SI = 0 - Torque_FMEP;
    }
    else {
        Torque_SI = ((Power_SI * 60.0) / (2.0 * LS_PI * RPM)) - Torque_FMEP;  //Torque = power / angular velocity
	// cout << Torque << " Nm\n";
    }

    //Calculate Exhaust gas temperature
    if(running)
	Calc_EGT();
    else
	EGT = 298.0;

    // Calculate Cylinder Head Temperature
    Calc_CHT();
    
    // Calculate oil temperature
    current_oil_temp = Calc_Oil_Temp(current_oil_temp);
    
    // Calculate Oil Pressure
    Oil_Pressure = Calc_Oil_Press( Oil_Temp, RPM );
    
    // Now do the Propeller Calculations
    Do_Prop_Calcs();
    
// Now do the engine - prop torque balance to calculate final RPM
    
    //Calculate new RPM from torque balance and inertia.
    Torque_Imbalance = Torque_SI - prop_torque;  //This gives a +ve value when the engine torque exeeds the prop torque
    // (Engine torque is +ve when it acts in the direction of engine revolution, prop torque is +ve when it opposes the direction of engine revolution)
    
    angular_acceleration = Torque_Imbalance / (engine_inertia + prop_inertia);
    angular_velocity_SI += (angular_acceleration * time_step);
    // Don't let the engine go into reverse
    if(angular_velocity_SI < 0)
        angular_velocity_SI = 0;
    RPM = (angular_velocity_SI * 60) / (2.0 * LS_PI);

    // And finally a last check on the engine state after doing the torque balance with the prop - have we stalled?
    if(running) { 
	//Check if we have stalled the engine
	if (RPM == 0) {
	    running = false;
	} else if((RPM <= 480) && (cranking)) {
	    //Make sure the engine noise dosn't play if the engine won't start due to eg mixture lever pulled out.
	    running = false;
	    EGT = 298.0;
	}
    }

    // And finally, do any unit conversions from internal units to output units
    EGT_degF = (EGT * 1.8) - 459.67;
    CHT_degF = (CHT * 1.8) - 459.67;
}

//*****************************************************************************************************

// FGNewEngine member functions

////////////////////////////////////////////////////////////////////////////////////////////
// Return the combustion efficiency as a function of equivalence ratio
//
// Combustion efficiency values from Heywood, 
// "Internal Combustion Engine Fundamentals", ISBN 0-07-100499-8
////////////////////////////////////////////////////////////////////////////////////////////
float FGNewEngine::Lookup_Combustion_Efficiency(float thi_actual)
{
    const int NUM_ELEMENTS = 11;
    float thi[NUM_ELEMENTS] = {0.0, 0.9, 1.0, 1.05, 1.1, 1.15, 1.2, 1.3, 1.4, 1.5, 1.6};  //array of equivalence ratio values
    float neta_comb[NUM_ELEMENTS] = {0.98, 0.98, 0.97, 0.95, 0.9, 0.85, 0.79, 0.7, 0.63, 0.57, 0.525};  //corresponding array of combustion efficiency values
    float neta_comb_actual = 0.0f;
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
    SG_LOG(SG_AIRCRAFT, SG_ALERT, "error in FGNewEngine::Lookup_Combustion_Efficiency");
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
    float mixPerPow_actual = 0.0f;
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
	    dydx = (mixPerPow[1] - mixPerPow[0]) / (AFR[1] - AFR[0]);
	    mixPerPow_actual = mixPerPow[0] + dydx * (AFR_actual - AFR[0]);
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
    SG_LOG(SG_AIRCRAFT, SG_ALERT, "error in FGNewEngine::Power_Mixture_Correlation");
    return mixPerPow_actual;
}

// Calculate Cylinder Head Temperature
// Crudely models the cylinder head as an arbitary lump of arbitary size and area with one third of combustion energy
// as heat input and heat output as a function of airspeed and temperature.  Could be improved!!!
void FGNewEngine::Calc_CHT()
{
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

    // The above values are hardwired to give reasonable results for an IO360 (C172 engine)
    // Now adjust to try to compensate for arbitary sized engines - this is currently untested
    arbitary_area *= (MaxHP / 180.0);
    MassCylinderHead *= (MaxHP / 180.0);

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
}

// Calculate exhaust gas temperature
void FGNewEngine::Calc_EGT()
{
    combustion_efficiency = Lookup_Combustion_Efficiency(equivalence_ratio);  //The combustion efficiency basically tells us what proportion of the fuels calorific value is released

    //now calculate energy release to exhaust
    //We will assume a three way split of fuel energy between useful work, the coolant system and the exhaust system
    //This is a reasonable first suck of the thumb estimate for a water cooled automotive engine - whether it holds for an air cooled aero engine is probably open to question
    //Regardless - it won't affect the variation of EGT with mixture, and we can always put a multiplier on EGT to get a reasonable peak value.
    enthalpy_exhaust = m_dot_fuel * calorific_value_fuel * combustion_efficiency * 0.33;
    heat_capacity_exhaust = (Cp_air * m_dot_air) + (Cp_fuel * m_dot_fuel);
    delta_T_exhaust = enthalpy_exhaust / heat_capacity_exhaust;

    EGT = T_amb + delta_T_exhaust;

    //The above gives the exhaust temperature immediately prior to leaving the combustion chamber
    //Now derate to give a more realistic figure as measured downstream
    //For now we will aim for a peak of around 400 degC (750 degF)

    EGT *= 0.444 + ((0.544 - 0.444) * Percentage_Power / 100.0);
}

// Calculate Manifold Pressure based on Throttle lever Position
float FGNewEngine::Calc_Manifold_Pressure ( float LeverPosn, float MaxMan, float MinMan)
{
    float Inches;

    //Note that setting the manifold pressure as a function of lever position only is not strictly accurate
    //MAP is also a function of engine speed. (and ambient pressure if we are going for an actual MAP model)
    Inches = MinMan + (LeverPosn * (MaxMan - MinMan) / 100);

    //allow for idle bypass valve or slightly open throttle stop
    if(Inches < MinMan)
	Inches = MinMan;

    //Check for stopped engine (crudest way of compensating for engine speed)
    if(RPM == 0)
	Inches = 29.92;

    return Inches;
}

// Calculate fuel flow in gals/hr
void FGNewEngine::Calc_Fuel_Flow_Gals_Hr()
{
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
}

// Calculate the percentage of maximum rated power delivered as a function of Manifold pressure multiplied by engine speed (rpm)
// This is not necessarilly the best approach but seems to work for now.
// May well need tweaking at the bottom end if the prop model is changed.
void FGNewEngine::Calc_Percentage_Power(bool mag_left, bool mag_right)
{
    // For a given Manifold Pressure and RPM calculate the % Power
    // Multiply Manifold Pressure by RPM
    float ManXRPM = True_Manifold_Pressure * RPM;

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

    // Adjust power for temperature - this is temporary until the power is done as a function of mass flow rate induced
    // Adjust for Temperature - Temperature above Standard decrease
    // power by 7/120 % per degree F increase, and incease power for
    // temps below at the same ratio
    float T_amb_degF = (T_amb * 1.8) - 459.67;
    float T_amb_sea_lev_degF = (288 * 1.8) - 459.67; 
    Percentage_Power = Percentage_Power + ((T_amb_sea_lev_degF - T_amb_degF) * 7 /120);

    //DCL - now adjust power to compensate for mixture
    Percentage_of_best_power_mixture_power = Power_Mixture_Correlation(equivalence_ratio);
    Percentage_Power = Percentage_Power * Percentage_of_best_power_mixture_power / 100.0;

    // Now Derate engine for the effects of Bad/Switched off magnetos
    //if (Magneto_Left == 0 && Magneto_Right == 0) {
    if(!running) {
	// cout << "Both OFF\n";
	Percentage_Power = 0;
    } else if (mag_left && mag_right) {
	// cout << "Both On    ";
    } else if (mag_left == 0 || mag_right== 0) {
	// cout << "1 Magneto Failed   ";
	Percentage_Power = Percentage_Power * ((100.0 - Mag_Derate_Percent)/100.0);
	//  cout << FGEng1_Percentage_Power <<  "%" << "\t";
    }
/*
    //DCL - stall the engine if RPM drops below 450 - this is possible if mixture lever is pulled right out
    //This is a kludge that I should eliminate by adding a total fmep estimation.
    if(RPM < 450)
        Percentage_Power = 0;
*/
    if(Percentage_Power < 0)
	Percentage_Power = 0;
}

// Calculate Oil Temperature in degrees Kelvin
float FGNewEngine::Calc_Oil_Temp (float oil_temp)
{
    float idle_percentage_power = 2.3;	// approximately
    float target_oil_temp;	    // Steady state oil temp at the current engine conditions
    float time_constant;	    // The time constant for the differential equation
    if(running) {
	target_oil_temp = 363;
	time_constant = 500;	    // Time constant for engine-on idling.
	if(Percentage_Power > idle_percentage_power) {
	    time_constant /= ((Percentage_Power / idle_percentage_power) / 10.0);	// adjust for power 
	}
    } else {
	target_oil_temp = 298;
	time_constant = 1000;  // Time constant for engine-off; reflects the fact that oil is no longer getting circulated
    }

    float dOilTempdt = (target_oil_temp - oil_temp) / time_constant;

    oil_temp += (dOilTempdt * time_step);

    return (oil_temp);
}

// Calculate Oil Pressure
float FGNewEngine::Calc_Oil_Press (float Oil_Temp, float Engine_RPM)
{
    float Oil_Pressure = 0;			//PSI
    float Oil_Press_Relief_Valve = 60;	//PSI
    float Oil_Press_RPM_Max = 1800;
    float Design_Oil_Temp = 85;		//Celsius
    float Oil_Viscosity_Index = 0.25;	// PSI/Deg C
//    float Temp_Deviation = 0;		// Deg C

    Oil_Pressure = (Oil_Press_Relief_Valve / Oil_Press_RPM_Max) * Engine_RPM;

    // Pressure relief valve opens at Oil_Press_Relief_Valve PSI setting
    if (Oil_Pressure >= Oil_Press_Relief_Valve) {
	Oil_Pressure = Oil_Press_Relief_Valve;
    }

    // Now adjust pressure according to Temp which affects the viscosity

    Oil_Pressure += (Design_Oil_Temp - Oil_Temp) * Oil_Viscosity_Index;

    return Oil_Pressure;
}


// Propeller calculations.
void FGNewEngine::Do_Prop_Calcs()
{
    float Gear_Ratio = 1.0;
    float forward_velocity;             // m/s
    float prop_power_consumed_SI;       // Watts
    double J;      			// advance ratio - dimensionless
    double Cp_20;                   // coefficient of power for 20 degree blade angle
    double Cp_25;                   // coefficient of power for 25 degree blade angle
    double Cp;                      // Our actual coefficient of power
    double neta_prop_20;
    double neta_prop_25;
    double neta_prop;               // prop efficiency

    FGProp1_RPS = RPM * Gear_Ratio / 60.0; 
    angular_velocity_SI = 2.0 * LS_PI * RPM / 60.0;
    forward_velocity = IAS * 0.514444444444;        // Convert to m/s
    
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
    
    prop_power_consumed_SI = Cp * rho_air * pow(FGProp1_RPS,3.0f) * pow(float(prop_diameter),5.0f);
    //cout << "prop HP consumed = " << prop_power_consumed_SI / 745.699 << '\n';
    if(angular_velocity_SI == 0)
        prop_torque = 0;
	// However this can give problems - if rpm == 0 but forward velocity increases the prop should be able to generate a torque to start the engine spinning
	// Unlikely to happen in practice - but I suppose someone could move the lever of a stopped large piston engine from feathered to windmilling.
        // This *does* give problems - if the plane is put into a steep climb whilst windmilling the engine friction will eventually stop it spinning.
        // When put back into a dive it never starts re-spinning again.  Although it is unlikely that anyone would do this in real life, they might well do it in a sim!!!
    else
        prop_torque = prop_power_consumed_SI / angular_velocity_SI;
    
    // calculate neta_prop here
    neta_prop_20 = 0.1328*J*J*J*J - 1.3073*J*J*J + 0.3525*J*J + 1.5591*J + 0.0007;
    neta_prop_25 = -0.3121*J*J*J*J + 0.4234*J*J*J - 0.7686*J*J + 1.5237*J - 0.0004;
    neta_prop = neta_prop_20 + ( (neta_prop_25 - neta_prop_20) * ((blade_angle - 20)/(25 - 20)) );
    
    // Check for zero forward velocity to avoid divide by zero
    if(forward_velocity < 0.0001)
        prop_thrust = 0.0;
	// I don't see how this works - how can the plane possibly start from rest ???
	// Hmmmm - it works because the forward_velocity at present never drops below about 0.03 even at rest
	// We can't really rely on this in the future - needs fixing !!!!
	// The problem is that we're doing this calculation backwards - we're working out the thrust from the power consumed and the velocity, which becomes invalid as velocity goes to zero.
	// It would be far more natural to work out the power consumed from the thrust - FIXME!!!!!.
    else
        prop_thrust = neta_prop * prop_power_consumed_SI / forward_velocity;       //TODO - rename forward_velocity to IAS_SI
    //cout << "prop_thrust = " << prop_thrust << '\n';
}
