#include <boost/tuple/tuple.hpp>

#include <simgear/math/SGMath.hxx>
#include <simgear/debug/logstream.hxx>

#include "atmosphere.hxx"

using namespace std;
#include <iostream>

const ISA_layer ISA_def[] = {
//        0    1        2        3           4         5      6         7         8
//       id   (m)      (ft)     (Pa)        (inHg)    (K)     (C)      (K/m)     (K/ft)
ISA_layer(0,      0,       0,   101325,    29.92126, 288.15,  15.00,  0.0065,   0.0019812),
ISA_layer(1,  11000,   36089,  22632.1,    6.683246, 216.65, -56.50,       0,           0),
ISA_layer(2,  20000,   65616,  5474.89,    1.616734, 216.65, -56.50, -0.0010,  -0.0003048),
ISA_layer(3,  32000,  104986,  868.019,    0.256326, 228.65, -44.50, -0.0028,  -0.0008534),
ISA_layer(4,  47000,  154199,  110.906,   0.0327506, 270.65,  -2.50,       0,           0),
ISA_layer(5,  51000,  167322,  66.9389,   0.0197670, 270.65,  -2.50,  0.0028,   0.0008534),
ISA_layer(6,  71000,  232939,  3.95642,  0.00116833, 214.65, -58.50,  0.0020,   0.0006096),
ISA_layer(7,  80000,  262467,  0.88628, 0.000261718, 196.65, -76.50),
};

const int ISA_def_size(sizeof(ISA_def) / sizeof(ISA_layer));

// Pressure within a layer, as a function of height.
// Physics model:  standard or nonstandard atmosphere,
//    depending on what parameters you pass in.
// Height in meters, pressures in pascals.
// As always, lapse is positive in the troposphere,
// and zero in the first part of the stratosphere.

double P_layer(const double height, const double href,
        const double Pref, const double Tref,
        const double lapse) {
    using namespace atmodel;
    if (lapse) {
        double N = lapse * Rgas / mm / g;
        return Pref * pow( (Tref - lapse*(height - href)) / Tref , (1/N));
    } else {
        return Pref * exp(-g * mm / Rgas / Tref * (height - href));
    }
}


// Temperature within a layer, as a function of height.
// Physics model:  standard or nonstandard atmosphere
//  depending on what parameters you pass in.
// $hh in meters, pressures in Pa.
// As always, $lambda is positive in the troposphere,
// and zero in the first part of the stratosphere.
double T_layer (
          const double hh, 
          const double hb, 
          const double Pb, 
          const double Tb, 
          const double lambda) {
  return Tb - lambda*(hh - hb);
}

// Pressure and temperature as a function of height, Psl, and Tsl.
// heights in meters, pressures in Pa.
// Daisy chain version.
// We need "seed" values for sea-level pressure and temperature.
// In addition, for every layer, we need three things
//  from the table: the reference height in that layer, 
//  the lapse in that layer, and the cap (if any) for that layer 
// (which we take from the /next/ row of the table, if any).
pair<double,double> PT_vs_hpt(
      const double hh, 
      const double _p0,
      const double _t0
) {
  
  const double d0(0);
  double hgt = ISA_def[0].height;
  double p0 =  _p0;
  double t0 =  _t0;
#if 0
    cout << "PT_vs_hpt: " << hh << "   " << p0 << "   " << t0 << endl;
#endif 

  int ii = 0;
  for (const ISA_layer* pp = ISA_def; pp->lapse != -1; pp++, ii++) {
#if 0
    cout << "PT_vs_hpt: " << ii
        << "  height: " << pp->height
        << "  temp: "   << pp->temp
        << "  lapse: "  << pp->lapse 
        << endl;
#endif
    double xhgt(9e99);
    double lapse = pp->lapse;
// Stratosphere starts at a definite temperature,
// not a definite height:
    if (ii == 0) {
      xhgt = hgt + (t0 - (pp+1)->temp) / lapse;
    } else if ((pp+1)->lapse != -1) {
      xhgt = (pp+1)->height;      
    }
    if (hh <= xhgt) {
      return make_pair(P_layer(hh, hgt, p0, t0, lapse),
                 T_layer(hh, hgt, p0, t0, lapse));
    }
    p0 = P_layer(xhgt, hgt, p0, t0, lapse);
    t0 = t0 - lapse * (xhgt - hgt);
    hgt = xhgt;
  }
  
// Should never get here.
  SG_LOG(SG_ENVIRONMENT, SG_ALERT, "PT_vs_hpt: ran out of layers for h=" << hh );
  return make_pair(d0, d0);
}


