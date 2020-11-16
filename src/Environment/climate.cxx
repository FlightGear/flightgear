// Köppen-Geiger climate interface class
//
// Written by Erik Hofman, started October 2020
//
// Copyright (C) 2020 by Erik Hofman <erik@ehofman.com>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License along
//  with this program; if not, write to the Free Software Foundation, Inc.,
//  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
//
// $Id$

#include <osgDB/ReadFile>

#include <simgear/misc/sg_path.hxx>
#include <simgear/math/SGVec3.hxx>
#include <simgear/math/SGVec4.hxx>
#include <simgear/timing/sg_time.hxx>
#include <simgear/constants.h>

#include "Main/fg_props.hxx"
#include "Time/light.hxx"

#include "climate.hxx"

// Based on "World Map of the Köppen-Geiger climate classification"
// The map is provided with a spatial resolution of approx. 10x10km per pixel.
// http://koeppen-geiger.vu-wien.ac.at/present.htm
//
// References:
// * Teuling, A. J.: Technical note: Towards a continuous classification of
//   climate usingbivariate colour mapping, Hydrol. Earth Syst. Sci., 15,
//   3071–3075, https://doi.org/10.5194/hess-15-3071-2011, 2011.
//
// * Lawrence, Mark G., 2005: The relationship between relative humidity and
//   the dewpoint temperature in moist air: A simple conversion and
//   applications. Bull. Amer. Meteor. Soc., 86, 225-233.
//   doi: http://dx.doi.org/10.1175/BAMS-86-2-225

#define HOUR	(0.5/24.0)
#define MONTH	(1.0/12.0)

FGClimate::FGClimate()
{
    SGPath img_path = globals->get_fg_root() / "Geodata" / "koppen-geiger.png";

    image = osgDB::readImageFile(img_path.utf8Str());
    if (image)
    {
        _image_width = static_cast<double>( image->s() );
        _image_height = static_cast<double>( image->t() );
        _epsilon = 36.0/_image_width;
    }
}

void FGClimate::init()
{
    _monthNode = fgGetNode("/sim/time/utc/month", true);
    _metarSnowLevelNode = fgGetNode("/environment/params/metar-updates-snow-level", true);

    _positionLatitudeNode = fgGetNode("/position/latitude-deg", true);
    _positionLongitudeNode= fgGetNode("/position/longitude-deg", true);
    _ground_elev_node = fgGetNode("/position/ground-elev-ft", true );
}

void FGClimate::bind()
{
    _rootNode = fgGetNode("/environment/climate", true);
    _rootNode->getNode("description", true)->setStringValue(_description[_code]);
    _rootNode->getNode("classification", true)->setStringValue(_classification[_code]);

    _tiedProperties.setRoot( _rootNode );

    // environment properties
    _tiedProperties.Tie( "environment-update", &_environment_adjust );

    // weather properties
    _tiedProperties.Tie( "weather-update", &_weather_update);
    _tiedProperties.Tie( "relative-humidity", &_relative_humidity_gl);
    _tiedProperties.Tie( "relative-humidity-sea-level", &_relative_humidity_sl);
    _tiedProperties.Tie( "dewpoint-degc", &_dewpoint_gl);
    _tiedProperties.Tie( "dewpoint-sea-level-degc", &_dewpoint_sl);
    _tiedProperties.Tie( "temperature-degc", &_temperature_gl);
    _tiedProperties.Tie( "temperature-sea-level-degc", &_temperature_sl);
    _tiedProperties.Tie( "temperature-mean-degc", &_temperature_mean_gl);
    _tiedProperties.Tie( "temperature-mean-sea-level-degc", &_temperature_mean_sl);
    _tiedProperties.Tie( "temperature-seawater-degc", &_temperature_seawater);
    _tiedProperties.Tie( "precipitation-month-mm", &_precipitation);
    _tiedProperties.Tie( "precipitation-annual-mm", & _precipitation_annual);
    _tiedProperties.Tie( "snow-level-m", &_snow_level);
    _tiedProperties.Tie( "wind-kmph", &_wind);
}

void FGClimate::unbind ()
{
  _tiedProperties.Untie();
}

void FGClimate::reinit()
{
    _prev_lat = -99999.0;
    _prev_lon = -99999.0;

    _relative_humidity_sl = -99999.0;
    _relative_humidity_gl = -99999.0;

    _dewpoint_gl = -99999.0;
    _dewpoint_sl = -99999.0;
    _temperature_gl = -99999.0;
    _temperature_sl = -99999.0;
    _temperature_mean_gl = -99999.0;
    _temperature_mean_sl = -99999.0;
    _temperature_water = -99999.0;
    _temperature_seawater = -99999.0;
    _precipitation = -99999.0;
    _wind = -99999.0;
    _precipitation_annual = -99999.0;

    _is_autumn = -99999.0;
    _snow_level = -99999.0;
    _snow_thickness = -99999.0;
    _ice_cover = -99999.0;
    _dust_cover = -99999.0;
    _wetness = -99999.0;
    _lichen_cover = -99999.0;
}

