// leastsqs.h -- Implements a simple linear least squares best fit routine
//
// Written by Curtis Olson, started September 1997.
//
// Copyright (C) 1997  Curtis L. Olson  - curt@infoplane.com
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$
// (Log is kept at end of this file)
///


#ifndef _LEASTSQS_H
#define _LEASTSQS_H


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


/* 
Least squares fit:

y = b0 + b1x

     n*sum(xi*yi) - (sum(xi)*sum(yi))
b1 = --------------------------------
     n*sum(xi^2) - (sum(xi))^2


b0 = sum(yi)/n - b1*(sum(xi)/n)
*/

void least_squares(double *x, double *y, int n, double *m, double *b);

/* incrimentally update existing values with a new data point */
void least_squares_update(double x, double y, double *m, double *b);


/* 
  return the least squares error:

              (y[i] - y_hat[i])^2
              -------------------
                      n
*/
double least_squares_error(double *x, double *y, int n, double m, double b);


/* 
  return the maximum least squares error:

              (y[i] - y_hat[i])^2
*/
double least_squares_max_error(double *x, double *y, int n, double m, double b);


#endif // _LEASTSQS_H


// $Log$
// Revision 1.1  1999/04/05 21:32:33  curt
// Initial revision
//
// Revision 1.1  1999/03/13 17:34:45  curt
// Moved to math subdirectory.
//
// Revision 1.2  1998/04/21 17:03:42  curt
// Prepairing for C++ integration.
//
// Revision 1.1  1998/04/08 22:57:25  curt
// Adopted Gnu automake/autoconf system.
//
// Revision 1.1  1998/03/19 02:54:48  curt
// Reorganized into a class lib called fgDEM.
//
// Revision 1.1  1997/10/13 17:02:35  curt
// Initial revision.
//
