// altimeter.hxx - an altimeter tied to the static port.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain and comes with no warranty.


#ifndef __INSTRUMENTS_ALTIMETER_HXX
#define __INSTRUMENTS_ALTIMETER_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>


class SGInterpTable;


/**
 * Model a barometric altimeter tied to the static port.
 *
 * Input properties:
 *
 * /instrumentation/"name"/serviceable
 * /instrumentation/"name"/setting-inhg
 * "static_port"/pressure-inhg
 *
 * Output properties:
 *
 * /instrumentation/"name"/indicated-altitude-ft
 */
class Altimeter : public SGSubsystem
{

public:

    Altimeter (SGPropertyNode *node);
    Altimeter ();
    virtual ~Altimeter ();

    virtual void init ();
    virtual void update (double dt);

private:

    string name;
    int num;
    string static_port;

    SGPropertyNode_ptr _serviceable_node;
    SGPropertyNode_ptr _setting_node;
    SGPropertyNode_ptr _pressure_node;
    SGPropertyNode_ptr _altitude_node;

    SGInterpTable * _altitude_table;
    
};

#endif // __INSTRUMENTS_ALTIMETER_HXX
