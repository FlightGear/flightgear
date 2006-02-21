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
               10/25/2001   (RD) Added new variables needed for the non-
	                    linear Twin Otter model at zero flaps
			    (Cnfxxf0)
	       11/12/2001   (RD) Added new variables needed for the non-
	                    linear Twin Otter model with flaps
			    (Cnfxxf).  Zero flap vairables removed.
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

#include "uiuc_coef_yaw.h"


void uiuc_coef_yaw()
{
  string linetoken1;
  string linetoken2;
  stack command_list;

  double p_nondim;
  double r_nondim;

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
	    Cno_save = Cno;
            Cn += Cno_save;
            break;
          }
        case Cn_beta_flag:
          {
            if (ice_on)
              {
                Cn_beta = uiuc_ice_filter(Cn_beta_clean,kCn_beta);
              }
	    Cn_beta_save = Cn_beta * Std_Beta;
	    if (eta_q_Cn_beta_fac)
	      {
		Cn += Cn_beta_save * eta_q_Cn_beta_fac;
	      }
	    else
	      {
		Cn += Cn_beta_save;
	      }
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
	    Cn_p_save = Cn_p * P_body * b_2U;
	    if (eta_q_Cn_p_fac)
	      {
		Cn += Cn_p_save * eta_q_Cn_p_fac;
	      }
	    else
	      {
		Cn += Cn_p_save;
	      }
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
	    Cn_r_save = Cn_r * R_body * b_2U;
	    if (eta_q_Cn_r_fac)
	      {
		Cn += Cn_r_save * eta_q_Cn_r_fac;
	      }
	    else
	      {
		Cn += Cn_r_save;
	      }
            break;
          }
        case Cn_da_flag:
          {
            if (ice_on)
              {
                Cn_da = uiuc_ice_filter(Cn_da_clean,kCn_da);
              }
	    Cn_da_save = Cn_da * aileron;
            Cn += Cn_da_save;
            break;
          }
        case Cn_dr_flag:
          {
            if (ice_on)
              {
                Cn_dr = uiuc_ice_filter(Cn_dr_clean,kCn_dr);
              }
	    Cn_dr_save = Cn_dr * rudder;
	    if (eta_q_Cn_dr_fac)
	      {
		Cn += Cn_dr_save * eta_q_Cn_dr_fac;
	      }
	    else
	      {
		Cn += Cn_dr_save;
	      }
            break;
          }
        case Cn_q_flag:
          {
            if (ice_on)
              {
                Cn_q = uiuc_ice_filter(Cn_q_clean,kCn_q);
              }
	    Cn_q_save = Cn_q * Q_body * cbar_2U;
            Cn += Cn_q_save;
            break;
          }
        case Cn_b3_flag:
          {
            if (ice_on)
              {
                Cn_b3 = uiuc_ice_filter(Cn_b3_clean,kCn_b3);
              }
	    Cn_b3_save = Cn_b3 * Std_Beta * Std_Beta * Std_Beta;
            Cn += Cn_b3_save;
            break;
          }
        case Cnfada_flag:
          {
            CnfadaI = uiuc_2Dinterpolation(Cnfada_aArray,
                                           Cnfada_daArray,
                                           Cnfada_CnArray,
                                           Cnfada_nAlphaArray,
                                           Cnfada_nda,
                                           Std_Alpha,
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
                                              Std_Beta,
                                              rudder);
            Cn += CnfbetadrI;
            break;
          }
        case Cnfabetaf_flag:
          {
	    if (Cnfabetaf_nice == 1)
	      CnfabetafI = uiuc_3Dinterp_quick(Cnfabetaf_fArray,
					       Cnfabetaf_aArray_nice,
					       Cnfabetaf_bArray_nice,
					       Cnfabetaf_CnArray,
					       Cnfabetaf_na_nice,
					       Cnfabetaf_nb_nice,
					       Cnfabetaf_nf,
					       flap_pos,
					       Std_Alpha,
					       Std_Beta);
	    else
	      CnfabetafI = uiuc_3Dinterpolation(Cnfabetaf_fArray,
						Cnfabetaf_aArray,
						Cnfabetaf_betaArray,
						Cnfabetaf_CnArray,
						Cnfabetaf_nAlphaArray,
						Cnfabetaf_nbeta,
						Cnfabetaf_nf,
						flap_pos,
						Std_Alpha,
						Std_Beta);
            Cn += CnfabetafI;
            break;
          }
        case Cnfadaf_flag:
          {
	    if (Cnfadaf_nice == 1)
	      CnfadafI = uiuc_3Dinterp_quick(Cnfadaf_fArray,
					     Cnfadaf_aArray_nice,
					     Cnfadaf_daArray_nice,
					     Cnfadaf_CnArray,
					     Cnfadaf_na_nice,
					     Cnfadaf_nda_nice,
					     Cnfadaf_nf,
					     flap_pos,
					     Std_Alpha,
					     aileron);
	    else
	      CnfadafI = uiuc_3Dinterpolation(Cnfadaf_fArray,
					      Cnfadaf_aArray,
					      Cnfadaf_daArray,
					      Cnfadaf_CnArray,
					      Cnfadaf_nAlphaArray,
					      Cnfadaf_nda,
					      Cnfadaf_nf,
					      flap_pos,
					      Std_Alpha,
					      aileron);
            Cn += CnfadafI;
            break;
          }
        case Cnfadrf_flag:
          {
	    if (Cnfadrf_nice == 1)
	      CnfadrfI = uiuc_3Dinterp_quick(Cnfadrf_fArray,
					     Cnfadrf_aArray_nice,
					     Cnfadrf_drArray_nice,
					     Cnfadrf_CnArray,
					     Cnfadrf_na_nice,
					     Cnfadrf_ndr_nice,
					     Cnfadrf_nf,
					     flap_pos,
					     Std_Alpha,
					     rudder);
	    else
	      CnfadrfI = uiuc_3Dinterpolation(Cnfadrf_fArray,
					      Cnfadrf_aArray,
					      Cnfadrf_drArray,
					      Cnfadrf_CnArray,
					      Cnfadrf_nAlphaArray,
					      Cnfadrf_ndr,
					      Cnfadrf_nf,
					      flap_pos,
					      Std_Alpha,
					      rudder);
            Cn += CnfadrfI;
            break;
          }
        case Cnfapf_flag:
          {
	    p_nondim = P_body * b_2U;
	    if (Cnfapf_nice == 1)
	      CnfapfI = uiuc_3Dinterp_quick(Cnfapf_fArray,
					    Cnfapf_aArray_nice,
					    Cnfapf_pArray_nice,
					    Cnfapf_CnArray,
					    Cnfapf_na_nice,
					    Cnfapf_np_nice,
					    Cnfapf_nf,
					    flap_pos,
					    Std_Alpha,
					    p_nondim);
	    else
	      CnfapfI = uiuc_3Dinterpolation(Cnfapf_fArray,
					     Cnfapf_aArray,
					     Cnfapf_pArray,
					     Cnfapf_CnArray,
					     Cnfapf_nAlphaArray,
					     Cnfapf_np,
					     Cnfapf_nf,
					     flap_pos,
					     Std_Alpha,
					     p_nondim);
            Cn += CnfapfI;
            break;
          }
        case Cnfarf_flag:
          {
	    r_nondim = R_body * b_2U;
	    if (Cnfarf_nice == 1)
	      CnfarfI = uiuc_3Dinterp_quick(Cnfarf_fArray,
					    Cnfarf_aArray_nice,
					    Cnfarf_rArray_nice,
					    Cnfarf_CnArray,
					    Cnfarf_na_nice,
					    Cnfarf_nr_nice,
					    Cnfarf_nf,
					    flap_pos,
					    Std_Alpha,
					    r_nondim);
	    else
	      CnfarfI = uiuc_3Dinterpolation(Cnfarf_fArray,
					     Cnfarf_aArray,
					     Cnfarf_rArray,
					     Cnfarf_CnArray,
					     Cnfarf_nAlphaArray,
					     Cnfarf_nr,
					     Cnfarf_nf,
					     flap_pos,
					     Std_Alpha,
					     r_nondim);
            Cn += CnfarfI;
            break;
          }
        };
    } // end Cn map

  return;
}

// end uiuc_coef_yaw.cpp
