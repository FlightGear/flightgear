// *                                                   *
// *   rah_ap.C                                        *
// *                                                   *
// *  Roll attitude Hold autopilot function. takes in  *
// *  the state                                        *
// *  variables and reference angle as arguments       *
// *  (there are other variable too as arguments       *
// *  as listed below)                                 *
// *  and returns the aileron and rudder deflection    * 
// *  angle at every time step                         *       
// *                                                   * 
// *                                                   *
// *   Written 2/14/03 by Vikrant Sharma               *
// *                                                   *
// *****************************************************

//#include <iostream.h>
//#include <stddef.h>  

// define u2prev,x1prev,x2prev,x3prev,x4prev,x5prev and x6prev in the main 
// function
// that uses this autopilot function. give them initial values at origin.
// Pass these values to the A/P function as an argument and pass by
// reference 
// Parameters passed as arguments to the function:
// phi - Current roll angle difference from the trim 
// rollrate - current rate of change of roll angle
// yawrate - current rate of change of yaw angle
// b - the wingspan
// roll_ref - reference roll angle to be tracked (increase or decrease 
// desired from the trim value)
// sample_t - sampling time
// V - aircraft's current velocity
// u2prev - u2 value at the previous time step. 
// x1prev - x1 ,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,
// x2prev - x2 ,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,
// x3prev - x3 ,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,
// x4prev - x4 ,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,
// x5prev - x5 ,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,
// x6prev - x6 ,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,, 
// the autpilot function (rah_ap) changes these values at every time step.
// so the simulator guys don't have to do it. Since these values are
// passed by reference to the function.  

// Another important thing to do and for you simulator guys to check is the 
// way I return the the deltaa (aileron deflection) and deltar (rudder deflection).
// I expect you guys to define an array that holds two float values. The first entry
// is for deltaa and the second for deltar and the pointer to this array (ctr_ptr) is also
// one of the arguments for the function rah_ap and then this function updates the value for
// deltae and deltaa every time its called. PLEASE CHECK IF THE SYNTAX IS RIGHT. Also note that
// these values have to be added to the initial trim values of the control deflection to get the
// entire control deflection.

#include "uiuc_rah_ap.h"

// (RD) changed float to double

void rah_ap(double phi, double phirate, double phi_ref, double V,
	    double sample_time, double b, double yawrate, double ctr_defl[2],
	    int init)
{

  static double u2prev;
  static double x1prev;
  static double x2prev;
  static double x3prev;
  static double x4prev;
  static double x5prev;
  static double x6prev;

  if (init==0)
    {
      u2prev = 0;
      x1prev = 0;
      x2prev = 0;
      x3prev = 0;
      x4prev = 0;
      x5prev = 0;
      x6prev = 0;
    }

	double Kphi;
	double Kr;
	double Ki;
	double drr;
	double dar;
	double deltar;
	double deltaa;
	double x1, x2, x3, x4, x5, x6;
	Kphi = 0.000975*V*V - 0.108*V + 2.335625;
	Kr = -4;
	Ki = 0.25;
	dar = 0.165;
	drr = -0.000075*V*V + 0.0095*V -0.4606;
	double u1,u2,u3,u4,u5,u6,u7,ubar,udbar;
	u1 = Kphi*(phi_ref-phi);
	u2 = u2prev + Ki*Kphi*(phi_ref-phi)*sample_time;
	u3 = dar*yawrate;
	u4 = u1 + u2 + u3;
	u2prev = u2;
	double K1,K2;
	K1 = Kr*9.8*sin(phi)/V;
	K2 = drr - Kr;
	u5 = K2*yawrate;
	u6 = K1*phi;
	u7 = u5 + u6; 
	ubar = phirate*b/(2*V);
	udbar = yawrate*b/(2*V);
// the following is using the actuator dynamics to get the aileron 
// angle, given in Beaver.
// the actuator dynamics for Twin Otter are still unavailable.
	x1 = x1prev +(-10.6*x1prev - 2.64*x2prev -7.58*x3prev +
27.46*u4 -0.0008*ubar)*sample_time;
	x2 = x2prev + x3prev*sample_time;
	x3 = x3prev + (1.09*x1prev - 558.86*x2prev - 23.35*x3prev +
3.02*u4 - 0.164*ubar)*sample_time;
	deltaa = 57.3*x2;
	x1prev = x1;
	x2prev = x2;
	x3prev = x3;

// the following is using the actuator dynamics to get the rudder
// angle, given in Beaver.
// the actuator dynamics for Twin Otter are still unavailable.
        x4 = x4prev +(-9.2131*x4prev + 5.52*x5prev + 16*x6prev +
24.571*u7 + 0.0036*udbar)*sample_time;
        x5 = x5prev + x6prev*sample_time;
        x6 = x6prev + (0.672*x4prev - 919.78*x5prev - 56.453*x6prev +
7.54*u7 - 0.636*udbar)*sample_time;
        deltar = 57.3*x5;
        x4prev = x4;
        x5prev = x5;
        x6prev = x6;
        ctr_defl[0] = deltaa;
	ctr_defl[1] = deltar;
return;
} 
