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
#include <simgear/math/vector.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/timing/sg_time.hxx>

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Main/viewer.hxx>
#include <Scenery/scenery.hxx>

#include "light.hxx"
#include "sunsolver.hxx"
#include "tmp.hxx"


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
    l->set_sunpos(SGVec3d::fromGeod(SGGeod::fromRad(sun_l, sun_gd_lat)));

    SG_LOG( SG_EVENT, SG_DEBUG, "    t->cur_time = " << t->get_cur_time() );
    SG_LOG( SG_EVENT, SG_DEBUG,
	    "    Sun Geodetic lat = " << sun_gd_lat
	    << " Geodetic lat = " << sun_gd_lat );

    // update the sun light vector
    l->sun_vec() = SGVec4f(toVec3f(normalize(l->get_sunpos())), 0);
    l->sun_vec_inv() = - l->sun_vec();

    // calculate the sun's relative angle to local up
    SGVec3f nup(normalize(v->get_world_up()));
    SGVec3f nsun(toVec3f(normalize(l->get_sunpos())));
    // cout << "nup = " << nup[0] << "," << nup[1] << "," 
    //      << nup[2] << endl;
    // cout << "nsun = " << nsun[0] << "," << nsun[1] << "," 
    //      << nsun[2] << endl;

    l->set_sun_angle( acos( dot ( nup, nsun ) ) );
    SG_LOG( SG_EVENT, SG_DEBUG, "sun angle relative to current location = "
	    << l->get_sun_angle() );
    
    // calculate vector to sun's position on the earth's surface
    SGVec3d rel_sunpos = l->get_sunpos() - v->get_view_pos();
    // vector in cartesian coordinates from current position to the
    // postion on the earth's surface the sun is directly over
    SGVec3f to_sun = toVec3f(rel_sunpos);
    // printf( "Vector to sun = %.2f %.2f %.2f\n",
    //         v->to_sun[0], v->to_sun[1], v->to_sun[2]);

    // Given a vector from the view position to the point on the
    // earth's surface the sun is directly over, map into onto the
    // local plane representing "horizontal".

    SGVec3f world_up = v->get_world_up();
    SGVec3f view_pos = toVec3f(v->get_view_pos());
    // surface direction to go to head towards sun
    SGVec3f surface_to_sun;
    sgmap_vec_onto_cur_surface_plane( world_up.data(), view_pos.data(),
				      to_sun.data(), surface_to_sun.data() );
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
    float east_dot = dot( surface_to_sun, v->get_surface_east() );
    // cout << "  East dot product = " << east_dot << endl;

    // calculate the angle between v->surface_to_sun and
    // v->surface_south.  this is how much we have to rotate the sky
    // for it to align with the sun
    float dot_ = dot( surface_to_sun, v->get_surface_south() );
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
}

