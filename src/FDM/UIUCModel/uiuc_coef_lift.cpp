/**********************************************************************

 FILENAME:     uiuc_coef_lift.cpp

----------------------------------------------------------------------

 DESCRIPTION:  computes aggregated aerodynamic lift coefficient

----------------------------------------------------------------------

 STATUS:       alpha version

----------------------------------------------------------------------

 REFERENCES:   Roskam, Jan.  Airplane Flight Dynamics and Automatic
               Flight Controls, Part I.  Lawrence, KS: DARcorporation,
               1995.

----------------------------------------------------------------------

 HISTORY:      04/15/2000   initial release
               06/18/2001   (RD) Added CZfa
               10/25/2001   (RD) Added new variables needed for the non-
	                    linear Twin Otter model at zero flaps
			    (CZfxxf0)
	       11/12/2001   (RD) Added new variables needed for the non-
	                    linear Twin Otter model with flaps
			    (CZfxxf).  Zero flap vairables removed.
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
               -lift coefficient components
               -icing parameters
               -cbar_2U multiplier

----------------------------------------------------------------------

 OUTPUTS:      -CL

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

#include "uiuc_coef_lift.h"

void uiuc_coef_lift()
{
  string linetoken1;
  string linetoken2;
  stack command_list;

  double q_nondim;

  command_list = aeroLiftParts -> getCommands();
  
  for (LIST command_line = command_list.begin(); command_line!=command_list.end(); ++command_line)
    {
      linetoken1 = aeroLiftParts -> getToken(*command_line, 1);
      linetoken2 = aeroLiftParts -> getToken(*command_line, 2);

      switch (CL_map[linetoken2])
        {
        case CLo_flag:
          {
            if (ice_on)
              {
                CLo = uiuc_ice_filter(CLo_clean,kCLo);
                if (beta_model)
                  {
                    CLclean_wing += CLo_clean;
                    CLclean_tail += CLo_clean;
                    CLiced_wing += CLo;
                    CLiced_tail += CLo;
                  }
              }
	    CLo_save = CLo;
            CL += CLo_save;
            break;
          }
        case CL_a_flag:
          {
            if (ice_on)
              {
                CL_a = uiuc_ice_filter(CL_a_clean,kCL_a);
                if (beta_model)
                  {
                    CLclean_wing += CL_a_clean * Std_Alpha;
                    CLclean_tail += CL_a_clean * Std_Alpha;
                    CLiced_wing += CL_a * Std_Alpha;
                    CLiced_tail += CL_a * Std_Alpha;
                  }
              }
	    CL_a_save = CL_a * Std_Alpha;
            CL += CL_a_save;
            break;
          }
        case CL_adot_flag:
          {
            if (ice_on)
              {
                CL_adot = uiuc_ice_filter(CL_adot_clean,kCL_adot);
                if (beta_model)
                  {
                    CLclean_wing += CL_adot_clean * Std_Alpha_dot * cbar_2U;
                    CLclean_tail += CL_adot_clean * Std_Alpha_dot * ch_2U;
                    CLiced_wing += CL_adot * Std_Alpha_dot * cbar_2U;
                    CLiced_tail += CL_adot * Std_Alpha_dot * ch_2U;
                  }
              }
            /* CL_adot must be mulitplied by cbar/2U 
               (see Roskam Control book, Part 1, pg. 147) */
	    CL_adot_save = CL_adot * Std_Alpha_dot * cbar_2U;
            CL += CL_adot_save;
            break;
          }
        case CL_q_flag:
          {
            if (ice_on)
              {
                CL_q = uiuc_ice_filter(CL_q_clean,kCL_q);
                if (beta_model)
                  {
                    CLclean_wing += CL_q_clean * Theta_dot * cbar_2U;
                    CLclean_tail += CL_q_clean * Theta_dot * ch_2U;
                    CLiced_wing += CL_q * Theta_dot * cbar_2U;
                    CLiced_tail += CL_q * Theta_dot * ch_2U;
                  }
              }
            /* CL_q must be mulitplied by cbar/2U 
               (see Roskam Control book, Part 1, pg. 147) */
            /* why multiply by Theta_dot instead of Q_body?
               that is what is done in c172_aero.c; assume it 
               has something to do with axes systems */
	    CL_q_save = CL_q * Theta_dot * cbar_2U;
            CL += CL_q_save;
            break;
          }
        case CL_ih_flag:
          {
	    CL_ih_save = CL_ih * ih;
            CL += CL_ih_save;
            break;
          }
        case CL_de_flag:
          {
            if (ice_on)
              {
                CL_de = uiuc_ice_filter(CL_de_clean,kCL_de);
                if (beta_model)
                  {
                    CLclean_wing += CL_de_clean * elevator;
                    CLclean_tail += CL_de_clean * elevator;
                    CLiced_wing += CL_de * elevator;
                    CLiced_tail += CL_de * elevator;
                  }
              }
	    CL_de_save = CL_de * elevator;
            CL += CL_de_save;
            break;
          }
        case CL_df_flag:
          {
	    CL_df_save = CL_df * flap_pos;
            CL += CL_df_save;
            break;
          }
        case CL_ds_flag:
          {
	    CL_ds_save = CL_ds * spoiler_pos;
            CL += CL_ds_save;
            break;
          }
        case CL_dg_flag:
          {
	    CL_dg_save = CL_dg * gear_pos_norm;
            CL += CL_dg_save;
            break;
          }
        case CLfa_flag:
          {
            CLfaI = uiuc_1Dinterpolation(CLfa_aArray,
                                         CLfa_CLArray,
                                         CLfa_nAlpha,
                                         Std_Alpha);
            CL += CLfaI;
            break;
          }
        case CLfade_flag:
          {
            CLfadeI = uiuc_2Dinterpolation(CLfade_aArray,
                                           CLfade_deArray,
                                           CLfade_CLArray,
                                           CLfade_nAlphaArray,
                                           CLfade_nde,
                                           Std_Alpha,
                                           elevator);
            CL += CLfadeI;
            break;
          }
        case CLfdf_flag:
          {
            CLfdfI = uiuc_1Dinterpolation(CLfdf_dfArray,
                                          CLfdf_CLArray,
                                          CLfdf_ndf,
                                          flap_pos);
            CL += CLfdfI;
            break;
          }
        case CLfadf_flag:
          {
            CLfadfI = uiuc_2Dinterpolation(CLfadf_aArray,
                                           CLfadf_dfArray,
                                           CLfadf_CLArray,
                                           CLfadf_nAlphaArray,
                                           CLfadf_ndf,
                                           Std_Alpha,
                                           flap_pos);
            CL += CLfadfI;
            break;
          }
        case CZo_flag:
          {
            if (ice_on)
              {
                CZo = uiuc_ice_filter(CZo_clean,kCZo);
                if (beta_model)
                  {
                    CZclean_wing += CZo_clean;
                    CZclean_tail += CZo_clean;
                    CZiced_wing += CZo;
                    CZiced_tail += CZo;
                  }
              }
	    CZo_save = CZo;
            CZ += CZo_save;
            break;
          }
        case CZ_a_flag:
          {
            if (ice_on)
              {
                CZ_a = uiuc_ice_filter(CZ_a_clean,kCZ_a);
                if (beta_model)
                  {
                    CZclean_wing += CZ_a_clean * Std_Alpha;
                    CZclean_tail += CZ_a_clean * Std_Alpha;
                    CZiced_wing += CZ_a * Std_Alpha;
                    CZiced_tail += CZ_a * Std_Alpha;
                  }
              }
	    CZ_a_save = CZ_a * Std_Alpha;
            CZ += CZ_a_save;
            break;
          }
        case CZ_a2_flag:
          {
            if (ice_on)
              {
                CZ_a2 = uiuc_ice_filter(CZ_a2_clean,kCZ_a2);
                if (beta_model)
                  {
                    CZclean_wing += CZ_a2_clean * Std_Alpha * Std_Alpha;
                    CZclean_tail += CZ_a2_clean * Std_Alpha * Std_Alpha;
                    CZiced_wing += CZ_a2 * Std_Alpha * Std_Alpha;
                    CZiced_tail += CZ_a2 * Std_Alpha * Std_Alpha;
                  }
              }
	    CZ_a2_save = CZ_a2 * Std_Alpha * Std_Alpha;
            CZ += CZ_a2_save;
            break;
          }
        case CZ_a3_flag:
          {
            if (ice_on)
              {
                CZ_a3 = uiuc_ice_filter(CZ_a3_clean,kCZ_a3);
                if (beta_model)
                  {
                    CZclean_wing += CZ_a3_clean * Std_Alpha * Std_Alpha * Std_Alpha;
                    CZclean_tail += CZ_a3_clean * Std_Alpha * Std_Alpha * Std_Alpha;
                    CZiced_wing += CZ_a3 * Std_Alpha * Std_Alpha * Std_Alpha;
                    CZiced_tail += CZ_a3 * Std_Alpha * Std_Alpha * Std_Alpha;
                  }
              }
	    CZ_a3_save = CZ_a3 * Std_Alpha * Std_Alpha * Std_Alpha;
            CZ += CZ_a3_save;
            break;
          }
        case CZ_adot_flag:
          {
            if (ice_on)
              {
                CZ_adot = uiuc_ice_filter(CZ_adot_clean,kCZ_adot);
                if (beta_model)
                  {
                    CZclean_wing += CZ_adot_clean * Std_Alpha_dot * cbar_2U;
                    CZclean_tail += CZ_adot_clean * Std_Alpha_dot * ch_2U;
                    CZiced_wing += CZ_adot * Std_Alpha_dot * cbar_2U;
                    CZiced_tail += CZ_adot * Std_Alpha_dot * ch_2U;
                  }
              }
            /* CZ_adot must be mulitplied by cbar/2U 
               (see Roskam Control book, Part 1, pg. 147) */
	    CZ_adot_save = CZ_adot * Std_Alpha_dot * cbar_2U;
            CZ += CZ_adot_save;
            break;
          }
        case CZ_q_flag:
          {
            if (ice_on)
              {
                CZ_q = uiuc_ice_filter(CZ_q_clean,kCZ_q);
                if (beta_model)
                  {
                    CZclean_wing += CZ_q_clean * Q_body * cbar_2U;
                    CZclean_tail += CZ_q_clean * Q_body * ch_2U;
                    CZiced_wing += CZ_q * Q_body * cbar_2U;
                    CZiced_tail += CZ_q * Q_body * ch_2U;
                  }
              }
            /* CZ_q must be mulitplied by cbar/2U 
               (see Roskam Control book, Part 1, pg. 147) */
	    CZ_q_save = CZ_q * Q_body * cbar_2U;
            CZ += CZ_q_save;
            break;
          }
        case CZ_de_flag:
          {
            if (ice_on)
              {
                CZ_de = uiuc_ice_filter(CZ_de_clean,kCZ_de);
                if (beta_model)
                  {
                    CZclean_wing += CZ_de_clean * elevator;
                    CZclean_tail += CZ_de_clean * elevator;
                    CZiced_wing += CZ_de * elevator;
                    CZiced_tail += CZ_de * elevator;
                  }
              }
	    CZ_de_save = CZ_de * elevator;
            CZ += CZ_de_save;
            break;
          }
        case CZ_deb2_flag:
          {
            if (ice_on)
              {
                CZ_deb2 = uiuc_ice_filter(CZ_deb2_clean,kCZ_deb2);
                if (beta_model)
                  {
                    CZclean_wing += CZ_deb2_clean * elevator * Std_Beta * Std_Beta;
                    CZclean_tail += CZ_deb2_clean * elevator * Std_Beta * Std_Beta;
                    CZiced_wing += CZ_deb2 * elevator * Std_Beta * Std_Beta;
                    CZiced_tail += CZ_deb2 * elevator * Std_Beta * Std_Beta;
                  }
              }
	    CZ_deb2_save = CZ_deb2 * elevator * Std_Beta * Std_Beta;
            CZ += CZ_deb2_save;
            break;
          }
        case CZ_df_flag:
          {
            if (ice_on)
              {
                CZ_df = uiuc_ice_filter(CZ_df_clean,kCZ_df);
                if (beta_model)
                  {
                    CZclean_wing += CZ_df_clean * flap_pos;
                    CZclean_tail += CZ_df_clean * flap_pos;
                    CZiced_wing += CZ_df * flap_pos;
                    CZiced_tail += CZ_df * flap_pos;
                  }
              }
	    CZ_df_save = CZ_df * flap_pos;
            CZ += CZ_df_save;
            break;
          }
        case CZ_adf_flag:
          {
            if (ice_on)
              {
                CZ_adf = uiuc_ice_filter(CZ_adf_clean,kCZ_adf);
                if (beta_model)
                  {
                    CZclean_wing += CZ_adf_clean * Std_Alpha * flap_pos;
                    CZclean_tail += CZ_adf_clean * Std_Alpha * flap_pos;
                    CZiced_wing += CZ_adf * Std_Alpha * flap_pos;
                    CZiced_tail += CZ_adf * Std_Alpha * flap_pos;
                  }
              }
	    CZ_adf_save = CZ_adf * Std_Alpha * flap_pos;
            CZ += CZ_adf_save;
            break;
          }
        case CZfa_flag:
          {
            CZfaI = uiuc_1Dinterpolation(CZfa_aArray,
                                         CZfa_CZArray,
                                         CZfa_nAlpha,
                                         Std_Alpha);
            CZ += CZfaI;
            break;
          }
        case CZfabetaf_flag:
          {
	    if (CZfabetaf_nice == 1)
	      CZfabetafI = uiuc_3Dinterp_quick(CZfabetaf_fArray,
					       CZfabetaf_aArray_nice,
					       CZfabetaf_bArray_nice,
					       CZfabetaf_CZArray,
					       CZfabetaf_na_nice,
					       CZfabetaf_nb_nice,
					       CZfabetaf_nf,
					       flap_pos,
					       Std_Alpha,
					       Std_Beta);
	    else
	      CZfabetafI = uiuc_3Dinterpolation(CZfabetaf_fArray,
						CZfabetaf_aArray,
						CZfabetaf_betaArray,
						CZfabetaf_CZArray,
						CZfabetaf_nAlphaArray,
						CZfabetaf_nbeta,
						CZfabetaf_nf,
						flap_pos,
						Std_Alpha,
						Std_Beta);
            CZ += CZfabetafI;
            break;
          }
        case CZfadef_flag:
          {
	    if (CZfadef_nice == 1)
	      CZfadefI = uiuc_3Dinterp_quick(CZfadef_fArray,
					     CZfadef_aArray_nice,
					     CZfadef_deArray_nice,
					     CZfadef_CZArray,
					     CZfadef_na_nice,
					     CZfadef_nde_nice,
					     CZfadef_nf,
					     flap_pos,
					     Std_Alpha,
					     elevator);
	    else
	      CZfadefI = uiuc_3Dinterpolation(CZfadef_fArray,
					      CZfadef_aArray,
					      CZfadef_deArray,
					      CZfadef_CZArray,
					      CZfadef_nAlphaArray,
					      CZfadef_nde,
					      CZfadef_nf,
					      flap_pos,
					      Std_Alpha,
					      elevator);
            CZ += CZfadefI;
            break;
          }
        case CZfaqf_flag:
          {
	    q_nondim = Q_body * cbar_2U;
	    if (CZfaqf_nice == 1)
	      CZfaqfI = uiuc_3Dinterp_quick(CZfaqf_fArray,
					    CZfaqf_aArray_nice,
					    CZfaqf_qArray_nice,
					    CZfaqf_CZArray,
					    CZfaqf_na_nice,
					    CZfaqf_nq_nice,
					    CZfaqf_nf,
					    flap_pos,
					    Std_Alpha,
					    q_nondim);
	    else
	      CZfaqfI = uiuc_3Dinterpolation(CZfaqf_fArray,
					     CZfaqf_aArray,
					     CZfaqf_qArray,
					     CZfaqf_CZArray,
					     CZfaqf_nAlphaArray,
					     CZfaqf_nq,
					     CZfaqf_nf,
					     flap_pos,
					     Std_Alpha,
					     q_nondim);
            CZ += CZfaqfI;
            break;
          }
        };
    } // end CL map

  return;
}

// end uiuc_coef_lift.cpp
