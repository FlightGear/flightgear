/**********************************************************************

 FILENAME:     uiuc_coefficients.cpp

----------------------------------------------------------------------

 DESCRIPTION:  computes aggregated aerodynamic coefficients

----------------------------------------------------------------------

 STATUS:       alpha version

----------------------------------------------------------------------

 REFERENCES:   Roskam, Jan.  Airplane Flight Dynamics and Automatic
               Flight Controls, Part I.  Lawrence, KS: DARcorporation,
               1995.

----------------------------------------------------------------------

 HISTORY:      01/29/2000   initial release
               02/01/2000   (JS) changed map name from aeroData to 
                            aeroPart
               02/18/2000   (JS) added calls to 1Dinterpolation 
                            function from CLfa and CDfa switches
               02/24/2000   added icing model functions
               02/29/2000   (JS) added calls to 2Dinterpolation 
                            function from CLfade, CDfade, Cmfade, 
                            CYfada, CYfbetadr, Clfada, Clfbetadr, 
                            Cnfada, and Cnfbetadr switches

----------------------------------------------------------------------

 AUTHOR(S):    Bipin Sehgal       <bsehgal@uiuc.edu>
               Jeff Scott         <jscott@mail.com>

----------------------------------------------------------------------

 VARIABLES:

----------------------------------------------------------------------

 INPUTS:       -Alpha
               -aileron
               -elevator
               -rudder
               -coefficient components

----------------------------------------------------------------------

 OUTPUTS:      -CL
               -CD
               -Cm
               -CY
               -Cl
               -Cn

----------------------------------------------------------------------

 CALLED BY:    ?

----------------------------------------------------------------------

 CALLS TO:     uiuc_1Dinterpolation
               uiuc_2Dinterpolation
               uiuc_ice

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

#include "uiuc_coefficients.h"


/* set speed at which dynamic pressure terms will be accounted for
   since if velocity is too small, coefficients will go to infinity */

#define ON_VELOCITY 33 /* 20 kts */


