//     SIS Twin Otter Iced aircraft Nonlinear model
//     Version 020409
//     read readme_020212.doc for information

#include "uiuc_iced_nonlin.h"

void Calc_Iced_Forces()
	{
	// alpha in deg
	  double alpha;
	  double de;
	double eta_ref_wing = 0.08;			 // eta of iced data used for curve fit
	double eta_ref_tail = 0.12;
	double eta_wing;
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
	
	
	
	alpha = Alpha*RAD_TO_DEG;
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
	delta_CL = eta_wing*KCL;
	
		
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
	delta_Cl = CL_diff/4;
	
	}

void add_ice_effects()
{
  CL_clean = -1*CZ*cos(Alpha) + CX*sin(Alpha);  //Check later
  CD_clean = -1*CZ*sin(Alpha) - CX*cos(Alpha);
  Cm_clean = Cm;
  Cl_clean = Cl;
  Ch_clean = Ch;

  CL_iced = CL_clean + delta_CL;
  CD_iced = CD_clean + delta_CD;
  Cm_iced = Cm_clean + delta_Cm;
  Cl_iced = Cl_clean + delta_Cl;
  //Ch_iced = Ch_clean + delta_Ch;

  CL = CL_iced;
  CD = CD_iced;
  Cm = Cm_iced;
  Cl = Cl_iced;
  //Ch = Ch_iced;

  CZ = -1*CL*cos(Alpha) - CD*sin(Alpha);
  CX = CL*sin(Alpha) - CD*cos(Alpha);

}
