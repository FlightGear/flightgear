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
*****************************************************************************/

/****************************************************************************/
/* SENTRY                                                                   */
/****************************************************************************/
#ifndef FGLocalWeatherDatabase_H
#define FGLocalWeatherDatabase_H

/****************************************************************************/
/* INCLUDES								    */
/****************************************************************************/
//This is only here for smoother code change. In the end the WD should be clean
//of *any* OpenGL:
#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif
#include <GL/glut.h>
#include <XGL/xgl.h>

#include <vector>

#include "sg.h"

#include "FGPhysicalProperties.h"
#include "FGGlobalWeatherDatabase.h"
#include "FGMicroWeather.h"
#include "FGWeatherFeature.h"
#include "FGWeatherDefs.h"

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
    FGGlobalWeatherDatabase *global;	//point to the global database

    typedef vector<FGMicroWeather>       FGMicroWeatherList;
    typedef FGMicroWeatherList::iterator FGMicroWeatherListIt;

    typedef vector<sgVec2>      pointVector;
    typedef vector<pointVector> tileVector;

    /************************************************************************/
    /* make tiles out of points on a 2D plane				    */
    /************************************************************************/
    void tileLocalWeather(const FGPhysicalProperties2DVector& EntryList);

    FGMicroWeatherList WeatherAreas;

    WeatherPrecision WeatherVisibility;	//how far do I need to simulate the
					//local weather? Unit: metres
    sgVec3 last_known_position;


    /************************************************************************/
    /* return the index of the area with point p			    */
    /************************************************************************/
    unsigned int AreaWith(const sgVec2& p) const;
    unsigned int AreaWith(const sgVec3& p) const
    {
	sgVec2 temp;
	sgSetVec2(temp, p[0], p[1]);

	return AreaWith(temp);
    }

public:
    static FGLocalWeatherDatabase *theFGLocalWeatherDatabase;  
    
    enum DatabaseWorkingType {
	use_global,	//use global database for data
	manual,		//use only user inputs
	distant,	//use distant information, e.g. like LAN when used in
			//a multiplayer environment
	random,		//generate weather randomly
	default_mode	//use only default values
    };

    DatabaseWorkingType DatabaseStatus;

    /************************************************************************/
    /* Constructor and Destructor					    */
    /************************************************************************/
    FGLocalWeatherDatabase(
	const sgVec3&             position,
	const WeatherPrecision    visibility = DEFAULT_WEATHER_VISIBILITY,
	const DatabaseWorkingType type       = PREFERED_WORKING_TYPE);

    FGLocalWeatherDatabase(
	const WeatherPrecision    position_lat,
	const WeatherPrecision    position_lon,
	const WeatherPrecision    position_alt,
	const WeatherPrecision    visibility = DEFAULT_WEATHER_VISIBILITY,
	const DatabaseWorkingType type       = PREFERED_WORKING_TYPE)
    {
	cout << "This constructor is broken and should *NOT* be used!" << endl;
	exit(-1);

	sgVec3 position;
	sgSetVec3( position, position_lat, position_lon, position_alt );

	// Christian: the following line does not do what you intend.
	// It just creates a new FGLocalWeatherDatabase which isn't
	// assigned to anything so it is immediately discared.

	/* BAD --> */ FGLocalWeatherDatabase( position, visibility, type );
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
    FGPhysicalProperties get(const sgVec2& p) const;
    FGPhysicalProperty   get(const sgVec3& p) const;

    WeatherPrecision     getAirDensity(const sgVec3& p) const;
    
    /************************************************************************/
    /* Add a weather feature at the point p and surrounding area	    */
    /************************************************************************/

    void addWind         (const WeatherPrecision alt, const sgVec3& x,          const sgVec2& p);
    void addTurbulence   (const WeatherPrecision alt, const sgVec3& x,          const sgVec2& p);
    void addTemperature  (const WeatherPrecision alt, const WeatherPrecision x, const sgVec2& p);
    void addAirPressure  (const WeatherPrecision alt, const WeatherPrecision x, const sgVec2& p);
    void addVaporPressure(const WeatherPrecision alt, const WeatherPrecision x, const sgVec2& p);
    void addCloud        (const WeatherPrecision alt, const FGCloudItem& x,     const sgVec2& p);

    void setSnowRainIntensity   (const WeatherPrecision x, const sgVec2& p);
    void setSnowRainType        (const SnowRainType x,     const sgVec2& p);
    void setLightningProbability(const WeatherPrecision x, const sgVec2& p);

    void addProperties(const FGPhysicalProperties2D& x);    //add a property
    void setProperties(const FGPhysicalProperties2D& x);    //change a property

    /************************************************************************/
    /* get/set weather visibility					    */
    /************************************************************************/
    void             setWeatherVisibility(const WeatherPrecision visibility);
    WeatherPrecision getWeatherVisibility(void) const;
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

#if 0
    //This code doesn't belong here as this is the optical visibility and not
    //the visibility of the weather database (that should be bigger...). The
    //optical visibility should be calculated from the vapor pressure e.g.
    //But for the sake of a smoother change from the old way to the new one...

    GLfloat fog_exp_density;
    GLfloat fog_exp2_density;
    
    // for GL_FOG_EXP
    fog_exp_density = -log(0.01 / WeatherVisibility);
    
    // for GL_FOG_EXP2
    fog_exp2_density = sqrt( -log(0.01) ) / WeatherVisibility;
    
    // Set correct opengl fog density
    xglFogf (GL_FOG_DENSITY, fog_exp2_density);
    
    // FG_LOG( FG_INPUT, FG_DEBUG, "Fog density = " << w->fog_density );
    //cerr << "FGLocalWeatherDatabase::setWeatherVisibility(" << visibility << "):\n";
    //cerr << "Fog density = " << fog_exp_density << "\n";
#endif
}

WeatherPrecision inline FGLocalWeatherDatabase::getWeatherVisibility(void) const
{
    //cerr << "FGLocalWeatherDatabase::getWeatherVisibility() = " << WeatherVisibility << "\n";
    return WeatherVisibility;
}


/****************************************************************************/
#endif /*FGLocalWeatherDatabase_H*/
