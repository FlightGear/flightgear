#include <stdio.h>

#include <cstring>
#include <cstdlib>

#include <simgear/props/props.hxx>
#include <simgear/xml/easyxml.hxx>
#include <simgear/misc/sg_path.hxx>

#include "yasim-common.hpp"
#include "FGFDM.hpp"
#include "Atmosphere.hpp"
#include "RigidBody.hpp"
#include "Airplane.hpp"

using namespace yasim;
using std::string;

// Stubs.  Not needed by a batch program, but required to link.
bool fgSetFloat (const char * name, float val) { return false; }
bool fgSetBool(char const * name, bool val) { return false; }
bool fgGetBool(char const * name, bool def) { return false; }
bool fgSetString(char const * name, char const * str) { return false; }
SGPropertyNode* fgGetNode (const char * path, bool create) { return 0; }
SGPropertyNode* fgGetNode (const char * path, int i, bool create) { return 0; }
float fgGetFloat (const char * name, float defaultValue) { return 0; }
double fgGetDouble (const char * name, double defaultValue = 0.0) { return 0; }
bool fgSetDouble (const char * name, double defaultValue = 0.0) { return 0; }


void _setup(Airplane* a, Airplane::Configuration cfgID, float altitude)
{
    Model* m = a->getModel();
    m->setStandardAtmosphere(altitude);
    switch (cfgID) {
        case Airplane::APPROACH:
            fprintf(stderr,"Setting approach controls.\n");
            a->setApproachControls();
            break;
        case Airplane::CRUISE:
            fprintf(stderr,"Setting cruise controls.\n");
            a->setCruiseControls();
            break;
        default:
            break;
    }
    m->getBody()->recalc();    
};

void _calculateAcceleration(Airplane* a, float aoa_rad, float speed_mps, float* output)
{
    Model* m = a->getModel();
    State s;
    s.setupState(aoa_rad, speed_mps, 0);
    m->getBody()->reset();
    m->initIteration();
    m->calcForces(&s);    
    m->getBody()->getAccel(output);
    s.localToGlobal(output, output);    
};

// Generate a graph of lift, drag and L/D against AoA at the specified
// speed and altitude.  The result is a tab-separated file of
// numbers: "aoa lift drag LD" (aoa in degrees, lift and drag in
// G's).  You can use this in gnuplot like so (assuming the output is
// in a file named "dat":
/*
 plot "dat" using 1:2 with lines title 'lift', \ 
      "dat" using 1:3 with lines title 'drag', \ 
      "dat" using 1:4 with lines title 'LD'
*/
void yasim_graph(Airplane* a, const float alt, const float kts, Airplane::Configuration cfgID)
{
    _setup(a, cfgID, alt);
    float speed = kts * KTS2MPS;
    float acc[3] {0,0,0};
    float cl_max = 0, cd_min = 1e6, ld_max = 0;
    int   cl_max_deg = 0, cd_min_deg = 0, ld_max_deg = 0;
    
    printf("aoa\tLift\tDrag\tLvsD\n");
    for(int deg=-5; deg<=29; deg++) {
        float aoa = deg * DEG2RAD;
        _calculateAcceleration(a, aoa, speed, acc);
        float drag = acc[0] * (-1/9.8);
        float lift = 1 + acc[2] * (1/9.8);
        float ld = lift/drag;
        
        if (cd_min > drag) {
            cd_min = drag;
            cd_min_deg = deg;
        }
        if (cl_max < lift) {
            cl_max = lift;
            cl_max_deg = deg;
        }
        if (ld_max < ld) {
            ld_max= ld;
            ld_max_deg = deg;
        }    
        printf("%2d\t%.4f\t%.4f\t%.4f\n", deg, lift, drag, ld);
    }
    printf("# cl_max %.4f at %d deg\n", cl_max, cl_max_deg);
    printf("# cd_min %.4f at %d deg\n", cd_min, cd_min_deg);
    printf("# ld_max %.4f at %d deg\n", ld_max, ld_max_deg);  
}

void yasim_masses(Airplane* a)
{
    RigidBody* body = a->getModel()->getBody();
    int N = body->numMasses();
    float pos[3];
    float m, mass = 0;
    printf("id\tposx\tposy\tposz\tmass\n");
    for (int i = 0; i < N; i++)
    {
        body->getMassPosition(i, pos);
        m = body->getMass(i);
        printf("%d\t%.3f\t%.3f\t%.3f\t%.3f\n", i, pos[0], pos[1], pos[2], m);
        mass += m;
    }
    printf("Total mass: %g", mass); 
}

