// gps.cxx - distance-measuring equipment.
// Written by David Megginson, started 2003.
//
// This file is in the Public Domain and comes with no warranty.

#include <simgear/compiler.h>
#include <simgear/math/sg_geodesy.hxx>

#include <Main/fg_props.hxx>

#include "gps.hxx"


GPS::GPS ()
    : _last_valid(false),
      _last_longitude_deg(0),
      _last_latitude_deg(0),
      _last_altitude_m(0)
{
}

GPS::~GPS ()
{
}

void
GPS::init ()
{
    _longitude_node = fgGetNode("/position/longitude-deg", true);
    _latitude_node = fgGetNode("/position/latitude-deg", true);
    _altitude_node = fgGetNode("/position/altitude-ft", true);
    _magvar_node = fgGetNode("/environment/magnetic-variation-deg", true);
    _serviceable_node = fgGetNode("/instrumentation/gps/serviceable", true);
    _electrical_node = fgGetNode("/systems/electrical/outputs/gps", true);

    _raim_node = fgGetNode("/instrumentation/gps/raim", true);
    _indicated_longitude_node =
        fgGetNode("/instrumentation/gps/indicated-longitude-deg", true);
    _indicated_latitude_node =
        fgGetNode("/instrumentation/gps/indicated-latitude-deg", true);
    _indicated_altitude_node =
        fgGetNode("/instrumentation/gps/indicated-altitude-ft", true);
    _true_track_node =
        fgGetNode("/instrumentation/gps/indicated-track-true-deg", true);
    _magnetic_track_node =
        fgGetNode("/instrumentation/gps/indicated-track-magnetic-deg", true);
    _speed_node =
        fgGetNode("/instrumentation/gps/indicated-ground-speed-kt", true);
}

void
GPS::update (double delta_time_sec)
{
                                // If it's off, don't bother.
    if (!_serviceable_node->getBoolValue() ||
        !_electrical_node->getBoolValue()) {
        _last_valid = false;
        _last_longitude_deg = 0;
        _last_latitude_deg = 0;
        _last_altitude_m = 0;
        _raim_node->setDoubleValue(false);
        _indicated_longitude_node->setDoubleValue(0);
        _indicated_latitude_node->setDoubleValue(0);
        _indicated_altitude_node->setDoubleValue(0);
        _true_track_node->setDoubleValue(0);
        _magnetic_track_node->setDoubleValue(0);
        _speed_node->setDoubleValue(0);
        return;
    }

                                // Get the aircraft position
    double longitude_deg = _longitude_node->getDoubleValue();
    double latitude_deg = _latitude_node->getDoubleValue();
    double altitude_m = _altitude_node->getDoubleValue() * SG_FEET_TO_METER;
    double magvar_deg = _magvar_node->getDoubleValue();

    _raim_node->setBoolValue(true);
    _indicated_longitude_node->setDoubleValue(longitude_deg);
    _indicated_latitude_node->setDoubleValue(latitude_deg);
    _indicated_altitude_node->setDoubleValue(altitude_m * SG_METER_TO_FEET);

    if (_last_valid) {
        double track1_deg, track2_deg, distance_m;
        geo_inverse_wgs_84(altitude_m,
                           _last_latitude_deg, _last_longitude_deg,
                           latitude_deg, longitude_deg,
                           &track1_deg, &track2_deg, &distance_m);
        double distance_nm = distance_m * SG_METER_TO_NM;
        double speed_kt = ((distance_m * SG_METER_TO_NM) *
                           ((1 / delta_time_sec) * 3600.0));
        _true_track_node->setDoubleValue(track1_deg);
        _magnetic_track_node->setDoubleValue(track1_deg - magvar_deg);
        _speed_node->setDoubleValue(speed_kt);
    } else {
        _true_track_node->setDoubleValue(0);
        _magnetic_track_node->setDoubleValue(0);
        _speed_node->setDoubleValue(0);
    }

    _last_valid = true;
    _last_longitude_deg = longitude_deg;
    _last_latitude_deg = latitude_deg;
    _last_altitude_m = altitude_m;
}

// end of gps.cxx
