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

#include <simgear/props/tiedpropertylist.hxx>
#include <simgear/math/SGGeod.hxx>

#define REPORT_TO_CONSOLE	0

/*
 * Update environment parameters based on the Köppen-Geiger climate
 * map of the world based on lattitude and longitude.
 */

class FGLight;

#define MAX_CLIMATE_CLASSES	32

class FGClimate : public SGSubsystem {
private:
    struct _ground_tile {
        SGGeod pos;

        double elevation_m = 0.0;
        double temperature = -99999.0;
        double temperature_mean = -99999.0;
        double temperature_water = -99999.0;
        double relative_humidity = -99999.0;
        double precipitation_annual = -99999.0;
        double precipitation = -99999.0;
        bool has_autumn = false;

        double dewpoint = -99999.0;
        double pressure = 0.0;

    } _ground_tile;
    using ClimateTile = struct _ground_tile;

public:
    FGClimate();
    virtual ~FGClimate() = default;

    void bind() override;
    void init() override;
    void reinit() override;
    void unbind() override;
    void update(double dt) override;;

    double get_snow_level_m() { return _snow_level; }
    double get_snow_thickness() { return _snow_thickness; }
    double get_ice_cover() { return _ice_cover; }
    double get_dust_cover() { return _dust_cover; }
    double get_wetness() { return _wetness; }
    double get_lichen_cover() { return _lichen_cover; }

    double get_relative_humidity_pct() { return _gl.relative_humidity; }
    double get_relative_humidity_sea_level_pct() { return _sl.relative_humidity; }
    double get_pressure_hpa() { return _gl.pressure; }
    double get_pressure_sea_leevel_hpa() { return _sl.pressure; }
    double get_dewpoint_degc() { return _gl.dewpoint; }
    double get_dewpoint_sl_degc() { return _sl.dewpoint; }
    double get_temperature_degc() { return _gl.temperature; }
    double get_temperature_sea_leevel_degc() { return _sl.temperature; }
    double get_temperature_mean_degc() { return _gl.temperature_mean; }
    double get_temperature_mean_sea_level_degc() { return _sl.temperature_mean; }
    double get_temperature_water_degc() { return _gl.temperature_water; }
    double get_temperature_seawater_degc() { return _sl.temperature_water; }
    double get_precipitation_month() { return _gl.precipitation; }
    double get_precipitation_annual() { return _gl.precipitation_annual; }

    double get_wind_mps() { return _wind_speed; }
    double get_wind_direction_deg() { return _wind_direction; }

    bool getEnvironmentUpdate() const { return _environment_adjust; }
    void setEnvironmentUpdate(bool value);

    const char* get_metar() const;

    void test();
private:
    static const std::string _classification[MAX_CLIMATE_CLASSES];
    static const std::string _description[MAX_CLIMATE_CLASSES];

#if REPORT_TO_CONSOLE
    void report();
#endif
    inline void _set(double& prev, double val) {
        prev = (prev < -1000.0) ? val : 0.99*prev + 0.01*val;
    }

    // interpolate val (from 0.0 to 1.0) between min and max
    double daytime(double val, double offset = 0.0);
    double season(double val, double offset = 0.0);

    double linear(double val, double min, double max);
    double triangular(double val, double min, double max);
    double sinusoidal(double val, double min, double max);
    double even(double val, double min, double max);
    double long_low(double val, double min, double max);
    double long_high(double val, double min, double max);
    double monsoonal(double val, double min, double max);

    void set_ocean();
    void set_dry();
    void set_tropical();
    void set_temperate();
    void set_continetal();
    void set_polar();
    void set_environment();

    void update_daylight();
    void update_day_factor();
    void update_season_factor();
    void update_pressure();
    void update_wind();

    SGPropertyNode_ptr _rootNode;
    simgear::TiedPropertyList _tiedProperties;

    SGPropertyNode_ptr _monthNode;
    SGPropertyNode_ptr _gravityNode;
    SGPropertyNode_ptr _metarSnowLevelNode;
    SGPropertyNode_ptr _positionLatitudeNode;
    SGPropertyNode_ptr _positionLongitudeNode;

    osg::ref_ptr<osg::Image> image;
    double _image_width = 0;
    double _image_height = 0;

    double _epsilon = 1.0;
    double _prev_lat = -99999.0;
    double _prev_lon = -99999.0;

    double _sun_latitude_deg = 0.0;
    double _sun_longitude_deg = 0.0;

    double _adj_latitude_deg = 0.0;	// viewer lat adjusted for sun lat
    double _adj_longitude_deg = 0.0;	// viewer lat adjusted for sun lon

    double _daytime = 0.0;
    double _day_noon = 1.0;
    double _day_light = 1.0;
    double _season_summer = 1.0;
    double _season_transistional = 0.0;
    double _seasons_year = 0.0;
    double _is_autumn = -99999.0;

    // Köppen-Geiger classicfications
    ClimateTile _tiles[3][3];

    // environment
    bool _environment_adjust = false;	// enable automatic adjestments
    double _snow_level = -99999.0;	// in meters
    double _snow_thickness = -99999.0;	// 0.0 = thin, 1.0 = thick
    double _ice_cover = -99999.0;	// 0.0 = none, 1.0 = thick
    double _dust_cover = -99999.0;	// 0.0 = none, 1.0 = dusty
    double _wetness = -99999.0;		// 0.0 = dry, 1.0 = wet
    double _lichen_cover = -99999.0;	// 0.0 = none, 1.0 = mossy

    // weather
    int _code = 0;			// Köppen-Geiger classicfication
    ClimateTile _gl;			// ground level parameters

    ClimateTile _sl;			// sea level parameters
    bool _weather_update = false;	// enable weather updates
    double _wind_speed = 0.0;		// wind in meters per second
    double _wind_direction = -99999.0;	// wind direction in degrees

    char _metar[256] = "";
};

#endif // _FGCLIMATE_HXX
