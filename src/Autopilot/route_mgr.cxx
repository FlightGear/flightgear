// route_mgr.cxx - manage a route (i.e. a collection of waypoints)
//
// Written by Curtis Olson, started January 2004.
//
// Copyright (C) 2004  Curtis L. Olson  - http://www.flightgear.org/~curt
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

#include <simgear/compiler.h>

#include <FDM/flight.hxx>
#include <Main/fg_props.hxx>

#include "auto_gui.hxx"        // FIXME temporary dependency (NewWaypoint)
#include "route_mgr.hxx"


FGRouteMgr::FGRouteMgr() :
    route( new SGRoute ),
    lon( NULL ),
    lat( NULL ),
    alt( NULL ),
    true_hdg_deg( NULL ),
    wp0_id( NULL ),
    wp0_dist( NULL ),
    wp0_eta( NULL ),
    wp1_id( NULL ),
    wp1_dist( NULL ),
    wp1_eta( NULL ),
    wpn_id( NULL ),
    wpn_dist( NULL ),
    wpn_eta( NULL )
{
}


FGRouteMgr::~FGRouteMgr() {
    delete route;
}


void FGRouteMgr::init() {
    lon = fgGetNode( "/position/longitude-deg", true );
    lat = fgGetNode( "/position/latitude-deg", true );
    alt = fgGetNode( "/position/altitude-ft", true );

    true_hdg_deg = fgGetNode( "/autopilot/settings/true-heading-deg", true );

    wp0_id = fgGetNode( "/autopilot/route-manager/wp[0]/id", true );
    wp0_dist = fgGetNode( "/autopilot/route-manager/wp[0]/dist", true );
    wp0_eta = fgGetNode( "/autopilot/route-manager/wp[0]/eta", true );

    wp1_id = fgGetNode( "/autopilot/route-manager/wp[1]/id", true );
    wp1_dist = fgGetNode( "/autopilot/route-manager/wp[1]/dist", true );
    wp1_eta = fgGetNode( "/autopilot/route-manager/wp[1]/eta", true );

    wpn_id = fgGetNode( "/autopilot/route-manager/wp-last/id", true );
    wpn_dist = fgGetNode( "/autopilot/route-manager/wp-last/dist", true );
    wpn_eta = fgGetNode( "/autopilot/route-manager/wp-last/eta", true );

    route->clear();
}


void FGRouteMgr::postinit() {
    string_list *waypoints = globals->get_initial_waypoints();
    if (!waypoints)
        return;

    vector<string>::iterator it;
    for (it = waypoints->begin(); it != waypoints->end(); ++it)
        NewWaypoint(*it);
}


void FGRouteMgr::bind() { }
void FGRouteMgr::unbind() { }


static double get_ground_speed() {
    // starts in ft/s so we convert to kts
    static const SGPropertyNode * speedup_node = fgGetNode("/sim/speed-up");

    double ft_s = cur_fdm_state->get_V_ground_speed() 
        * speedup_node->getIntValue();
    double kts = ft_s * SG_FEET_TO_METER * 3600 * SG_METER_TO_NM;

    return kts;
}


void FGRouteMgr::update( double dt ) {
    double accum = 0.0;
    double wp_course, wp_distance;
    char eta_str[128];

    // first way point
    if ( route->size() > 0 ) {
        SGWayPoint wp = route->get_waypoint( 0 );
        wp.CourseAndDistance( lon->getDoubleValue(), lat->getDoubleValue(),
                              alt->getDoubleValue(), &wp_course, &wp_distance );

        true_hdg_deg->setDoubleValue( wp_course );

        if ( wp_distance < 200.0 ) {
            pop_waypoint();
        }
    }

    // next way point
    if ( route->size() > 0 ) {
        SGWayPoint wp = route->get_waypoint( 0 );
        // update the property tree info

        wp0_id->setStringValue( wp.get_id().c_str() );

        accum += wp_distance;
        wp0_dist->setDoubleValue( accum * SG_METER_TO_NM );

        double eta = accum * SG_METER_TO_NM / get_ground_speed();
	if ( eta >= 100.0 ) { eta = 99.999; }
	int major, minor;
	if ( eta < (1.0/6.0) ) {
	    // within 10 minutes, bump up to min/secs
	    eta *= 60.0;
	}
	major = (int)eta;
	minor = (int)((eta - (int)eta) * 60.0);
	snprintf( eta_str, 128, "%d:%02d", major, minor );
        wp0_eta->setStringValue( eta_str );
    }

    // next way point
    if ( route->size() > 1 ) {
        SGWayPoint wp = route->get_waypoint( 1 );

        // update the property tree info

        wp1_id->setStringValue( wp.get_id().c_str() );

        accum += wp.get_distance();
        wp1_dist->setDoubleValue( accum * SG_METER_TO_NM );

        double eta = accum * SG_METER_TO_NM / get_ground_speed();
	if ( eta >= 100.0 ) { eta = 99.999; }
	int major, minor;
	if ( eta < (1.0/6.0) ) {
	    // within 10 minutes, bump up to min/secs
	    eta *= 60.0;
	}
	major = (int)eta;
	minor = (int)((eta - (int)eta) * 60.0);
	snprintf( eta_str, 128, "%d:%02d", major, minor );
        wp1_eta->setStringValue( eta_str );
    }

    // summarize remaining way points
    if ( route->size() > 2 ) {
        SGWayPoint wp;
	for ( int i = 2; i < route->size(); ++i ) {
            wp = route->get_waypoint( i );
	    accum += wp.get_distance();
	}

        // update the property tree info

        wpn_id->setStringValue( wp.get_id().c_str() );

        wpn_dist->setDoubleValue( accum * SG_METER_TO_NM );

        double eta = accum * SG_METER_TO_NM / get_ground_speed();
	if ( eta >= 100.0 ) { eta = 99.999; }
	int major, minor;
	if ( eta < (1.0/6.0) ) {
	    // within 10 minutes, bump up to min/secs
	    eta *= 60.0;
	}
	major = (int)eta;
	minor = (int)((eta - (int)eta) * 60.0);
	snprintf( eta_str, 128, "%d:%02d", major, minor );
        wpn_eta->setStringValue( eta_str );
    }
}


SGWayPoint FGRouteMgr::pop_waypoint() {
    SGWayPoint wp;

    if ( route->size() > 0 ) {
        wp = route->get_first();
        route->delete_first();
    }

    if ( route->size() <= 2 ) {
        wpn_id->setStringValue( "" );
        wpn_dist->setDoubleValue( 0.0 );
        wpn_eta->setStringValue( "" );
    }

    if ( route->size() <= 1 ) {
        wp1_id->setStringValue( "" );
        wp1_dist->setDoubleValue( 0.0 );
        wp1_eta->setStringValue( "" );
    }

    if ( route->size() <= 0 ) {
        wp0_id->setStringValue( "" );
        wp0_dist->setDoubleValue( 0.0 );
        wp0_eta->setStringValue( "" );
    }

    return wp;
}


bool FGRouteMgr::build() {
    return true;
}
