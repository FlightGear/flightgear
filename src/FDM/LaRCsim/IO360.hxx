// IO360.hxx - a piston engine model currently for the IO360 engine fitted to the C172
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


#ifndef _IO360_HXX_
#define _IO360_HXX_

#include <simgear/compiler.h>

#include <math.h>

class FGNewEngine {

private:

    // These constants should probably be moved eventually
    float CONVERT_CUBIC_INCHES_TO_METERS_CUBED;
    float CONVERT_HP_TO_WATTS;

    // Properties of working fluids
    float Cp_air;		// J/KgK
    float Cp_fuel;		// J/KgK
    float calorific_value_fuel; // W/Kg
    float rho_fuel;		// kg/m^3
    float rho_air;		// kg/m^3

    // environment inputs
    float p_amb_sea_level;	// Sea level ambient pressure in Pascals
    float p_amb;		// Ambient pressure at current altitude in Pascals
    float T_amb;		// ditto deg Kelvin

    // Control inputs
    float Throttle_Lever_Pos;	// 0 = Closed, 100 = Fully Open
    float Propeller_Lever_Pos;	// 0 = Full Course 100 = Full Fine
    float Mixture_Lever_Pos;	// 0 = Idle Cut Off 100 = Full Rich
    int mag_pos;		// 0=off, 1=left, 2=right, 3=both.
    bool starter;

    //misc
    float IAS;
    double time_step;

    // Engine Specific Variables that should be read in from a config file
    float MaxHP;		// Horsepower
    float displacement;		// Cubic inches
    float displacement_SI;	//m^3 (derived from above rather than read in)
    float engine_inertia;	//kg.m^2
    float prop_inertia;		//kg.m^2
    float Max_Fuel_Flow;	// Units??? Do we need this variable any more??

    // Engine specific variables that maybe should be read in from config but are pretty generic and won't vary much for a naturally aspirated piston engine.
    float Max_Manifold_Pressure;    // inches Hg - typical manifold pressure at full throttle and typical max rpm
    float Min_Manifold_Pressure;    // inches Hg - typical manifold pressure at idle (closed throttle)
    float Max_RPM;		// rpm - this is really a bit of a hack and could be make redundant if the prop model works properly and takes tips at speed of sound into account.
    float Min_RPM;		// rpm - possibly redundant ???
    float Mag_Derate_Percent;	// Percentage reduction in power when mags are switched from 'both' to either 'L' or 'R'
    float Gear_Ratio;		// Gearing between engine and propellor
    float n_R;                  // Number of cycles per power stroke - 2 for a 4 stroke engine.

    // Engine Variables not read in from config file
    float RPM;			// rpm
    float Percentage_Power;	// Power output as percentage of maximum power output
    float Manifold_Pressure;	// Inches Hg
    float Fuel_Flow_gals_hr;	// USgals/hour
    float Torque_lbft;		// lb-ft		
    float Torque_SI;		// Nm
    float CHT;			// Cylinder head temperature deg K
    float CHT_degF;		// Ditto in deg Fahrenheit
    float Mixture;
    float Oil_Pressure;		// PSI
    float Oil_Temp;		// Deg C
    float current_oil_temp;	// deg K
    /**** one of these is superfluous !!!!***/
    float HP;			// Current power output in HP
    float Power_SI;		// Current power output in Watts
    float RPS;
    float angular_velocity_SI;  // rad/s
    float Torque_FMEP;          // The component of Engine torque due to FMEP (Nm)
    float Torque_Imbalance;	// difference between engine and prop torque
    float EGT;			// Exhaust gas temperature deg K
    float EGT_degF;		// Exhaust gas temperature deg Fahrenheit
    float volumetric_efficiency;
    float combustion_efficiency;
    float equivalence_ratio;	// ratio of stoichiometric AFR over actual AFR 
    float thi_sea_level;	// the equivalence ratio we would be running at assuming sea level air denisity in the manifold
    float v_dot_air;		// volume flow rate of air into engine  - m^3/s
    float m_dot_air;		// mass flow rate of air into engine - kg/s
    float m_dot_fuel;		// mass flow rate of fuel into engine - kg/s
    float swept_volume;		// total engine swept volume - m^3
    /********* swept volume or the geometry used to calculate it should be in the config read section surely ??? ******/
    float True_Manifold_Pressure;   //in Hg - actual manifold gauge pressure
    float rho_air_manifold;	// denisty of air in the manifold - kg/m^3
    float R_air;		// Gas constant of air (287) UNITS???
    float delta_T_exhaust;	// Temperature change of exhaust this time step - degK
    float heat_capacity_exhaust;    // exhaust gas specific heat capacity - J/kgK
    float enthalpy_exhaust;	    // Enthalpy at current exhaust gas conditions - UNITS???
    float Percentage_of_best_power_mixture_power;   // Current power as a percentage of what power we would have at the same conditions but at best power mixture
    float abstract_mixture;	//temporary hack
    float angular_acceleration;	//rad/s^2
    float FMEP;                 //Friction Mean Effective Pressure (Pa)

