/*****************************************************************************

 Header:       FGWeatherDefs.h	
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
definitions uses in most weather classes

HISTORY
------------------------------------------------------------------------------
28.05.1999 Christian Mayer	Created
16.06.1999 Durk Talsma		Portability for Linux
20.06.1999 Christian Mayer	added lots of consts
11.10.1999 Christian Mayer	changed set<> to map<> on Bernie Bright's 
				suggestion
19.10.1999 Christian Mayer	change to use PLIB's sg instead of Point[2/3]D
				and lots of wee code cleaning
*****************************************************************************/

/****************************************************************************/
/* SENTRY                                                                   */
/****************************************************************************/
#ifndef FGWeatherDefs_H
#define FGWeatherDefs_H

/****************************************************************************/
/* INCLUDES								    */
/****************************************************************************/
		
/****************************************************************************/
/* DEFINES								    */
/****************************************************************************/
typedef float WeatherPrecision;

//set the minimum visibility to get a at least half way realistic weather
#define MINIMUM_WEATHER_VISIBILITY 10.0    /* metres */
#define DEFAULT_WEATHER_VISIBILITY 32000.0 /* metres */
//prefered way the database is working
// #define PREFERED_WORKING_TYPE default_mode
#define PREFERED_WORKING_TYPE use_internet

#define FG_WEATHER_DEFAULT_TEMPERATURE   (15.0+273.16)	    /*15°C or 288.16°K*/
#define FG_WEATHER_DEFAULT_VAPORPRESSURE (0.0)		    /*in Pascal 1 Pa = N/m^2*/
#define FG_WEATHER_DEFAULT_AIRPRESSURE   (1013.25*100.0)    /*in Pascal 1 Pa = N/m^2*/
#define FG_WEATHER_DEFAULT_AIRDENSITY    (1.22501)	    /*in kg/m^3*/

/****************************************************************************/
/* CLASS DECLARATION							    */
/****************************************************************************/

/****************************************************************************/
#endif /*FGWeatherDefs_H*/
