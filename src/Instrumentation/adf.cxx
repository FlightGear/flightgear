// adf.cxx - distance-measuring equipment.
// Written by David Megginson, started 2003.
//
// This file is in the Public Domain and comes with no warranty.

#include <simgear/compiler.h>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/math/sg_random.h>

#include <Main/fg_props.hxx>
#include <Main/util.hxx>
#include <Navaids/navlist.hxx>

#include "adf.hxx"


// Use a bigger number to be more responsive, or a smaller number
// to be more sluggish.
#define RESPONSIVENESS 0.5


/**
 * Fiddle with the reception range a bit.
 *
 * TODO: better reception at night (??).
 */
static double
adjust_range (double transmitter_elevation_ft, double aircraft_altitude_ft,
              double max_range_nm)
{
    double delta_elevation_ft =
        aircraft_altitude_ft - transmitter_elevation_ft;
    double range_nm = max_range_nm;

                                // kludge slightly better reception at
                                // altitude
    if (delta_elevation_ft < 0)
        delta_elevation_ft = 200;
    if (delta_elevation_ft <= 1000)
        range_nm *= sqrt(delta_elevation_ft / 1000);
    else if (delta_elevation_ft >= 5000)
        range_nm *= sqrt(delta_elevation_ft / 5000);
    if (range_nm >= max_range_nm * 3)
        range_nm = max_range_nm * 3;

    double rand = sg_random();
    return range_nm + (range_nm * rand * rand);
}


ADF::ADF ()
    : _time_before_search_sec(0),
      _last_frequency_khz(-1),
      _transmitter_valid(false),
      _transmitter_elevation_ft(0),
      _transmitter_range_nm(0)
{
}

ADF::~ADF ()
{
}

void
ADF::init ()
{
    _longitude_node = fgGetNode("/position/longitude-deg", true);
    _latitude_node = fgGetNode("/position/latitude-deg", true);
    _altitude_node = fgGetNode("/position/altitude-ft", true);
    _heading_node = fgGetNode("/orientation/heading-deg", true);
    _serviceable_node = fgGetNode("/instrumentation/adf/serviceable", true);
    _error_node = fgGetNode("/instrumentation/adf/error-deg", true);
    _electrical_node = fgGetNode("/systems/electrical/outputs/adf", true);
    _frequency_node =
        fgGetNode("/instrumentation/adf/frequencies/selected-khz", true);
    _mode_node = fgGetNode("/instrumentation/adf/mode", true);

    _in_range_node = fgGetNode("/instrumentation/adf/in-range", true);
    _bearing_node =
        fgGetNode("/instrumentation/adf/indicated-bearing-deg", true);
    _ident_node = fgGetNode("/instrumentation/adf/ident", true);
}

void
ADF::update (double delta_time_sec)
{
                                // If it's off, don't waste any time.
    if (!_electrical_node->getBoolValue() ||
        !_serviceable_node->getBoolValue()) {
        set_bearing(delta_time_sec, 90);
        _ident_node->setStringValue("");
        return;
    }

                                // Get the frequency
    int frequency_khz = _frequency_node->getIntValue();
    if (frequency_khz != _last_frequency_khz) {
        _time_before_search_sec = 0;
        _last_frequency_khz = frequency_khz;
    }

                                // Get the aircraft position
    double longitude_deg = _longitude_node->getDoubleValue();
    double latitude_deg = _latitude_node->getDoubleValue();
    double altitude_m = _altitude_node->getDoubleValue();

    double longitude_rad = longitude_deg * SGD_DEGREES_TO_RADIANS;
    double latitude_rad = latitude_deg * SGD_DEGREES_TO_RADIANS;

                                // On timeout, scan again
    _time_before_search_sec -= delta_time_sec;
    if (_time_before_search_sec < 0)
        search(frequency_khz, longitude_rad, latitude_rad, altitude_m);

                                // If it's off, don't bother.
    string mode = _mode_node->getStringValue();
    if (!_transmitter_valid || (mode != "bfo" && mode != "adf"))
    {
        set_bearing(delta_time_sec, 90);
        _ident_node->setStringValue("");
        return;
    }

                                // Calculate the bearing to the transmitter
    Point3D location =
        sgGeodToCart(Point3D(longitude_rad, latitude_rad, altitude_m));

    double distance_nm = _transmitter.distance3D(location) * SG_METER_TO_NM;
    double range_nm = adjust_range(_transmitter_elevation_ft,
                                   altitude_m * SG_METER_TO_FEET,
                                   _transmitter_range_nm);
    if (distance_nm <= range_nm) {

        double bearing, az2, s;
        double heading = _heading_node->getDoubleValue();

        geo_inverse_wgs_84(altitude_m,
                           latitude_deg,
                           longitude_deg,
                           _transmitter_lat_deg,
                           _transmitter_lon_deg,
                           &bearing, &az2, &s);
        _in_range_node->setBoolValue(true);

        bearing -= heading;
        if (bearing < 0)
            bearing += 360;
        set_bearing(delta_time_sec, bearing);
    } else {
        _in_range_node->setBoolValue(false);
        set_bearing(delta_time_sec, 90);
        _ident_node->setStringValue("");
    }
}

void
ADF::search (double frequency_khz, double longitude_rad,
             double latitude_rad, double altitude_m)
{
    string ident = "";

                                // reset search time
    _time_before_search_sec = 1.0;

                                // try the ILS list first
    FGNav * nav =
        current_navlist->findByFreq(frequency_khz, longitude_rad,
                                    latitude_rad, altitude_m);

    if (nav !=0) {
        _transmitter_valid = true;
        ident = nav->get_trans_ident();
        if (ident !=_last_ident) {
            _transmitter_lon_deg = nav->get_lon();
            _transmitter_lat_deg = nav->get_lat();
            _transmitter = Point3D(nav->get_x(), nav->get_y(), nav->get_z());
            _transmitter_elevation_ft = nav->get_elev_ft();
            _transmitter_range_nm = nav->get_range();
        }
    } else {
        _transmitter_valid = false;
    }
    _last_ident = ident;
    _ident_node->setStringValue(ident.c_str());
}

void
ADF::set_bearing (double dt, double bearing_deg)
{
    double old_bearing_deg = _bearing_node->getDoubleValue();

    bearing_deg += _error_node->getDoubleValue();

    while ((bearing_deg - old_bearing_deg) >= 180)
        old_bearing_deg += 360;
    while ((bearing_deg - old_bearing_deg) <= -180)
        old_bearing_deg -= 360;

    bearing_deg =
        fgGetLowPass(old_bearing_deg, bearing_deg, dt * RESPONSIVENESS);

    _bearing_node->setDoubleValue(bearing_deg);
}


// end of adf.cxx
