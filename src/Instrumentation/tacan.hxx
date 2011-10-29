// dme.hxx - distance-measuring equipment.
// Written by David Megginson, started 2003.
//
// This file is in the Public Domain and comes with no warranty.


#ifndef __INSTRUMENTS_TACAN_HXX
#define __INSTRUMENTS_TACAN_HXX 1

#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>


/**
 * Model a TACAN radio.
 *
 * Input properties:
 *
 * /position/longitude-deg
 * /position/latitude-deg
 * /position/altitude-ft
 * /heading
 * /systems/electrical/outputs/TACAN
 * /instrumentation/"name"/serviceable
 * /instrumentation/"name"/frequencies/selected-mhz
 *
 * Output properties:
 *
 * /instrumentation/"name"/in-range
 * /instrumentation/"name"/indicated-distance-nm
 * /instrumentation/"name"/indicated-ground-speed-kt
 * /instrumentation/"name"/indicated-time-kt
 */
class TACAN : public SGSubsystem, public SGPropertyChangeListener
{

public:

    TACAN ( SGPropertyNode *node );
    virtual ~TACAN ();

    virtual void init ();
    virtual void update (double delta_time_sec);

private:

    void search (double frequency, double longitude_rad,
                 double latitude_rad, double altitude_m);
  double searchChannel (const std::string& channel);
    void valueChanged (SGPropertyNode *);

    std::string _name;
    unsigned int _num;

    SGPropertyNode_ptr _longitude_node;
    SGPropertyNode_ptr _latitude_node;
    SGPropertyNode_ptr _altitude_node;
    SGPropertyNode_ptr _heading_node;
    SGPropertyNode_ptr _yaw_node;
    SGPropertyNode_ptr _serviceable_node;
    SGPropertyNode_ptr _electrical_node;
    SGPropertyNode_ptr _frequency_node;
    SGPropertyNode_ptr _display_node;
    SGPropertyNode_ptr _x_shift_node;
    SGPropertyNode_ptr _y_shift_node;
    SGPropertyNode_ptr _rotation_node;

    SGPropertyNode_ptr _in_range_node;
    SGPropertyNode_ptr _distance_node;
    SGPropertyNode_ptr _speed_node;
    SGPropertyNode_ptr _time_node;
    SGPropertyNode_ptr _bearing_node;
    SGPropertyNode_ptr _ident_node;
    SGPropertyNode_ptr _name_node;

    SGPropertyNode_ptr _channel_node;
    SGPropertyNode_ptr _channel_in0_node;
    SGPropertyNode_ptr _channel_in1_node;
    SGPropertyNode_ptr _channel_in2_node;
    SGPropertyNode_ptr _channel_in3_node;
    SGPropertyNode_ptr _channel_in4_node;

    SGPropertyNode_ptr _carrier_name_node;		// FIXME unused
    SGPropertyNode_ptr _tanker_callsign_node;		// FIXME
    SGPropertyNode_ptr _mp_callsign_node;		// FIXME

    bool _new_frequency;
    std::string _channel;
    double _last_distance_nm;
    double _frequency_mhz;
    double _time_before_search_sec;

    bool _mobile_valid;
    bool _transmitter_valid;

    SGVec3d _transmitter;
    SGGeod _transmitter_pos;
    double _transmitter_range_nm;
    double _transmitter_bearing_deg;
    double _transmitter_bias;
    std::string _transmitter_name;
    std::string _transmitter_ident;

    double _mobile_lat, _mobile_lon;
    double _mobile_elevation_ft;
    double _mobile_range_nm;
    double _mobile_bearing_deg;
    double _mobile_bias;
    std::string _mobile_name;
    std::string _mobile_ident;

    int _listener_active;
};


#endif // __INSTRUMENTS_TACAN_HXX
