
#ifndef _RECORDER_H
#define _RECORDER_H

#include "uiuc_parsefile.h"
#include "uiuc_aircraft.h"
#include <FDM/LaRCsim/ls_generic.h>
#include <FDM/LaRCsim/ls_cockpit.h>
#include <FDM/LaRCsim/ls_constants.h>

#ifdef __cplusplus
extern "C" {
#endif

extern double Simtime;

#ifdef __cplusplus
}
#endif

void uiuc_recorder(double dt );

#endif //_RECORDER_H