void yasim_drag(Airplane* a, const float aoa, const float alt, Airplane::Configuration cfgID)
{
    _setup(a, cfgID, alt);
    
    float cd_min = 1e6;
    int cd_min_kts = 0;
    float acc[3] {0,0,0};
    
    printf("#kts, drag\n");    
    for(int kts=15; kts<=150; kts++) {
        _calculateAcceleration(a, aoa,kts * KTS2MPS, acc);        
        float drag = acc[0] * (-1/9.8);        
        if (cd_min > drag) {
            cd_min = drag;
            cd_min_kts = kts;
        }
        printf("%d %g\n", kts, drag);
    }
    printf("# cd_min %g at %d kts\n", cd_min, cd_min_kts);
}

/* Returns AoA for zero lift at specified altitude and speed. */
float yasim_find_zero_lift_aoa(Airplane* a, const float alt, float kts, Airplane::Configuration cfgID, float* acc)
{
    _setup(a, cfgID, alt);
    float   aoa_deg = 0;
    float   aoa_deg_delta = 10;
    while (1) {
        float aoa = aoa_deg * DEG2RAD;
        _calculateAcceleration(a, aoa, kts * KTS2MPS, acc);
        float lift = acc[2];
        if (lift > 0)   aoa_deg -= aoa_deg_delta;
        if (lift < 0)   aoa_deg += aoa_deg_delta;
        aoa_deg_delta /= 2;
        if (aoa_deg_delta < 0.001)    break;
        //printf("aoa_deg=%g\n", aoa_deg);
    }
    //printf("alt=%g kts=%g: aoa_deg=%g\n", alt, kts, aoa_deg);
    return aoa_deg;
}

/* Returns info about best speed at a particular height. */
void yasim_best_speed_at_height(Airplane* a, const float alt, Airplane::Configuration cfgID,
        bool verbose, float& o_best_speed_kts, float& o_best_drag, float& o_best_aoa_deg)
{
    o_best_speed_kts = 0;
    o_best_drag = 1e9;
    o_best_aoa_deg = 0.0f;

    /* Could probably use a gradient-descent method, but for now we just use
    linear list of speeds. */
    for (float kts = 50; kts < 600; kts += 1) {
        float acc[3];
        float aoa_deg = yasim_find_zero_lift_aoa(a, alt, kts, cfgID, acc);
        float aoa = aoa_deg * DEG2RAD;
        float drag = acc[0] / -9.8;
        float idrag = 9.8 * tan(aoa);
        float drag_total = drag + idrag;

        if (verbose) {
            printf("acc=(% 10.4f % 10.2g % 10.4f)."
                    " alt=% 10.4f kts=% 10.4f: aoa_deg=% 10.4f drag=% 10.4f"
                    " idrag=% 10.4f drag_total=% 10.4f\n",
                    acc[0], acc[1], acc[2],
                    alt, kts, aoa_deg, drag,
                    idrag, drag_total);
        }

        if (drag_total < o_best_drag) {
            o_best_speed_kts = kts;
            o_best_drag = drag_total;
            o_best_aoa_deg = aoa_deg;
        }
    }
}

/* Shows best speed at various heights. */
void yasim_show_best_speed_at_heights(Airplane* a, Airplane::Configuration cfgID)
{
    for (int alt_ft=0; alt_ft < 50*1000; alt_ft += 1000) {
        float alt_m = alt_ft * 12.0 * 2.54 / 100.0;
        float best_speed_kts;
        float best_drag;
        float best_aoa_deg;

        yasim_best_speed_at_height(a, alt_m, cfgID, false /*verbose*/,
                best_speed_kts, best_drag, best_aoa_deg);

        printf("altitude=% 6ift: best_speed=%gkts best_aoa=%.2fdeg drag=%g\n",
                alt_ft, best_speed_kts, best_aoa_deg, best_drag);
    }
}


void findMinSpeed(Airplane* a, float alt)
{
    a->addControlSetting(Airplane::CRUISE, "/controls/flight/elevator-trim", 0.7f);
    _setup(a, Airplane::CRUISE, alt);
    float acc[3];

    printf("aoa\tknots\tlift\n");
    for(int deg=0; deg<=20; deg++) {
        float aoa = deg * DEG2RAD;
        for(int kts=15; kts<=180; kts++) {
            _calculateAcceleration(a, aoa, kts * KTS2MPS, acc);
            float lift = acc[2];
            if (lift > 0) {
                printf("%d\t%d\t%f\n", deg, kts, lift);
                break;
            }
        }
    }
}

