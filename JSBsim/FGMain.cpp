#define FDM_MAIN
#include "FGFDMExec.h"
#undef FDM_MAIN

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

  Aircraft->LoadAircraft(argv[1]);
  State->Reset(argv[2]);

  while (State->Setsim_time(State->Getsim_time() + 0.1) <= 25.0)
  {
    FDMExec->Run();
    nanosleep(&short_wait,&no_wait);
  }

  delete FDMExec;
}
