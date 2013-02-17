// vertical_speed_indicator.hxx - a regular VSI tied to the static port.
// Written by David Megginson, started 2002.
//
// Last change by E. van den Berg, 17.02.1013
//
// This file is in the Public Domain and comes with no warranty.


#ifndef __INSTRUMENTS_VERTICAL_SPEED_INDICATOR_HXX
#define __INSTRUMENTS_VERTICAL_SPEED_INDICATOR_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>


/**
 * Model a non-instantaneous VSI tied to the static port.
 *
 * Input properties:
 *
 * /instrumentation/"name"/serviceable
 * "static_port"/pressure-inhg
 *
 * Output properties:
 *
 * /instrumentation/"name"/indicated-speed-fpm
 * /instrumentation/"name"/indicated-speed-mps
 * /instrumentation/"name"/indicated-speed-kts
 */
class VerticalSpeedIndicator : public SGSubsystem
{

public:

    VerticalSpeedIndicator ( SGPropertyNode *node );
    virtual ~VerticalSpeedIndicator ();

    virtual void init ();
    virtual void reinit ();
    virtual void update (double dt);

private:

    double _casing_pressure_Pa;
    double _casing_airmass_kg;
    double _casing_density_kgpm3;
    double _orifice_massflow_kgps;

    std::string _name;
    int _num;
    std::string _static_pressure;
    std::string _static_temperature;

    SGPropertyNode_ptr _serviceable_node;
    SGPropertyNode_ptr _pressure_node;
    SGPropertyNode_ptr _temperature_node;
    SGPropertyNode_ptr _speed_fpm_node;
    SGPropertyNode_ptr _speed_mps_node;
    SGPropertyNode_ptr _speed_kts_node;
    SGPropertyNode_ptr _speed_up_node;
    
};

#endif // __INSTRUMENTS_VERTICAL_SPEED_INDICATOR_HXX
