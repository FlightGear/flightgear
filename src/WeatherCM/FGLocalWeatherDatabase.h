/*****************************************************************************

 Header:       FGLocalWeatherDatabase.h	
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
Database for the local weather
This database is the only one that gets called from FG

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
14.12.1999 Christian Mayer	Changed the internal structure to use Dave
                                Eberly's spherical interpolation code. This
				stops our dependancy on the (ugly) voronoi
				code and simplyfies the code structure a lot.
*****************************************************************************/

/****************************************************************************/
/* SENTRY                                                                   */
/****************************************************************************/
#ifndef FGLocalWeatherDatabase_H
#define FGLocalWeatherDatabase_H

/****************************************************************************/
/* INCLUDES								    */
/****************************************************************************/
#include <vector>

#include <plib/sg.h>

#include "sphrintp.h"

#include "FGPhysicalProperties.h"
#include "FGPhysicalProperty.h"


#include "FGWeatherFeature.h"
#include "FGWeatherDefs.h"
#include "FGThunderstorm.h"

/****************************************************************************/
/* DEFINES								    */
/****************************************************************************/
FG_USING_STD(vector);
FG_USING_NAMESPACE(std);

/****************************************************************************/
/* CLASS DECLARATION							    */
/****************************************************************************/
class FGLocalWeatherDatabase
{
private:
protected:
    SphereInterpolate<FGPhysicalProperties> *database;

    typedef vector<sgVec2>      pointVector;
    typedef vector<pointVector> tileVector;

    /************************************************************************/
    /* make tiles out of points on a 2D plane				    */
    /************************************************************************/
    WeatherPrecision WeatherVisibility;	//how far do I need to simulate the
					//local weather? Unit: metres
    sgVec3 last_known_position;

    bool Thunderstorm;			//is there a thunderstorm near by?
    FGThunderstorm *theThunderstorm;	//pointer to the thunderstorm.

public:
    static FGLocalWeatherDatabase *theFGLocalWeatherDatabase;  
    
    enum DatabaseWorkingType {
	use_global,	//use global database for data !!obsolete!!
	use_internet,   //use the weather data that came from the internet
	manual,		//use only user inputs
	distant,	//use distant information, e.g. like LAN when used in
			//a multiplayer environment
	random,		//generate weather randomly
	default_mode	//use only default values
    };

    DatabaseWorkingType DatabaseStatus;

    void init( const WeatherPrecision visibility, const DatabaseWorkingType type );

    /************************************************************************/
    /* Constructor and Destructor					    */
    /************************************************************************/
    FGLocalWeatherDatabase(
	const sgVec3&             position,
	const WeatherPrecision    visibility = DEFAULT_WEATHER_VISIBILITY,
	const DatabaseWorkingType type       = PREFERED_WORKING_TYPE)
    {
	sgCopyVec3( last_known_position, position );

	init( visibility, type );

	theFGLocalWeatherDatabase = this;
    }

    FGLocalWeatherDatabase(
	const WeatherPrecision    position_lat,
	const WeatherPrecision    position_lon,
	const WeatherPrecision    position_alt,
	const WeatherPrecision    visibility = DEFAULT_WEATHER_VISIBILITY,
	const DatabaseWorkingType type       = PREFERED_WORKING_TYPE)
    {
	sgSetVec3( last_known_position, position_lat, position_lon, position_alt );

	init( visibility, type );

	theFGLocalWeatherDatabase = this;
    }

    ~FGLocalWeatherDatabase();

    /************************************************************************/
    /* reset the whole database						    */
    /************************************************************************/
    void reset(const DatabaseWorkingType type = PREFERED_WORKING_TYPE);

    /************************************************************************/
    /* update the database. Since the last call we had dt seconds	    */
    /************************************************************************/
    void update(const WeatherPrecision dt);			//time has changed
    void update(const sgVec3& p);				//position has  changed
    void update(const sgVec3& p, const WeatherPrecision dt);	//time and/or position has changed

    /************************************************************************/
    /* Get the physical properties on the specified point p		    */
    /************************************************************************/
#ifdef MACOS
    /* fix a problem with mw compilers in that they don't know the
       difference between the next two methods. Since the first one
       doesn't seem to be used anywhere, I commented it out. This is
       supposed to be fixed in the forthcoming CodeWarrior Release
       6. */
#else
    FGPhysicalProperties get(const sgVec2& p) const;
#endif
    FGPhysicalProperty   get(const sgVec3& p) const;

    WeatherPrecision     getAirDensity(const sgVec3& p) const;
    
    /************************************************************************/
    /* Add a weather feature at the point p and surrounding area	    */
    /************************************************************************/
    // !! Adds aren't supported anymore !!

    void setSnowRainIntensity   (const WeatherPrecision x, const sgVec2& p);
    void setSnowRainType        (const SnowRainType x,     const sgVec2& p);
    void setLightningProbability(const WeatherPrecision x, const sgVec2& p);

    void setProperties(const FGPhysicalProperties2D& x);    //change a property

    /************************************************************************/
    /* get/set weather visibility					    */
    /************************************************************************/
    void             setWeatherVisibility(const WeatherPrecision visibility);
    WeatherPrecision getWeatherVisibility(void) const;

    /************************************************************************/
    /* figure out if there's a thunderstorm that has to be taken care of    */
    /************************************************************************/
    void updateThunderstorm(const float dt)
    {
	if (Thunderstorm == false)
	    return;

	theThunderstorm->update( dt );
    }
};

extern FGLocalWeatherDatabase *WeatherDatabase;
void fgUpdateWeatherDatabase(void);

/****************************************************************************/
/* get/set weather visibility						    */
/****************************************************************************/
void inline FGLocalWeatherDatabase::setWeatherVisibility(const WeatherPrecision visibility)
{
    if (visibility >= MINIMUM_WEATHER_VISIBILITY)
	WeatherVisibility = visibility;
    else
	WeatherVisibility = MINIMUM_WEATHER_VISIBILITY;
}

WeatherPrecision inline FGLocalWeatherDatabase::getWeatherVisibility(void) const
{
    return WeatherVisibility;
}


/****************************************************************************/
#endif /*FGLocalWeatherDatabase_H*/