// Set all environment parameters based on the koppen-classicfication
// https://en.wikipedia.org/wiki/K%C3%B6ppen_climate_classification
// http://vectormap.si.edu/Climate.htm
void FGClimate::update(double dt)
{
    FGLight *l = globals->get_subsystem<FGLight>();
    if (l)
    {
        _sun_longitude_deg = l->get_sun_lon()*SGD_RADIANS_TO_DEGREES;
        _sun_latitude_deg = l->get_sun_lat()*SGD_RADIANS_TO_DEGREES;
    }

    double latitude_deg  = _positionLatitudeNode->getDoubleValue();
    double longitude_deg = _positionLongitudeNode->getDoubleValue();

    _adj_latitude_deg = latitude_deg - _sun_latitude_deg;
    _adj_longitude_deg = _sun_longitude_deg - longitude_deg;
    if (_adj_longitude_deg < 0.0) _adj_longitude_deg += 360.0;
    else if (_adj_longitude_deg >= 360.0) _adj_longitude_deg -= 360.0;

    double diff_pos = fabs(_prev_lat - _adj_latitude_deg) +
                      fabs(_prev_lon - _adj_longitude_deg);
    if (diff_pos > _epsilon )
    {
        if (diff_pos > 1.0) reinit();

         double north = latitude_deg >= 0.0 ? 1.0 : -1.0; // hemisphere
         if (north) {
             _set(_is_autumn, (_monthNode->getIntValue() > 6) ? 1.0 : 0.0);
         } else {
             _set(_is_autumn, (_monthNode->getIntValue() <= 6) ? 1.0 : 0.0);
         }

        _prev_lat = _adj_latitude_deg;
        _prev_lon = _adj_longitude_deg;

        update_day_factor();
        update_season_factor();
        update_daylight();

        _code = 0; // Ocean
        osg::Vec4f color;
        if (image)
        {
            // from lat/lon to screen coordinates
            double x = 180.0 + longitude_deg;
            double y =  90.0 + latitude_deg;
            double xs = x * _image_width/360.0;
            double yt = y * _image_height/180.0;
            double rxs = round(xs);
            double ryt = round(yt);

            int s = static_cast<int>(rxs);
            int t = static_cast<int>(ryt);
            color = image->getColor(s, t);

            // convert from color shades to koppen-classicfication
            _code = static_cast<int>(255.0f*color[0]/4.0f);

            if (_code == 0) set_ocean();
            else if (_code < 5) set_tropical();
            else if (_code < 9) set_dry();
            else if (_code < 18) set_temperate();
            else if (_code < 30) set_continetal();
            else if (_code < 32) set_polar();
            else set_ocean();
        }

        _rootNode->getNode("description")->setStringValue(_description[_code]);
        _rootNode->getNode("classification")->setStringValue(_classification[_code]);

        double alt_km;
        alt_km = _ground_elev_node->getDoubleValue()*SG_FEET_TO_METER/1000.0;

        // Relative humidity decreases with an average of 4% per kilometer
        _relative_humidity_sl = std::min(_relative_humidity_gl + 4.0*alt_km, 100.0);

        // The temperature decreases by about 9.8°C per kilometer
        _temperature_sl = _temperature_gl + 9.8*alt_km;
        _temperature_mean_sl = _temperature_mean_gl + 9.8*alt_km;
        _temperature_seawater = _temperature_water + 9.8*alt_km;

        _set(_snow_level, 1000.0*_temperature_mean_sl/9.8);

        // Mark G. Lawrence:
        _dewpoint_sl = _temperature_sl - ((100.0 - _relative_humidity_sl)/5.0);
        _dewpoint_gl = _temperature_gl - ((100.0 - _relative_humidity_gl)/5.0);

        set_environment();

#if REPORT_TO_CONSOLE
        report();
#endif
    }
}

// https://en.wikipedia.org/wiki/Sunrise_equation
void FGClimate::update_daylight()
{
    double declination = _sun_latitude_deg*SGD_DEGREES_TO_RADIANS;
    double latitude_deg = _positionLatitudeNode->getDoubleValue();
    double latitude = latitude_deg*SGD_DEGREES_TO_RADIANS;

    double fact = -tan(latitude) * tan(declination);
    if (fact > 1.0) fact = 1.0;
    else if (fact < -1.0) fact = -1.0;

    _day_light = acos(fact)/SGD_PI;
}

// _day_noon returns 0.0 for night up to 1.0 for noon
void FGClimate::update_day_factor()
{
    // noon is when lon == 180.0
    _day_noon = fabs(_adj_longitude_deg - 180.0)/180.0;

    double adj_lon = _adj_longitude_deg;
    if (adj_lon >= 180.0) adj_lon -= 360.0;
    _daytime = 1.0 - (adj_lon + 180.0)/360.0;
}

// _season_summer returns 0.0 for winter up to 1.0 for summer
// _season_transistional returns 0.0 for early autumn/late spring up to
//                               1.0 for late autumn/early spring
//                        to distinguish between the two use the _autumn flag.
void FGClimate::update_season_factor()
{
    double latitude_deg = _positionLatitudeNode->getDoubleValue();
    double sign = latitude_deg >= 0.0 ? 1.0 : -1.0; // hemisphere

    _season_summer = (23.5 + sign*_sun_latitude_deg)/(2.0*23.5);

    _seasons_year = (_is_autumn > 0.05) ? 1.0-0.5*_season_summer :  0.5*_season_summer;

    _season_transistional = 2.0*(1.0 - _season_summer) - 1.0;
    if (_season_transistional < 0.0) _season_transistional = 0.0;
    else if (_season_transistional > 1.0) _season_transistional = 1.0;
}


void FGClimate::set_ocean()
{
    double day = _day_noon;
    double summer = _season_summer;

    // temperature based on latitude, season and time of day
    // the equator
    double temp_equator_night = triangular(season(summer, MONTH), 17.5, 22.5);
    double temp_equator_day = triangular(season(summer, MONTH), 27.5, 32.5);
    double temp_equator_mean = linear(_day_light, temp_equator_night, temp_equator_day);
    double temp_equator = linear(daytime(day, 3.0*HOUR), temp_equator_night, temp_equator_day);
    double temp_sw_Am = triangular(season(summer, 2.0*MONTH), 22.0, 27.5);

    // the poles
    double temp_pole_night = sinusoidal(season(summer, MONTH), -30.0, 0.0);
    double temp_pole_day = sinusoidal(season(summer, 1.5*MONTH), -22.5, 4.0);
    double temp_pole_mean = linear(_day_light, temp_pole_night, temp_pole_day);
    double temp_pole = linear(daytime(day, 3.0*HOUR), temp_pole_night, temp_pole_day);
    double temp_sw_ET = long_low(season(summer, 2.0*MONTH), -27.5, -3.5);

    // interpolate based on the viewers latitude
    double latitude_deg = _positionLatitudeNode->getDoubleValue();
    double fact_lat = pow(fabs(latitude_deg)/90.0, 2.5);
    double ifact_lat = 1.0 - fact_lat;

    _set(_temperature_gl, linear(ifact_lat, temp_pole, temp_equator));
    _set(_temperature_mean_gl, linear(ifact_lat, temp_pole_mean, temp_equator_mean));
    _set(_temperature_water, linear(ifact_lat, temp_sw_ET, temp_sw_Am));

    // high relative humidity around the equator and poles
    _set(_relative_humidity_gl, triangular(fabs(fact_lat-0.5), 70.0, 87.0));

    _set(_precipitation_annual, 990.0); // global average
    _set(_precipitation, 100.0 - (_precipitation_annual/25.0));

    _has_autumn = false;
    _set(_wind, 3.0);
}

