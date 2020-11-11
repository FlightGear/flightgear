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

#define MONTH	(1.0/6.0)

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
    _precipitation = -99999.0;
    _wind = -99999.0;
    _precipitation_annual = -99999.0;

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
    if (!l) return; // e.g. during unit-testing

    _sun_longitude_deg = l->get_sun_lon()*SGD_RADIANS_TO_DEGREES;
    _sun_latitude_deg = l->get_sun_lat()*SGD_RADIANS_TO_DEGREES;

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
             _is_autumn = (_monthNode->getIntValue() > 6) ? true : false;
         } else {
             _is_autumn = (_monthNode->getIntValue() <= 6) ? true : false;
         }

        _prev_lat = _adj_latitude_deg;
        _prev_lon = _adj_longitude_deg;

        update_day_factor();
        update_season_factor();

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

            int xoffs = (rxs < xs) ? -1 : 1;
            int yoffs = (ryt < yt) ? -1 : 1;

            int s = static_cast<int>(rxs + xoffs);
            int t = static_cast<int>(ryt + yoffs);
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

// _day_noon returns 0.0 for night up to 1.0 for noon
void FGClimate::update_day_factor()
{
    // noon is when lon == 180.0
    _day_noon = fabs(_adj_longitude_deg - 180.0)/180.0;
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

    _year = (_is_autumn) ? 0.5+0.5*_season_summer : 0.5-0.5*_season_summer;

    _season_transistional = 2.5*(1.0 - _season_summer) - 1.0;
    if (_season_transistional < 0.0) _season_transistional = 0.0;
    else if (_season_transistional > 1.0) _season_transistional = 1.0;
}


void FGClimate::set_ocean()
{
    double day = _day_noon;
    double summer = _season_summer;

    // temperature based on latitude, season and time of day
    // the equator does not really have seasons, only day and night
    double temp_equator_day = 31.0;
    double temp_equator_night = 23.0;
    double temp_equator = season_even(day, temp_equator_night, temp_equator_day);

    // the poles do not really have day or night, only seasons
    double temp_pole = season_even(summer, -48.0, -8.0);

    double latitude_deg = _positionLatitudeNode->getDoubleValue();
    double fact_lat = pow(fabs(latitude_deg)/90.0, 2.5);
    double ifact_lat = 1.0 - fact_lat;

    _set(_temperature_gl, season_even(ifact_lat, temp_pole, temp_equator));

    double temp_mean_equator = 0.5*(temp_equator_day + temp_equator_night);
    _set(_temperature_mean_gl, season_even(ifact_lat, temp_pole, temp_mean_equator));

    // relative humidity based on latitude
    _set(_relative_humidity_gl, triangular(fabs(fact_lat-0.5), 70.0, 87.0));

    // steady winds
    _set(_wind, 3.0);

    // no autumn
    _has_autumn = false;

    // average precipitation
    _precipitation_annual = 990.0; // global average
    double avg_precipitation = 100.0 - (_precipitation_annual/25.0);
    _set(_precipitation, avg_precipitation);
}

