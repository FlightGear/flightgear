/********************************************************************** 
 * 
 * FILENAME:     uiuc_initializemaps.cpp 
 *
 * ---------------------------------------------------------------------- 
 *
 * DESCRIPTION:  Initializes the maps for various keywords 
 *
 * ----------------------------------------------------------------------
 * 
 * STATUS:       alpha version
 *
 * ----------------------------------------------------------------------
 * 
 * REFERENCES:   
 * 
 * ----------------------------------------------------------------------
 * 
 * HISTORY:      01/26/2000   initial release
 * 
 * ----------------------------------------------------------------------
 * 
 * AUTHOR(S):    Bipin Sehgal       <bsehgal@uiuc.edu>
 * 
 * ----------------------------------------------------------------------
 * 
 * VARIABLES:
 * 
 * ----------------------------------------------------------------------
 * 
 * INPUTS:       *
 * 
 * ----------------------------------------------------------------------
 * 
 * OUTPUTS:      *
 * 
 * ----------------------------------------------------------------------
 * 
 * CALLED BY:    uiuc_wrapper.cpp
 * 
 * ----------------------------------------------------------------------
 * 
 * CALLS TO:     *
 * 
 * ----------------------------------------------------------------------
 * 
 * COPYRIGHT:    (C) 2000 by Michael Selig
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
 * USA or view http://www.gnu.org/copyleft/gpl.html.
 * 
 ***********************************************************************/


#include "uiuc_initializemaps.h"

void uiuc_initializemaps ()
{
    uiuc_initializemaps1();
    uiuc_initializemaps2();
    uiuc_initializemaps3();
    uiuc_initializemaps4();
}

// end uiuc_initializemaps.cpp
