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

#ifndef _10520D_HXX_
#define _10520D_HXX_


#include <iostream.h>
#include <math.h>


class FGEngine {

private:

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
    float Max_Manifold_Pressure;
    float Max_RPM;
    float Min_RPM;
    float Max_Fuel_Flow;
    float Mag_Derate_Percent;
    float MaxHP;
    float Gear_Ratio;

    // Initialise Engine Variables used by this instance
    float Percentage_Power;
    float Manifold_Pressure;	// Inches
    float RPM;
    float Fuel_Flow;		// lbs/hour
    float Torque;
    float CHT;
    float Mixture;
    float Oil_Pressure;		// PSI
    float Oil_Temp;		// Deg C
    float HP;
    float RPS;
    float Torque_Imbalance;
    float Desired_RPM;

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
    float FGProp_Course_Pitch_Stop;

    // Other internal values
    float Rho;

    // Calculate Engine RPM based on Propellor Lever Position
    float Calc_Engine_RPM (float Position);

public:

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
    inline float get_FGProp1_Thrust() const { return FGProp1_Thrust; }

    inline float get_Rho() const { return Rho; }
};


#endif _10520D_HXX_
