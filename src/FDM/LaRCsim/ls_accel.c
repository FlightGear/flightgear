/***************************************************************************
  
  TITLE:	ls_Accel
  
  ----------------------------------------------------------------------------
  
  FUNCTION:	Sums forces and moments and calculates accelerations
  
  ----------------------------------------------------------------------------
  
  MODULE STATUS:	developmental
  
  ----------------------------------------------------------------------------
  
  GENEALOGY:	Written 920731 by Bruce Jackson.  Based upon equations
  given in reference [1] and a Matrix-X/System Build block
  diagram model of equations of motion coded by David Raney
  at NASA-Langley in June of 1992.
  
  ----------------------------------------------------------------------------
  
  DESIGNED BY:	Bruce Jackson
  
  CODED BY:		Bruce Jackson
  
  MAINTAINED BY:	
  
  ----------------------------------------------------------------------------
  
  MODIFICATION HISTORY:
  
  DATE		PURPOSE		
  
  931007    Moved calculations of auxiliary accelerations here from ls_aux.c									BY
	    and corrected minus sign in front of A_Y_Pilot 
	    contribution from Q_body*P_body*D_X_pilot term.
  940111    Changed DATA to SCALAR; updated header files
	    
$Header$
$Log$
Revision 1.1  2002/09/10 01:14:01  curt
Initial revision

Revision 1.1.1.1  1999/06/17 18:07:33  curt
Start of 0.7.x branch

Revision 1.1.1.1  1999/04/05 21:32:45  curt
Start of 0.6.x branch.

Revision 1.4  1998/08/24 20:09:26  curt
Code optimization tweaks from Norman Vine.

Revision 1.3  1998/08/06 12:46:38  curt
Header change.

Revision 1.2  1998/01/19 18:40:24  curt
Tons of little changes to clean up the code and to remove fatal errors
when building with the c++ compiler.

Revision 1.1  1997/05/29 00:09:53  curt
Initial Flight Gear revision.

 * Revision 1.5  1994/01/11  18:42:16  bjax
 * Oops! Changed data types from DATA to SCALAR for precision control.
 *
 * Revision 1.4  1994/01/11  18:36:58  bjax
 * Removed ls_eom.h include directive; replaced with ls_types, ls_constants,
 * and ls_generic.h includes.
 *
 * Revision 1.3  1993/10/07  18:45:24  bjax
 * Added local defn of d[xyz]_pilot_from_cg to support previous mod. EBJ
 *
 * Revision 1.2  1993/10/07  18:41:31  bjax
 * Moved calculations of auxiliary accelerations here from ls_aux, and
 * corrected sign on Q_body*P_body*d_x_pilot term of A_Y_pilot calc.  EBJ
 *
 * Revision 1.1  1992/12/30  13:17:02  bjax
 * Initial revision
 *
  
  ----------------------------------------------------------------------------
  
  REFERENCES:
  
  [  1]	McFarland, Richard E.: "A Standard Kinematic Model
  for Flight Simulation at NASA-Ames", NASA CR-2497,
  January 1975 
  
  ----------------------------------------------------------------------------
  
  CALLED BY:
  
  ----------------------------------------------------------------------------
  
  CALLS TO:
  
  ----------------------------------------------------------------------------
  
  INPUTS:  Aero, engine, gear forces & moments
  
  ----------------------------------------------------------------------------
  
  OUTPUTS:	State derivatives
  
  -------------------------------------------------------------------------*/
#include "ls_types.h"
#include "ls_generic.h"
#include "ls_constants.h"
#include "ls_accel.h"
#include <math.h>

