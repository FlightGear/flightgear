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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/math/vector.hxx>
#include <simgear/math/polar3d.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/magvar/magvar.hxx>
#include <simgear/timing/sg_time.hxx>

#include <FDM/flight.hxx>
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

