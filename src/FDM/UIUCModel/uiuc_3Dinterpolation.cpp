/**********************************************************************

 FILENAME:     uiuc_3Dinterpolation.cpp

----------------------------------------------------------------------

 DESCRIPTION:  A 3D interpolator.  Does a linear interpolation between
               two values that were found from using the 2D
	       interpolator (3Dinterpolation()), or uses 3Dinterp_quick()
	       to perform a 3D linear interpolation on "nice" data

----------------------------------------------------------------------

 STATUS:       alpha version

----------------------------------------------------------------------

 REFERENCES:

----------------------------------------------------------------------

 HISTORY:      11/07/2001   initial release
               02/18/2002   (RD) Created uiuc_3Dinterp_quick() to take
	                    advantage of the "nice" format of the
			    nonlinear Twin Otter data.  Performs a
			    quicker 3D interpolation.  Modified
			    uiuc_3Dinterpolation() to handle new input
			    form of the data.
----------------------------------------------------------------------

 AUTHOR(S):    Robert Deters      <rdeters@uiuc.edu>

----------------------------------------------------------------------

 VARIABLES:

----------------------------------------------------------------------

 INPUTS:       

----------------------------------------------------------------------

 OUTPUTS:      interpI

----------------------------------------------------------------------

 CALLED BY:    uiuc_coef_drag
               uiuc_coef_lift
	       uiuc_coef_pitch
	       uiuc_coef_roll
	       uiuc_coef_sideforce
	       uiuc_coef_yaw

----------------------------------------------------------------------

 CALLS TO:     2Dinterpolation

----------------------------------------------------------------------

 COPYRIGHT:    (C) 2001 by Michael Selig

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

#include "uiuc_3Dinterpolation.h"

void uiuc_1DtoSingle( int temp1Darray[30], 
		      int filenumber,
		      int &single_value)
{
  single_value = temp1Darray[filenumber];
}

void uiuc_2Dto1D( double temp2Darray[30][100], 
		  int filenumber,
		  double array1D[100])
{
  int count1;
  
  for (count1=0; count1<=99; count1++)
    {
      array1D[count1] = temp2Darray[filenumber][count1];
    }
}

void uiuc_2Dto1D_int( int temp2Darray[30][100], 
		      int filenumber,
		      int array1D[100])
{
  int count1;
  
  for (count1=0; count1<=99; count1++)
    {
      array1D[count1] = temp2Darray[filenumber][count1];
    }
}

void uiuc_3Dto2D( double temp3Darray[30][100][100],
		  int filenumber,
		  double array2D[100][100])
{
  int count1, count2;
  
  for (count1=0; count1<=99; count1++)
    {
      for (count2=0; count2<=99; count2++)
	{
	  array2D[count1][count2] = temp3Darray[filenumber][count1][count2];
	}
    }
}

double uiuc_3Dinterpolation( double third_Array[30],
			     double full_xArray[30][100][100],
			     double full_yArray[30][100],
			     double full_zArray[30][100][100],
			     int full_nxArray[30][100],
			     int full_ny[30],
			     int third_max,
			     double third_bet,
			     double x_value,
			     double y_value)
{
  double reduced_xArray[100][100], reduced_yArray[100];
  double reduced_zArray[100][100];
  int reduced_nxArray[100], reduced_ny;

  double interpmin, interpmax, third_u, third_l;
  double interpI;
  int third_min;
  int k=1;
  bool third_same=false;

  if (third_bet <= third_Array[1])
    {
      third_min = 1;
      third_same = true;
    }
  else if (third_bet >= third_Array[third_max])
    {
      third_min = third_max;
      third_same = true;
    }
  else
    {
      while (third_Array[k] <= third_bet)
	{
	  k++;
	}
      third_max = k;
      third_min = k-1;
    }
  if (third_same)
    {

      uiuc_3Dto2D(full_xArray, third_min, reduced_xArray);
      uiuc_2Dto1D(full_yArray, third_min, reduced_yArray);
      uiuc_3Dto2D(full_zArray, third_min, reduced_zArray);
      uiuc_2Dto1D_int(full_nxArray, third_min, reduced_nxArray);
      uiuc_1DtoSingle(full_ny, third_min, reduced_ny);

      interpI = uiuc_2Dinterpolation(reduced_xArray,
				     reduced_yArray,
				     reduced_zArray,
				     reduced_nxArray,
				     reduced_ny,
				     x_value,
				     y_value);
    }
  else
    {
      uiuc_3Dto2D(full_xArray, third_min, reduced_xArray);
      uiuc_2Dto1D(full_yArray, third_min, reduced_yArray);
      uiuc_3Dto2D(full_zArray, third_min, reduced_zArray);
      uiuc_2Dto1D_int(full_nxArray, third_min, reduced_nxArray);
      uiuc_1DtoSingle(full_ny, third_min, reduced_ny);

      interpmin = uiuc_2Dinterpolation(reduced_xArray,
				     reduced_yArray,
				     reduced_zArray,
				     reduced_nxArray,
				     reduced_ny,
				     x_value,
				     y_value);

      uiuc_3Dto2D(full_xArray, third_max, reduced_xArray);
      uiuc_2Dto1D(full_yArray, third_max, reduced_yArray);
      uiuc_3Dto2D(full_zArray, third_max, reduced_zArray);
      uiuc_2Dto1D_int(full_nxArray, third_max, reduced_nxArray);
      uiuc_1DtoSingle(full_ny, third_max, reduced_ny);

      interpmax = uiuc_2Dinterpolation(reduced_xArray,
				     reduced_yArray,
				     reduced_zArray,
				     reduced_nxArray,
				     reduced_ny,
				     x_value,
				     y_value);

      third_u = third_Array[third_max];
      third_l = third_Array[third_min];

      interpI=interpmax - (third_u-third_bet)*(interpmax-interpmin)/(third_u-third_l);
    }

  return interpI;

}


double uiuc_3Dinterp_quick( double z[30],
			    double x[100],
			    double y[100],
			    double fxyz[30][100][100],
			    int xmax,
			    int ymax,
			    int zmax,
			    double zp,
			    double xp,
			    double yp)
{

  int xnuml, xnumu, ynuml, ynumu, znuml, znumu;
  double xl, xu, yl, yu, zl, zu;
  double ptxl, ptxu, ptyl, ptyu, ptylxl, ptylxu, ptyuxl, ptyuxu;
  double ptzl, ptzu, ptzlxl, ptzlxu, ptzuxl, ptzuxu;
  double ptzlyl, ptzlyu, ptzuyl, ptzuyu;
  double ptzlylxl, ptzlylxu, ptzlyuxl, ptzlyuxu;
  double ptzuylxl, ptzuylxu, ptzuyuxl, ptzuyuxu, data_point;

  int i=1;
  int j=1;
  int k=1;

  bool xsame=false;
  bool ysame=false;
  bool zsame=false;


  // Find the z's
  if (zp <= z[1])
    {
      znuml=1;
      zsame=true;
    }
  else if (zp >= z[zmax])
    {
      znuml=zmax;
      zsame=true;
    }
  else
    {
      while (z[k] <= zp)
	k++;
      zu=z[k];
      zl=z[k-1];
      znumu=k;
      znuml=k-1;
    }

  // Find the y's
  if (yp <= y[1])
    {
      ynuml=1;
      ysame=true;
    }
  else if (yp >= y[ymax])
    {
      ynuml=ymax;
      ysame=true;
    }
  else
    {
      while (y[j] <= yp)
	j++;
      yu=y[j];
      yl=y[j-1];
      ynumu=j;
      ynuml=j-1;
    }


  // Find the x's
  if (xp <= x[1])
    {
      xnuml=1;
      xsame=true;
    }
  else if (xp >= x[xmax])
    {
      xnuml=xmax;
      xsame=true;
    }
  else
    {
      while (x[i] <= xp)
	i++;
      xu=x[i];
      xl=x[i-1];
      xnumu=i;
      xnuml=i-1;
    }

  if (zsame)
    {
      if (ysame && xsame)
	{
	  data_point = fxyz[znuml][ynuml][xnuml];
	}
      else if (ysame)
	{
	  ptxl = fxyz[znuml][ynuml][xnuml];
	  ptxu = fxyz[znuml][ynuml][xnumu];
	  data_point = ptxu - (xu-xp)*(ptxu-ptxl)/(xu-xl);
	}
      else if (xsame)
	{
	  ptyl = fxyz[znuml][ynuml][xnuml];
	  ptyu = fxyz[znuml][ynumu][xnuml];
	  data_point = ptyu - (yu-yp)*(ptyu-ptyl)/(yu-yl);
	}
      else
	{
	  ptylxl = fxyz[znuml][ynuml][xnuml];
	  ptylxu = fxyz[znuml][ynuml][xnumu];
	  ptyuxl = fxyz[znuml][ynumu][xnuml];
	  ptyuxu = fxyz[znuml][ynumu][xnumu];
	  ptyl = ptylxu - (xu-xp)*(ptylxu-ptylxl)/(xu-xl);
	  ptyu = ptyuxu - (xu-xp)*(ptyuxu-ptyuxl)/(xu-xl);
	  data_point = ptyu - (yu-yp)*(ptyu-ptyl)/(yu-yl);
	}
    }
  else
    {
      if (ysame && xsame)
	{
	  ptzl = fxyz[znuml][ynuml][xnuml];
	  ptzu = fxyz[znumu][ynuml][xnuml];
	  data_point = ptzu - (zu-zp)*(ptzu-ptzl)/(zu-zl);
	}
      else if (ysame)
	{
	  ptzlxl = fxyz[znuml][ynuml][xnuml];
	  ptzlxu = fxyz[znuml][ynuml][xnumu];
	  ptzuxl = fxyz[znumu][ynuml][xnuml];
	  ptzuxu = fxyz[znumu][ynuml][xnumu];
	  ptzl = ptzlxu - (xu-xp)*(ptzlxu-ptzlxl)/(xu-xl);
	  ptzu = ptzuxu - (xu-xp)*(ptzuxu-ptzuxl)/(xu-xl);
	  data_point = ptzu - (zu-zp)*(ptzu-ptzl)/(zu-zl);
	}
      else if (xsame)
	{
	  ptzlyl = fxyz[znuml][ynuml][xnuml];
	  ptzlyu = fxyz[znuml][ynumu][xnuml];
	  ptzuyl = fxyz[znumu][ynuml][xnuml];
	  ptzuyu = fxyz[znumu][ynumu][xnuml];
	  ptzl = ptzlyu - (yu-yp)*(ptzlyu-ptzlyl)/(yu-yl);
	  ptzu = ptzuyu - (yu-yp)*(ptzuyu-ptzuyl)/(yu-yl);
	  data_point = ptzu - (zu-zp)*(ptzu-ptzl)/(zu-zl);
	}
      else
	{
	  ptzlylxl = fxyz[znuml][ynuml][xnuml];
	  ptzlylxu = fxyz[znuml][ynuml][xnumu];
	  ptzlyuxl = fxyz[znuml][ynumu][xnuml];
	  ptzlyuxu = fxyz[znuml][ynumu][xnumu];
	  ptzuylxl = fxyz[znumu][ynuml][xnuml];
	  ptzuylxu = fxyz[znumu][ynuml][xnumu];
	  ptzuyuxl = fxyz[znumu][ynumu][xnuml];
	  ptzuyuxu = fxyz[znumu][ynumu][xnumu];
	  ptzlyl = ptzlylxu - (xu-xp)*(ptzlylxu-ptzlylxl)/(xu-xl);
	  ptzlyu = ptzlyuxu - (xu-xp)*(ptzlyuxu-ptzlyuxl)/(xu-xl);
	  ptzuyl = ptzuylxu - (xu-xp)*(ptzuylxu-ptzuylxl)/(xu-xl);
	  ptzuyu = ptzuyuxu - (xu-xp)*(ptzuyuxu-ptzuyuxl)/(xu-xl);
	  ptzl = ptzlyu - (yu-yp)*(ptzlyu-ptzlyl)/(yu-yl);
	  ptzu = ptzuyu - (yu-yp)*(ptzuyu-ptzuyl)/(yu-yl);
	  data_point = ptzu - (zu-zp)*(ptzu-ptzl)/(zu-zl);
	}

    }


  return data_point;
}
