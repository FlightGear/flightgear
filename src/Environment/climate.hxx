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

#ifndef _FGCLIMATE_HXX
#define _FGCLIMATE_HXX

#include <osg/ref_ptr>
#include <osg/Image>

// #include <simgear/structure/SGReferenced.hxx>
#include <simgear/math/SGGeod.hxx>

#define REPORT_TO_CONSOLE	false

/*
 * Update environment parameters based on the Köppen-Geiger climate
 * map of the world based on lattitude and longitude.
 */

class FGLight;

class FGClimate {
public:
    FGClimate(const SGGeod& position);
    virtual ~FGClimate() = default;

    void update(const SGGeod& position);

    double get_snow_level_m() { return _snow_level; }
    double get_snow_thickness() { return _snow_thickness; }
    double get_ice_cover() { return _ice_cover; }
    double get_dust_cover() { return _dust_cover; }
    double get_wetness() { return _wetness; }
    double get_lichen_cover() { return _lichen_cover; }

    double get_dew_point_degc() { return _dew_point; }
    double get_temperature_degc() { return _temperature_gl; }
    double get_humidity_pct() { return _relative_humidity; }
    double get_wind_kmh() { return _wind; }

private:
#if REPORT_TO_CONSOLE
    void report();
#endif

    void set_ocean();
    void set_dry();
    void set_tropical();
    void set_temperate();
    void set_continetal();
    void set_polar();
    void set_environment();

    void update_day_factor();
    void update_season_factor();

    osg::ref_ptr<osg::Image> image;
    int _image_width = 0;
    int _image_height = 0;

    double _epsilon = 1.0;
    double _prev_lat = -99999.0;
    double _prev_lon = -99999.0;

    SGGeod pos;

    double _sun_latitude_deg = 0.0;
    double _sun_longitude_deg = 0.0;

    double _adj_latitude_deg = 0.0;	// viewer lat adjusted for sun lat
    double _adj_longitude_deg = 0.0;	// viewer lat adjusted for sun lon

    double _day_noon = 1.0;
    double _season_summer = 1.0;
    double _season_winter = 0.0;
    bool _autumn = false;

    int _col = 0;			// screen coordinates
    int _row = 0;
    int _classicfication = 0;		// Köppen-Geiger classicfication
 
    // environment
    bool _environment_adjust = true;	// enable automatic adjestments
    double _snow_level = 7500.0;	// in meters
    double _snow_thickness = 0.0;	// 0.0 = thin, 1.0 = thick
    double _ice_cover = 0.0;		// 0.0 = none, 1.0 = thick
    double _dust_cover = 0.0;		// 0.0 = none, 1.0 = dusty
    double _wetness = 0.0;		// 0.0 = dry, 1.0 = wet
    double _lichen_cover = 0.0;		// 0.0 = none, 1.0 = mossy

    // weather
    bool _weather_update = false;	// enable weather updates
    double _relative_humidity = 0.6;	// 0.0 = dry, 1.0 is fully humid

    double _dew_point = 0.5;
    double _temperature_gl = 20.0;	// ground level temperature in deg. C.
    double _temperature_sl = 20.0;	// seal level temperature in deg. C.
    double _temperature_mean = 20.0;	// mean temperature in deg. C.
    double _precipitation = 0.0; // minimal average precipitation in mm/month
    double _wind = 3.0;			// wind in km/h

    // total annual precipitation in mm/month
    double _total_annual_precipitation = 990.0; // global
};

#endif // _FGCLIMATE_HXX
