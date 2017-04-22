#include <stdio.h>

#include <cstring>
#include <cstdlib>

#include <simgear/props/props.hxx>
#include <simgear/xml/easyxml.hxx>
#include <simgear/misc/sg_path.hxx>

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

static const float RAD2DEG = 57.2957795131;
static const float DEG2RAD = 0.0174532925199;
/// knots 2 meters per second
static const float KTS2MPS = 0.514444444444;


enum Config 
{
  CONFIG_NONE,
  CONFIG_APPROACH,
  CONFIG_CRUISE,
};

// Generate a graph of lift, drag and L/D against AoA at the specified
// speed and altitude.  The result is a space-separated file of
// numbers: "aoa lift drag LD" (aoa in degrees, lift and drag in
// G's).  You can use this in gnuplot like so (assuming the output is
// in a file named "dat":
/*
 plot "dat" using 1:2 with lines title 'lift', \ 
      "dat" using 1:3 with lines title 'drag', \ 
      "dat" using 1:4 with lines title 'LD'
*/
void yasim_graph(Airplane* a, const float alt, const float kts, int cfg = CONFIG_NONE)
{
  Model* m = a->getModel();
  State s;

  m->setAirFromStandardAtmosphere(alt);

  switch (cfg) {
    case CONFIG_APPROACH:
      a->loadApproachControls();
      break;
    case CONFIG_CRUISE:
      a->loadCruiseControls();
      break;
    case CONFIG_NONE:
      break;
  }
  //if we fake the properties we could also use FGFDM::getExternalInput()

  m->getBody()->recalc();
  float cl_max = 0, cd_min = 1e6, ld_max = 0;
  int cl_max_deg = 0, cd_min_deg = 0, ld_max_deg = 0;

  for(int deg=-15; deg<=90; deg++) {
    float aoa = deg * DEG2RAD;
    a->setupState(aoa, kts * KTS2MPS, 0 ,&s);
    m->getBody()->reset();
    m->initIteration();
    m->calcForces(&s);

    float acc[3];
    m->getBody()->getAccel(acc);
    Math::tmul33(s.orient, acc, acc);

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
    printf("%d %g %g %g\n", deg, lift, drag, ld);
  }
  printf("# cl_max %g at %d deg\n", cl_max, cl_max_deg);
  printf("# cd_min %g at %d deg\n", cd_min, cd_min_deg);
  printf("# ld_max %g at %d deg\n", ld_max, ld_max_deg);  
}

void yasim_masses(Airplane* a)
{
  RigidBody* body = a->getModel()->getBody();
  int i, N = body->numMasses();
  float pos[3];
  float m, mass = 0;
  printf("id posx posy posz mass\n");
  for (i = 0; i < N; i++)
  {
    body->getMassPosition(i, pos);
    m = body->getMass(i);
    printf("%d %.3f %.3f %.3f %.3f\n", i, pos[0], pos[1], pos[2], m);
    mass += m;
  }
  printf("Total mass: %g", mass); 
}

void yasim_drag(Airplane* a, const float aoa, const float alt, int cfg = CONFIG_NONE)
{
  fprintf(stderr,"yasim_drag");
  Model* m = a->getModel();
  State s;
  
  m->setAirFromStandardAtmosphere(alt);
  
  switch (cfg) {
    case CONFIG_APPROACH:
      a->loadApproachControls();
      break;
    case CONFIG_CRUISE:
      a->loadCruiseControls();
      break;
    case CONFIG_NONE:
      break;
  }
  
  m->getBody()->recalc();
  float cd_min = 1e6;
  int cd_min_kts = 0;
  printf("#kts, drag\n");
  
  for(int kts=15; kts<=150; kts++) {
    a->setupState(aoa, kts * KTS2MPS, 0 ,&s);
    m->getBody()->reset();
    m->initIteration();
    m->calcForces(&s);
    
    float acc[3];
    m->getBody()->getAccel(acc);
    Math::tmul33(s.orient, acc, acc);
    
    float drag = acc[0] * (-1/9.8);
    
    if (cd_min > drag) {
      cd_min = drag;
      cd_min_kts = kts;
    }
    printf("%d %g\n", kts, drag);
  }
  printf("# cd_min %g at %d kts\n", cd_min, cd_min_kts);
}

