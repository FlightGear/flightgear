
#ifndef _AERODEFLECTIONS_H_
#define _AERODEFLECTIONS_H_

#include "uiuc_aircraft.h"  /* uses aileron, elevator, rudder               */
#include "../FDM/LaRCsim/ls_cockpit.h"     /* uses Long_control, Lat_control, Rudder_pedal */
#include "../FDM/LaRCsim/ls_constants.h"   /* uses RAD_TO_DEG, DEG_TO_RAD                  */

void uiuc_aerodeflections();

#endif  // _AERODEFLECTIONS_H_
