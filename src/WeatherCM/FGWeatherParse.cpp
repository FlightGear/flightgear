/*****************************************************************************

 Module:       FGWeatherParse.cpp
 Author:       Christian Mayer
 Date started: 28.05.99
 Called by:    FGMicroWeather

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
Parse the weather that can be downloaded from 

    http://129.13.102.67/out/flight/yymmddhhdata.txt.gz

where yy stands for the year, mm for the month, dd for the day and hh for the
hour.
The columns are explained at

    http://129.13.102.67/out/flight/kopf.txt

and a list of the stations can be found at

    http://129.13.102.67/out/flight/wmoconv.txt.gz

Many thanks to Georg Mueller (Georg.Mueller@imk.fzk.de) of the 

    Institut fuer Meteorologie und Klimaforschung, Universitaet Karlsruhe

for makeking such a service aviable. 
You can also visit his homepage at http://www.wetterzentrale.de

HISTORY
------------------------------------------------------------------------------
18.10.1999 Christian Mayer	Created
14.12.1999 Christian Mayer	minor internal changes
*****************************************************************************/

/****************************************************************************/
/* INCLUDES								    */
/****************************************************************************/
#include <simgear/constants.h>

#include "FGWeatherParse.h"
#include "FGWeatherUtils.h"

/****************************************************************************/
/********************************** CODE ************************************/
/****************************************************************************/

FGWeatherParse::FGWeatherParse()
{
}

FGWeatherParse::~FGWeatherParse()
{
}

void FGWeatherParse::input(const char *file)
{
    unsigned int nr = 0;

    fg_gzifstream in;

    cerr << "Parsing \"" << file << "\" for weather datas:\n";
    
    in.open( file );
    
    if (! in.is_open() )
    {
	cerr << "Couldn't find that file!\nExiting...\n";
	exit(-1);
    }
    else
    {
	bool skip = false;
	while ( in )
	{
	    entry temp;
	    
	    in >> temp.year; 
	    in >> temp.month; 
	    in >> temp.day; 
	    in >> temp.hour; 
	    in >> temp.station_number; 
	    in >> temp.lat; 
	    in >> temp.lon; 
	    in >> temp.x_wind; 
	    in >> temp.y_wind; 
	    in >> temp.temperature; 
	    in >> temp.dewpoint; 
	    in >> temp.airpressure; 
	    in >> temp.airpressure_history[0]; 
	    in >> temp.airpressure_history[1]; 
	    in >> temp.airpressure_history[2]; 
	    in >> temp.airpressure_history[3]; 
	    
	    for (int i = 0; i < weather_station.size(); i++)
	    {
		if ((weather_station[i].lat == temp.lat) && (weather_station[i].lon == temp.lon))
		{
			// Two weatherstations are at the same positon 
			//   => averageing both

			// just taking care of the stuff that metters for us
			weather_station[i].x_wind      += temp.x_wind;      weather_station[i].x_wind      *= 0.5;
			weather_station[i].y_wind      += temp.y_wind;      weather_station[i].y_wind      *= 0.5;
			weather_station[i].temperature += temp.temperature; weather_station[i].temperature *= 0.5;
			weather_station[i].dewpoint    += temp.dewpoint;    weather_station[i].dewpoint    *= 0.5;
			weather_station[i].airpressure += temp.airpressure; weather_station[i].airpressure *= 0.5;

			skip = true;
		}
	    }
	    
	    if (skip == false)
		weather_station.push_back( temp );
	    
	    skip = false;
	    
	    // output a point to ease the waiting
	    if ( ((nr++)%100) == 0 )
		cerr << ".";
	}
	
	cerr << "\n" << nr << " stations read\n";
    }
}

FGPhysicalProperties FGWeatherParse::getFGPhysicalProperties(const unsigned int nr) const
{
    FGPhysicalProperties ret_val;

    //chache this entry
    entry this_entry = weather_station[nr];
    
    ret_val.Wind[-1000.0]          = FGWindItem(this_entry.x_wind, this_entry.y_wind, 0.0);
    ret_val.Wind[10000.0]          = FGWindItem(this_entry.x_wind, this_entry.y_wind, 0.0);
    ret_val.Temperature[0.0]       = Celsius( this_entry.temperature );
    ret_val.AirPressure            = FGAirPressureItem( this_entry.airpressure * 10.0 );    //*10 to go from 10 hPa to Pa

    //I have the dewpoint and the temperature, so I can get the vapor pressure
    ret_val.VaporPressure[-1000.0] = sat_vp( this_entry.dewpoint );
    ret_val.VaporPressure[10000.0] = sat_vp( this_entry.dewpoint );

    //I've got no ideas about clouds...
    //ret_val.Clouds[0]              = 0.0;
    
    return ret_val;
}

void FGWeatherParse::getPosition(const unsigned int nr, sgVec2 pos) const
{
    //set the position of the station
    sgSetVec2( pos, weather_station[nr].lat * SGD_DEGREES_TO_RADIANS, weather_station[nr].lon * SGD_DEGREES_TO_RADIANS ); 
}

