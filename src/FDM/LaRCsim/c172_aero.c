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
#include "c172_aero.h"

#include <math.h>
#include <stdio.h>


#define NCL 9
#define Ndf 4
#define DYN_ON_SPEED 33 /*20 knots*/


#ifdef USENZ
	#define NZ generic_.n_cg_body_v[2]
#else
	#define NZ 1
#endif		


extern COCKPIT cockpit_;


   SCALAR CLadot;
   SCALAR CLq;
   SCALAR CLde;
   SCALAR CLob;
  
  
   SCALAR Cdob;
   SCALAR Cda;  /*Not used*/
   SCALAR Cdde;
  
   SCALAR Cma;
   SCALAR Cmadot;
   SCALAR Cmq;
   SCALAR Cmob; 
   SCALAR Cmde;
  
   SCALAR Clbeta;
   SCALAR Clp;
   SCALAR Clr;
   SCALAR Clda;
   SCALAR Cldr;
  
   SCALAR Cnbeta;
   SCALAR Cnp;
   SCALAR Cnr;
   SCALAR Cnda;
   SCALAR Cndr;
  
   SCALAR Cybeta;
   SCALAR Cyp;
   SCALAR Cyr;
   SCALAR Cyda;
   SCALAR Cydr;

  /*nondimensionalization quantities*/
  /*units here are ft and lbs */
   SCALAR cbar; /*mean aero chord ft*/
   SCALAR b; /*wing span ft */
   SCALAR Sw; /*wing planform surface area ft^2*/
   SCALAR rPiARe; /*reciprocal of Pi*AR*e*/
   SCALAR lbare;  /*elevator moment arm  MAC*/
   
   SCALAR Weight; /*lbs*/
   SCALAR MaxTakeoffWeight,EmptyWeight;
   SCALAR Cg;     /*%MAC*/
   SCALAR Zcg;    /*%MAC*/
  
  
  SCALAR CLwbh,CL,cm,cd,cn,cy,croll,cbar_2V,b_2V,qS,qScbar,qSb;
  SCALAR CLo,Cdo,Cmo;
  
  SCALAR F_X_wind,F_Y_wind,F_Z_wind;
  
  SCALAR long_trim;

  
  SCALAR elevator, aileron, rudder;

  
  SCALAR Flap_Position;
 
  int Flaps_In_Transit;

static SCALAR interp(SCALAR *y_table, SCALAR *x_table, int Ntable, SCALAR x)
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
		slope=(y_table[Ntable-1]-y_table[Ntable-2])/(x_table[Ntable-1]-x_table[Ntable-2]);
	    y=slope*(x-x_table[Ntable-1]) +y_table[Ntable-1];
		 
/* 		 printf("x larger than x_table[N]: %g %g %d\n",x,x_table[NCL-1],Ntable-1);
 */	}	 
	else /*x is within the table, interpolate linearly to find y value*/
	{
	    
	    while(x_table[i] <= x) {i++;} 
	    slope=(y_table[i]-y_table[i-1])/(x_table[i]-x_table[i-1]);
		/* printf("x: %g, i: %d, cl[i]: %g, cl[i-1]: %g, slope: %g\n",x,i,y_table[i],y_table[i-1],slope); */
	    y=slope*(x-x_table[i-1]) +y_table[i-1];
	}
	return y;
}    	
				

