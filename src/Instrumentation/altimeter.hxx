// altimeter.hxx - an altimeter tied to the static port.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain and comes with no warranty.


#ifndef __INSTRUMENTS_ALTIMETER_HXX
#define __INSTRUMENTS_ALTIMETER_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/misc/props.hxx>
#include <Main/fgfs.hxx>


class SGInterpTable;


/**
 * Model a barometric altimeter tied to the static port.
 *
 * Input properties:
 *
 * /instrumentation/altimeter/serviceable
 * /instrumentation/altimeter/setting-inhg
 * /systems/static[0]/pressure-inhg
 *
 * Output properties:
 *
 * /instrumentation/altimeter/indicated-altitude-ft
 */
class Altimeter : public FGSubsystem
{

public:

    Altimeter ();
    virtual ~Altimeter ();

    virtual void init ();
    virtual void bind ();
    virtual void unbind ();
    virtual void update (double dt);

private:

    SGPropertyNode_ptr _serviceable_node;
    SGPropertyNode_ptr _setting_node;
    SGPropertyNode_ptr _pressure_node;
    SGPropertyNode_ptr _altitude_node;

    SGInterpTable * _altitude_table;
    
};

#endif // __INSTRUMENTS_ALTIMETER_HXX
