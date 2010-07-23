// tmp.cxx -- stuff I don't know what to do with at the moment
//
// Written by Curtis Olson, started July 2000.
//
// Copyright (C) 2000  Curtis L. Olson  - http://www.flightgear.org/~curt
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/math/SGMath.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/timing/sg_time.hxx>

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Main/viewer.hxx>
#include <Scenery/scenery.hxx>

#include "light.hxx"
#include "sunsolver.hxx"
#include "tmp.hxx"

/**
 * Map i.e. project a vector onto a plane.
 * @param normal (in) normal vector for the plane
 * @param v0 (in) a point on the plane
 * @param vec (in) the vector to map onto the plane
 */
static SGVec3f map_vec_onto_cur_surface_plane(const SGVec3f& normal, 
					      const SGVec3f& v0, 
					      const SGVec3f& vec)
{
    // calculate a vector "u1" representing the shortest distance from
    // the plane specified by normal and v0 to a point specified by
    // "vec".  "u1" represents both the direction and magnitude of
    // this desired distance.

    // u1 = ( (normal <dot> vec) / (normal <dot> normal) ) * normal
    SGVec3f u1 = (dot(normal, vec) / dot(normal, normal)) * normal;

    // calculate the vector "v" which is the vector "vec" mapped onto
    // the plane specified by "normal" and "v0".

    // v = v0 + vec - u1
    SGVec3f v = v0 + vec - u1;
    
    // Calculate the vector "result" which is "v" - "v0" which is a
    // directional vector pointing from v0 towards v

    // result = v - v0
    return v - v0;
}


// periodic time updater wrapper
void fgUpdateLocalTime() {
    static const SGPropertyNode *longitude
	= fgGetNode("/position/longitude-deg");
    static const SGPropertyNode *latitude
	= fgGetNode("/position/latitude-deg");

    SGPath zone( globals->get_fg_root() );
    zone.append( "Timezone" );

    SG_LOG(SG_GENERAL, SG_INFO, "updateLocal("
	   << longitude->getDoubleValue() * SGD_DEGREES_TO_RADIANS
	   << ", "
	   << latitude->getDoubleValue() * SGD_DEGREES_TO_RADIANS
	   << ", " << zone.str() << ")");
    globals->get_time_params()->updateLocal( longitude->getDoubleValue()
					       * SGD_DEGREES_TO_RADIANS, 
					     latitude->getDoubleValue()
					       * SGD_DEGREES_TO_RADIANS,
					     zone.str() );
}


