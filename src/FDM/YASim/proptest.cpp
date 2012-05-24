#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>

#include "Math.hpp"
#include "FGFDM.hpp"
#include "PropEngine.hpp"
#include "Propeller.hpp"
#include "Atmosphere.hpp"

using namespace yasim;

// Usage: proptest plane.xml [alt-ft] [spd-ktas]

// Stubs.  Not needed by a batch program, but required to link.
class SGPropertyNode;
bool fgSetFloat (const char * name, float val) { return false; }
bool fgSetBool(char const * name, bool val) { return false; }
bool fgGetBool(char const * name, bool def) { return false; }
bool fgSetString(char const * name, char const * str) { return false; }
SGPropertyNode* fgGetNode (const char * path, bool create) { return 0; }
SGPropertyNode* fgGetNode (const char * path, int i, bool create) { return 0; }
float fgGetFloat (const char * name, float defaultValue) { return 0; }
double fgGetDouble (const char * name, double defaultValue) { return 0; }
bool fgSetDouble (const char * name, double defaultValue) { return false; }

static const float KTS2MPS = 0.514444444444;
static const float RPM2RAD = 0.10471975512;
static const float HP2W = 745.700;
static const float FT2M = 0.3048;
static const float N2LB = 0.224809;

// static const float DEG2RAD = 0.0174532925199;
// static const float LBS2N = 4.44822;
// static const float LBS2KG = 0.45359237;
// static const float KG2LBS = 2.2046225;
// static const float CM2GALS = 264.172037284;
// static const float INHG2PA = 3386.389;
// static const float K2DEGF = 1.8;
// static const float K2DEGFOFFSET = -459.4;
// static const float CIN2CM = 1.6387064e-5;
// static const float YASIM_PI = 3.14159265358979323846;

const int COUNT = 100;

int main(int argc, char** argv)
{
    FGFDM fdm;

    // Read
    try {
        readXML(argv[1], fdm);
    } catch (const sg_exception &e) {
        printf("XML parse error: %s (%s)\n",
               e.getFormattedMessage().c_str(), e.getOrigin());
    }

    Airplane* airplane = fdm.getAirplane();
    PropEngine* pe = airplane->getThruster(0)->getPropEngine();
    Propeller* prop = pe->getPropeller();
    Engine* eng = pe->getEngine();

    pe->init();
    pe->setMixture(1);
    pe->setFuelState(true);
    eng->setBoost(1);

    float alt = (argc > 2 ? atof(argv[2]) : 0) * FT2M;
    pe->setAir(Atmosphere::getStdPressure(alt),
               Atmosphere::getStdTemperature(alt),
               Atmosphere::getStdDensity(alt));
 
    float speed = (argc > 3 ? atof(argv[3]) : 0) * KTS2MPS;
    float wind[3];
    wind[0] = -speed; wind[1] = wind[2] = 0;
    pe->setWind(wind);

    printf("Alt: %f\n", alt / FT2M);
    printf("Spd: %f\n", speed / KTS2MPS);
    for(int i=0; i<COUNT; i++) {
        float throttle = i/(COUNT-1.0);
        pe->setThrottle(throttle);
        pe->stabilize();

        float rpm = pe->getOmega() * (1/RPM2RAD);

        float tmp[3];
        pe->getThrust(tmp);
        float thrust = Math::mag3(tmp);

        float power = pe->getOmega() * eng->getTorque();

        float eff = thrust * speed / power;

        printf("%6.3f: %6.1frpm %6.1flbs %6.1fhp %6.1f%% torque: %f\n",
               throttle, rpm, thrust * N2LB, power * (1/HP2W), 100*eff, eng->getTorque());
    }

    printf("\n");
    printf("Propeller vs. RPM\n");
    printf("-----------------\n");
    for(int i=0; i<COUNT; i++) {
        float thrust, torque, rpm = 3000 * i/(COUNT-1.0);
        float omega = rpm * RPM2RAD;
        prop->calc(Atmosphere::getStdDensity(alt),
                   speed, omega, &thrust, &torque);
        float power = torque * omega;
        float eff = (thrust * speed) / power;
        printf("%6.1frpm %6.1flbs %6.1fhp %.1f%% torque: %f\n",
               rpm, thrust * N2LB, power * (1/HP2W), 100*eff, torque);
    }
}
