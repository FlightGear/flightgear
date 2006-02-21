/**********************************************************************

 FILENAME:     uiuc_1Dinterpolation.cpp

----------------------------------------------------------------------

 DESCRIPTION:  reads in the yData and xData arrays and the value of x 
               to be interpolated on; performs 1D interpolation, 
               i.e. y=f(x)

----------------------------------------------------------------------

 STATUS:       alpha version

----------------------------------------------------------------------

 REFERENCES:   syntax based on interp function in c172_aero.c
               mathematics based on linear interpolation functions 
               found in
               Kreyszig, Erwin. Advanced Engineering Mathematics, 
               7th ed. NY: John Wiley & Sons, 1993.

----------------------------------------------------------------------

 HISTORY:      02/03/2000   initial release
               09/01/2002   (RD) added second interpolation routine
                            for integer case

----------------------------------------------------------------------

 AUTHOR(S):    Jeff Scott         <jscott@mail.com>
               Robert Deters      <rdeters@uiuc.edu>

----------------------------------------------------------------------

 VARIABLES:

----------------------------------------------------------------------

 INPUTS:       -array of x data
               -array of y data
               -max number of data pairs
               -x value to be interpolated on

----------------------------------------------------------------------

 OUTPUTS:      -y as function of x

----------------------------------------------------------------------

 CALLED BY:    uiuc_coefficients.cpp

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
#include <simgear/compiler.h>    // MSVC: to disable C4244 d to f warning

#include "uiuc_1Dinterpolation.h"


double uiuc_1Dinterpolation( double xData[100], double yData[100], int xmax, double x )
{
  double x1=0, x2=0, y1=0, y2=0, L1=0, L2=0;
  int i=2;
  float yfx=0;

  //check bounds on x to see if data range encloses it
  // NOTE: [1] is first element of all arrays, [0] not used
  if (x <= xData[1])         //if x less than lowest x
    {
      yfx = yData[1];        //let y equal lowest y
    }
  else if (x >= xData[xmax]) //if x greater than greatest x
    {
      yfx = yData[xmax];     //let y equal greatest y
    }
  else                       //x between xmax and x min
    {
      /*loop increases i until x is less than a known x, 
        e.g. Alpha from LaRCsim less than Alpha given in 
        tabulated data; once this value is found, i becomes 
        the upper bound and i-1 the lower bound*/
      while (xData[i] <= x)    //bracket upper bound
        {
          i++;
        }
      x2 = xData[i];          //set upper bounds
      y2 = yData[i];
      x1 = xData[i-1];        //set lower bounds
      y1 = yData[i-1];

      //calculate Langrange polynomial coefficients 
      //(see Kreyszig, pg. 937)
      L1 = (x - x2) / (x1 - x2);
      L2 = (x - x1) / (x2 - x1);

      //solve for y=f(x)
      yfx = L1 * y1 + L2 * y2;
    }
  return yfx;
}


int uiuc_1Dinterpolation( double xData[], int yData[], int xmax, double x )
{
  double x1=0, x2=0, xdiff=0;
  int y1=0, y2=0;
  int i=2;
  int yfx=0;

  //check bounds on x to see if data range encloses it
  // NOTE: [1] is first element of all arrays, [0] not used
  if (x <= xData[1])         //if x less than lowest x
    {
      yfx = yData[1];        //let y equal lowest y
    }
  else if (x >= xData[xmax]) //if x greater than greatest x
    {
      yfx = yData[xmax];     //let y equal greatest y
    }
  else                       //x between xmax and x min
    {
      /*loop increases i until x is less than a known x, 
        e.g. Alpha from LaRCsim less than Alpha given in 
        tabulated data; once this value is found, i becomes 
        the upper bound and i-1 the lower bound*/
      while (xData[i] <= x)    //bracket upper bound
        {
          i++;
        }
      x2 = xData[i];          //set upper bounds
      y2 = yData[i];
      x1 = xData[i-1];        //set lower bounds
      y1 = yData[i-1];

      xdiff = x2 - x1;
      if (y1 == y2)
	yfx = y1;
      else if (x < x1+xdiff/2)
	yfx = y1;
      else
	yfx = y2;
    }
  return yfx;
}

// end uiuc_1Dinterpolation.cpp
