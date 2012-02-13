// altimeter.hxx - an altimeter tied to the static port.
// Written by David Megginson, started 2002.
// Updated by John Denker to match changes in altimeter.cxx in 2007
//
// This file is in the Public Domain and comes with no warranty.


#ifndef __INSTRUMENTS_ALTIMETER_HXX
#define __INSTRUMENTS_ALTIMETER_HXX 1

#include <simgear/props/props.hxx>
#include <simgear/props/tiedpropertylist.hxx>
#include <simgear/structure/subsystem_mgr.hxx>
#include <Environment/atmosphere.hxx>


/**
 * Model a barometric altimeter tied to the static port.
 *
 * Input properties:
 *
 * /instrumentation/<name>/serviceable
 * /instrumentation/<name>/setting-inhg
 * <static_pressure>
 *
 * Output properties:
 *
 * /instrumentation/<name>/indicated-altitude-ft
 */
class Altimeter : public SGSubsystem
{

public:

    Altimeter (SGPropertyNode *node, double quantum = 0);
    virtual ~Altimeter ();

    virtual void init ();
    virtual void update (double dt);
    virtual void bind();
    virtual void unbind();

    double getSettingInHg() const;
    void setSettingInHg( double value );
    double getSettingHPa() const;
    void setSettingHPa( double value );

private:
    SGPropertyNode_ptr _rootNode;
    string _static_pressure;
    double _tau;
    double _quantum;
    double _kollsman;
    double raw_PA;
    double _settingInHg;

    SGPropertyNode_ptr _serviceable_node;
    SGPropertyNode_ptr _pressure_node;
    SGPropertyNode_ptr _press_alt_node;
    SGPropertyNode_ptr _mode_c_node;
    SGPropertyNode_ptr _altitude_node;

    FGAltimeter _altimeter;

    simgear::TiedPropertyList _tiedProperties;
};

#endif // __INSTRUMENTS_ALTIMETER_HXX
