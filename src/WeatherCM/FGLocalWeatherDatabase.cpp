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
14.12.1999 Christian Mayer	Changed the internal structure to use Dave
                                Eberly's spherical interpolation code. This
				stops our dependancy on the (ugly) voronoi
				code and simplyfies the code structure a lot.
07.05.2000 Tony Peden           Added functionality to get the weather data
                                on 'the bus'
18.05.2000 Christian Mayer      Minor clean-ups. Changed the code to use 
                                FGWeatherUtils.h for unit conversion
*****************************************************************************/

/****************************************************************************/
/* INCLUDES								    */
/****************************************************************************/
#include <simgear/compiler.h>
#include <simgear/constants.h>

#include <Aircraft/aircraft.hxx>

#include "FGLocalWeatherDatabase.h"

#include "FGWeatherParse.h"

#include "FGWeatherUtils.h"

/****************************************************************************/
/********************************** CODE ************************************/
/****************************************************************************/

FGLocalWeatherDatabase* FGLocalWeatherDatabase::theFGLocalWeatherDatabase = 0;
FGLocalWeatherDatabase *WeatherDatabase;

void FGLocalWeatherDatabase::init( const WeatherPrecision visibility,
				   const DatabaseWorkingType type,
				   const string &root )
{
    cerr << "Initializing FGLocalWeatherDatabase\n";
    cerr << "-----------------------------------\n";

    if (theFGLocalWeatherDatabase)
    {
	cerr << "Error: only one local weather allowed";
	exit(-1);
    }

    setWeatherVisibility(visibility);

    DatabaseStatus = type;
    database = 0;	    //just get sure...

    Thunderstorm = false;
    //I don't need to set theThunderstorm as Thunderstorm == false

    switch(DatabaseStatus)
    {
    case use_global:
	{
	    cerr << "Error: there's no global database anymore!\n";
	    exit(-1);
	}
	break;

    case use_internet:
	{
	    FGWeatherParse *parsed_data = new FGWeatherParse();

	    parsed_data->input( "weather/current.gz" );
	    unsigned int n = parsed_data->stored_stations();

	    sgVec2               *p = new sgVec2              [n];
	    FGPhysicalProperties *f = new FGPhysicalProperties[n];

	    // fill the database
	    for (unsigned int i = 0; i < n; i++) 
	    {
		f[i] = parsed_data->getFGPhysicalProperties(i);
		parsed_data->getPosition(i, p[i]);

		if ( (i%100) == 0)
		    cerr << ".";
	    }

	    // free the memory of the parsed data to ease the required memory
	    // for the very memory consuming spherical interpolation
	    delete parsed_data;

	    //and finally init the interpolation
	    cerr << "\nInitialiating Interpolation. (2-3 minutes on a PII-350)\n";
	    database = new SphereInterpolate<FGPhysicalProperties>(n, p, f);

	    //and free my allocations:
	    delete[] p;
	    delete[] f;
	    cerr << "Finished weather init.\n";

	}
	break;

    case distant:
	cerr << "FGLocalWeatherDatabase error: Distant database isn't implemented yet!\n";
	cerr << "  using random mode instead!\n";
    case random:
    case manual:
    case default_mode:
	{
	    double x[2] = {0.0,  0.0};	//make an standard weather that's the same at the whole world
	    double y[2] = {0.0,  0.0};	//make an standard weather that's the same at the whole world
	    double z[2] = {1.0, -1.0};	//make an standard weather that's the same at the whole world
	    FGPhysicalProperties f[2];	//make an standard weather that's the same at the whole world
	    database = new SphereInterpolate<FGPhysicalProperties>(2,x,y,z,f);
	}
	break;

    default:
	cerr << "FGLocalWeatherDatabase error: Unknown database type specified!\n";
    };
}

FGLocalWeatherDatabase::~FGLocalWeatherDatabase()
{
    //Tidying up:
    delete database;
}

/****************************************************************************/
/* reset the whole database						    */
/****************************************************************************/
void FGLocalWeatherDatabase::reset(const DatabaseWorkingType type)
{
    cerr << "FGLocalWeatherDatabase::reset isn't supported yet\n";
}

