/***************************************************************************

  TITLE:	Cherokee_aero
		
----------------------------------------------------------------------------

  FUNCTION:	Linear aerodynamics model

----------------------------------------------------------------------------

  MODULE STATUS:	developmental

----------------------------------------------------------------------------

  GENEALOGY:

----------------------------------------------------------------------------

  MODIFICATION HISTORY:
		

----------------------------------------------------------------------------

  REFERENCES:

  		Based upon book:
				Barnes W. McCormick,
				"Aerodynamics, Aeronautics and Flight Mechanics",
				John Wiley & Sons,1995, ISBN 0-471-11087-6

	any suggestions, corrections, aditional data, flames, everything to 
	Gordan Sikic
	gsikic@public.srce.hr

This source is not checked in this configuration in any way.


----------------------------------------------------------------------------

  CALLED BY:

----------------------------------------------------------------------------

  CALLS TO:

----------------------------------------------------------------------------

  INPUTS:

----------------------------------------------------------------------------

  OUTPUTS:

--------------------------------------------------------------------------*/




#include <float.h>
#include <math.h>
#include "ls_types.h"
#include "ls_generic.h"
#include "ls_cockpit.h"
#include "ls_constants.h"





void cherokee_aero()
/*float ** Cherokee (float t, VectorStanja &X, float *U)*/
{
 	static float
		Cza  = -19149.0/(146.69*146.69*157.5/2.0*0.00238), 
		Czat = -73.4*4*146.69/0.00238/157.5/5.25, 
		Czq  = -2.655*4*2400.0/32.2/0.00238/157.5/146.69/5.25, 
		Cma  = -21662.0 *2/146.69/0.00238/157.5/146.69/5.25, 
		Cmat = -892.4 *4/146.69/0.00238/157.5/146.69/5.25, 
		Cmq  = -2405.1 *4/0.00238/157.5/146.69/5.25/5.25, 
		Czde = -1050.49 *2/0.00238/157.5/146.69/146.69, 
		Cmde = -12771.9 *2/0.00238/157.5/146.69/146.69/5.25, 
		Clb  = -12891.0/(146.69*146.69*157.5/2.0*0.00238)/30.0, 
		Clp  = -0.4704, 
		Clr  = 0.1665, 
		Cyb  = -1169.8/(146.69*146.69*157.5/2.0*0.00238),
		Cyp  = -0.0342, 
		Cnb  = 11127.2/(146.69*146.69*157.5/2.0*0.00238)/30.0, 
		Cnp  = -0.0691, 
		Cnr  = -0.0930, 
		Cyf  = -14.072/(146.69*146.69*157.5/2.0*0.00238), 
		Cyps = 89.229/(146.69*146.69*157.5/2.0*0.00238), 
		Clf  = -5812.4/(146.69*146.69*157.5/2.0*0.00238)/30.0,  //%Clda ?  
		Cnf  = -853.93/(146.69*146.69*157.5/2.0*0.00238)/30.0,  //%Cnda ?  
		Cnps = -1149.0/(146.69*146.69*157.5/2.0*0.00238)/30.0,  //%Cndr ?  
		Cyr  = 1.923/(146.69*146.69*157.5/2.0*0.00238), 

		Cx0 = -0.4645/(157.5*0.3048*0.3048), 

		Cz0 = -0.11875, 
		Cm0 =  0.0959, 

		Clda = -5812.4/(146.69*146.69*157.5/2.0*0.00238)/30.0, // Clf
		Cnda = -853.93/(146.69*146.69*157.5/2.0*0.00238)/30.0, // Cnf
		Cndr = -1149.0/(146.69*146.69*157.5/2.0*0.00238)/30.0, // Cnps

/*
	Possible problems: convention about positive control surfaces offset
*/
		elevator = 0.0, // 20.0 * 180.0/57.3 * Long_control
		aileron  = 0.0, // 30.0 * 180.0/57.3 * Lat_control
		rudder   = 0.0, // 30.0 * 180.0/57.3 * Rudder_pedal,


//		m = 2400/32.2,		// mass 
		S = 157.5,			// wing area
		b = 30.0,			// wing span
		c = 5.25,			// main aerodynamic chrod

//		Ixyz[3] = {1070.0*14.59*0.3048*0.3048, 1249.0*14.59*0.3048*0.3048, 2312.0*14.59*0.3048*0.3048}, 
//		Fa[3], 
//		Ma[3], 
//		*RetVal[4] = {&m, Ixyz, Fa, Ma}; 


//	float
	        V = 0.0, // V_rel_wind
	        qd = 0.0, // Density*V*V/2.0, 			//dinamicki tlak  

		Cx,Cy,Cz,
		Cl,Cm,Cn,
		p,q,r;


/* derivatives are defined in "wind" axes so... */
		p =  P_body*Cos_alpha + R_body*Sin_alpha;
		q =  Q_body;
		r = -P_body*Sin_alpha + R_body*Cos_alpha;



		Cz = Cz0 + Cza*Std_Alpha + Czat*(Std_Alpha_dot*c/2.0/V) + Czq*(q*c/2.0/V) + Czde * elevator;
		Cm = Cm0 + Cma*Std_Alpha + Cmat*(Std_Alpha_dot*c/2.0/V) + Cmq*(q*c/2.0/V) + Cmde * elevator; 

		Cx = Cx0 - (Cza*Std_Alpha)*(Cza*Std_Alpha)/(LS_PI*5.71*0.6);
		Cl = Clb*Std_Beta + Clp*(p*b/2.0/V) + Clr*(r*b/2.0/V) + Clda * aileron;

		Cy = Cyb*Std_Beta + Cyr*(r*b/2.0/V); 
		Cn = Cnb*Std_Beta + Cnp*(p*b/2.0/V) + Cnr*(r*b/2.0/V) + Cndr * rudder; 

/* back to body axes */
	{
		float
			CD = Cx,
			CL = Cz;

			Cx = CD - CL*Sin_alpha;
			Cz = CL;
	}

/* AD forces and moments   */
		F_X_aero = Cx*qd*S;
		F_Y_aero = Cy*qd*S;
		F_Z_aero = Cz*qd*S;

		M_l_aero = (Cl*Cos_alpha - Cn*Sin_alpha)*b*qd*S; 
		M_m_aero = Cm*c*qd*S; 
		M_n_aero = (Cl*Sin_alpha + Cn*Cos_alpha)*b*qd*S; 
}



