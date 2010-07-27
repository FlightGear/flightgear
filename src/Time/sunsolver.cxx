/*
 * sunsolver.cxx - given a location on earth and a time of day/date,
 *                 find the number of seconds to various sun positions.
 *
 * Written by Curtis Olson, started September 2003.
 *
 * Copyright (C) 2003  Curtis L. Olson  - http://www.flightgear.org/~curt
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#      include <config.h>
#endif

#include <cmath>
#include <ctime>
#include <cassert>

#include <simgear/math/SGMath.hxx>
#include <simgear/timing/sg_time.hxx>

#include <Main/globals.hxx>
#include <Main/fg_props.hxx>

#include "tmp.hxx"
#include "sunsolver.hxx"


static const time_t day_secs = 86400;
static const time_t half_day_secs = day_secs / 2;
static const time_t step_secs = 60;

/* given a particular time expressed in side real time at prime
 * meridian (GST), compute position on the earth (lat, lon) such that
 * sun is directly overhead.  (lat, lon are reported in radians */

void fgSunPositionGST(double gst, double *lon, double *lat) {
    /* time_t  ssue;           seconds since unix epoch */
    /* double *lat;            (return) latitude        */
    /* double *lon;            (return) longitude       */

    double alpha, delta;
    double tmp;

    SGPropertyNode* sun = fgGetNode("/ephemeris/sun");
    assert(sun);
    double beta = sun->getDoubleValue("lat-deg");
    // double r = globals->get_ephem()->get_sun()->getDistance();
    double xs = sun->getDoubleValue("xs");
    double ys = sun->getDoubleValue("ys");
    double ye = sun->getDoubleValue("ye");
    double ze = sun->getDoubleValue("ze");
    alpha = atan2(ys - tan(beta)*ze/ys, xs);
    delta = asin(sin(beta)*ye/ys + cos(beta)*ze);

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

static double sun_angle( const SGTime &t, const SGVec3d& world_up,
                         double lon_rad, double lat_rad ) {
    SG_LOG( SG_EVENT, SG_DEBUG, "  Updating Sun position" );
    SG_LOG( SG_EVENT, SG_DEBUG, "  Gst = " << t.getGst() );

    double sun_lon, sun_gd_lat;
    fgSunPositionGST( t.getGst(), &sun_lon, &sun_gd_lat );
    SGVec3d sunpos = SGVec3d::fromGeod(SGGeod::fromRad(sun_lon, sun_gd_lat));

    SG_LOG( SG_EVENT, SG_DEBUG, "    t.cur_time = " << t.get_cur_time() );
    SG_LOG( SG_EVENT, SG_DEBUG, 
	    "    Sun Geodetic lat = " << sun_gd_lat );

    // calculate the sun's relative angle to local up
    SGVec3f nup = normalize(toVec3f(world_up));
    SGVec3f nsun = normalize(toVec3f(sunpos));
    // cout << "nup = " << nup[0] << "," << nup[1] << "," 
    //      << nup[2] << endl;
    // cout << "nsun = " << nsun[0] << "," << nsun[1] << "," 
    //      << nsun[2] << endl;

    double sun_angle = acos( dot( nup, nsun ) );
    double sun_angle_deg = sun_angle * SG_RADIANS_TO_DEGREES;
    while ( sun_angle_deg < -180 ) { sun_angle += 360; }
    SG_LOG( SG_EVENT, SG_DEBUG, "sun angle relative to current location = "
	    << sun_angle_deg );

    return sun_angle_deg;
}


/**
 * Given the current unix time in seconds, calculate seconds to the
 * specified sun angle (relative to straight up.)  Also specify if we
 * want the angle while the sun is ascending or descending.  For
 * instance noon is when the sun angle is 0 (or the closest it can
 * get.)  Dusk is when the sun angle is 90 and descending.  Dawn is
 * when the sun angle is 90 and ascending.
 */
time_t fgTimeSecondsUntilSunAngle( time_t cur_time,
                                   double lon_rad,
                                   double lat_rad,
                                   double target_angle_deg,
                                   bool ascending )
{
    // cout << "location = " << lon_rad * SG_RADIANS_TO_DEGREES << ", "
    //      << lat_rad * SG_RADIANS_TO_DEGREES << endl;
    SGVec3d world_up = SGVec3d::fromGeod(SGGeod::fromRad(lon_rad, lat_rad));
    SGTime t = SGTime( lon_rad, lat_rad, "", 0 );

    double best_diff = 180.0;
    double last_angle = -99999.0;
    time_t best_time = cur_time;

    for ( time_t secs = cur_time - half_day_secs;
          secs < cur_time + half_day_secs;
          secs += step_secs )
    {
        t.update( lon_rad, lat_rad, secs, 0 );
        double angle_deg = sun_angle( t, world_up, lon_rad, lat_rad );
        double diff = fabs( angle_deg - target_angle_deg );
        if ( diff < best_diff ) {
            if ( last_angle <= 180.0 && ascending
                 && ( last_angle > angle_deg ) ) {
                // cout << "best angle = " << angle << " offset = "
                //      << secs - cur_time << endl;
                best_diff = diff;
                best_time = secs;
            } else if ( last_angle <= 180.0 && !ascending
                        && ( last_angle < angle_deg ) ) {
                // cout << "best angle = " << angle << " offset = "
                //      << secs - cur_time << endl;
                best_diff = diff;
                best_time = secs;
            }
        }

        last_angle = angle_deg;
    }

    return best_time - cur_time;
}
