/*
cherokee_engine.c

very rough model of Lycoming engine built in PA-28 Cherokee
Only "AD" characteristics are modelled, no transient behaviour,
meaning that there is no time delay, engine acts as gain only.



  		Based upon book:
				Barnes W. McCormick,
				"Aerodynamics, Aeronautics and Flight Mechanics",
				John Wiley & Sons,1995, ISBN 0-471-11087-6

	any suggestions, corrections, aditional data, flames, everything to 
	Gordan Sikic
	gsikic@public.srce.hr


This source is not checked in this configuration in any way.


*/



#include <float.h>
#include <math.h>
#include "ls_types.h"
#include "ls_constants.h"
#include "ls_generic.h"
#include "ls_cockpit.h"
#include "ls_sim_control.h"





void cherokee_engine( SCALAR dt, int init )
{

	static float
		dP = (180.0-117.0)*745.7,   // in Wats
		dn = (2700.0-2350.0)/60.0,  // d_rpm (I mean d_rps, in seconds)
		D  = 6.17*0.3048,			// propeller diameter
		dPh = (58.0-180.0)*745.7,	// change of power as f-cn of height
		dH = 25000.0*0.3048;

	float 	n, 			// rps
	                H,
			J,			// advance ratio (ratio of horizontal speed to speed of propeller's tip)
			eta,		// iskoristivost elise
			T,
			V,
			P;

/* copied from  navion_engine.c */
    if (init || sim_control_.sim_type != cockpit) 
	Throttle[3] = Throttle_pct;

	/*assumption -> 0.0 <= Throttle[3] <=1.0 */
	P = fabs(Throttle[3])*180.0*745.7;		/*180.0*745.5 ->max avail power in W */
	n = dn/dP*(P-117.0*745.7) + 2350.0/60.0;

/*  V  [m/s]   */
	V = (V_rel_wind < 10.0*0.3048 ? 10.0 : V_rel_wind*0.3048);

	J = V/n/D;  


/*
	propeller efficiency

if J >0.7 & J < .85
	eta = 0.8;
elseif J < 0.7
	eta = (0.8-0.55)/(.7-.3)*(J-0.3) + 0.55;
else
	eta = (0.6-0.8)/(1.0-0.85)*(J-0.85) + 0.8;
end
*/
	eta = (J < 0.7 ? ((0.8-0.55)/(.7-.3)*(J-0.3) + 0.55) : 
			(J > 0.85 ? ((0.6-0.8)/(1.0-0.85)*(J-0.85) + 0.8) : 0.8));



/* power on Altitude  (I mean Altitude, not Attitude...)*/

	H = Altitude/0.3048; /* H == Altitude in [m] */
	
	P *= (dPh/dH*H + 180.0*745.7)/(180.0*745.7);

	T = eta*P/V;	/* T in N (Thrust in Newtons) */ 

/*assumption: Engine's line of thrust passes through cg */

    F_X_engine = T*0.2248;	/* F_X_engine in lb */
    F_Y_engine = 0.0;
    F_Z_engine = 0.0;

/* copied from  navion_engine.c */
    Throttle_pct = Throttle[3];

}
