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
#include "metarairportfilter.hxx"
#include <simgear/scene/sky/cloud.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/magvar/magvar.hxx>
#include <simgear/timing/sg_time.hxx>
#include <Main/fg_props.hxx>

using std::string;

namespace Environment {

static vector<string> coverage_string;

/**
 * @brief Helper class to wrap SGMagVar functionality and cache the variation and dip for
 *        a certain position. 
 */
class MagneticVariation : public SGMagVar {
public:
  /**
   * Constructor
   */
  MagneticVariation() : _lat(1), _lon(1), _alt(1) {
    recalc( 0.0, 0.0, 0.0 );
  }

  /**
   * @brief get the magnetic variation for a specific position at the current time
   * @param lon the positions longitude in degrees
   * @param lat the positions latitude in degrees
   * @param alt the positions height above MSL (aka altitude) in feet
   * @return the magnetic variation in degrees
   */
  double get_variation_deg( double lon, double lat, double alt );

  /**
   * @brief get the magnetic dip for a specific position at the current time
   * @param lon the positions longitude in degrees
   * @param lat the positions latitude in degrees
   * @param alt the positions height above MSL (aka altitude) in feet
   * @return the magnetic dip in degrees
   */
  double get_dip_deg( double lon, double lat, double alt );
private:
  void recalc( double lon, double lat, double alt );
  SGTime _time;
  double _lat, _lon, _alt;
};

inline void MagneticVariation::recalc( double lon, double lat, double alt )
{
  // calculation of magnetic variation is expensive. Cache the position
  // and perform this calculation only if it has changed
  if( _lon != lon || _lat != lat || _alt != alt ) {
    SG_LOG(SG_ENVIRONMENT, SG_DEBUG, "Recalculating magvar for lon=" << lon << ", lat=" << lat << ", alt=" << alt );
    _lon = lon;
    _lat = lat;
    _alt = alt;

    lon *= SGD_DEGREES_TO_RADIANS;
    lat *= SGD_DEGREES_TO_RADIANS;
    alt *= SG_FEET_TO_METER;
   _time.update( lon, lat, 0, 0 );
    update( lon, lat, alt, _time.getJD() );
  }
}

inline double MagneticVariation::get_variation_deg( double lon, double lat, double alt )
{
  recalc( lon, lat, alt );
  return get_magvar() * SGD_RADIANS_TO_DEGREES;
}

inline double MagneticVariation::get_dip_deg( double lon, double lat, double alt )
{
  recalc( lon, lat, alt );
  return get_magdip() * SGD_RADIANS_TO_DEGREES;
}

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
  _snow_cover(false),
  _magneticVariation(new MagneticVariation())
{
  // Hack to avoid static initialization order problems on OSX
  if( coverage_string.empty() ) {
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
  _tiedProperties.Tie("station-id", this, &MetarProperties::get_station_id, &MetarProperties::set_station_id );
  _tiedProperties.Tie("station-elevation-ft", &_station_elevation );
  _tiedProperties.Tie("station-latitude-deg", &_station_latitude );
  _tiedProperties.Tie("station-longitude-deg", &_station_longitude );
  _tiedProperties.Tie("station-magnetic-variation-deg", this, &MetarProperties::get_magnetic_variation_deg );
  _tiedProperties.Tie("station-magnetic-dip-deg", this, &MetarProperties::get_magnetic_dip_deg );
  _tiedProperties.Tie("min-visibility-m", &_min_visibility );
  _tiedProperties.Tie("max-visibility-m", &_max_visibility );
  _tiedProperties.Tie("base-wind-range-from", &_base_wind_range_from );
  _tiedProperties.Tie("base-wind-range-to", &_base_wind_range_to );
  _tiedProperties.Tie("base-wind-speed-kt", this, &MetarProperties::get_wind_speed, &MetarProperties::set_wind_speed );
  _tiedProperties.Tie("base-wind-dir-deg", this, &MetarProperties::get_base_wind_dir, &MetarProperties::set_base_wind_dir );
  _tiedProperties.Tie("base-wind-from-north-fps", this, &MetarProperties::get_wind_from_north_fps, &MetarProperties::set_wind_from_north_fps );
  _tiedProperties.Tie("base-wind-from-east-fps",this, &MetarProperties::get_wind_from_east_fps, &MetarProperties::set_wind_from_east_fps );
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
  _tiedProperties.Tie("decoded", this, &MetarProperties::get_decoded );
}

MetarProperties::~MetarProperties()
{
  delete _magneticVariation;
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
        SG_LOG( SG_ENVIRONMENT, SG_WARN, "Can't parse metar: " << _metar );
        _metarValidNode->setBoolValue(false);
        return;
    }

    _decoded.clear();
    const vector<string> weather = m->getWeather();
    for( vector<string>::const_iterator it = weather.begin(); it != weather.end(); ++it ) {
        if( false == _decoded.empty() ) _decoded.append(", ");
        _decoded.append(*it);
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

    set_base_wind_dir(m->getWindDir());
    _base_wind_range_from = m->getWindRangeFrom();
    _base_wind_range_to = m->getWindRangeTo();
    set_wind_speed(m->getWindSpeed_kt());

    _gusts = m->getGustSpeed_kt();
    _temperature = m->getTemperature_C();
    _dewpoint = m->getDewpoint_C();
    _humidity = m->getRelHumidity();
    _pressure = m->getPressure_inHg();

    {
        // 1. check the id given in the metar
        FGAirport* a = FGAirport::findByIdent(m->getId());

        // 2. if unknown, find closest airport with metar to current position
        if( a == NULL ) {
            SGGeod pos = SGGeod::fromDeg(
                fgGetDouble( "/position/longitude-deg", 0.0 ),
                fgGetDouble( "/position/latitude-deg", 0.0 ) );
            a = FGAirport::findClosest(pos, 10000.0, MetarAirportFilter::instance() );
        }

        // 3. otherwise use ground elevation
        if( a != NULL ) {
            _station_elevation = a->getElevation();
            const SGGeod & towerPosition = a->getTowerLocation();
            _station_latitude = towerPosition.getLatitudeDeg();
            _station_longitude = towerPosition.getLongitudeDeg();
            _station_id = a->ident();
        } else {
            _station_elevation = fgGetDouble("/position/ground-elev-m", 0.0 ) * SG_METER_TO_FEET;
            _station_latitude = fgGetDouble( "/position/latitude-deg", 0.0 );
            _station_longitude = fgGetDouble( "/position/longitude-deg", 0.0 );
            _station_id = "XXXX";
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

    bool isBC = false;
    bool isBR = false;
    bool isFG = false;
    bool isMI = false;
    bool isHZ = false;

    {
        for( unsigned i = 0; i < 3; i++ ) {
            SGPropertyNode_ptr n = _rootNode->getChild("weather", i, true );
            vector<struct SGMetar::Weather> weather = m->getWeather2();
            struct SGMetar::Weather * w = i < weather.size() ? &weather[i] : NULL;
            n->getNode("intensity",true)->setIntValue( w != NULL ? w->intensity : 0 );
            n->getNode("vincinity",true)->setBoolValue( w != NULL ? w->vincinity : false );
            for( unsigned j = 0; j < 3; j++ ) { 

                const string & phenomenon = w != NULL && j < w->phenomena.size() ? w->phenomena[j].c_str() : "";
                n->getChild( "phenomenon", j, true )->setStringValue( phenomenon );

                const string & description = w != NULL && j < w->descriptions.size() ? w->descriptions[j].c_str() : "";
                n->getChild( "description", j, true )->setStringValue( description );

                // need to know later, 
                // if its fog(FG) (might be shallow(MI) or patches(BC)) or haze (HZ) or mist(BR)
                if( phenomenon == "FG" ) isFG = true;
                if( phenomenon == "HZ" ) isHZ = true;
                if( phenomenon == "BR" ) isBR = true;
                if( description == "MI" ) isMI = true;
                if( description == "BC" ) isBC = true;
            }
        }
    }

    {
        static const char * LAYER = "layer";
        SGPropertyNode_ptr cloudsNode = _rootNode->getNode("clouds", true );
        const vector<SGMetarCloud> & metarClouds = m->getClouds();
        unsigned layerOffset = 0; // Oh, this is ugly!

        // fog/mist/haze cloud layer does not work with 3d clouds yet :-(
        bool setGroundCloudLayer = _rootNode->getBoolValue("set-ground-cloud-layer", false ) &&
              false == (fgGetBool("/sim/rendering/shader-effects", false ) && 
                        fgGetBool("/sim/rendering/clouds3d-enable", false ) );

        if( setGroundCloudLayer ) {
            // create a cloud layer #0 starting at the ground if its fog, mist or haze

            // make sure layer actually starts at ground and set it's bottom at a constant
            // value below the station's elevation
            const double LAYER_BOTTOM_STATION_OFFSET =
              fgGetDouble( "/environment/params/fog-mist-haze-layer/offset-from-station-elevation-ft", -200 );

            SGMetarCloud::Coverage coverage = SGMetarCloud::COVERAGE_NIL;
            double thickness = 0;
            double alpha = 1.0;

            if( isFG ) { // fog
                coverage = SGMetarCloud::getCoverage( isBC ? 
                    fgGetString( "/environment/params/fog-mist-haze-layer/fog-bc-2dlayer-coverage", SGMetarCloud::COVERAGE_SCATTERED_STRING ) :
                    fgGetString( "/environment/params/fog-mist-haze-layer/fog-2dlayer-coverage", SGMetarCloud::COVERAGE_BROKEN_STRING )
                );

                thickness = isMI ? 
                   fgGetDouble("/environment/params/fog-mist-haze-layer/fog-shallow-thickness-ft",30) - LAYER_BOTTOM_STATION_OFFSET : // shallow fog, 10m/30ft
                   fgGetDouble("/environment/params/fog-mist-haze-layer/fog-thickness-ft",500) - LAYER_BOTTOM_STATION_OFFSET; // fog, 150m/500ft
                alpha =  fgGetDouble("/environment/params/fog-mist-haze-layer/fog-2dlayer-alpha", 1.0);
            } else if( isBR ) { // mist
                coverage = SGMetarCloud::getCoverage(fgGetString("/environment/params/fog-mist-haze-layer/mist-2dlayer-coverage", SGMetarCloud::COVERAGE_OVERCAST_STRING));
                thickness =  fgGetDouble("/environment/params/fog-mist-haze-layer/mist-thickness-ft",2000) - LAYER_BOTTOM_STATION_OFFSET;
                alpha =  fgGetDouble("/environment/params/fog-mist-haze-layer/mist-2dlayer-alpha",0.8);
            } else if( isHZ ) { // haze
                coverage = SGMetarCloud::getCoverage(fgGetString("/environment/params/fog-mist-haze-layer/mist-2dlayer-coverage", SGMetarCloud::COVERAGE_OVERCAST_STRING));
                thickness =  fgGetDouble("/environment/params/fog-mist-haze-layer/haze-thickness-ft",2000) - LAYER_BOTTOM_STATION_OFFSET;
                alpha =  fgGetDouble("/environment/params/fog-mist-haze-layer/haze-2dlayer-alpha",0.6);
            }

            if( coverage != SGMetarCloud::COVERAGE_NIL ) {

                // if there is a layer above the fog, limit the top to one foot below that layer's bottom
                if( metarClouds.size() > 0 && metarClouds[0].getCoverage() != SGMetarCloud::COVERAGE_CLEAR )
                    thickness = metarClouds[0].getAltitude_ft() - LAYER_BOTTOM_STATION_OFFSET - 1;

                SGPropertyNode_ptr layerNode = cloudsNode->getChild(LAYER, 0, true );
                layerNode->setDoubleValue( "coverage-type", SGCloudLayer::getCoverageType(coverage_string[coverage]) );
                layerNode->setStringValue( "coverage", coverage_string[coverage] );
                layerNode->setDoubleValue( "elevation-ft", _station_elevation + LAYER_BOTTOM_STATION_OFFSET );
                layerNode->setDoubleValue( "thickness-ft", thickness );
                layerNode->setDoubleValue( "visibility-m", _min_visibility );
                layerNode->setDoubleValue( "alpha", alpha );
                _min_visibility = _max_visibility =
                  fgGetDouble("/environment/params/fog-mist-haze-layer/visibility-above-layer-m",20000.0); // assume good visibility above the fog
                layerOffset = 1;  // shudder
            }
        } 

        for( unsigned i = 0; i < 5-layerOffset; i++ ) {
            SGPropertyNode_ptr layerNode = cloudsNode->getChild(LAYER, i+layerOffset, true );
            SGMetarCloud::Coverage coverage = i < metarClouds.size() ? metarClouds[i].getCoverage() : SGMetarCloud::COVERAGE_CLEAR;
            double elevation = 
                i >= metarClouds.size() || coverage == SGMetarCloud::COVERAGE_CLEAR ? 
                -9999.0 : 
                metarClouds[i].getAltitude_ft() + _station_elevation;

            layerNode->setDoubleValue( "alpha", 1.0 );
            layerNode->setStringValue( "coverage", coverage_string[coverage] );
            layerNode->setDoubleValue( "coverage-type", SGCloudLayer::getCoverageType(coverage_string[coverage]) );
            layerNode->setDoubleValue( "elevation-ft", elevation );
            layerNode->setDoubleValue( "thickness-ft", thickness_value[coverage]);
            layerNode->setDoubleValue( "span-m", 40000 );
            layerNode->setDoubleValue( "visibility-m", 50.0 );
        }
    }

    _rain = m->getRain();
    _hail = m->getHail();
    _snow = m->getSnow();
    _snow_cover = m->getSnowCover();
    _metarValidNode->setBoolValue(true);
}

void MetarProperties::setStationId( const std::string & value )
{ 
    set_station_id(simgear::strutils::strip(value).c_str());
}

double MetarProperties::get_magnetic_variation_deg() const 
{
  return _magneticVariation->get_variation_deg( _station_longitude, _station_latitude, _station_elevation );
}

double MetarProperties::get_magnetic_dip_deg() const
{
  return _magneticVariation->get_dip_deg( _station_longitude, _station_latitude, _station_elevation );
}

static inline void calc_wind_hs( double north_fps, double east_fps, int & heading_deg, double & speed_kt )
{
    speed_kt = sqrt((north_fps)*(north_fps)+(east_fps)*(east_fps)) * 3600.0 / (SG_NM_TO_METER * SG_METER_TO_FEET);
    heading_deg = SGMiscd::roundToInt( 
        SGMiscd::normalizeAngle2( atan2( east_fps, north_fps ) ) * SGD_RADIANS_TO_DEGREES );
}

void MetarProperties::set_wind_from_north_fps( double value )
{
    _wind_from_north_fps = value;
    calc_wind_hs( _wind_from_north_fps, _wind_from_east_fps, _base_wind_dir, _wind_speed );
}

void MetarProperties::set_wind_from_east_fps( double value )
{
    _wind_from_east_fps = value;
    calc_wind_hs( _wind_from_north_fps, _wind_from_east_fps, _base_wind_dir, _wind_speed );
}

static inline void calc_wind_ne( double heading_deg, double speed_kt, double & north_fps, double & east_fps )
{
    double speed_fps = speed_kt * SG_NM_TO_METER * SG_METER_TO_FEET / 3600.0;
    north_fps = speed_fps * cos(heading_deg * SGD_DEGREES_TO_RADIANS);
    east_fps = speed_fps * sin(heading_deg * SGD_DEGREES_TO_RADIANS);
}

void MetarProperties::set_base_wind_dir( double value )
{
    _base_wind_dir = value;
    calc_wind_ne( (double)_base_wind_dir, _wind_speed, _wind_from_north_fps, _wind_from_east_fps );
}

void MetarProperties::set_wind_speed( double value )
{
    _wind_speed = value;
    calc_wind_ne( (double)_base_wind_dir, _wind_speed, _wind_from_north_fps, _wind_from_east_fps );
}


} // namespace Environment
