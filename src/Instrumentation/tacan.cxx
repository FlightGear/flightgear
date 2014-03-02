// tacan.cxx - Tactical Navigation Beacon.
// Written by Vivian Meazaa, started 2005.
//
// This file is in the Public Domain and comes with no warranty.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/math/sg_random.h>

#include <Main/fg_props.hxx>
#include <Navaids/navlist.hxx>
#include <vector>

#include "tacan.hxx"

/**
 * Adjust the range.
 *
 * Start by calculating the radar horizon based on the elevation
 * difference, then clamp to the maximum, then add a fudge for
 * borderline reception.
 */
static double
adjust_range( double station_elevation_ft,
              double aircraft_altitude_ft,
              double max_range_nm )
{
  // See http://en.wikipedia.org/wiki/Line-of-sight_propagation for approximate
  // line-of-sight distance to the horizon

  double range_nm = 0;
  if( station_elevation_ft > 5 )
    range_nm += 1.23 * sqrt(station_elevation_ft);

  if( aircraft_altitude_ft > 5 )
    range_nm += 1.23 * sqrt(aircraft_altitude_ft);

  return std::max(20., std::min(range_nm, max_range_nm))
       * (1 + SGMiscd::pow<2>(0.3 * sg_random()));
}

//------------------------------------------------------------------------------
TACAN::TACAN( SGPropertyNode *node ):
  _name(node->getStringValue("name", "tacan")),
  _num(node->getIntValue("number", 0)),
  _was_disabled(true),
  _new_frequency(false),
  _channel("0000"),
  _last_distance_nm(0),
  _frequency_mhz(-1),
  _time_before_search_sec(0),
  _listener_active(0)
{

}

//------------------------------------------------------------------------------
TACAN::~TACAN()
{

}

//------------------------------------------------------------------------------
void TACAN::init()
{
    std::string branch = "/instrumentation/" + _name;

    SGPropertyNode *node = fgGetNode(branch.c_str(), _num, true );

    _serviceable_node = node->getChild("serviceable", 0, true);
    _ident_node = node->getChild("ident", 0, true);

    SGPropertyNode *fnode = node->getChild("frequencies", 0, true);
    _frequency_node = fnode->getChild("selected-mhz", 0, true);

    _channel_in0_node = fnode->getChild("selected-channel", 0, true);
    _channel_in1_node = fnode->getChild("selected-channel", 1, true);
    _channel_in2_node = fnode->getChild("selected-channel", 2, true);
    _channel_in3_node = fnode->getChild("selected-channel", 3, true);
    _channel_in4_node = fnode->getChild("selected-channel", 4, true);

    _in_range_node = node->getChild("in-range", 0, true);
    _distance_node = node->getChild("indicated-distance-nm", 0, true);
    _speed_node = node->getChild("indicated-ground-speed-kt", 0, true);
    _time_node = node->getChild("indicated-time-min", 0, true);
    _name_node = node->getChild("name", 0, true);
    _bearing_node = node->getChild("indicated-bearing-true-deg", 0, true);

    SGPropertyNode *dnode = node->getChild("display", 0, true);
    _x_shift_node = dnode->getChild("x-shift", 0, true);
    _y_shift_node = dnode->getChild("y-shift", 0, true);
    _channel_node = dnode->getChild("channel", 0, true);

    _heading_node = fgGetNode("/orientation/heading-deg", true);
    _electrical_node = fgGetNode("/systems/electrical/outputs/tacan", true);

    // Add/trigger change listener after creating all nodes
    _channel_in0_node->addChangeListener(this);
    _channel_in1_node->addChangeListener(this);
    _channel_in2_node->addChangeListener(this);
    _channel_in3_node->addChangeListener(this);
    _channel_in4_node->addChangeListener(this, true);

    disabled(true);
}

//------------------------------------------------------------------------------
void TACAN::reinit()
{
  _time_before_search_sec = 0;
}

//------------------------------------------------------------------------------
void TACAN::update(double delta_time_sec)
{
  // don't do anything when paused
  if( delta_time_sec == 0 )
    return;

  if(   !_serviceable_node->getBoolValue()
     || !_electrical_node->getBoolValue() )
    return disabled();

  SGGeod pos(globals->get_aircraft_position());

  // On timeout, scan again
  _time_before_search_sec -= delta_time_sec;
  if ((_time_before_search_sec < 0 || _new_frequency) && _frequency_mhz >= 0)
      search(_frequency_mhz, pos);

  if( !_active_station || !_active_station->get_serviceable() )
    return disabled();

  const SGGeod& nav_pos = _active_station->geod();

  // Calculate the bearing and range of the station from the aircraft
  double az = 0;
  double bearing = 0;
  double distance = SGLimitsd::max();
  if( !SGGeodesy::inverse(pos, nav_pos, bearing, az, distance) )
    return disabled();

  // Increase distance due to difference in altitude
  distance =
    sqrt( SGMiscd::pow<2>(distance)
        + SGMiscd::pow<2>(pos.getElevationM() - nav_pos.getElevationM()) );
  double distance_nm = distance * SG_METER_TO_NM;

  double range_nm = adjust_range( nav_pos.getElevationFt(),
                                  pos.getElevationFt(),
                                  _active_station->get_range() );

  if( distance_nm > range_nm )
    return disabled();

  //// calculate look left/right to target, without yaw correction
  // double horiz_offset = bearing - heading;
  //
  // if (horiz_offset > 180.0) horiz_offset -= 360.0;
  // if (horiz_offset < -180.0) horiz_offset += 360.0;

  //// now correct look left/right for yaw
  // horiz_offset += yaw;

  // use the bearing for a plan position indicator display
  double horiz_offset = bearing;

  // calculate values for radar display
  double y_shift = distance_nm * cos(horiz_offset * SG_DEGREES_TO_RADIANS);
  double x_shift = distance_nm * sin(horiz_offset * SG_DEGREES_TO_RADIANS);

  // TODO probably not the best way to do this (especially with mobile stations)
  double speed_kt = fabs(distance_nm - _last_distance_nm)
                  * (3600 / delta_time_sec);
  _speed_node->setDoubleValue(speed_kt);
  _last_distance_nm = distance_nm;

  double bias = _active_station->get_multiuse();
  _distance_node->setDoubleValue( SGMiscd::max(0.0, distance_nm - bias) );

  _time_node->setDoubleValue(speed_kt > 0 ? (distance_nm/speed_kt*60.0) : 0);
  _bearing_node->setDoubleValue(bearing);
  _x_shift_node->setDoubleValue(x_shift);
  _y_shift_node->setDoubleValue(y_shift);

  _name_node->setStringValue(_active_station->name());
  _ident_node->setStringValue(_active_station->ident());
  _in_range_node->setBoolValue(true);

  _was_disabled = false;
}