void report(Airplane* a)
{
    printf("==========================\n");
    printf("= YASim solution results =\n");
    printf("==========================\n");
    float aoa = a->getCruiseAoA() * RAD2DEG;
    float tailIncidence = a->getTailIncidence() * RAD2DEG;
    float drag = 1000 * a->getDragCoefficient();
    float cg[3];
    a->getModel()->getBody()->getCG(cg);
    a->getModel()->getBody()->recalc();

    float SI_inertia[9];
    a->getModel()->getBody()->getInertiaMatrix(SI_inertia);
    float MAC = 0, MACx = 0, MACy = 0;
    float sweepMin = 0, sweepMax = 0;
    Wing* wing {nullptr};
    Wing* tail {nullptr};
    if (a->hasWing()) {
      wing = a->getWing();
      tail = a->getTail();
    }
    printf("Iterations        : %d\n", a->getSolutionIterations());
    printf("Drag Coefficient  : %.3f\n", drag);
    printf("Lift Ratio        : %.3f\n", a->getLiftRatio());
    printf("Cruise AoA        : %.2f deg\n", aoa);
    printf("Tail Incidence    : %.2f deg\n", tailIncidence);
    printf("Approach Elevator : %.3f\n\n", a->getApproachElevator());
    printf("CG                : x:%.3f, y:%.3f, z:%.3f\n", cg[0], cg[1], cg[2]);
    if (wing) {
      MAC = wing->getMACLength();
      MACx = wing->getMACx();
      MACy = wing->getMACy();
      sweepMin = wing->getSweepLEMin() * RAD2DEG;
      sweepMax = wing->getSweepLEMax() * RAD2DEG;
      printf("Wing MAC          : (x:%.2f, y:%.2f), length:%.1f \n", MACx, MACy, MAC);
      printf("hard limit CG-x   : %.3f m\n", a->getCGHardLimitXMax());
      printf("soft limit CG-x   : %.3f m\n", a->getCGSoftLimitXMax());
      printf("CG-x              : %.3f m\n", cg[0]);
      printf("CG-x rel. MAC     : %3.0f%%\n", a->getCGMAC()*100);
      printf("soft limit CG-x   : %.3f m\n", a->getCGSoftLimitXMin());
      printf("hard limit CG-x   : %.3f m\n", a->getCGHardLimitXMin());
      printf("\n");
      printf("wing lever        : %.3f m\n", a->getWingLever());
      printf("tail lever        : %.3f m\n", a->getTailLever());
      printf("\n");
      printf("max thrust        : %.2f kN\n", a->getMaxThrust()/1000);
      printf("thrust/empty      : %.2f\n", a->getThrust2WeightEmpty());
      printf("thrust/mtow       : %.2f\n", a->getThrust2WeightMTOW());
      printf("\n");
      printf("wing span         : %.2f m\n", a->getWingSpan());
      printf("sweep lead. edge  : %.1f .. %.1f deg\n", sweepMin, sweepMax);
      printf("wing area         : %.2f m^2\n", a->getWingArea());
      printf("wing load empty   : %.2f kg/m^2 (Empty %.0f kg)\n", a->getWingLoadEmpty(), a->getEmptyWeight());
      printf("wing load MTOW    : %.2f kg/m^2 (MTOW  %.0f kg)\n", a->getWingLoadMTOW(), a->getMTOW());
      printf("\n");
      printf("tail span         : %.3f m\n", tail->getSpan());
      printf("tail area         : %.3f m^2\n", tail->getArea());
      printf("\n");
      wing->printSectionInfo();
    }
    printf("\nInertia tensor [kg*m^2], origo at CG:\n\n");
    printf("  %7.0f, %7.0f, %7.0f\n", SI_inertia[0], SI_inertia[1], SI_inertia[2]);
    printf("  %7.0f, %7.0f, %7.0f\n", SI_inertia[3], SI_inertia[4], SI_inertia[5]);
    printf("  %7.0f, %7.0f, %7.0f\n", SI_inertia[6], SI_inertia[7], SI_inertia[8]);
}