FGAtmoCache::FGAtmoCache() :
    a_tvs_p(0)
{}

FGAtmoCache::~FGAtmoCache() {
    delete a_tvs_p;
}


/////////////
// The following two routines are called "fake" because they
// bypass the exceedingly complicated layer model implied by
// the "weather conditioins" popup menu.
// For now we must bypass it for several reasons, including
// the fact that we don't have an "environment" object for
// the airport (only for the airplane).
// degrees C, height in feet
double FGAtmo::fake_T_vs_a_us(const double h_ft, 
                const double Tsl) const {
    using namespace atmodel;
    return Tsl - ISA::lam0 * h_ft * foot;
}

// Dewpoint.  degrees C or K, height in feet
double FGAtmo::fake_dp_vs_a_us(const double dpsl, const double h_ft) {
    const double dp_lapse(0.002);	// [K/m] approximate
    // Reference:  http://en.wikipedia.org/wiki/Lapse_rate
    return dpsl - dp_lapse * h_ft * atmodel::foot;
}

// Height as a function of pressure.
// Valid in the troposphere only.
double FGAtmo::a_vs_p(const double press, const double qnh) {
    using namespace atmodel;
    using namespace ISA;
    double nn = lam0 * Rgas / g / mm;
    return T0 * ( pow(qnh/P0,nn) - pow(press/P0,nn) ) / lam0;
}

// force retabulation
void FGAtmoCache::tabulate() {
    using namespace atmodel;
    delete a_tvs_p;
    a_tvs_p = new SGInterpTable;

    for (double hgt = -1000; hgt <= 32000;) {
        double press,temp;
        boost::tie(press, temp) = PT_vs_hpt(hgt);
        a_tvs_p->addEntry(press / inHg, hgt / foot);

#ifdef DEBUG_EXPORT_P_H
        char buf[100];
        char* fmt = " { %9.2f  , %5.0f },";
        if (press < 10000) fmt = " {  %9.3f , %5.0f },";
        snprintf(buf, 100, fmt, press, hgt);
        cout << buf << endl;
#endif
        if (hgt < 6000) {
            hgt += 500;
        } else {
            hgt += 1000;
        }
    }
}

// make sure cache is valid
void FGAtmoCache::cache() {
    if (!a_tvs_p)
        tabulate();
}

// Check the basic function,
// then compare against the interpolator.
void FGAtmoCache::check_model() {
    double hgts[] = {
        -1000,
        -250,
        0,
        250,
        1000,
        5250,
        11000,
        11000.00001,
        15500,
        20000,
        20000.00001,
        25500,
        32000,
        32000.00001,
       -9e99
    };

    for (int i = 0; ; i++) {
        double height = hgts[i];
        if (height < -1e6)
            break;
        using namespace atmodel;
        cache();
        double press,temp;
        boost::tie(press, temp) = PT_vs_hpt(height);
        cout << "Height: " << height
             << " \tpressure: " << press << endl;
        cout << "Check:  "
             << a_tvs_p->interpolate(press / inHg)*foot << endl;
    }
}

//////////////////////////////////////////////////////////////////////

