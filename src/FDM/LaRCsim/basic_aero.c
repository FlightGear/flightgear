/***************************************************************************

  TITLE:        basic_aero
                
----------------------------------------------------------------------------

  FUNCTION:      aerodynamics model based on stability derivatives 
                 + tweaks for nonlinear aero

----------------------------------------------------------------------------

  MODULE STATUS: developmental

----------------------------------------------------------------------------

  GENEALOGY:     based on aero model from crrcsim code (Drela's aero model)

----------------------------------------------------------------------------

  DESIGNED BY:
                
  CODED BY:             Michael Selig
                
  MAINTAINED BY:        Michael Selig

----------------------------------------------------------------------------

  MODIFICATION HISTORY:
                
  DATE          PURPOSE                                                                                         BY
  7/25/03       LaRCsim debugging purposes

----------------------------------------------------------------------------

  REFERENCES:
  
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
#include "basic_aero.h"

#include <math.h>
#include <stdio.h>


#ifdef USENZ
#define NZ generic_.n_cg_body_v[2]
#else
#define NZ 1
#endif          


extern COCKPIT cockpit_;


void basic_aero(SCALAR dt, int Initialize)
// Calculate forces and moments for the current time step.  If Initialize is
// zero, then re-initialize coefficients by reading in the coefficient file.
{
 static int init = 0;
 static SCALAR elevator_drela, aileron_drela, rudder_drela;

 SCALAR C_ref;
 SCALAR B_ref;
 SCALAR S_ref;
 SCALAR U_ref;
 
 /*  SCALAR Mass; */
 /*  SCALAR I_xx; */
 /*  SCALAR I_yy; */
 /*  SCALAR I_zz; */
 /*  SCALAR I_xz; */
 
 SCALAR Alpha_0;
 SCALAR Cm_0;
 SCALAR CL_0;
 SCALAR CL_max;
 SCALAR CL_min;
 
 SCALAR CD_prof;
 SCALAR Uexp_CD;
 
 SCALAR CL_a;
 SCALAR Cm_a;
 
 SCALAR CY_b;
 SCALAR Cl_b;
 SCALAR Cn_b;
 
 SCALAR CL_q;
 SCALAR Cm_q;
 
 SCALAR CY_p;
 SCALAR Cl_p;
 SCALAR Cn_p;
 
 SCALAR CY_r;
 SCALAR Cl_r;
 SCALAR Cn_r;
 
 SCALAR CL_de;
 SCALAR Cm_de;
 
 SCALAR CY_dr;
 SCALAR Cl_dr;
 SCALAR Cn_dr;
 
 SCALAR CY_da;
 SCALAR Cl_da;
 SCALAR Cn_da;
 
 SCALAR eta_loc;
 SCALAR CG_arm;
 SCALAR CL_drop;
 SCALAR CD_stall = 0.05;
 int stalling;
 
 SCALAR span_eff;
 SCALAR CL_CD0;
 SCALAR CD_CLsq;
 SCALAR CD_AIsq;
 SCALAR CD_ELsq;
 
 SCALAR Phat, Qhat, Rhat;  // dimensionless rotation rates
 SCALAR CL_left, CL_cent, CL_right; // CL values across the span
 SCALAR dCL_left,dCL_cent,dCL_right; // stall-induced CL changes
 SCALAR dCD_left,dCD_cent,dCD_right; // stall-induced CD changes
 SCALAR dCl,dCn,dCl_stall,dCn_stall;  // stall-induced roll,yaw moments
 SCALAR dCm_stall;  // Stall induced pitching moment.
 SCALAR CL_wing, CD_all, CD_scaled, Cl_w;
 SCALAR Cl_r_mod,Cn_p_mod;
 SCALAR CL_drela,CD_drela,Cx_drela,Cy_drela,Cz_drela,Cl_drela,Cm_drela,Cn_drela;
 SCALAR QS;
 SCALAR G_11,G_12,G_13;
 SCALAR G_21,G_22,G_23;
 SCALAR G_31,G_32,G_33;
 SCALAR U_body_X,U_body_Y,U_body_Z;
 SCALAR V_body_X,V_body_Y,V_body_Z;
 SCALAR W_body_X,W_body_Y,W_body_Z;
 SCALAR P_atmo,Q_atmo,R_atmo;

 // set the parameters

 C_ref = 0.551667;
 B_ref = 6.55000;
 S_ref = 3.61111;
 U_ref = 19.6850;
 Alpha_0 = 0.349066E-01;
 Cm_0 = -0.112663E-01;
 CL_0 = 0.563172;
 CL_max = 1.10000;
 CL_min = -0.600000;
 CD_prof = 0.200000E-01;
 Uexp_CD = -0.500000;
 CL_a = 5.50360;
 Cm_a = -0.575335;
 CY_b = -0.415610;
 Cl_b = -0.250926;
 Cn_b = 0.567069E-01;
 CL_q = 7.50999;
 Cm_q = -11.4975;
 CY_p = -0.423820;
 Cl_p = -0.611798;
 Cn_p = -0.740898E-01;
 CY_r = 0.297540;
 Cl_r = 0.139581;
 Cn_r = -0.687755E-01;
 CL_de = 0.162000;
 Cm_de = -0.597537;
 CY_dr = 0.000000E+00;
 Cl_dr = 0.000000E+00;
 Cn_dr = 0.000000E+00;
 CY_da = -0.135890;
 Cl_da = -0.307921E-02;
 Cn_da = 0.527143E-01;
 span_eff = 0.95;
 CL_CD0 = 0.0;
 CD_CLsq = 0.01;
 CD_AIsq = 0.0;
 CD_ELsq = 0.0;
 eta_loc = 0.3;
 CG_arm = 0.25;
 CL_drop = 0.5;

 if (!init)
  {
   init = -1;
  }  

 // jan's data goes -.5 to .5 while
 // fgfs data goes +- 1.
 // so I need to divide by 2 below.
  elevator = Long_control + Long_trim;
  aileron  = Lat_control;
  rudder   = Rudder_pedal;

  elevator = elevator * 0.5;
  aileron  = aileron * 0.5;
  rudder   = rudder * 0.5;



