/*****************************************************************************

 Module:       FGLocalWeatherDatabase.cpp
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
Database for the local weather
This database is the only one that gets called from FG

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

#include <Aircraft/aircraft.hxx>

#include "FGLocalWeatherDatabase.h"
#include "FGVoronoi.h"


/****************************************************************************/
/********************************** CODE ************************************/
/****************************************************************************/

/****************************************************************************/
/* return the index (better: ID) of the area with point p		    */
/****************************************************************************/
unsigned int FGLocalWeatherDatabase::AreaWith(const sgVec2& p) const
{
    
    for (FGMicroWeatherList::size_type i = 0; i != WeatherAreas.size(); i++)
    {
	if (WeatherAreas[i].hasPoint(p) == true)
	    return i+1;
    }

    return 0;	//nothing found
}

/****************************************************************************/
/* make tiles out of points on a 2D plane				    */
/****************************************************************************/
void FGLocalWeatherDatabase::tileLocalWeather(const FGPhysicalProperties2DVector& EntryList)
{
    FGVoronoiInputList input;

    for (FGPhysicalProperties2DVectorConstIt it1 = EntryList.begin(); it1 != EntryList.end(); it1++)
	input.push_back(FGVoronoiInput(it1->p, *it1));

    FGVoronoiOutputList output = Voronoiate(input);

    for (FGVoronoiOutputList::iterator it2 = output.begin(); it2 != output.end(); it2++)
	WeatherAreas.push_back(FGMicroWeather(it2->value, it2->boundary));
}

/****************************************************************************/
/* Constructor and Destructor						    */
/****************************************************************************/
FGLocalWeatherDatabase* FGLocalWeatherDatabase::theFGLocalWeatherDatabase = 0;
FGLocalWeatherDatabase *WeatherDatabase;

FGLocalWeatherDatabase::FGLocalWeatherDatabase(const sgVec3& position, const WeatherPrecision visibility, const DatabaseWorkingType type)
{
    cerr << "Initializing FGLocalWeatherDatabase\n";
    cerr << "-----------------------------------\n";

    if (theFGLocalWeatherDatabase)
    {
	//FG_LOG( FG_GENERAL, FG_ALERT, "Error: only one local weather allowed" );
	cerr << "Error: only one local weather allowed";
	exit(-1);
    }

    setWeatherVisibility(visibility);

    DatabaseStatus = type;
    global = 0;	    //just get sure...
    sgCopyVec3(last_known_position, position);


    theFGLocalWeatherDatabase = this;

    switch(DatabaseStatus)
    {
    case use_global:
	{
	    global = new FGGlobalWeatherDatabase;	//initialize GlobalDatabase
	    global->setDatabaseStatus(FGGlobalWeatherDatabase_working);
	    tileLocalWeather(global->getAll(position, WeatherVisibility, 3));
	}
	break;

    case distant:
	cerr << "FGLocalWeatherDatabase error: Distant database isn't implemented yet!\n";
	cerr << "  using random mode instead!\n";
    case random:
    case manual:
    case default_mode:
	{
	    vector<sgVec2Wrap> emptyList;
	    WeatherAreas.push_back(FGMicroWeather(FGPhysicalProperties2D(), emptyList));   //in these cases I've only got one tile
	}
	break;

    default:
	cerr << "FGLocalWeatherDatabase error: Unknown database type specified!\n";
    };
}

FGLocalWeatherDatabase::~FGLocalWeatherDatabase()
{
    //Tidying up:

    //delete every stored area
    WeatherAreas.erase(WeatherAreas.begin(), WeatherAreas.end());

    //delete global database if necessary
    if (DatabaseStatus == use_global)
	delete global;
}

/****************************************************************************/
/* reset the whole database						    */
/****************************************************************************/
void FGLocalWeatherDatabase::reset(const DatabaseWorkingType type)
{
    //delete global database if necessary
    if ((DatabaseStatus == use_global) && (type != use_global))
	delete global;

    DatabaseStatus = type;
    if (DatabaseStatus == use_global)
	tileLocalWeather(global->getAll(last_known_position, WeatherVisibility, 3));

    //delete every stored area
    WeatherAreas.erase(WeatherAreas.begin(), WeatherAreas.end());
}

/****************************************************************************/
/* update the database. Since the last call we had dt seconds		    */
/****************************************************************************/
void FGLocalWeatherDatabase::update(const WeatherPrecision dt)
{
    if (DatabaseStatus==use_global)
	global->update(dt);
}

void FGLocalWeatherDatabase::update(const sgVec3& p) //position has changed
{
    sgCopyVec3(last_known_position, p);
    //cerr << "****\nupdate inside\n";
    //cerr << "Parameter: " << p << "\n";
    //cerr << "****\n";
}

void FGLocalWeatherDatabase::update(const sgVec3& p, const WeatherPrecision dt)   //time and/or position has changed
{
    sgCopyVec3(last_known_position, p);

    if (DatabaseStatus==use_global)
	global->update(dt);
}

/****************************************************************************/
/* Get the physical properties on the specified point p	out of the database */
/****************************************************************************/
FGPhysicalProperty FGLocalWeatherDatabase::get(const sgVec3& p) const
{
    unsigned int a = AreaWith(p);
    if (a != 0)
	return WeatherAreas[a-1].get(p[3]);
    else    //point is outside => ask GlobalWeatherDatabase
	return global->get(p);
}

