#include <stdio.h>

#include <simgear/props/props.hxx>
#include <simgear/xml/easyxml.hxx>

#include "FGFDM.hpp"
#include "Airplane.hpp"

using namespace yasim;

// Stubs.  Not needed by a batch program, but required to link.
bool fgSetFloat (const char * name, float val) { return false; }
bool fgSetBool(char const * name, bool val) { return false; }
bool fgGetBool(char const * name, bool def) { return false; }
SGPropertyNode* fgGetNode (const char * path, bool create) { return 0; }
SGPropertyNode* fgGetNode (const char * path, int i, bool create) { return 0; }
float fgGetFloat (const char * name, float defaultValue) { return 0; }
float fgGetDouble (const char * name, double defaultValue) { return 0; }
float fgSetDouble (const char * name, double defaultValue) { return 0; }

static const float RAD2DEG = 57.2957795131;

int main(int argc, char** argv)
{
    FGFDM fdm;
    Airplane* a = fdm.getAirplane();

    // Read
    try {
        readXML(argv[1], fdm);
    } catch (const sg_exception &e) {
        printf("XML parse error: %s (%s)\n",
               e.getFormattedMessage().c_str(), e.getOrigin().c_str());
    }

    // ... and run
    a->compile();

    float aoa = a->getCruiseAoA() * RAD2DEG;
    float tail = -1 * a->getTailIncidence() * RAD2DEG;
    float drag = 1000 * a->getDragCoefficient();
    float cg[3];
    a->getModel()->getBody()->getCG(cg);

    printf("Solution results:");
    printf("       Iterations: %d\n", a->getSolutionIterations());
    printf(" Drag Coefficient: %f\n", drag);
    printf("       Lift Ratio: %f\n", a->getLiftRatio());
    printf("       Cruise AoA: %f\n", aoa);
    printf("   Tail Incidence: %f\n", tail);
    printf("Approach Elevator: %f\n", a->getApproachElevator());
    printf("               CG: %.3f, %.3f, %.3f\n", cg[0], cg[1], cg[2]);

    if(a->getFailureMsg())
        printf("SOLUTION FAILURE: %s\n", a->getFailureMsg());
}
