/**********************************************************************

 FILENAME:     uiuc_coef_roll.cpp

----------------------------------------------------------------------

 DESCRIPTION:  computes aggregated aerodynamic roll coefficient

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
               -roll coefficient components
               -icing parameters
               -b_2U multiplier

----------------------------------------------------------------------

 OUTPUTS:      -Cl

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

#include "uiuc_coef_roll.h"


void uiuc_coef_roll()
{
  string linetoken1;
  string linetoken2;
  stack command_list;
  
  command_list = aeroRollParts -> getCommands();
  
  for (LIST command_line = command_list.begin(); command_line!=command_list.end(); ++command_line)
    {
      linetoken1 = aeroRollParts -> getToken(*command_line, 1);
      linetoken2 = aeroRollParts -> getToken(*command_line, 2);

      switch(Cl_map[linetoken2])
        {
        case Clo_flag:
          {
            if (ice_on)
              {
                Clo = uiuc_ice_filter(Clo_clean,kClo);
              }
            Cl += Clo;
            break;
          }
        case Cl_beta_flag:
          {
            if (ice_on)
              {
                Cl_beta = uiuc_ice_filter(Cl_beta_clean,kCl_beta);
              }
            Cl += Cl_beta * Beta;
            break;
          }
        case Cl_p_flag:
          {
            if (ice_on)
              {
                Cl_p = uiuc_ice_filter(Cl_p_clean,kCl_p);
              }
            /* Cl_p must be mulitplied by b/2U 
               (see Roskam Control book, Part 1, pg. 147) */
            Cl += Cl_p * P_body * b_2U;
            break;
          }
        case Cl_r_flag:
          {
            if (ice_on)
              {
                Cl_r = uiuc_ice_filter(Cl_r_clean,kCl_r);
              }
            /* Cl_r must be mulitplied by b/2U 
               (see Roskam Control book, Part 1, pg. 147) */
            Cl += Cl_r * R_body * b_2U;
            break;
          }
        case Cl_da_flag:
          {
            if (ice_on)
              {
                Cl_da = uiuc_ice_filter(Cl_da_clean,kCl_da);
              }
            Cl += Cl_da * aileron;
            break;
          }
        case Cl_dr_flag:
          {
            if (ice_on)
              {
                Cl_dr = uiuc_ice_filter(Cl_dr_clean,kCl_dr);
              }
            Cl += Cl_dr * rudder;
            break;
          }
        case Cl_daa_flag:
          {
            if (ice_on)
              {
                Cl_daa = uiuc_ice_filter(Cl_daa_clean,kCl_daa);
              }
            Cl += Cl_daa * aileron * Alpha;
            break;
          }
        case Clfada_flag:
          {
            ClfadaI = uiuc_2Dinterpolation(Clfada_aArray,
                                           Clfada_daArray,
                                           Clfada_ClArray,
                                           Clfada_nAlphaArray,
                                           Clfada_nda,
                                           Alpha,
                                           aileron);
            Cl += ClfadaI;
            break;
          }
        case Clfbetadr_flag:
          {
            ClfbetadrI = uiuc_2Dinterpolation(Clfbetadr_betaArray,
                                              Clfbetadr_drArray,
                                              Clfbetadr_ClArray,
                                              Clfbetadr_nBetaArray,
                                              Clfbetadr_ndr,
                                              Beta,
                                              rudder);
            Cl += ClfbetadrI;
            break;
          }
        };
    } // end Cl map

  return;
}

// end uiuc_coef_roll.cpp