FGPhysicalProperties FGLocalWeatherDatabase::get(const sgVec2& p) const
{
    sgVec3 temp;
    sgSetVec3(temp, p[0], p[1], 0.0);

    unsigned int a = AreaWith(temp);
    if (a != 0)
	return WeatherAreas[a-1].get();
    else    //point is outside => ask GlobalWeatherDatabase
	return global->get(p);
}

WeatherPrecision FGLocalWeatherDatabase::getAirDensity(const sgVec3& p) const
{
    FGPhysicalProperty dummy;
    unsigned int a = AreaWith(p);
    if (a != 0)
	dummy = WeatherAreas[a-1].get(p[3]);
    else    //point is outside => ask GlobalWeatherDatabase
	dummy = global->get(p);

    return 
	(dummy.AirPressure*FG_WEATHER_DEFAULT_AIRDENSITY*FG_WEATHER_DEFAULT_TEMPERATURE) / 
	(dummy.Temperature*FG_WEATHER_DEFAULT_AIRPRESSURE);
}

/****************************************************************************/
/* Add a weather feature at the point p and surrounding area		    */
/****************************************************************************/
void FGLocalWeatherDatabase::addWind(const WeatherPrecision alt, const sgVec3& x, const sgVec2& p)
{
    unsigned int a = AreaWith(p);
    if (a != 0)
	WeatherAreas[a-1].addWind(alt, x);
}

void FGLocalWeatherDatabase::addTurbulence(const WeatherPrecision alt, const sgVec3& x, const sgVec2& p)
{
    unsigned int a = AreaWith(p);
    if (a != 0)
	WeatherAreas[a-1].addTurbulence(alt, x);
}

void FGLocalWeatherDatabase::addTemperature(const WeatherPrecision alt, const WeatherPrecision x, const sgVec2& p)
{
    unsigned int a = AreaWith(p);
    if (a != 0)
	WeatherAreas[a-1].addTemperature(alt, x);
}

void FGLocalWeatherDatabase::addAirPressure(const WeatherPrecision alt, const WeatherPrecision x, const sgVec2& p)
{
    unsigned int a = AreaWith(p);
    if (a != 0)
	WeatherAreas[a-1].addAirPressure(alt, x);
}

void FGLocalWeatherDatabase::addVaporPressure(const WeatherPrecision alt, const WeatherPrecision x, const sgVec2& p)
{
    unsigned int a = AreaWith(p);
    if (a != 0)
	WeatherAreas[a-1].addVaporPressure(alt, x);
}

void FGLocalWeatherDatabase::addCloud(const WeatherPrecision alt, const FGCloudItem& x, const sgVec2& p)
{
    unsigned int a = AreaWith(p);
    if (a != 0)
	WeatherAreas[a-1].addCloud(alt, x);
}

void FGLocalWeatherDatabase::setSnowRainIntensity(const WeatherPrecision x, const sgVec2& p)
{
    unsigned int a = AreaWith(p);
    if (a != 0)
	WeatherAreas[a-1].setSnowRainIntensity(x);
}

void FGLocalWeatherDatabase::setSnowRainType(const SnowRainType x, const sgVec2& p)
{
    unsigned int a = AreaWith(p);
    if (a != 0)
	WeatherAreas[a-1].setSnowRainType(x);
}

void FGLocalWeatherDatabase::setLightningProbability(const WeatherPrecision x, const sgVec2& p)
{
    unsigned int a = AreaWith(p);
    if (a != 0)
	WeatherAreas[a-1].setLightningProbability(x);
}

void FGLocalWeatherDatabase::addProperties(const FGPhysicalProperties2D& x)
{
    if (DatabaseStatus==use_global)
    {
	global->add(x);

	//BAD, BAD, BAD thing I'm doing here: I'm adding to the global database a point that
	//changes my voronoi diagram but I don't update it! instead I'm changing one local value
	//that could be anywhere!!
	//This only *might* work when the plane moves so far so fast that the diagram gets new
	//calculated soon...
	unsigned int a = AreaWith(x.p);
	if (a != 0)
	    WeatherAreas[a-1].setStoredWeather(x);
    }
    else
    {
	unsigned int a = AreaWith(x.p);
	if (a != 0)
	    WeatherAreas[a-1].setStoredWeather(x);
    }
}

void FGLocalWeatherDatabase::setProperties(const FGPhysicalProperties2D& x)
{
    if (DatabaseStatus==use_global)
    {
	global->change(x);

	//BAD, BAD, BAD thing I'm doing here: I'm adding to the global database a point that
	//changes my voronoi diagram but I don't update it! Instead I'm changing one local value
	//that could be anywhere!!
	//This only *might* work when the plane moves so far so fast that the diagram gets newly
	//calculated soon...
	unsigned int a = AreaWith(x.p);
	if (a != 0)
	    WeatherAreas[a-1].setStoredWeather(x);
    }
    else
    {
	unsigned int a = AreaWith(x.p);
	if (a != 0)
	    WeatherAreas[a-1].setStoredWeather(x);
    }
}

void fgUpdateWeatherDatabase(void)
{
    //cerr << "FGLocalWeatherDatabase::update()\n";
    sgVec3 position;
    sgSetVec3(position, 
	current_aircraft.fdm_state->get_Latitude(),
	current_aircraft.fdm_state->get_Longitude(),
	current_aircraft.fdm_state->get_Altitude() * FEET_TO_METER);

    WeatherDatabase->update( position );
}

