#ifndef _ICED_NONLIN_H_
#define _ICED_NONLIN_H_

#include "uiuc_aircraft.h"
#include <FDM/LaRCsim/ls_generic.h>
#include <FDM/LaRCsim/ls_constants.h>   /* RAD_TO_DEG, DEG_TO_RAD*/

void Calc_Iced_Forces();
void add_ice_effects();

#endif // _ICED_NONLIN_H_
