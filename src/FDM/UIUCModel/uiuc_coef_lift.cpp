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

----------------------------------------------------------------------

 AUTHOR(S):    Bipin Sehgal       <bsehgal@uiuc.edu>
               Jeff Scott         <jscott@mail.com>

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
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
 USA or view http://www.gnu.org/copyleft/gpl.html.

**********************************************************************/

#include "uiuc_coef_lift.h"


void uiuc_coef_lift()
{
  string linetoken1;
  string linetoken2;
  stack command_list;
  
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
            CL += CLo;
            break;
          }
        case CL_a_flag:
          {
            if (ice_on)
              {
                CL_a = uiuc_ice_filter(CL_a_clean,kCL_a);
                if (beta_model)
                  {
                    CLclean_wing += CL_a_clean * Alpha;
                    CLclean_tail += CL_a_clean * Alpha;
                    CLiced_wing += CL_a * Alpha;
                    CLiced_tail += CL_a * Alpha;
                  }
              }
            CL += CL_a * Alpha;
            break;
          }
        case CL_adot_flag:
          {
            if (ice_on)
              {
                CL_adot = uiuc_ice_filter(CL_adot_clean,kCL_adot);
                if (beta_model)
                  {
                    CLclean_wing += CL_adot_clean * Alpha_dot * cbar_2U;
                    CLclean_tail += CL_adot_clean * Alpha_dot * ch_2U;
                    CLiced_wing += CL_adot * Alpha_dot * cbar_2U;
                    CLiced_tail += CL_adot * Alpha_dot * ch_2U;
                  }
              }
            /* CL_adot must be mulitplied by cbar/2U 
               (see Roskam Control book, Part 1, pg. 147) */
            CL += CL_adot * Alpha_dot * cbar_2U;
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
            CL += CL_q * Theta_dot * cbar_2U;
            break;
          }
        case CL_ih_flag:
          {
            CL += CL_ih * ih;
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
            CL += CL_de * elevator;
            break;
          }
        case CLfa_flag:
          {
            CLfaI = uiuc_1Dinterpolation(CLfa_aArray,
                                         CLfa_CLArray,
                                         CLfa_nAlpha,
                                         Alpha);
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
                                           Alpha,
                                           elevator);
            CL += CLfadeI;
            break;
          }
        case CLfdf_flag:
          {
            CLfdfI = uiuc_1Dinterpolation(CLfdf_dfArray,
                                          CLfdf_CLArray,
                                          CLfdf_ndf,
                                          flap);
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
                                           Alpha,
                                           flap);
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
            CZ += CZo;
            break;
          }
        case CZ_a_flag:
          {
            if (ice_on)
              {
                CZ_a = uiuc_ice_filter(CZ_a_clean,kCZ_a);
                if (beta_model)
                  {
                    CZclean_wing += CZ_a_clean * Alpha;
                    CZclean_tail += CZ_a_clean * Alpha;
                    CZiced_wing += CZ_a * Alpha;
                    CZiced_tail += CZ_a * Alpha;
                  }
              }
            CZ += CZ_a * Alpha;
            break;
          }
        case CZ_a2_flag:
          {
            if (ice_on)
              {
                CZ_a2 = uiuc_ice_filter(CZ_a2_clean,kCZ_a2);
                if (beta_model)
                  {
                    CZclean_wing += CZ_a2_clean * Alpha * Alpha;
                    CZclean_tail += CZ_a2_clean * Alpha * Alpha;
                    CZiced_wing += CZ_a2 * Alpha * Alpha;
                    CZiced_tail += CZ_a2 * Alpha * Alpha;
                  }
              }
            CZ += CZ_a2 * Alpha * Alpha;
            break;
          }
        case CZ_a3_flag:
          {
            if (ice_on)
              {
                CZ_a3 = uiuc_ice_filter(CZ_a3_clean,kCZ_a3);
                if (beta_model)
                  {
                    CZclean_wing += CZ_a3_clean * Alpha * Alpha * Alpha;
                    CZclean_tail += CZ_a3_clean * Alpha * Alpha * Alpha;
                    CZiced_wing += CZ_a3 * Alpha * Alpha * Alpha;
                    CZiced_tail += CZ_a3 * Alpha * Alpha * Alpha;
                  }
              }
            CZ += CZ_a3 * Alpha * Alpha * Alpha;
            break;
          }
        case CZ_adot_flag:
          {
            if (ice_on)
              {
                CZ_adot = uiuc_ice_filter(CZ_adot_clean,kCZ_adot);
                if (beta_model)
                  {
                    CZclean_wing += CZ_adot_clean * Alpha_dot * cbar_2U;
                    CZclean_tail += CZ_adot_clean * Alpha_dot * ch_2U;
                    CZiced_wing += CZ_adot * Alpha_dot * cbar_2U;
                    CZiced_tail += CZ_adot * Alpha_dot * ch_2U;
                  }
              }
            /* CZ_adot must be mulitplied by cbar/2U 
               (see Roskam Control book, Part 1, pg. 147) */
            CZ += CZ_adot * Alpha_dot * cbar_2U;
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
            CZ += CZ_q * Q_body * cbar_2U;
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
            CZ += CZ_de * elevator;
            break;
          }
        case CZ_deb2_flag:
          {
            if (ice_on)
              {
                CZ_deb2 = uiuc_ice_filter(CZ_deb2_clean,kCZ_deb2);
                if (beta_model)
                  {
                    CZclean_wing += CZ_deb2_clean * elevator * Beta * Beta;
                    CZclean_tail += CZ_deb2_clean * elevator * Beta * Beta;
                    CZiced_wing += CZ_deb2 * elevator * Beta * Beta;
                    CZiced_tail += CZ_deb2 * elevator * Beta * Beta;
                  }
              }
            CZ += CZ_deb2 * elevator * Beta * Beta;
            break;
          }
        case CZ_df_flag:
          {
            if (ice_on)
              {
                CZ_df = uiuc_ice_filter(CZ_df_clean,kCZ_df);
                if (beta_model)
                  {
                    CZclean_wing += CZ_df_clean * flap;
                    CZclean_tail += CZ_df_clean * flap;
                    CZiced_wing += CZ_df * flap;
                    CZiced_tail += CZ_df * flap;
                  }
              }
            CZ += CZ_df * flap;
            break;
          }
        case CZ_adf_flag:
          {
            if (ice_on)
              {
                CZ_adf = uiuc_ice_filter(CZ_adf_clean,kCZ_adf);
                if (beta_model)
                  {
                    CZclean_wing += CZ_adf_clean * Alpha * flap;
                    CZclean_tail += CZ_adf_clean * Alpha * flap;
                    CZiced_wing += CZ_adf * Alpha * flap;
                    CZiced_tail += CZ_adf * Alpha * flap;
                  }
              }
            CZ += CZ_adf * Alpha * flap;
            break;
          }
        };
    } // end CL map

  return;
}

// end uiuc_coef_lift.cpp
