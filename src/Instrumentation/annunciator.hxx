// annunciator.hxx - manage the annunciator states
// Written by Curtis Olson, started May, 2003.


#ifndef __INSTRUMENTS_ANNUNCIATOR_HXX
#define __INSTRUMENTS_ANNUNCIATOR_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/props/props.hxx>

#include <Main/fgfs.hxx>


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
class Annunciator : public FGSubsystem
{

    // timers
    double timer0;              // used to sync flashing
    double timer1;
    double timer2;
    double timer3;
    double timer4;

    // inputs
    SGPropertyNode *_volts;
    SGPropertyNode *_vac_l;
    SGPropertyNode *_vac_r;
    SGPropertyNode *_fuel_l;
    SGPropertyNode *_fuel_r;
    SGPropertyNode *_oil_px;
    
    // outputs
    SGPropertyNode *_ann_volts;  // VOLTS        (red)
    SGPropertyNode *_ann_vac_l;  // L VAC        (amber)
    SGPropertyNode *_ann_vac_r;  //   VAC R      (amber
    SGPropertyNode *_ann_fuel_l; // L LOW FUEL   (amber)
    SGPropertyNode *_ann_fuel_r; //   LOW FUEL R (amber)
    SGPropertyNode *_ann_oil_px; // OIL PRESS    (red)

public:

    Annunciator ();
    virtual ~Annunciator ();

    virtual void init ();
    virtual void update (double dt);

private:

};

#endif // __INSTRUMENTS_ANNUNCIATOR_HXX