// update the cur_time_params structure with the current sun position
void fgUpdateSunPos( void ) {
#if 0
    // This only works at lat,lon = 0,0
    // need to find a way to get it working at other locations

    FGLight *light = (FGLight *)(globals->get_subsystem("lighting"));
    FGViewer *viewer = globals->get_current_view();
    SGTime *time_now = globals->get_time_params();

    SG_LOG( SG_EVENT, SG_DEBUG, "  Updating Sun position" );
    SG_LOG( SG_EVENT, SG_DEBUG, "  Gst = " << time_now->getGst() );

    double sun_lon, sun_lat;
    fgSunPositionGST(time_now->getGst(), &sun_lon, &sun_lat);
    light->set_sun_lon(sun_lon);
    light->set_sun_lat(sun_lat);

    // update the sun light vector
    // calculations are in the horizontal normal plane: x-north, y-east, z-down
    static SGQuatd q = SGQuatd::fromLonLat(SGGeod::fromRad(0,0));

    // sun orientation
    SGGeod geodSunPos = SGGeod::fromRad(sun_lon, sun_lat);
    SGQuatd sunOr = SGQuatd::fromLonLat(geodSunPos);

    // scenery orientation
    SGGeod geodViewPos = SGGeod::fromCart(viewer->getViewPosition());
    SGQuatd hlOr = SGQuatd::fromLonLat(geodViewPos);
    SGVec3d localAt = hlOr.backTransform(SGVec3d::e3());

    // transpose the sun direction from (lat,lon) to (0,0)
    SGVec3d transSunDir = (q*sunOr).transform(-localAt);
    SGQuatd sunDirOr = SGQuatd::fromRealImag(0, transSunDir);

    // transpose the calculated sun vector back to (lat,lon)
    SGVec3d sunDirection = sunDirOr.transform(localAt);
    light->set_sun_rotation( acos(sunDirection[1])-SGD_PI_2 );
    light->set_sun_angle( acos(-sunDirection[2]) );

    SGVec3d sunPos = SGVec3d::fromGeod(geodSunPos);
    light->sun_vec() = SGVec4f(toVec3f(normalize(sunPos)), 0);
    light->sun_vec_inv() = -light->sun_vec();

#else
    FGLight *l = (FGLight *)(globals->get_subsystem("lighting"));
    SGTime *t = globals->get_time_params();
    FGViewer *v = globals->get_current_view();

    SG_LOG( SG_EVENT, SG_DEBUG, "  Updating Sun position" );
    SG_LOG( SG_EVENT, SG_DEBUG, "  Gst = " << t->getGst() );

    double sun_l;
    double sun_gd_lat;
    fgSunPositionGST(t->getGst(), &sun_l, &sun_gd_lat);
    l->set_sun_lon(sun_l);
    l->set_sun_lat(sun_gd_lat);
    SGVec3d sunpos(SGVec3d::fromGeod(SGGeod::fromRad(sun_l, sun_gd_lat)));

    SG_LOG( SG_EVENT, SG_DEBUG, "    t->cur_time = " << t->get_cur_time() );
    SG_LOG( SG_EVENT, SG_DEBUG,
            "    Sun Geodetic lat = " << sun_gd_lat
            << " Geodetic lat = " << sun_gd_lat );

    // update the sun light vector
    l->sun_vec() = SGVec4f(toVec3f(normalize(sunpos)), 0);
    l->sun_vec_inv() = - l->sun_vec();

    // calculate the sun's relative angle to local up
    SGVec3d viewPos = v->get_view_pos();
    SGQuatd hlOr = SGQuatd::fromLonLat(SGGeod::fromCart(viewPos));
    SGVec3f world_up = toVec3f(hlOr.backTransform(-SGVec3d::e3()));
    SGVec3f nsun = toVec3f(normalize(sunpos));
    // cout << "nup = " << nup[0] << "," << nup[1] << ","
    //      << nup[2] << endl;
    // cout << "nsun = " << nsun[0] << "," << nsun[1] << ","
    //      << nsun[2] << endl;

    l->set_sun_angle( acos( dot ( world_up, nsun ) ) );
    SG_LOG( SG_EVENT, SG_DEBUG, "sun angle relative to current location = "
            << l->get_sun_angle() );

    // calculate vector to sun's position on the earth's surface
    SGVec3d rel_sunpos = sunpos - v->get_view_pos();
    // vector in cartesian coordinates from current position to the
    // postion on the earth's surface the sun is directly over
    SGVec3f to_sun = toVec3f(rel_sunpos);
    // printf( "Vector to sun = %.2f %.2f %.2f\n",
    //         v->to_sun[0], v->to_sun[1], v->to_sun[2]);

    // Given a vector from the view position to the point on the
    // earth's surface the sun is directly over, map into onto the
    // local plane representing "horizontal".

    // surface direction to go to head towards sun
    SGVec3f view_pos = toVec3f(v->get_view_pos());
    SGVec3f surface_to_sun = map_vec_onto_cur_surface_plane(world_up, view_pos, to_sun);
    
    surface_to_sun = normalize(surface_to_sun);
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
    SGVec3f surface_east(toVec3f(hlOr.backTransform(SGVec3d::e2())));
    float east_dot = dot( surface_to_sun, surface_east );
    // cout << "  East dot product = " << east_dot << endl;

    // calculate the angle between v->surface_to_sun and
    // v->surface_south.  this is how much we have to rotate the sky
    // for it to align with the sun
    SGVec3f surface_south(toVec3f(hlOr.backTransform(-SGVec3d::e1())));
    float dot_ = dot( surface_to_sun, surface_south );
    // cout << "  Dot product = " << dot << endl;

    if (dot_ > 1.0) {
        SG_LOG( SG_ASTRO, SG_INFO,
                "Dot product  = " << dot_ << " is greater than 1.0" );
        dot_ = 1.0;
    }
    else if (dot_ < -1.0) {
         SG_LOG( SG_ASTRO, SG_INFO,
                 "Dot product  = " << dot_ << " is less than -1.0" );
         dot_ = -1.0;
     }

    if ( east_dot >= 0 ) {
        l->set_sun_rotation( acos(dot_) );
    } else {
        l->set_sun_rotation( -acos(dot_) );
    }
    // cout << "  Sky needs to rotate = " << angle << " rads = "
    //      << angle * SGD_RADIANS_TO_DEGREES << " degrees." << endl;

#endif
}

