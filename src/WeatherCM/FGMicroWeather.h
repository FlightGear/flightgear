/*****************************************************************************

 Header:       FGMicroWeather.h	
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
Store the single weather areas

HISTORY
------------------------------------------------------------------------------
28.05.1999 Christian Mayer	Created
16.06.1999 Durk Talsma		Portability for Linux
20.06.1999 Christian Mayer	added lots of consts
30.06.1999 Christian Mayer	STL portability
11.10.1999 Christian Mayer	changed set<> to map<> on Bernie Bright's 
				suggestion
19.10.1999 Christian Mayer	change to use PLIB's sg instead of Point[2/3]D
				and lots of wee code cleaning
*****************************************************************************/

/****************************************************************************/
/* SENTRY                                                                   */
/****************************************************************************/
#ifndef FGMicroWeather_H
#define FGMicroWeather_H

/****************************************************************************/
/* INCLUDES								    */
/****************************************************************************/
#include <set>

#include "sg.h"
#include "FGWeatherVectorWrap.h"

#include "FGWeatherDefs.h"

//Include all the simulated weather features
#include "FGCloud.h"
#include "FGSnowRain.h"

#include "FGAirPressureItem.h"
#include "FGWindItem.h"
#include "FGTurbulenceItem.h"

#include "FGPhysicalProperties.h"
#include "FGPhysicalProperty.h"

/****************************************************************************/
/* DEFINES								    */
/****************************************************************************/
FG_USING_STD(set);
FG_USING_NAMESPACE(std);

/****************************************************************************/
/* CLASS DECLARATION							    */
/****************************************************************************/
class FGMicroWeather
{
private:
protected:
    typedef vector<sgVec2Wrap>           positionList;
    typedef positionList::iterator       positionListIt;
    typedef positionList::const_iterator const_positionListIt;
    positionList position;	//the points that specify the outline of the
				//micro weather (lat/lon)

    FGPhysicalProperties2D StoredWeather;    //property if nothing is specified

public:
    /************************************************************************/
    /* Constructor and Destructor					    */
    /************************************************************************/
    FGMicroWeather(const FGPhysicalProperties2D& p, const positionList& points);
    ~FGMicroWeather();

    /************************************************************************/
    /* Add a feature to the micro weather				    */
    /************************************************************************/
    void addWind         (const WeatherPrecision alt, const FGWindItem&       x);
    void addTurbulence   (const WeatherPrecision alt, const FGTurbulenceItem& x);
    void addTemperature  (const WeatherPrecision alt, const WeatherPrecision  x);
    void addAirPressure  (const WeatherPrecision alt, const WeatherPrecision  x);
    void addVaporPressure(const WeatherPrecision alt, const WeatherPrecision  x);
    void addCloud        (const WeatherPrecision alt, const FGCloudItem&      x);

    void setSnowRainIntensity   (const WeatherPrecision x);
    void setSnowRainType        (const SnowRainType x);
    void setLightningProbability(const WeatherPrecision x);

    void setStoredWeather       (const FGPhysicalProperties2D& x);

    /************************************************************************/
    /* get physical properties in the micro weather			    */
    /* NOTE: I don't neet to speify a positon as the properties don't	    */
    /*       change in a micro weather					    */
    /************************************************************************/
    FGPhysicalProperties get(void) const
    {
	return FGPhysicalProperties();
    }

    FGPhysicalProperty   get(const WeatherPrecision altitude) const
    {
	return FGPhysicalProperty(StoredWeather, altitude);
    }

    /************************************************************************/
    /* return true if p is inside this micro weather			    */
    /************************************************************************/
    bool hasPoint(const sgVec2& p) const;
    bool hasPoint(const sgVec3& p) const 
    { 
	sgVec2 temp;
	sgSetVec2(temp, p[0], p[1]);

	return hasPoint( temp ); 
    } 
};

/****************************************************************************/
#endif /*FGMicroWeather_H*/
