// *                                                   *
// *   pah_ap.C                                        *
// *                                                   *
// *  Pah autopilot function. takes in the state       *
// *  variables and reference angle as arguments       *
// *  (there are other variable too as arguments       *
// *  as listed below)                                 *
// *  and returns the elevator deflection angle at     *
// *  every time step.                                 * 
// *                                                   *
// *   Written 2/11/02 by Vikrant Sharma               *
// *                                                   *
// *****************************************************

//#include <iostream.h>
//#include <stddef.h>  

// define u2prev,x1prev,x2prev and x3prev in the main function
// that uses this autopilot function. give them initial values at origin.
// Pass these values to the A/P function as an argument and pass by
// reference 
// Parameters passed as arguments to the function:
// pitch - Current pitch angle 
// pitchrate - current rate of change of pitch angle
// pitch_ref - reference pitch angle to be tracked
// sample_t - sampling time
// V - aircraft's current velocity
// u2prev - u2 value at the previous time step. 
// x1prev - x1 ,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,
// x2prev - x2 ,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,
// x3prev - x3 ,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,, 
// the autpilot function (pah_ap) changes these values at every time step.
// so the simulator guys don't have to do it. Since these values are
// passed by reference to the function.  

// (RD) Units for the variables
// pitch = radians
// pitchrate = rad/s
// pitch_ref = rad
// sample_t = seconds
// V = m/s

// (RD) changed from float to double

#include "uiuc_pah_ap.h"
//#include <stdio.h>
double pah_ap(double pitch, double pitchrate, double pitch_ref, double V,
	      double sample_time, int init)
{
  // changes by RD so function keeps previous values
  static double u2prev;
  static double x1prev;
  static double x2prev;
  static double x3prev;

  if (init == 0)
    {
      u2prev = 0;
      x1prev = 0;
      x2prev = 0;
      x3prev = 0;
    }
  // end changes

  double Ki;
  double Ktheta;
  double Kq;
  double deltae;
  double x1, x2, x3;
  Ktheta = -0.0004*V*V + 0.0479*V - 2.409;
  Kq = -0.0005*V*V + 0.054*V - 1.5931;
  Ki = 0.5;
  double u1,u2,u3;
  u1 = Ktheta*(pitch_ref-pitch);
  u2 = u2prev + Ki*Ktheta*(pitch_ref-pitch)*sample_time;
  u3 = Kq*pitchrate;
  double totalU;
  totalU = u1 + u2 - u3;
  //printf("\nu1=%f\n",u1);
  //printf("u2=%f\n",u2);
  //printf("u3=%f\n",u3);
  //printf("totalU=%f\n",totalU);
  u2prev = u2;
  // the following is using the actuator dynamics given in Beaver.
  // the actuator dynamics for Twin Otter are still unavailable.
  x1 = x1prev +(-10.951*x1prev + 7.2721*x2prev + 20.7985*x3prev +
		25.1568*totalU)*sample_time;
  x2 = x2prev + x3prev*sample_time;
  x3 = x3prev + (7.3446*x1prev - 668.6713*x2prev - 16.8697*x3prev +
		 5.8694*totalU)*sample_time;
  deltae = 57.2958*x2;
  //deltae = x2;
  //printf("x1prev=%f\n",x1prev);
  //printf("x2prev=%f\n",x2prev);
  //printf("x3prev=%f\n",x3prev);
  x1prev = x1;
  x2prev = x2;
  x3prev = x3;
  //printf("x1=%f\n",x1);
  //printf("x2=%f\n",x2);
  //printf("x3=%f\n",x3);
  //printf("deltae=%f\n",deltae);
  return deltae;
} 
			

		

