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

using std::vector;
using std::string;

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
    max_range_nm = 150;
    double delta_elevation_ft =
        fabs(aircraft_altitude_ft - transmitter_elevation_ft);
    double range_nm = 1.23 * sqrt(delta_elevation_ft);
    if (range_nm > max_range_nm)
        range_nm = max_range_nm;
    else if (range_nm < 20.0)
        range_nm = 20.0;
    double rand = sg_random();
    SG_LOG( SG_INSTR, SG_DEBUG, " tacan range " << range_nm << " max range " << max_range_nm);
    return range_nm + (range_nm * rand * rand);
}


TACAN::TACAN ( SGPropertyNode *node ) :
    _name(node->getStringValue("name", "tacan")),
    _num(node->getIntValue("number", 0)),
    _new_frequency(false),
    _channel("0000"),
    _last_distance_nm(0),
    _frequency_mhz(-1),
    _time_before_search_sec(0),
    _mobile_valid(false),
    _transmitter_valid(false),
    _transmitter_pos(SGGeod::fromDeg(0, 0)),
    _transmitter_range_nm(0),
    _transmitter_bias(0.0),
    _mobile_lat(0.0),
    _mobile_lon(0.0),
    _listener_active(0)
{
}

TACAN::~TACAN ()
{
}

void
TACAN::init ()
{
    string branch;
    branch = "/instrumentation/" + _name;

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

    _channel_in0_node->addChangeListener(this);
    _channel_in1_node->addChangeListener(this);
    _channel_in2_node->addChangeListener(this);
    _channel_in3_node->addChangeListener(this);
    _channel_in4_node->addChangeListener(this, true);

    _in_range_node = node->getChild("in-range", 0, true);
    _distance_node = node->getChild("indicated-distance-nm", 0, true);
    _speed_node = node->getChild("indicated-ground-speed-kt", 0, true);
    _time_node = node->getChild("indicated-time-min", 0, true);
    _name_node = node->getChild("name", 0, true);
    _bearing_node = node->getChild("indicated-bearing-true-deg", 0, true);

    SGPropertyNode *dnode = node->getChild("display", 0, true);
    _x_shift_node = dnode->getChild("x-shift", 0, true);
    _y_shift_node = dnode->getChild("y-shift", 0, true);
    _rotation_node = dnode->getChild("rotation", 0, true);
    _channel_node = dnode->getChild("channel", 0, true);

    SGPropertyNode *cnode = fgGetNode("/ai/models/carrier", _num, false );
    _carrier_name_node = cnode ? cnode->getChild("name", 0, false) : 0;

    SGPropertyNode *tnode = fgGetNode("/ai/models/aircraft", _num, false);
    _tanker_callsign_node = tnode ? tnode->getChild("callsign", 0, false) : 0;

    SGPropertyNode *mnode = fgGetNode("/ai/models/multiplayer", _num, false);
    _mp_callsign_node = mnode ? mnode->getChild("callsign", 0, false) : 0;

    _longitude_node = fgGetNode("/position/longitude-deg", true);
    _latitude_node = fgGetNode("/position/latitude-deg", true);
    _altitude_node = fgGetNode("/position/altitude-ft", true);
    _heading_node = fgGetNode("/orientation/heading-deg", true);
    _yaw_node = fgGetNode("/orientation/side-slip-deg", true);
    _electrical_node = fgGetNode("/systems/electrical/outputs/tacan", true);
}

