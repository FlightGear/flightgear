// route_mgr.cxx - manage a route (i.e. a collection of waypoints)
//
// Written by Curtis Olson, started January 2004.
//            Norman Vine
//            Melchior FRANZ
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

#include <Airports/simple.hxx>
#include <FDM/flight.hxx>
#include <Main/fg_props.hxx>
#include <Navaids/fixlist.hxx>
#include <Navaids/navlist.hxx>

#include "route_mgr.hxx"

#define RM "/autopilot/route-manager/"


FGRouteMgr::FGRouteMgr() :
    route( new SGRoute ),
    lon( NULL ),
    lat( NULL ),
    alt( NULL ),
    true_hdg_deg( NULL ),
    target_altitude_ft( NULL ),
    altitude_lock( NULL ),
    wp0_id( NULL ),
    wp0_dist( NULL ),
    wp0_eta( NULL ),
    wp1_id( NULL ),
    wp1_dist( NULL ),
    wp1_eta( NULL ),
    wpn_id( NULL ),
    wpn_dist( NULL ),
    wpn_eta( NULL ),
    input(fgGetNode( RM "input", true )),
    listener(new Listener(this)),
    mirror(fgGetNode( RM "route", true )),
    altitude_set( false )
{
    input->setStringValue("");
    input->addChangeListener(listener);
}


FGRouteMgr::~FGRouteMgr() {
    input->removeChangeListener(listener);
    delete route;
}


void FGRouteMgr::init() {
    lon = fgGetNode( "/position/longitude-deg", true );
    lat = fgGetNode( "/position/latitude-deg", true );
    alt = fgGetNode( "/position/altitude-ft", true );

    true_hdg_deg = fgGetNode( "/autopilot/settings/true-heading-deg", true );
    target_altitude_ft = fgGetNode( "/autopilot/settings/target-altitude-ft", true );
    altitude_lock = fgGetNode( "/autopilot/locks/altitude", true );

    wp0_id = fgGetNode( RM "wp[0]/id", true );
    wp0_dist = fgGetNode( RM "wp[0]/dist", true );
    wp0_eta = fgGetNode( RM "wp[0]/eta", true );

    wp1_id = fgGetNode( RM "wp[1]/id", true );
    wp1_dist = fgGetNode( RM "wp[1]/dist", true );
    wp1_eta = fgGetNode( RM "wp[1]/eta", true );

    wpn_id = fgGetNode( RM "wp-last/id", true );
    wpn_dist = fgGetNode( RM "wp-last/dist", true );
    wpn_eta = fgGetNode( RM "wp-last/eta", true );

    route->clear();
    update_mirror();
}


