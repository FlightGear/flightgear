/*
Provide Data for the ATIS Encoder from metarproperties
Copyright (C) 2014 Torsten Dreyer

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "CurrentWeatherATISInformationProvider.hxx"
#include <Main/fg_props.hxx>

using std::string;

CurrentWeatherATISInformationProvider::CurrentWeatherATISInformationProvider( const std::string & airportId ) :
  _airportId(airportId),
  _environment(fgGetNode("/environment"))
{
}

static inline int roundToInt( double d )
{
  return (static_cast<int>(10.0 * (d + .5))) / 10;
}

static inline int roundToInt( SGPropertyNode_ptr n )
{
  return roundToInt( n->getDoubleValue() );
}

static inline int roundToInt( SGPropertyNode_ptr n, const char * child )
{
  return roundToInt( n->getNode(child,true) );
}


CurrentWeatherATISInformationProvider::~CurrentWeatherATISInformationProvider()
{
}

bool CurrentWeatherATISInformationProvider::isValid()
{
  return true;
}

string CurrentWeatherATISInformationProvider::airportId()
{
  return _airportId;
}

long CurrentWeatherATISInformationProvider::getTime()
{
  int h = fgGetInt( "/sim/time/utc/hour", 12 );
  int m = 20 + fgGetInt( "/sim/time/utc/minute", 0 ) / 30 ; // fake twice per hour
  return makeAtisTime( 0, h, m );
}

int CurrentWeatherATISInformationProvider::getWindDeg()
{
  // round to 10 degs
  int i = 5 + roundToInt( _environment->getNode("config/boundary/entry[0]/wind-from-heading-deg",true) );
  i /= 10;
  return i*10;
}

int CurrentWeatherATISInformationProvider::getWindSpeedKt()
{
  return roundToInt( _environment, "config/boundary/entry[0]/wind-speed-kt" );
}

int CurrentWeatherATISInformationProvider::getGustsKt()
{
  return 0;
}

int CurrentWeatherATISInformationProvider::getQnh()
{
  return roundToInt( _environment->getNode("pressure-sea-level-inhg",true)->getDoubleValue() * SG_INHG_TO_PA / 100 );
}

bool CurrentWeatherATISInformationProvider::isCavok()
{
  return false;
}

int CurrentWeatherATISInformationProvider::getVisibilityMeters()
{
  return roundToInt( _environment, "ground-visibility-m" );
}

string CurrentWeatherATISInformationProvider::getPhenomena()
{
  return "";
}

ATISInformationProvider::CloudEntries CurrentWeatherATISInformationProvider::getClouds()
{
  using simgear::PropertyList;

  ATISInformationProvider::CloudEntries cloudEntries;
  PropertyList layers = _environment->getNode("clouds",true)->getChildren("layer");
  for( PropertyList::iterator it = layers.begin(); it != layers.end(); ++it ) {
    string coverage = (*it)->getStringValue( "coverage", "clear" );
    int alt = roundToInt( (*it)->getDoubleValue("elevation-ft", -9999 ) ) / 100;
    alt *= 100;

    if( coverage != "clear" && alt > 0 )
      cloudEntries[alt] = coverage;
    
  }
  return cloudEntries;
}

int CurrentWeatherATISInformationProvider::getTemperatureDeg()
{
  return roundToInt( _environment, "temperature-sea-level-degc" );
}

int CurrentWeatherATISInformationProvider::getDewpointDeg()
{
  return roundToInt( _environment, "dewpoint-sea-level-degc" );
}

string CurrentWeatherATISInformationProvider::getTrend()
{
  return "nosig";
}

