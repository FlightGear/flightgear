// dme.hxx - distance-measuring equipment.
// Written by David Megginson, started 2003.
//
// This file is in the Public Domain and comes with no warranty.


#ifndef __INSTRUMENTS_DME_HXX
#define __INSTRUMENTS_DME_HXX 1

#include <Instrumentation/AbstractInstrument.hxx>

// forward decls
class FGNavRecord;

/**
 * Model a DME radio.
 *
 * Input properties:
 *
 * /position/longitude-deg
 * /position/latitude-deg
 * /position/altitude-ft
 * /systems/electrical/outputs/dme
 * /instrumentation/"name"/serviceable
 * /instrumentation/"name"/frequencies/source
 * /instrumentation/"name"/frequencies/selected-mhz
 *
 * Output properties:
 *
 * /instrumentation/"name"/in-range
 * /instrumentation/"name"/indicated-distance-nm
 * /instrumentation/"name"/indicated-ground-speed-kt
 * /instrumentation/"name"/indicated-time-kt
 */
class DME : public AbstractInstrument
{
public:
    DME ( SGPropertyNode *node );
    virtual ~DME ();

    // Subsystem API.
    void init() override;
    void reinit() override;
    void update(double delta_time_sec) override;

    // Subsystem identification.
    static const char* staticSubsystemClassId() { return "dme"; }

private:
	void clear();

    SGPropertyNode_ptr _source_node;
    SGPropertyNode_ptr _frequency_node;

    SGPropertyNode_ptr _in_range_node;
    SGPropertyNode_ptr _distance_node;
    SGPropertyNode_ptr _speed_node;
    SGPropertyNode_ptr _time_node;
    SGPropertyNode_ptr _ident_btn_node;
    SGPropertyNode_ptr _volume_node;

    SGPropertyNode_ptr _distance_string;
    SGPropertyNode_ptr _speed_string;
    SGPropertyNode_ptr _time_string;

    double _last_distance_nm;
    double _last_frequency_mhz;
    double _time_before_search_sec;

    FGNavRecord * _navrecord;

    class AudioIdent * _audioIdent;
};

#endif // __INSTRUMENTS_DME_HXX
