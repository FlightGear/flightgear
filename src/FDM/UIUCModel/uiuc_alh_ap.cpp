// *                                                   *
// *   alh_ap.C                                        *
// *                                                   *
// *  ALH autopilot function. takes in the state       *
// *  variables and reference height as arguments      *
// *  (there are other variable too as arguments       *
// *  as listed below)                                 *
// *  and returns the elevator deflection angle at     *
// *  every time step.                                 * 
// *                                                   *
// *   Written 7/8/02 by Vikrant Sharma               *
// *                                                   *
// *****************************************************

//#include <iostream.h>
//#include <stddef.h>  

// define u2prev,ubarprev,x1prev,x2prev and x3prev in the main function
// that uses this autopilot function. give them initial values at origin.
// Pass these values to the A/P function as an argument and pass by
// reference 
// Parameters passed as arguments to the function:
// H - Current Height of the a/c at the current simulation time.
// pitch - Current pitch angle ,,,,,,,,,,,,
// pitchrate - current rate of change of pitch angle
// H_ref - reference Height differential wanted
// sample_t - sampling time
// V - aircraft's current velocity
// u2prev - u2 value at the previous time step.
// ubar prev - ubar ,,,,,,,,,,,,,,,,,,,,,,, 
// x1prev - x1 ,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,
// x2prev - x2 ,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,
// x3prev - x3 ,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,, 
// the autpilot function (alh_ap) changes these values at every time step.
// so the simulator guys don't have to do it. Since these values are
// passed by reference to the function.

// RD changed float to double

#include "uiuc_alh_ap.h"  

double alh_ap(double pitch, double pitchrate, double H_ref, double H, 
	      double V, double sample_time, int init)
{
  // changes by RD so function keeps previous values
  static double u2prev;
  static double x1prev;
  static double x2prev;
  static double x3prev;
  static double ubarprev;

  if (init == 0)
    {
      u2prev = 0;
      x1prev = 0;
      x2prev = 0;
      x3prev = 0;
      ubarprev = 0;
    }

	double Ki;
	double Ktheta;
	double Kq;
	double deltae;
	double Kh,Kd;
	double x1, x2, x3;
	Ktheta = -0.0004*V*V + 0.0479*V - 2.409;
	Kq = -0.0005*V*V + 0.054*V - 1.5931;
	Ki = 0.5;
	Kh = -0.25*LS_PI/180 + (((-0.15 + 0.25)*LS_PI/180)/(20))*(V-60); 
	Kd = -0.0025*V + 0.2875;
	double u1,u2,u3,ubar;
	ubar = (1-Kd*sample_time)*ubarprev + Ktheta*pitchrate*sample_time;
	u1 = Kh*(H_ref-H) - ubar;
	u2 = u2prev + Ki*(Kh*(H_ref-H)-ubar)*sample_time;
	u3 = Kq*pitchrate;
	double totalU;
	totalU = u1 + u2 - u3;
	u2prev = u2;
	ubarprev = ubar;
// the following is using the actuator dynamics given in Beaver.
// the actuator dynamics for Twin Otter are still unavailable.
	x1 = x1prev +(-10.951*x1prev + 7.2721*x2prev + 20.7985*x3prev +
25.1568*totalU)*sample_time;
	x2 = x2prev + x3prev*sample_time;
	x3 = x3prev + (7.3446*x1prev - 668.6713*x2prev - 16.8697*x3prev +
5.8694*totalU)*sample_time;
	deltae = 57.2958*x2;
	x1prev = x1;
	x2prev = x2;
	x3prev = x3;
return deltae;
} 
