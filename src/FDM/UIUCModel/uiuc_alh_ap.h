
#ifndef _ALH_AP_H_
#define _ALH_AP_H_

#include <FDM/LaRCsim/ls_constants.h>

double alh_ap(double pitch, double pitchrate, double H_ref, double H, 
	      double V, double sample_t, int init);

#endif //_ALH_AP_H_