int usage()
{
  fprintf(stderr, "Usage: \n");
  fprintf(stderr, "  yasim <aircraft.xml> [-g [-a meters] [-s kts] [-approach | -cruise] ]\n");
  fprintf(stderr, "  yasim <aircraft.xml> [-d [-a meters] [-approach | -cruise] ]\n");
  fprintf(stderr, "  yasim <aircraft.xml> [-m]\n");
  fprintf(stderr, "                       -g print lift/drag table: aoa, lift, drag, lift/drag \n");
  fprintf(stderr, "                       -d print drag over TAS: kts, drag\n");
  fprintf(stderr, "                       -a set altitude in meters!\n");
  fprintf(stderr, "                       -s set speed in knots\n");
  fprintf(stderr, "                       -m print mass distribution table: id, x, y, z, mass \n");
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
  a->compile();
  if(a->getFailureMsg())
      printf("SOLUTION FAILURE: %s\n", a->getFailureMsg());
  if(!a->getFailureMsg() && argc > 2 ) {
    if(strcmp(argv[2], "-g") == 0) {
      float alt = 5000, kts = 100;
      int cfg = CONFIG_NONE;
      for(int i=3; i<argc; i++) {
        if (std::strcmp(argv[i], "-a") == 0) {
          if (i+1 < argc) alt = std::atof(argv[++i]);
        }
        else if(std::strcmp(argv[i], "-s") == 0) {
          if(i+1 < argc) kts = std::atof(argv[++i]);
        }
        else if(std::strcmp(argv[i], "-approach") == 0) cfg = CONFIG_APPROACH;
        else if(std::strcmp(argv[i], "-cruise") == 0) cfg = CONFIG_CRUISE;
        else return usage();
      }
      yasim_graph(a, alt, kts, cfg);
    } 
    else if(strcmp(argv[2], "-d") == 0) {
      float alt = 2000, aoa = a->getCruiseAoA();
      int cfg = CONFIG_NONE;
      for(int i=3; i<argc; i++) {
	if (std::strcmp(argv[i], "-a") == 0) {
	  if (i+1 < argc) alt = std::atof(argv[++i]);
	}
	else if(std::strcmp(argv[i], "-approach") == 0) cfg = CONFIG_APPROACH;
	else if(std::strcmp(argv[i], "-cruise") == 0) cfg = CONFIG_CRUISE;
	else return usage();
      }
      yasim_drag(a, aoa, alt, cfg);
    } 
    else if(strcmp(argv[2], "-m") == 0) {
      yasim_masses(a);
    }
  }
  else {
    printf("==========================\n");
    printf("= YASim solution results =\n");
    printf("==========================\n");
    float aoa = a->getCruiseAoA() * RAD2DEG;
    float tail = -1 * a->getTailIncidence() * RAD2DEG;
    float drag = 1000 * a->getDragCoefficient();
    float cg[3];
    a->getModel()->getBody()->getCG(cg);
    a->getModel()->getBody()->recalc();

    float SI_inertia[9];
    a->getModel()->getBody()->getInertiaMatrix(SI_inertia);
    float MAC = a->getWing()->getMAC();
    float MACx = a->getWing()->getMACx();
    float MACy = a->getWing()->getMACy();
    
    printf("       Iterations: %d\n", a->getSolutionIterations());
    printf(" Drag Coefficient: %f\n", drag);
    printf("       Lift Ratio: %f\n", a->getLiftRatio());
    printf("       Cruise AoA: %f deg\n", aoa);
    printf("   Tail Incidence: %f deg\n", tail);
    printf("Approach Elevator: %f\n\n", a->getApproachElevator());
    printf("               CG: x:%.3f, y:%.3f, z:%.3f\n", cg[0], cg[1], cg[2]);
    printf("    Wing MAC (*1): x:%.2f, y:%.2f, length:%.1f \n", MACx, MACy, MAC);
    printf("    CG-x rel. MAC: %.3f\n", a->getCGMAC());
    printf("    CG-x  desired: %.3f < %.3f < %.3f \n", a->getCGSoftLimitXMin(), cg[0], a->getCGSoftLimitXMax());
    
    printf("\nInertia tensor [kg*m^2], origo at CG:\n\n");
    printf("  %7.3f, %7.3f, %7.3f\n", SI_inertia[0], SI_inertia[1], SI_inertia[2]);
    printf("  %7.3f, %7.3f, %7.3f\n", SI_inertia[3], SI_inertia[4], SI_inertia[5]);
    printf("  %7.3f, %7.3f, %7.3f\n", SI_inertia[6], SI_inertia[7], SI_inertia[8]);
    printf("\n(*1) MAC calculation works on <wing> only! Numbers will be wrong for segmented wings, e.g. <wing>+<mstab>.\n");
  }
    delete fdm;
    return 0;
}