//  printf("%f\n",V_rel_wind);

  /* compute gradients of Local velocities w.r.t. Body coordinates
      G_11  =  dU_local/dx_body
      G_12  =  dU_local/dy_body   etc.  */

  /*
  G_11 = U_atmo_X*T_local_to_body_11
       + U_atmo_Y*T_local_to_body_12
       + U_atmo_Z*T_local_to_body_13;
  G_12 = U_atmo_X*T_local_to_body_21
       + U_atmo_Y*T_local_to_body_22
       + U_atmo_Z*T_local_to_body_23;
  G_13 = U_atmo_X*T_local_to_body_31
       + U_atmo_Y*T_local_to_body_32
       + U_atmo_Z*T_local_to_body_33;

  G_21 = V_atmo_X*T_local_to_body_11
       + V_atmo_Y*T_local_to_body_12
       + V_atmo_Z*T_local_to_body_13;
  G_22 = V_atmo_X*T_local_to_body_21
       + V_atmo_Y*T_local_to_body_22
       + V_atmo_Z*T_local_to_body_23;
  G_23 = V_atmo_X*T_local_to_body_31
       + V_atmo_Y*T_local_to_body_32
       + V_atmo_Z*T_local_to_body_33;

  G_31 = W_atmo_X*T_local_to_body_11
       + W_atmo_Y*T_local_to_body_12
       + W_atmo_Z*T_local_to_body_13;
  G_32 = W_atmo_X*T_local_to_body_21
       + W_atmo_Y*T_local_to_body_22
       + W_atmo_Z*T_local_to_body_23;
  G_33 = W_atmo_X*T_local_to_body_31
       + W_atmo_Y*T_local_to_body_32
       + W_atmo_Z*T_local_to_body_33;
  */

  //printf("%f %f %f %f\n",W_atmo_X,W_atmo_Y,G_31,G_32);

  /* now compute gradients of Body velocities w.r.t. Body coordinates */
  /*  U_body_x  =  dU_body/dx_body   etc.  */

  /*
  U_body_X = T_local_to_body_11*G_11
           + T_local_to_body_12*G_21
           + T_local_to_body_13*G_31;
  U_body_Y = T_local_to_body_11*G_12
           + T_local_to_body_12*G_22
           + T_local_to_body_13*G_32;
  U_body_Z = T_local_to_body_11*G_13
           + T_local_to_body_12*G_23
           + T_local_to_body_13*G_33;
                              
  V_body_X = T_local_to_body_21*G_11
           + T_local_to_body_22*G_21
           + T_local_to_body_23*G_31;
  V_body_Y = T_local_to_body_21*G_12
           + T_local_to_body_22*G_22
           + T_local_to_body_23*G_32;
  V_body_Z = T_local_to_body_21*G_13
           + T_local_to_body_22*G_23
           + T_local_to_body_23*G_33;
                              
  W_body_X = T_local_to_body_31*G_11
           + T_local_to_body_32*G_21
           + T_local_to_body_33*G_31;
  W_body_Y = T_local_to_body_31*G_12
           + T_local_to_body_32*G_22
           + T_local_to_body_33*G_32;
  W_body_Z = T_local_to_body_31*G_13
           + T_local_to_body_32*G_23
           + T_local_to_body_33*G_33;
  */

  /* set rotation rates of airmass motion */
  /*   BUG
        P_atmo =  W_body_X;
        Q_atmo = -W_body_Y;
        R_atmo =  V_body_Z;
  */
      
  /*
        P_atmo =  W_body_Y;
        Q_atmo = -W_body_X;
        R_atmo =  V_body_X;
  */

        P_atmo = 0;
        Q_atmo = 0;
        R_atmo = 0;

  //  printf("%f %f %f\n",P_atmo,Q_atmo,R_atmo);
  if (V_rel_wind != 0)
   {
  /* set net effective dimensionless rotation rates */
    Phat = (P_body - P_atmo) * B_ref / (2.0*V_rel_wind);
    Qhat = (Q_body - Q_atmo) * C_ref / (2.0*V_rel_wind);
    Rhat = (R_body - R_atmo) * B_ref / (2.0*V_rel_wind);
   }
  else
   {
    Phat=0;
    Qhat=0;
    Rhat=0;
   }

