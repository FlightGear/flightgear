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

SG_USING_STD(vector);


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


TACAN::TACAN ( SGPropertyNode *node )
    : _last_distance_nm(0),
      _last_frequency_mhz(-1),
      _time_before_search_sec(0),
      _carrier_valid(false),
      _transmitter_valid(false),
      _transmitter_elevation_ft(0),
      _transmitter_range_nm(0),
      _transmitter_bias(0.0),

      name("tacan"),
      num(0)
{

    int i;
    for ( i = 0; i < node->nChildren(); ++i ) {
        SGPropertyNode *child = node->getChild(i);
        string cname = child->getName();
        string cval = child->getStringValue();
        if ( cname == "name" ) {
            name = cval;
        } else if ( cname == "number" ) {
            num = child->getIntValue();
        } else {
            SG_LOG( SG_INSTR, SG_DEBUG, "Error in TACAN config logic" );
            if ( name.length() ) {
                SG_LOG( SG_INSTR, SG_DEBUG, "Section = " << name );
            }
        }
    }
}

TACAN::TACAN ()
    : _last_distance_nm(0),
      _last_frequency_mhz(-1),
      _time_before_search_sec(0),
      _carrier_valid(false),
      _transmitter_valid(false),
      _transmitter_elevation_ft(0),
      _transmitter_range_nm(0),
      _transmitter_bearing_deg(0),
      _transmitter_bias(0.0),
      _transmitter_name(""),
      name("tacan")
{
}

TACAN::~TACAN ()
{
}

void
TACAN::init ()
{
    string branch;
    branch = "/instrumentation/" + name;

    SGPropertyNode *node = fgGetNode(branch.c_str(), num, true );

    _longitude_node = fgGetNode("/position/longitude-deg", true);
    _latitude_node = fgGetNode("/position/latitude-deg", true);
    _altitude_node = fgGetNode("/position/altitude-ft", true);
    _heading_node   = fgGetNode("/orientation/heading-deg", true);
    _yaw_node       = fgGetNode("/orientation/side-slip-deg", true);
    _serviceable_node = node->getChild("serviceable", 0, true);
    _electrical_node = fgGetNode("/systems/electrical/outputs/tacan", true);
    SGPropertyNode *fnode = node->getChild("frequencies", 0, true);
    _source_node = fnode->getChild("source", 0, true);
    _frequency_node = fnode->getChild("selected-mhz", 0, true);
    _channel_node = fnode->getChild("selected-channel", 1, true);
    _channel_node = fnode->getChild("selected-channel", 2, true);
    _channel_node = fnode->getChild("selected-channel", 3, true);
    _channel_node = fnode->getChild("selected-channel", 4, true);
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

    SGPropertyNode *cnode = fgGetNode("/ai/models/carrier", num, false );
    if (cnode)
        _carrier_name_node = cnode->getChild("name", 0, false);

    SGPropertyNode *tnode = fgGetNode("/ai/models/aircraft", num, false);
    if (tnode)
        _tanker_callsign_node = tnode->getChild("callsign", 0, false);

    SGPropertyNode *mnode = fgGetNode("/ai/models/multiplayer", num, false);
    if (mnode)
        _mp_callsign_node = mnode->getChild("callsign", 0, false);
}