// https://en.wikipedia.org/wiki/Tropical_rainforest_climate
// https://en.wikipedia.org/wiki/Tropical_monsoon_climate
void FGClimate::set_tropical()
{
    double day = _day_noon;

    double summer = _season_summer;
    double winter = -summer;

    double hum_fact = linear(daytime(day, 3.0*HOUR), sinusoidal(summer, 0.0, 0.36),
                                  sinusoidal(season(winter), 0.86, 1.0));

    // wind based on latitude (0.0 - 15 degrees)
    double latitude_deg = _positionLatitudeNode->getDoubleValue();
    double fact_lat = std::max(abs(latitude_deg), 15.0)/15.0;
    double wind = 3.0*fact_lat*fact_lat;

    double temp_water = _temperature_water;
    double temp_night = _temperature_sl;
    double temp_day = _temperature_sl;
    double precipitation = _precipitation;
    double relative_humidity = _relative_humidity_gl;
    switch(_code)
    {
    case 1: // Af: equatorial, fully humid
        temp_night = triangular(summer, 20.0, 22.5);
        temp_day = triangular(summer, 29.5, 32.5);
        temp_water = triangular(season(summer, MONTH), 25.0, 27.5);
        precipitation = sinusoidal(season(winter), 150.0, 280.0);
        relative_humidity = triangular(hum_fact, 75.0, 85.0);
        break;
    case 2: // Am: equatorial, monsoonal
        temp_night = triangular(season(summer, MONTH), 17.5, 22.5);
        temp_day = triangular(season(summer, MONTH), 27.5, 32.5);
        temp_water = triangular(season(summer, MONTH), 22.0, 27.5);
        precipitation = linear(season(summer, MONTH), 45.0, 340.0);
        relative_humidity = triangular(season(hum_fact, MONTH), 75.0, 85.0);
        wind *= 2.0*_precipitation/320.0;
        break;
    case 3: // As: equatorial, summer dry
        temp_night = long_high(season(summer, .15*MONTH), 15.0, 22.5);
        temp_day = triangular(season(summer, MONTH), 27.5, 35.0);
        temp_water = triangular(season(summer, 2.0*MONTH), 21.5, 26.5);
        precipitation = sinusoidal(season(summer, 2.0*MONTH), 35.0, 150.0);
        relative_humidity = triangular(season(hum_fact, 2.0*MONTH), 60.0, 80.0);
        wind *= 2.0*_precipitation/350.0;
        break;
    case 4: // Aw: equatorial, winter dry
        temp_night = long_high(season(summer, 1.5*MONTH), 15.0, 22.5);
        temp_day = triangular(season(summer, 2.0*MONTH), 27.5, 35.0);
        temp_water = triangular(season(summer, 2.0*MONTH), 21.5, 28.5);
        precipitation = sinusoidal(season(summer, 2.0*MONTH), 10.0, 230.0);
        relative_humidity = triangular(season(hum_fact, 2.0*MONTH), 60.0, 80.0);
        wind *= 2.0*_precipitation/230.0;
        break;
    default:
        break;
    }

    _set(_temperature_gl, linear(daytime(day, 3.0*HOUR), temp_night, temp_day));
    _set(_temperature_mean_gl, linear(_day_light, temp_night, temp_day));
    _set(_temperature_water, temp_water);

    _set(_relative_humidity_gl, relative_humidity);

    _set(_precipitation_annual, 3000.0);
    _set(_precipitation, precipitation);

    _has_autumn = false;
    _set(_wind, wind);
}

// https://en.wikipedia.org/wiki/Desert_climate
// https://en.wikipedia.org/wiki/Semi-arid_climate
void FGClimate::set_dry()
{
    double day = _day_noon;

    double summer = _season_summer;
    double winter = -summer;

    double hum_fact = linear(daytime(day, 3.0*HOUR), sinusoidal(summer, 0.0, 0.36),
                                  sinusoidal(season(winter), 0.86, 1.0));

    double temp_water = _temperature_water;
    double temp_night = _temperature_sl;
    double temp_day = _temperature_sl;
    double precipitation = _precipitation;
    double relative_humidity = _relative_humidity_gl;
    switch(_code)
    {
    case 5: // BSh: arid, steppe, hot arid
        temp_night = long_high(season(summer, MONTH), 10.0, 22.0);
        temp_day = triangular(season(summer, 2.0*MONTH), 27.5, 35.0);
        temp_water = triangular(season(summer, 2.5*MONTH), 18.5, 28.5);
        precipitation = long_low(season(summer, 2.0*MONTH), 8.0, 117.0);
        relative_humidity = triangular(season(hum_fact, 2.0*MONTH), 20.0, 30.0);
        break;
    case 6: // BSk: arid, steppe, cold arid
        temp_night = sinusoidal(season(summer, MONTH), -14.0, 12.0);
        temp_day = sinusoidal(season(summer, MONTH), 0.0, 30.0);
        temp_water = sinusoidal(season(summer, 2.0*MONTH), 5.0, 25.5);
        precipitation = sinusoidal(season(summer, MONTH), 15.0, 34.0);
        relative_humidity = sinusoidal(season(hum_fact, MONTH), 48.0, 67.0);
        break;
    case 7: // BWh: arid, desert, hot arid
        temp_night = sinusoidal(season(summer, 1.5*MONTH), 7.5, 22.0);
        temp_day = even(season(summer, 1.5*MONTH), 22.5, 37.5);
        temp_water = even(season(summer, 2.5*MONTH), 15.5, 33.5);
        precipitation = monsoonal(season(summer, 2.0*MONTH), 3.0, 18.0);
        relative_humidity = monsoonal(season(hum_fact, 2.0*MONTH), 25.0, 55.0);
        break;
    case 8: // BWk: arid, desert, cold arid
        temp_night = sinusoidal(season(summer, MONTH), -15.0, 15.0);
        temp_day = sinusoidal(season(summer, MONTH), -2.0, 30.0);
        temp_water = sinusoidal(season(summer, 2.0*MONTH), 4.0, 26.5);
        precipitation = linear(season(summer, MONTH), 4.0, 14.0);
        relative_humidity = linear(season(hum_fact, MONTH), 45.0, 61.0);
        break;
    default:
        break;
    }

    _set(_temperature_gl, linear(daytime(day, 3.0*HOUR), temp_night, temp_day));
    _set(_temperature_mean_gl, linear(_day_light, temp_night, temp_day));
    _set(_temperature_water, temp_water);

    _set(_relative_humidity_gl, relative_humidity);

    if (_code == 5 || _code == 6) {     // steppe
        _set(_precipitation_annual, 200.0);
    } else {
        _set(_precipitation_annual, 100.0);
    }
    _set(_precipitation, precipitation);
    _has_autumn = false;
    _set(_wind, 3.0);
}