FGAltimeter::FGAltimeter() : kset(atmodel::ISA::P0), kft(0)
{
    cache();
}

double FGAltimeter::reading_ft(const double p_inHg, const double set_inHg) {
    using namespace atmodel;
    double press_alt      = a_tvs_p->interpolate(p_inHg);
    double kollsman_shift = a_tvs_p->interpolate(set_inHg);
    return (press_alt - kollsman_shift);
}

// Altimeter setting _in pascals_ 
//  ... caller gets to convert to inHg or millibars
// Field elevation in m
// Field pressure in pascals
// Valid for fields within the troposphere only.
double FGAtmo::QNH(const double field_elev, const double field_press) {
    using namespace atmodel;

    //  Equation derived in altimetry.htm
    // exponent in QNH equation:
    double nn = ISA::lam0 * Rgas / g / mm;
    // pressure ratio factor:
    double prat = pow(ISA::P0 / field_press, nn);
    double rslt =  field_press
            * pow(1. + ISA::lam0 * field_elev / ISA::T0 * prat, 1./nn);
#if 0
    SG_LOG(SG_ENVIRONMENT, SG_ALERT, "QNH: elev: " << field_elev
                << "  press: " << field_press
                << "  prat: "  << prat
                << "  rslt: " << rslt
                << "  inHg: " << inHg
                << "  rslt/inHG: " << rslt/inHg);
#endif
    return rslt;
}

// Invert the QNH calculation to get the field pressure from a metar
// report.
// field pressure _in pascals_ 
//  ... caller gets to convert to inHg or millibars
// Field elevation in m
// Altimeter setting (QNH) in pascals
// Valid for fields within the troposphere only.
double FGAtmo::fieldPressure(const double field_elev, const double qnh)
{
    using namespace atmodel;
    static const double nn = ISA::lam0 * Rgas / g / mm;
    const double pratio = pow(qnh / ISA::P0, nn);
    return ISA::P0 * pow(pratio - field_elev * ISA::lam0 / ISA::T0, 1.0 / nn);
}

void FGAltimeter::dump_stack1(const double Tref) {
    using namespace atmodel;
    const int bs(200);
    char buf[bs];
    double Psl = P_layer(0, 0, ISA::P0, Tref, ISA::lam0);
    snprintf(buf, bs, "Tref: %6.2f  Psl:  %5.0f = %7.4f",
                         Tref,        Psl,     Psl / inHg);
    cout << buf << endl;

    snprintf(buf, bs,
        " %6s  %6s  %6s  %6s   %6s  %6s   %6s",
        "A", "Aind", "Apr", "Aprind", "P", "Psl", "Qnh");
    cout << buf << endl;

    double hgts[] = {0, 2500, 5000, 7500, 10000, -9e99};
    for (int ii = 0; ; ii++) {
        double hgt_ft = hgts[ii];
        double hgt = hgt_ft * foot;
        if (hgt_ft < -1e6)
            break;
        double press = P_layer(hgt, 0, ISA::P0, Tref, ISA::lam0);
        double qnhx = QNH(hgt, press) / inHg;
        double qnh2 = SGMiscd::round(qnhx*100)/100;

        double p_inHg = press / inHg;
        double Aprind = reading_ft(p_inHg);
        double Apr    = a_vs_p(p_inHg*inHg) / foot;
        double hind = reading_ft(p_inHg, qnh2);
        snprintf(buf, bs,
            " %6.0f  %6.0f  %6.0f  %6.0f   %6.2f  %6.2f   %6.2f",
            hgt_ft,  hind,   Apr,  Aprind,  p_inHg, Psl/inHg, qnh2);
        cout << buf << endl;
    }
}


void FGAltimeter::dump_stack() {
    using namespace atmodel;
    cout << "........." << endl;
    cout << "Size: " << sizeof(FGAtmo) << endl;
    dump_stack1(ISA::T0);
    dump_stack1(ISA::T0 - 20);
}