void uiuc_coefficients()
{
  string linetoken1;
  string linetoken2;
  stack command_list;
  double cbar_2U = 0, b_2U = 0;
  double slope = 0;
  bool ice_on = false;
  
  // determine if speed is sufficient to compute dynamic pressure
  if (U_body > ON_VELOCITY)
    {
      cbar_2U = cbar / (2.0 * U_body);
      b_2U = bw / (2.0 * U_body);
    }
  else
    {
      cbar_2U = 0.0;
      b_2U = 0.0;
    }

  //check to see if icing model engaged and set flag
  if (Simtime >= iceTime)
    {
      ice_on = true;
    }

  // slowly increase icing severity over period of transientTime
  if (Simtime >= iceTime && Simtime < (iceTime + transientTime))
    {
      slope = eta_final / transientTime;
      eta = slope * (Simtime - iceTime);
    }
  else
    {
      eta = eta_final;
    }

  CL = CD = Cm = CY = Cl = Cn = 0.0;
  command_list = aeroParts -> getCommands();
  
  
  for (LIST command_line = command_list.begin(); command_line!=command_list.end(); ++command_line)
    {
      linetoken1 = aeroParts -> getToken(*command_line, 1); // function parameters gettoken(string,tokenNo);
      linetoken2 = aeroParts -> getToken(*command_line, 2); // 2 represents token No 2

      switch (Keyword_map[linetoken1])
        {
        case CD_flag:
          {
            switch(CD_map[linetoken2])
              {
              case CDo_flag:
                {
                  if (ice_on == true)
                    {
                      CDo = uiuc_ice(CDo_clean,kCDo,eta);
                    }
                  CD += CDo;
                  break;
                }
              case CDK_flag:
                {
                  if (ice_on == true)
                    {
                      CDK = uiuc_ice(CDK_clean,kCDK,eta);
                    }
                  CD += CDK * CL * CL;
                  break;
                }
              case CD_a_flag:
                {
                  if (ice_on == true)
                    {
                      CD_a = uiuc_ice(CD_a_clean,kCD_a,eta);
                    }
                  CD += CD_a * Alpha;
                  break;
                }
              case CD_de_flag:
                {
                  if (ice_on == true)
                    {
                      CD_de = uiuc_ice(CD_de_clean,kCD_de,eta);
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
              };
            break;
          } // end CD map
        case CL_flag:
          {
            switch(CL_map[linetoken2])
              {
              case CLo_flag:
                {
                  if (ice_on == true)
                    {
                      CLo = uiuc_ice(CLo_clean,kCLo,eta);
                    }
                  CL += CLo;
                  break;
                }
              case CL_a_flag:
                {
                  if (ice_on == true)
                    {
                      CL_a = uiuc_ice(CL_a_clean,kCL_a,eta);
                    }
                  CL += CL_a * Alpha;
                  break;
                }
              case CL_adot_flag:
                {
                  if (ice_on == true)
                    {
                      CL_adot = uiuc_ice(CL_adot_clean,kCL_a,eta);
                    }
                  /* CL_adot must be mulitplied by cbar/2U 
                     (see Roskam Control book, Part 1, pg. 147) */
                  CL += CL_adot * Alpha_dot * cbar_2U;
                  break;
                }
              case CL_q_flag:
                {
                  if (ice_on == true)
                    {
                      CL_q = uiuc_ice(CL_q_clean,kCL_q,eta);
                    }
                  /* CL_q must be mulitplied by cbar/2U 
                     (see Roskam Control book, Part 1, pg. 147) */
                  /* why multiply by Theta_dot instead of Q_body?
                     that is what is done in c172_aero.c; assume it 
                     has something to do with axes systems */
                  CL += CL_q * Theta_dot * cbar_2U;
                  break;
                }
              case CL_de_flag:
                {
                  if (ice_on == true)
                    {
                      CL_de = uiuc_ice(CL_de_clean,kCL_de,eta);
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
              };
            break;
          } // end CL map
        case Cm_flag:
          {
            switch(Cm_map[linetoken2])
              {
              case Cmo_flag:
                {
                  if (ice_on == true)
                    {
                      Cmo = uiuc_ice(Cmo_clean,kCmo,eta);
                    }
                  Cm += Cmo;
                  break;
                }
              case Cm_a_flag:
                {
                  if (ice_on == true)
                    {
                      Cm_a = uiuc_ice(Cm_a_clean,kCm_a,eta);
                    }
                  Cm += Cm_a * Alpha;
                  break;
                }
              case Cm_adot_flag:
                {
                  if (ice_on == true)
                    {
                      Cm_adot = uiuc_ice(Cm_adot_clean,kCm_a,eta);
                    }
                  /* Cm_adot must be mulitplied by cbar/2U 
                     (see Roskam Control book, Part 1, pg. 147) */
                  Cm += Cm_adot * Alpha_dot * cbar_2U;
                  break;
                }
              case Cm_q_flag:
                {
                  if (ice_on == true)
                    {
                      Cm_q = uiuc_ice(Cm_q_clean,kCm_q,eta);
                    }
                  /* Cm_q must be mulitplied by cbar/2U 
                     (see Roskam Control book, Part 1, pg. 147) */
                  Cm += Cm_q * Q_body * cbar_2U;
                  break;
                }
              case Cm_de_flag:
                {
                  if (ice_on == true)
                    {
                      Cm_de = uiuc_ice(Cm_de_clean,kCm_de,eta);
                    }
                  Cm += Cm_de * elevator;
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
              };
            break;
          } // end Cm map
        case CY_flag:
          {
            switch(CY_map[linetoken2])
              {
              case CYo_flag:
                {
                  if (ice_on == true)
                    {
                      CYo = uiuc_ice(CYo_clean,kCYo,eta);
                    }
                  CY += CYo;
                  break;
                }
              case CY_beta_flag:
                {
                  if (ice_on == true)
                    {
                      CY_beta = uiuc_ice(CY_beta_clean,kCY_beta,eta);
                    }
                  CY += CY_beta * Beta;
                  break;
                }
              case CY_p_flag:
                {
                  if (ice_on == true)
                    {
                      CY_p = uiuc_ice(CY_p_clean,kCY_p,eta);
                    }
                  /* CY_p must be mulitplied by b/2U 
                     (see Roskam Control book, Part 1, pg. 147) */
                  CY += CY_p * P_body * b_2U;
                  break;
                }
              case CY_r_flag:
                {
                  if (ice_on == true)
                    {
                      CY_r = uiuc_ice(CY_r_clean,kCY_r,eta);
                    }
                  /* CY_r must be mulitplied by b/2U 
                     (see Roskam Control book, Part 1, pg. 147) */
                  CY += CY_r * R_body * b_2U;
                  break;
                }
              case CY_da_flag:
                {
                  if (ice_on == true)
                    {
                      CY_da = uiuc_ice(CY_da_clean,kCY_da,eta);
                    }
                  CY += CY_da * aileron;
                  break;
                }
              case CY_dr_flag:
                {
                  if (ice_on == true)
                    {
                      CY_dr = uiuc_ice(CY_dr_clean,kCY_dr,eta);
                    }
                  CY += CY_dr * rudder;
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
            break;
          } // end CY map
        case Cl_flag:
          {
            switch(Cl_map[linetoken2])
              {
              case Clo_flag:
                {
                  if (ice_on == true)
                    {
                      Clo = uiuc_ice(Clo_clean,kClo,eta);
                    }
                  Cl += Clo;
                  break;
                }
              case Cl_beta_flag:
                {
                  if (ice_on == true)
                    {
                      Cl_beta = uiuc_ice(Cl_beta_clean,kCl_beta,eta);
                    }
                  Cl += Cl_beta * Beta;
                  break;
                }
              case Cl_p_flag:
                {
                  if (ice_on == true)
                    {
                      Cl_p = uiuc_ice(Cl_p_clean,kCl_p,eta);
                    }
                  /* Cl_p must be mulitplied by b/2U 
                     (see Roskam Control book, Part 1, pg. 147) */
                  Cl += Cl_p * P_body * b_2U;
                  break;
                }
              case Cl_r_flag:
                {
                  if (ice_on == true)
                    {
                      Cl_r = uiuc_ice(Cl_r_clean,kCl_r,eta);
                    }
                  /* Cl_r must be mulitplied by b/2U 
                     (see Roskam Control book, Part 1, pg. 147) */
                  Cl += Cl_r * R_body * b_2U;
                  break;
                }
              case Cl_da_flag:
                {
                  if (ice_on == true)
                    {
                      Cl_da = uiuc_ice(Cl_da_clean,kCl_da,eta);
                    }
                  Cl += Cl_da * aileron;
                  break;
                }
              case Cl_dr_flag:
                {
                  if (ice_on == true)
                    {
                      Cl_dr = uiuc_ice(Cl_dr_clean,kCl_dr,eta);
                    }
                  Cl += Cl_dr * rudder;
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
            break;
          } // end Cl map
        case Cn_flag:
          {
            switch(Cn_map[linetoken2])
              {
              case Cno_flag:
                {
                  if (ice_on == true)
                    {
                      Cno = uiuc_ice(Cno_clean,kCno,eta);
                    }
                  Cn += Cno;
                  break;
                }
              case Cn_beta_flag:
                {
                  if (ice_on == true)
                    {
                      Cn_beta = uiuc_ice(Cn_beta_clean,kCn_beta,eta);
                    }
                  Cn += Cn_beta * Beta;
                  break;
                }
              case Cn_p_flag:
                {
                  if (ice_on == true)
                    {
                      Cn_p = uiuc_ice(Cn_p_clean,kCn_p,eta);
                    }
                  /* Cn_p must be mulitplied by b/2U 
                     (see Roskam Control book, Part 1, pg. 147) */
                  Cn += Cn_p * P_body * b_2U;
                  break;
                }
              case Cn_r_flag:
                {
                  if (ice_on == true)
                    {
                      Cn_r = uiuc_ice(Cn_r_clean,kCn_r,eta);
                    }
                  /* Cn_r must be mulitplied by b/2U 
                     (see Roskam Control book, Part 1, pg. 147) */
                  Cn += Cn_r * R_body * b_2U;
                  break;
                }
              case Cn_da_flag:
                {
                  if (ice_on == true)
                    {
                      Cn_da = uiuc_ice(Cn_da_clean,kCn_da,eta);
                    }
                  Cn += Cn_da * aileron;
                  break;
                }
              case Cn_dr_flag:
                {
                  if (ice_on == true)
                    {
                      Cn_dr = uiuc_ice(Cn_dr_clean,kCn_dr,eta);
                    }
                  Cn += Cn_dr * rudder;
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
            break;
          } // end Cn map
        };
    } // end keyword map

  return;
}

// end uiuc_coefficients.cpp
