#ifndef _COEFFICIENTS_H_
#define _COEFFICIENTS_H_

#include "uiuc_aircraft.h"
#include "uiuc_controlInput.h"
#include "uiuc_coef_drag.h"
#include "uiuc_coef_lift.h"
#include "uiuc_coef_pitch.h"
#include "uiuc_coef_sideforce.h"
#include "uiuc_coef_roll.h"
#include "uiuc_coef_yaw.h"
#include "uiuc_iceboot.h"  //Anne's code
#include "uiuc_iced_nonlin.h"
#include "uiuc_icing_demo.h"
#include "uiuc_auto_pilot.h"
#include "uiuc_1Dinterpolation.h"
#include "uiuc_3Dinterpolation.h"
#include "uiuc_warnings_errors.h"
#include <FDM/LaRCsim/ls_generic.h>
#include <FDM/LaRCsim/ls_cockpit.h>     /*Long_control,Lat_control,Rudder_pedal */
#include <FDM/LaRCsim/ls_constants.h>   /* RAD_TO_DEG, DEG_TO_RAD*/
#include <string>

extern double Simtime;

void uiuc_coefficients(double dt);

#endif // _COEFFICIENTS_H_
