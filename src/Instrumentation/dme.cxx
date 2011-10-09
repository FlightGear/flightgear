// dme.cxx - distance-measuring equipment.
// Written by David Megginson, started 2003.
//
// This file is in the Public Domain and comes with no warranty.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>
#include <simgear/sg_inlines.h>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/math/sg_random.h>

#include <Main/fg_props.hxx>
#include <Navaids/navlist.hxx>
#include <Sound/audioident.hxx>

#include "dme.hxx"

/**
 * Adjust the range.
 *
 * Start by calculating the radar horizon based on the elevation
 * difference, then clamp to the maximum, then add a fudge for
 * borderline reception.
 */
static double
adjust_range (double transmitter_elevation_ft, double aircraft_altitude_ft,
              double max_range_nm)
{
    double delta_elevation_ft =
        fabs(aircraft_altitude_ft - transmitter_elevation_ft);
    double range_nm = 1.23 * sqrt(delta_elevation_ft);
    if (range_nm > max_range_nm)
        range_nm = max_range_nm;
    else if (range_nm < 20.0)
        range_nm = 20.0;
    double rand = sg_random();
    return range_nm + (range_nm * rand * rand);
}


DME::DME ( SGPropertyNode *node )
    : _last_distance_nm(0),
      _last_frequency_mhz(-1),
      _time_before_search_sec(0),
      _navrecord(NULL),
      _name(node->getStringValue("name", "dme")),
      _num(node->getIntValue("number", 0)),
      _audioIdent(NULL)
{
}

DME::~DME ()
{
    delete _audioIdent;
}

void
DME::init ()
{
    string branch;
    branch = "/instrumentation/" + _name;

    SGPropertyNode *node = fgGetNode(branch.c_str(), _num, true );

    _serviceable_node = node->getChild("serviceable", 0, true);
    _electrical_node = fgGetNode("/systems/electrical/outputs/dme", true);
    SGPropertyNode *fnode = node->getChild("frequencies", 0, true);
    _source_node = fnode->getChild("source", 0, true);
    _frequency_node = fnode->getChild("selected-mhz", 0, true);
    _in_range_node = node->getChild("in-range", 0, true);
    _distance_node = node->getChild("indicated-distance-nm", 0, true);
    _speed_node = node->getChild("indicated-ground-speed-kt", 0, true);
    _time_node = node->getChild("indicated-time-min", 0, true);

    double d = node->getDoubleValue( "volume", 1.0 );
    _volume_node = node->getChild("volume", 0, true);
    _volume_node->setDoubleValue( d );

    bool b = node->getBoolValue( "ident", false );
    _ident_btn_node = node->getChild("ident", 0, true);
    _ident_btn_node->setBoolValue( b );

    std::ostringstream temp;
    temp << _name << "-ident-" << _num;
    if( NULL == _audioIdent ) 
        _audioIdent = new DMEAudioIdent( temp.str() );
    _audioIdent->init();
}

void
DME::update (double delta_time_sec)
{
    if( delta_time_sec < SGLimitsd::min() )
        return;  //paused

                                // Figure out the source
    const char * source = _source_node->getStringValue();
    if (source[0] == '\0') {
        string branch;
        branch = "/instrumentation/" + _name + "/frequencies/selected-mhz";
        _source_node->setStringValue(branch.c_str());
        source = _source_node->getStringValue();
    }
                                // Get the frequency

    double frequency_mhz = fgGetDouble(source, 108.0);
    if (frequency_mhz != _last_frequency_mhz) {
        _time_before_search_sec = 0;
        _last_frequency_mhz = frequency_mhz;
    }
    _frequency_node->setDoubleValue(frequency_mhz);

                                // Get the aircraft position
    // On timeout, scan again
    _time_before_search_sec -= delta_time_sec;
    if (_time_before_search_sec < 0) {
        _time_before_search_sec = 1.0;

        if( fgGetBool( "/sim/realism/dme-fallback-to-loc", true ) ) {
            if( NULL == (_navrecord = globals->get_loclist()->findByFreq( frequency_mhz,
                globals->get_aircraft_position())) ) {

                _navrecord = globals->get_dmelist()->findByFreq( frequency_mhz,
                    globals->get_aircraft_position());
            }
        } else {
            _navrecord = globals->get_dmelist()->findByFreq( frequency_mhz,
                globals->get_aircraft_position());
        }
    }

    // If it's off, don't bother.
    if (!_serviceable_node->getBoolValue() ||
        !_electrical_node->getBoolValue() ||
        NULL == _navrecord ) {
        _last_distance_nm = 0;
        _in_range_node->setBoolValue(false);
        _distance_node->setDoubleValue(0);
        _speed_node->setDoubleValue(0);
        _time_node->setDoubleValue(0);
        _audioIdent->setIdent("", 0.0 );
        return;
    }

    // Calculate the distance to the transmitter
    SGVec3d location = SGVec3d::fromGeod(globals->get_aircraft_position());
    
    double distance_nm = dist(_navrecord->cart(), location) * SG_METER_TO_NM;

    double range_nm = adjust_range(_navrecord->get_elev_ft(),
                                   globals->get_aircraft_position().getElevationFt(),
                                   _navrecord->get_range());

    if (distance_nm <= range_nm) {
        double volume = _volume_node->getDoubleValue();
        if( false == _ident_btn_node->getBoolValue() )
            volume = 0.0;

        _audioIdent->setIdent(_navrecord->ident(), volume );

        double speed_kt = (fabs(distance_nm - _last_distance_nm) *
                           ((1 / delta_time_sec) * 3600.0));
        _last_distance_nm = distance_nm;

        _in_range_node->setBoolValue(true);
        double tmp_dist = distance_nm - _navrecord->get_multiuse();
        if ( tmp_dist < 0.0 ) {
            tmp_dist = 0.0;
        }
        _distance_node->setDoubleValue( tmp_dist );
        _speed_node->setDoubleValue(speed_kt);
        if (SGLimitsd::min() < fabs(speed_kt))
          _time_node->setDoubleValue(distance_nm/speed_kt*60.0);
        
    } else {
        _last_distance_nm = 0;
        _in_range_node->setBoolValue(false);
        _distance_node->setDoubleValue(0);
        _speed_node->setDoubleValue(0);
        _time_node->setDoubleValue(0);
        _audioIdent->setIdent("", 0.0 );
    }
    
    _audioIdent->update( delta_time_sec );
}

// end of dme.cxx
