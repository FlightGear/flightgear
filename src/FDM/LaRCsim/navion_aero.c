/***************************************************************************

  TITLE:	Navion_aero
		
----------------------------------------------------------------------------

  FUNCTION:	Linear aerodynamics model

----------------------------------------------------------------------------

  MODULE STATUS:	developmental

----------------------------------------------------------------------------

  GENEALOGY:	Based upon class notes from AA271, Stanford University,
                Spring 1988.  Dr. Robert Cannon, instructor.  

----------------------------------------------------------------------------

  DESIGNED BY:	Bruce Jackson
		
  CODED BY:		Bruce Jackson
		
  MAINTAINED BY:	Bruce Jackson

----------------------------------------------------------------------------

  MODIFICATION HISTORY:
		
  DATE		PURPOSE												BY
  921229     Changed Alpha, Beta into radians; added Alpha bias.
		                                 EBJ
  930105     Modified to support linear airframe simulation by
		           adding shared memory initialization routine. EBJ
  931013     Added scaling by airspeed,  to allow for low-airspeed
			    ground operations.				EBJ
  940216    Scaled long, lat stick and rudder to more appropriate values 
	    of elevator and aileron. EBJ

----------------------------------------------------------------------------

  REFERENCES:

The Navion "aero" routine is a simple representation of the North
American Navion airplane, a 1950-s vintage single-engine, low-wing
mono-lane built by NAA (who built the famous P-51 Mustang) supposedly
as a plane for returning WW-II fighter jocks to carry the family
around the country in. Unfortunately underpowered, it can still be
found in small airports across the United States. From behind, it sort
of looks like a Volkswagen driving a Piper by virtue of its nicely
rounded cabin roof and small rear window.

The aero routine is only valid around 100 knots; it is referred to as
a "linear model" of the navion; the data having been extracted by
someone unknown from a more complete model, or more likely, from
in-flight measurements and manuever time histories.  It probably came
from someone at Princeton U; they owned a couple modified Navions that
had a variable-stability system installed, and were highly
instrumented (and well calibrated, I assume).

In any event, a linearized model, such as this one, contains various
"stability derivatives", or estimates of how aerodynamic forces and
moments vary with changes in angle of attack, angular body rates, and
control surface deflections. For example, L_beta is an estimate of how
much roll moment varies per degree of sideslip increase.  A decoding
ring is given below:

	X	Aerodynamic force, lbs, in X-axis (+ forward)
	Y	Aerodynamic force, lbs, in Y-axis (+ right)
	Z	Aerodynamic force, lbs, in Z-axis (+ down)
	L	Aero. moment about X-axis (+ roll right), ft-lbs
	M	Aero. moment about Y-axis (+ pitch up), ft-lbs
	N	Aero. moment about Z-axis (+ nose right), ft-lbs

	0	Subscript implying initial, or nominal, value
	u	X-axis component of airspeed (ft/sec) (+ forward)
	v	Y-axis component of airspeed (ft/sec) (+ right)	
	w	Z-axis component of airspeed (ft/sec) (+ down)
	p	X-axis ang. rate (rad/sec) (+ roll right), rad/sec
	q	Y-axis ang. rate (rad/sec) (+ pitch up), rad/sec
	r	Z-axis ang. rate (rad/sec) (+ yaw right), rad/sec
	beta	Angle of sideslip, degrees (+ wind in RIGHT ear)
	da	Aileron deflection, degrees (+ left ail. TE down)
	de	Elevator deflection, degrees (+ trailing edge down)
	dr	Rudder deflection, degrees (+ trailing edge LEFT)

----------------------------------------------------------------------------

  CALLED BY:

----------------------------------------------------------------------------

  CALLS TO:

----------------------------------------------------------------------------

  INPUTS:

----------------------------------------------------------------------------

  OUTPUTS:

--------------------------------------------------------------------------*/

#include "ls_types.h"
#include "ls_generic.h"
#include "ls_cockpit.h"

/* define trimmed w_body to correspond with alpha_trim = 5 */
#define TRIMMED_W  15.34

extern COCKPIT cockpit_;


void navion_aero( SCALAR dt, int Initialize ) {
  static int init = 0;

  SCALAR u, w;
  static SCALAR elevator, aileron, rudder;
  static SCALAR long_scale = 0.3;
  static SCALAR lat_scale  = 0.1;
  static SCALAR yaw_scale  = -0.1;
  static SCALAR scale = 1.0;
  
  /* static SCALAR trim_inc = 0.0002; */
  /* static SCALAR long_trim; */

  static DATA U_0;
  static DATA X_0;
  static DATA M_0;
  static DATA Z_0;
  static DATA X_u;
  static DATA X_w;
  static DATA X_de;
  static DATA Y_v;
  static DATA Z_u;
  static DATA Z_w;
  static DATA Z_de;
  static DATA L_beta;
  static DATA L_p;
  static DATA L_r;
  static DATA L_da;
  static DATA L_dr;
  static DATA M_w;
  static DATA M_q;
  static DATA M_de;
  static DATA N_beta;
  static DATA N_p;    
  static DATA N_r;
  static DATA N_da;
  static DATA N_dr;

  if (!init)
    {
      init = -1;

      /* Initialize aero coefficients */

      U_0 = 176;
      X_0 = -573.75;
      M_0 = 0;
      Z_0 = -2750;
      X_u = -0.0451;        /* original value */
      /* X_u = 0.0000; */   /* for MUCH better performance - EBJ */
      X_w =  0.03607;
      X_de = 0;
      Y_v = -0.2543;
      Z_u = -0.3697;        /* original value */
      /* Z_u = -0.03697; */ /* for better performance - EBJ */
      Z_w = -2.0244;
      Z_de = -28.17;
      L_beta = -15.982;
      L_p = -8.402;
      L_r = 2.193;
      L_da = 28.984;
      L_dr = 2.548;
      M_w = -0.05;
      M_q = -2.0767;
      M_de = -11.1892;
      N_beta = 4.495;
      N_p = -0.3498;    
      N_r = -0.7605;
      N_da = -0.2218;
      N_dr = -4.597;
    }
    
  u = V_rel_wind - U_0;
  w = W_body - TRIMMED_W;
  
  elevator = long_scale * Long_control;
  aileron  = lat_scale  * Lat_control;
  rudder   = yaw_scale  * Rudder_pedal;
  
  /* if(Aft_trim) long_trim = long_trim - trim_inc; */
  /* if(Fwd_trim) long_trim = long_trim + trim_inc; */
  
  scale = V_rel_wind*V_rel_wind/(U_0*U_0); 
  if (scale > 1.0) scale = 1.0; /* ebj */
    
  
  F_X_aero = scale*(X_0 + Mass*(X_u*u + X_w*w + X_de*elevator));
  F_Y_aero = scale*(Mass*Y_v*V_body);
  F_Z_aero = scale*(Z_0 + Mass*(Z_u*u + Z_w*w + Z_de*elevator));
  
  M_l_aero = scale*(I_xx*(L_beta*Std_Beta + L_p*P_body + L_r*R_body
		   + L_da*aileron + L_dr*rudder));
  M_m_aero = scale*(M_0 + I_yy*(M_w*w + M_q*Q_body + M_de*(elevator + Long_trim)));
  M_n_aero = scale*(I_zz*(N_beta*Std_Beta + N_p*P_body + N_r*R_body
		   + N_da*aileron + N_dr*rudder));
  
}


