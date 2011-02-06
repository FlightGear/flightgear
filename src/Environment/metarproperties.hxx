// metarproperties.hxx -- Parse a METAR and write properties
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

#ifndef __METARPROPERTIES_HXX
#define __METARPROPERTIES_HXX

#include <Airports/simple.hxx>
#include <simgear/props/props.hxx>
#include <simgear/props/tiedpropertylist.hxx>

namespace Environment {

class MagneticVariation;

class MetarProperties : public SGReferenced
{
public:
    MetarProperties( SGPropertyNode_ptr rootNode );
    virtual ~MetarProperties();

    SGPropertyNode_ptr get_root_node() const { return _rootNode; }
    virtual bool isValid() const { return _metarValidNode->getBoolValue(); }
    virtual const std::string & getStationId() const { return _station_id; }
    virtual void setStationId( const std::string & value );
    virtual void setMetar( const std::string & value ) { set_metar( value.c_str() ); }

private:
    const char * get_metar() const { return _metar.c_str(); }
    void set_metar( const char * metar );
    const char * get_station_id() const { return _station_id.c_str(); }
    void set_station_id( const char * value );
    const char * get_decoded() const { return _decoded.c_str(); }
    double get_magnetic_variation_deg() const;
    double get_magnetic_dip_deg() const;
    double get_wind_from_north_fps() const { return _wind_from_north_fps; }
    double get_wind_from_east_fps() const { return _wind_from_east_fps; }
    double get_base_wind_dir() const { return _base_wind_dir; }
    double get_wind_speed() const { return _wind_speed; }
    void set_wind_from_north_fps( double value );
    void set_wind_from_east_fps( double value );
    void set_base_wind_dir( double value );
    void set_wind_speed( double value );

    SGPropertyNode_ptr _rootNode;
    SGPropertyNode_ptr _metarValidNode;
    std::string _metar;
    std::string _station_id;
    double _station_elevation;
    double _station_latitude;
    double _station_longitude;
    double _min_visibility;
    double _max_visibility;
    int _base_wind_dir;
    int _base_wind_range_from;
    int _base_wind_range_to;
    double _wind_speed;
    double _wind_from_north_fps;
    double _wind_from_east_fps;
    double _gusts;
    double _temperature;
    double _dewpoint;
    double _humidity;
    double _pressure;
    double _sea_level_temperature;
    double _sea_level_dewpoint;
    double _sea_level_pressure;
    double _rain;
    double _hail;
    double _snow;
    bool _snow_cover;
    std::string _decoded;
protected:
    simgear::TiedPropertyList _tiedProperties;
    MagneticVariation * _magneticVariation;
};

inline void MetarProperties::set_station_id( const char * value )
{ 
    _station_id = value; 
}

} // namespace
#endif // __METARPROPERTIES_HXX