// https://en.wikipedia.org/wiki/Tropical_rainforest_climate
// https://en.wikipedia.org/wiki/Tropical_monsoon_climate
void FGClimate::set_tropical()
{
    double day = _day_noon;

    double summer = _season_summer;
    double winter = 1.0 - summer;

    double humidity_fact = season_even(winter, 0.5, 1.0-0.5*day);

    // wind based on latitude (0.0 - 15 degrees)
    double latitude_deg = _positionLatitudeNode->getDoubleValue();
    double fact_lat = std::max(abs(latitude_deg), 15.0)/15.0;
    double wind = 3.0*fact_lat*fact_lat;

    double temp_night = _temperature_sl;
    double temp_day = _temperature_sl;
    double precipitation = _precipitation;
    double relative_humidity = _relative_humidity_gl;
    switch(_code)
    {
    case 1: // Af: equatorial, fully humid
        temp_night = triangular(summer, 20.0, 22.5);
        temp_day = triangular(summer, 29.5, 32.5);
        precipitation = season_even(winter, 150.0, 280.0);
        relative_humidity = triangular(humidity_fact, 75.0, 85.0);
        break;
    case 2: // Am: equatorial, monsoonal
        temp_night = triangular(summer, 17.5, 22.5, MONTH);
        temp_day = triangular(summer, 27.5, 32.5, MONTH);
        precipitation = season_even(summer, 75.0, 320.0, MONTH);
        relative_humidity = triangular(humidity_fact, 75.0, 85.0, MONTH);
        wind *= 2.0*_precipitation/320.0;
        break;
    case 3: // As: equatorial, summer dry
        temp_night = season_long_high(summer, 15.0, 22.5, 1.5*MONTH);
        temp_day = triangular(summer, 27.5, 35.0, MONTH);
        precipitation = season_even(summer , 35.0, 150.0, 2.0*MONTH);
        relative_humidity = triangular(humidity_fact, 60.0, 80.0, 2.0*MONTH);
        wind *= 2.0*_precipitation/350.0;
        break;
    case 4: // Aw: equatorial, winter dry
        temp_night = season_long_high(summer, 15.0, 22.5, 1.5*MONTH);
        temp_day = triangular(summer, 27.5, 35.0, 2.0*MONTH);
        precipitation = season_even(summer, 10.0, 230.0, 2.0*MONTH);
        relative_humidity = triangular(humidity_fact, 60.0, 80.0, 2.0*MONTH);
        wind *= 2.0*_precipitation/230.0;
        break;
    default:
        break;
    }

    _set(_temperature_gl, season_even(day, temp_night, temp_day));
    _set(_temperature_mean_gl, 0.5*(temp_night + temp_day));

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
    double winter = 1.0 - summer;

    double humidity_fact = season_even(winter, 0.5, 1.0-0.5*day);

    double temp_night = _temperature_sl;
    double temp_day = _temperature_sl;
    double precipitation = _precipitation;
    double relative_humidity = _relative_humidity_gl;
    switch(_code)
    {
    case 5: // BSh: arid, steppe, hot arid
        temp_night = season_long_high(summer, 10.0, 22.0, MONTH);
        temp_day = triangular(summer, 27.5, 35.0, 2.0*MONTH);
        precipitation = season_long_low(summer, 8.0, 117.0, 2.0*MONTH);
        relative_humidity = triangular(humidity_fact, 20.0, 30.0, 2.0*MONTH);
        break;
    case 6: // BSk: arid, steppe, cold arid
        temp_night = season_even(summer, -14.0, 12.0, MONTH);
        temp_day = season_even(summer, 0.0, 30.0, MONTH);
        precipitation = season_even(summer, 15.0, 34.0, MONTH);
        relative_humidity = season_even(humidity_fact, 48.0, 67.0, MONTH);
        break;
    case 7: // BWh: arid, desert, hot arid
        temp_night = season_even(summer, 7.5, 22.0, 1.5*MONTH);
        temp_day = season_long(summer, 22.5, 37.5, 1.5*MONTH);
        precipitation = monsoonal(summer, 3.0, 18.0, 2.0*MONTH);
        relative_humidity = monsoonal(humidity_fact, 25.0, 55.0, 2.0*MONTH);
        break;
    case 8: // BWk: arid, desert, cold arid
        temp_night = season_even(summer, -15.0, 15.0, MONTH);
        temp_day = season_even(summer, -2.0, 30.0, MONTH);
        precipitation = linear(summer, 4.0, 14.0, MONTH);
        relative_humidity = linear(humidity_fact, 45.0, 61.0, MONTH);
        break;
    default:
        break;
    }

    _set(_temperature_gl, season_even(day, temp_night, temp_day));
    _set(_temperature_mean_gl, 0.5*(temp_night + temp_day));

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
    double winter = 1.0 - summer;

    double humidity_fact = season_even(winter, 0.5, 1.0-0.5*day);

    double temp_night = _temperature_sl;
    double temp_day = _temperature_sl;
    double precipitation = _precipitation;
    double relative_humidity = _relative_humidity_gl;
    switch(_code)
    {
    case 9: // Cfa: warm temperature, fully humid hot summer
        temp_night = season_even(summer, -3.0, 20.0, 1.5*MONTH);
        temp_day = season_even(summer, 10.0, 33.0, 1.5*MONTH);
        precipitation = season_even(summer, 60.0, 123.0);
        relative_humidity = season_even(humidity_fact, 65.0, 80.0);
        break;
    case 10: // Cfb: warm temperature, fully humid, warm summer
        temp_night = season_even(summer, -3.0, 10.0, 1.5*MONTH);
        temp_day = season_even(summer, 5.0, 25.0, 1.5*MONTH);
        precipitation = season_long(summer, 44.0, 88.0);
        relative_humidity = season_even(humidity_fact, 68.0, 87.0, 1.5*MONTH);
        break;
    case 11: // Cfc: warm temperature, fully humid, cool summer
        temp_night = season_long_low(summer, -3.0, 8.0, 1.5*MONTH);
        temp_day = season_long_low(summer, 2.0, 14.0, 1.5*MONTH);
        precipitation = season_even(winter, 44.0, 88.0, 4.0*MONTH);
        relative_humidity = season_long_low(humidity_fact, 70.0, 85.0, 1.5*MONTH);
        break;
    case 12: // Csa: warm temperature, summer dry, hot summer
        temp_night = season_even(summer, 2.0, 16.0, MONTH);
        temp_day = season_even(summer, 12.0, 33.0, MONTH);
        precipitation = season_long_high(summer, 25.0, 70.0);
        relative_humidity = season_even(humidity_fact, 58.0, 72.0, MONTH);
        break;
    case 13: // Csb: warm temperature, summer dry, warm summer
        temp_night = linear(summer, -4.0, 10.0, 1.5*MONTH);
        temp_day = linear(summer, 6.0, 27.0, 1.5*MONTH);
        precipitation = season_even(winter, 25.0, 120.0, MONTH);
        relative_humidity = linear(humidity_fact, 50.0, 72.0, 1.5*MONTH);
        break;
    case 14: // Csc: warm temperature, summer dry, cool summer
        temp_night = season_even(summer, -4.0, 5.0, 0.5*MONTH);
        temp_day = season_even(summer, 5.0, 16.0, 0.5*MONTH);
        precipitation = season_even(winter, 60.0, 95.0, -MONTH);
        relative_humidity = season_even(humidity_fact, 55.0, 75.0);
        break;
    case 15: // Cwa: warm temperature, winter dry, hot summer
        temp_night = season_even(summer, 4.0, 20.0, MONTH);
        temp_day = season_long_low(summer, 15.0, 30.0, MONTH);
        precipitation = season_long_low(summer, 10.0, 320.0, MONTH);
        relative_humidity = season_even(humidity_fact, 60.0, 79.0, MONTH);
        break;
    case 16: // Cwb: warm temperature, winter dry, warm summer
        temp_night = season_even(summer, 1.0, 13.0, MONTH);
        temp_day = season_long_low(summer, 15.0, 27.0, MONTH);
        precipitation = season_long_low(summer, 10.0, 250.0, MONTH);
        relative_humidity = season_even(humidity_fact, 58.0, 72.0, MONTH);
        break;
    case 17: // Cwc: warm temperature, winter dry, cool summer
        temp_night = season_long_high(summer, -9.0, 6.0, MONTH);
        temp_day = season_long_high(summer, 6.0, 17.0, MONTH);
        precipitation = season_long_low(summer, 10.0, 200.0, MONTH);
        relative_humidity = season_long_high(humidity_fact, 50.0, 58.0, MONTH);
        break;
    default:
        break;
    }

    _set(_temperature_gl, season_even(day, temp_night, temp_day));
    _set(_temperature_mean_gl, 0.5*(temp_night + temp_day));

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
    double winter = 1.0 - summer;

    double humidity_fact = season_even(winter, 0.5, 1.0-0.5*day);

    double temp_day = _temperature_sl;
    double temp_night = _temperature_sl;
    double precipitation = _precipitation;
    double relative_humidity = _relative_humidity_gl;
    switch(_code)
    {
    case 18: // Dfa: snow, fully humid, hot summer
        temp_night = season_even(summer, -15.0, 13.0, MONTH);
        temp_day = season_even(summer, -5.0, 30.0, MONTH);
        precipitation = season_even(summer, 25.0, 65.0);
        relative_humidity = season_even(humidity_fact, 68.0, 72.0);
        break;
    case 19: // Dfb: snow, fully humid, warm summer, warm summer
        temp_night = season_even(summer, -17.5, 10.0, MONTH);
        temp_day = season_even(summer, -7.5, 25.0, MONTH);
        precipitation = season_even(summer, 30.0, 70.0, MONTH);
        relative_humidity = season_even(humidity_fact, 69.0, 81.0, MONTH);
        break;
    case 20: // Dfc: snow, fully humid, cool summer, cool summer
        temp_night = season_even(summer, -30.0, 4.0, MONTH);
        temp_day = season_even(summer, -20.0, 15.0, MONTH);
        precipitation = season_even(summer, 22.0, 68.0, 1.5*MONTH);
        relative_humidity = season_even(humidity_fact, 70.0, 88.0, MONTH);
        break;
    case 21: // Dfd: snow, fully humid, extremely continetal
        temp_night = season_even(summer, -45.0, 4.0, MONTH);
        temp_day = season_even(summer, -35.0, 10.0, MONTH);
        precipitation = season_long_low(summer, 7.5, 45.0, 1.5*MONTH);
        relative_humidity = season_even(humidity_fact, 80.0, 90.0, MONTH);
        break;
    case 22: // Dsa: snow, summer dry, hot summer
        temp_night = season_even(summer, -10.0, 10.0, 1.5*MONTH);
        temp_day = season_even(summer, 0.0, 30.0, 1.5*MONTH);
        precipitation = season_long_high(winter, 2.0, 70.0, 2.0*MONTH);
        relative_humidity = season_even(humidity_fact, 48.0, 58.08, 1.5*MONTH);
        break;
    case 23: // Dsb: snow, summer dry, warm summer
        temp_night = season_even(summer, -15.0, 6.0, 1.5*MONTH);
        temp_day = season_even(summer, -4.0, 25.0, 1.5*MONTH);
        precipitation = season_long_high(winter, 12.0, 73.0, 2.0*MONTH);
        relative_humidity = season_even(humidity_fact, 50.0, 68.0, 1.5*MONTH);
        break;
    case 24: // Dsc: snow, summer dry, cool summer
        temp_night = season_even(summer, -27.5, 2.0, MONTH);
        temp_day = season_even(summer, -4.0, 15.0, MONTH);
        precipitation = season_long_low(summer, 32.5, 45.0, MONTH);
        relative_humidity = season_even(humidity_fact, 50.0, 60.0, MONTH);
        break;
    case 25: // Dsd: snow, summer dry, extremely continetal
        temp_night = season_even(summer, -11.5, -6.5, MONTH);
        temp_day = season_even(summer, 14.0, 27.0, MONTH);
        precipitation = season_long_low(summer, 5.0, 90.0, MONTH);
        relative_humidity = season_even(humidity_fact, 48.0, 62.0, MONTH);
        break;
    case 26: // Dwa: snow, winter dry, hot summer
        temp_night = season_even(summer, -18.0, 16.5, MONTH);
        temp_day = season_even(summer, -5.0, 25.0, MONTH);
        precipitation = season_long_low(summer, 5.0, 180.0, 1.5*MONTH);
        relative_humidity = season_even(humidity_fact, 60.0, 68.0, MONTH);
        break;
    case 27: // Dwb: snow, winter dry, warm summer
        temp_night = season_even(summer, -28.0, 10.0, MONTH);
        temp_day = season_even(summer, -12.5, 22.5, MONTH);
        precipitation = season_long_low(summer, 10.0, 140.0, 1.5*MONTH);
        relative_humidity = season_even(humidity_fact, 60.0, 72.0, MONTH);
        break;
    case 28: // Dwc: snow, winter dry, cool summer
        temp_night = season_even(summer, -33.0, 5.0, MONTH);
        temp_day = season_even(summer, -20.0, 20.0, MONTH);
        precipitation = season_long_low(summer, 10.0, 110.0, 1.5*MONTH);
        relative_humidity = season_even(humidity_fact, 60.0, 78.0, MONTH);
        break;
    case 29: // Dwd: snow, winter dry, extremely continetal
        temp_night = season_even(summer, -57.5, 0.0, MONTH);
        temp_day = season_even(summer, -43.0, 15.0, MONTH);
        precipitation = season_even(summer, 8.0, 63.0, 1.5*MONTH);
        relative_humidity = 80.0;
        break;
    default:
        break;
    }

    _set(_temperature_gl, season_even(day, temp_night, temp_day));
    _set(_temperature_mean_gl, 0.5*(temp_night + temp_day));

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
    double winter = 1.0 - summer;

    double humidity_fact = season_even(winter, 0.5, 1.0-0.5*day);

    // polar climate also occurs high in the mountains
    double temp_day = _temperature_sl;
    double temp_night = _temperature_sl;
    double precipitation = _precipitation;
    double relative_humidity = _relative_humidity_gl;
    switch(_code)
    {
    case 30: // EF: polar frost
        temp_night = season_long_low(summer, -35.0, -6.0, MONTH);
        temp_day = season_long_low(summer, -32.5, 0.0, MONTH);
        precipitation = season_even(summer, 50.0, 80.0, 3.0*MONTH);
        relative_humidity = season_long_low(humidity_fact, 65.0, 75.0, MONTH);
        break;
    case 31: // ET: polar tundra
        temp_night = season_even(summer, -30.0, 0.0, MONTH);
        temp_day = season_even(summer, -22.5, 8.0, 1.5*MONTH);
        precipitation = season_even(summer, 15.0, 45.0, 2*MONTH);
        relative_humidity = season_even(humidity_fact, 60.0, 88.0, MONTH);
        break;
    default:
        break;
    }

    _set(_temperature_gl, season_even(day, temp_night, temp_day));
    _set(_temperature_mean_gl, 0.5*(temp_night + temp_day));

    _set(_relative_humidity_gl, relative_humidity);

    _set(_precipitation_annual, 990.0);
    _set(_precipitation, precipitation);

    _has_autumn = false;
    _set(_wind, 3.0);
}

