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
// The map is provided with a spatial resolution of 0.5 degree (3km).
// http://koeppen-geiger.vu-wien.ac.at/present.htm

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
        _prev_lat = _adj_latitude_deg;
        _prev_lon = _adj_longitude_deg;

        update_day_factor();
        update_season_factor();

        // from lat/lon to screen coordinates
        double x = 180.0 + longitude_deg;
        double y =  90.0 + latitude_deg;

        _col = static_cast<int>(x * _image_width/360.0);
        _row = static_cast<int>(y * _image_height/180.0);

        osg::Vec4f color;
        if (image) {
            color = image->getColor(_col, _row);
        }

        // convert from color shades to koppen-classicfication
        _classicfication = static_cast<int>( 255.0*color[0]/4.0 );

        if (_classicfication == 0) set_ocean();
        else if (_classicfication < 5) set_tropical();
        else if (_classicfication < 9) set_dry();
        else if (_classicfication < 18) set_temperate();
        else if (_classicfication < 30) set_continetal();
        else if (_classicfication < 32) set_polar();
        else set_ocean();

        set_environment();

#if REPORT_TO_CONSOLE
        report();
#endif
    }
}

// returns 0.0 for night up to 1.0 for noon
void FGClimate::update_day_factor()
{
    // noon is when lon == 180.0
    _day_noon = fabs(_adj_longitude_deg - 180.0)/180.0;
}

// The seasons are currently based purely on where the sun's perpendicular
// (perp.) rays strike the earth. Winter (perp. Rays moving from 23.5 S to
// equator), Spring (perp. Rays moving from the equator to 23.5 N), Summer
// (perp. Rays moving from 23.5 N to equator) and Fall (perp. Rays moving
// from equator to 23.5 S).
// returns 0.0 for winter up to 1.0 for summer
void FGClimate::update_season_factor()
{
    double latitude_deg = pos.getLatitudeDeg();
    double sign = latitude_deg >= 0.0 ? 1.0 : -1.0;
    _season_summer = (23.5 + sign*_sun_latitude_deg)/(2.0*23.5);

    _season_autumn = 0.0;
    if (latitude_deg*_sun_latitude_deg < 0.0) // autumn
    {
        _season_autumn = 2.0*(1.0 - _season_summer) - 1.0;
        if (_season_autumn < 0.0) _season_autumn = 0.0;
        else if (_season_autumn > 1.0) _season_autumn = 1.0;

        double lat_fact = 6.0 - 12.0*fabs(latitude_deg)/90.0;
        _season_autumn *= (0.5 - 0.5*sin(atan(SGD_2PI*lat_fact)));
    }
}


void FGClimate::set_ocean()
{
    _total_annual_precipitation = 990.0; // use the global value, for now.

    double day = _day_noon;
    double night = 1.0 - day;

    double summer = _season_summer;
    double winter = 1.0 - summer;

    // temperature based on latitude, season and time of day
    // the equator does not really have seasons, only day and night
    double day_temp_equator = 31.0;
    double night_temp_equator = 23.0;
    double temp_equator = day*day_temp_equator + night*night_temp_equator;

    // the poles do not really have day or night, only seasons
    double summer_temp_pole = -8.0;
    double winter_temp_pole = -48.0;
    double temp_pole = summer*summer_temp_pole + winter*winter_temp_pole;

    double lat_fact = pow(fabs(pos.getLatitudeDeg())/90.0, 2.5);
    double ifact_lat = 1.0 - lat_fact;

    _temperature_sl = ifact_lat*temp_equator + lat_fact*temp_pole;

    double temp_mean_equator = 0.5*(day_temp_equator + night_temp_equator);
    _temperature_mean = ifact_lat*temp_mean_equator + lat_fact*temp_pole;

    // relative humidity based on latitude
    _relative_humidity = 0.8 - 0.7*lat_fact;

    // steady winds
    _wind = 3.0;

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
//  double avg_precipitation = 100.0 - (_total_annual_precipitation/25.0);

    double day = _day_noon;
    double night = 1.0 - day;

    double summer = _season_summer;
    double winter = 1.0 - summer;

    double day_temp_equator = 31.0;
    double night_temp_equator = 23.0;
    double temp_equator = day*day_temp_equator + night*night_temp_equator;

    // winter temperatures are three-quarter of the summer temperatures
    _temperature_mean = 0.5*(day_temp_equator + night_temp_equator);
    _temperature_sl = 0.75*temp_equator + 0.25*summer*temp_equator;
    _temperature_gl = _temperature_sl;

    // relative humidity based on latitude
    double latitude_deg = pos.getLatitudeDeg();
    double lat_fact = fabs(latitude_deg)/90.0;
    _relative_humidity = 0.8 - 0.7*lat_fact;

    // wind based on latitude (0.0 - 15 degrees)
    lat_fact = std::max(abs(latitude_deg), 15.0)/15.0;
    _wind = 3.0*lat_fact*lat_fact;

    double monsoon;
    double monsoon_precipitation = _total_annual_precipitation;
    switch(_classicfication)
    {
    case 1: // Af: equatorial, fully humid
        _precipitation = 60.0 + sqrt(summer)*40.0;
        break;
    case 2: // Am: equatorial, monsoonal
        monsoon = cos(atan(6 + 12.0*winter)); // monsoon around the 6th month
        _precipitation = monsoon*monsoon_precipitation;
        _wind = 2.0*monsoon*_wind;
        break;
    case 3: // As: equatorial, summer dry
        monsoon = 0.5 + 0.5*atan(cos(SGD_2PI*winter/12.0));
        _precipitation = monsoon*monsoon_precipitation;
        _wind = 2.0*monsoon*_wind;
        break;
    case 4: // Aw: equatorial, winter dry
        monsoon = 0.5 - 0.5*atan(cos(SGD_2PI*winter/12.0));
        _precipitation = monsoon*monsoon_precipitation;
        _wind = 2.0*monsoon*_wind;
        break;
    default:
        break;
    }
}

