/*****************************************************************************

 Module:       FGMicroWeather.cpp
 Author:       Christian Mayer
 Date started: 28.05.99
 Called by:    FGLocalWeatherDatabase

 ---------- Copyright (C) 1999  Christian Mayer (vader@t-online.de) ----------

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
*****************************************************************************/

/****************************************************************************/
/* INCLUDES								    */
/****************************************************************************/
#include "FGMicroWeather.h"
#include "fg_constants.h"

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
void FGMicroWeather::addWind(const FGWindItem& x)
{
    StoredWeather.Wind.insert(x);
}

void FGMicroWeather::addTurbulence(const FGTurbulenceItem& x)
{
    StoredWeather.Turbulence.insert(x);
}

void FGMicroWeather::addTemperature(const FGTemperatureItem& x)
{
    StoredWeather.Temperature.insert(x);
}

void FGMicroWeather::addAirPressure(const FGAirPressureItem& x)
{
    cerr << "Error: caught attempt to add AirPressure which is logical wrong\n";
    //StoredWeather.AirPressure.insert(x);
}

void FGMicroWeather::addVaporPressure(const FGVaporPressureItem& x)
{
    StoredWeather.VaporPressure.insert(x);
}

void FGMicroWeather::addCloud(const FGCloudItem& x)
{
    StoredWeather.Clouds.insert(x);
}

void FGMicroWeather::setSnowRainIntensity(const WeatherPrecition& x)
{
    StoredWeather.SnowRainIntensity = x;
}

void FGMicroWeather::setSnowRainType(const SnowRainType& x)
{
    StoredWeather.snowRainType = x;
}

void FGMicroWeather::setLightningProbability(const WeatherPrecition& x)
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

bool FGMicroWeather::hasPoint(const Point2D& p) const
{
    if (position.size()==0)
	return true;	//no border => this tile is infinite
    
    if (position.size()==1)
	return false;	//a border on 1 point?!?

    //when I'm here I've got at least 2 points
    
    WeatherPrecition t;
    signed char side1, side2;
    const_positionListIt it = position.begin();
    const_positionListIt it2 = it; it2++;

    for (;;)
    {
	
	if (it2 == position.end())
	    break;

	if (fabs(it->x() - it2->x()) >= FG_EPSILON)
	{
	    t = (it->y() - it2->y()) / (it->x() - it2->x());
	    side1 = FG_SIGN (t * (StoredWeather.p.x() - it2->x()) + it2->y() - StoredWeather.p.y());
	    side2 = FG_SIGN (t * (              p.x() - it2->x()) + it2->y() -               p.y());
	    if ( side1 != side2 ) 
		return false; //cout << "failed side check\n";
	}
	else
	{
	    t = (it->x() - it2->x()) / (it->y() - it2->y());
	    side1 = FG_SIGN (t * (StoredWeather.p.y() - it2->y()) + it2->x() - StoredWeather.p.x());
	    side2 = FG_SIGN (t * (              p.y() - it2->y()) + it2->x() -               p.x());
	    if ( side1 != side2 ) 
		return false; //cout << "failed side check\n";
	}

	it++; it2++;
    }

    return true;
}
