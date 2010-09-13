// metarproperties.cxx -- Parse a METAR and write properties
//
// Written by David Megginson, started May 2002.
// Rewritten by Torsten Dreyer, August 2010
//
// Copyright (C) 2002  David Megginson - david@megginson.com
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "metarproperties.hxx"
#include "fgmetar.hxx"
#include "environment.hxx"
#include "atmosphere.hxx"
#include <simgear/scene/sky/cloud.hxx>
#include <simgear/structure/exception.hxx>

using std::string;

namespace Environment {

static vector<string> coverage_string;

MetarProperties::MetarProperties( SGPropertyNode_ptr rootNode ) :
  _rootNode(rootNode),
  _metarValidNode( rootNode->getNode( "valid", true ) ),
  _station_elevation(0.0),
  _station_latitude(0.0),
  _station_longitude(0.0),
  _min_visibility(16000.0),
  _max_visibility(16000.0),
  _base_wind_dir(0),
  _base_wind_range_from(0),
  _base_wind_range_to(0),
  _wind_speed(0.0),
  _wind_from_north_fps(0.0),
  _wind_from_east_fps(0.0),
  _gusts(0.0),
  _temperature(0.0),
  _dewpoint(0.0),
  _humidity(0.0),
  _pressure(0.0),
  _sea_level_temperature(0.0),
  _sea_level_dewpoint(0.0),
  _sea_level_pressure(29.92),
  _rain(0.0),
  _hail(0.0),
  _snow(0.0),
  _snow_cover(false)
{
  // Hack to avoid static initialization order problems on OSX
  if( coverage_string.size() == 0 ) {
    coverage_string.push_back(SGCloudLayer::SG_CLOUD_CLEAR_STRING);
    coverage_string.push_back(SGCloudLayer::SG_CLOUD_FEW_STRING);
    coverage_string.push_back(SGCloudLayer::SG_CLOUD_SCATTERED_STRING);
    coverage_string.push_back(SGCloudLayer::SG_CLOUD_BROKEN_STRING);
    coverage_string.push_back(SGCloudLayer::SG_CLOUD_OVERCAST_STRING);
  }
  // don't tie metar-valid, so listeners get triggered
  _metarValidNode->setBoolValue( false );
  _tiedProperties.setRoot( _rootNode );
  _tiedProperties.Tie("data", this, &MetarProperties::get_metar, &MetarProperties::set_metar );
  _tiedProperties.Tie("station-id", this, &MetarProperties::get_station_id );
  _tiedProperties.Tie("station-elevation-ft", &_station_elevation );
  _tiedProperties.Tie("station-latitude-deg", &_station_latitude );
  _tiedProperties.Tie("station-longitude-deg", &_station_longitude );
  _tiedProperties.Tie("min-visibility-m", &_min_visibility );
  _tiedProperties.Tie("max-visibility-m", &_max_visibility );
  _tiedProperties.Tie("base-wind-range-from", &_base_wind_range_from );
  _tiedProperties.Tie("base-wind-range-to", &_base_wind_range_to );
  _tiedProperties.Tie("base-wind-speed-kt", &_wind_speed );
  _tiedProperties.Tie("base-wind-dir-deg", &_base_wind_dir );
  _tiedProperties.Tie("base-wind-from-north-fps", &_wind_from_north_fps );
  _tiedProperties.Tie("base-wind-from-east-fps", &_wind_from_east_fps );
  _tiedProperties.Tie("gust-wind-speed-kt", &_gusts );
  _tiedProperties.Tie("temperature-degc", &_temperature );
  _tiedProperties.Tie("dewpoint-degc", &_dewpoint );
  _tiedProperties.Tie("rel-humidity-norm", &_humidity );
  _tiedProperties.Tie("pressure-inhg", &_pressure );
  _tiedProperties.Tie("temperature-sea-level-degc", &_sea_level_temperature );
  _tiedProperties.Tie("dewpoint-sea-level-degc", &_sea_level_dewpoint );
  _tiedProperties.Tie("pressure-sea-level-inhg", &_sea_level_pressure );
  _tiedProperties.Tie("rain-norm", &_rain );
  _tiedProperties.Tie("hail-norm", &_hail );
  _tiedProperties.Tie("snow-norm", &_snow);
  _tiedProperties.Tie("snow-cover", &_snow_cover );
}

MetarProperties::~MetarProperties()
{
}


static const double thickness_value[] = { 0, 65, 600, 750, 1000 };

void MetarProperties::set_metar( const char * metar )
{
    _metar = metar;

    SGSharedPtr<FGMetar> m;
    try {
        m = new FGMetar( _metar );
    }
    catch( sg_io_exception ) {
        SG_LOG( SG_GENERAL, SG_WARN, "Can't parse metar: " << _metar );
        _metarValidNode->setBoolValue(false);
        return;
    }

    _min_visibility = m->getMinVisibility().getVisibility_m();
    _max_visibility = m->getMaxVisibility().getVisibility_m();

    const SGMetarVisibility *dirvis = m->getDirVisibility();
    for ( int i = 0; i < 8; i++, dirvis++) {
        SGPropertyNode *vis = _rootNode->getChild("visibility", i, true);
        double v = dirvis->getVisibility_m();

        vis->setDoubleValue("min-m", v);
        vis->setDoubleValue("max-m", v);
    }

    _base_wind_dir = m->getWindDir();
    _base_wind_range_from = m->getWindRangeFrom();
    _base_wind_range_to = m->getWindRangeTo();
    _wind_speed = m->getWindSpeed_kt();

    double speed_fps = _wind_speed * SG_NM_TO_METER * SG_METER_TO_FEET / 3600.0;
    _wind_from_north_fps = speed_fps * cos((double)_base_wind_dir * SGD_DEGREES_TO_RADIANS);
    _wind_from_east_fps = speed_fps * sin((double)_base_wind_dir * SGD_DEGREES_TO_RADIANS);
    _gusts = m->getGustSpeed_kt();
    _temperature = m->getTemperature_C();
    _dewpoint = m->getDewpoint_C();
    _humidity = m->getRelHumidity();
    _pressure = m->getPressure_inHg();

    {
        // 1. check the id given in the metar
        FGAirport* a = FGAirport::findByIdent(m->getId());
/*
        // 2. if unknown, find closest airport with metar to current position
        if( a == NULL ) {
            SGGeod pos = SGGeod::fromDeg(_longitude_n->getDoubleValue(), _latitude_n->getDoubleValue());
            a = FGAirport::findClosest(pos, 10000.0, &_airportWithMetarFilter);
        }
*/
        // 3. otherwise use ground elevation
        if( a != NULL ) {
            _station_elevation = a->getElevation();
            const SGGeod & towerPosition = a->getTowerLocation();
            _station_latitude = towerPosition.getLatitudeDeg();
            _station_longitude = towerPosition.getLongitudeDeg();
            _station_id = a->ident();
        } else {
            _station_elevation = 0.0;
            _station_latitude = 0.0;
            _station_longitude = 0.0;
            _station_id = "XXXX";
//            station_elevation_ft = _ground_elevation_n->getDoubleValue() * SG_METER_TO_FEET;
//            _station_id = m->getId();
        }
    }

    {    // calculate sea level temperature, dewpoint and pressure
        FGEnvironment dummy; // instantiate a dummy so we can leech a method
        dummy.set_elevation_ft( _station_elevation );
        dummy.set_temperature_degc( _temperature );
        dummy.set_dewpoint_degc( _dewpoint );
        _sea_level_temperature = dummy.get_temperature_sea_level_degc();
        _sea_level_dewpoint = dummy.get_dewpoint_sea_level_degc();

        double elevation_m = _station_elevation * SG_FEET_TO_METER;
        double fieldPressure = FGAtmo::fieldPressure( elevation_m, _pressure * atmodel::inHg );
        _sea_level_pressure = P_layer(0, elevation_m, fieldPressure, _temperature + atmodel::freezing, atmodel::ISA::lam0) / atmodel::inHg;
    }

    vector<SGMetarCloud> cv = m->getClouds();
    vector<SGMetarCloud>::const_iterator cloud, cloud_end = cv.end();

    {
        static const char * LAYER = "layer";
        SGPropertyNode_ptr cloudsNode = _rootNode->getNode("clouds", true );
        const vector<SGMetarCloud> & metarClouds = m->getClouds();
        for( unsigned i = 0; i < 5; i++ ) {
            SGPropertyNode_ptr layerNode = cloudsNode->getChild(LAYER, i, true );
            int coverage = i < metarClouds.size() ? metarClouds[i].getCoverage() : 0;
            double elevation = i >= metarClouds.size() || coverage == 0 ? -9999.0 : metarClouds[i].getAltitude_ft() + _station_elevation;
            layerNode->setStringValue( "coverage", coverage_string[coverage] );
            layerNode->setDoubleValue( "coverage-type", SGCloudLayer::getCoverageType(coverage_string[coverage]) );
            layerNode->setDoubleValue( "elevation-ft", elevation );
            layerNode->setDoubleValue( "thickness-ft", thickness_value[coverage]);
            layerNode->setDoubleValue( "span-m", 40000 );
        }

    }

    _rain = m->getRain();
    _hail = m->getHail();
    _snow = m->getSnow();
    _snow_cover = m->getSnowCover();
    _metarValidNode->setBoolValue(true);
}

} // namespace Environment
