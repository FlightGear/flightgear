/********************************************************************** 
 
 FILENAME:     uiuc_map_CD.cpp 

---------------------------------------------------------------------- 

 DESCRIPTION:  initializes the CD map

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

#include "uiuc_map_CD.h"


void uiuc_map_CD()
{
  CD_map["CDo"]                   =      CDo_flag                   ;
  CD_map["CDK"]                   =      CDK_flag                   ;
  CD_map["CD_a"]                  =      CD_a_flag                  ;
  CD_map["CD_adot"]               =      CD_adot_flag               ;
  CD_map["CD_q"]                  =      CD_q_flag                  ;
  CD_map["CD_ih"]                 =      CD_ih_flag                 ;
  CD_map["CD_de"]                 =      CD_de_flag                 ;
  CD_map["CDfa"]                  =      CDfa_flag                  ;
  CD_map["CDfCL"]                 =      CDfCL_flag                 ;
  CD_map["CDfade"]                =      CDfade_flag                ;
  CD_map["CDfdf"]                 =      CDfdf_flag                 ;
  CD_map["CDfadf"]                =      CDfadf_flag                ;
  CD_map["CXo"]                   =      CXo_flag                   ;
  CD_map["CXK"]                   =      CXK_flag                   ;
  CD_map["CX_a"]                  =      CX_a_flag                  ;
  CD_map["CX_a2"]                 =      CX_a2_flag                 ;
  CD_map["CX_a3"]                 =      CX_a3_flag                 ;
  CD_map["CX_adot"]               =      CX_adot_flag               ;
  CD_map["CX_q"]                  =      CX_q_flag                  ;
  CD_map["CX_de"]                 =      CX_de_flag                 ;
  CD_map["CX_dr"]                 =      CX_dr_flag                 ;
  CD_map["CX_df"]                 =      CX_df_flag                 ;
  CD_map["CX_adf"]                =      CX_adf_flag                ;
}

// end uiuc_map_CD.cpp