void ls_accel( void ) {
  
  SCALAR	inv_Mass, inv_Radius;
  SCALAR	ixz2, c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, c10;
  SCALAR	dx_pilot_from_cg, dy_pilot_from_cg, dz_pilot_from_cg;
  SCALAR	tan_Lat_geocentric;
  
  
  /* Sum forces and moments at reference point */
  
  
  F_X = F_X_aero + F_X_engine + F_X_gear;
  F_Y = F_Y_aero + F_Y_engine + F_Y_gear;
  F_Z = F_Z_aero + F_Z_engine + F_Z_gear;
  
  M_l_rp = M_l_aero + M_l_engine + M_l_gear;
  M_m_rp = M_m_aero + M_m_engine + M_m_gear;
  M_n_rp = M_n_aero + M_n_engine + M_n_gear;
  
  /* Transfer moments to center of gravity */
  
  M_l_cg = M_l_rp + F_Y*Dz_cg - F_Z*Dy_cg;
  M_m_cg = M_m_rp + F_Z*Dx_cg - F_X*Dz_cg;
  M_n_cg = M_n_rp + F_X*Dy_cg - F_Y*Dx_cg;
  
  /* Transform from body to local frame */
  
  F_north = T_local_to_body_11*F_X + T_local_to_body_21*F_Y 
    + T_local_to_body_31*F_Z;
  F_east  = T_local_to_body_12*F_X + T_local_to_body_22*F_Y 
    + T_local_to_body_32*F_Z;
  F_down  = T_local_to_body_13*F_X + T_local_to_body_23*F_Y 
    + T_local_to_body_33*F_Z;
  
  /* Calculate linear accelerations */
  
  tan_Lat_geocentric = tan(Lat_geocentric);
  inv_Mass = 1/Mass;
  inv_Radius = 1/Radius_to_vehicle;
  V_dot_north = inv_Mass*F_north + 
    inv_Radius*(V_north*V_down - V_east*V_east*tan_Lat_geocentric);
  V_dot_east  = inv_Mass*F_east  + 
    inv_Radius*(V_east*V_down + V_north*V_east*tan_Lat_geocentric);
  V_dot_down  = inv_Mass*(F_down) + Gravity -
    inv_Radius*(V_north*V_north + V_east*V_east);
  
  /* Invert the symmetric inertia matrix */
  
  ixz2 = I_xz*I_xz;
  c0  = 1/(I_xx*I_zz - ixz2);
  c1  = c0*((I_yy-I_zz)*I_zz - ixz2);
  c4  = c0*I_xz;
  /* c2  = c0*I_xz*(I_xx - I_yy + I_zz); */
  c2  = c4*(I_xx - I_yy + I_zz);
  c3  = c0*I_zz;
  c7  = 1/I_yy;
  c5  = c7*(I_zz - I_xx);
  c6  = c7*I_xz;
  c8  = c0*((I_xx - I_yy)*I_xx + ixz2);
  /* c9  = c0*I_xz*(I_yy - I_zz - I_xx); */
  c9  = c4*(I_yy - I_zz - I_xx);
  c10 = c0*I_xx;
  
  /* Calculate the rotational body axis accelerations */
  
  P_dot_body = (c1*R_body + c2*P_body)*Q_body + c3*M_l_cg +  c4*M_n_cg;
  Q_dot_body = c5*R_body*P_body + c6*(R_body*R_body - P_body*P_body) + c7*M_m_cg;
  R_dot_body = (c8*P_body + c9*R_body)*Q_body + c4*M_l_cg + c10*M_n_cg;
  
  /* Calculate body axis accelerations (move to ls_accel?)	*/

    inv_Mass = 1/Mass;
    
    A_X_cg = F_X * inv_Mass;
    A_Y_cg = F_Y * inv_Mass;
    A_Z_cg = F_Z * inv_Mass;
    
    dx_pilot_from_cg = Dx_pilot - Dx_cg;
    dy_pilot_from_cg = Dy_pilot - Dy_cg;
    dz_pilot_from_cg = Dz_pilot - Dz_cg;
    
    A_X_pilot = A_X_cg 	+ (-R_body*R_body - Q_body*Q_body)*dx_pilot_from_cg
					    + ( P_body*Q_body - R_dot_body   )*dy_pilot_from_cg
					    + ( P_body*R_body + Q_dot_body   )*dz_pilot_from_cg;
    A_Y_pilot = A_Y_cg 	+ ( P_body*Q_body + R_dot_body   )*dx_pilot_from_cg
					    + (-P_body*P_body - R_body*R_body)*dy_pilot_from_cg
					    + ( Q_body*R_body - P_dot_body   )*dz_pilot_from_cg;
    A_Z_pilot = A_Z_cg 	+ ( P_body*R_body - Q_dot_body   )*dx_pilot_from_cg
					    + ( Q_body*R_body + P_dot_body   )*dy_pilot_from_cg
					    + (-Q_body*Q_body - P_body*P_body)*dz_pilot_from_cg;
					    
    N_X_cg = INVG*A_X_cg;
    N_Y_cg = INVG*A_Y_cg;
    N_Z_cg = INVG*A_Z_cg;
    
    N_X_pilot = INVG*A_X_pilot;
    N_Y_pilot = INVG*A_Y_pilot;
    N_Z_pilot = INVG*A_Z_pilot;
    
    
    U_dot_body = T_local_to_body_11*V_dot_north + T_local_to_body_12*V_dot_east
				    + T_local_to_body_13*V_dot_down - Q_total*W_body + R_total*V_body;
    V_dot_body = T_local_to_body_21*V_dot_north + T_local_to_body_22*V_dot_east
				    + T_local_to_body_23*V_dot_down - R_total*U_body + P_total*W_body;
    W_dot_body = T_local_to_body_31*V_dot_north + T_local_to_body_32*V_dot_east
				    + T_local_to_body_33*V_dot_down - P_total*V_body + Q_total*U_body;
    /* End of ls_accel */
  
}
/**************************************************************************/




