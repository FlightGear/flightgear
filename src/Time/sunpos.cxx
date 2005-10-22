// sunpos.cxx (adapted from XEarth)
// kirk johnson
// july 1993
//
// code for calculating the position on the earth's surface for which
// the sun is directly overhead (adapted from _practical astronomy
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

#ifdef SG_HAVE_STD_INCLUDES
#  include <cmath>
#  include <cstdio>
#  include <ctime>
#  ifdef macintosh
     SG_USING_STD(time_t);
#  endif
#else
#  include <math.h>
#  include <stdio.h>
#  include <time.h>
#endif

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/ephemeris/ephemeris.hxx>
#include <simgear/math/point3d.hxx>
#include <simgear/math/polar3d.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/math/vector.hxx>
#include <simgear/timing/sg_time.hxx>

#include <Main/globals.hxx>
#include <Main/viewer.hxx>
#include <Scenery/scenery.hxx>
#include <Time/light.hxx>

#include "sunpos.hxx"

// #undef E // should no longer be needed
#define MeanObliquity (23.440592*(SGD_2PI/360))

static void   ecliptic_to_equatorial(double, double, double *, double *);
static double julian_date(int, int, int);
static double GST(time_t);

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
	SG_LOG( SG_EVENT, SG_ALERT, 
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
 * epoch), compute position on the earth (lat, lon) such that sun is
 * directly overhead.  (lat, lon are reported in radians */

void fgSunPosition(time_t ssue, double *lon, double *lat) {
    /* time_t  ssue;           seconds since unix epoch */
    /* double *lat;            (return) latitude        */
    /* double *lon;            (return) longitude       */

    /* double lambda; */
    double alpha, delta;
    double tmp;

    /* lambda = sun_ecliptic_longitude(ssue); */
    /* ecliptic_to_equatorial(lambda, 0.0, &alpha, &delta); */
    //ecliptic_to_equatorial (solarPosition.lonSun, 0.0, &alpha, &delta);
    
    /* ********************************************************************** 
     * NOTE: in the next function, each time the sun's position is updated, the
     * the sun's longitude is returned from solarSystem->sun. Note that the 
     * sun's position is updated at a much higher frequency than the rate at 
     * which the solar system's rebuilds occur. This is not a problem, however,
     * because the fgSunPosition we're talking about here concerns the changing
     * position of the sun due to the daily rotation of the earth.
     * The ecliptic longitude, however, represents the position of the sun with
     * respect to the stars, and completes just one cycle over the course of a 
     * year. Its therefore pretty safe to update the sun's longitude only once
     * every ten minutes. (Comment added by Durk Talsma).
     ************************************************************************/

    ecliptic_to_equatorial( globals->get_ephem()->get_sun()->getLon(),
			    0.0, &alpha, &delta );
    tmp = alpha - (SGD_2PI/24)*GST(ssue);
    if (tmp < -SGD_PI) {
	do tmp += SGD_2PI;
	while (tmp < -SGD_PI);
    } else if (tmp > SGD_PI) {
	do tmp -= SGD_2PI;
	while (tmp < -SGD_PI);
    }

    *lon = tmp;
    *lat = delta;
}


/* given a particular time expressed in side real time at prime
 * meridian (GST), compute position on the earth (lat, lon) such that
 * sun is directly overhead.  (lat, lon are reported in radians */

void fgSunPositionGST(double gst, double *lon, double *lat) {
    /* time_t  ssue;           seconds since unix epoch */
    /* double *lat;            (return) latitude        */
    /* double *lon;            (return) longitude       */

    /* double lambda; */
    double alpha, delta;
    double tmp;

    /* lambda = sun_ecliptic_longitude(ssue); */
    /* ecliptic_to_equatorial(lambda, 0.0, &alpha, &delta); */
    //ecliptic_to_equatorial (solarPosition.lonSun, 0.0, &alpha, &delta);
    ecliptic_to_equatorial( globals->get_ephem()->get_sun()->getLon(),
			    globals->get_ephem()->get_sun()->getLat(),
			    &alpha, &delta );

    // tmp = alpha - (SGD_2PI/24)*GST(ssue);
    tmp = alpha - (SGD_2PI/24)*gst;	
    if (tmp < -SGD_PI) {
	do tmp += SGD_2PI;
	while (tmp < -SGD_PI);
    } else if (tmp > SGD_PI) {
	do tmp -= SGD_2PI;
	while (tmp < -SGD_PI);
    }

    *lon = tmp;
    *lat = delta;
}


// update the cur_time_params structure with the current sun position
void fgUpdateSunPos( void ) {
    sgVec3 nup, nsun;
    Point3D rel_sunpos;
    double dot, east_dot;
    double sun_gd_lat, sl_radius;

    // vector in cartesian coordinates from current position to the
    // postion on the earth's surface the sun is directly over
    sgVec3 to_sun;

    // surface direction to go to head towards sun
    sgVec3 surface_to_sun;

    FGLight *l = (FGLight *)(globals->get_subsystem("lighting"));
    SGTime *t = globals->get_time_params();
    FGViewer *v = globals->get_current_view();

    SG_LOG( SG_EVENT, SG_DEBUG, "  Updating Sun position" );
    SG_LOG( SG_EVENT, SG_DEBUG, "  Gst = " << t->getGst() );

    double sun_l;
    fgSunPositionGST(t->getGst(), &sun_l, &sun_gd_lat);
    l->set_sun_lon(sun_l);

    sgGeodToGeoc(sun_gd_lat, 0.0, &sl_radius, &sun_l);
    l->set_sun_gc_lat(sun_l);

    Point3D p = Point3D( l->get_sun_lon(), l->get_sun_gc_lat(), sl_radius );
    l->set_sunpos( sgPolarToCart3d(p) );

    SG_LOG( SG_EVENT, SG_DEBUG, "    t->cur_time = " << t->get_cur_time() );
    SG_LOG( SG_EVENT, SG_DEBUG,
	    "    Sun Geodetic lat = " << sun_gd_lat
	    << " Geocentric lat = " << l->get_sun_gc_lat() );

    // update the sun light vector
    sgSetVec4( l->sun_vec(), l->get_sunpos().x(),
	       l->get_sunpos().y(), l->get_sunpos().z(), 0.0 );
    sgNormalizeVec4( l->sun_vec() );
    sgCopyVec4( l->sun_vec_inv(), l->sun_vec() );
    sgNegateVec4( l->sun_vec_inv() );

    // make sure these are directional light sources only
    l->sun_vec()[3] = l->sun_vec_inv()[3] = 0.0;
    // cout << "  l->sun_vec = " << l->sun_vec[0] << "," << l->sun_vec[1]
    //      << ","<< l->sun_vec[2] << endl;

    // calculate the sun's relative angle to local up
    sgCopyVec3( nup, v->get_world_up() );
    sgSetVec3( nsun, l->get_sunpos().x(),
               l->get_sunpos().y(), l->get_sunpos().z() );
    sgNormalizeVec3(nup);
    sgNormalizeVec3(nsun);
    // cout << "nup = " << nup[0] << "," << nup[1] << "," 
    //      << nup[2] << endl;
    // cout << "nsun = " << nsun[0] << "," << nsun[1] << "," 
    //      << nsun[2] << endl;

    l->set_sun_angle( acos( sgScalarProductVec3 ( nup, nsun ) ) );
    SG_LOG( SG_EVENT, SG_DEBUG, "sun angle relative to current location = "
	    << l->get_sun_angle() );
    
    // calculate vector to sun's position on the earth's surface
    Point3D vp( v->get_view_pos()[0],
		v->get_view_pos()[1],
		v->get_view_pos()[2] );
    rel_sunpos = l->get_sunpos() - (vp + globals->get_scenery()->get_center());
    sgSetVec3( to_sun, rel_sunpos.x(), rel_sunpos.y(), rel_sunpos.z() );
    // printf( "Vector to sun = %.2f %.2f %.2f\n",
    //         v->to_sun[0], v->to_sun[1], v->to_sun[2]);

    // Given a vector from the view position to the point on the
    // earth's surface the sun is directly over, map into onto the
    // local plane representing "horizontal".

    sgmap_vec_onto_cur_surface_plane( v->get_world_up(), v->get_view_pos(),
				      to_sun, surface_to_sun );
    sgNormalizeVec3(surface_to_sun);
    // cout << "(sg) Surface direction to sun is "
    //   << surface_to_sun[0] << "," 
    //   << surface_to_sun[1] << ","
    //   << surface_to_sun[2] << endl;
    // cout << "Should be close to zero = " 
    //   << sgScalarProductVec3(nup, surface_to_sun) << endl;

    // calculate the angle between surface_to_sun and
    // v->get_surface_east().  We do this so we can sort out the
    // acos() ambiguity.  I wish I could think of a more efficient
    // way. :-(
    east_dot = sgScalarProductVec3( surface_to_sun, v->get_surface_east() );
    // cout << "  East dot product = " << east_dot << endl;

    // calculate the angle between v->surface_to_sun and
    // v->surface_south.  this is how much we have to rotate the sky
    // for it to align with the sun
    dot = sgScalarProductVec3( surface_to_sun, v->get_surface_south() );
    // cout << "  Dot product = " << dot << endl;

    if (dot > 1.0) {
        SG_LOG( SG_ASTRO, SG_INFO,
                "Dot product  = " << dot << " is greater than 1.0" );
        dot = 1.0;
    }
    else if (dot < -1.0) {
         SG_LOG( SG_ASTRO, SG_INFO,
                 "Dot product  = " << dot << " is less than -1.0" );
         dot = -1.0;
     }

    if ( east_dot >= 0 ) {
	l->set_sun_rotation( acos(dot) );
    } else {
	l->set_sun_rotation( -acos(dot) );
    }
    // cout << "  Sky needs to rotate = " << angle << " rads = "
    //      << angle * SGD_RADIANS_TO_DEGREES << " degrees." << endl;
}


