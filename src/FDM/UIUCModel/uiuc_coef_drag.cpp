/**********************************************************************

 FILENAME:     uiuc_coef_drag.cpp

----------------------------------------------------------------------

 DESCRIPTION:  computes aggregated aerodynamic drag coefficient

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
               -drag coefficient components
               -icing parameters

----------------------------------------------------------------------

 OUTPUTS:      -CD

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

#include "uiuc_coef_drag.h"


void uiuc_coef_drag()
{
  string linetoken1;
  string linetoken2;
  stack command_list;
  
  command_list = aeroDragParts -> getCommands();
  
  for (LIST command_line = command_list.begin(); command_line!=command_list.end(); ++command_line)
    {
      linetoken1 = aeroDragParts -> getToken(*command_line, 1);
      linetoken2 = aeroDragParts -> getToken(*command_line, 2);

      switch (CD_map[linetoken2])
        {
        case CDo_flag:
          {
            if (ice_on)
              {
                CDo = uiuc_ice_filter(CDo_clean,kCDo);
              }
            CD += CDo;
            break;
          }
        case CDK_flag:
          {
            if (ice_on)
              {
                CDK = uiuc_ice_filter(CDK_clean,kCDK);
              }
            CD += CDK * CL * CL;
            break;
          }
        case CD_a_flag:
          {
            if (ice_on)
              {
                CD_a = uiuc_ice_filter(CD_a_clean,kCD_a);
              }
            CD += CD_a * Alpha;
            break;
          }
        case CD_adot_flag:
          {
            if (ice_on)
              {
                CD_adot = uiuc_ice_filter(CD_adot_clean,kCD_adot);
              }
            /* CD_adot must be mulitplied by cbar/2U 
               (see Roskam Control book, Part 1, pg. 147) */
            CD += CD_adot * Alpha_dot * cbar_2U;
            break;
          }
        case CD_q_flag:
          {
            if (ice_on)
              {
                CD_q = uiuc_ice_filter(CD_q_clean,kCD_q);
              }
            /* CD_q must be mulitplied by cbar/2U 
               (see Roskam Control book, Part 1, pg. 147) */
            /* why multiply by Theta_dot instead of Q_body? 
               see note in coef_lift.cpp */
            CD += CD_q * Theta_dot * cbar_2U;
            break;
          }
        case CD_ih_flag:
          {
            CD += CD_ih * ih;
            break;
          }
        case CD_de_flag:
          {
            if (ice_on)
              {
                CD_de = uiuc_ice_filter(CD_de_clean,kCD_de);
              }
            CD += CD_de * elevator;
            break;
          }
        case CDfa_flag:
          {
            CDfaI = uiuc_1Dinterpolation(CDfa_aArray,
                                         CDfa_CDArray,
                                         CDfa_nAlpha,
                                         Alpha);
            CD += CDfaI;
            break;
          }
        case CDfCL_flag:
          {
            CDfCLI = uiuc_1Dinterpolation(CDfCL_CLArray,
                                          CDfCL_CDArray,
                                          CDfCL_nCL,
                                          CL);
            CD += CDfCLI;
            break;
          }
        case CDfdf_flag:
          {
            CDfdfI = uiuc_1Dinterpolation(CDfdf_dfArray,
                                          CDfdf_CDArray,
                                          CDfdf_ndf,
                                          flap);
            CD += CDfdfI;
            break;
          }
        case CDfade_flag:
          {
            CDfadeI = uiuc_2Dinterpolation(CDfade_aArray,
                                           CDfade_deArray,
                                           CDfade_CDArray,
                                           CDfade_nAlphaArray,
                                           CDfade_nde,
                                           Alpha,
                                           elevator);
                  CD += CDfadeI;
                  break;
          }
        case CDfadf_flag:
          {
            CDfadfI = uiuc_2Dinterpolation(CDfadf_aArray,
                                           CDfadf_dfArray,
                                           CDfadf_CDArray,
                                           CDfadf_nAlphaArray,
                                           CDfadf_ndf,
                                           Alpha,
                                           flap);
            CD += CDfadfI;
            break;
          }
        case CXo_flag:
          {
            if (ice_on)
              {
                CXo = uiuc_ice_filter(CXo_clean,kCXo);
                if (beta_model)
                  {
                    CXclean_wing += CXo_clean;
                    CXclean_tail += CXo_clean;
                    CXiced_wing += CXo;
                    CXiced_tail += CXo;
                  }
              }
            CX += CXo;
            break;
          }
        case CXK_flag:
          {
            if (ice_on)
              {
                CXK = uiuc_ice_filter(CXK_clean,kCXK);
                if (beta_model)
                  {
                    CXclean_wing += CXK_clean * CLclean_wing * CLclean_wing;
                    CXclean_tail += CXK_clean * CLclean_tail * CLclean_tail;
                    CXiced_wing += CXK * CLiced_wing * CLiced_wing;
                    CXiced_tail += CXK * CLiced_tail * CLiced_tail;
                  }
              }
            CX += CXK * CZ * CZ;
            break;
          }
        case CX_a_flag:
          {
            if (ice_on)
              {
                CX_a = uiuc_ice_filter(CX_a_clean,kCX_a);
                if (beta_model)
                  {
                    CXclean_wing += CX_a_clean * Alpha;
                    CXclean_tail += CX_a_clean * Alpha;
                    CXiced_wing += CX_a * Alpha;
                    CXiced_tail += CX_a * Alpha;
                  }
              }
            CX += CX_a * Alpha;
            break;
          }
        case CX_a2_flag:
          {
            if (ice_on)
              {
                CX_a2 = uiuc_ice_filter(CX_a2_clean,kCX_a2);
                if (beta_model)
                  {
                    CXclean_wing += CX_a2_clean * Alpha * Alpha;
                    CXclean_tail += CX_a2_clean * Alpha * Alpha;
                    CXiced_wing += CX_a2 * Alpha * Alpha;
                    CXiced_tail += CX_a2 * Alpha * Alpha;
                  }
              }
            CX += CX_a2 * Alpha * Alpha;
            break;
          }
        case CX_a3_flag:
          {
            if (ice_on)
              {
                CX_a3 = uiuc_ice_filter(CX_a3_clean,kCX_a3);
                if (beta_model)
                  {
                    CXclean_wing += CX_a3_clean * Alpha * Alpha * Alpha;
                    CXclean_tail += CX_a3_clean * Alpha * Alpha * Alpha;
                    CXiced_wing += CX_a3 * Alpha * Alpha * Alpha;
                    CXiced_tail += CX_a3 * Alpha * Alpha * Alpha;
                  }
              }
            CX += CX_a3 * Alpha * Alpha * Alpha;
            break;
          }
        case CX_adot_flag:
          {
            if (ice_on)
              {
                CX_adot = uiuc_ice_filter(CX_adot_clean,kCX_adot);
                if (beta_model)
                  {
                    CXclean_wing += CX_adot_clean * Alpha_dot * cbar_2U;
                    CXclean_tail += CX_adot_clean * Alpha_dot * ch_2U;
                    CXiced_wing += CX * Alpha_dot * cbar_2U;
                    CXiced_tail += CX * Alpha_dot * ch_2U;
                  }
              }
            /* CX_adot must be mulitplied by cbar/2U 
               (see Roskam Control book, Part 1, pg. 147) */
            CX += CX_adot * Alpha_dot * cbar_2U;
            break;
          }
        case CX_q_flag:
          {
            if (ice_on)
              {
                CX_q = uiuc_ice_filter(CX_q_clean,kCX_q);
                if (beta_model)
                  {
                    CXclean_wing += CX_q_clean * Q_body * cbar_2U;
                    CXclean_tail += CX_q_clean * Q_body * ch_2U;
                    CXiced_wing += CX_q * Q_body * cbar_2U;
                    CXiced_tail += CX_q * Q_body * ch_2U;
                  }
              }
            /* CX_q must be mulitplied by cbar/2U 
               (see Roskam Control book, Part 1, pg. 147) */
            CX += CX_q * Q_body * cbar_2U;
            break;
          }
        case CX_de_flag:
          {
            if (ice_on)
              {
                CX_de = uiuc_ice_filter(CX_de_clean,kCX_de);
                if (beta_model)
                  {
                    CXclean_wing += CX_de_clean * elevator;
                    CXclean_tail += CX_de_clean * elevator;
                    CXiced_wing += CX_de * elevator;
                    CXiced_tail += CX_de * elevator;
                  }
              }
            CX += CX_de * elevator;
            break;
          }
        case CX_dr_flag:
          {
            if (ice_on)
              {
                CX_dr = uiuc_ice_filter(CX_dr_clean,kCX_dr);
                if (beta_model)
                  {
                    CXclean_wing += CX_dr_clean * rudder;
                    CXclean_tail += CX_dr_clean * rudder;
                    CXiced_wing += CX_dr * rudder;
                    CXiced_tail += CX_dr * rudder;
                  }
              }
            CX += CX_dr * rudder;
            break;
          }
        case CX_df_flag:
          {
            if (ice_on)
              {
                CX_df = uiuc_ice_filter(CX_df_clean,kCX_df);
                if (beta_model)
                  {
                    CXclean_wing += CX_df_clean * flap;
                    CXclean_tail += CX_df_clean * flap;
                    CXiced_wing += CX * flap;
                    CXiced_tail += CX * flap;
                  }
              }
            CX += CX_df * flap;
            break;
          }
        case CX_adf_flag:
          {
            if (ice_on)
              {
                CX_adf = uiuc_ice_filter(CX_adf_clean,kCX_adf);
                if (beta_model)
                  {
                    CXclean_wing += CX_adf_clean * Alpha * flap;
                    CXclean_tail += CX_adf_clean * Alpha * flap;
                    CXiced_wing += CX_adf * Alpha * flap;
                    CXiced_tail += CX_adf * Alpha * flap;
                  }
              }
            CX += CX_adf * Alpha * flap;
            break;
          }
        };
    } // end CD map

  return;
}

// end uiuc_coef_drag.cpp
