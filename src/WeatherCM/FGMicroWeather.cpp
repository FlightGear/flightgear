/*****************************************************************************

 Module:       FGMicroWeather.cpp
 Author:       Christian Mayer
 Date started: 28.05.99
 Called by:    FGLocalWeatherDatabase

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
Handle the weather areas

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
/* INCLUDES								    */
/****************************************************************************/
#include <Include/compiler.h>
#include <Include/fg_constants.h>

#include "FGMicroWeather.h"

/****************************************************************************/
/********************************** CODE ************************************/
/****************************************************************************/
FGMicroWeather::FGMicroWeather(const FGPhysicalProperties2D& p, const positionList& points)
{
    StoredWeather = p;
    position = points;
}

FGMicroWeather::~FGMicroWeather()
{
}

/****************************************************************************/
/* Add the features to the micro weather				    */
/* return succss							    */
/****************************************************************************/
void FGMicroWeather::addWind(const WeatherPrecision alt, const FGWindItem& x)
{
    StoredWeather.Wind[alt] = x;
}

void FGMicroWeather::addTurbulence(const WeatherPrecision alt, const FGTurbulenceItem& x)
{
    StoredWeather.Turbulence[alt] = x;
}

void FGMicroWeather::addTemperature(const WeatherPrecision alt, const WeatherPrecision x)
{
    StoredWeather.Temperature[alt] = x;
}

void FGMicroWeather::addAirPressure(const WeatherPrecision alt, const WeatherPrecision x)
{
    cerr << "Error: caught attempt to add AirPressure which is logical wrong\n";
    //StoredWeather.AirPressure[alt] = x;
}

void FGMicroWeather::addVaporPressure(const WeatherPrecision alt, const WeatherPrecision x)
{
    StoredWeather.VaporPressure[alt] = x;
}

void FGMicroWeather::addCloud(const WeatherPrecision alt, const FGCloudItem& x)
{
    StoredWeather.Clouds[alt] = x;
}

void FGMicroWeather::setSnowRainIntensity(const WeatherPrecision x)
{
    StoredWeather.SnowRainIntensity = x;
}

void FGMicroWeather::setSnowRainType(const SnowRainType x)
{
    StoredWeather.snowRainType = x;
}

void FGMicroWeather::setLightningProbability(const WeatherPrecision x)
{
    StoredWeather.LightningProbability = x;
}

void FGMicroWeather::setStoredWeather(const FGPhysicalProperties2D& x)
{
    StoredWeather = x;
}

/****************************************************************************/
/* return true if p is inside this micro weather			    */
/* code stolen from $FG_ROOT/Simulator/Objects/fragment.cxx, which was	    */
/* written by Curtis L. Olson - curt@me.umn.edu				    */
/****************************************************************************/

template <class T> //template to help with the calulation
inline const int FG_SIGN(const T& x) {
    return x < T(0) ? -1 : 1;
}

bool FGMicroWeather::hasPoint(const sgVec2& p) const
{
    if (position.size()==0)
	return true;	//no border => this tile is infinite
    
    if (position.size()==1)
	return false;	//a border on 1 point?!?

    //when I'm here I've got at least 2 points
    
    WeatherPrecision t;
    signed char side1, side2;
    const_positionListIt it = position.begin();
    const_positionListIt it2 = it; it2++;

    for (;;)
    {
	
	if (it2 == position.end())
	    break;

	if (fabs(it->p[0] - it2->p[0]) >= FG_EPSILON)
	{
	    t = (it->p[1] - it2->p[1]) / (it->p[0] - it2->p[0]);

	    side1 = FG_SIGN (t * (StoredWeather.p[0] - it2->p[0]) + it2->p[1] - StoredWeather.p[1]);
	    side2 = FG_SIGN (t * (              p[0] - it2->p[0]) + it2->p[1] -               p[1]);
	    if ( side1 != side2 ) 
		return false; //cout << "failed side check\n";
	}
	else
	{
	    t = (it->p[0] - it2->p[0]) / (it->p[1] - it2->p[1]);

	    side1 = FG_SIGN (t * (StoredWeather.p[1] - it2->p[1]) + it2->p[0] - StoredWeather.p[0]);
	    side2 = FG_SIGN (t * (              p[1] - it2->p[1]) + it2->p[0] -               p[0]);
	    if ( side1 != side2 ) 
		return false; //cout << "failed side check\n";
	}

	it++; it2++;
    }

    return true;
}