//  printf("Phat: %f Qhat: %f Rhat: %f\n",Phat,Qhat,Rhat);

    /* compute local CL at three spanwise locations */
  CL_left  = CL_0 + CL_a*(Std_Alpha - Alpha_0 - Phat*eta_loc);
  CL_cent  = CL_0 + CL_a*(Std_Alpha - Alpha_0               );
  CL_right = CL_0 + CL_a*(Std_Alpha - Alpha_0 + Phat*eta_loc);

// printf("CL_left: %f CL_cent: %f CL_right: %f\n",CL_left,CL_cent,CL_right);

    /* set CL-limit changes */
  dCL_left  = 0.;
  dCL_cent  = 0.;
  dCL_right = 0.;

  stalling=0;
  if (CL_left  > CL_max)
    {
      dCL_left  = CL_max-CL_left -CL_drop; 
      stalling=1;
    }

  if (CL_cent  > CL_max)
    {
      dCL_cent  = CL_max-CL_cent -CL_drop;
      stalling=1;
    }

  if (CL_right > CL_max)
    {
      dCL_right = CL_max-CL_right -CL_drop;
      stalling=1;      
    }

  if (CL_left  < CL_min)
    {
      dCL_left  = CL_min-CL_left -CL_drop;
      stalling=1;
    }

  if (CL_cent  < CL_min)
    {
      dCL_cent  = CL_min-CL_cent -CL_drop; 
      stalling=1;
    }

  if (CL_right < CL_min)
    {
      dCL_right = CL_min-CL_right -CL_drop;
      stalling=1;
    }

  /* set average wing CL */
  CL_wing = CL_0 + CL_a*(Std_Alpha-Alpha_0)
          + 0.25*dCL_left + 0.5*dCL_cent + 0.25*dCL_right;

//  printf("CL_wing: %f\n",CL_wing);


  /* correct profile CD for CL dependence and aileron dependence */
  CD_all = CD_prof
         + CD_CLsq*(CL_wing-CL_CD0)*(CL_wing-CL_CD0)
         + CD_AIsq*aileron*aileron
         + CD_ELsq*elevator*elevator;