int usage()
{
    fprintf(stderr, "Usage: \n");
    fprintf(stderr, "  yasim <aircraft.xml> [-g [-a meters] [-s kts] [-approach | -cruise] ]\n");
    fprintf(stderr, "  yasim <aircraft.xml> [-d [-a meters] [-approach | -cruise] ]\n");
    fprintf(stderr, "  yasim <aircraft.xml> [-m]\n");
    fprintf(stderr, "  yasim <aircraft.xml> [-test] [-a meters] [-s kts] [-approach | -cruise] ]\n");
    fprintf(stderr, "                       -g print lift/drag table: aoa, lift, drag, lift/drag \n");
    fprintf(stderr, "                       -d print drag over TAS: kts, drag\n");
    fprintf(stderr, "                       -D print kts at lowest drag at specified altitude\n");
    fprintf(stderr, "                       --aD print kts at lowest drag at different altitudes\n");
    fprintf(stderr, "                       -a set altitude in meters!\n");
    fprintf(stderr, "                       -s set speed in knots\n");
    fprintf(stderr, "                       -m print mass distribution table: id, x, y, z, mass \n");
    fprintf(stderr, "                       -test print summary and output like -g -m \n");
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
        readXML(SGPath(file), *fdm);
    } 
    catch (const sg_exception &e) {
        printf("XML parse error: %s (%s)\n", e.getFormattedMessage().c_str(), e.getOrigin());
    }
    // ... and run
    bool verbose {false};
    if (argc > 2 && strcmp(argv[2], "-v") == 0) {
        verbose=true;
    }
    if ((argc == 4) && (strcmp(argv[2], "--tweak") == 0)) {
        float tweak = std::atof(argv[3]);
        a->setSolverTweak(tweak);
        a->setSolverMaxIterations(2000);
        verbose=true;
    }
    a->compile(verbose);
    if(a->getFailureMsg()) {
        printf("SOLUTION FAILURE: %s\n", a->getFailureMsg());
    }
    if(!a->getFailureMsg() && argc > 2 ) {
        bool test = (strcmp(argv[2], "-test") == 0);
        Airplane::Configuration cfg = Airplane::NONE;
        float alt = 5000, kts = 100;

        if((strcmp(argv[2], "-g") == 0) || test) {
            for(int i=3; i<argc; i++) {
                if (std::strcmp(argv[i], "-a") == 0) {
                    if (i+1 < argc) alt = std::atof(argv[++i]);
                }
                else if(std::strcmp(argv[i], "-s") == 0) {
                    if(i+1 < argc) kts = std::atof(argv[++i]);
                }
                else if(std::strcmp(argv[i], "-approach") == 0) cfg = Airplane::APPROACH;
                else if(std::strcmp(argv[i], "-cruise") == 0) cfg = Airplane::CRUISE;
                else return usage();
            }
            if (test) {
                report(a);
                printf("\n#-- lift, drag at altitude %.0f meters, %.0f knots, Config %d --\n", alt, kts, cfg);
            }
            yasim_graph(a, alt, kts, cfg);
            if (test) {
                printf("\n#-- mass distribution --\n");
                yasim_masses(a);
            }
        } 
        else if(!strcmp(argv[2], "-d") || !strcmp(argv[2], "-D") || !strcmp(argv[2], "--aD")) {
            float alt = 2000;
            for(int i=3; i<argc; i++) {
                if (std::strcmp(argv[i], "-a") == 0) {
                    if (i+1 < argc) alt = std::atof(argv[++i]);
                }
                else if(std::strcmp(argv[i], "-approach") == 0) cfg = Airplane::APPROACH;
                else if(std::strcmp(argv[i], "-cruise") == 0) cfg = Airplane::CRUISE;
                else return usage();
            }
            if (strcmp(argv[2], "-d") == 0) {
                float aoa = a->getCruiseAoA();
                yasim_drag(a, aoa, alt, cfg);
            }
            else if (!strcmp(argv[2], "-D")) {
                float best_speed_kts;
                float best_drag;
                float best_aoa_deg;
                yasim_best_speed_at_height(a, alt, cfg, true /*verbose*/, best_speed_kts, best_drag, best_aoa_deg);
                printf("altitude=%gm: best speed=%g best_aoa_deg=%g drag=%g\n", alt, best_speed_kts, best_aoa_deg, best_drag);
            }
            else if (!strcmp(argv[2], "--aD")) {
                yasim_show_best_speed_at_heights(a, cfg);
            }
        }
        else if(strcmp(argv[2], "-m") == 0) {
            yasim_masses(a);
        }
        else if(strcmp(argv[2], "--min-speed") == 0) {
            alt = 10;
            for(int i=3; i<argc; i++) {
                if (std::strcmp(argv[i], "-a") == 0) {
                    if (i+1 < argc) alt = std::atof(argv[++i]);
                }
                else if(std::strcmp(argv[i], "-approach") == 0) cfg = Airplane::APPROACH;
                else if(std::strcmp(argv[i], "-cruise") == 0) cfg = Airplane::CRUISE;
                else return usage();
            }
            findMinSpeed(a, alt);
        }
    }
    else {
        report(a);
    }
    delete fdm;
    return 0;
}
