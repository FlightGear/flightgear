#include "FGFDMExec.h"
#include "FGRotation.h"
#include "FGAtmosphere.h"
#include "FGState.h"
#include "FGFCS.h"
#include "FGAircraft.h"
#include "FGTranslation.h"
#include "FGPosition.h"
#include "FGAuxiliary.h"
#include "FGOutput.h"

#include <iostream.h>
#include <time.h>

void main(int argc, char** argv)
{
  struct timespec short_wait = {0,100000000};
  struct timespec no_wait    = {0,100000000};

  if (argc != 3) {
    cout << endl
         << "  You must enter the name of a registered aircraft and reset point:"
         << endl << endl << "  FDM <aircraft name> <reset file>" << endl;
    exit(0);
  }

  FDMExec = new FGFDMExec();

  FDMExec->GetAircraft()->LoadAircraft(argv[1]);
  FDMExec->GetState()->Reset(argv[2]);

  while (FDMExec->GetState()->Getsim_time() <= 25.0)
  {
    FDMExec->Run();
    nanosleep(&short_wait,&no_wait);
  }

  delete FDMExec;
}