void FGClimate::set_environment()
{
    double latitude_deg = _positionLatitudeNode->getDoubleValue();
    double fact_lat = pow(fabs(latitude_deg)/90.0, 2.5);
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

    // The temperature decreases by about 9.8°C per kilometer
    _set(_snow_thickness,  pow(snow_fact, 2.0));
    _set(_ice_cover, pow(fact_lat, 2.5));

    // less than 20 mm/month of precipitation is considered dry.
    if (_precipitation < 20.0 || _precipitation_annual < 240.0)
    {
        _set(_dust_cover, 0.3 - 0.3*sqrtf(_precipitation/20.0));
        _set(_lichen_cover, 0.0);
        _set(_wetness, 0.0);
    }
    else
    {
        double wetness = _precipitation - 20.0;
        wetness = std::min(12.0*wetness/_precipitation_annual, 1.0);

        _set(_dust_cover, 0.0);
        _set(_wetness, pow(wetness, 3.0));

        double cover = std::min(_precipitation_annual, 990.0)/990.0;
        _set(_lichen_cover, 0.5*pow(wetness*cover, 1.5));
    }

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
        if (_has_autumn && _is_autumn)
            fgSetDouble("/environment/season", 2.0*_season_transistional);
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

// val is the progress indicator between 0.0 and 1.0
double FGClimate::linear(double val, double min, double max, double offs)
{
    double diff = max-min;
    val += fmod(offs, 1.0);
    if (val > 1.0) val -= 1.0;
    else if (val < 1.0) val += 1.0;
    return min + val*diff;
}

// google: y=1-abs(-1+2*x)
// low season is around 0.0 and 1.0, high season around 0.5
double FGClimate::triangular(double val, double min, double max, double offs)
{
    double diff = max-min;
    val -= fmod(offs, 1.0);
    if (val > 1.0) val -= 1.0;
    else if (val < 1.0) val += 1.0;
    val = 1.0 - fabs(-1.0 + 2*val);
    return min + val*diff;
}

// google: y=0.5-0.5*cos(x)
// the short low season is round 0.0, the short high season around 1.0
double FGClimate::season_even(double val, double min, double max, double offs)
{
    double diff = max-min;
    val -= fmod(offs, 1.0);
    if (val > 1.0) val -= 1.0;
    else if (val < 1.0) val += 1.0;
    return min + diff*(0.5 + 0.5*cos(SGD_PI*val));
}

// google: y=0.5-0.6366*atan(cos(x))
// the medium long low season is round 0.0, medium long high season round 1.0
double FGClimate::season_long(double val, double min, double max, double offs)
{
    double diff = max-min;
    val -= fmod(offs, 1.0);
    if (val > 1.0) val -= 1.0;
    else if (val < 1.0) val += 1.0;
    return min + diff*(0.5 + 0.6366*atan(cos(SGD_PI*val)));
}

// google: y=0.5-0.5*cos(x^1.5)
// 2.145 = pow(SGD_PI, 1.0/1.5)
// the long low season is around 0.0, the short high season around 1.0
double FGClimate::season_long_low(double val, double min, double max, double offs)
{
    double diff = max-min;
    val -= fmod(offs, 1.0);
    if (val > 1.0) val -= 1.0;
    else if (val < 1.0) val += 1.0;
    return min + diff*(0.5 + 0.5*cos(pow(2.145*val, 1.5)));
}

// google: y=0.5+0.5*cos(x^1.5)
// 2.14503 = pow(SGD_PI, 1.0/1.5)
// the long high season is around 0.0, the short low season around 1.0
double FGClimate::season_long_high(double val, double min, double max, double offs)
{
    double diff = max-min;
    val -= fmod(offs, 1.0);
    if (val > 1.0) val -= 1.0;
    else if (val < 1.0) val += 1.0;
    return max + diff*(0.5 - 0.5*cos(pow(2.14503 - 2.14503*val, 1.5)));
}

// goole: y=cos(atan(x*x))
// the monsoon is around 0.0
double FGClimate::monsoonal(double val, double min, double max, double offs)
{
    double diff = max-min;
    val -= fmod(offs, 1.0);
    if (val > 1.0) val -= 1.0;
    else if (val < 1.0) val += 1.0;
    val = 2.0*SGD_2PI*(1.0-val);
    return min + diff*cos(atan(val*val));
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
    std::cout << "  Year:                                  " << _year
              << std::endl;
    std::cout << "  Season (0.0 = winter .. 1.0 = summer): " << _season_summer
              << std::endl;
    std::cout << "  Daytime....(0.0 = night .. 1.0 = day): " << _day_noon
              << std::endl;
    std::cout << "  Sea level temperature: " << _temperature_sl << " deg. C."
              << std::endl;
    std::cout << "  Ground temperature:    " << _temperature_gl << " deg. C."
              << std::endl;
    std::cout << "  Mean temperature:      " << _temperature_mean_gl
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
              << ((_has_autumn && _is_autumn) ? _season_transistional : 0.0)
              << std::endl;
    std::cout << "===============================================" << std::endl;
}
#endif // REPORT_TO_CONSOLE
