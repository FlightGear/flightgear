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

FGClimate::FGClimate(const SGGeod& position)
{
    SGPath img_path = globals->get_fg_root() / "Geodata" / "koppen-geiger.png";

    image = osgDB::readImageFile(img_path.utf8Str());
    if (image)
    {
        _image_width = image->s();
        _image_height = image->t();
        _epsilon = 360.0/_image_width;
    }

    update(position);
}

// Set all environment parameters based on the koppen-classicfication
// https://en.wikipedia.org/wiki/K%C3%B6ppen_climate_classification
// http://vectormap.si.edu/Climate.htm
void FGClimate::update(const SGGeod& position)
{
    FGLight *l = globals->get_subsystem<FGLight>();
    if (!l) return; // e.g. during unit-testing

    _sun_longitude_deg = l->get_sun_lon()*SGD_RADIANS_TO_DEGREES;
    _sun_latitude_deg = l->get_sun_lat()*SGD_RADIANS_TO_DEGREES;

    pos = position;
    double latitude_deg  = pos.getLatitudeDeg();
    double longitude_deg = pos.getLongitudeDeg();

    _adj_latitude_deg = latitude_deg - _sun_latitude_deg;
    _adj_longitude_deg = _sun_longitude_deg - longitude_deg;
    if (_adj_longitude_deg < 0.0) _adj_longitude_deg += 360.0;
    else if (_adj_longitude_deg >= 360.0) _adj_longitude_deg -= 360.0;

    if (fabs(_prev_lat - _adj_latitude_deg) > _epsilon ||
        fabs(_prev_lon - _adj_longitude_deg) > _epsilon)
    {
        _is_autumn = false;
        if ((latitude_deg*_sun_latitude_deg < 0.0) // autumn + spring
              && (_adj_latitude_deg > _prev_lat)) // autumn
        {
            _is_autumn = true;
        }

        _prev_lat = _adj_latitude_deg;
        _prev_lon = _adj_longitude_deg;

        update_day_factor();
        update_season_factor();

        osg::Vec4f color;
        if (image)
        {
            // from lat/lon to screen coordinates
            double x = 180.0 + longitude_deg;
            double y =  90.0 + latitude_deg;
            double s, t;

            s = static_cast<int>(x * static_cast<double>(_image_width)/360.0);
            t = static_cast<int>(y * static_cast<double>(_image_height)/180.0);

            color = image->getColor(s, t);
        }

        // convert from color shades to koppen-classicfication
        _classicfication = static_cast<int>( 255.0f*color[0]/4.0f );

        if (_classicfication == 0) set_ocean();
        else if (_classicfication < 5) set_tropical();
        else if (_classicfication < 9) set_dry();
        else if (_classicfication < 18) set_temperate();
        else if (_classicfication < 30) set_continetal();
        else if (_classicfication < 32) set_polar();
        else set_ocean();

        // Mark G. Lawrence:
        _dew_point = _temperature_gl - ((100.0 - 100.0*_relative_humidity)/5.0);

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
    double latitude_deg = pos.getLatitudeDeg();
    double sign = latitude_deg >= 0.0 ? 1.0 : -1.0; // hemisphere

    _season_summer = (23.5 + sign*_sun_latitude_deg)/(2.0*23.5);

    _season_transistional = 2.0*(1.0 - _season_summer) - 1.0;
    if (_season_transistional < 0.0) _season_transistional = 0.0;
    else if (_season_transistional > 1.0) _season_transistional = 1.0;
}


void FGClimate::set_ocean()
{
    _total_annual_precipitation = 990.0; // use the global value, for now.

    double day = _day_noon;
    double summer = _season_summer;

    // temperature based on latitude, season and time of day
    // the equator does not really have seasons, only day and night
    double temp_equator_max = 31.0;
    double temp_equator_min = 23.0;
    double temp_equator = season_even(day, temp_equator_min, temp_equator_max);

    // the poles do not really have day or night, only seasons
    double temp_summer_pole = -8.0;
    double temp_winter_pole = -48.0;
    double temp_pole = season_even(summer, temp_winter_pole, temp_summer_pole);

    double fact_lat = pow(fabs(pos.getLatitudeDeg())/90.0, 2.5);
    double ifact_lat = 1.0 - fact_lat;

    _temperature_sl = season_even(ifact_lat, temp_pole, temp_equator);

    double temp_mean_equator = 0.5*(temp_equator_max + temp_equator_min);
    _temperature_mean = season_even(ifact_lat, temp_pole, temp_mean_equator);

    // relative humidity based on latitude
    _relative_humidity = season_even(fact_lat, 0.1, 0.8);

    // steady winds
    _wind = 3.0;

    // no autumn
    _has_autumn = false;

    // average precipitation
    _total_annual_precipitation = 990.0; // global
    double avg_precipitation = 100.0 - (_total_annual_precipitation/25.0);
    _precipitation = avg_precipitation;
}

// https://en.wikipedia.org/wiki/Tropical_rainforest_climate
// https://en.wikipedia.org/wiki/Tropical_monsoon_climate
void FGClimate::set_tropical()
{
    // weather
    _total_annual_precipitation = 3000.0;

    double day = _day_noon;

    double summer = _season_summer;
    double winter = 1.0 - summer;

    double humidity_fact = season_even(winter, 0.5, 1.0-0.5*day);

    // wind based on latitude (0.0 - 15 degrees)
    double latitude_deg = pos.getLatitudeDeg();
    double fact_lat = std::max(abs(latitude_deg), 15.0)/15.0;
    double wind = 3.0*fact_lat*fact_lat;

    double temp_night = _temperature_sl;
    double temp_day = _temperature_sl;
    double precipitation = _precipitation;
    double relative_humidity = _relative_humidity;
    switch(_classicfication)
    {
    case 1: // Af: equatorial, fully humid
        temp_night = triangular(summer, 20.0, 22.5);
        temp_day = triangular(summer, 29.5, 32.5);
        precipitation = season_even(winter, 150.0, 280.0);
        relative_humidity = triangular(humidity_fact, 0.75, 0.85);
        break;
    case 2: // Am: equatorial, monsoonal
        temp_night = triangular(summer, 17.5, 22.5);
        temp_day = triangular(summer, 27.5, 32.5);
        precipitation = season_even(summer, 75.0, 320.0);
        relative_humidity = triangular(humidity_fact, 0.75, 0.85);
        wind *= 2.0*_precipitation/320.0;
        break;
    case 3: // As: equatorial, summer dry
        temp_night = season_long_high(summer, 15.0, 22.5);
        temp_day = triangular(summer, 27.5, 35.0);
        precipitation = season_even(summer , 35.0, 150.0);
        relative_humidity = triangular(humidity_fact, 0.60, 0.80);
        wind *= 2.0*_precipitation/350.0;
        break;
    case 4: // Aw: equatorial, winter dry
        temp_night = season_long_high(summer, 15.0, 22.5);
        temp_day = triangular(summer, 27.5, 35.0);
        precipitation = season_even(summer, 10.0, 230.0);
        relative_humidity = triangular(humidity_fact, 0.60, 0.80);
        wind *= 2.0*_precipitation/230.0;
        break;
    default:
        break;
    }

    _temperature_gl = season_even(day, temp_night, temp_day);
    _temperature_mean = 0.5*(temp_night + temp_day);
    _temperature_sl = _temperature_gl;

    _relative_humidity = relative_humidity;
    _precipitation = precipitation;
    _has_autumn = false;
    _wind = wind;
}

// https://en.wikipedia.org/wiki/Desert_climate
// https://en.wikipedia.org/wiki/Semi-arid_climate
void FGClimate::set_dry()
{
    double day = _day_noon;

    double summer = _season_summer;
    double winter = 1.0 - summer;

    double humidity_fact = season_even(winter, 0.5, 1.0-0.5*day);

    _total_annual_precipitation = 100.0;
    if (_classicfication == 5 || _classicfication == 6) {	// steppe
        _total_annual_precipitation *= 2.0;
    }

    double temp_night = _temperature_sl;
    double temp_day = _temperature_sl;
    double precipitation = _precipitation;
    double relative_humidity = _relative_humidity;
    switch(_classicfication)
    {
    case 5: // BSh: arid, steppe, hot arid
        temp_night = season_long_high(summer, 10.0, 22.0);
        temp_day = triangular(summer, 27.5, 35.0);
        precipitation = season_long_low(summer, 8.0, 117.0);
        relative_humidity = triangular(humidity_fact, 0.20, 0.30);
        break;
    case 6: // BSk: arid, steppe, cold arid
        temp_night = season_even(summer, -14.0, 12.0);
        temp_day = season_even(summer, 0.0, 30.0);
        precipitation = season_even(summer, 15.0, 34.0);
        relative_humidity = season_even(humidity_fact, 0.48, 0.67);
        break;
    case 7: // BWh: arid, desert, hot arid
        temp_night = season_even(summer, 7.5, 22.0);
        temp_day = season_even(summer, 22.5, 37.5);
        precipitation = monsoonal(summer, 3.0, 18.0);
        relative_humidity = monsoonal(humidity_fact, 0.25, 0.55);
        break;
    case 8: // BWk: arid, desert, cold arid
        temp_night = season_even(summer, -15.0, 15.0);
        temp_day = season_even(summer, -2.0, 30.0);
        precipitation = linear(summer, 4.0, 14.0);
        relative_humidity = linear(humidity_fact, 0.45, 0.61);
        break;
    default:
        break;
    }

    _temperature_gl = season_even(day, temp_night, temp_day);
    _temperature_mean = 0.5*(temp_night + temp_day);
    _temperature_sl = _temperature_gl;

    _relative_humidity = relative_humidity;
    _precipitation = precipitation;
    _has_autumn = false;
    _wind = 3.0;
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
    double relative_humidity = _relative_humidity;
    switch(_classicfication)
    {
    case 9: // Cfa: warm temperature, fully humid hot summer
        temp_night = season_even(summer, -3.0, 20.0);
        temp_day = season_even(summer, 10.0, 33.0);
        precipitation = season_even(summer, 60.0, 123.0);
        relative_humidity = season_even(humidity_fact, 0.65, 0.80);
        break;
    case 10: // Cfb: warm temperature, fully humid, warm summer
        temp_night = season_even(summer, -3.0, 10.0);
        temp_day = season_even(summer, 5.0, 25.0);
        precipitation = linear(0.5*(summer+_season_transistional), 44.0, 88.0);
        relative_humidity = season_even(humidity_fact, 0.68, 0.87);
        break;
    case 11: // Cfc: warm temperature, fully humid, cool summer
        temp_night = season_long_low(summer, -3.0, 8.0);
        temp_day = season_long_low(summer, 2.0, 14.0);
        precipitation = season_even(winter, 44.0, 88.0);
        relative_humidity = season_long_low(humidity_fact, 0.70, 0.85);
        break;
    case 12: // Csa: warm temperature, summer dry, hot summer
        temp_night = season_even(summer, 2.0, 16.0);
        temp_day = season_even(summer, 12.0, 33.0);
        precipitation = season_long_low(winter, 25.0, 70.0);
        relative_humidity = season_even(humidity_fact, 0.58, 0.72);
        break;
    case 13: // Csb: warm temperature, summer dry, warm summer
        temp_night = linear(summer, -4.0, 10.0);
        temp_day = linear(summer, 6.0, 27.0);
        precipitation = season_short(winter, 25.0, 120.0);
        relative_humidity = linear(humidity_fact, 0.50, 0.72);
        break;
    case 14: // Csc: warm temperature, summer dry, cool summer
        temp_night = season_even(summer, -4.0, 5.0);
        temp_day = season_even(summer, 5.0, 16.0);
        precipitation = season_even(winter, 60.0, 95.0);
        relative_humidity = season_even(humidity_fact, 0.55, 0.75);
        break;
    case 15: // Cwa: warm temperature, winter dry, hot summer
        temp_night = season_even(summer, 4.0, 20.0);
        temp_day = season_long_low(summer, 15.0, 30.0);
        precipitation = season_long_low(summer, 10.0, 320.0);
        relative_humidity = season_even(humidity_fact, 0.60, 0.79);
        break;
    case 16: // Cwb: warm temperature, winter dry, warm summer
        temp_night = season_even(summer, 1.0, 13.0);
        temp_day = season_long_low(summer, 15.0, 27.0);
        precipitation = season_long_low(summer, 10.0, 250.0);
        relative_humidity = season_even(humidity_fact, 0.58, 0.72);
        break;
    case 17: // Cwc: warm temperature, winter dry, cool summer
        temp_night = season_long_high(summer, -9.0, 6.0);
        temp_day = season_long_high(summer, 6.0, 17.0);
        precipitation = season_long_low(summer, 10.0, 200.0);
        relative_humidity = season_long_high(humidity_fact, 0.50, 0.58);
        break;
    default:
        break;
    }

    _temperature_gl = season_even(day, temp_night, temp_day);
    _temperature_mean = 0.5*(temp_night + temp_day);
    _temperature_sl = _temperature_gl;

    _relative_humidity = relative_humidity;
    _precipitation = precipitation;
    _has_autumn = true;
    _wind = 3.0;

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
    double relative_humidity = _relative_humidity;
    switch(_classicfication)
    {
    case 18: // Dfa: snow, fully humid, hot summer
        temp_night = season_even(summer, -15.0, 13.0);
        temp_day = season_even(summer, -5.0, 30.0);
        precipitation = season_even(summer, 25.0, 65.0);
        relative_humidity = season_even(humidity_fact, 0.68, 0.72);
        break;
    case 19: // Dfb: snow, fully humid, warm summer, warm summer
        temp_night = season_even(summer, -17.5, 10.0);
        temp_day = season_even(summer, -7.5, 25.0);
        precipitation = season_even(summer, 30.0, 70.0);
        relative_humidity = season_even(humidity_fact, 0.69, 0.81);
        break;
    case 20: // Dfc: snow, fully humid, cool summer, cool summer
        temp_night = season_even(summer, -30.0, 4.0);
        temp_day = season_even(summer, -20.0, 15.0);
        precipitation = season_even(summer, 22.0, 68.0);
        relative_humidity = season_even(humidity_fact, 0.70, 0.88);
        break;
    case 21: // Dfd: snow, fully humid, extremely continetal
        temp_night = season_short(summer, -45.0, 4.0);
        temp_day = season_short(summer, -35.0, 10.0);
        precipitation = season_long_low(summer, 7.5, 45.0);
        relative_humidity = season_short(humidity_fact, 0.80, 0.90);
        break;
    case 22: // Dsa: snow, summer dry, hot summer
        temp_night = season_even(summer, -10.0, 10.0);
        temp_day = season_even(summer, 0.0, 30.0);
        precipitation = season_long_high(winter, 2.0, 70.0);
        relative_humidity = season_even(humidity_fact, 0.48, 0.58);
        break;
    case 23: // Dsb: snow, summer dry, warm summer
        temp_night = season_even(summer, -15.0, 6.0);
        temp_day = season_even(summer, -4.0, 25.0);
        precipitation = season_long_high(winter, 12.0, 73.0);
        relative_humidity = season_even(humidity_fact, 0.50, 0.68);
        break;
    case 24: // Dsc: snow, summer dry, cool summer
        temp_night = season_even(summer, -27.5, 2.0);
        temp_day = season_even(summer, -4.0, 15.0);
        precipitation = season_long_low(summer, 32.5, 45.0);
        relative_humidity = season_even(humidity_fact, 0.50, 0.60);
        break;
    case 25: // Dsd: snow, summer dry, extremely continetal
        temp_night = season_even(summer, -11.5, -6.5);
        temp_day = season_even(summer, 14.0, 27.0);
        precipitation = season_long_low(summer, 5.0, 90.0);
        relative_humidity = season_even(humidity_fact, 0.48, 0.62);
        break;
    case 26: // Dwa: snow, winter dry, hot summer
        temp_night = season_even(summer, -18.0, 16.5);
        temp_day = season_even(summer, -5.0, 25.0);
        precipitation = season_long_low(summer, 5.0, 180.0);
        relative_humidity = season_even(humidity_fact, 0.60, 0.68);
        break;
    case 27: // Dwb: snow, winter dry, warm summer
        temp_night = season_even(summer, -28.0, 10.0);
        temp_day = season_even(summer, -12.5, 22.5);
        precipitation = season_long_low(summer, 10.0, 140.0);
        relative_humidity = season_even(humidity_fact, 0.60, 0.72);
        break;
    case 28: // Dwc: snow, winter dry, cool summer
        temp_night = season_even(summer, -33.0, 5.0);
        temp_day = season_even(summer, -20.0, 20.0);
        precipitation = season_long_low(summer, 10.0, 110.0);
        relative_humidity = season_even(humidity_fact, 0.60, 0.78);
        break;
    case 29: // Dwd: snow, winter dry, extremely continetal
        temp_night = season_even(summer, -57.5, 0.0);
        temp_day = season_even(summer, -43.0, 15.0);
        precipitation = season_even(summer, 8.0, 63.0);
        relative_humidity = 0.80;
        break;
    default:
        break;
    }

    _temperature_gl = season_even(day, temp_night, temp_day);
    _temperature_mean = 0.5*(temp_night + temp_day);
    _temperature_sl = _temperature_gl;

    _relative_humidity = relative_humidity;
    _precipitation = precipitation;
    _has_autumn = true;
    _wind = 3.0;

    double fact_lat = pow(fabs(pos.getLatitudeDeg())/90.0, 2.5);
    _relative_humidity = season_even(fact_lat, 0.1, 0.8);
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
    double relative_humidity = _relative_humidity;
    switch(_classicfication)
    {
    case 30: // EF: polar frost
        temp_night = season_long_low(summer, -35.0, -6.0);
        temp_day = season_long_low(summer, -32.5, 0.0);
        precipitation = season_even(summer, 50.0, 80.0);
        relative_humidity = season_long_low(humidity_fact, 0.65, 0.75);
        break;
    case 31: // ET: polar tundra
        temp_night = season_even(summer, -30.0, 0.0);
        temp_day = season_even(summer, -22.5, 8.0);
        precipitation = season_even(summer, 15.0, 45.0);
        relative_humidity = season_even(humidity_fact, 0.60, 0.88);
        break;
    default:
        break;
    }

    _temperature_gl = season_even(day, temp_night, temp_day);
    _temperature_mean = 0.5*(temp_night + temp_day);
    _temperature_sl = _temperature_gl;

    _relative_humidity = relative_humidity;
    _precipitation = precipitation;
    _has_autumn = false;
    _wind = 3.0;
}

void FGClimate::set_environment()
{
    double latitude_deg = pos.getLatitudeDeg();
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

    if (_temperature_mean > 5.0 || precipitation < 0.1) {
        snow_fact = 0.0;
    }
    else
    {
        snow_fact = fabs(_temperature_mean) - 5.0;
        snow_fact = (snow_fact > 10.0) ? 1.0 : snow_fact/10.0;
        snow_fact *= precipitation;
    }

    _snow_level = (7500.0 - 8000.0*fact_lat)*(1.0 - snow_fact);
    _snow_thickness = pow(snow_fact, 2.0);
    _ice_cover = pow(fact_lat, 2.5);

    // less than 20 mm/month of precipitation is considered dry.
    if (_precipitation < 20.0 && _total_annual_precipitation < 240.0)
    {
        _dust_cover = 0.3 - 0.3*sqrtf(_precipitation/20.0);
        _lichen_cover = 0.0;
        _wetness = 0.0;
    }
    else
    {
        double wetness = _precipitation - 20.0;
        wetness = std::min(12.0*wetness/_total_annual_precipitation, 1.0);

        _dust_cover = 0.0;
        _wetness = pow(wetness, 3.0);

        double cover = std::min(_total_annual_precipitation, 990.0)/990.0;
        _lichen_cover = 0.5*pow(wetness*cover, 1.5);
    }

    if (_environment_adjust)
    {
        fgSetDouble("/environment/snow-level-m", _snow_level);
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
    val = 1.0 - fabs(-1.0 + 2*val);
    return min + val*diff;
}

// google: y =0.5-0.5*atan(cos(x))
// the short low season is round 0.0, the short high season around 1.0
double FGClimate::season_short(double val, double min, double max)
{
    double diff = max-min;
    return min + diff*(0.5 - 0.5*cos(SGD_PI*val));
}

// google: y =0.5-0.6366*atan(cos(x))
// the medium long low season is round 0.0, medium long high season round 1.0
double FGClimate::season_even(double val, double min, double max)
{
    double diff = max-min;
    return min + diff*(0.5 - 0.6366*atan(cos(SGD_PI*val)));
}

// google: y=0.5-0.5*cos(x^1.5)
// 2.145 = pow(SGD_PI, 1.0/1.5)
// the long low season is around 0.0, the short high season around 1.0
double FGClimate::season_long_low(double val, double min, double max)
{
    double diff = max-min;
    return min + diff*(0.5 - 0.5*cos(pow(2.145*val, 1.5)));
}

// google: y=0.5+0.5*cos(x^1.5)
// 2.14503 = pow(SGD_PI, 1.0/1.5)
// the long high season is around 0.0, the short low season around 1.0
double FGClimate::season_long_high(double val, double min, double max)
{
    double diff = max-min;
    return max + diff*(0.5 + 0.5*cos(pow(2.14503 - 2.14503*val, 1.5)));
}

// goole: y=cos(atan(x*x))
// the monsoon is around 0.0
double FGClimate::monsoonal(double val, double min, double max)
{
    double diff = max-min;
    val = 2.0*SGD_2PI*(1.0-val);
    return min + diff*cos(atan(val*val));
}


#if REPORT_TO_CONSOLE
void FGClimate::report()
{
    const std::string description[32] = {
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
      "Snow, fully humid, warm summer, warm summer",
      "Snow, fully humid, cool summer, cool summer",
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
    const std::string koppen_str[32] = {
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

    struct tm *t = globals->get_time_params()->getGmt();

    std::cout << "===============================================" << std::endl;
    std::cout << "Climate report for:" << std::endl;
    std::cout << "  Date: " << sgTimeFormatTime(t) << " GMT" << std::endl;
    std::cout << "  Sun latitude:    " << _sun_latitude_deg << " deg."
              << std::endl;
    std::cout << "  Sun longitude:   " << _sun_longitude_deg << " deg."
              << std::endl;
    std::cout << "  Viewer latitude:  " << pos.getLatitudeDeg() << " deg."
              << " (adjusted: " << _adj_latitude_deg << ")" << std::endl;
    std::cout << "  Viewer longitude: " << pos.getLongitudeDeg() << " deg."
              << " (adjusted: " << _adj_longitude_deg << ")" << std::endl;
    std::cout << std::endl;
    std::cout << "  Köppen classification: " << koppen_str[_classicfication]
              << std::endl;
    std::cout << "  Description: " << description[_classicfication]
              << std::endl << std::endl;
    std::cout << "  Season (0.0 = winter .. 1.0 = summer): " << _season_summer
              << std::endl;
    std::cout << "  Daytime....(0.0 = night .. 1.0 = day): " << _day_noon
              << std::endl;
    std::cout << "  Sea level temperature: " << _temperature_sl << " deg. C."
              << std::endl;
    std::cout << "  Ground temperature:    " << _temperature_gl << " deg. C."
              << std::endl;
    std::cout << "  Mean temperature:      " << _temperature_mean << " deg. C."
              << std::endl;
    std::cout << "  Months precipitation:  " << _precipitation << " mm"
              << std::endl;
    std::cout << "  Annual precipitation:  " << _total_annual_precipitation
              << " mm" << std::endl;
    std::cout << "  Relative humidity:     " << _relative_humidity*100.0 << " %"
              << std::endl;
    std::cout << "  Dew point: " << _dew_point << " deg. C." << std::endl;
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
