/**********************************************************************

 FILENAME:     uiuc_coef_sideforce.cpp

----------------------------------------------------------------------

 DESCRIPTION:  computes aggregated aerodynamic sideforce coefficient

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
               -aileron
               -rudder
               -sideforce coefficient components
               -icing parameters
               -b_2U multiplier

----------------------------------------------------------------------

 OUTPUTS:      -CY

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

#include "uiuc_coef_sideforce.h"


void uiuc_coef_sideforce()
{
  string linetoken1;
  string linetoken2;
  stack command_list;
  
  command_list = aeroSideforceParts -> getCommands();
  
  for (LIST command_line = command_list.begin(); command_line!=command_list.end(); ++command_line)
    {
      linetoken1 = aeroSideforceParts -> getToken(*command_line, 1);
      linetoken2 = aeroSideforceParts -> getToken(*command_line, 2);

      switch(CY_map[linetoken2])
        {
        case CYo_flag:
          {
            if (ice_on)
              {
                CYo = uiuc_ice_filter(CYo_clean,kCYo);
              }
            CY += CYo;
            break;
          }
        case CY_beta_flag:
          {
            if (ice_on)
              {
                CY_beta = uiuc_ice_filter(CY_beta_clean,kCY_beta);
              }
            CY += CY_beta * Beta;
            break;
          }
        case CY_p_flag:
          {
            if (ice_on)
              {
                CY_p = uiuc_ice_filter(CY_p_clean,kCY_p);
              }
            /* CY_p must be mulitplied by b/2U 
               (see Roskam Control book, Part 1, pg. 147) */
            CY += CY_p * P_body * b_2U;
            break;
          }
        case CY_r_flag:
          {
            if (ice_on)
              {
                CY_r = uiuc_ice_filter(CY_r_clean,kCY_r);
              }
            /* CY_r must be mulitplied by b/2U 
               (see Roskam Control book, Part 1, pg. 147) */
            CY += CY_r * R_body * b_2U;
            break;
          }
        case CY_da_flag:
          {
            if (ice_on)
              {
                CY_da = uiuc_ice_filter(CY_da_clean,kCY_da);
              }
            CY += CY_da * aileron;
            break;
          }
        case CY_dr_flag:
          {
            if (ice_on)
              {
                CY_dr = uiuc_ice_filter(CY_dr_clean,kCY_dr);
              }
            CY += CY_dr * rudder;
            break;
          }
        case CY_dra_flag:
          {
            if (ice_on)
              {
                CY_dra = uiuc_ice_filter(CY_dra_clean,kCY_dra);
              }
            CY += CY_dra * rudder * Alpha;
            break;
          }
        case CY_bdot_flag:
          {
            if (ice_on)
              {
                CY_bdot = uiuc_ice_filter(CY_bdot_clean,kCY_bdot);
              }
            CY += CY_bdot * Beta_dot * b_2U;
            break;
          }
        case CYfada_flag:
          {
            CYfadaI = uiuc_2Dinterpolation(CYfada_aArray,
                                           CYfada_daArray,
                                           CYfada_CYArray,
                                           CYfada_nAlphaArray,
                                           CYfada_nda,
                                           Alpha,
                                           aileron);
            CY += CYfadaI;
            break;
          }
        case CYfbetadr_flag:
          {
            CYfbetadrI = uiuc_2Dinterpolation(CYfbetadr_betaArray,
                                              CYfbetadr_drArray,
                                              CYfbetadr_CYArray,
                                              CYfbetadr_nBetaArray,
                                              CYfbetadr_ndr,
                                              Beta,
                                              rudder);
            CY += CYfbetadrI;
            break;
          }
        };
    } // end CY map
  
  return;
}

// end uiuc_coef_sideforce.cpp
