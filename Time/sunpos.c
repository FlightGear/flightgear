/*
 * sunpos.c
 * kirk johnson
 * july 1993
 *
 * code for calculating the position on the earth's surface for which
 * the sun is directly overhead (adapted from _practical astronomy
 * with your calculator, third edition_, peter duffett-smith,
 * cambridge university press, 1988.)
 *
 * RCS $Id$
 *
 * Copyright (C) 1989, 1990, 1993, 1994, 1995 Kirk Lauritz Johnson
 *
 * Parts of the source code (as marked) are:
 *   Copyright (C) 1989, 1990, 1991 by Jim Frost
 *   Copyright (C) 1992 by Jamie Zawinski <jwz@lucid.com>
 *
 * Permission to use, copy, modify and freely distribute xearth for
 * non-commercial and not-for-profit purposes is hereby granted
 * without fee, provided that both the above copyright notice and this
 * permission notice appear in all copies and in supporting
 * documentation.
 *
 * The author makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, INDIRECT
 * OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * $Id$
 * (Log is kept at end of this file)
 */


#include <math.h>
#include <stdio.h>
#include <time.h>

#include "sunpos.h"
#include "fg_time.h"
#include "../constants.h"
#include "../GLUT/views.h"
#include "../Math/fg_geodesy.h"
#include "../Math/mat3.h"
#include "../Math/polar.h"

#undef E


/*
 * the epoch upon which these astronomical calculations are based is
 * 1990 january 0.0, 631065600 seconds since the beginning of the
 * "unix epoch" (00:00:00 GMT, Jan. 1, 1970)
 *
 * given a number of seconds since the start of the unix epoch,
 * DaysSinceEpoch() computes the number of days since the start of the
 * astronomical epoch (1990 january 0.0)
 */

#define EpochStart           (631065600)
#define DaysSinceEpoch(secs) (((secs)-EpochStart)*(1.0/(24*3600)))

/*
 * assuming the apparent orbit of the sun about the earth is circular,
 * the rate at which the orbit progresses is given by RadsPerDay --
 * FG_2PI radians per orbit divided by 365.242191 days per year:
 */

#define RadsPerDay (FG_2PI/365.242191)

/*
 * details of sun's apparent orbit at epoch 1990.0 (after
 * duffett-smith, table 6, section 46)
 *
 * Epsilon_g    (ecliptic longitude at epoch 1990.0) 279.403303 degrees
 * OmegaBar_g   (ecliptic longitude of perigee)      282.768422 degrees
 * Eccentricity (eccentricity of orbit)                0.016713
 */

#define Epsilon_g    (279.403303*(FG_2PI/360))
#define OmegaBar_g   (282.768422*(FG_2PI/360))
#define Eccentricity (0.016713)

/*
 * MeanObliquity gives the mean obliquity of the earth's axis at epoch
 * 1990.0 (computed as 23.440592 degrees according to the method given
 * in duffett-smith, section 27)
 */
#define MeanObliquity (23.440592*(FG_2PI/360))

static double solve_keplers_equation(double);
static double sun_ecliptic_longitude(time_t);
static void   ecliptic_to_equatorial(double, double, double *, double *);
static double julian_date(int, int, int);
static double GST(time_t);

/*
 * solve Kepler's equation via Newton's method
 * (after duffett-smith, section 47)
 */
static double solve_keplers_equation(double M) {
    double E;
    double delta;

    E = M;
    while (1) {
	delta = E - Eccentricity*sin(E) - M;
	if (fabs(delta) <= 1e-10) break;
	E -= delta / (1 - Eccentricity*cos(E));
    }

    return E;
}


/* compute ecliptic longitude of sun (in radians) (after
 * duffett-smith, section 47) */

static double sun_ecliptic_longitude(time_t ssue) {
    /* time_t ssue;              seconds since unix epoch */
    double D, N;
    double M_sun, E;
    double v;

    D = DaysSinceEpoch(ssue);

    N = RadsPerDay * D;
    N = fmod(N, FG_2PI);
    if (N < 0) N += FG_2PI;

    M_sun = N + Epsilon_g - OmegaBar_g;
    if (M_sun < 0) M_sun += FG_2PI;

    E = solve_keplers_equation(M_sun);
    v = 2 * atan(sqrt((1+Eccentricity)/(1-Eccentricity)) * tan(E/2));

    return (v + OmegaBar_g);
}


/* convert from ecliptic to equatorial coordinates (after
 * duffett-smith, section 27) */

static void ecliptic_to_equatorial(double lambda, double beta, 
				   double *alpha, double *delta) {
    /* double  lambda;            ecliptic longitude       */
    /* double  beta;              ecliptic latitude        */
    /* double *alpha;             (return) right ascension */
    /* double *delta;             (return) declination     */

    double sin_e, cos_e;

    sin_e = sin(MeanObliquity);
    cos_e = cos(MeanObliquity);

    *alpha = atan2(sin(lambda)*cos_e - tan(beta)*sin_e, cos(lambda));
    *delta = asin(sin(beta)*cos_e + cos(beta)*sin_e*sin(lambda));
}


/* computing julian dates (assuming gregorian calendar, thus this is
 * only valid for dates of 1582 oct 15 or later) (after duffett-smith,
 * section 4) */

