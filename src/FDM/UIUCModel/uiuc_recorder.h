
#ifndef _RECORDER_H
#define _RECORDER_H

#include "uiuc_parsefile.h"
#include "uiuc_aircraft.h"
#include <FDM/LaRCsim/ls_generic.h>
#include <FDM/LaRCsim/ls_cockpit.h>
#include <FDM/LaRCsim/ls_constants.h>

extern double Simtime;

void uiuc_recorder(double dt );

#endif //_RECORDER_H