// https://en.wikipedia.org/wiki/Desert_climate
// https://en.wikipedia.org/wiki/Semi-arid_climate
void FGClimate::set_dry()
{
    double day = _day_noon;
    double night = 1.0 - day;

    double summer = _season_summer;
    double winter = 1.0 - summer;

    double temp_avg_summer = 32.0;
    double temp_day_summer  = 41.0;
    double temp_night_summer = 2.0*temp_avg_summer - temp_day_summer;
    double temp_day_winter = temp_day_summer - temp_night_summer;
    double temp_night_winter = 0.0;

    _temperature_gl = winter*(day*temp_day_winter + night*temp_night_winter)
                    + summer*(day*temp_day_summer + night*temp_night_summer);
    if (_classicfication == 6 || _classicfication == 8) { 	// cold arid
        _temperature_mean -= 14.0;
        _temperature_gl -= 14.0;
    }
    _temperature_sl = _temperature_gl;

    double temp_mean_winter = 0.5*(temp_day_winter + temp_night_winter);
    _temperature_mean = summer*temp_avg_summer + winter*temp_mean_winter;

    // low relative humidity
    _relative_humidity = 0.25;

    // steady winds
    _wind = 3.0;

    _total_annual_precipitation = 100.0;
    if (_classicfication == 5 || _classicfication == 6) {	// steppe
        _total_annual_precipitation *= 2.0;
    }

    double avg_precipitation = _total_annual_precipitation/12.0;
    _precipitation = winter*2.0*avg_precipitation;
}

void FGClimate::set_temperate()
{
    set_ocean(); // for now

    _temperature_gl = _temperature_sl;
    switch(_classicfication)
    {
    case 9: // Cfa: warm temperature, fully humid hot summer
    case 10: // Cfb: warm temperature, fully humid, warm summer
    case 11: // Cfc: warm temperature, fully humid, cool summer
    case 12: // Csa: warm temperature, summer dry, hot summer
    case 13: // Csb: warm temperature, summer dry, warm summer
    case 14: // Csc: warm temperature, summer dry, cool summer
    case 15: // Cwa: warm temperature, winter dry, hot summer
    case 16: // Cwb: warm temperature, winter dry, warm summer
    case 17: // Cwc: warm temperature, winter dry, cool summer
    default:
        break;
    }
}

void FGClimate::set_continetal()
{
    set_ocean(); // for now

    _temperature_gl = _temperature_sl;
    switch(_classicfication)
    {
    case 18: // Dfa: snow, fully humid, hot summer
    case 19: // Dfb: snow, fully humid, warm summer, warm summer
    case 20: // Dfc: snow, fully humid, cool summer, cool summer
    case 21: // Dfd: snow, fully humid, extremely continetal
    case 22: // Dsa: snow, summer dry, hot summer
    case 23: // Dsb: snow, summer dry, warm summer
    case 24: // Dsc: snow, summer dry, cool summer
    case 25: // Dsd: snow, summer dry, extremely continetal
    case 26: // Dwa: snow, winter dry, hot summer
    case 27: // Dwb: snow, winter dry, warm summer
    case 28: // Dwc: snow, winter dry, cool summer
    case 29: // Dwd: snow, winter dry, extremely continetal
    default:
        break;
    }
}

