/*****************************************************************************

 Header:       FGWeatherUtils.h	
 Author:       Christian Mayer
 Date started: 28.05.99

 -------- Copyright (C) 1999 Christian Mayer (fgfs@christianmayer.de) --------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 details.

 You should have received a copy of the GNU General Public License along with
 this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 Place - Suite 330, Boston, MA  02111-1307, USA.

 Further information about the GNU General Public License can also be found on
 the world wide web at http://www.gnu.org.

FUNCTIONAL DESCRIPTION
------------------------------------------------------------------------------
Utilities for the weather calculation like converting formulas

HISTORY
------------------------------------------------------------------------------
02.06.1999 Christian Mayer      Created
08.06.1999 Christian Mayer      Changed sat_vp
16.06.1999 Durk Talsma		Portability for Linux
20.06.1999 Christian Mayer	added lots of consts
11.10.1999 Christian Mayer	changed set<> to map<> on Bernie Bright's 
				suggestion
19.10.1999 Christian Mayer	change to use PLIB's sg instead of Point[2/3]D
				and lots of wee code cleaning
17.01.2000 Christian Mayer	Added conversion routines make it easier for
				JSBsim to use the weather database.
*****************************************************************************/

/****************************************************************************/
/* SENTRY                                                                   */
/****************************************************************************/
#ifndef FGWeatherUtils_H
#define FGWeatherUtils_H

/****************************************************************************/
/* INCLUDES								    */
/****************************************************************************/
#include <math.h>
		
/****************************************************************************/
/* DEFINES								    */
/****************************************************************************/

/****************************************************************************/
/*assuming as given:							    */
/*									    */
/* t:	    temperature in °C						    */
/* p:	    preasure in mbar						    */
/* //abs_hum: absoloute humidity in g/m^3,				    */
/* act_vp:  actual vapor pressure pascal				    */
/*									    */
/* Calculated vaues:							    */
/* //max_hum: maximum of humidity in g/m^3,				    */
/* sat_vp:  saturated vapor pressure in pascal				    */
/* rel_hum: relative humidity in %					    */
/* dp:	    dew point in °C						    */
/* wb:	    approximate wetbulp in °C					    */
/*									    */
/* NOTE: Pascal is the SI unit for preasure and is defined as Pa = N/m^2    */
/*       1 mbar = 1 hPa = 100 Pa					    */
/* NOTE: °C isn't a SI unit, so I should use °K instead. But as all	    */
/*       formulas are given in °C and the weather database only uses	    */
/*       'normal' temperatures, I've kept it in °C			    */
/****************************************************************************/

#define SAT_VP_CONST1 610.483125
#define SAT_VP_CONST2 7.444072452
#define SAT_VP_CONST3 235.3120919

inline WeatherPrecision sat_vp(const WeatherPrecision temp)
{
    //old:
    //return 6.112 * pow( 10, (7.5*dp)/(237.7+dp) );    //in mbar

    //new:
    //advantages: return the result as SI unit pascal and the constants
    //are choosen that the correct results are returned for 0°C, 20°C and
    //100°C. By 100°C I'm now returning a preasure of 1013.25 hPa
    return SAT_VP_CONST1 * pow( 10, (SAT_VP_CONST2*temp)/(SAT_VP_CONST3+temp) );    //in pascal
}

inline WeatherPrecision rel_hum(const WeatherPrecision act_vp, const WeatherPrecision sat_vp)
{
    return (act_vp / sat_vp) * 100;	//in %
}

inline WeatherPrecision dp(const WeatherPrecision sat_vp)
{
    return (SAT_VP_CONST3*log10(sat_vp/SAT_VP_CONST1))/(SAT_VP_CONST2-log10(sat_vp/SAT_VP_CONST1)); //in °C 
}

inline WeatherPrecision wb(const WeatherPrecision t, const WeatherPrecision p, const WeatherPrecision dp)
{
    WeatherPrecision e = sat_vp(dp); 
    WeatherPrecision tcur, tcvp, peq, diff;
    WeatherPrecision tmin, tmax;

    if (t > dp)
    {
	tmax = t;
	tmin = dp;
    }
    else
    {
	tmax = dp;
	tmin = t;
    }

    while (true) 
    { 
	tcur=(tmax+tmin)/2;
	tcvp=sat_vp(tcur);
	
	peq = 0.000660*(1+0.00155*tcur)*p*(t-tcur);
	
	diff = peq-tcvp+e;
	
	if (fabs(diff) < 0.01) 
	    return tcur;	//in °C
	
	if (diff < 0) 
	    tmax=tcur; 
	else 
	    tmin=tcur; 
    }; 

}

inline WeatherPrecision Celsius		    (const WeatherPrecision celsius)
{
    return celsius + 273.16;				//Kelvin
}

inline WeatherPrecision Fahrenheit	    (const WeatherPrecision fahrenheit)
{
    return (fahrenheit * 9.0 / 5.0) + 32.0 + 273.16;	//Kelvin
}

inline WeatherPrecision Kelvin2Celsius	    (const WeatherPrecision kelvin)
{
    return kelvin - 273.16;				//Celsius
}

inline WeatherPrecision Kelvin2Fahrenheit   (const WeatherPrecision kelvin)
{
    return ((kelvin - 273.16) * 9.0 / 5.0) + 32.0;	//Fahrenheit
}

inline WeatherPrecision Celsius2Fahrenheit  (const WeatherPrecision celsius)
{
    return (celsius * 9.0 / 5.0) + 32.0;		//Fahrenheit
}

inline WeatherPrecision Fahrenheit2Celsius  (const WeatherPrecision fahrenheit)
{
    return (fahrenheit - 32.0) * 5.0 / 9.0;		//Celsius
}

inline WeatherPrecision Torr2Pascal	    (const WeatherPrecision torr)
{
    return (101325.0/760.0)*torr;			//Pascal
}

inline WeatherPrecision Rankine2Kelvin	    (const WeatherPrecision Rankine)
{
    return (5.0 / 9.0) * Rankine;			//Kelvin
}

inline WeatherPrecision JSBsim2SIdensity    (const WeatherPrecision JSBsim)
{
    return JSBsim / 0.0019403203;			//kg / cubic metres
}

inline WeatherPrecision psf2Pascal	    (const WeatherPrecision psf)
{
    return psf / 0.0020885434;				//lbs / square foot (used in JSBsim)
}

inline WeatherPrecision Kelvin2Rankine	    (const WeatherPrecision kelvin)
{
    return (9.0 / 5.0) * kelvin;			//Rankine (used in JSBsim)
}

inline WeatherPrecision SIdensity2JSBsim    (const WeatherPrecision SIdensity)
{
    return 0.0019403203 * SIdensity;			//slug / cubic feet (used in JSBsim)
}

inline WeatherPrecision Pascal2psf	    (const WeatherPrecision Pascal)
{
    return 0.0020885434 * Pascal;			//lbs / square feet (used in JSBsim)
}

/****************************************************************************/
/* CLASS DECLARATION							    */
/****************************************************************************/

/****************************************************************************/
#endif /*FGWeatherUtils_H*/
