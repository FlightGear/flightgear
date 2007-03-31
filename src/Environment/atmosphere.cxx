#include "atmosphere.hxx"

using namespace std;
#include <iostream>

FGAtmoCache::FGAtmoCache() :
    a_tvs_p(0)
{}

FGAtmoCache::~FGAtmoCache() {
    delete a_tvs_p;
}

// Pressure as a function of height.
// Valid below 32000 meters,
//   i.e. troposphere and first two layers of stratosphere.
// Does not depend on any caching; can be used to
// *construct* caches and interpolation tables.
//
// Height in meters, pressure in pascals.

double FGAtmo::p_vs_a(const double height) {
    using namespace atmodel;
    if (height <= 11000.) {
        return P_layer(height, 0.0, ISA::P0, ISA::T0, ISA::lam0);
    } else if (height <= 20000.) {
        return P_layer(height, 11000., 22632.06, 216.65, 0.0);
    } else if (height <= 32000.) {
        return P_layer(height, 20000.,  5474.89, 216.65, -0.001);
    }
    return 0;
}

// degrees C, height in feet
double FGAtmo::fake_t_vs_a_us(const double h_ft) {
    using namespace atmodel;
    return ISA::T0 - ISA::lam0 * h_ft * foot - freezing;
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
        double press = p_vs_a(hgt);
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

// Pressure within a layer, as a function of height.
// Physics model:  standard or nonstandard atmosphere,
//    depending on what parameters you pass in.
// Height in meters, pressures in pascals.
// As always, lapse is positive in the troposphere,
// and zero in the first part of the stratosphere.

double FGAtmo::P_layer(const double height, const double href,
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
        double press = p_vs_a(height);
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

// Altimeter setting.
// Field elevation in feet
// Field pressure in inHg
// field elevation in troposphere only
double FGAtmo::qnh(const double field_ft, const double press_inHg) {
    using namespace atmodel;

    //  Equation derived in altimetry.htm
    // exponent in QNH equation:
    double nn = ISA::lam0 * Rgas / g / mm;
    // pressure ratio factor:
    double prat = pow(ISA::P0/inHg / press_inHg, nn);

    return press_inHg
            * pow(1 + ISA::lam0 * field_ft * foot / ISA::T0 * prat, 1/nn);
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
        if (hgt_ft < -1e6)
            break;
        double press = P_layer(hgt_ft*foot, 0, ISA::P0, Tref, ISA::lam0);
        double p_inHg = press / inHg;
        double qnhx = qnh(hgt_ft, p_inHg);
        double qnh2 = round(qnhx*100)/100;

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