void
TACAN::update (double delta_time_sec)
{
    // don't do anything when paused
    if (delta_time_sec == 0) return;

    if (!_serviceable_node->getBoolValue() || !_electrical_node->getBoolValue()) {
        _last_distance_nm = 0;
        _in_range_node->setBoolValue(false);
        _distance_node->setDoubleValue(0);
        _speed_node->setDoubleValue(0);
        _time_node->setDoubleValue(0);
        SG_LOG( SG_INSTR, SG_DEBUG, "skip tacan" );
        return;
    }

                                // Get the aircraft position
    double longitude_deg = _longitude_node->getDoubleValue();
    double latitude_deg  = _latitude_node->getDoubleValue();
    double altitude_m    = _altitude_node->getDoubleValue() * SG_FEET_TO_METER;
    double longitude_rad = longitude_deg * SGD_DEGREES_TO_RADIANS;
    double latitude_rad  = latitude_deg * SGD_DEGREES_TO_RADIANS;

                                // On timeout, scan again
    _time_before_search_sec -= delta_time_sec;
    if ((_time_before_search_sec < 0 || _new_frequency) && _frequency_mhz >= 0)
        search(_frequency_mhz, longitude_rad, latitude_rad, altitude_m);

                                 // Calculate the distance to the transmitter

    //calculate the bearing and range of the mobile from the aircraft
    double mobile_az2 = 0;
    double mobile_bearing = 0;
    double mobile_distance = 0;

    SG_LOG( SG_INSTR, SG_DEBUG, "mobile_lat " << _mobile_lat);
    SG_LOG( SG_INSTR, SG_DEBUG, "mobile_lon " << _mobile_lon);
    SG_LOG( SG_INSTR, SG_DEBUG, "mobile_name " << _mobile_name);
    SG_LOG( SG_INSTR, SG_DEBUG, "mobile_valid " << _mobile_valid);
    geo_inverse_wgs_84(altitude_m,
                       latitude_deg,
                       longitude_deg,
                       _mobile_lat,
                       _mobile_lon,
                       &mobile_bearing, &mobile_az2, &mobile_distance);


    //calculate the bearing and range of the station from the aircraft
    double az2 = 0;
    double bearing = 0;
    double distance = 0;

    SGGeod pos = SGGeod::fromDegM(longitude_deg, latitude_deg, altitude_m);
    geo_inverse_wgs_84(pos, _transmitter_pos,
                       &bearing, &az2, &distance);


    //select the nearer
    if ( mobile_distance <= distance && _mobile_valid) {
        SG_LOG( SG_INSTR, SG_DEBUG, "mobile_distance_m " << mobile_distance);
        SG_LOG( SG_INSTR, SG_DEBUG, "distance_m " << distance);
        bearing = mobile_bearing;
        distance = mobile_distance;
        _transmitter_pos.setElevationFt(_mobile_elevation_ft);
        _transmitter_range_nm = _mobile_range_nm;
        _transmitter_bias = _mobile_bias;
        _transmitter_name = _mobile_name;
        _name_node->setStringValue(_transmitter_name.c_str());
        _transmitter_ident = _mobile_ident;
        _ident_node->setStringValue(_transmitter_ident.c_str());
        _channel_node->setStringValue(_channel.c_str());
    }

    //// calculate some values for boresight display
    double distance_nm = distance * SG_METER_TO_NM;

    //// calculate look left/right to target, without yaw correction
    // double horiz_offset = bearing - heading;
    //
    // if (horiz_offset > 180.0) horiz_offset -= 360.0;
    // if (horiz_offset < -180.0) horiz_offset += 360.0;

    //// now correct look left/right for yaw
    // horiz_offset += yaw;

    // use the bearing for a plan position indicator display

    double horiz_offset = bearing;

    SG_LOG( SG_INSTR, SG_DEBUG, "distance_nm " << distance_nm << " bearing "
            << bearing << " horiz_offset " << horiz_offset);

    // calculate values for radar display
    double y_shift = distance_nm * cos( horiz_offset * SG_DEGREES_TO_RADIANS);
    double x_shift = distance_nm * sin( horiz_offset * SG_DEGREES_TO_RADIANS);

    SG_LOG( SG_INSTR, SG_DEBUG, "y_shift " << y_shift  << " x_shift " << x_shift);

    double rotation = 0;

    double range_nm = adjust_range(_transmitter_pos.getElevationFt(),
                                   altitude_m * SG_METER_TO_FEET,
                                   _transmitter_range_nm);

    if (distance_nm <= range_nm) {
        double speed_kt = (fabs(distance_nm - _last_distance_nm) *
                           ((1 / delta_time_sec) * 3600.0));
        _last_distance_nm = distance_nm;

        _in_range_node->setBoolValue(true);
        double tmp_dist = distance_nm - _transmitter_bias;
        if ( tmp_dist < 0.0 ) {
            tmp_dist = 0.0;
        }
        _distance_node->setDoubleValue( tmp_dist );
        _speed_node->setDoubleValue(speed_kt);
        _time_node->setDoubleValue(speed_kt > 0 ? (distance_nm/speed_kt*60.0) : 0);
        _bearing_node->setDoubleValue(bearing);
        _x_shift_node->setDoubleValue(x_shift);
        _y_shift_node->setDoubleValue(y_shift);
        _rotation_node->setDoubleValue(rotation);
    } else {
        _last_distance_nm = 0;
        _in_range_node->setBoolValue(false);
        _distance_node->setDoubleValue(0);
        _speed_node->setDoubleValue(0);
        _time_node->setDoubleValue(0);
        _bearing_node->setDoubleValue(0);
        _x_shift_node->setDoubleValue(0);
        _y_shift_node->setDoubleValue(0);
        _rotation_node->setDoubleValue(0);
    }

                                // If we can't find a valid station set everything to zero
    if (!_transmitter_valid && !_mobile_valid ) {
        _in_range_node->setBoolValue(false);
        _distance_node->setDoubleValue(0);
        _speed_node->setDoubleValue(0);
        _time_node->setDoubleValue(0);
        _bearing_node->setDoubleValue(0);
        _x_shift_node->setDoubleValue(0);
        _y_shift_node->setDoubleValue(0);
        _rotation_node->setDoubleValue(0);
        _transmitter_name = "";
        _name_node->setStringValue(_transmitter_name.c_str());
        _transmitter_ident = "";
        _ident_node->setStringValue(_transmitter_ident.c_str());
        _channel_node->setStringValue(_channel.c_str());
        return;
    }
} // end function update

