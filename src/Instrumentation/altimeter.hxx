// altimeter.hxx - an altimeter tied to the static port.
// Written by David Megginson, started 2002.
// Updated by John Denker to match changes in altimeter.cxx in 2007
//
// This file is in the Public Domain and comes with no warranty.


#ifndef __INSTRUMENTS_ALTIMETER_HXX
#define __INSTRUMENTS_ALTIMETER_HXX 1

#include <simgear/props/props.hxx>
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

private:

    string _name;
    int _num;
    string _static_source;
    SGPropertyNode_ptr _static_pressure_node;
    double _tau;
    double _quantum;
    double _setting;
    double _kollsman;

    bool _serviceable;
    double _press_alt;
    double _mode_c_alt;
    double _altitude;
    double _filtered_PA;

    FGAltimeter _altimeter;

public:

// Constructor:
    Altimeter (SGPropertyNode *node, double quantum = 0);
    ~Altimeter ();

    void init ();
    void bind ();
    void unbind ();
    void update (double dt);

    inline bool get_serviceable() const     { return _serviceable; }
    inline void set_serviceable( bool val ) { _serviceable = val; }

    inline double get_setting() const     { return _setting; }
    inline void set_setting( double val ) { _setting = val; }

    inline double get_press_alt() const     { return _press_alt; }
    inline void set_press_alt( double val ) { _press_alt = val; }

    inline double get_mode_c() const     { return _mode_c_alt; }
    inline void set_mode_c( double val ) { _mode_c_alt = val; }

    inline double get_altitude() const     { return _altitude; }
    inline void set_altitude( double val ) { _altitude = val; }

};

#endif // __INSTRUMENTS_ALTIMETER_HXX
