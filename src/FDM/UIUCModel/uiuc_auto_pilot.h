#ifndef _AUTO_PILOT_H_
#define _AUTO_PILOT_H_

#include "uiuc_aircraft.h"
#include "uiuc_pah_ap.h"
#include "uiuc_alh_ap.h"
#include "uiuc_rah_ap.h"
#include "uiuc_hh_ap.h"
#include <FDM/LaRCsim/ls_generic.h>
#include <FDM/LaRCsim/ls_constants.h>   /* RAD_TO_DEG, DEG_TO_RAD*/

#ifdef __cplusplus
extern "C" {
#endif

extern double Simtime;

#ifdef __cplusplus
}
#endif

void uiuc_auto_pilot(double dt);

#endif // _AUTO_PILOT_H_
