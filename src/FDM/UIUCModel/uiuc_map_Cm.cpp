/********************************************************************** 
 
 FILENAME:     uiuc_map_Cm.cpp 

---------------------------------------------------------------------- 

 DESCRIPTION:  initializes the Cm map

----------------------------------------------------------------------
 
 STATUS:       alpha version

----------------------------------------------------------------------
 
 REFERENCES:   
 
----------------------------------------------------------------------
 
 HISTORY:      04/08/2000   initial release
               10/25/2001   (RD) Added new variables needed for the non-
	                    linear Twin Otter model at zero flaps
			    (Cmfxxf0)
	       11/12/2001   (RD) Added new variables needed for the non-
	                    linear Twin Otter model with flaps
			    (Cmfxxf).  Zero flap vairables removed.

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

#include "uiuc_map_Cm.h"


void uiuc_map_Cm()
{
  Cm_map["Cmo"]                   =      Cmo_flag                   ;
  Cm_map["Cm_a"]                  =      Cm_a_flag                  ;
  Cm_map["Cm_a2"]                 =      Cm_a2_flag                 ;
  Cm_map["Cm_adot"]               =      Cm_adot_flag               ;
  Cm_map["Cm_q"]                  =      Cm_q_flag                  ;
  Cm_map["Cm_ih"]                 =      Cm_ih_flag                 ;
  Cm_map["Cm_de"]                 =      Cm_de_flag                 ;
  Cm_map["Cm_b2"]                 =      Cm_b2_flag                 ;
  Cm_map["Cm_r"]                  =      Cm_r_flag                  ;
  Cm_map["Cm_df"]                 =      Cm_df_flag                 ;
  Cm_map["Cm_ds"]                 =      Cm_ds_flag                 ;
  Cm_map["Cm_dg"]                 =      Cm_dg_flag                 ;
  Cm_map["Cmfa"]                  =      Cmfa_flag                  ;
  Cm_map["Cmfade"]                =      Cmfade_flag                ;
  Cm_map["Cmfdf"]                 =      Cmfdf_flag                 ;
  Cm_map["Cmfadf"]                =      Cmfadf_flag                ;
  Cm_map["Cmfabetaf"]             =      Cmfabetaf_flag             ;
  Cm_map["Cmfadef"]               =      Cmfadef_flag               ;
  Cm_map["Cmfaqf"]                =      Cmfaqf_flag                ;
}

// end uiuc_map_Cm.cpp
