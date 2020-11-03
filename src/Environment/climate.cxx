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
#include <simgear/constants.h>

#include "Main/fg_props.hxx"
#include "Time/light.hxx"

#include "climate.hxx"

// Based on "World Map of the Köppen-Geiger climate classification"
// The map is provided with a spatial resolution of 0.5 degree (3km).
// http://koeppen-geiger.vu-wien.ac.at/present.htm

FGClimate::FGClimate(double lat, double lon)
{
    SGPath image_path( fgGetString("/sim/fg-root") );
    image_path.append("Geodata/koppen-geiger.png");

    image = osgDB::readImageFile(image_path.str());
    _image_width = image->s();
    _image_height = image->t();

    _epsilon = _image_width/360.0;
    update(lat, lon);
}

// Set all environment parameters based on the koppen-classicfication
// https://en.wikipedia.org/wiki/K%C3%B6ppen_climate_classification
void FGClimate::update(double lat, double lon)
{
    _latitude_deg  = lat*SGD_RADIANS_TO_DEGREES;
    _longitude_deg = lon*SGD_RADIANS_TO_DEGREES;

    if (fabs(_prev_lat - _latitude_deg) > _epsilon ||
        fabs(_prev_lon - _longitude_deg) > _epsilon)
    {
        _prev_lat = _latitude_deg;
        _prev_lon = _longitude_deg;

        light = static_cast<FGLight*>(globals->get_subsystem("lighting"));
        update_day_factor();
        update_season_factor();
 
        // from lat/lon to screen coordinates
        double s = 180.0 + _longitude_deg;
        double t =  90.0 + _latitude_deg;

        _col = static_cast<int>(s * _image_width/360.0);
        _row = static_cast<int>(t * _image_height/180.0);

        osg::Vec4f color = image->getColor(_col, _row);

        // convert from color shades to koppen-classicfication
        _classicfication = static_cast<int>( 255.0*color[0]/4.0 );

        if (_classicfication == 0) set_ocean();
        else if (_classicfication < 5) set_tropical();
        else if (_classicfication < 9) set_dry();
        else if (_classicfication < 18) set_temperate();
        else if (_classicfication < 30) set_continetal();
        else if (_classicfication < 32)set_polar();
        else set_ocean();

        set_environment();
    }
}

// returns 0.0 for night up to 1.0 for noon
void FGClimate::update_day_factor()
{
    double lon = light->get_sun_lon()*SGD_RADIANS_TO_DEGREES - _longitude_deg;

    if (lon > 360.0) lon -= 360.0;
    else if (lon < 0.0) lon += 360.0;

    // noon is when lon == 180.0
    _day_noon = 1.0 - fabs(lon - 180.0)/180.0;
}

// The seasons are currently based purely on where the sun's perpendicular
// (perp.) rays strike the earth. Winter (perp. Rays moving from 23.5 S to
// equator), Spring (perp. Rays moving from the equator to 23.5 N), Summer
// (perp. Rays moving from 23.5 N to equator) and Fall (perp. Rays moving
// from equator to 23.5 S).
// returns 0.0 for winter up to 1.0 for summer
void FGClimate::update_season_factor()
{
  double sign = (_latitude_deg >= 0.0) ? 1.0 : -1.0;
  double sun_lat = light->get_sun_lat()*SGD_RADIANS_TO_DEGREES;

   _season_summer = 0.5 + sign*sun_lat/(2.0*23.5);
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
 
    double fact_lat = fabs(_latitude_deg)/90.0;
    double ifact_lat = 1.0 - fact_lat;

    _temperature = ifact_lat*temp_equator + fact_lat*temp_pole;

    // relative humidity based on latitude
    _relative_humidity = 0.8 - 0.7*fact_lat;

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
    _temperature = 0.75*temp_equator + 0.25*summer*temp_equator;

    // relative humidity based on latitude
    double fact_lat = fabs(_latitude_deg)/90.0;
    _relative_humidity = 0.8 - 0.7*fact_lat;

    // wind based on latitude (0.0 - 15 degrees)
    fact_lat = std::max(abs(_latitude_deg), 15.0)/15.0;
    _wind = 3.0*fact_lat*fact_lat;

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
        monsoon = 0.5 + 0.5*cos(SGD_2PI*winter/12.0);
        _precipitation = monsoon*monsoon_precipitation;
        _wind = 2.0*monsoon*_wind;
        break;
    case 4: // Aw: equatorial, winter dry
        monsoon = 0.5 - 0.5*cos(SGD_2PI*winter/12.0);
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

    _temperature = winter*(day*temp_day_winter + night*temp_night_winter)
                 + summer*(day*temp_day_summer + night*temp_night_summer);
    if (_classicfication == 6 || _classicfication == 8) { 	// cold arid
        _temperature -= 14.0;
    }

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
    switch(_classicfication)
    {
    case 30: // EF: polar frost
        _temperature += 16.0;
        break;
    case 31: // ET: polar tundra
    default:
        break;
    }
}

void FGClimate::set_environment()
{
    double fact = fabs(_latitude_deg)/90.0;
    double frost = _temperature > 0.0 ? 0.0 : fact*fact*fact;
    _snow_level = 7500.0 - 8000*fact;
    _snow_thickness = fact*fact;
    _ice_cover = 0.3*frost;

    double precipitation = 12.0*_precipitation/_total_annual_precipitation;
    _dust_cover = 1.0 - sqrtf(precipitation);
    _wetness = precipitation*precipitation*precipitation;
    _lichen_cover = 0.0;
}
