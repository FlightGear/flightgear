// annunciator.hxx - manage the annunciator states
// Written by Curtis Olson, started May, 2003.

#include <math.h>

#include <simgear/math/interpolater.hxx>

#include <Main/fg_props.hxx>
#include <Main/util.hxx>

#include "annunciator.hxx"


Annunciator::Annunciator ():
    timer0( 0.0 ),
    timer1( 0.0 ),
    timer2( 0.0 ),
    timer3( 0.0 ),
    timer4( 0.0 )
{
}

Annunciator::~Annunciator ()
{
}

void
Annunciator::init ()
{
    _volts = fgGetNode( "/systems/electrical/volts", true );
    _vac_l = fgGetNode( "/systems/vacuum[0]/suction-inhg", true );
    _vac_r = fgGetNode( "/systems/vacuum[1]/suction-inhg", true );
    _fuel_l = fgGetNode( "/consumables/fuel/tank[0]/level-gal_us", true );
    _fuel_r = fgGetNode( "/consumables/fuel/tank[1]/level-gal_us", true );
    _oil_px = fgGetNode( "/engines/engine[0]/oil-pressure-psi", true );
    _elec_serv = fgGetNode( "/systems/electrical/serviceable", true );

    _ann_volts = fgGetNode( "/instrumentation/annunciator/volts", true );
    _ann_vac_l = fgGetNode( "/instrumentation/annunciator/vacuum-left", true );
    _ann_vac_r = fgGetNode( "/instrumentation/annunciator/vacuum-right", true );
    _ann_fuel_l = fgGetNode( "/instrumentation/annunciator/fuel-left", true );
    _ann_fuel_r = fgGetNode( "/instrumentation/annunciator/fuel-right", true );
    _ann_oil_px = fgGetNode( "/instrumentation/annunciator/oil-pressure", true );
}

void
Annunciator::update (double dt)
{
    // timers
    timer0 += dt;
    timer1 += dt;
    timer2 += dt;
    timer3 += dt;
    timer4 += dt;

    if ( _volts->getDoubleValue() < 5.0 || !_elec_serv->getBoolValue() ) {
        // Not enough juice to illuminate the display
        _ann_volts->setBoolValue( false );
        _ann_vac_l->setBoolValue( false );
        _ann_vac_r->setBoolValue( false );
        _ann_fuel_l->setBoolValue( false );
        _ann_fuel_r->setBoolValue( false );
        _ann_oil_px->setBoolValue( false );
    } else {
        // Volts
        if ( _volts->getDoubleValue() < 24.5 ) {
            if ( timer1 < 10 ) { 
                double rem = timer0 - (int)timer0;
                if ( rem <= 0.5 ) {
                    _ann_volts->setBoolValue( true );
                } else {
                    _ann_volts->setBoolValue( false );
                }
            } else {
                _ann_volts->setBoolValue( true );
            }
        } else {
            _ann_volts->setBoolValue( false );
            timer1 = 0.0;
        }

        if ( _fuel_l->getDoubleValue() < 5.0
             && _fuel_r->getDoubleValue() < 5.0 )
        {
            if ( timer2 < 10 ) {
                double rem = timer0 - (int)timer0;
                if ( rem <= 0.5 ) {
                    _ann_fuel_l->setBoolValue( true );
                    _ann_fuel_r->setBoolValue( true );
                } else {
                    _ann_fuel_l->setBoolValue( false );
                    _ann_fuel_r->setBoolValue( false );
                }
            } else {
                _ann_fuel_l->setBoolValue( true );
                _ann_fuel_r->setBoolValue( true );
            }
        } else if ( _fuel_l->getDoubleValue() < 5.0 ) {
            if ( timer2 < 10 ) {
                double rem = timer0 - (int)timer0;
                if ( rem <= 0.5 ) {
                    _ann_fuel_l->setBoolValue( true );
                } else {
                    _ann_fuel_l->setBoolValue( false );
                }
            } else {
                _ann_fuel_l->setBoolValue( true );
            }
            _ann_fuel_r->setBoolValue( false );
        } else if ( _fuel_r->getDoubleValue() < 5.0 ) {
            if ( timer2 < 10 ) {
                double rem = timer0 - (int)timer0;
                if ( rem <= 0.5 ) {
                    _ann_fuel_r->setBoolValue( true );
                } else {
                    _ann_fuel_r->setBoolValue( false );
                }
            } else {
                _ann_fuel_r->setBoolValue( true );
            }
            _ann_fuel_l->setBoolValue( false );
        } else {
            _ann_fuel_l->setBoolValue( false );
            _ann_fuel_r->setBoolValue( false );
            timer2 = 0.0;
        }

        // vacuum pumps
        if ( _vac_l->getDoubleValue() < 3.0
             && _vac_r->getDoubleValue() < 3.0 )
        {
            if ( timer3 < 10 ) {
                double rem = timer0 - (int)timer0;
                if ( rem <= 0.5 ) {
                    _ann_vac_l->setBoolValue( true );
                    _ann_vac_r->setBoolValue( true );
                } else {
                    _ann_vac_l->setBoolValue( false );
                    _ann_vac_r->setBoolValue( false );
                }
            } else {
                _ann_vac_l->setBoolValue( true );
                _ann_vac_r->setBoolValue( true );
            }
        } else if ( _vac_l->getDoubleValue() < 3.0 ) {
            if ( timer3 < 10 ) {
                double rem = timer0 - (int)timer0;
                if ( rem <= 0.5 ) {
                    _ann_vac_l->setBoolValue( true );
                } else {
                    _ann_vac_l->setBoolValue( false );
                }
            } else {
                _ann_vac_l->setBoolValue( true );
            }
            _ann_vac_r->setBoolValue( false );
        } else if ( _vac_r->getDoubleValue() < 3.0 ) {
            if ( timer3 < 10 ) {
                double rem = timer0 - (int)timer0;
                if ( rem <= 0.5 ) {
                    _ann_vac_r->setBoolValue( true );
                } else {
                    _ann_vac_r->setBoolValue( false );
                }
            } else {
                _ann_vac_r->setBoolValue( true );
            }
            _ann_vac_l->setBoolValue( false );
        } else {
            _ann_vac_l->setBoolValue( false );
            _ann_vac_r->setBoolValue( false );
            timer3 = 0.0;
        }

        // Oil pressure
        if ( _oil_px->getDoubleValue() < 20.0 ) {
            if ( timer4 < 10 ) {
                double rem = timer0 - (int)timer0;
                if ( rem <= 0.5 ) {
                    _ann_oil_px->setBoolValue( true );
                } else {
                    _ann_oil_px->setBoolValue( false );
                }
            } else {
                _ann_oil_px->setBoolValue( true );
            }
        } else {
            _ann_oil_px->setBoolValue( false );
            timer4 = 0.0;
        }
    }
}


// end of annunciator.cxx
