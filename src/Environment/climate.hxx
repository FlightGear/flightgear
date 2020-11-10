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

    double get_dewpoint_degc() { return _dewpoint_gl; }
    double get_temperature_degc() { return _temperature_gl; }
    double get_humidity_pct() { return _relative_humidity_gl; }
    double get_wind_kmh() { return _wind; }

private:
    static const std::string _classification[MAX_CLIMATE_CLASSES];
    static const std::string _description[MAX_CLIMATE_CLASSES];

#if REPORT_TO_CONSOLE
    void report();
#endif
    inline void _set(double& prev, double val) {
        prev = (prev < -1000.0) ? val : 0.95*prev + 0.05*val;
    }

    // interpolate val (from 0.0 to 1.0) between min and max
    double linear(double val, double min, double max);
    double triangular(double val, double min, double max);
    double season_short(double val, double min, double max);
    double season_even(double val, double min, double max);
    double season_long_low(double val, double min, double max);
    double season_long_high(double val, double min, double max);
    double monsoonal(double val, double min, double max);

    void set_ocean();
    void set_dry();
    void set_tropical();
    void set_temperate();
    void set_continetal();
    void set_polar();
    void set_environment();

    void update_day_factor();
    void update_season_factor();

    SGPropertyNode_ptr _rootNode;
    simgear::TiedPropertyList _tiedProperties;

    SGPropertyNode_ptr _metarSnowLevelNode;
    SGPropertyNode_ptr _positionLatitudeNode;
    SGPropertyNode_ptr _positionLongitudeNode;
    SGPropertyNode_ptr _ground_elev_node;

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

    double _day_noon = 1.0;
    double _season_summer = 1.0;
    double _season_transistional = 0.0;
    bool _has_autumn = false;
    bool _is_autumn = false;

    int _code = 0;			// Köppen-Geiger classicfication

    // environment
    bool _environment_adjust = false;	// enable automatic adjestments
    double _snow_level = 7500.0;	// in meters
    double _snow_thickness = 0.0;	// 0.0 = thin, 1.0 = thick
    double _ice_cover = 0.0;		// 0.0 = none, 1.0 = thick
    double _dust_cover = 0.0;		// 0.0 = none, 1.0 = dusty
    double _wetness = 0.0;		// 0.0 = dry, 1.0 = wet
    double _lichen_cover = 0.0;		// 0.0 = none, 1.0 = mossy

    // weather
    bool _weather_update = false;	// enable weather updates
    double _relative_humidity_sl = -99999.0;// 0.0 = dry, 1.0 is fully humid
    double _relative_humidity_gl = -99999.0;

    double _dewpoint_gl = -99999.0;
    double _dewpoint_sl = -99999.0;
    double _temperature_gl = -99999.0;	// ground level temperature in deg. C.
    double _temperature_sl = -99999.0;	// seal level temperature in deg. C.
    double _temperature_mean_gl = -99999.0; // mean temperature in deg. C.
    double _temperature_mean_sl = -99999.0;
    double _precipitation = -99999.0; // minimal avg. precipitation in mm/month
    double _wind = -99999.0;		// wind in km/h
    double _precipitation_annual = -99999.0; // global
};

#endif // _FGCLIMATE_HXX
