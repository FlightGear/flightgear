/********************************************************************** 
 
 FILENAME:     uiuc_betaprobe.cpp 

---------------------------------------------------------------------- 

 DESCRIPTION:  Computes flow angle, beta, for use in ice detection 
               scheme

----------------------------------------------------------------------
 
 STATUS:       alpha version

----------------------------------------------------------------------
 
 REFERENCES:   
 
----------------------------------------------------------------------

 HISTORY:      05/15/2000   initial release
 
----------------------------------------------------------------------
 
 AUTHOR(S):    Jeff Scott         <jscott@mail.com>

----------------------------------------------------------------------

 VARIABLES:

----------------------------------------------------------------------

 INPUTS:       CLclean_wing
               CLiced_wing
               CLclean_tail
               CLiced_tail

----------------------------------------------------------------------

 OUTPUTS:      Dbeta_flow_wing
               Dbeta_flow_tail

----------------------------------------------------------------------
 
 CALLED BY:    uiuc_wrapper
 
----------------------------------------------------------------------
 
 CALLS TO:     none
 
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

#include "uiuc_betaprobe.h"


void uiuc_betaprobe()
{
  if (CX && CZ)
    {
      CLclean_wing = CXclean_wing * sin(Std_Alpha) - CZclean_wing * cos(Std_Alpha);
      CLiced_wing  = CXiced_wing * sin(Std_Alpha) - CZiced_wing * cos(Std_Alpha);
      CLclean_tail = CXclean_tail * sin(Std_Alpha) - CZclean_tail * cos(Std_Alpha);
      CLiced_tail  = CXiced_tail * sin(Std_Alpha) - CZiced_tail * cos(Std_Alpha);
    }

  /* calculate lift per unit span*/
  Lift_clean_wing = CLclean_wing * Dynamic_pressure * Sw / bw;
  Lift_iced_wing = CLiced_wing * Dynamic_pressure * Sw / bw;
  Lift_clean_tail = CLclean_tail * Dynamic_pressure * Sh / bh;
  Lift_iced_tail = CLiced_tail * Dynamic_pressure * Sh / bh;

  Gamma_clean_wing = Lift_clean_wing / (Density * V_rel_wind);
  Gamma_iced_wing = Lift_iced_wing / (Density * V_rel_wind);
  Gamma_clean_tail = Lift_clean_tail / (Density * V_rel_wind);
  Gamma_iced_tail = Lift_iced_tail / (Density * V_rel_wind);

  w_clean_wing = Gamma_clean_wing / (2 * LS_PI * x_probe_wing);
  w_iced_wing = Gamma_iced_wing / (2 * LS_PI * x_probe_wing);
  w_clean_tail = Gamma_clean_tail / (2 * LS_PI * x_probe_tail);
  w_iced_tail = Gamma_iced_tail / (2 * LS_PI * x_probe_tail);

  V_total_clean_wing = sqrt(w_clean_wing*w_clean_wing + 
			    V_rel_wind*V_rel_wind - 
			    2*w_clean_wing*V_rel_wind * 
			    cos(LS_PI/2 + Std_Alpha));
  V_total_iced_wing = sqrt(w_iced_wing*w_iced_wing + 
			   V_rel_wind*V_rel_wind - 
			   2*w_iced_wing*V_rel_wind * 
			   cos(LS_PI/2 + Std_Alpha));
  V_total_clean_tail = sqrt(w_clean_tail*w_clean_tail + 
			    V_rel_wind*V_rel_wind - 
			    2*w_clean_tail*V_rel_wind * 
			    cos(LS_PI/2 + Std_Alpha));
  V_total_iced_tail = sqrt(w_iced_tail*w_iced_tail + 
			   V_rel_wind*V_rel_wind - 
			   2*w_iced_tail*V_rel_wind * 
			   cos(LS_PI/2 + Std_Alpha));

  beta_flow_clean_wing = asin((w_clean_wing / V_total_clean_wing) * 
			      sin (LS_PI/2 + Std_Alpha));
  beta_flow_iced_wing = asin((w_iced_wing / V_total_iced_wing) * 
			     sin (LS_PI/2 + Std_Alpha));
  beta_flow_clean_tail = asin((w_clean_tail / V_total_clean_tail) * 
			      sin (LS_PI/2 + Std_Alpha));
  beta_flow_iced_tail = asin((w_iced_tail / V_total_iced_tail) * 
			     sin (LS_PI/2 + Std_Alpha));

  Dbeta_flow_wing = fabs(beta_flow_clean_wing - beta_flow_iced_wing);
  Dbeta_flow_tail = fabs(beta_flow_clean_tail - beta_flow_iced_tail);

  pct_beta_flow_wing = beta_flow_iced_wing / beta_flow_clean_wing;
  pct_beta_flow_tail = beta_flow_iced_tail / beta_flow_clean_tail;
}

//end uiuc_betaprobe.cpp
