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

#include "MetarPropertiesATISInformationProvider.hxx"
#include <Main/globals.hxx>
#include <simgear/constants.h>

using std::string;

MetarPropertiesATISInformationProvider::MetarPropertiesATISInformationProvider( SGPropertyNode_ptr metar ) :
  _metar( metar )
{
}

MetarPropertiesATISInformationProvider::~MetarPropertiesATISInformationProvider()
{
}

bool MetarPropertiesATISInformationProvider::isValid()
{
  return _metar->getBoolValue( "valid", false );
}

string MetarPropertiesATISInformationProvider::airportId()
{
  return _metar->getStringValue( "station-id", "xxxx" );
}

long MetarPropertiesATISInformationProvider::getTime()
{
  return makeAtisTime( 0,
    _metar->getIntValue( "hour" ) % 24, 
    _metar->getIntValue( "minute" ) % 60 );
}

int MetarPropertiesATISInformationProvider::getWindDeg()
{
   return _metar->getIntValue( "base-wind-dir-deg" );
}

int MetarPropertiesATISInformationProvider::getWindMinDeg()
{
   return _metar->getIntValue( "base-wind-range-from" );
}
int MetarPropertiesATISInformationProvider::getWindMaxDeg()
{
   return _metar->getIntValue( "base-wind-range-to" );
}
int MetarPropertiesATISInformationProvider::getWindSpeedKt()
{
  return _metar->getIntValue( "base-wind-speed-kt" );
}

int MetarPropertiesATISInformationProvider::getGustsKt()
{
  return _metar->getIntValue( "gust-wind-speed-kt" );
}


int MetarPropertiesATISInformationProvider::getQnh()
{
  return _metar->getDoubleValue("pressure-sea-level-inhg") * SG_INHG_TO_PA / 100.0;
}

bool MetarPropertiesATISInformationProvider::isCavok()
{
  return _metar->getBoolValue( "cavok" );
}

int MetarPropertiesATISInformationProvider::getVisibilityMeters()
{
  return _metar->getIntValue( "min-visibility-m" );
}

string MetarPropertiesATISInformationProvider::getPhenomena()
{
  return _metar->getStringValue( "decoded" );
}

ATISInformationProvider::CloudEntries MetarPropertiesATISInformationProvider::getClouds()
{
  CloudEntries reply;

  using simgear::PropertyList;
  PropertyList layers = _metar->getNode("clouds", true )->getChildren("layer");
  for( PropertyList::iterator it = layers.begin(); it != layers.end(); ++it ) {
    const char * coverage = (*it)->getStringValue("coverage","clear");
    double elevation = (*it)->getDoubleValue("elevation-ft", -9999 );
    if( elevation > 0 ) {
      reply[elevation] = coverage;
    }
  }
  return reply;
}

int MetarPropertiesATISInformationProvider::getTemperatureDeg()
{
  return _metar->getIntValue( "temperature-degc" );
}

int MetarPropertiesATISInformationProvider::getDewpointDeg()
{
  return _metar->getIntValue( "dewpoint-degc" );
}

string MetarPropertiesATISInformationProvider::getTrend()
{
  return "nosig";
}

