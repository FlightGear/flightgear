#ifndef _ENGINE_H_
#define _ENGINE_H_

#include "uiuc_aircraft.h"
#include "uiuc_1Dinterpolation.h"
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

void uiuc_engine();
#endif // _ENGINE_H_
