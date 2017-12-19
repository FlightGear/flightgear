#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>

#include "yasim-common.hpp"
#include "Math.hpp"
#include "FGFDM.hpp"
#include "PropEngine.hpp"
#include "Propeller.hpp"
#include "Atmosphere.hpp"

#include <simgear/misc/sg_path.hxx>

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

const int COUNT = 100;

int main(int argc, char** argv)
{
    FGFDM fdm;

    // Read
    try {
        readXML(SGPath::fromLocal8Bit(argv[1]), fdm);
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
    pe->setStandardAtmosphere(alt);
 
    float speed = (argc > 3 ? atof(argv[3]) : 0) * KTS2MPS;
    float wind[3];
    wind[0] = -speed; wind[1] = wind[2] = 0;
    pe->setWind(wind);

    printf("Alt: %f\n", alt / FT2M);
    printf("Spd: %f\n", speed / KTS2MPS);
    printf("-----------------\n");
    printf("Throt   RPM   thrustlbs      HP   eff %%   torque\n");
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

        printf("%5.3f %7.1f %8.1f %8.1f %7.1f %8.1f\n",
               throttle, rpm, thrust * N2LB, power * (1/HP2W), 100*eff, eng->getTorque());
    }

    printf("\n");
    printf("Propeller vs. RPM\n");
    printf("-----------------\n");
    printf("RPM       thrustlbs         HP   eff %%      torque\n");
    for(int i=0; i<COUNT; i++) {
        float thrust, torque, rpm = 3000 * i/(COUNT-1.0);
        float omega = rpm * RPM2RAD;
        prop->calc(Atmosphere::getStdDensity(alt),
                   speed, omega, &thrust, &torque);
        float power = torque * omega;
        float eff = (thrust * speed) / power;
        printf("%7.1f %11.1f %10.1f %7.1f %11.1f\n",
               rpm, thrust * N2LB, power * (1/HP2W), 100*eff, torque);
    }
}