// https://en.wikipedia.org/wiki/Temperate_climate
void FGClimate::set_temperate()
{
    double day = _day_noon;

    double summer = _season_summer;
    double winter = -summer;

    double hum_fact = linear(daytime(day, 3.0*HOUR), sinusoidal(summer, 0.0, 0.36),
                                  sinusoidal(season(winter), 0.86, 1.0));

    double temp_water = _temperature_water;
    double temp_night = _temperature_sl;
    double temp_day = _temperature_sl;
    double precipitation = _precipitation;
    double relative_humidity = _relative_humidity_gl;
    switch(_code)
    {
    case 9: // Cfa: warm temperature, fully humid hot summer
        temp_night = sinusoidal(season(summer, 1.5*MONTH), -3.0, 20.0);
        temp_day = sinusoidal(season(summer, 1.5*MONTH), 10.0, 33.0);
        temp_water = sinusoidal(season(summer, 2.5*MONTH), 8.0, 28.5);
        precipitation = sinusoidal(summer, 60.0, 140.0);
        relative_humidity = sinusoidal(hum_fact, 65.0, 80.0);
        break;
    case 10: // Cfb: warm temperature, fully humid, warm summer
        temp_night = sinusoidal(season(summer, 1.5*MONTH), -3.0, 10.0);
        temp_day = sinusoidal(season(summer, 1.5*MONTH), 5.0, 25.0);
        temp_water = sinusoidal(season(summer, 2.5*MONTH), 3.0, 20.5);
        precipitation = sinusoidal(season(winter, 3.5*MONTH), 65.0, 90.0);
        relative_humidity = sinusoidal(season(hum_fact, 1.5*MONTH), 68.0, 87.0);
        break;
    case 11: // Cfc: warm temperature, fully humid, cool summer
        temp_night = long_low(season(summer, 1.5*MONTH), -3.0, 8.0);
        temp_day = long_low(season(summer, 1.5*MONTH), 2.0, 14.0);
        temp_water = long_low(season(summer, 2.5*MONTH), 3.0, 11.5);
        precipitation = linear(season(winter), 90.0, 200.0);
        relative_humidity = long_low(season(hum_fact, 1.5*MONTH), 70.0, 85.0);
        break;
    case 12: // Csa: warm temperature, summer dry, hot summer
        temp_night = sinusoidal(season(summer, MONTH), 2.0, 16.0);
        temp_day = sinusoidal(season(summer, MONTH), 12.0, 33.0);
        temp_water = sinusoidal(season(summer, 2.0*MONTH), 10.0, 27.5);
        precipitation = linear(season(winter), 25.0, 70.0);
        relative_humidity = sinusoidal(season(hum_fact, MONTH), 58.0, 72.0);
        break;
    case 13: // Csb: warm temperature, summer dry, warm summer
        temp_night = linear(season(summer, 1.5*MONTH), -4.0, 10.0);
        temp_day = linear(season(summer, 1.5*MONTH), 6.0, 27.0);
        temp_water = linear(season(summer, 2.5*MONTH), 4.0, 21.5);
        precipitation = linear(season(winter), 25.0, 120.0);
        relative_humidity = linear(season(hum_fact, 1.5*MONTH), 50.0, 72.0);
        break;
    case 14: // Csc: warm temperature, summer dry, cool summer
        temp_night = sinusoidal(season(summer, 0.5*MONTH), -4.0, 5.0);
        temp_day = sinusoidal(season(summer, 0.5*MONTH), 5.0, 16.0);
        temp_water = sinusoidal(season(summer, 1.5*MONTH), 3.0, 14.5);
        precipitation = sinusoidal(season(winter, -MONTH), 60.0, 95.0);
        relative_humidity = sinusoidal(hum_fact, 55.0, 75.0);
        break;
    case 15: // Cwa: warm temperature, winter dry, hot summer
        temp_night = even(season(summer, MONTH), 4.0, 20.0);
        temp_day = long_low(season(summer, MONTH), 15.0, 30.0);
        temp_water = long_low(season(summer, 2.0*MONTH), 7.0, 24.5);
        precipitation = long_low(season(summer, MONTH), 10.0, 320.0);
        relative_humidity = sinusoidal(season(hum_fact, MONTH), 60.0, 79.0);
        break;
    case 16: // Cwb: warm temperature, winter dry, warm summer
        temp_night = even(season(summer, MONTH), 1.0, 13.0);
        temp_day = long_low(season(summer, MONTH), 15.0, 27.0);
        temp_water = even(season(summer, 2.0*MONTH), 5.0, 22.5);
        precipitation = long_low(season(summer, MONTH), 10.0, 250.0);
        relative_humidity = sinusoidal(season(hum_fact, MONTH), 58.0, 72.0);
        break;
    case 17: // Cwc: warm temperature, winter dry, cool summer
        temp_night = long_low(season(summer, MONTH), -9.0, 6.0);
        temp_day = long_high(season(summer, MONTH), 6.0, 17.0);
        temp_water = long_high(season(summer, 2.0*MONTH), 8.0, 15.5);
        precipitation = long_low(season(summer, MONTH), 5.0, 200.0);
        relative_humidity = long_high(season(hum_fact, MONTH), 50.0, 58.0);
        break;
    default:
        break;
    }

    _set(_temperature_gl, linear(daytime(day, 3.0*HOUR), temp_night, temp_day));
    _set(_temperature_mean_gl, linear(_day_light, temp_night, temp_day));
    _set(_temperature_water, temp_water);

    _set(_relative_humidity_gl, relative_humidity);

    _set(_precipitation_annual, 990.0);
    _set(_precipitation, precipitation);

    _has_autumn = true;
    _set(_wind, 3.0);

}

