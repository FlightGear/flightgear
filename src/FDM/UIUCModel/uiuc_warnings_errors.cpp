/**********************************************************************
                                                                         
 FILENAME:     uiuc_warnings_errors.cpp

----------------------------------------------------------------------

 DESCRIPTION:  

Prints to screen the follow: 

- Error Code (errorCode)

- Message indicating the problem. This message should be preceded by
"Warning", "Error" or "Note".  Warnings are non-fatal and the code
will pause.  Errors are fatal and will stop the code.  Notes are only
for information.
 

----------------------------------------------------------------------

 STATUS:       alpha version

----------------------------------------------------------------------

 REFERENCES:   based on "menu reader" format of Michael Selig

----------------------------------------------------------------------

 HISTORY:      01/29/2000   initial release

----------------------------------------------------------------------

 AUTHOR(S):    Bipin Sehgal       <bsehgal@uiuc.edu>
               Jeff Scott         <jscott@mail.com>
               Michael Selig      <m-selig@uiuc.edu>

----------------------------------------------------------------------

 VARIABLES:

----------------------------------------------------------------------

 INPUTS:       -error code
               -input line

----------------------------------------------------------------------

 OUTPUTS:      -warning/error message to screen

----------------------------------------------------------------------

 CALLED BY:    uiuc_menu.cpp

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
#include <stdlib.h>
#include <string.h>

#include "uiuc_warnings_errors.h"

#if !defined (SG_HAVE_NATIVE_SGI_COMPILERS)
SG_USING_STD (cerr);
SG_USING_STD (endl);

#  ifndef _MSC_VER
SG_USING_STD (exit);
#  endif
#endif

void uiuc_warnings_errors(int errorCode, string line)
{
  switch (errorCode)
    {
    case 1:
      cerr << "UIUC ERROR 1: The value of the coefficient in \"" << line << "\" should be of type float" << endl;
      exit(-1);
      break;
    case 2:
      cerr << "UIUC ERROR 2: Unknown identifier in \"" << line << "\" (check uiuc_maps_*.cpp)" << endl;
      exit(-1);
      break;
    case 3:
      cerr << "UIUC ERROR 3: Slipstream effects only work w/ the engine simpleSingleModel line: \"" << line  << endl;
      exit(-1);
      break;
    case 4:
      cerr << "UIUC ERROR 4: Downwash mode does not exist: \"" << line  << endl;
      exit(-1);
      break;
    case 5:
      cerr << "UIUC ERROR 5: Must use dyn_on_speed not equal to zero: \"" << line << endl;
      exit(-1);
      break;
    }
}

// end uiuc_warnings_errors.cpp
