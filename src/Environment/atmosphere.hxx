// atmosphere.hxx -- routines to model the air column
//
// Written by David Megginson, started February 2002.
// Modified by John Denker to correct physics errors in 2007
//
// Copyright (C) 2002  David Megginson - david@megginson.com
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$


#ifndef _ATMOSPHERE_HXX
#define _ATMOSPHERE_HXX

#include <simgear/compiler.h>
#include <simgear/math/interpolater.hxx>

#include <cmath>

using namespace std;

/**
 * Model the atmosphere in a way consistent with the laws
 * of physics.
 *
 * Each instance of this class models a particular air mass.
 * You may freely move up, down, or sideways in the air mass.
 * In contrast, if you want to compare different air masses,
 * you should use a separate instance for each one.
 *
 * See also ./environment.hxx
 */

#define SCD(name,val) const double name(val)
namespace atmodel {
    SCD(g, 9.80665);		// [m/s/s] acceleration of gravity
    SCD(mm, .0289644);		// [kg/mole] molar mass of air (dry?)
    SCD(Rgas, 8.31432);		// [J/K/mole] gas constant
    SCD(inch, 0.0254);		// [m] definition of inch
    SCD(foot, 12 * inch);		// [m]
    SCD(inHg, 101325.0 / 760 * 1000 * inch);  // [Pa] definition of inHg
    SCD(freezing, 273.15);	// [K] centigrade - kelvin offset
    SCD(nm, 1852);		// [m] nautical mile (NIST)
    SCD(sm, 5280*foot);		// [m] nautical mile (NIST)

    namespace ISA {
        SCD(P0, 101325.0); 		// [pascals] ISA sea-level pressure
        SCD(T0, 15. + freezing);	// [K] ISA sea-level temperature
        SCD(lam0, .0065);		// [K/m] ISA troposphere lapse rate
    }
}
#undef SCD



// The base class is little more than a namespace.
// It has no constructor, no destructor, and no variables.
class FGAtmo {
public:
    double p_vs_a(const double height);
    double a_vs_p(const double press, const double qnh = atmodel::ISA::P0);
    double fake_t_vs_a_us(const double h_ft);
    double fake_dp_vs_a_us(const double dpsl, const double h_ft);
    double P_layer(const double height, const double href,
    const double Pref, const double Tref,
    const double lapse );
    void check_one(const double height);
    double qnh(const double field_ft, const double press_in);
};



class FGAtmoCache : FGAtmo {
    friend class FGAltimeter;
    SGInterpTable * a_tvs_p;	// _tvs_ means "tabulated versus"

public:
    FGAtmoCache();
    ~FGAtmoCache();
    void tabulate();
    void cache();
    void check_model();  // debug
};



class FGAltimeter : public FGAtmoCache {
    double kset;
    double kft;

public:
    FGAltimeter();
    double reading_ft(const double p_inHg,
            const double set_inHg = atmodel::ISA::P0/atmodel::inHg);
    inline double press_alt_ft(const double p_inHg) {
        return a_tvs_p->interpolate(p_inHg);
    }
    inline double kollsman_ft(const double set_inHg) {
        return a_tvs_p->interpolate(set_inHg);
    }

    // debug
    void dump_stack();
    void dump_stack1(const double Tref);
};

#endif // _ATMOSPHERE_HXX