// https://en.wikipedia.org/wiki/Continental_climate
void FGClimate::set_continetal()
{
    double day = _day_noon;

    double summer = _season_summer;
    double winter = -summer;

    double hum_fact = linear(daytime(day, 3.0*HOUR), sinusoidal(summer, 0.0, 0.36),
                                  sinusoidal(season(winter), 0.86, 1.0));

    double temp_water = _temperature_water;
    double temp_day = _temperature_sl;
    double temp_night = _temperature_sl;
    double precipitation = _precipitation;
    double relative_humidity = _relative_humidity_gl;
    switch(_code)
    {
    case 18: // Dfa: snow, fully humid, hot summer
        temp_night = sinusoidal(season(summer, MONTH), -15.0, 13.0);
        temp_day = sinusoidal(season(summer, MONTH), -5.0, 30.0);
        temp_water = sinusoidal(season(summer, 2.0*MONTH), 0.0, 26.5);
        precipitation = linear(season(summer, MONTH), 30.0, 70.0);
        relative_humidity = sinusoidal(hum_fact, 68.0, 72.0);
        break;
    case 19: // Dfb: snow, fully humid, warm summer, warm summer
        temp_night = sinusoidal(season(summer, MONTH), -17.5, 10.0);
        temp_day = sinusoidal(season(summer, MONTH), -7.5, 25.0);
        temp_water = sinusoidal(season(summer, 2.0*MONTH), -2.0, 22.5);
        precipitation = linear(season(summer, MONTH), 30.0, 70.0);
        relative_humidity = sinusoidal(season(hum_fact, MONTH), 69.0, 81.0);
        break;
    case 20: // Dfc: snow, fully humid, cool summer, cool summer
        temp_night = sinusoidal(season(summer, MONTH), -30.0, 4.0);
        temp_day = sinusoidal(season(summer, MONTH), -20.0, 15.0);
        temp_water = sinusoidal(season(summer, 2.0*MONTH), -10.0, 12.5);
        precipitation = linear(season(summer, 1.5*MONTH), 22.0, 68.0);
        relative_humidity = sinusoidal(season(hum_fact, MONTH), 70.0, 88.0);
        break;
    case 21: // Dfd: snow, fully humid, extremely continetal
        temp_night = sinusoidal(season(summer, MONTH), -45.0, 4.0);
        temp_day = sinusoidal(season(summer, MONTH), -35.0, 10.0);
        temp_water = sinusoidal(season(summer, 2.0*MONTH), -15.0, 8.5);
        precipitation = long_low(season(summer, 1.5*MONTH), 7.5, 45.0);
        relative_humidity = sinusoidal(season(hum_fact, MONTH), 80.0, 90.0);
        break;
    case 22: // Dsa: snow, summer dry, hot summer
        temp_night = sinusoidal(season(summer, 1.5*MONTH), -10.0, 10.0);
        temp_day = sinusoidal(season(summer, 1.5*MONTH), 0.0, 30.0);
        temp_water = sinusoidal(season(summer, 3.5*MONTH), 4.0, 24.5);
        precipitation = long_high(season(winter, 2.0*MONTH), 5.0, 65.0);
        relative_humidity = sinusoidal(season(hum_fact, 1.5*MONTH), 48.0, 58.08);
        break;
    case 23: // Dsb: snow, summer dry, warm summer
        temp_night = sinusoidal(season(summer, 1.5*MONTH), -15.0, 6.0);
        temp_day = sinusoidal(season(summer, 1.5*MONTH), -4.0, 25.0);
        temp_water = sinusoidal(season(summer, 2.5*MONTH), 0.0, 19.5);
        precipitation = long_high(season(winter, 2.0*MONTH), 12.0, 65.0);
        relative_humidity = sinusoidal(season(hum_fact, 1.5*MONTH), 50.0, 68.0);
        break;
    case 24: // Dsc: snow, summer dry, cool summer
        temp_night = sinusoidal(season(summer, MONTH), -27.5, 2.0);
        temp_day = sinusoidal(season(summer, MONTH), -4.0, 15.0);
        temp_water = sinusoidal(season(summer, 2.0*MONTH), 0.0, 12.5);
        precipitation = long_low(season(summer, MONTH), 32.5, 45.0);
        relative_humidity = sinusoidal(season(hum_fact, MONTH), 50.0, 60.0);
        break;
    case 25: // Dsd: snow, summer dry, extremely continetal
        temp_night = sinusoidal(season(summer, MONTH), -11.5, -6.5);
        temp_day = sinusoidal(season(summer, MONTH), 14.0, 27.0);
        temp_water = sinusoidal(season(summer, 2.0*MONTH), 8.0, 20.5);
        precipitation = long_low(season(summer, MONTH), 5.0, 90.0);
        relative_humidity = sinusoidal(season(hum_fact, MONTH), 48.0, 62.0);
        break;
    case 26: // Dwa: snow, winter dry, hot summer
        temp_night = sinusoidal(season(summer, MONTH), -18.0, 16.5);
        temp_day = sinusoidal(season(summer, MONTH), -5.0, 25.0);
        temp_water = sinusoidal(season(summer, 2.0*MONTH), 0.0, 23.5);
        precipitation = long_low(season(summer, 1.5*MONTH), 5.0, 180.0);
        relative_humidity = sinusoidal(season(hum_fact, MONTH), 60.0, 68.0);
        break;
    case 27: // Dwb: snow, winter dry, warm summer
        temp_night = sinusoidal(season(summer, MONTH), -28.0, 10.0);
        temp_day = sinusoidal(season(summer, MONTH), -12.5, 22.5);
        temp_water = sinusoidal(season(summer, 2.0*MONTH), -5.0, 18.5);
        precipitation = long_low(season(summer, 1.5*MONTH), 10.0, 140.0);
        relative_humidity = sinusoidal(season(hum_fact, MONTH), 60.0, 72.0);
        break;
    case 28: // Dwc: snow, winter dry, cool summer
        temp_night = sinusoidal(season(summer, MONTH), -33.0, 5.0);
        temp_day = sinusoidal(season(summer, MONTH), -20.0, 20.0);
        temp_water = sinusoidal(season(summer, 2.0*MONTH), -10.0, 16.5);
        precipitation = long_low(season(summer, 1.5*MONTH), 10.0, 110.0);
        relative_humidity = sinusoidal(season(hum_fact, MONTH), 60.0, 78.0);
        break;
    case 29: // Dwd: snow, winter dry, extremely continetal
        temp_night = sinusoidal(season(summer, MONTH), -57.5, 0.0);
        temp_day = sinusoidal(season(summer, MONTH), -43.0, 15.0);
        temp_water = sinusoidal(season(summer, 2.0*MONTH), -28.0, 11.5);
        precipitation = sinusoidal(season(summer, 1.5*MONTH), 8.0, 63.0);
        relative_humidity = 80.0;
        break;
    default:
        break;
    }

    _set(_temperature_gl, linear(daytime(day, 3.0*HOUR), temp_night, temp_day));
    _set(_temperature_mean_gl, linear(_day_light, temp_night, temp_day));
    _set(_temperature_water, temp_water);

    _set(_relative_humidity_gl, relative_humidity);

    _set(_precipitation_annual, 990.0);
    _set(_precipitation, precipitation);

    _has_autumn = true;
    _set(_wind, 3.0);
}

