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
               10/25/2001   (RD) Added new variables needed for the non-
	                    linear Twin Otter model at zero flaps
			    (CYfxxf0)
	       11/12/2001   (RD) Added new variables needed for the non-
	                    linear Twin Otter model with flaps
			    (CYfxxf).  Zero flap vairables removed.
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

#include "uiuc_coef_sideforce.h"


void uiuc_coef_sideforce()
{
  string linetoken1;
  string linetoken2;
  stack command_list;

  double p_nondim;
  double r_nondim;

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
	    CYo_save = CYo;
            CY += CYo_save;
            break;
          }
        case CY_beta_flag:
          {
            if (ice_on)
              {
                CY_beta = uiuc_ice_filter(CY_beta_clean,kCY_beta);
              }
	    CY_beta_save = CY_beta * Std_Beta;
	    if (eta_q_CY_beta_fac)
	      {
		CY += CY_beta_save * eta_q_CY_beta_fac;
	      }
	    else
	      {
		CY += CY_beta_save;
	      }
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
	    CY_p_save = CY_p * P_body * b_2U;
	    if (eta_q_CY_p_fac)
	      {
		CY += CY_p_save * eta_q_CY_p_fac;
	      }
	    else
	      {
		CY += CY_p_save;
	      }
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
	    CY_r_save = CY_r * R_body * b_2U;
	    if (eta_q_CY_r_fac)
	      {
		CY += CY_r_save * eta_q_CY_r_fac;
	      }
	    else
	      {
		CY += CY_r_save;
	      }
            break;
          }
        case CY_da_flag:
          {
            if (ice_on)
              {
                CY_da = uiuc_ice_filter(CY_da_clean,kCY_da);
              }
	    CY_da_save = CY_da * aileron;
            CY += CY_da_save;
            break;
          }
        case CY_dr_flag:
          {
            if (ice_on)
              {
                CY_dr = uiuc_ice_filter(CY_dr_clean,kCY_dr);
              }
	    CY_dr_save = CY_dr * rudder;
	    if (eta_q_CY_dr_fac)
	      {
		CY += CY_dr_save * eta_q_CY_dr_fac;
	      }
	    else
	      {
		CY += CY_dr_save;
	      }
            break;
          }
        case CY_dra_flag:
          {
            if (ice_on)
              {
                CY_dra = uiuc_ice_filter(CY_dra_clean,kCY_dra);
              }
	    CY_dra_save = CY_dra * rudder * Std_Alpha;
            CY += CY_dra_save;
            break;
          }
        case CY_bdot_flag:
          {
            if (ice_on)
              {
                CY_bdot = uiuc_ice_filter(CY_bdot_clean,kCY_bdot);
              }
	    CY_bdot_save = CY_bdot * Std_Beta_dot * b_2U;
            CY += CY_bdot_save;
            break;
          }
        case CYfada_flag:
          {
            CYfadaI = uiuc_2Dinterpolation(CYfada_aArray,
                                           CYfada_daArray,
                                           CYfada_CYArray,
                                           CYfada_nAlphaArray,
                                           CYfada_nda,
                                           Std_Alpha,
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
                                              Std_Beta,
                                              rudder);
            CY += CYfbetadrI;
            break;
          }
        case CYfabetaf_flag:
          {
	    if (CYfabetaf_nice == 1)
	      CYfabetafI = uiuc_3Dinterp_quick(CYfabetaf_fArray,
					       CYfabetaf_aArray_nice,
					       CYfabetaf_bArray_nice,
					       CYfabetaf_CYArray,
					       CYfabetaf_na_nice,
					       CYfabetaf_nb_nice,
					       CYfabetaf_nf,
					       flap_pos,
					       Std_Alpha,
					       Std_Beta);
	    else
	      CYfabetafI = uiuc_3Dinterpolation(CYfabetaf_fArray,
						CYfabetaf_aArray,
						CYfabetaf_betaArray,
						CYfabetaf_CYArray,
						CYfabetaf_nAlphaArray,
						CYfabetaf_nbeta,
						CYfabetaf_nf,
						flap_pos,
						Std_Alpha,
						Std_Beta);
            CY += CYfabetafI;
            break;
          }
        case CYfadaf_flag:
          {
	    if (CYfadaf_nice == 1)
	      CYfadafI = uiuc_3Dinterp_quick(CYfadaf_fArray,
					     CYfadaf_aArray_nice,
					     CYfadaf_daArray_nice,
					     CYfadaf_CYArray,
					     CYfadaf_na_nice,
					     CYfadaf_nda_nice,
					     CYfadaf_nf,
					     flap_pos,
					     Std_Alpha,
					     aileron);
	    else
	      CYfadafI = uiuc_3Dinterpolation(CYfadaf_fArray,
					      CYfadaf_aArray,
					      CYfadaf_daArray,
					      CYfadaf_CYArray,
					      CYfadaf_nAlphaArray,
					      CYfadaf_nda,
					      CYfadaf_nf,
					      flap_pos,
					      Std_Alpha,
					      aileron);
            CY += CYfadafI;
            break;
          }
        case CYfadrf_flag:
          {
	    if (CYfadrf_nice == 1)
	      CYfadrfI = uiuc_3Dinterp_quick(CYfadrf_fArray,
					     CYfadrf_aArray_nice,
					     CYfadrf_drArray_nice,
					     CYfadrf_CYArray,
					     CYfadrf_na_nice,
					     CYfadrf_ndr_nice,
					     CYfadrf_nf,
					     flap_pos,
					     Std_Alpha,
					     rudder);
	    else
	      CYfadrfI = uiuc_3Dinterpolation(CYfadrf_fArray,
					      CYfadrf_aArray,
					      CYfadrf_drArray,
					      CYfadrf_CYArray,
					      CYfadrf_nAlphaArray,
					      CYfadrf_ndr,
					      CYfadrf_nf,
					      flap_pos,
					      Std_Alpha,
					      rudder);
            CY += CYfadrfI;
            break;
	  }
        case CYfapf_flag:
	  {
	    p_nondim = P_body * b_2U;
	    if (CYfapf_nice == 1)
	      CYfapfI = uiuc_3Dinterp_quick(CYfapf_fArray,
					    CYfapf_aArray_nice,
					    CYfapf_pArray_nice,
					    CYfapf_CYArray,
					    CYfapf_na_nice,
					    CYfapf_np_nice,
					    CYfapf_nf,
					    flap_pos,
					    Std_Alpha,
					    p_nondim);
	    else
	      CYfapfI = uiuc_3Dinterpolation(CYfapf_fArray,
					     CYfapf_aArray,
					     CYfapf_pArray,
					     CYfapf_CYArray,
					     CYfapf_nAlphaArray,
					     CYfapf_np,
					     CYfapf_nf,
					     flap_pos,
					     Std_Alpha,
					     p_nondim);
            CY += CYfapfI;
            break;
          }
	case CYfarf_flag:
          {
	    r_nondim = R_body * b_2U;
	    if (CYfarf_nice == 1)
	      CYfarfI = uiuc_3Dinterp_quick(CYfarf_fArray,
					    CYfarf_aArray_nice,
					    CYfarf_rArray_nice,
					    CYfarf_CYArray,
					    CYfarf_na_nice,
					    CYfarf_nr_nice,
					    CYfarf_nf,
					    flap_pos,
					    Std_Alpha,
					    r_nondim);
	    else
	      CYfarfI = uiuc_3Dinterpolation(CYfarf_fArray,
					     CYfarf_aArray,
					     CYfarf_rArray,
					     CYfarf_CYArray,
					     CYfarf_nAlphaArray,
					     CYfarf_nr,
					     CYfarf_nf,
					     flap_pos,
					     Std_Alpha,
					     r_nondim);
            CY += CYfarfI;
            break;
          }
       };
    } // end CY map
  
  return;
}

// end uiuc_coef_sideforce.cpp
