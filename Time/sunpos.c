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
#include "../Include/constants.h"
#include "../Main/views.h"
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
    struct fgLIGHT *l;
    struct fgTIME *t;
    struct fgVIEW *v;
    MAT3vec nup, nsun;
    double sun_gd_lat, sl_radius, temp;
    static int time_warp = 0;

    l = &cur_light_params;
    t = &cur_time_params;
    v = &current_view;

    time_warp += 0; /* increase this to make the world spin real fast */

    fgSunPosition(t->cur_time + time_warp, &l->sun_lon, &sun_gd_lat);

    fgGeodToGeoc(sun_gd_lat, 0.0, &sl_radius, &l->sun_gc_lat);

    l->fg_sunpos = fgPolarToCart(l->sun_lon, l->sun_gc_lat, sl_radius);

    /* printf("Geodetic lat = %.5f Geocentric lat = %.5f\n", sun_gd_lat,
       t->sun_gc_lat); */

    /* FALSE! (?> the sun position has to be translated just like
     * everything else */
    /* l->sun_vec_inv[0] = l->fg_sunpos.x - scenery_center.x;  */
    /* l->sun_vec_inv[1] = l->fg_sunpos.y - scenery_center.y; */
    /* l->sun_vec_inv[2] = l->fg_sunpos.z - scenery_center.z; */
    /* MAT3_SCALE_VEC(l->sun_vec, l->sun_vec_inv, -1.0); */

    /* I think this will work better for generating the sun light vector */
    l->sun_vec[0] = l->fg_sunpos.x;
    l->sun_vec[1] = l->fg_sunpos.y;
    l->sun_vec[2] = l->fg_sunpos.z;
    MAT3_NORMALIZE_VEC(l->sun_vec, temp);
    MAT3_SCALE_VEC(l->sun_vec_inv, l->sun_vec, -1.0);

    /* make these are directional light sources only */
    l->sun_vec[3] = 0.0;
    l->sun_vec_inv[3] = 0.0;

    /* calculate thesun's relative angle to local up */
    MAT3_COPY_VEC(nup, v->local_up);
    nsun[0] = l->fg_sunpos.x; 
    nsun[1] = l->fg_sunpos.y;
    nsun[2] = l->fg_sunpos.z;
    MAT3_NORMALIZE_VEC(nup, temp);
    MAT3_NORMALIZE_VEC(nsun, temp);

    l->sun_angle = acos(MAT3_DOT_PRODUCT(nup, nsun));
    printf("SUN ANGLE relative to current location = %.3f rads.\n", 
	   l->sun_angle);
}


/* $Log$
/* Revision 1.17  1997/12/15 23:55:08  curt
/* Add xgl wrappers for debugging.
/* Generate terrain normals on the fly.
/*
 * Revision 1.16  1997/12/11 04:43:57  curt
 * Fixed sun vector and lighting problems.  I thing the moon is now lit
 * correctly.
 *
 * Revision 1.15  1997/12/10 22:37:55  curt
 * Prepended "fg" on the name of all global structures that didn't have it yet.
 * i.e. "struct WEATHER {}" became "struct fgWEATHER {}"
 *
 * Revision 1.14  1997/12/09 04:25:39  curt
 * Working on adding a global lighting params structure.
 *
 * Revision 1.13  1997/11/25 19:25:42  curt
 * Changes to integrate Durk's moon/sun code updates + clean up.
 *
 * Revision 1.12  1997/11/15 18:15:39  curt
 * Reverse direction of sun vector, so object normals can be more normal.
 *
 * Revision 1.11  1997/10/28 21:07:21  curt
 * Changed GLUT/ -> Main/
 *
 * Revision 1.10  1997/09/13 02:00:09  curt
 * Mostly working on stars and generating sidereal time for accurate star
 * placement.
 *
 * Revision 1.9  1997/09/05 14:17:31  curt
 * More tweaking with stars.
 *
 * Revision 1.8  1997/09/05 01:36:04  curt
 * Working on getting stars right.
 *
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