/****************************************************************************/
/* update the database. Since the last call we had dt seconds		    */
/****************************************************************************/
void FGLocalWeatherDatabase::update(const WeatherPrecision dt)
{
    //if (DatabaseStatus==use_global)
    //	global->update(dt);
}

void FGLocalWeatherDatabase::update(const sgVec3& p) //position has changed
{
    sgCopyVec3(last_known_position, p);

    //uncomment this when you are using the GlobalDatabase
    /*
    cerr << "****\nupdate(p) inside\n";
    cerr << "Parameter: " << p[0] << "/" << p[1] << "/" << p[2] << "\n";
    sgVec2 p2d;
    sgSetVec2( p2d, p[0], p[1] );
    cerr << FGPhysicalProperties2D(get(p2d), p2d);
    cerr << "****\n";
    */
    
}

void FGLocalWeatherDatabase::update(const sgVec3& p, const WeatherPrecision dt)   //time and/or position has changed
{
    sgCopyVec3(last_known_position, p);
}

/****************************************************************************/
/* Get the physical properties on the specified point p	out of the database */
/****************************************************************************/
FGPhysicalProperty FGLocalWeatherDatabase::get(const sgVec3& p) const
{
    return FGPhysicalProperty(database->Evaluate(p), p[3]);
}

#ifdef macintosh
    /* fix a problem with mw compilers in that they don't know the
       difference between the next two methods. Since the first one
       doesn't seem to be used anywhere, I commented it out. This is
       supposed to be fixed in the forthcoming CodeWarrior Release
       6. */
#else
FGPhysicalProperties FGLocalWeatherDatabase::get(const sgVec2& p) const
{
    return database->Evaluate(p);
}
#endif

WeatherPrecision FGLocalWeatherDatabase::getAirDensity(const sgVec3& p) const
{
    FGPhysicalProperty dummy(database->Evaluate(p), p[3]);

    return 
	(dummy.AirPressure*FG_WEATHER_DEFAULT_AIRDENSITY*FG_WEATHER_DEFAULT_TEMPERATURE) / 
	(dummy.Temperature*FG_WEATHER_DEFAULT_AIRPRESSURE);
}


void FGLocalWeatherDatabase::setSnowRainIntensity(const WeatherPrecision x, const sgVec2& p)
{
    /* not supported yet */
}

void FGLocalWeatherDatabase::setSnowRainType(const SnowRainType x, const sgVec2& p)
{
    /* not supported yet */
}

void FGLocalWeatherDatabase::setLightningProbability(const WeatherPrecision x, const sgVec2& p)
{
    /* not supported yet */
}

void FGLocalWeatherDatabase::setProperties(const FGPhysicalProperties2D& x)
{
    /* not supported yet */
}

void fgUpdateWeatherDatabase(void)
{
    sgVec3 position;
    sgVec3 wind;
    
    
    sgSetVec3(position, 
        current_aircraft.fdm_state->get_Latitude(),
        current_aircraft.fdm_state->get_Longitude(),
        current_aircraft.fdm_state->get_Altitude() * FEET_TO_METER);
    
    WeatherDatabase->update( position );
       
    // get the data on 'the bus' for the FDM

   /*  FGPhysicalProperty porperty = WeatherDatabase->get(position);

    current_aircraft.fdm_state->set_Static_temperature( Kelvin2Rankine(porperty.Temperature) );
    current_aircraft.fdm_state->set_Static_pressure( Pascal2psf(porperty.AirPressure) );

    current_aircraft.fdm_state->set_Density( SIdensity2JSBsim( Density(porperty.AirPressure, porperty.Temperature) ) );
    
#define MSTOFPS  3.2808 //m/s to ft/s
    current_aircraft.fdm_state->set_Velocities_Local_Airmass(porperty.Wind[1]*MSTOFPS,
        porperty.Wind[0]*MSTOFPS,
        porperty.Wind[2]*MSTOFPS); */

    
}

