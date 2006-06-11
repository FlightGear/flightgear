// annunciator.hxx - manage the annunciator states
// Written by Curtis Olson, started May, 2003.


#ifndef __INSTRUMENTS_ANNUNCIATOR_HXX
#define __INSTRUMENTS_ANNUNCIATOR_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>


/**
 * Model the annunciators.  This is innitially hard coded for a C172S
 *
 * Input properties:
 *
 * Amps
 * L/R Fuel qty
 * L/R Vacuum pumps
 * Oil pressure
 *
 * Output properties:
 *
 * /instrumentation/airspeed-indicator/indicated-speed-kt
 */
class Annunciator : public SGSubsystem
{

    // timers
    double timer0;              // used to sync flashing
    double timer1;
    double timer2;
    double timer3;
    double timer4;

    // inputs
    SGPropertyNode_ptr _volts;
    SGPropertyNode_ptr _vac_l;
    SGPropertyNode_ptr _vac_r;
    SGPropertyNode_ptr _fuel_l;
    SGPropertyNode_ptr _fuel_r;
    SGPropertyNode_ptr _oil_px;
    SGPropertyNode_ptr _elec_serv;

    // outputs
    SGPropertyNode_ptr _ann_volts;  // VOLTS        (red)
    SGPropertyNode_ptr _ann_vac_l;  // L VAC        (amber)
    SGPropertyNode_ptr _ann_vac_r;  //   VAC R      (amber
    SGPropertyNode_ptr _ann_fuel_l; // L LOW FUEL   (amber)
    SGPropertyNode_ptr _ann_fuel_r; //   LOW FUEL R (amber)
    SGPropertyNode_ptr _ann_oil_px; // OIL PRESS    (red)

public:

    Annunciator ();
    virtual ~Annunciator ();

    virtual void init ();
    virtual void update (double dt);

private:

};

#endif // __INSTRUMENTS_ANNUNCIATOR_HXX
