#ifndef _COEF_FLAP_H_
#define _COEF_FLAP_H_

#include <FDM/LaRCsim/ls_generic.h>

#include "uiuc_aircraft.h"
#include "uiuc_2Dinterpolation.h"


double uiuc_3Dinterpolation( double third_Array[30],
			     double full_xArray[30][100][100],
			     double full_yArray[30][100],
			     double full_zArray[30][100][100],
			     int full_nxArray[30][100],
			     int full_ny[30],
			     int third_max,
			     double third_bet,
			     double x_value,
			     double y_value);
double uiuc_3Dinterp_quick( double z[30],
			    double x[100],
			    double y[100],
			    double fxyz[30][100][100],
			    int xmax,
			    int ymax,
			    int zmax,
			    double zp,
			    double xp,
			    double yp);

#endif // _COEF_FLAP_H_