//  printf("CD_all:%lf\n",CD_all);

    /* scale profile CD with Reynolds number via simple power law */
 if (V_rel_wind > 0.1)
 {
  CD_scaled = CD_all*pow(((double)V_rel_wind/(double)U_ref),Uexp_CD);
 }
 else
 {
        CD_scaled=CD_all;
 }

//  printf("CD_scaled:%lf\n",CD_scaled);



  /* Scale lateral cross-coupling derivatives with wing CL */
  Cl_r_mod = Cl_r*CL_wing/CL_0;
  Cn_p_mod = Cn_p*CL_wing/CL_0;

  //  printf("Cl_r_mod: %f Cn_p_mod: %f\n",Cl_r_mod,Cn_p_mod);

    /* total average CD with induced and stall contributions */
  dCD_left  = CD_stall*dCL_left *dCL_left ;
  dCD_cent  = CD_stall*dCL_cent *dCL_cent ;
  dCD_right = CD_stall*dCL_right*dCL_right;

  /* total CL, with pitch rate and elevator contributions */
  CL_drela = (CL_wing + CL_q*Qhat + CL_de*elevator)*Cos_alpha;

//  printf("CL:%f\n",CL);

    /* assymetric stall will cause roll and yaw moments */
  dCl =  0.45*-1*(dCL_right-dCL_left)*0.5*eta_loc;
  dCn =  0.45*(dCD_right-dCD_left)*0.5*eta_loc;
  dCm_stall = (0.25*dCL_left + 0.5*dCL_cent + 0.25*dCL_right)*CG_arm;

//  printf("dCl: %f dCn:%f\n",dCl,dCn);

    /* stall-caused moments in body axes */
  dCl_stall = dCl*Cos_alpha - dCn*Sin_alpha;
  dCn_stall = dCl*Sin_alpha + dCn*Cos_alpha;

    /* total CD, with induced and stall contributions */

  Cl_w = Cl_b*Std_Beta  + Cl_p*Phat + Cl_r_mod*Rhat
       + dCl_stall  + Cl_da*aileron;
  CD_drela = CD_scaled
     + (CL*CL + 32.0*Cl_w*Cl_w)*S_ref/(B_ref*B_ref*3.14159*span_eff)
     + 0.25*dCD_left + 0.5*dCD_cent + 0.25*dCD_right;

  //printf("CL: %f CD:%f L/D:%f\n",CL,CD,CL/CD);

    /* total forces in body axes */
  Cx_drela = -CD_drela*Cos_alpha + CL_drela*Sin_alpha*Cos_beta*Cos_beta;
  Cz_drela = -CD_drela*Sin_alpha - CL_drela*Cos_alpha*Cos_beta*Cos_beta;
  Cy_drela = CY_b*Std_Beta  + CY_p*Phat + CY_r*Rhat + CY_dr*rudder;

    /* total moments in body axes */
  Cl_drela =        Cl_b*Std_Beta  + Cl_p*Phat + Cl_r_mod*Rhat + Cl_dr*rudder
           + dCl_stall                               + Cl_da*aileron;
  Cn_drela =        Cn_b*Std_Beta  + Cn_p_mod*Phat + Cn_r*Rhat + Cn_dr*rudder
           + dCn_stall                               + Cn_da*aileron;
  Cm_drela = Cm_0 + Cm_a*(Std_Alpha-Alpha_0) +dCm_stall
                         + Cm_q*Qhat                 + Cm_de*elevator;

    /* set dimensional forces and moments */
  QS = 0.5*Density*V_rel_wind*V_rel_wind * S_ref;
  
  F_X_aero = Cx_drela * QS;
  F_Y_aero = Cy_drela * QS;
  F_Z_aero = Cz_drela * QS;

  M_l_aero = Cl_drela * QS * B_ref;
  M_m_aero = Cm_drela * QS * C_ref;
  M_n_aero = Cn_drela * QS * B_ref;
//  printf("%f %f %f %f %f %f\n",F_X_aero,F_Y_aero,F_Z_aero,M_l_aero,M_m_aero,M_n_aero);
}

