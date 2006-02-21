/**********************************************************************

 FILENAME:     uiuc_coef_pitch.cpp

----------------------------------------------------------------------

 DESCRIPTION:  computes aggregated aerodynamic pitch coefficient

----------------------------------------------------------------------

 STATUS:       alpha version

----------------------------------------------------------------------

 REFERENCES:   Roskam, Jan.  Airplane Flight Dynamics and Automatic
               Flight Controls, Part I.  Lawrence, KS: DARcorporation,
               1995.

----------------------------------------------------------------------

 HISTORY:      04/15/2000   initial release
               10/25/2001   (RD) Added new variables needed for the non-
	                    linear Twin Otter model at zero flaps
			    (Cmfxxf0)
	       11/12/2001   (RD) Added new variables needed for the non-
	                    linear Twin Otter model with flaps
			    (Cmfxxf).  Zero flap vairables removed.
	       02/13/2002   (RD) Added variables so linear aero model
	                    values can be recorded
	       02/18/2002   (RD) Added uiuc_3Dinterp_quick() function
	                    for a quicker 3D interpolation.  Takes
			    advantage of "nice" data.

----------------------------------------------------------------------

 AUTHOR(S):    Bipin Sehgal       <bsehgal@uiuc.edu>
               Jeff Scott         <jscott@mail.com>
	       Robert Deters      <rdeters@uiuc.edu>

----------------------------------------------------------------------

 VARIABLES:

----------------------------------------------------------------------

 INPUTS:       -Alpha
               -elevator
               -pitch coefficient components
               -icing parameters
               -cbar_2U multiplier

----------------------------------------------------------------------

 OUTPUTS:      -Cm

----------------------------------------------------------------------

 CALLED BY:    uiuc_coefficients.cpp

----------------------------------------------------------------------

 CALLS TO:     uiuc_1Dinterpolation
               uiuc_2Dinterpolation
               uiuc_ice_filter
	       uiuc_3Dinterpolation
	       uiuc_3Dinterp_quick

----------------------------------------------------------------------

 COPYRIGHT:    (C) 2000 by Michael Selig

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

**********************************************************************/

#include "uiuc_coef_pitch.h"


