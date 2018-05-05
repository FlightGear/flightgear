// adf.hxx - automatic direction finder.
// Written by David Megginson, started 2003.
//
// This file is in the Public Domain and comes with no warranty.


#ifndef __INSTRUMENTS_ADF_HXX
#define __INSTRUMENTS_ADF_HXX 1

#include <string>

#include <Instrumentation/AbstractInstrument.hxx>
#include <simgear/math/SGMath.hxx>

class SGSampleGroup;

/**
 * Model an ADF radio.
 *
 * Input properties:
 *
 * /position/longitude-deg
 * /position/latitude-deg
 * /position/altitude-ft
 * /orientation/heading-deg
 * /systems/electrical/outputs/adf
 * /instrumentation/adf/serviceable
 * /instrumentation/adf/error-deg
 * /instrumentation/adf/frequencies/selected-khz
 * /instrumentation/adf/mode
 * /instrumentation/adf/ident-audible
 * /instrumentation/adf/volume-norm
 *
 * Output properties:
 *
 * /instrumentation/adf/in-range
 * /instrumentation/adf/indicated-bearing-deg
 * /instrumentation/adf/ident
 */
class ADF : public AbstractInstrument
{
public:
    ADF ( SGPropertyNode *node );
    virtual ~ADF ();

    // Subsystem API.
    void init() override;
    void update(double delta_time_sec) override;

    // Subsystem identification.
    static const char* staticSubsystemClassId() { return "adf"; }

private:
    void set_bearing (double delta_time_sec, double bearing);

    void search (double frequency, const SGGeod& pos);

    SGPropertyNode_ptr _heading_node;
    SGPropertyNode_ptr _error_node;
    SGPropertyNode_ptr _frequency_node;
    SGPropertyNode_ptr _mode_node;

    SGPropertyNode_ptr _in_range_node;
    SGPropertyNode_ptr _bearing_node;
    SGPropertyNode_ptr _ident_node;
    SGPropertyNode_ptr _ident_audible_node;
    SGPropertyNode_ptr _volume_node;

    double _time_before_search_sec;

    int _last_frequency_khz;
    bool _transmitter_valid;
    std::string _last_ident;
    SGGeod _transmitter_pos;
    SGVec3d _transmitter_cart;
    double _transmitter_range_nm;

    int _ident_count;
    time_t _last_ident_time;
    float _last_volume;
    std::string _adf_ident;

    SGSharedPtr<SGSampleGroup> _sgr;
};

#endif // __INSTRUMENTS_ADF_HXX
