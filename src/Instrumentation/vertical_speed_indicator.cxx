// vertical_speed_indicator.cxx - a regular VSI.
// Written by David Megginson, started 2002.
//
// Last change by E. van den Berg, 17.02.1013
//
// This file is in the Public Domain and comes with no warranty.

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <simgear/constants.h>
#include <simgear/math/interpolater.hxx>

#include "vertical_speed_indicator.hxx"
#include <Main/fg_props.hxx>
#include <Main/util.hxx>

//** NOTE: do not change these values. If you change one of them the others need to be changed too */
//** these values calibrate the VSI at SL. */
#define Vol_casing 1.25e-4          //m3
#define A_orifice 7.853982e-9       //m2
#define Factor_cal 189.145628       //- 

using std::string;

VerticalSpeedIndicator::VerticalSpeedIndicator ( SGPropertyNode *node )
    : _casing_pressure_Pa(101325),
      _name(node->getStringValue("name", "vertical-speed-indicator")),
      _num(node->getIntValue("number", 0)),
      _static_pressure(node->getStringValue("static-pressure", "/systems/static/pressure-inhg")),
      _static_temperature(node->getStringValue("static-temperature", "/environment/temperature-degc"))
{
}

VerticalSpeedIndicator::~VerticalSpeedIndicator ()
{
}

void
VerticalSpeedIndicator::init ()
{
    string branch;
    branch = "/instrumentation/" + _name;

    SGPropertyNode *node = fgGetNode(branch.c_str(), _num, true );
    _serviceable_node = node->getChild("serviceable", 0, true);
    _pressure_node = fgGetNode(_static_pressure.c_str(), true);
    _temperature_node = fgGetNode(_static_temperature.c_str(), true);
    _speed_fpm_node = node->getChild("indicated-speed-fpm", 0, true);
    _speed_mps_node = node->getChild("indicated-speed-mps", 0, true);
    _speed_kts_node = node->getChild("indicated-speed-kts", 0, true);
    _speed_up_node = fgGetNode("/sim/speed-up", true);

    reinit();
}

void
VerticalSpeedIndicator::reinit ()
{
    // Initialize at ambient conditions
    double casing_pressure_inHg = _pressure_node->getDoubleValue();
    _casing_pressure_Pa =  casing_pressure_inHg * SG_INHG_TO_PA;
    double casing_temperature_C = _temperature_node->getDoubleValue();
    double casing_temperature_K = casing_temperature_C + 273.15;
    _casing_density_kgpm3 = _casing_pressure_Pa / (casing_temperature_K * SG_R_m2_p_s2_p_K);
    _casing_airmass_kg = _casing_density_kgpm3 * Vol_casing;
    _orifice_massflow_kgps = 0.0;
}

void
VerticalSpeedIndicator::update (double dt)
{
    if (_serviceable_node->getBoolValue()) {
        double pressure_inHg = _pressure_node->getDoubleValue() ;
        double pressure_Pa = pressure_inHg * SG_INHG_TO_PA;
        double speed_up = _speed_up_node->getDoubleValue();
        double Fsign = 0.;
        double orifice_mach = 0.0;
        if( speed_up > 1 )
            dt *= speed_up;

// This is a thermodynamically correct model of a mechanical vertical speed indicator:
// It represents an aneroid in a closed (constant volume) casing with the aneroid internal pressure = static pressure
// The casing has an orifice to static pressure 
// the mass flow through the orifice is calculated using compressible aerodynamics (but adiabatic and of course a perfect gas)
// using the pressure in the casing and static pressure
//
// sadly at very low flows (small VS) in conjunction with the fact discrete timesteps (dt) are used, a numerical instability is formed.
// this is counteracted by setting the massflow 0 at very small pressure differentials
// this causes a small funny jump of your VSI when passing through 0...cannot be helped!
//
// also note the calibration is only valid for 0ft, so at higher altitudes, the vertical speed is not correct, but would indicate as a real mechanical VSI.
// Only use for conventional mechanical VSI-s. Dont use in an Air Data Computer.
//
// (...and it is supposed to lag!)
    
        _casing_airmass_kg = _casing_airmass_kg - _orifice_massflow_kgps * dt;
        double new_density_kgpm3 = _casing_airmass_kg / Vol_casing;
        _casing_pressure_Pa = _casing_pressure_Pa * pow(new_density_kgpm3 / _casing_density_kgpm3 , SG_gamma);
        double casing_temperature_K = _casing_pressure_Pa / (new_density_kgpm3 * SG_R_m2_p_s2_p_K);

        if( _casing_pressure_Pa - pressure_Pa > 0.0 ) {
            Fsign = 1.0;         //outflow, pos VS
        } else {
            Fsign = -1.0;        //inflow, neg VS
        }

        if( fabs(_casing_pressure_Pa - pressure_Pa) < 0.01 ) {
            orifice_mach = 0.0;   
        } else { 
            orifice_mach = sqrt(fabs (2.0*SG_cp_m2_p_s2_p_K / (SG_gamma * SG_R_m2_p_s2_p_K) * ( pow(pressure_Pa / _casing_pressure_Pa ,(SG_gamma-1)/SG_gamma ) -1 ) ) );
        }

        _orifice_massflow_kgps = Fsign * _casing_pressure_Pa / sqrt(casing_temperature_K) * sqrt(SG_gamma/SG_R_m2_p_s2_p_K) * orifice_mach * pow(1+(SG_gamma-1)/2*orifice_mach*orifice_mach,-(SG_gamma+1)/(2*(SG_gamma-1))) * A_orifice;

        double vs_fpm = Fsign * sqrt( fabs( pressure_Pa - _casing_pressure_Pa ) ) * Factor_cal;
        double vs_kts = vs_fpm / 60 * SG_FPS_TO_KT;
        double vs_mps = vs_fpm / 60 * SG_FEET_TO_METER;

        _speed_fpm_node
          ->setDoubleValue(vs_fpm);
        _speed_kts_node
          ->setDoubleValue(vs_kts);
        _speed_mps_node
          ->setDoubleValue(vs_mps);

        _casing_density_kgpm3 = new_density_kgpm3;

    }
}

// end of vertical_speed_indicator.cxx