void uiuc_coef_pitch()
{
  string linetoken1;
  string linetoken2;
  stack command_list;

  double q_nondim;
  
  command_list = aeroPitchParts -> getCommands();
  
  for (LIST command_line = command_list.begin(); command_line!=command_list.end(); ++command_line)
    {
      linetoken1 = aeroPitchParts -> getToken(*command_line, 1);
      linetoken2 = aeroPitchParts -> getToken(*command_line, 2);

      switch(Cm_map[linetoken2])
        {
        case Cmo_flag:
          {
            if (ice_on)
              {
                Cmo = uiuc_ice_filter(Cmo_clean,kCmo);
              }
	    Cmo_save = Cmo;
            Cm += Cmo_save;
            break;
          }
        case Cm_a_flag:
          {
            if (ice_on)
              {
                Cm_a = uiuc_ice_filter(Cm_a_clean,kCm_a);
              }
	    Cm_a_save = Cm_a * Std_Alpha;
            Cm += Cm_a_save;
            break;
          }
        case Cm_a2_flag:
          {
            if (ice_on)
              {
                Cm_a2 = uiuc_ice_filter(Cm_a2_clean,kCm_a2);
              }
	    Cm_a2_save = Cm_a2 * Std_Alpha * Std_Alpha;
            Cm += Cm_a2_save;
            break;
          }
        case Cm_adot_flag:
          {
            if (ice_on)
              {
                Cm_adot = uiuc_ice_filter(Cm_adot_clean,kCm_adot);
              }
            /* Cm_adot must be mulitplied by cbar/2U 
               (see Roskam Control book, Part 1, pg. 147) */
	    Cm_adot_save = Cm_adot * Std_Alpha_dot * cbar_2U;
	    if (eta_q_Cm_adot_fac)
	      {
		Cm += Cm_adot_save * eta_q_Cm_adot_fac;
	      }
	    else
	      {
		Cm += Cm_adot_save;
	      }
            break;
          }
        case Cm_q_flag:
          {
            if (ice_on)
              {
                Cm_q = uiuc_ice_filter(Cm_q_clean,kCm_q);
              }
            /* Cm_q must be mulitplied by cbar/2U 
               (see Roskam Control book, Part 1, pg. 147) */
	    Cm_q_save = Cm_q * Q_body * cbar_2U;
	    if (eta_q_Cm_q_fac)
	      {
		Cm += Cm_q_save * eta_q_Cm_q_fac;
	      }
	    else
	      {
		Cm += Cm_q_save;
	      }
            break;
          }
        case Cm_ih_flag:
          {
	    Cm_ih_save = Cm_ih * ih;
            Cm += Cm_ih_save;
            break;
          }
        case Cm_de_flag:
          {
            if (ice_on)
              {
                Cm_de = uiuc_ice_filter(Cm_de_clean,kCm_de);
              }
	    Cm_de_save = Cm_de * elevator;
	    if (eta_q_Cm_de_fac)
	      {
		Cm += Cm_de_save * eta_q_Cm_de_fac;
	      }
	    else
	      {
		Cm += Cm_de_save;
	      }
            break;
          }
        case Cm_b2_flag:
          {
            if (ice_on)
              {
                Cm_b2 = uiuc_ice_filter(Cm_b2_clean,kCm_b2);
              }
	    Cm_b2_save = Cm_b2 * Std_Beta * Std_Beta;
            Cm += Cm_b2_save;
            break;
          }
        case Cm_r_flag:
          {
            if (ice_on)
              {
                Cm_r = uiuc_ice_filter(Cm_r_clean,kCm_r);
              }
	    Cm_r_save = Cm_r * R_body * b_2U;
            Cm += Cm_r_save;
            break;
          }
        case Cm_df_flag:
          {
            if (ice_on)
              {
                Cm_df = uiuc_ice_filter(Cm_df_clean,kCm_df);
              }
	    Cm_df_save = Cm_df * flap_pos;
            Cm += Cm_df_save;
            break;
          }
        case Cm_ds_flag:
          {
	    Cm_ds_save = Cm_ds * spoiler_pos;
            Cm += Cm_ds_save;
            break;
          }
        case Cm_dg_flag:
          {
	    Cm_dg_save = Cm_dg * gear_pos_norm;
            Cm += Cm_dg_save;
            break;
          }
        case Cmfa_flag:
          {
            CmfaI = uiuc_1Dinterpolation(Cmfa_aArray,
                                         Cmfa_CmArray,
                                         Cmfa_nAlpha,
                                         Std_Alpha);
            Cm += CmfaI;
            break;
          }
        case Cmfade_flag:
          {
	    if(b_downwashMode)
	      {
		// compute the induced velocity on the tail to account for tail downwash
		switch(downwashMode)
		  {
		  case 100:	       
		    if (V_rel_wind < dyn_on_speed)
		      {
			alphaTail = Std_Alpha;
		      }
		    else
		      {
			gammaWing = V_rel_wind * Sw * CL / (2.0 * bw);
			// printf("gammaWing = %.4f\n", (gammaWing));
			downwash  = downwashCoef * gammaWing;
			downwashAngle = atan(downwash/V_rel_wind);
			alphaTail = Std_Alpha - downwashAngle;
		      }
		    CmfadeI = uiuc_2Dinterpolation(Cmfade_aArray,
						   Cmfade_deArray,
						   Cmfade_CmArray,
						   Cmfade_nAlphaArray,
						   Cmfade_nde,
						   alphaTail,
						   elevator);
		    break;
		  }
	      }
	    else
	      {
		CmfadeI = uiuc_2Dinterpolation(Cmfade_aArray,
					       Cmfade_deArray,
					       Cmfade_CmArray,
					       Cmfade_nAlphaArray,
					       Cmfade_nde,
					       Std_Alpha,
					       elevator); 
	      }
	    if (eta_q_Cmfade_fac)
	      {
		Cm += CmfadeI * eta_q_Cmfade_fac;
	      }
	    else
	      {
		Cm += CmfadeI;
	      }
            break;
          }
        case Cmfdf_flag:
          {
            CmfdfI = uiuc_1Dinterpolation(Cmfdf_dfArray,
                                          Cmfdf_CmArray,
                                          Cmfdf_ndf,
                                          flap_pos);
            Cm += CmfdfI;
            break;
          }
        case Cmfadf_flag:
          {
            CmfadfI = uiuc_2Dinterpolation(Cmfadf_aArray,
                                           Cmfadf_dfArray,
                                           Cmfadf_CmArray,
                                           Cmfadf_nAlphaArray,
                                           Cmfadf_ndf,
                                           Std_Alpha,
                                           flap_pos);
            Cm += CmfadfI;
            break;
          }
        case Cmfabetaf_flag:
          {
	    if (Cmfabetaf_nice == 1)
	      CmfabetafI = uiuc_3Dinterp_quick(Cmfabetaf_fArray,
					       Cmfabetaf_aArray_nice,
					       Cmfabetaf_bArray_nice,
					       Cmfabetaf_CmArray,
					       Cmfabetaf_na_nice,
					       Cmfabetaf_nb_nice,
					       Cmfabetaf_nf,
					       flap_pos,
					       Std_Alpha,
					       Std_Beta);
	    else
	      CmfabetafI = uiuc_3Dinterpolation(Cmfabetaf_fArray,
						Cmfabetaf_aArray,
						Cmfabetaf_betaArray,
						Cmfabetaf_CmArray,
						Cmfabetaf_nAlphaArray,
						Cmfabetaf_nbeta,
						Cmfabetaf_nf,
						flap_pos,
						Std_Alpha,
						Std_Beta);
            Cm += CmfabetafI;
            break;
          }
        case Cmfadef_flag:
          {
	    if (Cmfadef_nice == 1)
	      CmfadefI = uiuc_3Dinterp_quick(Cmfadef_fArray,
					     Cmfadef_aArray_nice,
					     Cmfadef_deArray_nice,
					     Cmfadef_CmArray,
					     Cmfadef_na_nice,
					     Cmfadef_nde_nice,
					     Cmfadef_nf,
					     flap_pos,
					     Std_Alpha,
					     elevator);
	    else
	      CmfadefI = uiuc_3Dinterpolation(Cmfadef_fArray,
					      Cmfadef_aArray,
					      Cmfadef_deArray,
					      Cmfadef_CmArray,
					      Cmfadef_nAlphaArray,
					      Cmfadef_nde,
					      Cmfadef_nf,
					      flap_pos,
					      Std_Alpha,
					      elevator);
	    Cm += CmfadefI;
            break;
          }
        case Cmfaqf_flag:
          {
	    q_nondim = Q_body * cbar_2U;
	    if (Cmfaqf_nice == 1)
	      CmfaqfI = uiuc_3Dinterp_quick(Cmfaqf_fArray,
					    Cmfaqf_aArray_nice,
					    Cmfaqf_qArray_nice,
					    Cmfaqf_CmArray,
					    Cmfaqf_na_nice,
					    Cmfaqf_nq_nice,
					    Cmfaqf_nf,
					    flap_pos,
					    Std_Alpha,
					    q_nondim);
	    else
	      CmfaqfI = uiuc_3Dinterpolation(Cmfaqf_fArray,
					     Cmfaqf_aArray,
					     Cmfaqf_qArray,
					     Cmfaqf_CmArray,
					     Cmfaqf_nAlphaArray,
					     Cmfaqf_nq,
					     Cmfaqf_nf,
					     flap_pos,
					     Std_Alpha,
					     q_nondim);
            Cm += CmfaqfI;
            break;
          }
        };
    } // end Cm map

  return;
}

// end uiuc_coef_pitch.cpp