    // Various bits of housekeeping describing the engines state.
    bool  running;		// flag to indicate the engine is running self sustaining
    bool  cranking;		// flag to indicate the engine is being cranked
    int   crank_counter;	// Number of iterations that the engine has been cranked non-stop
    bool  spark;		// flag to indicate a spark is available
    bool  fuel;			// flag to indicate fuel is available

    // Propellor Variables
    float FGProp1_RPS;		// rps
    float prop_torque;		// Nm
    float prop_thrust; 		// Newtons
    double prop_diameter;       // meters
    double blade_angle;         // degrees


// MEMBER FUNCTIONS
    
    // Calculate Engine RPM based on Propellor Lever Position
    float Calc_Engine_RPM(float Position);

    // Calculate Manifold Pressure based on throttle lever position
    // Note that this is simplistic and needs altering to include engine speed effects
    float Calc_Manifold_Pressure( float LeverPosn, float MaxMan, float MinMan);

    // Calculate combustion efficiency based on equivalence ratio
    float Lookup_Combustion_Efficiency(float thi_actual);

    // Calculate percentage of best power mixture power based on equivalence ratio
    float Power_Mixture_Correlation(float thi_actual);

    // Calculate exhaust gas temperature change
    float Calculate_Delta_T_Exhaust(void);

    // Calculate cylinder head temperature
    void Calc_CHT(void);

    // Calculate exhaust gas temperature
    void Calc_EGT(void);

    // Calculate fuel flow in gals/hr
    void Calc_Fuel_Flow_Gals_Hr(void);

    // Calculate current percentage power
    void Calc_Percentage_Power(bool mag_left, bool mag_right);

    // Calculate Oil Temperature
    float Calc_Oil_Temp (float oil_temp);
    
    // Calculate Oil Pressure
    float Calc_Oil_Press (float Oil_Temp, float Engine_RPM);

    // Propeller calculations.
    void Do_Prop_Calcs(void);

public:

//    ofstream outfile;

    //constructor
    FGNewEngine() {
//	outfile.open("FGNewEngine.dat", ios::out|ios::trunc);
    }

    //destructor
    ~FGNewEngine() {
//	outfile.close();
    }

    // set initial default values
    void init(double dt);

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
    // set the magneto switch position
    inline void set_Magneto_Switch_Pos( int value ) {
	mag_pos = value;
    }
    inline void setStarterFlag( bool flag ) {
	starter = flag;
    }
    // set ambient pressure - takes pounds per square foot
    inline void set_p_amb( float value ) { 
	p_amb = value * 47.88026;
	// Convert to Pascals
    }
    // set ambient temperature - takes degrees Rankine
    inline void set_T_amb( float value ) { 
	T_amb = value * 0.555555555556;
	// Convert to degrees Kelvin
    }

    // accessors
    inline float get_RPM() const { return RPM; }
    inline float get_Manifold_Pressure() const { return True_Manifold_Pressure; }
 //   inline float get_Rho() const { return Rho; }
    inline float get_MaxHP() const { return MaxHP; }
    inline float get_Percentage_Power() const { return Percentage_Power; }
    inline float get_EGT() const { return EGT_degF; }	 // Returns EGT in Fahrenheit
    inline float get_CHT() const { return CHT_degF; }    // Note this returns CHT in Fahrenheit
    inline float get_prop_thrust_SI() const { return prop_thrust; }
    inline float get_prop_thrust_lbs() const { return (prop_thrust * 0.2248); }
    inline float get_fuel_flow_gals_hr() const { return (Fuel_Flow_gals_hr); }
    inline float get_oil_temp() const { return ((current_oil_temp * 1.8) - 459.67); }
    inline bool getRunningFlag() const { return running; }
    inline bool getCrankingFlag() const { return cranking; }
    inline bool getStarterFlag() const { return starter; }
};


#endif // _IO360_HXX_
