/***************************************************************************

  TITLE:	c172_aero
		
----------------------------------------------------------------------------

  FUNCTION:	aerodynamics model based on constant stability derivatives

----------------------------------------------------------------------------

  MODULE STATUS:	developmental

----------------------------------------------------------------------------

  GENEALOGY:	Based on data from:
  				Part 1 of Roskam's S&C text
				The FAA type certificate data sheet for the 172
				Various sources on the net
				John D. Anderson's Intro to Flight text (NACA 2412 data)
  				UIUC's airfoil data web site  

----------------------------------------------------------------------------

  DESIGNED BY:	Tony Peden
		
  CODED BY:		Tony Peden
		
  MAINTAINED BY:	Tony Peden

----------------------------------------------------------------------------

  MODIFICATION HISTORY:
		
  DATE		PURPOSE												BY
  6/10/99   Initial test release  
  

----------------------------------------------------------------------------

  REFERENCES:
  
  Aero Coeffs:
  	CL 			lift
	Cd 			drag
	Cm 			pitching moment
	Cy 			sideforce
	Cn 			yawing moment
	Croll,Cl 	rolling moment (yeah, I know.  Shoot me.)
  
  Subscripts
  	o 		constant i.e. not a function of alpha or beta
	a 		alpha
	adot    d(alpha)/dt
	q       pitch rate
	qdot    d(q)/dt
	beta    sideslip angle
	p		roll rate
	r		yaw rate
	da      aileron deflection
	de      elevator deflection
	dr      rudder deflection
	
	s		stability axes
	
    

----------------------------------------------------------------------------

  CALLED BY:

----------------------------------------------------------------------------

  CALLS TO:

----------------------------------------------------------------------------

  INPUTS:	

----------------------------------------------------------------------------

  OUTPUTS:

--------------------------------------------------------------------------*/


	
#include "ls_generic.h"
#include "ls_cockpit.h"
#include "ls_constants.h"
#include "ls_types.h"
#include <math.h>
#include <stdio.h>


#define NCL 11
#define DYN_ON_SPEED 33 /*20 knots*/


#ifdef USENZ
	#define NZ generic_.n_cg_body_v[2]
#else
	#define NZ 1
#endif		


extern COCKPIT cockpit_;


