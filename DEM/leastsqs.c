/* leastsqs.c -- Implements a simple linear least squares best fit routine
 *
 * Written by Curtis Olson, started September 1997.
 *
 * Copyright (C) 1997  Curtis L. Olson  - curt@infoplane.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id$
 * (Log is kept at end of this file)
 */


#include <stdio.h>

#include "leastsqs.h"


/* 
Least squares fit:

y = b0 + b1x

     n*sum(xi*yi) - (sum(xi)*sum(yi))
b1 = --------------------------------
     n*sum(xi^2) - (sum(xi))^2


b0 = sum(yi)/n - b1*(sum(xi)/n)
*/

void least_squares(double *x, double *y, int n, double *m, double *b) {
    double sum_xi, sum_yi, sum_xi_2, sum_xi_yi;
    int i;

    sum_xi = sum_yi = sum_xi_2 = sum_xi_yi = 0.0;

    for ( i = 0; i < n; i++ ) {
	sum_xi += x[i];
	sum_yi += y[i];
	sum_xi_2 += x[i] * x[i];
	sum_xi_yi += x[i] * y[i];
    }

    /* printf("sum(xi)=%.2f  sum(yi)=%.2f  sum(xi^2)=%.2f  sum(xi*yi)=%.2f\n",
	   sum_xi, sum_yi, sum_xi_2, sum_xi_yi); */

    *m = ( (double)n * sum_xi_yi - sum_xi * sum_yi ) / 
	( (double)n * sum_xi_2 - sum_xi * sum_xi );
    *b = (sum_yi / (double)n) - (*m) * (sum_xi / (double)n);

    /* printf("slope = %.2f  intercept = %.2f\n", *m, *b); */
}


/* 
  return the least squares error:

              (y[i] - y_hat[i])^2
              -------------------
                      n
 */
double least_squares_error(double *x, double *y, int n, double m, double b) {
    int i;
    double error, sum;

    sum = 0.0;

    for ( i = 0; i < n; i++ ) {
	error = y[i] - (m * x[i] + b);
	sum += error * error;
	/* printf("%.2f %.2f\n", error, sum); */
    }

    return ( sum / (double)n );
}


/* 
  return the maximum least squares error:

              (y[i] - y_hat[i])^2
 */
double least_squares_max_error(double *x, double *y, int n, double m, double b){
    int i;
    double error, max_error;

    max_error = 0.0;

    for ( i = 0; i < n; i++ ) {
	error = y[i] - (m * x[i] + b);
	error = error * error;
	if ( error > max_error ) {
	    max_error = error;
	}
    }

    return ( max_error );
}


/* $Log$
/* Revision 1.1  1998/03/19 02:54:47  curt
/* Reorganized into a class lib called fgDEM.
/*
 * Revision 1.1  1997/10/13 17:02:35  curt
 * Initial revision.
 *
 */