void FGRouteMgr::postinit() {
    string_list *waypoints = globals->get_initial_waypoints();
    if (!waypoints)
        return;

    vector<string>::iterator it;
    for (it = waypoints->begin(); it != waypoints->end(); ++it)
        new_waypoint(*it);
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
        double target_alt = wp.get_target_alt();

        if ( !altitude_set && target_alt > -9990 ) {
            target_altitude_ft->setDoubleValue( target_alt * SG_METER_TO_FEET );
            altitude_set = true;

            if ( !near_ground() )
                altitude_lock->setStringValue( "altitude-hold" );
        }

        if ( wp_distance < 200.0 ) {
            pop_waypoint();
            altitude_set = false;
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


void FGRouteMgr::add_waypoint( const SGWayPoint& wp, int n ) {
    if ( n == 0 || !route->size() )
        altitude_set = false;

    route->add_waypoint( wp, n );
    update_mirror();
}


SGWayPoint FGRouteMgr::pop_waypoint( int n ) {
    SGWayPoint wp;

    if ( route->size() > 0 ) {
        if ( n < 0 )
            n = route->size() - 1;
        wp = route->get_waypoint(n);
        route->delete_waypoint(n);
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

    if ( n == 0 && route->size() )
        altitude_set = false;

    update_mirror();
    return wp;
}


bool FGRouteMgr::build() {
    return true;
}


int FGRouteMgr::new_waypoint( const string& target, int n ) {
    SGWayPoint *wp = 0;
    int type = make_waypoint( &wp, target );

    if (wp) {
        add_waypoint( *wp, n );
        delete wp;

        if ( !near_ground() )
            fgSetString( "/autopilot/locks/heading", "true-heading-hold" );
    }
    return type;
}


int FGRouteMgr::make_waypoint( SGWayPoint **wp, const string& tgt ) {
    string target = tgt;

    // make upper case
    for (unsigned int i = 0; i < target.size(); i++)
        if (target[i] >= 'a' && target[i] <= 'z')
            target[i] -= 'a' - 'A';

    // extract altitude
    double alt = -9999.0;
    size_t pos = target.find( '@' );
    if ( pos != string::npos ) {
        alt = atof( target.c_str() + pos + 1 );
        target = target.substr( 0, pos );
        if ( !strcmp(fgGetString("/sim/startup/units"), "feet") )
            alt *= SG_FEET_TO_METER;
    }

    // check for lon,lat
    pos = target.find( ',' );
    if ( pos != string::npos ) {
        double lon = atof( target.substr(0, pos).c_str());
        double lat = atof( target.c_str() + pos + 1);

        SG_LOG( SG_GENERAL, SG_INFO, "Adding waypoint lon = " << lon << ", lat = " << lat );
        *wp = new SGWayPoint( lon, lat, alt, SGWayPoint::WGS84, target );
        return 1;
    }

    // check for airport id
    const FGAirport *apt = fgFindAirportID( target );
    if (apt) {
        SG_LOG( SG_GENERAL, SG_INFO, "Adding waypoint (airport) = " << target );
        *wp = new SGWayPoint( apt->getLongitude(), apt->getLatitude(), alt, SGWayPoint::WGS84, target );
        return 2;
    }

    double lat, lon;
    // The base lon/lat are determined by the last WP,
    // or the current pos if the WP list is empty.
    const int wps = this->size();

    if (wps > 0) {
        SGWayPoint wp = get_waypoint(wps-1);
        lat = wp.get_target_lat();
        lon = wp.get_target_lon();
    } else {
        lat = fgGetNode("/position/latitude-deg")->getDoubleValue();
        lon = fgGetNode("/position/longitude-deg")->getDoubleValue();
    }

    // check for fix id
    FGFix f;
    double heading;
    double dist;

    if ( globals->get_fixlist()->query_and_offset( target, lon, lat, 0, &f, &heading, &dist ) ) {
        SG_LOG( SG_GENERAL, SG_INFO, "Adding waypoint (fix) = " << target );
        *wp = new SGWayPoint( f.get_lon(), f.get_lat(), alt, SGWayPoint::WGS84, target );
        return 3;
    }

    // Try finding a nav matching the ID
    lat *= SGD_DEGREES_TO_RADIANS;
    lon *= SGD_DEGREES_TO_RADIANS;

    SG_LOG( SG_GENERAL, SG_INFO, "Looking for nav " << target << " at " << lon << " " << lat);

    if (FGNavRecord* nav = globals->get_navlist()->findByIdent(target.c_str(), lon, lat)) {
        SG_LOG( SG_GENERAL, SG_INFO, "Adding waypoint (nav) = " << target );
        *wp = new SGWayPoint( nav->get_lon(), nav->get_lat(), alt, SGWayPoint::WGS84, target );
        return 4;
    }

    // unknown target
    return 0;
}


// mirror internal route to the property system for inspection by other subsystems
void FGRouteMgr::update_mirror() {
    mirror->removeChildren("wp");
    for (int i = 0; i < route->size(); i++) {
        SGWayPoint wp = route->get_waypoint(i);
        SGPropertyNode *prop = mirror->getChild("wp", i, 1);

        prop->setStringValue("id", wp.get_id().c_str());
        prop->setStringValue("name", wp.get_name().c_str());
        prop->setDoubleValue("longitude-deg", wp.get_target_lon());
        prop->setDoubleValue("latitude-deg", wp.get_target_lat());
        prop->setDoubleValue("altitude-m", wp.get_target_alt());
        prop->setDoubleValue("altitude-ft", wp.get_target_alt() * SG_METER_TO_FEET);
    }
    // set number as listener attachment point
    mirror->setIntValue("num", route->size());
}


bool FGRouteMgr::near_ground() {
    SGPropertyNode *gear = fgGetNode( "/gear/gear/wow", false );
    if ( !gear || gear->getType() == SGPropertyNode::NONE )
        return fgGetBool( "/sim/presets/onground", true );

    if ( fgGetDouble("/position/altitude-agl-ft", 300.0)
            < fgGetDouble("/autopilot/route-manager/min-lock-altitude-agl-ft") )
        return true;

    return gear->getBoolValue();
}


// command interface /autopilot/route-manager/input:
//
//   @CLEAR             ... clear route
//   @POP               ... remove first entry
//   @DELETE3           ... delete 4th entry
//   @INSERT2:KSFO@900  ... insert "KSFO@900" as 3rd entry
//   KSFO@900           ... append "KSFO@900"
//
void FGRouteMgr::Listener::valueChanged(SGPropertyNode *prop)
{
    const char *s = prop->getStringValue();
    if (!strcmp(s, "@CLEAR"))
        mgr->init();
    else if (!strcmp(s, "@POP"))
        mgr->pop_waypoint(0);
    else if (!strncmp(s, "@DELETE", 7))
        mgr->pop_waypoint(atoi(s + 7));
    else if (!strncmp(s, "@INSERT", 7)) {
        char *r;
        int pos = strtol(s + 7, &r, 10);
        if (*r++ != ':')
            return;
        while (isspace(*r))
            r++;
        if (*r)
            mgr->new_waypoint(r, pos);
    } else
        mgr->new_waypoint(s);
}


