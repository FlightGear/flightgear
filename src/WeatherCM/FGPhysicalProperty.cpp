/*****************************************************************************

 Module:       FGPhysicalProperty.cpp
 Author:       Christian Mayer
 Date started: 28.05.99
 Called by:    main program

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
Initialice the FGPhysicalProperty struct to something sensible(?)

HISTORY
------------------------------------------------------------------------------
29.05.1999 Christian Mayer	Created
16.06.1999 Durk Talsma		Portability for Linux
20.06.1999 Christian Mayer	added lots of consts
11.10.1999 Christian Mayer	changed set<> to map<> on Bernie Bright's 
				suggestion
19.10.1999 Christian Mayer	change to use PLIB's sg instead of Point[2/3]D
				and lots of wee code cleaning
*****************************************************************************/

/****************************************************************************/
/* INCLUDES								    */
/****************************************************************************/
#include "FGWeatherDefs.h"
#include "FGPhysicalProperty.h"

/****************************************************************************/
/********************************** CODE ************************************/
/****************************************************************************/
FGPhysicalProperty::FGPhysicalProperty()
{
    sgZeroVec3(Wind);		//Wind vector

    sgZeroVec3(Turbulence);	//Turbulence vector

    Temperature = FG_WEATHER_DEFAULT_TEMPERATURE;	//a nice warm day
    AirPressure = FG_WEATHER_DEFAULT_AIRPRESSURE;	//mbar, that's ground level
    VaporPressure = FG_WEATHER_DEFAULT_VAPORPRESSURE;	//that gives about 50% relatvie humidity
}

FGPhysicalProperty::FGPhysicalProperty(const FGPhysicalProperties& p, const WeatherPrecision altitude)
{
    p.WindAt(Wind, altitude);
    p.TurbulenceAt(Turbulence, altitude);
    Temperature = p.TemperatureAt(altitude);
    AirPressure = p.AirPressureAt(altitude);
    VaporPressure = p.VaporPressureAt(altitude);
}


