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
18.07.2001 Christian Mayer      Added the posibility to limit the amount of 
                                stations for a faster init.
*****************************************************************************/

/****************************************************************************/
/* INCLUDES								    */
/****************************************************************************/
#include <simgear/compiler.h>
#include <simgear/constants.h>

#include <Aircraft/aircraft.hxx>
#include <Main/fg_props.hxx>

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
    database_logic = 0;	    //just get sure...

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
	    sgVec2       *p;
            unsigned int *f;
            string path_to_weather = root + "/weather/current.txt.gz";
	    parsed_data->input( path_to_weather.c_str() );
	    unsigned int n = parsed_data->stored_stations();
            int m = fgGetInt("/environment/weather/max-stations", -1);

            if ( ( m < 0 ) || ( m > n ) )
            {
                m = n;

	        p = new sgVec2[n];
	        f = new unsigned int[n];

	        // fill the database
	        for (unsigned int i = 0; i < n; i++) 
	        {
                    f[i] = i;
		    database_data[i] = parsed_data->getFGPhysicalProperties(i);
		    parsed_data->getPosition(i, p[i]);

		    if ( (i%100) == 0)
		        cerr << ".";
	        }
            }
            else
            {   // we have to limit the amount of stations

                //store the "distance" between the station and the current
                //position. As the distance is calculated from the lat/lon
                //values it's not worth much - but it's good enough for
                //comparison
                map<float, unsigned int> squared_distance;
                sgVec2 cur_pos;

                cur_pos[0] = cache->last_known_position[0];
                cur_pos[1] = cache->last_known_position[1];
                
                unsigned int i;
                for( i = 0; i < n; i++ )
                {
                  sgVec2 pos;
                  parsed_data->getPosition(i, pos);
                  squared_distance[sgDistanceSquaredVec2(cur_pos, pos)] = i;
                }

                p = new sgVec2      [m];
                f = new unsigned int[m];

                map<float, unsigned int>::const_iterator ci;
                ci = squared_distance.begin();

	        // fill the database
	        for ( i = 0; i < m; i++ ) 
                { 
                    f[i] = i;
		    database_data.push_back( parsed_data->getFGPhysicalProperties(ci->second) );
		    parsed_data->getPosition(ci->second, p[i]);

		    if ( (i%100) == 0)
		        cerr << ".";

                    ci++;
	        }
            }

	    // free the memory of the parsed data to ease the required memory
	    // for the very memory consuming spherical interpolation
	    delete parsed_data;

	    //and finally init the interpolation
	    cerr << "\nInitialiating Interpolation. (2-3 minutes on a PII-350 for ca. 3500 stations)\n";
	    database_logic = new SphereInterpolate(m, p, f);

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
            unsigned int f[2] = {0, 0};
            database_data.push_back( FGPhysicalProperties() ); // == database_date[0]
 	    database_logic = new SphereInterpolate(2,x,y,z,f);
	}
	break;

    default:
	cerr << "FGLocalWeatherDatabase error: Unknown database type specified!\n";
    };

    cache->latitude_deg  = fgGetNode("/position/latitude-deg" );
    cache->longitude_deg = fgGetNode("/position/longitude-deg");
    cache->altitude_ft   = fgGetNode("/position/altitude-ft"  );

}

