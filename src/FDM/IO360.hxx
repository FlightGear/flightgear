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

#ifndef _IO360_HXX_
#define _IO360_HXX_

#define NEVS_PROP_MODEL

#ifndef NEVS_PROP_MODEL
#define PHILS_PROP_MODEL
#endif 


#include <iostream.h>
#include <fstream.h>
#include <math.h>


class FGEngine {

private:



    float CONVERT_HP_TO_WATTS;
    float CONVERT_CUBIC_INCHES_TO_METERS_CUBED;

    // Control and environment inputs
    float IAS;
    // 0 = Closed, 100 = Fully Open
    float Throttle_Lever_Pos;
    // 0 = Full Course 100 = Full Fine
    float Propeller_Lever_Pos;
    // 0 = Idle Cut Off 100 = Full Rich
    float Mixture_Lever_Pos;

    // Engine Specific Variables used by this program that have limits.
    // Will be set in a parameter file to be read in to create
    // and instance for each engine.
    float Max_Manifold_Pressure;  //will be lower than ambient pressure for a non turbo/super charged engine due to losses through the throttle.  This is the sea level full throttle value.
    float Min_Manifold_Pressure;  //Closed throttle valueat idle - governed by the idle bypass valve
    float Max_RPM;
    float Min_RPM;
    float Max_Fuel_Flow;
    float Mag_Derate_Percent;
    float MaxHP;
    float Gear_Ratio;

    // Initialise Engine Variables used by this instance
    float Percentage_Power;	// Power output as percentage of maximum power output
    float Manifold_Pressure;	// Inches
    float RPM;
    float Fuel_Flow;		// lbs/hour
    float Torque;
    float CHT;			// Cylinder head temperature
    float EGT;			// Exhaust gas temperature
    float Mixture;
    float Oil_Pressure;		// PSI
    float Oil_Temp;		// Deg C
    float HP;			// Current power output in HP
    float Power_SI;		// Current power output in Watts
    float Torque_SI;		// Torque in Nm
    float RPS;
    float Torque_Imbalance;
    float Desired_RPM;		// The RPM that we wish the constant speed prop to maintain if possible
    bool  started;		//flag to indicate the engine is running self sustaining
    bool  cranking;		//flag to indicate the engine is being cranked

    //DCL
    float volumetric_efficiency;
    float combustion_efficiency;
    float equivalence_ratio;
    float v_dot_air;
    float m_dot_air;
    float m_dot_fuel;
    float swept_volume;
    float True_Manifold_Pressure;   //in Hg
    float rho_air_manifold;
    float R_air;
    float p_amb_sea_level;	// Pascals
    float p_amb;		// Pascals
    float T_amb;		// deg Kelvin
    float calorific_value_fuel;
    float thi_sea_level;
    float delta_T_exhaust;
    float displacement;		// Engine displacement in cubic inches - to be read in from config file for each engine
    float displacement_SI;	// ditto in meters cubed
    float Cp_air;		// J/KgK
    float Cp_fuel;		// J/KgK
    float heat_capacity_exhaust;
    float enthalpy_exhaust;
    float Percentage_of_best_power_mixture_power;
    float abstract_mixture;	//temporary hack

    // Initialise Propellor Variables used by this instance
    float FGProp1_Angular_V;
    float FGProp1_Coef_Drag;
    float FGProp1_Torque;
    float FGProp1_Thrust;
    float FGProp1_RPS;
    float FGProp1_Coef_Lift;
    float Alpha1;
    float FGProp1_Blade_Angle;
    float FGProp_Fine_Pitch_Stop;

#ifdef NEVS_PROP_MODEL
    //Extra Propellor variables used by Nev's prop model
    float prop_fudge_factor;
    float prop_torque;	//Nm
    float prop_thrust;
    float blade_length;
    float allowance_for_spinner;
    float num_elements;
    float distance;
    float number_of_blades;
    float forward_velocity;
    float angular_velocity_SI;
    float element;
    float element_drag;
    float element_lift;
    float element_torque;
    float rho_air;
    float prop_power_consumed_SI;
    float prop_power_consumed_HP;
    float theta[6];	//prop angle of each element
#endif // NEVS_PROP_MODEL

    // Other internal values
    float Rho;

    // Calculate Engine RPM based on Propellor Lever Position
    float Calc_Engine_RPM (float Position);

    // Calculate combustion efficiency based on equivalence ratio
    float Lookup_Combustion_Efficiency(float thi_actual);

    // Calculate exhaust gas temperature rise
    float Calculate_Delta_T_Exhaust(void);

public:

    ofstream outfile;

    //constructor
    FGEngine() {
	outfile.open("FGEngine.dat", ios::out|ios::trunc);
    }

    //destructor
    ~FGEngine() {
	outfile.close();
    }

    // set initial default values
    void init();

    // update the engine model based on current control positions
    void update();

    inline void set_IAS( float value ) { IAS = value; }
    inline void set_Throttle_Lever_Pos( float value ) {
	Throttle_Lever_Pos = value;
    }
    inline void set_Propeller_Lever_Pos( float value ) {
	Propeller_Lever_Pos = value;
    }
    inline void set_Mixture_Lever_Pos( float value ) {
	Mixture_Lever_Pos = value;
    }

    // accessors
    inline float get_RPM() const { return RPM; }
    inline float get_Manifold_Pressure() const { return Manifold_Pressure; }
    inline float get_FGProp1_Thrust() const { return FGProp1_Thrust; }
    inline float get_FGProp1_Blade_Angle() const { return FGProp1_Blade_Angle; }

    inline float get_Rho() const { return Rho; }
    inline float get_MaxHP() const { return MaxHP; }
    inline float get_Percentage_Power() const { return Percentage_Power; }
    inline float get_EGT() const { return EGT; }
    inline float get_prop_thrust_SI() const { return prop_thrust; }
};


#endif // _10520D_HXX_
