// moonpos.cxx (basically, this is a slightly modified version of the 'sunpos.cxx' file, adapted from XEarth)

// kirk johnson
// july 1993
//
// code for calculating the position on the earth's surface for which
// the moon is directly overhead (adapted from _practical astronomy
// with your calculator, third edition_, peter duffett-smith,
// cambridge university press, 1988.)
//
// Copyright (C) 1989, 1990, 1993, 1994, 1995 Kirk Lauritz Johnson
//
// Parts of the source code (as marked) are:
//   Copyright (C) 1989, 1990, 1991 by Jim Frost
//   Copyright (C) 1992 by Jamie Zawinski <jwz@lucid.com>
//
// Permission to use, copy, modify and freely distribute xearth for
// non-commercial and not-for-profit purposes is hereby granted
// without fee, provided that both the above copyright notice and this
// permission notice appear in all copies and in supporting
// documentation.
//
// The author makes no representations about the suitability of this
// software for any purpose. It is provided "as is" without express or
// implied warranty.
//
// THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
// INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
// IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, INDIRECT
// OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
// LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
// NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
// CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
//
// $Id$


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#ifdef FG_HAVE_STD_INCLUDES
#  include <cmath>
#  include <cstdio>
#  include <ctime>
#else
#  include <math.h>
#  include <stdio.h>
#  include <time.h>
#endif

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/ephemeris/ephemeris.hxx>
#include <simgear/math/fg_geodesy.hxx>
#include <simgear/math/point3d.hxx>
#include <simgear/math/polar3d.hxx>
#include <simgear/math/vector.hxx>

#include <Main/globals.hxx>
#include <Main/views.hxx>
#include <Scenery/scenery.hxx>

#include "moonpos.hxx"

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
 * assuming the apparent orbit of the moon about the earth is circular,
 * the rate at which the orbit progresses is given by RadsPerDay --
 * FG_2PI radians per orbit divided by 365.242191 days per year:
 */

#define RadsPerDay (FG_2PI/365.242191)

/*
 * details of moon's apparent orbit at epoch 1990.0 (after
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

/* static double solve_keplers_equation(double); */
/* static double moon_ecliptic_longitude(time_t); */
static void   ecliptic_to_equatorial(double, double, double *, double *);
static double julian_date(int, int, int);
static double GST(time_t);

/*
 * solve Kepler's equation via Newton's method
 * (after duffett-smith, section 47)
 */
/*
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
*/


/* compute ecliptic longitude of moon (in radians) (after
 * duffett-smith, section 47) */
/*
static double moon_ecliptic_longitude(time_t ssue) {
    // time_t ssue;              //  seconds since unix epoch
    double D, N;
    double M_moon, E;
    double v;

    D = DaysSinceEpoch(ssue);

    N = RadsPerDay * D;
    N = fmod(N, FG_2PI);
    if (N < 0) N += FG_2PI;

    M_moon = N + Epsilon_g - OmegaBar_g;
    if (M_moon < 0) M_moon += FG_2PI;

    E = solve_keplers_equation(M_moon);
    v = 2 * atan(sqrt((1+Eccentricity)/(1-Eccentricity)) * tan(E/2));

    return (v + OmegaBar_g);
}
*/


/* convert from ecliptic to equatorial coordinates (after
 * duffett-smith, section 27) */