void
TACAN::update (double delta_time_sec)
{
    double az2 = 0;
    double bearing = 0;
    double distance = 0;
    double carrier_az2 = 0;
    double carrier_bearing = 0;
    double carrier_distance = 0;
    double tanker_az2 = 0;
    double tanker_bearing = 0;
    double tanker_distance = 0;
    double frequency_mhz = 0;
    double mobile_bearing = 0;
    double mobile_distance = 0;
    double mobile_elevation_ft = 0;
    double mobile_range_nm = 0;
    double mobile_bias = 0;
    string mobile_name = "";
    bool   mobile_valid = false;

    string _channel, _last_channel, _channel_1, _channel_2,_channel_3, _channel_4;

                                // If it's off, don't waste any time.
    if (!_serviceable_node->getBoolValue() || !_electrical_node->getBoolValue()) {
        _last_distance_nm = 0;
        _in_range_node->setBoolValue(false);
        _distance_node->setDoubleValue(0);
        _speed_node->setDoubleValue(0);
        _time_node->setDoubleValue(0);
        SG_LOG( SG_INSTR, SG_DEBUG, "skip tacan" );
        return;
    }

                                // Figure out the source
    const char * source = _source_node->getStringValue();

    if (source[0] == '\0') {
        string branch;
        branch = "/instrumentation/" + name + "/frequencies/selected-channel";
        _source_node->setStringValue(branch.c_str());
        source = _source_node->getStringValue();
        SG_LOG( SG_INSTR, SG_DEBUG, "source " << source  );
    }
                                // Get the channel
    _channel_1 = fgGetString("/instrumentation/tacan/frequencies/selected-channel[1]");
    _channel_2 = fgGetString("/instrumentation/tacan/frequencies/selected-channel[2]");
    _channel_3 = fgGetString("/instrumentation/tacan/frequencies/selected-channel[3]");
    _channel_4 = fgGetString("/instrumentation/tacan/frequencies/selected-channel[4]");

    SG_LOG( SG_INSTR, SG_DEBUG, "channels " << _channel_1 << _channel_2 << _channel_3 << _channel_4);

    _channel = _channel_1 + _channel_2 + _channel_3 + _channel_4;

                                    // Get the frequency
    if (_channel != _last_channel) {
        _time_before_search_sec = 0;
        _last_channel = _channel;
        frequency_mhz = searchChannel(_channel);
        SG_LOG( SG_INSTR, SG_DEBUG, "frequency " << frequency_mhz );
        _frequency_node->setDoubleValue(frequency_mhz);
    }

    SG_LOG( SG_INSTR, SG_DEBUG, "channel " << _channel );
                                // Get the aircraft position
    double longitude_deg = _longitude_node->getDoubleValue();
    double latitude_deg  = _latitude_node->getDoubleValue();
    double altitude_m    = _altitude_node->getDoubleValue() * SG_FEET_TO_METER;
    double heading       = _heading_node->getDoubleValue() ;
    double yaw           = _yaw_node->getDoubleValue() ;
    double longitude_rad = longitude_deg * SGD_DEGREES_TO_RADIANS;
    double latitude_rad  = latitude_deg * SGD_DEGREES_TO_RADIANS;

                                // On timeout, scan again
    _time_before_search_sec -= delta_time_sec;
    if (_time_before_search_sec < 0 && frequency_mhz >= 0)
        search(frequency_mhz, longitude_rad, latitude_rad, altitude_m);

                                 // Calculate the distance to the transmitter

    //calculate the bearing and range of the carrier from the aircraft
    SG_LOG( SG_INSTR, SG_DEBUG, "carrier_lat " << _carrier_lat);
    SG_LOG( SG_INSTR, SG_DEBUG, "carrier_lon " << _carrier_lon);
    SG_LOG( SG_INSTR, SG_DEBUG, "carrier_name " << _carrier_name);
    SG_LOG( SG_INSTR, SG_DEBUG, "carrier_valid " << _carrier_valid);
    geo_inverse_wgs_84(altitude_m,
                       latitude_deg,
                       longitude_deg,
                       _carrier_lat,
                       _carrier_lon,
                       &carrier_bearing, &carrier_az2, &carrier_distance);

    //calculate the bearing and range of the tanker from the aircraft
    SG_LOG( SG_INSTR, SG_DEBUG, "tanker_lat " << _tanker_lat);
    SG_LOG( SG_INSTR, SG_DEBUG, "tanker_lon " << _tanker_lon);
    SG_LOG( SG_INSTR, SG_DEBUG, "tanker_name " << _tanker_name);
    SG_LOG( SG_INSTR, SG_DEBUG, "tanker_valid " << _tanker_valid);
    geo_inverse_wgs_84(altitude_m,
                       latitude_deg,
                       longitude_deg,
                       _tanker_lat,
                       _tanker_lon,
                       &tanker_bearing, &tanker_az2, &tanker_distance);


    //calculate the bearing and range of the station from the aircraft
    geo_inverse_wgs_84(altitude_m,
                       latitude_deg,
                       longitude_deg,
                       _transmitter_lat,
                       _transmitter_lon,
                       &bearing, &az2, &distance);

    //select the nearest valid mobile transmitter
    if ( _carrier_valid && (_tanker_valid || _mp_tanker_valid ) ) {
        if( carrier_distance <= tanker_distance ){
            SG_LOG( SG_INSTR, SG_DEBUG, " select carrier (dist) " );
            mobile_bearing = carrier_bearing;
            mobile_distance = carrier_distance;
            mobile_elevation_ft = _carrier_elevation_ft;
            mobile_range_nm = _carrier_range_nm;
            mobile_bias = _carrier_bias;
            mobile_name = _carrier_name;
        }else{
            SG_LOG( SG_INSTR, SG_DEBUG, " select tanker (dist)  " );
            mobile_bearing = tanker_bearing;
            mobile_distance = tanker_distance;
            mobile_elevation_ft = _tanker_elevation_ft;
            mobile_range_nm = _tanker_range_nm;
            mobile_bias = _tanker_bias;
            mobile_name = _tanker_name;
        }
        mobile_valid = true;
    }
    else if ( _carrier_valid && (!_tanker_valid || _mp_tanker_valid) ) {
        SG_LOG( SG_INSTR, SG_DEBUG, " select carrier  " );
        mobile_bearing = carrier_bearing;
        mobile_distance = carrier_distance;
        mobile_elevation_ft = _carrier_elevation_ft;
        mobile_range_nm = _carrier_range_nm;
        mobile_bias = _carrier_bias;
        mobile_name = _carrier_name;
        mobile_valid = true;
    }
    else if ( !_carrier_valid && (_tanker_valid || _mp_tanker_valid ) ) {
        SG_LOG( SG_INSTR, SG_DEBUG, " select tanker  " );
        mobile_bearing = tanker_bearing;
        mobile_distance = tanker_distance;
        mobile_elevation_ft = _tanker_elevation_ft;
        mobile_range_nm = _tanker_range_nm;
        mobile_bias = _tanker_bias;
        mobile_name = _tanker_name;
        mobile_valid = true;
    }else{
        mobile_valid = false;
    }




    //select the nearer
    if ( mobile_distance <= distance && mobile_valid) {
        SG_LOG( SG_INSTR, SG_DEBUG, "mobile_distance_m " << mobile_distance);
        SG_LOG( SG_INSTR, SG_DEBUG, "distance_m " << distance);
        bearing = mobile_bearing;
        distance = mobile_distance;
        _transmitter_elevation_ft = mobile_elevation_ft;
        _transmitter_range_nm = mobile_range_nm;
        _transmitter_bias = mobile_bias;
        _transmitter_name = mobile_name;
        _name_node->setStringValue(_transmitter_name.c_str());
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

    // use the bearing for plan position indicator display

    double horiz_offset = bearing;

    SG_LOG( SG_INSTR, SG_DEBUG, "distance_nm " << distance_nm  << " bearing "
            << bearing << " horiz_offset " << horiz_offset);

    // calculate values for radar display
    double y_shift = distance_nm * cos( horiz_offset * SG_DEGREES_TO_RADIANS);
    double x_shift = distance_nm * sin( horiz_offset * SG_DEGREES_TO_RADIANS);

    SG_LOG( SG_INSTR, SG_DEBUG, "y_shift " << y_shift  << " x_shift " << x_shift);

    double rotation = 0;

    /*Point3D location =
        sgGeodToCart(Point3D(longitude_rad, latitude_rad, altitude_m));
    double distance_nm = _transmitter.distance3D(location) * SG_METER_TO_NM;*/

    double range_nm = adjust_range(_transmitter_elevation_ft,
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
        _time_node->setDoubleValue(distance_nm/speed_kt*60.0);
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
    if (!_transmitter_valid && !mobile_valid ) {
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
        _channel_node->setStringValue(_channel.c_str());
        return;
    }
} // end function update

void
TACAN::search (double frequency_mhz, double longitude_rad,
               double latitude_rad, double altitude_m)
{
    int number, i;

    SG_LOG( SG_INSTR, SG_DEBUG, "tacan freq " << frequency_mhz );

    // reset search time
    _time_before_search_sec = 1.0;

    //try any carriers first
     FGNavRecord *carrier_tacan
          = globals->get_carrierlist()->findStationByFreq( frequency_mhz );
    _carrier_valid = (carrier_tacan != NULL);

    if ( _carrier_valid ) {
        SG_LOG( SG_INSTR, SG_DEBUG, "carrier transmitter valid " << _carrier_valid  );

        string str1( carrier_tacan->get_name() );

        SGPropertyNode * branch = fgGetNode("ai/models", true);
        vector<SGPropertyNode_ptr> carrier = branch->getChildren("carrier");

        number = carrier.size();

        SG_LOG( SG_INSTR, SG_DEBUG, "carrier " << number );

        for ( i = 0; i < number; ++i ) {
            string str2 ( carrier[i]->getStringValue("name", ""));
            SG_LOG( SG_INSTR, SG_DEBUG, "carrier name " << str2 );

            SG_LOG( SG_INSTR, SG_DEBUG, "strings 1 " << str1 << " 2 " << str2 );
            unsigned int loc1= str1.find( str2, 0 );
            if ( loc1 != string::npos && str2 != "" ) {
                SG_LOG( SG_INSTR, SG_DEBUG, " string found" );
                _carrier_lat = carrier[i]->getDoubleValue("position/latitude-deg");
                _carrier_lon = carrier[i]->getDoubleValue("position/longitude-deg");
                _carrier_elevation_ft = carrier_tacan->get_elev_ft();
                _carrier_range_nm = carrier_tacan->get_range();
                _carrier_bias = carrier_tacan->get_multiuse();
                _carrier_name = carrier_tacan->get_name();
                _carrier_valid = 1;
                SG_LOG( SG_INSTR, SG_DEBUG, " carrier transmitter valid " << _carrier_valid );
                break;
            } else {
                _carrier_valid = 0;
                SG_LOG( SG_INSTR, SG_DEBUG, " carrier transmitter invalid " << _carrier_valid );
            }
        }

        SG_LOG( SG_INSTR, SG_DEBUG, "name " << _carrier_name);
        SG_LOG( SG_INSTR, SG_DEBUG, "lat " << _carrier_lat << "lon " << _carrier_lon);
        SG_LOG( SG_INSTR, SG_DEBUG, "elev " << _carrier_elevation_ft);

    } else {
        SG_LOG( SG_INSTR, SG_DEBUG, " carrier transmitter invalid " << _carrier_valid  );
    }

    //try any tankers second
     FGNavRecord *tanker_tacan
          = globals->get_carrierlist()->findStationByFreq( frequency_mhz );

     _tanker_valid = (tanker_tacan != NULL);


    if ( _tanker_valid ) {
        SG_LOG( SG_INSTR, SG_DEBUG, "tanker transmitter valid start " << _tanker_valid  );

        string str3( tanker_tacan->get_name() );

        SGPropertyNode * branch = fgGetNode("ai/models", true);
        vector<SGPropertyNode_ptr> tanker = branch->getChildren("aircraft");

        number = tanker.size();

        SG_LOG( SG_INSTR, SG_DEBUG, "tanker number " << number );

        for ( i = 0; i < number; ++i ) {
            string str4 ( tanker[i]->getStringValue("callsign", ""));
            SG_LOG( SG_INSTR, SG_DEBUG, "tanker callsign " << str4 );

            SG_LOG( SG_INSTR, SG_DEBUG, "strings 3 " << str3 << " 4 " << str4 );
            unsigned int loc1= str3.find( str4, 0 );
            if ( loc1 != string::npos && str4 != "" ) {
                SG_LOG( SG_INSTR, SG_DEBUG, " string found" );
                _tanker_lat = tanker[i]->getDoubleValue("position/latitude-deg");
                _tanker_lon = tanker[i]->getDoubleValue("position/longitude-deg");
                _tanker_elevation_ft = tanker[i]->getDoubleValue("position/altitude-ft");
                _tanker_range_nm = tanker_tacan->get_range();
                _tanker_bias = tanker_tacan->get_multiuse();
                _tanker_name = tanker_tacan->get_name();
                _tanker_valid = 1;
                SG_LOG( SG_INSTR, SG_DEBUG, " tanker transmitter valid " << _tanker_valid );
                break;
            } else {
                _tanker_valid = 0;
                SG_LOG( SG_INSTR, SG_DEBUG, " tanker transmitter invalid " << _tanker_valid );
            }
        }

        SG_LOG( SG_INSTR, SG_DEBUG, "tanker name " << _tanker_name);
        SG_LOG( SG_INSTR, SG_DEBUG, "lat " << _tanker_lat << "lon " << _tanker_lon);
        SG_LOG( SG_INSTR, SG_DEBUG, "elev " << _tanker_elevation_ft);
        SG_LOG( SG_INSTR, SG_DEBUG, "range " << _tanker_range_nm);

    } else {
        SG_LOG( SG_INSTR, SG_DEBUG, " tanker transmitter invalid " << _tanker_valid  );
    }

    //try any mp tankers third, if we haven't found the tanker in the ai aircraft
    FGNavRecord *mp_tanker_tacan
            = globals->get_carrierlist()->findStationByFreq( frequency_mhz );

    _mp_tanker_valid = (mp_tanker_tacan != NULL);


    if ( _mp_tanker_valid && ! _tanker_valid ) {
        SG_LOG( SG_INSTR, SG_DEBUG, " mp tanker transmitter valid start " << _mp_tanker_valid  );

        string str5( mp_tanker_tacan->get_name() );

        SGPropertyNode * branch = fgGetNode("ai/models", true);
        vector<SGPropertyNode_ptr> mp_tanker = branch->getChildren("multiplayer");

        number = mp_tanker.size();

        SG_LOG( SG_INSTR, SG_DEBUG, " mp tanker number " << number );

        for ( i = 0; i < number; ++i ) {
            string str6 ( mp_tanker[i]->getStringValue("callsign", ""));
            SG_LOG( SG_INSTR, SG_DEBUG, "mp tanker callsign " << str6 );

            SG_LOG( SG_INSTR, SG_DEBUG, "strings 5 " << str5 << " 5 " << str6 );
            unsigned int loc1= str5.find( str6, 0 );
            if ( loc1 != string::npos && str6 != "" ) {
                SG_LOG( SG_INSTR, SG_DEBUG, " string found" );
                _tanker_lat = mp_tanker[i]->getDoubleValue("position/latitude-deg");
                _tanker_lon = mp_tanker[i]->getDoubleValue("position/longitude-deg");
                _tanker_elevation_ft = mp_tanker[i]->getDoubleValue("position/altitude-ft");
                _tanker_range_nm = mp_tanker_tacan->get_range();
                _tanker_bias = mp_tanker_tacan->get_multiuse();
                _tanker_name = mp_tanker_tacan->get_name();
                _mp_tanker_valid = 1;

                SG_LOG( SG_INSTR, SG_DEBUG, "  mp tanker transmitter valid " << _mp_tanker_valid );
                SG_LOG( SG_INSTR, SG_DEBUG, "mp_tanker name " << _tanker_name);
                SG_LOG( SG_INSTR, SG_DEBUG, "lat " << _tanker_lat << "lon " << _tanker_lon);
                SG_LOG( SG_INSTR, SG_DEBUG, "elev " << _tanker_elevation_ft);
                SG_LOG( SG_INSTR, SG_DEBUG, "range " << _tanker_range_nm);
                break;
            } else {
                _mp_tanker_valid = 0;
                SG_LOG( SG_INSTR, SG_DEBUG, " tanker transmitter invalid " << _mp_tanker_valid );
            }
        }



    } else {
        SG_LOG( SG_INSTR, SG_DEBUG, " tanker transmitter invalid " << _tanker_valid  );
    }
    // try the TACAN/VORTAC list next
    FGNavRecord *tacan
        = globals->get_tacanlist()->findByFreq( frequency_mhz, longitude_rad,
                                                latitude_rad, altitude_m);

    _transmitter_valid = (tacan != NULL);

    if ( _transmitter_valid ) {
        SG_LOG( SG_INSTR, SG_DEBUG, "transmitter valid " << _transmitter_valid  );

        _transmitter_lat = tacan->get_lat();
        _transmitter_lon = tacan->get_lon();
        _transmitter_elevation_ft = tacan->get_elev_ft();
        _transmitter_range_nm = tacan->get_range();
        _transmitter_bias = tacan->get_multiuse();
        _transmitter_name = tacan->get_name();
        _name_node->setStringValue(_transmitter_name.c_str());

        SG_LOG( SG_INSTR, SG_DEBUG, "name " << _transmitter_name);
        SG_LOG( SG_INSTR, SG_DEBUG, "lat " << _transmitter_lat << "lon " << _transmitter_lon);
        SG_LOG( SG_INSTR, SG_DEBUG, "elev " << _transmitter_elevation_ft);

    } else {
        SG_LOG( SG_INSTR, SG_DEBUG, "transmitter invalid " << _transmitter_valid  );
    }
}

double
TACAN::searchChannel (const string& _channel){

    double frequency_khz = 0;

    FGTACANRecord *freq
        = globals->get_channellist()->findByChannel( _channel );
    double _freq_valid = (freq != NULL);
    SG_LOG( SG_INSTR, SG_DEBUG, "freq valid " << _freq_valid  );
    if ( _freq_valid ) {
        frequency_khz = freq->get_freq();
        SG_LOG( SG_INSTR, SG_DEBUG, "freq output " << frequency_khz  );
        //check sanity
        if (frequency_khz >=9620 && frequency_khz <= 12130)
            return frequency_khz/100;
    }
    return frequency_khz = 0;
} // end TACAN::searchChannel

// end of TACAN.cxx
