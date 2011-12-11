// adf.cxx - distance-measuring equipment.
// Written by David Megginson, started 2003.
//
// This file is in the Public Domain and comes with no warranty.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/math/sg_random.h>
#include <simgear/timing/sg_time.hxx>

#include <Main/fg_props.hxx>
#include <Main/util.hxx>
#include <Navaids/navlist.hxx>

#include "adf.hxx"
#include <Sound/morse.hxx>


#include <iostream>
#include <string>
#include <sstream>


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


ADF::ADF (SGPropertyNode *node )
    :
    _name(node->getStringValue("name", "adf")),
    _num(node->getIntValue("number", 0)),
    _time_before_search_sec(0),
    _last_frequency_khz(-1),
    _transmitter_valid(false),
    _transmitter_pos(SGGeod::fromDeg(0, 0)),
    _transmitter_cart(0, 0, 0),
    _transmitter_range_nm(0),
    _ident_count(0),
    _last_ident_time(0),
    _last_volume(-1),
    _sgr(0)
{
}

ADF::~ADF ()
{
}

void
ADF::init ()
{
    string branch;
    branch = "/instrumentation/" + _name;

    SGPropertyNode *node = fgGetNode(branch.c_str(), _num, true );
    _longitude_node = fgGetNode("/position/longitude-deg", true);
    _latitude_node = fgGetNode("/position/latitude-deg", true);
    _altitude_node = fgGetNode("/position/altitude-ft", true);
    _heading_node = fgGetNode("/orientation/heading-deg", true);
    _serviceable_node = node->getChild("serviceable", 0, true);
    _error_node = node->getChild("error-deg", 0, true);
    _electrical_node = fgGetNode("/systems/electrical/outputs/adf", true);
    branch = branch + "/frequencies";
    SGPropertyNode *fnode = node->getChild("frequencies", 0, true);
    _frequency_node = fnode->getChild("selected-khz", 0, true);
    _mode_node = node->getChild("mode", 0, true);
    _volume_node = node->getChild("volume-norm", 0, true);
    _in_range_node = node->getChild("in-range", 0, true);
    _bearing_node = node->getChild("indicated-bearing-deg", 0, true);
    _ident_node = node->getChild("ident", 0, true);
    _ident_audible_node = node->getChild("ident-audible", 0, true);
    _power_btn_node = node->getChild("power-btn", 0, true);

    if (_power_btn_node->getType() == simgear::props::NONE) 
      _power_btn_node->setBoolValue(true); // front end didn't implement a power button

    SGSoundMgr *smgr = globals->get_soundmgr();
    _sgr = smgr->find("avionics", true);
    _sgr->tie_to_listener();

    std::ostringstream temp;
    temp << _name << _num;
    _adf_ident = temp.str();
}

void
ADF::update (double delta_time_sec)
{
                                // If it's off, don't waste any time.
    if (_electrical_node->getDoubleValue() < 8.0
            || !_serviceable_node->getBoolValue()
            || !_power_btn_node->getBoolValue()     ) {
        _in_range_node->setBoolValue(false);
        _ident_node->setStringValue("");
        return;
    }

    string mode = _mode_node->getStringValue();
    if (mode == "ant" || mode == "test") set_bearing(delta_time_sec, 90);
    if (mode != "bfo" && mode != "adf") {
        _in_range_node->setBoolValue(false);
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

    if (!_transmitter_valid) {
        _in_range_node->setBoolValue(false);
        _ident_node->setStringValue("");
        return;
    }

                                // Calculate the bearing to the transmitter
    SGGeod geod = SGGeod::fromRadM(longitude_rad, latitude_rad, altitude_m);
    SGVec3d location = SGVec3d::fromGeod(geod);
    
    double distance_nm = dist(_transmitter_cart, location) * SG_METER_TO_NM;
    double range_nm = adjust_range(_transmitter_pos.getElevationFt(),
                                   altitude_m * SG_METER_TO_FEET,
                                   _transmitter_range_nm);

    if (distance_nm <= range_nm) {

        double bearing, az2, s;
        double heading = _heading_node->getDoubleValue();

        geo_inverse_wgs_84(geod, _transmitter_pos,
                           &bearing, &az2, &s);
        _in_range_node->setBoolValue(true);

        bearing -= heading;
        if (bearing < 0)
            bearing += 360;
        set_bearing(delta_time_sec, bearing);

        // adf ident sound
        float volume;
        if ( _ident_audible_node->getBoolValue() )
            volume = _volume_node->getFloatValue();
        else
            volume = 0.0;

        if ( volume != _last_volume ) {
            _last_volume = volume;

            SGSoundSample *sound;
            sound = _sgr->find( _adf_ident );
            if ( sound != NULL )
                sound->set_volume( volume );
            else
                SG_LOG( SG_INSTR, SG_ALERT, "Can't find adf-ident sound" );
        }

        time_t cur_time = globals->get_time_params()->get_cur_time();
        if ( _last_ident_time < cur_time - 30 ) {
            _last_ident_time = cur_time;
            _ident_count = 0;
        }

        if ( _ident_count < 4 ) {
            if ( !_sgr->is_playing(_adf_ident) && (volume > 0.05) ) {
                _sgr->play_once( _adf_ident );
                ++_ident_count;
            }
        }
    } else {
        _in_range_node->setBoolValue(false);
        _ident_node->setStringValue("");
        _sgr->stop( _adf_ident );
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
    FGNavRecord *nav = globals->get_navlist()->findByFreq(frequency_khz,
      SGGeod::fromRadM(longitude_rad, latitude_rad, altitude_m));

    _transmitter_valid = (nav != NULL);
    if ( _transmitter_valid ) {
        ident = nav->get_trans_ident();
        if ( ident != _last_ident ) {
            _transmitter_pos = nav->geod();
            _transmitter_cart = nav->cart();
            _transmitter_range_nm = nav->get_range();
        }
    }

    if ( _last_ident != ident ) {
        _last_ident = ident;
        _ident_node->setStringValue(ident.c_str());

        if ( _sgr->exists( _adf_ident ) ) {
	    // stop is required! -- remove alone wouldn't stop immediately
            _sgr->stop( _adf_ident );
            _sgr->remove( _adf_ident );
        }

        SGSoundSample *sound;
        sound = FGMorse::instance()->make_ident( ident, FGMorse::LO_FREQUENCY );
        sound->set_volume(_last_volume = 0);
        _sgr->add( sound, _adf_ident );

        int offset = (int)(sg_random() * 30.0);
        _ident_count = offset / 4;
        _last_ident_time = globals->get_time_params()->get_cur_time() -
            offset;
    }
}

void
ADF::set_bearing (double dt, double bearing_deg)
{
    double old_bearing_deg = _bearing_node->getDoubleValue();

    while ((bearing_deg - old_bearing_deg) >= 180)
        old_bearing_deg += 360;
    while ((bearing_deg - old_bearing_deg) <= -180)
        old_bearing_deg -= 360;
    bearing_deg += _error_node->getDoubleValue();
    bearing_deg =
        fgGetLowPass(old_bearing_deg, bearing_deg, dt * RESPONSIVENESS);

    _bearing_node->setDoubleValue(bearing_deg);
}


// end of adf.cxx
