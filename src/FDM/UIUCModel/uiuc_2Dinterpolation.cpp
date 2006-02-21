/**********************************************************************

 FILENAME:     uiuc_2Dinterpolation.cpp

----------------------------------------------------------------------

 DESCRIPTION:  reads in the zData, yData, and xData arrays and the 
               values of x and y to be interpolated on; performs 2D 
               interpolation, i.e. z=f(x,y)

----------------------------------------------------------------------

 STATUS:       alpha version

----------------------------------------------------------------------

 REFERENCES:   similar to 2D interpolation in Selig's propid code
               (see alcl.f)
               mathematics based on linear interpolation functions 
               (see 1Dinterpolation.cpp for references)

----------------------------------------------------------------------

 HISTORY:      02/06/2000   initial release

----------------------------------------------------------------------

 AUTHOR(S):    Jeff Scott         <jscott@mail.com>

----------------------------------------------------------------------

 VARIABLES:

----------------------------------------------------------------------

 INPUTS:       -2D array of x data for each y case
               -1D array of y data
               -2D array of z data for each y case
               -1D array of max number of x-z data sets for each y case
               -max number of y data
               -x value to be interpolated on
               -y value to be interpolated on

----------------------------------------------------------------------

 OUTPUTS:      -z as function of x and y

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

#include "uiuc_2Dinterpolation.h"


double uiuc_2Dinterpolation( double xData[100][100], 
                             double yData[100], 
                             double zData[100][100], 
                             int xmax[100], 
                             int ymax, 
                             double x, 
                             double y )
{
  int xmaxl=0, xmaxu=0;
  double x11=0, x12=0, x21=0, x22=0, y1=0, y2=0;
  double z11=0, z12=0, z21=0, z22=0, z1=0, z2=0;
  double L11=0, L12=0, L21=0, L22=0, L1=0, L2=0;
  int i=2, j=2;
  float zfxy=0;

  bool luby=false;       //upper bound on y (deflection)
  bool llby=false;       //lower bound on y
  bool lubx1=false;      //upper x bound on lower y
  bool llbx1=false;      //lower x bound on lower y
  bool lubx2=false;      //upper x bound on upper y
  bool llbx2=false;      //lower x bound on upper y

  // check bounds on y (control deflection) to see if data range encloses it
  // NOTE: [1] is first element of all arrays, [0] not used
  if (y <= yData[1])          //if y less than lowest y
    {
      llby = true;
      y1 = yData[1];
      xmaxl = xmax[1];
      xmaxu = xmax[ymax];
    }
  else if (y >= yData[ymax])  //if y greater than greatest y
    {
      luby = true;
      y2 = yData[ymax];
      j = ymax;
      xmaxl = xmax[1];
      xmaxu = xmax[ymax];
    }
  else                        //y between ymax and ymin
    {
      /* loop increases j until y is less than a known y, 
         e.g. elevator from LaRCsim less than de given in 
         tabulated data; once this value is found, j becomes 
         the upper bound and j-1 the lower bound on y */
      while (yData[j] <= y)    //bracket upper bound
        {
          j++;
        }
      y2 = yData[j];      //set upper bound on y
      y1 = yData[j-1];    //set lower bound on y

      xmaxu = xmax[j];    //set max x on upper y
      xmaxl = xmax[j-1];  //set max x on lower y
    }

  //check bounds on x (alpha) to see if data range encloses it
  //x less than lowest x on lower y curve:
  if (x <= xData[j-1][1])
    {
      llbx1 = true;
      z11 = zData[j-1][1];
      z1 = z2 = z11;
    }
  //x greater than greatest x on lower y curve:
  if (x >= xData[j-1][xmaxl])
    {
      lubx1 = true;
      z12 = zData[j-1][xmaxl];
      z1 = z2 = z12;
    }
  //x less than lowest x on upper y curve:
  if (x <= xData[j][1])
    {
      llbx2 = true;
      z21 = zData[j][1];
      z1 = z2 = z21;
    }
  //x greater than greatest x on upper y curve:
  if (x >= xData[j][xmaxu])
    {
      lubx2 = true;
      z22 = zData[j][xmaxu];
      z1 = z2 = z22;
    }

  //x between xmax and x min
  //interpolate on lower y-curve
  if (llbx1 == false && lubx1 == false)
    {
      /* loop increases i until x is less than a known x, 
         e.g. Alpha from LaRCsim less than Alpha given in 
         tabulated data; once this value is found, i becomes 
         the upper bound and i-1 the lower bound */
      //bracket x bounds on lower y curve
      while (xData[j-1][i] <= x)
        {
          i++;
        }
      x12 = xData[j-1][i];        //set upper x and z on lower y
      z12 = zData[j-1][i];
      x11 = xData[j-1][i-1];      //set lower x and z on lower y
      z11 = zData[j-1][i-1];

      //do linear interpolation on x1 terms (lower y-curve)
      L11 = (x - x12) / (x11 - x12);
      L12 = (x - x11) / (x12 - x11);
      z1 = L11 * z11 + L12 * z12;
    }
  //interpolate on upper y-curve
  if (llbx2 == false && lubx2 == false)
    {
      //bracket x bounds on upper y curve
      i = 1;
      while (xData[j][i] <= x)
        {
          i++;
        }
      x22 = xData[j][i];          //set upper x and z on upper y
      z22 = zData[j][i];
      x21 = xData[j][i-1];        //set lower x and z on upper y
      z21 = zData[j][i-1];

      //do linear interpolation on x2 terms (upper y-curve)
      L21 = (x - x22) / (x21 - x22);
      L22 = (x - x21) / (x22 - x21);
      z2 = L21 * z21 + L22 * z22;
    }

  //now have all data needed to find coefficient, check cases:
  if (llby == true)
    {
      if (llbx1 == true || llbx2 == true)
        {
          z1 = z11;
          z2 = z21;
        }
      if (lubx1 == true || lubx2 == true)
        {
          z1 = z12;
          z2 = z22;
        }
      else
        {
          zfxy = z1;
        }
    }
  else if (luby == true)
    {
      if (llbx1 == true || llbx2 == true)
        {
          z1 = z11;
          z2 = z21;
        }
      if (lubx1 == true || lubx2 == true)
        {
          z1 = z12;
          z2 = z22;
        }
      else
        {
          zfxy = z2;
        }
    }

  //do linear interpolation on y terms
  L1 = (y - y2) / (y1 - y2);
  L2 = (y - y1) / (y2 - y1);

  //solve for z=f(x,y)
  zfxy = L1 * z1 + L2 * z2;

  return zfxy;
}

// end uiuc_2Dinterpolation.cpp
