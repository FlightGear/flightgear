
#ifndef _RAH_AP_H_
#define _RAH_AP_H_

#include <FDM/LaRCsim/ls_constants.h>
#include <cmath>

void rah_ap(double phi, double phirate, double phi_ref, double V,
	    double sample_time, double b, double yawrate, double ctr_defl[2],
	    int init);

#endif //_RAH_AP_H_
