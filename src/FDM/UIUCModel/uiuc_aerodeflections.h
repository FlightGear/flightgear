#ifndef _AERODEFLECTIONS_H_
#define _AERODEFLECTIONS_H_

#include "uiuc_aircraft.h"                 /* aileron, elevator, rudder               */
#include "uiuc_find_position.h"
#include <FDM/LaRCsim/ls_cockpit.h>     /* Long_control, Lat_control, Rudder_pedal */
#include <FDM/LaRCsim/ls_constants.h>   /* RAD_TO_DEG, DEG_TO_RAD                  */
#include <FDM/LaRCsim/ls_generic.h> //For global LaRCsim variables

void uiuc_aerodeflections( double dt );

#endif  // _AERODEFLECTIONS_H_