void c172_aero( SCALAR dt, int Initialize ) {
  
  
  // static int init = 0;
  static int fi=0;
  static SCALAR lastFlapHandle=0;
  static SCALAR Ai;
  
  static SCALAR trim_inc = 0.0002;

  static SCALAR alpha_ind[NCL]={-0.087,0,0.14,0.21,0.24,0.26,0.28,0.31,0.35};	
  static SCALAR CLtable[NCL]={-0.22,0.25,1.02,1.252,1.354,1.44,1.466,1.298,0.97};  
  
  static SCALAR flap_ind[Ndf]={0,10,20,30};
  static SCALAR flap_times[Ndf]={0,4,2,2};
  static SCALAR dCLf[Ndf]={0,0.20,0.30,0.35};
  static SCALAR dCdf[Ndf]={0,0.0021,0.0085,0.0191};
  static SCALAR dCmf[Ndf]={0,-0.0654,-0.0981,-0.114};
  
  static SCALAR flap_transit_rate=2.5;
  
  
  

 
   /* printf("Initialize= %d\n",Initialize); */
/* 	   printf("Initializing aero model...Initialize= %d\n", Initialize);
 */	   
        /*nondimensionalization quantities*/
	   /*units here are ft and lbs */
	   cbar=4.9; /*mean aero chord ft*/
	   b=35.8; /*wing span ft */
	   Sw=174; /*wing planform surface area ft^2*/
	   rPiARe=0.054; /*reciprocal of Pi*AR*e*/
	   lbare=3.682;  /*elevator moment arm / MAC*/
 	   
	   CLadot=1.7;
	   CLq=3.9;
	   
	   CLob=0;

       Ai=1.24;
	   Cdob=0.036;
	   Cda=0.13;  /*Not used*/
	   Cdde=0.06;

	   Cma=-1.8;
	   Cmadot=-5.2;
	   Cmq=-12.4;
	   Cmob=-0.02; 
	   Cmde=-1.28;
	   
	   CLde=-Cmde / lbare; /* kinda backwards, huh? */

	   Clbeta=-0.089;
	   Clp=-0.47;
	   Clr=0.096;
	   Clda=-0.09;
	   Cldr=0.0147;

	   Cnbeta=0.065;
	   Cnp=-0.03;
	   Cnr=-0.099;
	   Cnda=-0.0053;
	   Cndr=-0.0657;

	   Cybeta=-0.31;
	   Cyp=-0.037;
	   Cyr=0.21;
	   Cyda=0.0;
	   Cydr=0.187;

	  
	   
	   MaxTakeoffWeight=2550;
	   EmptyWeight=1500;
       
	   Zcg=0.51;
  
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
  
  /*do weight & balance here since there is no better place*/
  Weight=Mass / INVG;
  
  if(Weight > 2550)
  {  Weight=2550; }
  else if(Weight < 1500)
  {  Weight=1500; }
  
  
  if(Dx_cg > 0.43)
  {  Dx_cg = 0.43; }
  else if(Dx_cg < -0.6)
  {  Dx_cg = -0.6; }
  
  Cg=0.25 - Dx_cg/cbar;
  
  Dz_cg=Zcg*cbar;
  Dy_cg=0;
  

  if(Flap_handle < flap_ind[0])
  {
  	fi=0;
	Flap_handle=flap_ind[0];
	lastFlapHandle=Flap_handle;
	Flap_Position=flap_ind[0];
  }
  else if(Flap_handle > flap_ind[Ndf-1])
  {
  	 fi=Ndf-1;
	 Flap_handle=flap_ind[fi];
	 lastFlapHandle=Flap_handle;
	 Flap_Position=flap_ind[fi];
  }
  else	 	
  {		
	 
	 if(dt <= 0)
	    Flap_Position=Flap_handle;
	 else	
	 {
		if(Flap_handle != lastFlapHandle)
		{
	 	   Flaps_In_Transit=1;
		}
		if(Flaps_In_Transit)
		{
		   fi=0;
	       while(flap_ind[fi] < Flap_handle) { fi++; }
		   if(Flap_Position < Flap_handle)
		   {
               if(flap_times[fi] > 0)
			    	flap_transit_rate=(flap_ind[fi] - flap_ind[fi-1])/flap_times[fi];
			   else
			        flap_transit_rate=(flap_ind[fi] - flap_ind[fi-1])/5;
		   }					
		   else 
		   {
		        if(flap_times[fi+1] > 0)
				   flap_transit_rate=(flap_ind[fi] - flap_ind[fi+1])/flap_times[fi+1];		
				else
			       flap_transit_rate=(flap_ind[fi] - flap_ind[fi+1])/5;   
		   }
		   if(fabs(Flap_Position - Flap_handle) > dt*flap_transit_rate)
			   Flap_Position+=flap_transit_rate*dt;
		   else
		   {
			   Flaps_In_Transit=0;
			   Flap_Position=Flap_handle;
		   }
    	}
	 }	
	 lastFlapHandle=Flap_handle;
  }	     	  
  
  if(Aft_trim) long_trim = long_trim - trim_inc;
  if(Fwd_trim) long_trim = long_trim + trim_inc;
  
/*   printf("Long_control: %7.4f, long_trim: %7.4f,DEG_TO_RAD: %7.4f, RAD_TO_DEG: %7.4f\n",Long_control,long_trim,DEG_TO_RAD,RAD_TO_DEG);
 */  /*scale pct control to degrees deflection*/
  if ((Long_control+Long_trim) <= 0)
  	elevator=(Long_control+Long_trim)*28*DEG_TO_RAD;
  else
  	elevator=(Long_control+Long_trim)*23*DEG_TO_RAD;
  
  aileron  = -1*Lat_control*17.5*DEG_TO_RAD;
  rudder   = -1*Rudder_pedal*16*DEG_TO_RAD; 
  /*
    The aileron travel limits are 20 deg. TEU and 15 deg TED
    but since we don't distinguish between left and right we'll
	use the average here (17.5 deg) 
  */	
  
    
  /*calculate rate derivative nondimensionalization (is that a word?) factors */
  /*hack to avoid divide by zero*/
  /*the dynamic terms are negligible at low ground speeds anyway*/ 
  
/*   printf("Vinf: %g, Span: %g\n",V_rel_wind,b);
 */  
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
  
  
/*   printf("aero: Wb: %7.4f, Ub: %7.4f, Alpha: %7.4f, elev: %7.4f, ail: %7.4f, rud: %7.4f, long_trim: %7.4f\n",W_body,U_body,Alpha*RAD_TO_DEG,elevator*RAD_TO_DEG,aileron*RAD_TO_DEG,rudder*RAD_TO_DEG,long_trim*RAD_TO_DEG);
  printf("aero: Theta: %7.4f, Gamma: %7.4f, Beta: %7.4f, Phi: %7.4f, Psi: %7.4f\n",Theta*RAD_TO_DEG,Gamma_vert_rad*RAD_TO_DEG,Beta*RAD_TO_DEG,Phi*RAD_TO_DEG,Psi*RAD_TO_DEG);
 */

 
  
  /* sum coefficients */
  CLwbh = interp(CLtable,alpha_ind,NCL,Std_Alpha);
/*   printf("CLwbh: %g\n",CLwbh);
 */
  CLo = CLob + interp(dCLf,flap_ind,Ndf,Flap_Position);
  Cdo = Cdob + interp(dCdf,flap_ind,Ndf,Flap_Position);
  Cmo = Cmob + interp(dCmf,flap_ind,Ndf,Flap_Position);
  
  /* printf("FP: %g\n",Flap_Position);
  printf("CLo: %g\n",CLo);
  printf("Cdo: %g\n",Cdo);
  printf("Cmo: %g\n",Cmo);	  */


 


  CL = CLo + CLwbh + (CLadot*Std_Alpha_dot + CLq*Theta_dot)*cbar_2V + CLde*elevator;
  cd = Cdo + rPiARe*Ai*Ai*CL*CL + Cdde*elevator;
  cy = Cybeta*Std_Beta + (Cyp*P_body + Cyr*R_body)*b_2V + Cyda*aileron + Cydr*rudder;
  
  cm = Cmo + Cma*Std_Alpha + (Cmq*Q_body + Cmadot*Std_Alpha_dot)*cbar_2V + Cmde*(elevator);
  cn = Cnbeta*Std_Beta + (Cnp*P_body + Cnr*R_body)*b_2V + Cnda*aileron + Cndr*rudder; 
  croll=Clbeta*Std_Beta + (Clp*P_body + Clr*R_body)*b_2V + Clda*aileron + Cldr*rudder;
  
/*   printf("aero: CL: %7.4f, Cd: %7.4f, Cm: %7.4f, Cy: %7.4f, Cn: %7.4f, Cl: %7.4f\n",CL,cd,cm,cy,cn,croll);
 */

  /*calculate wind axes forces*/
  F_X_wind=-1*cd*qS;
  F_Y_wind=cy*qS;
  F_Z_wind=-1*CL*qS;
  
/*   printf("V_rel_wind: %7.4f, Fxwind: %7.4f Fywind: %7.4f Fzwind: %7.4f\n",V_rel_wind,F_X_wind,F_Y_wind,F_Z_wind);
 */
  
  /*calculate moments and body axis forces */
  
  
  
  /* requires ugly wind-axes to body-axes transform */
  F_X_aero = F_X_wind*Cos_alpha*Cos_beta - F_Y_wind*Cos_alpha*Sin_beta - F_Z_wind*Sin_alpha;
  F_Y_aero = F_X_wind*Sin_beta + F_Y_wind*Cos_beta;
  F_Z_aero = F_X_wind*Sin_alpha*Cos_beta - F_Y_wind*Sin_alpha*Sin_beta + F_Z_wind*Cos_alpha;
  
  /*no axes transform here */
  M_l_aero = croll*qSb;
  M_m_aero = cm*qScbar;
  M_n_aero = cn*qSb;
  
/*   printf("I_yy: %7.4f, qScbar: %7.4f, qbar: %7.4f, Sw: %7.4f, cbar: %7.4f, 0.5*rho*V^2: %7.4f\n",I_yy,qScbar,Dynamic_pressure,Sw,cbar,0.5*0.0023081*V_rel_wind*V_rel_wind);
  
  printf("Fxaero: %7.4f Fyaero: %7.4f Fzaero: %7.4f Weight: %7.4f\n",F_X_aero,F_Y_aero,F_Z_aero,Weight); 

  printf("Maero: %7.4f Naero: %7.4f Raero: %7.4f\n",M_m_aero,M_n_aero,M_l_aero);
  */
 
}