void FGClimate::set_polar()
{
    double day = _day_noon;

    double summer = _season_summer;
    double winter = -summer;

    double hum_fact = linear(daytime(day, 3.0*HOUR),
                             sinusoidal(summer, 0.0, 0.36),
                             sinusoidal(season(winter), 0.86, 1.0));

    // polar climate also occurs high in the mountains
    double temp_water = _temperature_water;
    double temp_day = _temperature_sl;
    double temp_night = _temperature_sl;
    double precipitation = _precipitation;
    double relative_humidity = _relative_humidity_gl;
    switch(_code)
    {
    case 30: // EF: polar frost
        temp_night = long_low(season(summer, MONTH), -35.0, -6.0);
        temp_day = long_low(season(summer, MONTH), -32.5, 0.0);
        temp_water = long_low(season(summer, 2.0*MONTH), -27.5, -3.5);
        precipitation = linear(season(summer, 2.5*MONTH), 50.0, 80.0);
        relative_humidity = long_low(season(hum_fact, MONTH), 65.0, 75.0);
        break;
    case 31: // ET: polar tundra
        temp_night = sinusoidal(season(summer, MONTH), -30.0, 0.0);
        temp_day = sinusoidal(season(summer, 1.5*MONTH), -22.5, 8.0);
        temp_water = sinusoidal(season(summer, 2.0*MONTH), -15.0, 5.0);
        precipitation = sinusoidal(season(summer, 2.0*MONTH), 15.0, 45.0);
        relative_humidity = sinusoidal(season(hum_fact, MONTH), 60.0, 88.0);
        break;
    default:
        break;
    }

    _set(_temperature_gl, linear(daytime(day, 3.0*HOUR), temp_night, temp_day));
    _set(_temperature_mean_gl, linear(_day_light, temp_night, temp_day));
    _set(_temperature_water, temp_water);

    _set(_relative_humidity_gl, relative_humidity);

    _set(_precipitation_annual, 990.0);
    _set(_precipitation, precipitation);

    _has_autumn = true;
    _set(_wind, 3.0);
}

void FGClimate::set_environment()
{
    double snow_fact, precipitation;

    // snow chance based on latitude, mean temperature and monthly precipitation
    if (_precipitation < 60.0) {
        precipitation = 0.0;
    }
    else
    {
        precipitation = _precipitation - 60.0;
        precipitation = (precipitation > 240.0) ? 1.0 : precipitation/240.0;
    }

    if (_temperature_mean_gl > 5.0 || precipitation < 0.1) {
        snow_fact = 0.0;
    }
    else
    {
        snow_fact = fabs(_temperature_mean_gl) - 5.0;
        snow_fact = (snow_fact > 10.0) ? 1.0 : snow_fact/10.0;
        snow_fact *= precipitation;
    }

    // Sea water will start to freeze at -2° water temperaure.
    if (_is_autumn < 0.95 || _temperature_mean_sl > -2.0) {
        _set(_ice_cover, 0.0);
    }
    else {
        _set(_ice_cover, std::min(-(_temperature_seawater+2.0)/10.0, 1.0));
    }

    // less than 20 mm/month of precipitation is considered dry.
    if (_precipitation < 20.0 || _precipitation_annual < 240.0)
    {
        _set(_dust_cover, 0.3 - 0.3*sqrtf(_precipitation/20.0));
        _set(_snow_thickness, 0.0);
        _set(_lichen_cover, 0.0);
        _set(_wetness, 0.0);
    }
    else
    {
        double wetness = std::max(_precipitation - 20.0, 0.0);
        wetness = powf(sin(atan(12.0*wetness/990.0)), 2.0);

        _set(_dust_cover, 0.0);

        if (_temperature_mean_gl < -5.0) {
            _set(_snow_thickness, wetness);
        } else {
            _set(_snow_thickness, 0.0);
        }

        if (_temperature_mean_gl > 0.0) {
            _set(_wetness, pow(wetness, 2.0));
        } else {
            _set(_wetness, 0.0);
        }

        double cover = std::min(_precipitation_annual, 990.0)/990.0;
        _set(_lichen_cover, 0.5*pow(wetness*cover, 1.5));
    }

    _rootNode->setDoubleValue("day-light-pct", _day_light*100.0);
    _rootNode->setDoubleValue("day-noon-pct", _day_noon*100.0);
    _rootNode->setDoubleValue("season-summer-pct", _season_summer*100.0);
    _rootNode->setDoubleValue("seasons-year-pct", _seasons_year*100.0);
    _rootNode->setDoubleValue("season-autumn-pct", ((_has_autumn && _is_autumn > 0.05) ? _season_transistional : 0.0));

    if (_weather_update)
    {
        fgSetDouble("/environment/relative-humidity", _relative_humidity_gl);
        fgSetDouble("/environment/dewpoint-sea-level-degc", _dewpoint_sl);
        fgSetDouble("/environment/dewpoint-degc", _dewpoint_gl);
        fgSetDouble("/environment/temperature-sea-level-degc", _temperature_sl);
        fgSetDouble("/environment/temperature-degc", _temperature_gl);
        fgSetDouble("/environment/wind-speed-kt", _wind*SG_KMH_TO_MPS*SG_MPS_TO_KT);
    }

    if (_environment_adjust)
    {
        if (!_metarSnowLevelNode->getBoolValue()) {
            fgSetDouble("/environment/snow-level-m", _snow_level);
        }
        fgSetDouble("/environment/surface/snow-thickness-factor", _snow_thickness);
        fgSetDouble("/environment/sea/surface/ice-cover", _ice_cover);
        fgSetDouble("/environment/surface/dust-cover-factor", _dust_cover);
        fgSetDouble("/environment/surface/wetness-set", _wetness);
        fgSetDouble("/environment/surface/lichen-cover-factor", _lichen_cover);
        if (_has_autumn && _is_autumn > 0.01)
            fgSetDouble("/environment/season", std::min(3.0*_is_autumn*_season_transistional, 2.0));
        else
            fgSetDouble("/environment/season", 0.0);
    }
}

