//     SIS Twin Otter Iced aircraft Nonlinear model
//     Version 020409
//     read readme_020212.doc for information

// 11-21-2002 (RD) Added e code from Kishwar to fix negative lift problem at
//            high etas

#include "uiuc_iced_nonlin.h"

void Calc_Iced_Forces()
	{
	// alpha in deg
	  double alpha;
	  double de;
	double eta_ref_wing = 0.08;			 // eta of iced data used for curve fit
	double eta_ref_tail = 0.20; //changed from 0.12 10-23-2002
	double eta_wing;
	double e;
	//double delta_CL;				// CL_clean - CL_iced;
	//double delta_CD;				// CD_clean - CD_iced;
	//double delta_Cm;				// CM_clean - CM_iced;
	double delta_Cm_a;				// (Cm_clean - Cm_iced) as a function of AoA;
	double delta_Cm_de;				// (Cm_clean - Cm_iced) as a function of de;
	double delta_Ch_a;
	double delta_Ch_e;
	double KCL;
	double KCD;
	double KCm_alpha;
	double KCm_de;
	double KCh;
	double CL_diff;
	double CD_diff;
	
	
	
	alpha = Std_Alpha*RAD_TO_DEG;
	de = elevator*RAD_TO_DEG;
	// lift fits
	if (alpha < 16)
		{
		delta_CL = (0.088449 + 0.004836*alpha - 0.0005459*alpha*alpha +
			    4.0859e-5*pow(alpha,3));
		}
	else
		{
		delta_CL = (-11.838 + 1.6861*alpha - 0.076707*alpha*alpha +
					0.001142*pow(alpha,3));
		}
	KCL = -delta_CL/eta_ref_wing;
	eta_wing = 0.5*(eta_wing_left + eta_wing_right);
	if (eta_wing <= 0.1)
	  {
	    e = eta_wing;
	  }
	else
	  {
	    e = -0.3297*exp(-5*eta_wing)+0.3;
	  }
	delta_CL = e*KCL;
	
		
	// drag fit
	delta_CD = (-0.0089 + 0.001578*alpha - 0.00046253*pow(alpha,2) +
		    -4.7511e-5*pow(alpha,3) + 2.3384e-6*pow(alpha,4));
	KCD = -delta_CD/eta_ref_wing;
	delta_CD = eta_wing*KCD;
	
	// pitching moment fit
	delta_Cm_a = (-0.01892 - 0.0056476*alpha + 1.0205e-5*pow(alpha,2)
		      - 4.0692e-5*pow(alpha,3) + 1.7594e-6*pow(alpha,4));
					
	delta_Cm_de = (-0.014928 - 0.0037783*alpha + 0.00039086*pow(de,2)
		       - 1.1304e-5*pow(de,3) - 1.8439e-6*pow(de,4));
					
	delta_Cm = delta_Cm_a + delta_Cm_de;
	KCm_alpha = delta_Cm_a/eta_ref_wing;
	KCm_de = delta_Cm_de/eta_ref_tail;
	delta_Cm = (0.75*eta_wing + 0.25*eta_tail)*KCm_alpha + (eta_tail)*KCm_de;
	
	// hinge moment
	if (alpha < 13)
	  {
	    delta_Ch_a = (-0.0012862 - 0.00022705*alpha + 1.5911e-5*pow(alpha,2)
			  + 5.4536e-7*pow(alpha,3));
	  }
	else
	  {
	    delta_Ch_a = 0;
	  }
	delta_Ch_e = -0.0011851 - 0.00049924*de;
	delta_Ch = -(delta_Ch_a + delta_Ch_e);
	KCh = -delta_Ch/eta_ref_tail;
	delta_Ch = eta_tail*KCh;
	
	// rolling moment
	CL_diff = (eta_wing_left - eta_wing_right)*KCL;
	delta_Cl = CL_diff/8.; // 10-23-02 Previously 4

	//yawing moment
	CD_diff = (eta_wing_left - eta_wing_right)*KCD;
	delta_Cn = CD_diff/8.;
	
	}

void add_ice_effects()
{
  CD_clean = -1*CX*Cos_alpha*Cos_beta - CY*Sin_beta - CZ*Sin_alpha*Cos_beta;
  CY_clean = -1*CX*Cos_alpha*Sin_beta + CY*Cos_beta - CZ*Sin_alpha*Sin_beta;
  CL_clean = CX*Sin_alpha - CZ*Cos_alpha;
  Cm_clean = Cm;
  Cl_clean = Cl;
  Cn_clean = Cn;
  Ch_clean = Ch;

  CD_iced = CD_clean + delta_CD;
  CY_iced = CY_clean;
  CL_iced = CL_clean + delta_CL;
  Cm_iced = Cm_clean + delta_Cm;
  Cl_iced = Cl_clean + delta_Cl;
  Cn_iced = Cn_clean + delta_Cn;
  //Ch_iced = Ch_clean + delta_Ch;

  CD = CD_iced;
  CY = CY_iced;
  CL = CL_iced;
  Cm = Cm_iced;
  Cl = Cl_iced;
  Cn = Cn_iced;
  //Ch = Ch_iced;

  CX = -1*CD*Cos_alpha*Cos_beta - CY*Cos_alpha*Sin_beta + CL*Sin_alpha;
  CY = -1*CD*Sin_beta + CY*Cos_beta;
  CZ = -1*CD*Sin_alpha*Cos_beta - CY*Sin_alpha*Sin_beta - CL*Cos_alpha;

}
