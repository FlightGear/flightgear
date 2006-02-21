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
               10/25/2001   (RD) Added new variables needed for the non-
	                    linear Twin Otter model at zero flaps
			    (Clfxxf0)
	       11/12/2001   (RD) Added new variables needed for the non-
	                    linear Twin Otter model with flaps
			    (Clfxxf).  Zero flap vairables removed.
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

#include "uiuc_coef_roll.h"


void uiuc_coef_roll()
{
  string linetoken1;
  string linetoken2;
  stack command_list;

  double p_nondim;
  double r_nondim;

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
	    Clo_save = Clo;
            Cl += Clo_save;
            break;
          }
        case Cl_beta_flag:
          {
            if (ice_on)
              {
                Cl_beta = uiuc_ice_filter(Cl_beta_clean,kCl_beta);
              }
	    Cl_beta_save = Cl_beta * Std_Beta;
	    if (eta_q_Cl_beta_fac)
	      {
		Cl += Cl_beta_save * eta_q_Cl_beta_fac;
	      }
	    else
	      {
		Cl += Cl_beta_save;
	      }
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
	    Cl_p_save = Cl_p * P_body * b_2U;
//  	    if (Cl_p_save > 0.1) {
//  	      Cl_p_save = 0.1;
//  	    }
//  	    if (Cl_p_save < -0.1) {
//  	      Cl_p_save = -0.1;
//  	    }
	    if (eta_q_Cl_p_fac)
	      {
		Cl += Cl_p_save * eta_q_Cl_p_fac;
	      }
	    else
	      {
		Cl += Cl_p_save;
	      }
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
	    Cl_r_save = Cl_r * R_body * b_2U;
	    if (eta_q_Cl_r_fac)
	      {
		Cl += Cl_r_save * eta_q_Cl_r_fac;
	      }
	    else
	      {
		Cl += Cl_r_save;
	      }
            break;
          }
        case Cl_da_flag:
          {
            if (ice_on)
              {
                Cl_da = uiuc_ice_filter(Cl_da_clean,kCl_da);
              }
	    Cl_da_save = Cl_da * aileron;
            Cl += Cl_da_save;
            break;
          }
        case Cl_dr_flag:
          {
            if (ice_on)
              {
                Cl_dr = uiuc_ice_filter(Cl_dr_clean,kCl_dr);
              }
	    Cl_dr_save = Cl_dr * rudder;
	    if (eta_q_Cl_dr_fac)
	      {
		Cl += Cl_dr_save * eta_q_Cl_dr_fac;
	      }
	    else
	      {
		Cl += Cl_dr_save;
	      }
            break;
          }
        case Cl_daa_flag:
          {
            if (ice_on)
              {
                Cl_daa = uiuc_ice_filter(Cl_daa_clean,kCl_daa);
              }
	    Cl_daa_save = Cl_daa * aileron * Std_Alpha;
            Cl += Cl_daa_save;
            break;
          }
        case Clfada_flag:
          {
            ClfadaI = uiuc_2Dinterpolation(Clfada_aArray,
                                           Clfada_daArray,
                                           Clfada_ClArray,
                                           Clfada_nAlphaArray,
                                           Clfada_nda,
                                           Std_Alpha,
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
                                              Std_Beta,
                                              rudder);
            Cl += ClfbetadrI;
            break;
          }
        case Clfabetaf_flag:
          {
	    if (Clfabetaf_nice == 1)
	      ClfabetafI = uiuc_3Dinterp_quick(Clfabetaf_fArray,
					       Clfabetaf_aArray_nice,
					       Clfabetaf_bArray_nice,
					       Clfabetaf_ClArray,
					       Clfabetaf_na_nice,
					       Clfabetaf_nb_nice,
					       Clfabetaf_nf,
					       flap_pos,
					       Std_Alpha,
					       Std_Beta);
	    else
	      ClfabetafI = uiuc_3Dinterpolation(Clfabetaf_fArray,
						Clfabetaf_aArray,
						Clfabetaf_betaArray,
						Clfabetaf_ClArray,
						Clfabetaf_nAlphaArray,
						Clfabetaf_nbeta,
						Clfabetaf_nf,
						flap_pos,
						Std_Alpha,
						Std_Beta);
            Cl += ClfabetafI;
            break;
          }
        case Clfadaf_flag:
          {
	    if (Clfadaf_nice == 1)
	      ClfadafI = uiuc_3Dinterp_quick(Clfadaf_fArray,
					     Clfadaf_aArray_nice,
					     Clfadaf_daArray_nice,
					     Clfadaf_ClArray,
					     Clfadaf_na_nice,
					     Clfadaf_nda_nice,
					     Clfadaf_nf,
					     flap_pos,
					     Std_Alpha,
					     aileron);
	    else
	      ClfadafI = uiuc_3Dinterpolation(Clfadaf_fArray,
					      Clfadaf_aArray,
					      Clfadaf_daArray,
					      Clfadaf_ClArray,
					      Clfadaf_nAlphaArray,
					      Clfadaf_nda,
					      Clfadaf_nf,
					      flap_pos,
					      Std_Alpha,
					      aileron);
            Cl += ClfadafI;
            break;
          }
        case Clfadrf_flag:
          {
	    if (Clfadrf_nice == 1)
	      ClfadrfI = uiuc_3Dinterp_quick(Clfadrf_fArray,
					     Clfadrf_aArray_nice,
					     Clfadrf_drArray_nice,
					     Clfadrf_ClArray,
					     Clfadrf_na_nice,
					     Clfadrf_ndr_nice,
					     Clfadrf_nf,
					     flap_pos,
					     Std_Alpha,
					     rudder);
	    else
	      ClfadrfI = uiuc_3Dinterpolation(Clfadrf_fArray,
					      Clfadrf_aArray,
					      Clfadrf_drArray,
					      Clfadrf_ClArray,
					      Clfadrf_nAlphaArray,
					      Clfadrf_ndr,
					      Clfadrf_nf,
					      flap_pos,
					      Std_Alpha,
					      rudder);
            Cl += ClfadrfI;
            break;
          }
	case Clfapf_flag:
          {
	    p_nondim = P_body * b_2U;
	    if (Clfapf_nice == 1)
	      ClfapfI = uiuc_3Dinterp_quick(Clfapf_fArray,
					    Clfapf_aArray_nice,
					    Clfapf_pArray_nice,
					    Clfapf_ClArray,
					    Clfapf_na_nice,
					    Clfapf_np_nice,
					    Clfapf_nf,
					    flap_pos,
					    Std_Alpha,
					    p_nondim);
	    else
	      ClfapfI = uiuc_3Dinterpolation(Clfapf_fArray,
					     Clfapf_aArray,
					     Clfapf_pArray,
					     Clfapf_ClArray,
					     Clfapf_nAlphaArray,
					     Clfapf_np,
					     Clfapf_nf,
					     flap_pos,
					     Std_Alpha,
					     p_nondim);
            Cl += ClfapfI;
            break;
          }
	case Clfarf_flag:
          {
	    r_nondim = R_body * b_2U;
	    if (Clfarf_nice == 1)
	      ClfarfI = uiuc_3Dinterp_quick(Clfarf_fArray,
					    Clfarf_aArray_nice,
					    Clfarf_rArray_nice,
					    Clfarf_ClArray,
					    Clfarf_na_nice,
					    Clfarf_nr_nice,
					    Clfarf_nf,
					    flap_pos,
					    Std_Alpha,
					    r_nondim);
	    else
	      ClfarfI = uiuc_3Dinterpolation(Clfarf_fArray,
					     Clfarf_aArray,
					     Clfarf_rArray,
					     Clfarf_ClArray,
					     Clfarf_nAlphaArray,
					     Clfarf_nr,
					     Clfarf_nf,
					     flap_pos,
					     Std_Alpha,
					     r_nondim);
            Cl += ClfarfI;
            break;
          }
        };
    } // end Cl map

  return;
}

// end uiuc_coef_roll.cpp