// ---------------------------------------------------------------------------

const std::string FGClimate::_classification[MAX_CLIMATE_CLASSES] = {
    "Ocean",
    "Af", "Am", "As", "Aw",
    "BSh", "BSk", "BWh", "BWk",
    "Cfa", "Cfb", "Cfc",
    "Csa", "Csb", "Csc",
    "Cwa", "Cwb", "Cwc",
    "Dfa", "Dfb", "Dfc", "Dfd",
    "Dsa", "Dsb", "Dsc", "Dsd",
    "Dwa", "Dwb", "Dwc", "Dwd",
    "EF", "ET"
};

const std::string FGClimate::_description[MAX_CLIMATE_CLASSES] = {
    "Ocean",
    "Equatorial, fully humid",
    "Equatorial, monsoonal",
    "Equatorial, summer dry",
    "Equatorial, winter dry",
    "Arid, steppe, hot arid",
    "Arid, steppe, cold arid",
    "Arid, desert, hot arid",
    "Arid, desert, cold arid",
    "Warm temperature, fully humid hot summer",
    "Warm temperature, fully humid, warm summer",
    "Warm temperature, fully humid, cool summer",
    "Warm temperature, summer dry, hot summer",
    "Warm temperature, summer dry, warm summer",
    "Warm temperature, summer dry, cool summer",
    "Warm temperature, winter dry, hot summer",
    "Warm temperature, winter dry, warm summer",
    "Warm temperature, winter dry, cool summer",
    "Snow, fully humid, hot summer",
    "Snow, fully humid, warm summer",
    "Snow, fully humid, cool summer",
    "Snow, fully humid, extremely continetal",
    "Snow, summer dry, hot summer",
    "Snow, summer dry, warm summer",
    "Snow, summer dry, cool summer",
    "Snow, summer dry, extremely continetal",
    "Snow, winter dry, hot summer",
    "Snow, winter dry, warm summer",
    "Snow, winter dry, cool summer",
    "Snow, winter dry, extremely continetal",
    "Polar frost",
    "Polar tundra"
};


// interpolators
double FGClimate::daytime(double val, double offset)
{
    val = fmod(fabs(_daytime - offset), 1.0);
    val = (val > 0.5)  ? 2.0 - 2.0*val : 2.0*val;
    return val;
}

double FGClimate::season(double val, double offset)
{
    val = (val >= 0.0) ? fmod(fabs(_seasons_year - offset), 1.0)
                       : fmod(fabs(1.0 - _seasons_year + offset), 1.0);
    val = (val > 0.5)  ? 2.0 - 2.0*val : 2.0*val;
    return val;
}

// val is the progress indicator between 0.0 and 1.0
double FGClimate::linear(double val, double min, double max)
{
    double diff = max-min;
    return min + val*diff;
}

// google: y=1-abs(-1+2*x)
// low season is around 0.0 and 1.0, high season around 0.5
double FGClimate::triangular(double val, double min, double max)
{
    double diff = max-min;
    val = 1.0 - fabs(-1.0 + 2.0*val);
    return min + val*diff;
}


// google: y=0.5-0.5*cos(x)
// the short low season is round 0.0, the short high season around 1.0
double FGClimate::sinusoidal(double val, double min, double max)
{
    double diff = max-min;
    return min + diff*(0.5 - 0.5*cos(SGD_PI*val));
}

// google: y=0.5-0.6366*atan(cos(x))
// the medium long low season is round 0.0, medium long high season round 1.0
double FGClimate::even(double val, double min, double max)
{
    double diff = max-min;
    return min + diff*(0.5 - 0.6366*atan(cos(SGD_PI*val)));
}

// google: y=0.5-0.5*cos(x^1.5)
// 2.145 = pow(SGD_PI, 1.0/1.5)
// the long low season is around 0.0, the short high season around 1.0
double FGClimate::long_low(double val, double min, double max)
{
    double diff = max-min;
    return min + diff*(0.5 - 0.5*cos(pow(2.145*val, 1.5)));
}

// google: y=0.5+0.5*cos(x^1.5)
// 2.14503 = pow(SGD_PI, 1.0/1.5)
// the long high season is around 0.0, the short low season around 1.0
double FGClimate::long_high(double val, double min, double max)
{
    double diff = max-min;
    return max - diff*(0.5 - 0.5*cos(pow(2.14503 - 2.14503*val, 1.5)));
}

