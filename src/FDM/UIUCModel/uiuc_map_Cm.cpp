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

----------------------------------------------------------------------
 
 AUTHOR(S):    Bipin Sehgal       <bsehgal@uiuc.edu>
               Jeff Scott         <jscott@mail.com>
 
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
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
 USA or view http://www.gnu.org/copyleft/gpl.html.
 
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
  Cm_map["Cmfa"]                  =      Cmfa_flag                  ;
  Cm_map["Cmfade"]                =      Cmfade_flag                ;
  Cm_map["Cmfdf"]                 =      Cmfdf_flag                 ;
  Cm_map["Cmfadf"]                =      Cmfadf_flag                ;
}

// end uiuc_map_Cm.cpp
