#include <stdio.h>

#include <simgear/props/props.hxx>
#include <simgear/xml/easyxml.hxx>

#include "FGFDM.hpp"
#include "Atmosphere.hpp"
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
static const float DEG2RAD = 0.0174532925199;
static const float KTS2MPS = 0.514444444444;


// Generate a graph of lift, drag and L/D against AoA at the specified
// speed and altitude.  The result is a space-separated file of
// numbers: "aoa lift drag LD" (aoa in degrees, lift and drag in
// G's).  You can use this in gnuplot like so (assuming the output is
// in a file named "dat":
//
// plot "dat" using 1:2 with lines title 'lift', \ 
//      "dat" using 1:3 with lines title 'drag', \ 
//      "dat" using 1:4 with lines title 'LD'
//
void yasim_graph(Airplane* a, float alt, float kts)
{
    Model* m = a->getModel();
    State s;

    m->setAir(Atmosphere::getStdPressure(alt),
              Atmosphere::getStdTemperature(alt),
              Atmosphere::getStdDensity(alt));
    m->getBody()->recalc();

    for(int deg=-179; deg<=179; deg++) {
        float aoa = deg * DEG2RAD;
        Airplane::setupState(aoa, kts * KTS2MPS, &s);
        m->getBody()->reset();
        m->initIteration();
        m->calcForces(&s);

        float acc[3];
	m->getBody()->getAccel(acc);
        Math::tmul33(s.orient, acc, acc);

        float drag = acc[0] * (-1/9.8);
        float lift = 1 + acc[2] * (1/9.8);

        printf("%d %g %g %g\n", deg, lift, drag, lift/drag);
    }
}

int usage()
{
    fprintf(stderr, "Usage: yasim <ac.xml> [-g [-a alt] [-s kts]]\n");
    return 1;
}

int main(int argc, char** argv)
{
    FGFDM* fdm = new FGFDM();
    Airplane* a = fdm->getAirplane();

    if(argc < 2) return usage();

    // Read
    try {
        string file = argv[1];
        readXML(file, *fdm);
    } catch (const sg_exception &e) {
        printf("XML parse error: %s (%s)\n",
               e.getFormattedMessage().c_str(), e.getOrigin().c_str());
    }

    // ... and run
    a->compile();
    if(a->getFailureMsg())
        printf("SOLUTION FAILURE: %s\n", a->getFailureMsg());

    if(!a->getFailureMsg() && argc > 2 && strcmp(argv[2], "-g") == 0) {
        float alt = 5000, kts = 100;
        for(int i=3; i<argc; i++) {
            if     (strcmp(argv[i], "-a") == 0) alt = atof(argv[++i]);
            else if(strcmp(argv[i], "-s") == 0) kts = atof(argv[++i]);
            else return usage();
        }
        yasim_graph(a, alt, kts);
    } else {
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
    }
    delete fdm;
    return 0;
}
