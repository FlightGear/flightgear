/*****************************************************************************

 Header:       FGLocalWeatherDatabase.h	
 Author:       Christian Mayer
 Date started: 28.05.99

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
Database for the local weather
This database is the only one that gets called from FG

HISTORY
------------------------------------------------------------------------------
28.05.1999 Christian Mayer	Created
16.06.1999 Durk Talsma		Portability for Linux
20.06.1999 Christian Mayer	added lots of consts
30.06.1999 Christian Mayer	STL portability
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

#include "FGPhysicalProperties.h"
#include "FGGlobalWeatherDatabase.h"
#include "FGMicroWeather.h"
#include "FGWeatherFeature.h"
#include "FGWeatherDefs.h"
#include <vector>

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

    typedef vector<FGMicroWeather> FGMicroWeatherList;
    typedef FGMicroWeatherList::iterator FGMicroWeatherListIt;

    typedef vector<Point2D> pointVector;
    typedef vector<pointVector> tileVector;

    /************************************************************************/
    /* make tiles out of points on a 2D plane				    */
    /************************************************************************/
    void tileLocalWeather(const FGPhysicalProperties2DVector& EntryList);

    FGMicroWeatherList WeatherAreas;

    WeatherPrecition WeatherVisibility;	//how far do I need to simulate the
					//local weather? Unit: metres
    Point3D last_known_position;

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

protected:
    DatabaseWorkingType DatabaseStatus;

    /************************************************************************/
    /* return the index of the area with point p			    */
    /************************************************************************/
    unsigned int AreaWith(const Point2D& p) const;

public:
    /************************************************************************/
    /* Constructor and Destructor					    */
    /************************************************************************/
    FGLocalWeatherDatabase(
	const Point3D& posititon,
	const WeatherPrecition& visibility = DEFAULT_WEATHER_VISIBILIY,
	const DatabaseWorkingType& type = PREFERED_WORKING_TYPE);
    ~FGLocalWeatherDatabase();

    /************************************************************************/
    /* reset the whole database						    */
    /************************************************************************/
    void reset(const DatabaseWorkingType& type = PREFERED_WORKING_TYPE);

    /************************************************************************/
    /* update the database. Since the last call we had dt seconds	    */
    /************************************************************************/
    void update(const WeatherPrecition& dt);			//time has changed
    void update(const Point3D& p);				//position has  changed
    void update(const Point3D& p, const WeatherPrecition& dt);	//time and/or position has changed

    /************************************************************************/
    /* Get the physical properties on the specified point p		    */
    /************************************************************************/
    FGPhysicalProperty get(const Point3D& p) const;
    WeatherPrecition getAirDensity(const Point3D& p) const;
    
    /************************************************************************/
    /* Add a weather feature at the point p and surrounding area	    */
    /************************************************************************/

    void addWind(const FGWindItem& x, const Point2D& p);
    void addTurbulence(const FGTurbulenceItem& x, const Point2D& p);
    void addTemperature(const FGTemperatureItem& x, const Point2D& p);
    void addAirPressure(const FGAirPressureItem& x, const Point2D& p);
    void addVaporPressure(const FGVaporPressureItem& x, const Point2D& p);
    void addCloud(const FGCloudItem& x, const Point2D& p);

    void setSnowRainIntensity(const WeatherPrecition& x, const Point2D& p);
    void setSnowRainType(const SnowRainType& x, const Point2D& p);
    void setLightningProbability(const WeatherPrecition& x, const Point2D& p);

    void addProperties(const FGPhysicalProperties2D& x);    //add a property
    void setProperties(const FGPhysicalProperties2D& x);    //change a property

    /************************************************************************/
    /* get/set weather visibility					    */
    /************************************************************************/
    void setWeatherVisibility(const WeatherPrecition& visibility);
    WeatherPrecition getWeatherVisibility(void) const;
};

extern FGLocalWeatherDatabase *WeatherDatabase;
void fgUpdateWeatherDatabase(void);

/****************************************************************************/
/* get/set weather visibility						    */
/****************************************************************************/
void inline FGLocalWeatherDatabase::setWeatherVisibility(const WeatherPrecition& visibility)
{
    if (visibility >= MINIMUM_WEATHER_VISIBILIY)
	WeatherVisibility = visibility;
    else
	WeatherVisibility = MINIMUM_WEATHER_VISIBILIY;

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
    

}

WeatherPrecition inline FGLocalWeatherDatabase::getWeatherVisibility(void) const
{
    //cerr << "FGLocalWeatherDatabase::getWeatherVisibility() = " << WeatherVisibility << "\n";
    return WeatherVisibility;
}


/****************************************************************************/
#endif /*FGLocalWeatherDatabase_H*/