void FGLocalWeatherDatabase::bind()
{
    fgTie("/environment/weather/wind-north-mps", this, &FGLocalWeatherDatabase::get_wind_north);
    fgTie("/environment/weather/wind-east-mps", this, &FGLocalWeatherDatabase::get_wind_east);
    fgTie("/environment/weather/wind-up-mps", this, &FGLocalWeatherDatabase::get_wind_up);
    fgTie("/environment/weather/temperature-K", this, &FGLocalWeatherDatabase::get_temperature);
    fgTie("/environment/weather/air-pressure-Pa", this, &FGLocalWeatherDatabase::get_air_pressure);
    fgTie("/environment/weather/vapor-pressure-Pa", this, &FGLocalWeatherDatabase::get_vapor_pressure);
    fgTie("/environment/weather/air-density", this, &FGLocalWeatherDatabase::get_air_density);
    

  SGPropertyNode * station_nodes = fgGetNode("/environment/weather");
  if (station_nodes == 0) {
    cerr << "No weatherstations (/environment/weather)!!";
    return;
  }
  
  int index = 0;
  for(vector<FGPhysicalProperties>::iterator it = database_data.begin(); it != database_data.end(); it++)
  {
      SGPropertyNode * station = station_nodes->getNode("station", index, true);

      station -> tie("air-pressure-Pa", 
        SGRawValueMethods<FGAirPressureItem,WeatherPrecision>(
          database_data[0].AirPressure,
          &FGAirPressureItem::getValue,
          &FGAirPressureItem::setValue)
        ,false);

    int i;
    for( i = 0; i < database_data[index].Wind.size(); i++)
    {
      SGPropertyNode * wind = station->getNode("wind", i, true);
      wind -> tie("north-mps", 
        SGRawValueMethodsIndexed<FGPhysicalProperties,WeatherPrecision>(
          database_data[index], i,
          &FGPhysicalProperties::getWind_x,
          &FGPhysicalProperties::setWind_x)
        ,false);
      wind -> tie("east-mps", 
        SGRawValueMethodsIndexed<FGPhysicalProperties,WeatherPrecision>(
          database_data[index], i,
          &FGPhysicalProperties::getWind_y,
          &FGPhysicalProperties::setWind_y)
        ,false);
      wind -> tie("up-mps", 
        SGRawValueMethodsIndexed<FGPhysicalProperties,WeatherPrecision>(
          database_data[index], i,
          &FGPhysicalProperties::getWind_z,
          &FGPhysicalProperties::setWind_z)
        ,false);
      wind -> tie("altitude-m", 
        SGRawValueMethodsIndexed<FGPhysicalProperties,WeatherPrecision>(
          database_data[index], i,
          &FGPhysicalProperties::getWind_a,
          &FGPhysicalProperties::setWind_a)
        ,false);
    }

    for( i = 0; i < database_data[index].Temperature.size(); i++)
    {
      SGPropertyNode * temperature = station->getNode("temperature", i, true);
      temperature -> tie("value-K", 
        SGRawValueMethodsIndexed<FGPhysicalProperties,WeatherPrecision>(
          database_data[index], i,
          &FGPhysicalProperties::getTemperature_x,
          &FGPhysicalProperties::setTemperature_x)
        ,false);
      temperature -> tie("altitude-m", 
        SGRawValueMethodsIndexed<FGPhysicalProperties,WeatherPrecision>(
          database_data[index], i,
          &FGPhysicalProperties::getTemperature_a,
          &FGPhysicalProperties::setTemperature_a)
        ,false);
    }

    for( i = 0; i < database_data[index].VaporPressure.size(); i++)
    {
      SGPropertyNode * vaporpressure = station->getNode("vapor-pressure", i, true);
      vaporpressure -> tie("value-Pa", 
        SGRawValueMethodsIndexed<FGPhysicalProperties,WeatherPrecision>(
          database_data[index], i,
          &FGPhysicalProperties::getVaporPressure_x,
          &FGPhysicalProperties::setVaporPressure_x)
        ,false);
      vaporpressure -> tie("altitude-m", 
        SGRawValueMethodsIndexed<FGPhysicalProperties,WeatherPrecision>(
          database_data[index], i,
          &FGPhysicalProperties::getVaporPressure_a,
          &FGPhysicalProperties::setVaporPressure_a)
        ,false);
    }

    index++;
  }
}

void FGLocalWeatherDatabase::unbind()
{
    fgUntie("/environment/weather/wind-north-mps");
    fgUntie("/environment/weather/wind-east-mps");
    fgUntie("/environment/weather/wind-up-mps");
    fgUntie("/environment/weather/temperature-K");
    fgUntie("/environment/weather/air-pressure-Pa");
    fgUntie("/environment/weather/vapor-pressure-Pa");
    fgUntie("/environment/weather/air-density");
}

FGLocalWeatherDatabase::~FGLocalWeatherDatabase()
{
    //Tidying up:
    delete database_logic;
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
    //uncomment this when you are using the GlobalDatabase
    /*
    cerr << "****\nupdate(p) inside\n";
    cerr << "Parameter: " << p[0] << "/" << p[1] << "/" << p[2] << "\n";
    cerr << FGPhysicalProperties2D(get(p2d), p2d);
    cerr << "****\n";
    */
    
}

void FGLocalWeatherDatabase::update(const sgVec3& p, const WeatherPrecision dt)   //time and/or position has changed
{
}

/****************************************************************************/
/* Get the physical properties on the specified point p	out of the database */
/****************************************************************************/
FGPhysicalProperty FGLocalWeatherDatabase::get(const sgVec3& p) const
{
  // check for bogous altitudes. Dunno why, but FGFS want's to know the
  // weather at an altitude of roughly -3000 meters...
  if (p[2] < -500.0f)
    return FGPhysicalProperty(DatabaseEvaluate(p), -500.0f);

  return FGPhysicalProperty(DatabaseEvaluate(p), p[2]);
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
    return DatabaseEvaluate(p);
}
#endif

WeatherPrecision FGLocalWeatherDatabase::getAirDensity(const sgVec3& p) const
{
    FGPhysicalProperty dummy(DatabaseEvaluate(p), p[2]);

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
    
    sgSetVec3(position, 
        current_aircraft.fdm_state->get_Latitude(),
        current_aircraft.fdm_state->get_Longitude(),
        current_aircraft.fdm_state->get_Altitude() * SG_FEET_TO_METER);
    
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