void FGClimate::set_polar()
{
    set_ocean();

    // polar climate also occurs high in the mountains
    switch(_classicfication)
    {
    case 30: // EF: polar frost
        _temperature_gl = _temperature_sl+16.0;
        break;
    case 31: // ET: polar tundra
        _temperature_gl = _temperature_sl;
        break;
    default:
        break;
    }
}

void FGClimate::set_environment()
{
    double latitude_deg = pos.getLatitudeDeg();
    double lat_fact = pow(fabs(latitude_deg)/90.0, 2.5);
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

    _snow_level = (7500.0 - 8000.0*lat_fact)*(1.0 - snow_fact);
    _snow_thickness = pow(snow_fact, 2.0);
    _ice_cover = pow(lat_fact, 2.5);

    if (_precipitation < 20.0 && _total_annual_precipitation < 240.0)
    {
        _dust_cover = 0.3 - 0.3*sqrtf(_precipitation/20.0);
        _lichen_cover = 0.0;
        _wetness = 0.0;
    }
    else
    {
        double wetness = _precipitation - 20.0;
        wetness = 12.0*wetness/_total_annual_precipitation;
        _wetness = pow(wetness, 3.0);

        _lichen_cover = 0.0;
        _dust_cover = 0.0;
    }

    if (_environment_adjust)
    {
        fgSetDouble("/environment/snow-level-m", _snow_level);
        fgSetDouble("/environment/surface/snow-thickness-factor", _snow_thickness);
        fgSetDouble("/environment/sea/surface/ice-cover", _ice_cover);
        fgSetDouble("/environment/surface/dust-cover-factor", _dust_cover);
        fgSetDouble("/environment/surface/wetness-set", _wetness);
        fgSetDouble("/environment/surface/lichen-cover-factor", _lichen_cover);
        fgSetDouble("/environment/season", 2.0*_season_autumn);
    }
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
    std::cout << "  Sun Latitude:    " << _sun_latitude_deg << " deggrees"
              << std::endl;
    std::cout << "  Sun Longitude:   " << _sun_longitude_deg << " deggrees"
              << std::endl;
    std::cout << "  Viewer Latitude:  " << pos.getLatitudeDeg() << " deggrees"
              << " (adjusted: " << _adj_latitude_deg << ")" << std::endl;
    std::cout << "  Viewer Longitude: " << pos.getLongitudeDeg() << " deggrees"
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
    std::cout << "  Seal level temperature: " << _temperature_sl << " deg. C."
              << std::endl;
    std::cout << "  Ground temperature:     " << _temperature_gl << " deg. C."
              << std::endl;
    std::cout << "  Monthly Precipitation:  " << _precipitation << " mm"
              << std::endl;
    std::cout << "  Annual precipitation:   " << _total_annual_precipitation
              << " mm" << std::endl;
    std::cout << "  Dew point: " << _dew_point << " deg. C." << std::endl;
    std::cout << "  Wind: " << _wind << " km/h" << std::endl << std::endl;
    std::cout << "  Snow level: " << _snow_level << " meters" << std::endl;
    std::cout << "  Snow Thickness (0.0 = thin .. 1.0 = thick): "
              << _snow_thickness << std::endl;
    std::cout << "  Ice cover......(0.0 = none .. 1.0 = thick): " << _ice_cover
              << std::endl;
    std::cout << "  Dust cover.....(0.0 = none .. 1.0 = dusty): " << _dust_cover
              << std::endl;
    std::cout << "  Wetness........(0.0 = dry  .. 1.0 = wet):   " << _wetness
              << std::endl;
    std::cout << "  Lichen cover...(0.0 = none .. 1.0 = muddy): "
              << _lichen_cover << std::endl;
    std::cout << "  Autumn (0.0 = summer .. 1.0 = late autumn): "
              << _season_autumn << std::endl;
    std::cout << "===============================================" << std::endl;
}
#endif // REPORT_TO_CONSOLE
