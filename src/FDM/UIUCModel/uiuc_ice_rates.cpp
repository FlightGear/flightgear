//#include <ansi_c.h>
//#include <math.h>
//#include <stdio.h>
//#include <stdlib.h>
#include "uiuc_ice_rates.h"

///////////////////////////////////////////////////////////////////////
// Calculates shed rate depending on current aero loads, eta, temp, and freezing fraction
// Code by Leia Blumenthal
//
// 13 Feb 02 - Created basic program with dummy variables and a constant shed rate (no dependency)
//
// Inputs:
// aero_load - aerodynamic load
// eta 
// T - Temperature in Farenheit
// ff - freezing fraction
//
// Output:
// rate - %eta shed/time
//
// Right now this is just a constant shed rate until we learn more...


double shed(double aero_load, double eta, double T, double ff, double time_step)
{
	double rate, eta_new;

	if (eta <= 0.0)
	  rate = 0.0;
	else
	  rate =  0.2;

	eta_new = eta-rate*eta*time_step;
	if (eta_new <= 0.0)
	  eta_new = 0.0;
	
	return(eta_new);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// Currently a simple linear approximation based on temperature and eta, but for next version, 
// should have so that it calculates sublimation rate depending on current temp,pressure, 
// dewpoint, radiation, and eta
//
// Code by Leia Blumenthal  
// 12 Feb 02 - Created basic program with linear rate for values when sublimation will occur
// 16 May 02 - Modified so that outputs new eta as opposed to rate
// Inputs:
// T - temperature and must be input in Farenheit
// P - pressure
// Tdew - Dew point Temperature
// rad - radiation
// time_step- increment since last run
//
// Intermediate:
// rate - sublimation rate (% eta change/time)
//
// Output:
// eta_new- eta after sublimation has occurred
//
// This takes a simple approximation that the rate of sublimation will decrease
// linearly with temperature increase.
//
// This code should be run every time step to every couple time steps 
//
// If eta is less than zero, than there should be no sublimation

double sublimation(double T, double eta, double time_step)
{
	double rate, eta_new;
	
	if (eta <= 0.0) rate = 0;
	
	else{  
	   // According to the Smithsonian Meteorological tables sublimation occurs
	   // between -40 deg F < T < 32 deg F and between pressures of 0 atm < P < 0.00592 atm
	   if (T < -40) rate = 0;
	   else if (T >= -40 && T < 32)
	   {
	   // For a simple linear approximation, assume largest value is a rate of .2% per sec
	      rate = 0.0028 * T + 0.0889;
	   }
	   else if (T >= 32) rate = 0;
	}

	eta_new = eta-rate*eta*time_step;
	if (eta_new <= 0.0)
	  eta_new = 0.0;
	
	return(eta_new);
}      
