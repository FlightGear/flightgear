#ifndef _GETWIND_H_
#define _GETWIND_H_

#include <math.h>
#include "uiuc_aircraft.h"
#include <FDM/LaRCsim/ls_generic.h> //For global state variables
#include <FDM/LaRCsim/ls_constants.h>

#ifdef __cplusplus
extern "C" {
#endif

extern double Simtime;

#ifdef __cplusplus
}
#endif

void uiuc_getwind();
#endif // _GETWIND_H_
