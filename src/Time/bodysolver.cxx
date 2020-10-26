/*
 * bodysolver.cxx - given a location on earth and a time of day/date,
 *                  find the number of seconds to various solar system body
 *                  positions.
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

#include "bodysolver.hxx"


static const time_t day_secs = 86400;
static const time_t half_day_secs = day_secs / 2;
static const time_t step_secs = 60;

/* given a particular time expressed in side real time at prime
 * meridian (GST), compute position on the earth (lat, lon) such that
 * solar system body is directly overhead.  (lat, lon are reported in
 * radians) */

void fgBodyPositionGST(double gst, double& lon, double& lat, bool sun_not_moon) {
    /* time_t  ssue;           seconds since unix epoch */
    /* double& lat;            (return) latitude        */
    /* double& lon;            (return) longitude       */

    double tmp;

    std::string body = sun_not_moon ? "sun" : "moon";
    SGPropertyNode* body_node = fgGetNode("/ephemeris/" + body);
    assert(body_node);
    double xs = sun_not_moon ? body_node->getDoubleValue("xs")
                             : body_node->getDoubleValue("xg");
    //double ys = body_node->getDoubleValue("ys");
    double ye = body_node->getDoubleValue("ye");
    double ze = body_node->getDoubleValue("ze");
    double ra = atan2(ye, xs);
    double dec = atan2(ze, sqrt(xs * xs + ye * ye));

    tmp = ra - (SGD_2PI/24)*gst;
    
    double signedPI = (tmp < 0.0) ? -SGD_PI : SGD_PI;
    tmp = fmod(tmp+signedPI, SGD_2PI) - signedPI;

    lon = tmp;
    lat = dec;
}

static double body_angle( const SGTime &t, const SGVec3d& world_up, bool sun_not_moon) {
    const char *body = sun_not_moon ? "sun" : "moon";
    SG_LOG( SG_EVENT, SG_DEBUG, "  Updating " << body << " position" );
    SG_LOG( SG_EVENT, SG_DEBUG, "  Gst = " << t.getGst() );

    double lon, gc_lat;
    fgBodyPositionGST( t.getGst(), lon, gc_lat, body );
    SGVec3d bodypos = SGVec3d::fromGeoc(SGGeoc::fromRadM(lon, gc_lat,
                                                        SGGeodesy::EQURAD));

    SG_LOG( SG_EVENT, SG_DEBUG, "    t.cur_time = " << t.get_cur_time() );
    SG_LOG( SG_EVENT, SG_DEBUG, 
	    "    " << body << " geocentric lat = " << gc_lat );

    // calculate the body's relative angle to local up
    SGVec3d nup = normalize(world_up);
    SGVec3d nbody = normalize(bodypos);
    // cout << "nup = " << nup[0] << "," << nup[1] << "," 
    //      << nup[2] << endl;
    // cout << "nbody = " << nbody[0] << "," << nbody[1] << ","
    //      << nbody[2] << endl;

    double body_angle = acos( dot( nup, nbody ) );

    double signedPI = (body_angle < 0.0) ? -SGD_PI : SGD_PI;
    body_angle = fmod(body_angle+signedPI, SGD_2PI) - signedPI;

    double body_angle_deg = body_angle * SG_RADIANS_TO_DEGREES;
    SG_LOG( SG_EVENT, SG_DEBUG, body << " angle relative to current location = "
	    << body_angle_deg );

    return body_angle_deg;
}


/**
 * Given the current unix time in seconds, calculate seconds to the
 * specified body angle (relative to straight up.)  Also specify if we
 * want the angle while the body is ascending or descending.  For
 * instance noon is when the sun angle is 0 (or the closest it can
 * get.)  Dusk is when the sun angle is 90 and descending.  Dawn is
 * when the sun angle is 90 and ascending.
 */
time_t fgTimeSecondsUntilBodyAngle( time_t cur_time,
                                   const SGGeod& loc,
                                   double target_angle_deg,
                                   bool ascending,
                                   bool sun_not_moon )
{
    SGVec3d world_up = SGVec3d::fromGeod(loc);
    SGTime t = SGTime( loc, SGPath(), 0 );

    double best_diff = 180.0;
    double last_angle = -99999.0;
    time_t best_time = cur_time;

    for ( time_t secs = cur_time - half_day_secs;
          secs < cur_time + half_day_secs;
          secs += step_secs )
    {
        t.update( loc, secs, 0 );
        double angle_deg = body_angle( t, world_up, sun_not_moon );
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
