/********************************************************************** 
 
 FILENAME:     uiuc_map_Croll.cpp 

---------------------------------------------------------------------- 

 DESCRIPTION:  initializes the Cl map

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

#include "uiuc_map_Croll.h"


void uiuc_map_Croll()
{
  Cl_map["Clo"]                   =      Clo_flag                   ;
  Cl_map["Cl_beta"]               =      Cl_beta_flag               ;
  Cl_map["Cl_p"]                  =      Cl_p_flag                  ;
  Cl_map["Cl_r"]                  =      Cl_r_flag                  ;
  Cl_map["Cl_da"]                 =      Cl_da_flag                 ;
  Cl_map["Cl_dr"]                 =      Cl_dr_flag                 ;
  Cl_map["Cl_daa"]                =      Cl_daa_flag                ;
  Cl_map["Clfada"]                =      Clfada_flag                ;
  Cl_map["Clfbetadr"]             =      Clfbetadr_flag             ;
}

// end uiuc_map_Croll.cpp
