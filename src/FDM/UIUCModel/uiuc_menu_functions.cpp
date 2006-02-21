/**********************************************************************
                                                                       
 FILENAME:     uiuc_menu_functions.cpp

----------------------------------------------------------------------

 DESCRIPTION:  provides common functions used by different menu
               routines

----------------------------------------------------------------------

 STATUS:       alpha version

----------------------------------------------------------------------

 REFERENCES:   

----------------------------------------------------------------------

 HISTORY:      04/04/2003   initial release
               06/30/2003   (RD) replaced istrstream with istringstream
                            to get rid of the annoying warning about
                            using the strstream header

----------------------------------------------------------------------

 AUTHOR(S):    Robert Deters      <rdeters@uiuc.edu>
               Michael Selig      <m-selig@uiuc.edu>

----------------------------------------------------------------------

 VARIABLES:

----------------------------------------------------------------------

 INPUTS:       n/a

----------------------------------------------------------------------

 OUTPUTS:      n/a

----------------------------------------------------------------------

 CALLED BY:    uiuc_menu_XX()

----------------------------------------------------------------------

 CALLS TO:     

 ----------------------------------------------------------------------

 COPYRIGHT:    (C) 2003 by Michael Selig

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

#include "uiuc_menu_functions.h"

bool check_float( const string &token)
{
  float value;
  istringstream stream(token.c_str()); 
  return (stream >> value);
}

void d_2_to_3( double array2D[100][100], double array3D[][100][100], int index3D)
{
  for (register int i=0; i<=99; i++)
    {
      for (register int j=1; j<=99; j++)
	{
	  array3D[index3D][i][j]=array2D[i][j];
	}
    }
}

void d_1_to_2( double array1D[100], double array2D[][100], int index2D)
{
  for (register int i=0; i<=99; i++)
    {
      array2D[index2D][i]=array1D[i];
    }
}

void d_1_to_1( double array1[100], double array2[100] )
{
  for (register int i=0; i<=99; i++)
    {
      array2[i]=array1[i];
    }
}

void i_1_to_2( int array1D[100], int array2D[][100], int index2D)
{
  for (register int i=0; i<=99; i++)
    {
      array2D[index2D][i]=array1D[i];
    }
}
