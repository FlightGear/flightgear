/**********************************************************************

 FILENAME:     uiuc_aircraftdir.h

----------------------------------------------------------------------

 DESCRIPTION: Stores the name of the aircraft directory to be used

----------------------------------------------------------------------

 STATUS:       alpha version

----------------------------------------------------------------------

 REFERENCES:   

----------------------------------------------------------------------

 HISTORY:      02/22/2000    initial release

----------------------------------------------------------------------

 AUTHOR(S):    Bipin Sehgal       <bsehgal@uiuc.edu>

----------------------------------------------------------------------

 VARIABLES:

----------------------------------------------------------------------

 INPUTS:       *

----------------------------------------------------------------------

 OUTPUTS:      *

----------------------------------------------------------------------

 CALLED BY:    *

----------------------------------------------------------------------

 CALLS TO:     *

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


#ifndef _AIRCRAFTDIR_H_
#define _AIRCRAFTDIR_H_

#include <string>

typedef struct 
{
    string aircraft_dir;
    #define aircraft_dir aircraftdir_->aircraft_dir
  
} AIRCRAFTDIR;

// usually defined in the first program that includes uiuc_aircraft.h
extern AIRCRAFTDIR *aircraftdir_;

#endif  // endif _AIRCRAFTDIR_H
