
#ifndef _HH_AP_H_
#define _HH_AP_H_

#include <FDM/LaRCsim/ls_constants.h>
#include <cmath>

void hh_ap(double phi, double yaw, double phirate, double yaw_ref, 
	   double V, double sample_time, double b, double yawrate,
	   double ctr_defl[2], int init);

#endif //_HH_AP_H_