static double julian_date(int y, int m, int d) {
    /* int y;                    year (e.g. 19xx)          */
    /* int m;                    month (jan=1, feb=2, ...) */
    /* int d;                    day of month              */

    int    A, B, C, D;
    double JD;

    /* lazy test to ensure gregorian calendar */
    if (y < 1583) {
	printf("WHOOPS! Julian dates only valid for 1582 oct 15 or later\n");
    }

    if ((m == 1) || (m == 2)) {
	y -= 1;
	m += 12;
    }

    A = y / 100;
    B = 2 - A + (A / 4);
    C = 365.25 * y;
    D = 30.6001 * (m + 1);

    JD = B + C + D + d + 1720994.5;

    return JD;
}


/* compute greenwich mean sidereal time (GST) corresponding to a given
 * number of seconds since the unix epoch (after duffett-smith,
 * section 12) */
static double GST(time_t ssue) {
    /* time_t ssue;           seconds since unix epoch */

    double     JD;
    double     T, T0;
    double     UT;
    struct tm *tm;

    tm = gmtime(&ssue);

    JD = julian_date(tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday);
    T  = (JD - 2451545) / 36525;

    T0 = ((T + 2.5862e-5) * T + 2400.051336) * T + 6.697374558;

    T0 = fmod(T0, 24.0);
    if (T0 < 0) T0 += 24;

    UT = tm->tm_hour + (tm->tm_min + tm->tm_sec / 60.0) / 60.0;

    T0 += UT * 1.002737909;
    T0 = fmod(T0, 24.0);
    if (T0 < 0) T0 += 24;

    return T0;
}


/* given a particular time (expressed in seconds since the unix
 * epoch), compute position on the earth (lat, lon) such that sun is
 * directly overhead.  (lat, lon are reported in radians */

void fgSunPosition(time_t ssue, double *lon, double *lat) {
    /* time_t  ssue;           seconds since unix epoch */
    /* double *lat;            (return) latitude        */
    /* double *lon;            (return) longitude       */

    double lambda;
    double alpha, delta;
    double tmp;

    lambda = sun_ecliptic_longitude(ssue);
    ecliptic_to_equatorial(lambda, 0.0, &alpha, &delta);

    tmp = alpha - (FG_2PI/24)*GST(ssue);
    if (tmp < -FG_PI) {
	do tmp += FG_2PI;
	while (tmp < -FG_PI);
    } else if (tmp > FG_PI) {
	do tmp -= FG_2PI;
	while (tmp < -FG_PI);
    }

    *lon = tmp;
    *lat = delta;
}


/* update the cur_time_params structure with the current sun position */
void fgUpdateSunPos(struct fgCartesianPoint scenery_center) {
    struct fgTIME *t;
    struct VIEW *v;
    MAT3vec nup, nsun;
    double sun_gd_lat, sl_radius, temp;
    static int time_warp = 0;

    t = &cur_time_params;
    v = &current_view;

    time_warp += 200; /* increase this to make the world spin real fast */
    time_warp = 3600*10; /* increase this to make the world spin real fast */

    fgSunPosition(time(NULL) + time_warp, &t->sun_lon, &sun_gd_lat);

    fgGeodToGeoc(sun_gd_lat, 0.0, &sl_radius, &t->sun_gc_lat);

    t->fg_sunpos = fgPolarToCart(t->sun_lon, t->sun_gc_lat, sl_radius);

    /* printf("Geodetic lat = %.5f Geocentric lat = %.5f\n", sun_gd_lat,
       t->sun_gc_lat); */

    /* the sun position has to be translated just like everything else */
    t->sun_vec[0] = t->fg_sunpos.x - scenery_center.x; 
    t->sun_vec[1] = t->fg_sunpos.y - scenery_center.y;
    t->sun_vec[2] = t->fg_sunpos.z - scenery_center.z;
    /* make this a directional light source only */
    t->sun_vec[3] = 0.0;

    /* calculate thesun's relative angle to local up */
    MAT3_COPY_VEC(nup, v->local_up);
    nsun[0] = t->fg_sunpos.x; 
    nsun[1] = t->fg_sunpos.y;
    nsun[2] = t->fg_sunpos.z;
    MAT3_NORMALIZE_VEC(nup, temp);
    MAT3_NORMALIZE_VEC(nsun, temp);

    t->sun_angle = acos(MAT3_DOT_PRODUCT(nup, nsun));
    printf("SUN ANGLE relative to current location = %.3f rads.\n", 
	   t->sun_angle);
}


/* $Log$
/* Revision 1.8  1997/09/05 01:36:04  curt
/* Working on getting stars right.
/*
 * Revision 1.7  1997/09/04 02:17:40  curt
 * Shufflin' stuff.
 *
 * Revision 1.6  1997/08/27 03:30:37  curt
 * Changed naming scheme of basic shared structures.
 *
 * Revision 1.5  1997/08/22 21:34:41  curt
 * Doing a bit of reorganizing and house cleaning.
 *
 * Revision 1.4  1997/08/19 23:55:09  curt
 * Worked on better simulating real lighting.
 *
 * Revision 1.3  1997/08/13 20:23:49  curt
 * The interface to sunpos now updates a global structure rather than returning
 * current sun position.
 *
 * Revision 1.2  1997/08/06 00:24:32  curt
 * Working on correct real time sun lighting.
 *
 * Revision 1.1  1997/08/01 15:27:56  curt
 * Initial revision.
 *
 */
