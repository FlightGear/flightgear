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

----------------------------------------------------------------------

 AUTHOR(S):    Bipin Sehgal       <bsehgal@uiuc.edu>
               Jeff Scott         <jscott@mail.com>

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

#include "uiuc_coef_pitch.h"


void uiuc_coef_pitch()
{
  string linetoken1;
  string linetoken2;
  stack command_list;
  
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
            Cm += Cmo;
            break;
          }
        case Cm_a_flag:
          {
            if (ice_on)
              {
                Cm_a = uiuc_ice_filter(Cm_a_clean,kCm_a);
              }
            Cm += Cm_a * Alpha;
            break;
          }
        case Cm_a2_flag:
          {
            if (ice_on)
              {
                Cm_a2 = uiuc_ice_filter(Cm_a2_clean,kCm_a2);
              }
            Cm += Cm_a2 * Alpha * Alpha;
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
            Cm += Cm_adot * Alpha_dot * cbar_2U;
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
            Cm += Cm_q * Q_body * cbar_2U;
            break;
          }
        case Cm_ih_flag:
          {
            Cm += Cm_ih * ih;
            break;
          }
        case Cm_de_flag:
          {
            if (ice_on)
              {
                Cm_de = uiuc_ice_filter(Cm_de_clean,kCm_de);
              }
            Cm += Cm_de * elevator;
            break;
          }
        case Cm_b2_flag:
          {
            if (ice_on)
              {
                Cm_b2 = uiuc_ice_filter(Cm_b2_clean,kCm_b2);
              }
            Cm += Cm_b2 * Beta * Beta;
            break;
          }
        case Cm_r_flag:
          {
            if (ice_on)
              {
                Cm_r = uiuc_ice_filter(Cm_r_clean,kCm_r);
              }
            Cm += Cm_r * R_body * b_2U;
            break;
          }
        case Cm_df_flag:
          {
            if (ice_on)
              {
                Cm_df = uiuc_ice_filter(Cm_df_clean,kCm_df);
              }
            Cm += Cm_df * flap;
            break;
          }
        case Cmfa_flag:
          {
            CmfaI = uiuc_1Dinterpolation(Cmfa_aArray,
                                         Cmfa_CmArray,
                                         Cmfa_nAlpha,
                                         Alpha);
            Cm += CmfaI;
            break;
          }
        case Cmfade_flag:
          {
            CmfadeI = uiuc_2Dinterpolation(Cmfade_aArray,
                                           Cmfade_deArray,
                                           Cmfade_CmArray,
                                           Cmfade_nAlphaArray,
                                           Cmfade_nde,
                                           Alpha,
                                           elevator);
            Cm += CmfadeI;
            break;
          }
        case Cmfdf_flag:
          {
            CmfdfI = uiuc_1Dinterpolation(Cmfdf_dfArray,
                                          Cmfdf_CmArray,
                                          Cmfdf_ndf,
                                          flap);
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
                                           Alpha,
                                           flap);
            Cm += CmfadfI;
            break;
          }
        };
    } // end Cm map

  return;
}

// end uiuc_coef_pitch.cpp
