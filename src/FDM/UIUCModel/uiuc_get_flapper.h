#ifndef _GET_FLAPPER_H_
#define _GET_FLAPPER_H_

#include "uiuc_flapdata.h"
#include "uiuc_aircraft.h"
#include <FDM/LaRCsim/ls_constants.h>
#include <FDM/LaRCsim/ls_generic.h>
#include <FDM/LaRCsim/ls_cockpit.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

extern double Simtime;

#ifdef __cplusplus
}
#endif

void uiuc_get_flapper(double dt);

#endif //_GET_FLAPPER_H_
