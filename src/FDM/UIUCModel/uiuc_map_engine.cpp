/********************************************************************** 
 
 FILENAME:     uiuc_map_engine.cpp 

---------------------------------------------------------------------- 

 DESCRIPTION:  initializes the engine map

----------------------------------------------------------------------
 
 STATUS:       alpha version

----------------------------------------------------------------------
 
 REFERENCES:   
 
----------------------------------------------------------------------
 
 HISTORY:      04/08/2000   initial release
               06/18/2001   (RD) Added Throttle_pct_input
               08/29/2002   (MS) Added simpleSingleModel

----------------------------------------------------------------------
 
 AUTHOR(S):    Bipin Sehgal       <bsehgal@uiuc.edu>
               Jeff Scott         <jscott@mail.com>
	       Robert Deters      <rdeters@uiuc.edu>
 
----------------------------------------------------------------------
 
 VARIABLES:
 
----------------------------------------------------------------------
 
 INPUTS:       none
 
----------------------------------------------------------------------
 
 OUTPUTS:      none
 
----------------------------------------------------------------------
 
 CALLED BY:    uiuc_initializemaps.cpp
 
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

#include "uiuc_map_engine.h"


void uiuc_map_engine()
{
  engine_map["simpleSingle"]      =      simpleSingle_flag         ;
  engine_map["simpleSingleModel"] =      simpleSingleModel_flag    ;
  engine_map["c172"]              =      c172_flag                 ;
  engine_map["cherokee"]          =      cherokee_flag             ;
  engine_map["Throttle_pct_input"]=      Throttle_pct_input_flag   ;
  engine_map["gyroForce_Q_body"]  =      gyroForce_Q_body_flag     ;
  engine_map["gyroForce_R_body"]  =      gyroForce_R_body_flag     ;
  engine_map["omega"]             =      omega_flag                ;
  engine_map["omegaRPM"]          =      omegaRPM_flag             ;
  engine_map["polarInertia"]      =      polarInertia_flag         ;
  engine_map["slipstream_effects"] = slipstream_effects_flag       ;
  engine_map["propDia"]            = propDia_flag                  ;
  engine_map["eta_q_Cm_q"]         = eta_q_Cm_q_flag               ;
  engine_map["eta_q_Cm_adot"]      = eta_q_Cm_adot_flag            ;
  engine_map["eta_q_Cmfade"]       = eta_q_Cmfade_flag             ;
  engine_map["eta_q_Cm_de"]        = eta_q_Cm_de_flag              ;
  engine_map["eta_q_Cl_beta"]      = eta_q_Cl_beta_flag            ;
  engine_map["eta_q_Cl_p"]         = eta_q_Cl_p_flag               ;
  engine_map["eta_q_Cl_r"]         = eta_q_Cl_r_flag               ;
  engine_map["eta_q_Cl_dr"]        = eta_q_Cl_dr_flag              ;
  engine_map["eta_q_CY_beta"]      = eta_q_CY_beta_flag            ;
  engine_map["eta_q_CY_p"]         = eta_q_CY_p_flag               ;
  engine_map["eta_q_CY_r"]         = eta_q_CY_r_flag               ;
  engine_map["eta_q_CY_dr"]        = eta_q_CY_dr_flag              ;
  engine_map["eta_q_Cn_beta"]      = eta_q_Cn_beta_flag            ;
  engine_map["eta_q_Cn_p"]         = eta_q_Cn_p_flag               ;
  engine_map["eta_q_Cn_r"]         = eta_q_Cn_r_flag               ;
  engine_map["eta_q_Cn_dr"]        = eta_q_Cn_dr_flag              ;
  engine_map["forcemom"]           =      forcemom_flag            ;
  engine_map["Xp_input"]           =      Xp_input_flag            ;
  engine_map["Zp_input"]           =      Zp_input_flag            ;
  engine_map["Mp_input"]           =      Mp_input_flag            ;
}

// end uiuc_map_engine.cpp
