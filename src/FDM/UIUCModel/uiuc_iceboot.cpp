/**********************************************************************

 FILENAME:     uiuc_iceboot.cpp

----------------------------------------------------------------------

 DESCRIPTION:  checks if the iceboot is on, if so then eta ice will equal 
               zero

----------------------------------------------------------------------

 STATUS:       alpha version

----------------------------------------------------------------------

 REFERENCES:   

----------------------------------------------------------------------

 HISTORY:      01/11/2002   initial release
               
----------------------------------------------------------------------

 AUTHOR(S):    Robert Deters       <rdeters@uiuc.edu>
   	       Ann Peedikayil	   <peedikay@uiuc.edu>

----------------------------------------------------------------------

 VARIABLES:

----------------------------------------------------------------------

 INPUTS:       -Simtime
               -icing times
               -dt
	       -bootTime 
              
----------------------------------------------------------------------

 OUTPUTS:      -icing severity (eta_ice)

----------------------------------------------------------------------

 CALLED BY:    uiuc_coefficients
              
----------------------------------------------------------------------

 CALLS TO:     none

----------------------------------------------------------------------

 COPYRIGHT:    (C) 2002 by Michael Selig

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

#include "uiuc_iceboot.h"


void uiuc_iceboot(double dt)
{ 
    	    
  if (bootTrue[bootindex])
    {
      if (bootTime[bootindex]- dt <Simtime && bootTime[bootindex]+ dt >Simtime)
      // checks if the boot is on
       { 
         eta_ice = 0;	      
	 // drops the eta ice to zero
	   
	 if (bootTime [bootindex] > iceTime)
	   iceTime = bootTime[bootindex];
	 bootindex++;
       }
    }
}