static void ecliptic_to_equatorial(double lambda, double beta, 
				   double *alpha, double *delta) {
    /* double  lambda;            ecliptic longitude       */
    /* double  beta;              ecliptic latitude        */
    /* double *alpha;             (return) right ascension */
    /* double *delta;             (return) declination     */

    double sin_e, cos_e;
    double sin_l, cos_l;

    sin_e = sin(MeanObliquity);
    cos_e = cos(MeanObliquity);
    sin_l = sin(lambda);
    cos_l = cos(lambda);

    *alpha = atan2(sin_l*cos_e - tan(beta)*sin_e, cos_l);
    *delta = asin(sin(beta)*cos_e + cos(beta)*sin_e*sin_l);
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
	FG_LOG( FG_EVENT, FG_ALERT, 
		"WHOOPS! Julian dates only valid for 1582 oct 15 or later" );
    }

    if ((m == 1) || (m == 2)) {
	y -= 1;
	m += 12;
    }

    A = y / 100;
    B = 2 - A + (A / 4);
    C = (int)(365.25 * y);
    D = (int)(30.6001 * (m + 1));

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
 * epoch), compute position on the earth (lat, lon) such that moon is
 * directly overhead.  (lat, lon are reported in radians */

void fgMoonPosition(time_t ssue, double *lon, double *lat) {
    /* time_t  ssue;           seconds since unix epoch */
    /* double *lat;            (return) latitude        */
    /* double *lon;            (return) longitude       */

    /* double lambda; */
    double alpha, delta;
    double tmp;

    /* lambda = moon_ecliptic_longitude(ssue); */
    /* ecliptic_to_equatorial(lambda, 0.0, &alpha, &delta); */
    //ecliptic_to_equatorial (solarPosition.lonMoon, 0.0, &alpha, &delta);
    
    /* ********************************************************************** 
     * NOTE: in the next function, each time the moon's position is updated, the
     * the moon's longitude is returned from solarSystem->moon. Note that the 
     * moon's position is updated at a much higher frequency than the rate at 
     * which the solar system's rebuilds occur. This is not a problem, however,
     * because the fgMoonPosition we're talking about here concerns the changing
     * position of the moon due to the daily rotation of the earth.
     * The ecliptic longitude, however, represents the position of the moon with
     * respect to the stars, and completes just one cycle over the course of a 
     * year. Its therefore pretty safe to update the moon's longitude only once
     * every ten minutes. (Comment added by Durk Talsma).
     ************************************************************************/

    ecliptic_to_equatorial( globals->get_ephem()->get_moon()->getLon(),
			    0.0, &alpha, &delta );
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


/* given a particular time expressed in side real time at prime
 * meridian (GST), compute position on the earth (lat, lon) such that
 * moon is directly overhead.  (lat, lon are reported in radians */

static void fgMoonPositionGST(double gst, double *lon, double *lat) {
    /* time_t  ssue;           seconds since unix epoch */
    /* double *lat;            (return) latitude        */
    /* double *lon;            (return) longitude       */

    /* double lambda; */
    double alpha, delta;
    double tmp;

    /* lambda = moon_ecliptic_longitude(ssue); */
    /* ecliptic_to_equatorial(lambda, 0.0, &alpha, &delta); */
    //ecliptic_to_equatorial (solarPosition.lonMoon, 0.0, &alpha, &delta);
    ecliptic_to_equatorial( globals->get_ephem()->get_moon()->getLon(),
			    globals->get_ephem()->get_moon()->getLat(), 
			    &alpha,  &delta );

//    tmp = alpha - (FG_2PI/24)*GST(ssue);
    tmp = alpha - (FG_2PI/24)*gst;	
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


// update the cur_time_params structure with the current moon position
void fgUpdateMoonPos( void ) {
    fgLIGHT *l;
    FGView *v;
    sgVec3 nup, nmoon, v0, surface_to_moon;
    Point3D p, rel_moonpos;
    double dot, east_dot;
    double moon_gd_lat, sl_radius;

    l = &cur_light_params;
    SGTime *t = globals->get_time_params();
    v = &current_view;

    FG_LOG( FG_EVENT, FG_INFO, "  Updating Moon position" );

    // (not sure why there was two)
    // fgMoonPosition(t->cur_time, &l->moon_lon, &moon_gd_lat);
    fgMoonPositionGST(t->getGst(), &l->moon_lon, &moon_gd_lat);

    fgGeodToGeoc(moon_gd_lat, 0.0, &sl_radius, &l->moon_gc_lat);

    p = Point3D( l->moon_lon, l->moon_gc_lat, sl_radius );
    l->fg_moonpos = fgPolarToCart3d(p);

    FG_LOG( FG_EVENT, FG_INFO, "    t->cur_time = " << t->get_cur_time() );
    FG_LOG( FG_EVENT, FG_INFO, 
	    "    Moon Geodetic lat = " << moon_gd_lat
	    << " Geocentric lat = " << l->moon_gc_lat );

    // update the sun light vector
    sgSetVec4( l->moon_vec,
	       l->fg_moonpos.x(), l->fg_moonpos.y(), l->fg_moonpos.z(), 0.0 );
    sgNormalizeVec4( l->moon_vec );
    sgCopyVec4( l->moon_vec_inv, l->moon_vec );
    sgNegateVec4( l->moon_vec_inv );

    // make sure these are directional light sources only
    l->moon_vec[3] = l->moon_vec_inv[3] = 0.0;
    // cout << "  l->moon_vec = " << l->moon_vec[0] << "," << l->moon_vec[1]
    //      << ","<< l->moon_vec[2] << endl;

    // calculate the moon's relative angle to local up
    sgCopyVec3( nup, v->get_local_up() );
    sgSetVec3( nmoon, l->fg_moonpos.x(), l->fg_moonpos.y(), l->fg_moonpos.z() );
    sgNormalizeVec3(nup);
    sgNormalizeVec3(nmoon);
    // cout << "nup = " << nup[0] << "," << nup[1] << "," 
    //      << nup[2] << endl;
    // cout << "nmoon = " << nmoon[0] << "," << nmoon[1] << "," 
    //      << nmoon[2] << endl;

    l->moon_angle = acos( sgScalarProductVec3( nup, nmoon ) );
    cout << "moon angle relative to current location = " 
	 << l->moon_angle << endl;
    
    // calculate vector to moon's position on the earth's surface
    rel_moonpos = l->fg_moonpos - (v->get_view_pos() + scenery.center);
    v->set_to_moon( rel_moonpos.x(), rel_moonpos.y(), rel_moonpos.z() );
    // printf( "Vector to moon = %.2f %.2f %.2f\n",
    //         v->to_moon[0], v->to_moon[1], v->to_moon[2]);

    // make a vector to the current view position
    Point3D view_pos = v->get_view_pos();
    sgSetVec3( v0, view_pos.x(), view_pos.y(), view_pos.z() );

    // Given a vector from the view position to the point on the
    // earth's surface the moon is directly over, map into onto the
    // local plane representing "horizontal".

    sgmap_vec_onto_cur_surface_plane( v->get_local_up(), v0, 
				      v->get_to_moon(), surface_to_moon );
    sgNormalizeVec3(surface_to_moon);
    v->set_surface_to_moon( surface_to_moon[0], surface_to_moon[1], 
			    surface_to_moon[2] );
    // cout << "(sg) Surface direction to moon is "
    //   << surface_to_moon[0] << "," 
    //   << surface_to_moon[1] << ","
    //   << surface_to_moon[2] << endl;
    // cout << "Should be close to zero = " 
    //   << sgScalarProductVec3(nup, surface_to_moon) << endl;

    // calculate the angle between v->surface_to_moon and
    // v->surface_east.  We do this so we can sort out the acos()
    // ambiguity.  I wish I could think of a more efficient way ... :-(
    east_dot = sgScalarProductVec3( surface_to_moon, v->get_surface_east() );
   // cout << "  East dot product = " << east_dot << endl;

    // calculate the angle between v->surface_to_moon and
    // v->surface_south.  this is how much we have to rotate the sky
    // for it to align with the moon
    dot = sgScalarProductVec3( surface_to_moon, v->get_surface_south() );
    // cout << "  Dot product = " << dot << endl;

    if ( east_dot >= 0 ) {
	l->moon_rotation = acos(dot);
    } else {
	l->moon_rotation = -acos(dot);
    }
    // cout << "  Sky needs to rotate = " << angle << " rads = "
    //      << angle * RAD_TO_DEG << " degrees." << endl;
}