//------------------------------------------------------------------------------
void TACAN::disabled(bool force)
{
  if( _was_disabled && !force )
    return;

  _last_distance_nm = 0;

  _in_range_node->setBoolValue(false);
  _distance_node->setDoubleValue(0);
  _speed_node->setDoubleValue(0);
  _time_node->setDoubleValue(0);
  _bearing_node->setDoubleValue(0);
  _x_shift_node->setDoubleValue(0);
  _y_shift_node->setDoubleValue(0);
  _name_node->setStringValue("");
  _ident_node->setStringValue("");

  _was_disabled = true;
}

//------------------------------------------------------------------------------
void TACAN::search (double frequency_mhz,const SGGeod& pos)
{
  // reset search time
  _time_before_search_sec = 5;

  // Get first matching mobile station (carriers/tankers/etc.)
  // TODO do we need to check for mobile stations with same frequency? Currently
  //      the distance is not taken into account.
  FGNavRecordRef mobile_tacan =
    FGNavList::findByFreq(frequency_mhz, FGNavList::mobileTacanFilter());

  double mobile_dist = (mobile_tacan && mobile_tacan->get_serviceable())
                     ? SGGeodesy::distanceM(pos, mobile_tacan->geod())
                     : SGLimitsd::max();

  // No get nearest TACAN/VORTAC
  FGNavRecordRef tacan =
    FGNavList::findByFreq(frequency_mhz, pos, FGNavList::tacanFilter());

  double tacan_dist = tacan
                    ? SGGeodesy::distanceM(pos, tacan->geod())
                    : SGLimitsd::max();

  // Select nearer station as active
  // TODO do we need to take more care of stations with same frequency?
  _active_station = mobile_dist < tacan_dist
                  ? mobile_tacan
                  : tacan;
}

//------------------------------------------------------------------------------
double TACAN::searchChannel(const std::string& channel)
{
  FGTACANRecord *freq = globals->get_channellist()->findByChannel(channel);
  if( !freq )
  {
    SG_LOG(SG_INSTR, SG_DEBUG, "Invalid TACAN channel: " << channel);
    return 0;
  }

  double frequency_khz = freq->get_freq();
  if(frequency_khz < 9620 || frequency_khz > 121300)
  {
    SG_LOG
    (
      SG_INSTR,
      SG_DEBUG,
      "TACAN frequency out of range: " << channel
                               << ": " << frequency_khz << "kHz"
    );
    return 0;
  }

  return frequency_khz/100;
}

/*
 * Listener callback. Maintains channel input properties,
 * searches new channel frequency, updates _channel and
 * _frequency and sets boolean _new_frequency appropriately.
 */
void
TACAN::valueChanged(SGPropertyNode *prop)
{
    if (_listener_active)
        return;
    _listener_active++;

    int index = prop->getIndex();
    std::string channel = _channel;

    if (index) {  // channel digit or X/Y input
        int c;
        if (isdigit(c = _channel_in1_node->getStringValue()[0]))
            channel[0] = c;
        if (isdigit(c = _channel_in2_node->getStringValue()[0]))
            channel[1] = c;
        if (isdigit(c = _channel_in3_node->getStringValue()[0]))
            channel[2] = c;
        c = _channel_in4_node->getStringValue()[0];
        if (c == 'X' || c == 'Y')
            channel[3] = c;

    } else {      // channel number input
        unsigned int f = prop->getIntValue();
        if (f >= 1 && f <= 126) {
            channel[0] = '0' + (f / 100) % 10;
            channel[1] = '0' + (f / 10) % 10;
            channel[2] = '0' + f % 10;
        }
    }

    if (channel != _channel) {
        SG_LOG(SG_INSTR, SG_DEBUG, "new channel " << channel);

        // write back result
        _channel_in0_node->setIntValue((channel[0] - '0') * 100
                + (channel[1] - '0') * 10 + (channel[2] - '0'));
        char s[2] = "0";
        s[0] = channel[0], _channel_in1_node->setStringValue(s);
        s[0] = channel[1], _channel_in2_node->setStringValue(s);
        s[0] = channel[2], _channel_in3_node->setStringValue(s);
        s[0] = channel[3], _channel_in4_node->setStringValue(s);

        // search channel frequency
        double freq = searchChannel(channel);
        if (freq != _frequency_mhz) {
            SG_LOG(SG_INSTR, SG_DEBUG, "new frequency " << freq);
            _frequency_node->setDoubleValue(freq);
            _frequency_mhz = freq;
            _new_frequency = true;
        }

        _channel = channel;
        _channel_node->setStringValue(_channel);
        _time_before_search_sec = 0;
    }

    _listener_active--;
}

// end of TACAN.cxx
