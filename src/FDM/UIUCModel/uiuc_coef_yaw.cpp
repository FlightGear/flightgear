/**********************************************************************

 FILENAME:     uiuc_coef_yaw.cpp

----------------------------------------------------------------------

 DESCRIPTION:  computes aggregated aerodynamic yaw coefficient

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
               -yaw coefficient components
               -icing parameters
               -b_2U multiplier

----------------------------------------------------------------------

 OUTPUTS:      -Cn

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

#include "uiuc_coef_yaw.h"


void uiuc_coef_yaw()
{
  string linetoken1;
  string linetoken2;
  stack command_list;
  
  command_list = aeroYawParts -> getCommands();
  
  for (LIST command_line = command_list.begin(); command_line!=command_list.end(); ++command_line)
    {
      linetoken1 = aeroYawParts -> getToken(*command_line, 1);
      linetoken2 = aeroYawParts -> getToken(*command_line, 2);

      switch(Cn_map[linetoken2])
        {
        case Cno_flag:
          {
            if (ice_on)
              {
                Cno = uiuc_ice_filter(Cno_clean,kCno);
              }
            Cn += Cno;
            break;
          }
        case Cn_beta_flag:
          {
            if (ice_on)
              {
                Cn_beta = uiuc_ice_filter(Cn_beta_clean,kCn_beta);
              }
            Cn += Cn_beta * Beta;
            break;
          }
        case Cn_p_flag:
          {
            if (ice_on)
              {
                Cn_p = uiuc_ice_filter(Cn_p_clean,kCn_p);
              }
            /* Cn_p must be mulitplied by b/2U 
               (see Roskam Control book, Part 1, pg. 147) */
            Cn += Cn_p * P_body * b_2U;
            break;
          }
        case Cn_r_flag:
          {
            if (ice_on)
              {
                Cn_r = uiuc_ice_filter(Cn_r_clean,kCn_r);
              }
            /* Cn_r must be mulitplied by b/2U 
               (see Roskam Control book, Part 1, pg. 147) */
            Cn += Cn_r * R_body * b_2U;
            break;
          }
        case Cn_da_flag:
          {
            if (ice_on)
              {
                Cn_da = uiuc_ice_filter(Cn_da_clean,kCn_da);
              }
            Cn += Cn_da * aileron;
            break;
          }
        case Cn_dr_flag:
          {
            if (ice_on)
              {
                Cn_dr = uiuc_ice_filter(Cn_dr_clean,kCn_dr);
              }
            Cn += Cn_dr * rudder;
            break;
          }
        case Cn_q_flag:
          {
            if (ice_on)
              {
                Cn_q = uiuc_ice_filter(Cn_q_clean,kCn_q);
              }
            Cn += Cn_q * Q_body * cbar_2U;
            break;
          }
        case Cn_b3_flag:
          {
            if (ice_on)
              {
                Cn_b3 = uiuc_ice_filter(Cn_b3_clean,kCn_b3);
              }
            Cn += Cn_b3 * Beta * Beta * Beta;
            break;
          }
        case Cnfada_flag:
          {
            CnfadaI = uiuc_2Dinterpolation(Cnfada_aArray,
                                           Cnfada_daArray,
                                           Cnfada_CnArray,
                                           Cnfada_nAlphaArray,
                                           Cnfada_nda,
                                           Alpha,
                                           aileron);
            Cn += CnfadaI;
            break;
          }
        case Cnfbetadr_flag:
          {
            CnfbetadrI = uiuc_2Dinterpolation(Cnfbetadr_betaArray,
                                              Cnfbetadr_drArray,
                                              Cnfbetadr_CnArray,
                                              Cnfbetadr_nBetaArray,
                                              Cnfbetadr_ndr,
                                              Beta,
                                              rudder);
            Cn += CnfbetadrI;
            break;
          }
        };
    } // end Cn map

  return;
}

// end uiuc_coef_yaw.cpp