// goole: y=cos(atan(x*x))
// the monsoon is around 0.0
double FGClimate::monsoonal(double val, double min, double max)
{
    double diff = max-min;
    val = 2.0*SGD_2PI*(1.0-val);
    return min + diff*cos(atan(val*val));
}

void FGClimate::test()
{
    double offs = 0.25;

    printf("month:\t\t");
    for (int month=0; month<12; ++month) {
        printf("%5i", month+1);
    }

    printf("\nlinear:\t\t");
    for (int month=0; month<12; ++month) {
        double val = (month > 6) ? (12 - month)/6.0 : month/6.0;
        _seasons_year = month/12.0;
        printf(" %-3.2f", linear(season(val, offs), 0.0, 1.0) );
    }

    printf("\ntriangular:\t");
    for (int month=0; month<12; ++month) {
        double val = (month > 6) ? (12 - month)/6.0 : month/6.0;
        _seasons_year = month/12.0;
        printf(" %-3.2f", triangular(season(val, offs), 0.0, 1.0) );
    }

    printf("\neven:\t\t");
    for (int month=0; month<12; ++month) {
        double val = (month > 6) ? (12 - month)/6.0 : month/6.0;
        _seasons_year = month/12.0;
        printf(" %-3.2f", sinusoidal(season(val, offs), 0.0, 1.0) );
    }

    printf("\nlong:\t\t");
    for (int month=0; month<12; ++month) {
        double val = (month > 6) ? (12 - month)/6.0 : month/6.0;
        _seasons_year = month/12.0;
        printf(" %-3.2f", even(season(val, offs), 0.0, 1.0) );
    }

    printf("\nlong low:\t");
    for (int month=0; month<12; ++month) {
        double val = (month > 6) ? (12 - month)/6.0 : month/6.0;
        _seasons_year = month/12.0;
        printf(" %-3.2f", long_low(season(val, offs), 0.0, 1.0) );
    }

    printf("\nlong high:\t");
    for (int month=0; month<12; ++month) {
        double val = (month > 6) ? (12 - month)/6.0 : month/6.0;
        _seasons_year = month/12.0;
        printf(" %-3.2f", long_high(season(val, offs), 0.0, 1.0) );
    }

    printf("\nmonsoonal:\t");
    for (int month=0; month<12; ++month) {
        double val = (month > 6) ? (12 - month)/6.0 : month/6.0;
        _seasons_year = month/12.0;
        printf(" %-3.2f", monsoonal(season(val, offs), 0.0, 1.0) );
    }
    printf("\n");
}

#if REPORT_TO_CONSOLE
void FGClimate::report()
{
    struct tm *t = globals->get_time_params()->getGmt();

    std::cout << "===============================================" << std::endl;
    std::cout << "Climate report for:" << std::endl;
    std::cout << "  Date: " << sgTimeFormatTime(t) << " GMT" << std::endl;
    std::cout << "  Sun latitude:    " << _sun_latitude_deg << " deg."
              << std::endl;
    std::cout << "  Sun longitude:   " << _sun_longitude_deg << " deg."
              << std::endl;
    std::cout << "  Viewer latitude:  "
              << _positionLatitudeNode->getDoubleValue() << " deg."
              << " (adjusted: " << _adj_latitude_deg << ")" << std::endl;
    std::cout << "  Viewer longitude: "
              << _positionLongitudeNode->getDoubleValue() << " deg."
              << " (adjusted: " << _adj_longitude_deg << ")" << std::endl;
    std::cout << std::endl;
    std::cout << "  Köppen classification: " << _classification[_code]
              << std::endl;
    std::cout << "  Description: " << _description[_code]
              << std::endl << std::endl;
    std::cout << "  Daylight: " << _day_light*100.0 << " %"
              << std::endl;
    std::cout << "  Daytime:  "<< _daytime*24.0 << " hours" << std::endl;
    std::cout << "  Daytime...(0.0 = night .. 1.0 = noon): " << _day_noon
              << std::endl;
    std::cout << "  Season (0.0 = winter .. 1.0 = summer): " << _season_summer
              << std::endl;
    std::cout << "  Year (0.25 = spring .. 0.75 = autumn): " << _seasons_year
              << std::endl << std::endl;
    std::cout << "  Ground temperature:    " << _temperature_gl << " deg. C."
              << std::endl;
    std::cout << "  Sea level temperature: " << _temperature_sl << " deg. C."
              << std::endl;
    std::cout << "  Ground Mean temp.:     " << _temperature_mean_gl
              << " deg. C." << std::endl;
    std::cout << "  Sea level mean temp.:   " << _temperature_mean_sl
              << " deg. C." << std::endl;
    std::cout << "  Seawater temperature: " << _temperature_seawater
              << " deg. C." << std::endl;

    std::cout << "  Months precipitation:  " << _precipitation << " mm"
              << std::endl;
    std::cout << "  Annual precipitation:  " << _precipitation_annual << " mm"
              << std::endl;
    std::cout << "  Relative humidity:     " << _relative_humidity_gl << " %"
              << std::endl;
    std::cout << "  Dewpoint:              " << _dewpoint_gl << " deg. C."
              << std::endl;
    std::cout << "  Wind: " << _wind << " km/h" << std::endl << std::endl;
    std::cout << "  Snow level: " << _snow_level << " m." << std::endl;
    std::cout << "  Snow thickness.(0.0 = thin .. 1.0 = thick): "
              << _snow_thickness << std::endl;
    std::cout << "  Ice cover......(0.0 = none .. 1.0 = thick): " << _ice_cover
              << std::endl;
    std::cout << "  Dust cover.....(0.0 = none .. 1.0 = dusty): " << _dust_cover
              << std::endl;
    std::cout << "  Wetness........(0.0 = dry  .. 1.0 = wet):   " << _wetness
              << std::endl;
    std::cout << "  Lichen cover...(0.0 = none .. 1.0 = mossy): "
              << _lichen_cover << std::endl;
    std::cout << "  Season (0.0 = summer .. 1.0 = late autumn): "
              << ((_has_autumn && _is_autumn > 0.05) ? _season_transistional : 0.0)
              << std::endl;
    std::cout << "===============================================" << std::endl;
}
#endif // REPORT_TO_CONSOLE
