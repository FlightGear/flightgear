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
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
 USA or view http://www.gnu.org/copyleft/gpl.html.
 
**********************************************************************/

#include "uiuc_map_engine.h"


void uiuc_map_engine()
{
  engine_map["simpleSingle"]      =      simpleSingle_flag         ;
  engine_map["c172"]              =      c172_flag                 ;
  engine_map["cherokee"]          =      cherokee_flag             ;
  engine_map["Throttle_pct_input"]=      Throttle_pct_input_flag   ;
}

// end uiuc_map_engine.cpp