void
TACAN::search (double frequency_mhz, double longitude_rad,
               double latitude_rad, double altitude_m)
{
    int number, i;
    _mobile_valid = false;

    SG_LOG( SG_INSTR, SG_DEBUG, "tacan freq " << frequency_mhz );

    // reset search time
    _time_before_search_sec = 1.0;

    //try any carriers first
    FGNavRecord *mobile_tacan
          = globals->get_carrierlist()->findStationByFreq( frequency_mhz );
    bool freq_valid = (mobile_tacan != NULL);
    SG_LOG( SG_INSTR, SG_DEBUG, "mobile freqency valid " << freq_valid );

    if ( freq_valid ) {

        string str1( mobile_tacan->name() );

        SGPropertyNode * branch = fgGetNode("ai/models", true);
        vector<SGPropertyNode_ptr> carrier = branch->getChildren("carrier");

        number = carrier.size();

        SG_LOG( SG_INSTR, SG_DEBUG, "carrier " << number );
        for ( i = 0; i < number; ++i ) {
            string str2 ( carrier[i]->getStringValue("name", ""));
            SG_LOG( SG_INSTR, SG_DEBUG, "carrier name " << str2 );

            SG_LOG( SG_INSTR, SG_DEBUG, "strings 1 " << str1 << " 2 " << str2 );
            string::size_type loc1= str1.find( str2, 0 );
            if ( loc1 != string::npos && str2 != "" ) {
                SG_LOG( SG_INSTR, SG_DEBUG, " string found" );
                _mobile_lat = carrier[i]->getDoubleValue("position/latitude-deg");
                _mobile_lon = carrier[i]->getDoubleValue("position/longitude-deg");
                _mobile_elevation_ft = mobile_tacan->get_elev_ft();
                _mobile_range_nm = mobile_tacan->get_range();
                _mobile_bias = mobile_tacan->get_multiuse();
                _mobile_name = mobile_tacan->name();
                _mobile_ident = mobile_tacan->get_trans_ident();
                _mobile_valid = true;
                SG_LOG( SG_INSTR, SG_DEBUG, " carrier transmitter valid " << _mobile_valid );
                break;
            } else {
                _mobile_valid = false;
                SG_LOG( SG_INSTR, SG_DEBUG, " carrier transmitter invalid " << _mobile_valid );
            }
        }

        //try any AI tankers second

        if ( !_mobile_valid) {
            SG_LOG( SG_INSTR, SG_DEBUG, "tanker transmitter valid start " << _mobile_valid );

        SGPropertyNode * branch = fgGetNode("ai/models", true);
        vector<SGPropertyNode_ptr> tanker = branch->getChildren("tanker");

        number = tanker.size();

        SG_LOG( SG_INSTR, SG_DEBUG, "tanker number " << number );

        for ( i = 0; i < number; ++i ) {
            string str4 ( tanker[i]->getStringValue("callsign", ""));
            SG_LOG( SG_INSTR, SG_DEBUG, "tanker callsign " << str4 );

            SG_LOG( SG_INSTR, SG_DEBUG, "strings 1 " << str1 << " 4 " << str4 );
            string::size_type loc1= str1.find( str4, 0 );
            if ( loc1 != string::npos && str4 != "" ) {
                SG_LOG( SG_INSTR, SG_DEBUG, " string found" );
                _mobile_lat = tanker[i]->getDoubleValue("position/latitude-deg");
                _mobile_lon = tanker[i]->getDoubleValue("position/longitude-deg");
                _mobile_elevation_ft = tanker[i]->getDoubleValue("position/altitude-ft");
                _mobile_range_nm = mobile_tacan->get_range();
                _mobile_bias = mobile_tacan->get_multiuse();
                _mobile_name = mobile_tacan->name();
                _mobile_ident = mobile_tacan->get_trans_ident();
                _mobile_valid = true;
                SG_LOG( SG_INSTR, SG_DEBUG, " tanker transmitter valid " << _mobile_valid );
                break;
            } else {
                _mobile_valid = false;
                SG_LOG( SG_INSTR, SG_DEBUG, " tanker transmitter invalid " << _mobile_valid );
            }
        }
    }

    //try any mp tankers third, if we haven't found the tanker in the ai aircraft

    if ( !_mobile_valid ) {
        SG_LOG( SG_INSTR, SG_DEBUG, " mp tanker transmitter valid start " << _mobile_valid );

        SGPropertyNode * branch = fgGetNode("ai/models", true);
        vector<SGPropertyNode_ptr> mp_tanker = branch->getChildren("multiplayer");

        number = mp_tanker.size();

        SG_LOG( SG_INSTR, SG_DEBUG, " mp tanker number " << number );

        if ( number > 0 ) {	  // don't do this if there are no MP aircraft
            for ( i = 0; i < number; ++i ) {
                string str6 ( mp_tanker[i]->getStringValue("callsign", ""));
                SG_LOG( SG_INSTR, SG_DEBUG, "mp tanker callsign " << str6 );

                SG_LOG( SG_INSTR, SG_DEBUG, "strings 1 " << str1 << " 5 " << str6 );
                string::size_type loc1= str1.find( str6, 0 );
                if ( loc1 != string::npos && str6 != "" ) {
                    SG_LOG( SG_INSTR, SG_DEBUG, " string found" );
                    _mobile_lat = mp_tanker[i]->getDoubleValue("position/latitude-deg");
                    _mobile_lon = mp_tanker[i]->getDoubleValue("position/longitude-deg");
                    _mobile_elevation_ft = mp_tanker[i]->getDoubleValue("position/altitude-ft");
                    _mobile_range_nm = mobile_tacan->get_range();
                    _mobile_bias = mobile_tacan->get_multiuse();
                    _mobile_name = mobile_tacan->name();
                    _mobile_ident = mobile_tacan->get_trans_ident();
                    _mobile_valid = true;

                    SG_LOG( SG_INSTR, SG_DEBUG, " mp tanker transmitter valid " << _mobile_valid );
                    SG_LOG( SG_INSTR, SG_DEBUG, " mp tanker name " << _mobile_name);
                    SG_LOG( SG_INSTR, SG_DEBUG, " mp lat " << _mobile_lat << "lon " << _mobile_lon);
                    SG_LOG( SG_INSTR, SG_DEBUG, " mp elev " << _mobile_elevation_ft);
                    SG_LOG( SG_INSTR, SG_DEBUG, " mp range " << _mobile_range_nm);
                    break;
                } else {
                    _mobile_valid = false;
                    SG_LOG( SG_INSTR, SG_DEBUG, " mp tanker transmitter invalid " << _mobile_valid );
                    }
                }
            }
        }
    } else {
        _mobile_valid = false;
        SG_LOG( SG_INSTR, SG_DEBUG, " mobile transmitter invalid " << _mobile_valid );
    }

    // try the TACAN/VORTAC list next
    FGNavRecord *tacan = globals->get_tacanlist()->findByFreq( frequency_mhz,
      SGGeod::fromRadM(longitude_rad, latitude_rad, altitude_m));

    _transmitter_valid = (tacan != NULL);

    if ( _transmitter_valid ) {
        SG_LOG( SG_INSTR, SG_DEBUG, "transmitter valid " << _transmitter_valid );

        _transmitter_pos = tacan->geod();
        _transmitter_range_nm = tacan->get_range();
        _transmitter_bias = tacan->get_multiuse();
        _transmitter_name = tacan->name();
        _name_node->setStringValue(_transmitter_name.c_str());
        _transmitter_ident = tacan->get_trans_ident();
        _ident_node->setStringValue(_transmitter_ident.c_str());

        SG_LOG( SG_INSTR, SG_DEBUG, "name " << _transmitter_name);
        SG_LOG( SG_INSTR, SG_DEBUG, _transmitter_pos);

    } else {
        SG_LOG( SG_INSTR, SG_DEBUG, "transmitter invalid " << _transmitter_valid );
    }
}

double
TACAN::searchChannel (const string& channel)
{
    double frequency_khz = 0;

    FGTACANRecord *freq
        = globals->get_channellist()->findByChannel( channel );
    bool _freq_valid = (freq != NULL);
    SG_LOG( SG_INSTR, SG_DEBUG, "freq valid " << _freq_valid );
    if ( _freq_valid ) {
        frequency_khz = freq->get_freq();
        SG_LOG( SG_INSTR, SG_DEBUG, "freq output " << frequency_khz );
        //check sanity
        if (frequency_khz >= 9620 && frequency_khz <= 121300)
            return frequency_khz/100;
    }
    return frequency_khz = 0;
} // end TACAN::searchChannel

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
    string channel = _channel;

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
        _time_before_search_sec = 0;
    }

    _listener_active--;
}

// end of TACAN.cxx