SCALAR interp(SCALAR *y_table, SCALAR *x_table, int Ntable, SCALAR x)
{
	SCALAR slope;
	int i=1;
	float y;
		
	
	/* if x is outside the table, return value at x[0] or x[Ntable-1]*/
	if(x <= x_table[0])
	{
		 y=y_table[0];
		 /* printf("x smaller than x_table[0]: %g %g\n",x,x_table[0]); */
	}	 
	else if(x >= x_table[Ntable-1])
	{
		 y=y_table[Ntable-1];
		 /* printf("x larger than x_table[N]: %g %g %d\n",x,x_table[NCL-1],Ntable-1); */
	}	 
	else /*x is within the table, interpolate linearly to find y value*/
	{
	    
	    while(x_table[i] <= x) {i++;} 
	    slope=(y_table[i]-y_table[i-1])/(x_table[i]-x_table[i-1]);
		/* printf("x: %g, i: %d, cl[i]: %g, cl[i-1]: %g, slope: %g\n",x,i,y_table[i],y_table[i-1],slope); */
	    y=slope*(x-x_table[i-1]) +y_table[i-1];
	}
	return y;
}    	
				

	
void aero( SCALAR dt, int Initialize ) {
  
  
  static int init = 0;

  
  static SCALAR trim_inc = 0.0002;
  SCALAR long_trim;

  
  SCALAR elevator, aileron, rudder;
  
  
  
  static SCALAR alpha_ind[NCL]={-0.087,0,0.175,0.209,0.24,0.262,0.278,0.303,0.314,0.332,0.367};	
  static SCALAR CLtable[NCL]={-0.14,0.31,1.21,1.376,1.51249,1.591,1.63,1.60878,1.53712,1.376,1.142};
  
  
   	
  
  /*Note that CLo,Cdo,Cmo will likely change with flap setting so 
  they may not be declared static in the future */
  
  
  static SCALAR CLadot=1.7;
  static SCALAR CLq=3.9;
  static SCALAR CLde=0.43;
  static SCALAR CLo=0;
  
  
  static SCALAR Cdo=0.031;
  static SCALAR Cda=0.13;  /*Not used*/
  static SCALAR Cdde=0.06;
  
  static SCALAR Cma=-0.89;
  static SCALAR Cmadot=-5.2;
  static SCALAR Cmq=-12.4;
  static SCALAR Cmo=-0.062; 
  static SCALAR Cmde=-1.28;
  
  static SCALAR Clbeta=-0.089;
  static SCALAR Clp=-0.47;
  static SCALAR Clr=0.096;
  static SCALAR Clda=-0.178;
  static SCALAR Cldr=0.0147;
  
  static SCALAR Cnbeta=0.065;
  static SCALAR Cnp=-0.03;
  static SCALAR Cnr=-0.099;
  static SCALAR Cnda=-0.053;
  static SCALAR Cndr=-0.0657;
  
  static SCALAR Cybeta=-0.31;
  static SCALAR Cyp=-0.037;
  static SCALAR Cyr=0.21;
  static SCALAR Cyda=0.0;
  static SCALAR Cydr=0.187;
  
  /*nondimensionalization quantities*/
  /*units here are ft and lbs */
  static SCALAR cbar=4.9; /*mean aero chord ft*/
  static SCALAR b=35.8; /*wing span ft */
  static SCALAR Sw=174; /*wing planform surface area ft^2*/
  static SCALAR rPiARe=0.054; /*reciprocal of Pi*AR*e*/
  
  SCALAR W=Mass/INVG;
  
  SCALAR CLwbh,CL,cm,cd,cn,cy,croll,cbar_2V,b_2V,qS,qScbar,qSb,ps,rs;
  
  SCALAR F_X_wind,F_Y_wind,F_Z_wind,W_X,W_Y,W_Z;
  
  
  

  
  if (Initialize != 0)
    {
      

     

      
    }
    
  
  
  /*
  LaRCsim uses:
    Cm > 0 => ANU
	Cl > 0 => Right wing down
	Cn > 0 => ANL
  so:	
    elevator > 0 => AND -- aircraft nose down
	aileron > 0  => right wing up
	rudder > 0   => ANL
  */
  
  if(Aft_trim) long_trim = long_trim - trim_inc;
  if(Fwd_trim) long_trim = long_trim + trim_inc;
  
  /*scale pct control to degrees deflection*/
  if ((Long_control+long_trim) <= 0)
  	elevator=(Long_control+long_trim)*28*DEG_TO_RAD;
  else
  	elevator=(Long_control+long_trim)*23*DEG_TO_RAD;
  
  aileron  = Lat_control*17.5*DEG_TO_RAD;
  rudder   = Rudder_pedal*16*DEG_TO_RAD; 
  /*
    The aileron travel limits are 20 deg. TEU and 15 deg TED
    but since we don't distinguish between left and right we'll
	use the average here (17.5 deg) 
  */	
  
    
  /*calculate rate derivative nondimensionalization (is that a word?) factors */
  /*hack to avoid divide by zero*/
  /*the dynamic terms might be negligible at low ground speeds anyway*/ 
  
  if(V_rel_wind > DYN_ON_SPEED) 
  {
  	cbar_2V=cbar/(2*V_rel_wind);
  	b_2V=b/(2*V_rel_wind);
  }
  else
  {
  	cbar_2V=0;
	b_2V=0;
  }		
  
  
  /*calcuate the qS nondimensionalization factors*/
  
  qS=Dynamic_pressure*Sw;
  qScbar=qS*cbar;
  qSb=qS*b;
  
  /*transform the aircraft rotation rates*/
  ps=-P_body*Cos_alpha + R_body*Sin_alpha;
  rs=-P_body*Sin_alpha + R_body*Cos_alpha;
  
/*   printf("Wb: %7.4f, Ub: %7.4f, Alpha: %7.4f, elev: %7.4f, ail: %7.4f, rud: %7.4f\n",W_body,U_body,Alpha*RAD_TO_DEG,elevator*RAD_TO_DEG,aileron*RAD_TO_DEG,rudder*RAD_TO_DEG);
  printf("Theta: %7.4f, Gamma: %7.4f, Beta: %7.4f, Phi: %7.4f, Psi: %7.4f\n",Theta*RAD_TO_DEG,Gamma_vert_rad*RAD_TO_DEG,Beta*RAD_TO_DEG,Phi*RAD_TO_DEG,Psi*RAD_TO_DEG);
 */
  
  /* sum coefficients */
  CLwbh = interp(CLtable,alpha_ind,NCL,Alpha);
  CL = CLo + CLwbh + (CLadot*Alpha_dot + CLq*Theta_dot)*cbar_2V + CLde*elevator;
  cd = Cdo + rPiARe*CL*CL + Cdde*elevator;
  cy = Cybeta*Beta + (Cyp*ps + Cyr*rs)*b_2V + Cyda*aileron + Cydr*rudder;
  
  cm = Cmo + Cma*Alpha + (Cmq*Theta_dot + Cmadot*Alpha_dot)*cbar_2V + Cmde*(elevator+long_trim);
  cn = Cnbeta*Beta + (Cnp*ps + Cnr*rs)*b_2V + Cnda*aileron + Cndr*rudder; 
  croll=Clbeta*Beta + (Clp*ps + Clr*rs)*b_2V + Clda*aileron + Cldr*rudder;
  
/*   printf("CL: %7.4f, Cd: %7.4f, Cm: %7.4f, Cy: %7.4f, Cn: %7.4f, Cl: %7.4f\n",CL,cd,cm,cy,cn,croll);
 */  /*calculate wind axes forces*/
  F_X_wind=-1*cd*qS;
  F_Y_wind=cy*qS;
  F_Z_wind=-1*CL*qS;
  
/*   printf("V_rel_wind: %7.4f, Fxwind: %7.4f Fywind: %7.4f Fzwind: %7.4f\n",V_rel_wind,F_X_wind,F_Y_wind,F_Z_wind);
 */  
  /*calculate moments and body axis forces */
  
  /*find body-axis components of weight*/
  /*with earth axis to body axis transform */
  W_X=-1*W*Sin_theta;
  W_Y=W*Sin_phi*Cos_theta;
  W_Z=W*Cos_phi*Cos_theta;
  
  /* requires ugly wind-axes to body-axes transform */
  F_X_aero = W_X + F_X_wind*Cos_alpha*Cos_beta - F_Y_wind*Cos_alpha*Sin_beta - F_Z_wind*Sin_alpha;
  F_Y_aero = W_Y + F_X_wind*Sin_beta + F_Y_wind*Cos_beta;
  F_Z_aero = W_Z*NZ + F_X_wind*Sin_alpha*Cos_beta - F_Y_wind*Sin_alpha*Sin_beta + F_Z_wind*Cos_alpha;
  
  /*no axes transform here */
  M_l_aero = croll*qSb;
  M_m_aero = cm*qScbar;
  M_n_aero = cn*qSb;
  
/*   printf("I_yy: %7.4f, qScbar: %7.4f, qbar: %7.4f, Sw: %7.4f, cbar: %7.4f, 0.5*rho*V^2: %7.4f\n",I_yy,qScbar,Dynamic_pressure,Sw,cbar,0.5*0.0023081*V_rel_wind*V_rel_wind);
  printf("Fxbody: %7.4f Fybody: %7.4f Fzbody: %7.4f Weight: %7.4f\n",F_X_wind,F_Y_wind,F_Z_wind,W);
  printf("Maero: %7.4f Naero: %7.4f Raero: %7.4f\n",M_m_aero,M_n_aero,M_l_aero);
 */
  
}


