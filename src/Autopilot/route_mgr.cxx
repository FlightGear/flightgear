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

#include <FDM/flight.hxx>
#include <Main/fg_props.hxx>
#include <Navaids/positioned.hxx>

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
    delete listener;
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

void FGRouteMgr::updateTargetAltitude() {
    if (route->size() == 0) {
        altitude_set = false;
        return;
    }
    
    SGWayPoint wp = route->get_waypoint( 0 );
    if (wp.get_target_alt() < -9990.0) {
        altitude_set = false;
        return;
    }
    
    altitude_set = true;
    target_altitude_ft->setDoubleValue( wp.get_target_alt() * SG_METER_TO_FEET );
            
    if ( !near_ground() ) {
        // James Turner [zakalawe]: there's no explanation for this logic,
        // it feels like the autopilot should pull the target altitude out of
        // wp0 instead of us pushing it through here. Hmmm.
        altitude_lock->setStringValue( "altitude-hold" );
    }
}

void FGRouteMgr::update( double dt ) {
    if (route->size() == 0) {
      return; // no route set, early return
    }
    
    double wp_course, wp_distance;

    // first way point
    SGWayPoint wp = route->get_waypoint( 0 );
    wp.CourseAndDistance( lon->getDoubleValue(), lat->getDoubleValue(),
                          alt->getDoubleValue(), &wp_course, &wp_distance );
    true_hdg_deg->setDoubleValue( wp_course );

    if ( wp_distance < 200.0 ) {
        pop_waypoint();
        if (route->size() == 0) {
            return; // end of route, we're done for the time being
        }
        
        wp = route->get_waypoint( 0 );
    }

  // update the property tree info for WP0
    wp0_id->setStringValue( wp.get_id().c_str() );
    double accum = wp_distance;
    wp0_dist->setDoubleValue( accum * SG_METER_TO_NM );
    setETAPropertyFromDistance(wp0_eta, accum);

    // next way point
    if ( route->size() > 1 ) {
        SGWayPoint wp = route->get_waypoint( 1 );

        // update the property tree info
        wp1_id->setStringValue( wp.get_id().c_str() );
        accum += wp.get_distance();
        wp1_dist->setDoubleValue( accum * SG_METER_TO_NM );
        setETAPropertyFromDistance(wp1_eta, accum);
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
        setETAPropertyFromDistance(wpn_eta, accum);
    }
}

void FGRouteMgr::setETAPropertyFromDistance(SGPropertyNode_ptr aProp, double aDistance) {
    char eta_str[64];
    double eta = aDistance * SG_METER_TO_NM / get_ground_speed();
    if ( eta >= 100.0 ) { 
        eta = 99.999; // clamp
    }
    
    if ( eta < (1.0/6.0) ) {
        // within 10 minutes, bump up to min/secs
        eta *= 60.0;
    }
    
    int major = (int)eta, 
        minor = (int)((eta - (int)eta) * 60.0);
    snprintf( eta_str, 64, "%d:%02d", major, minor );
    aProp->setStringValue( eta_str );
}

void FGRouteMgr::add_waypoint( const SGWayPoint& wp, int n ) {
    route->add_waypoint( wp, n );
    update_mirror();
    if ((n==0) || (route->size() == 1)) {
        updateTargetAltitude();
    }
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

    updateTargetAltitude();
    update_mirror();
    return wp;
}


bool FGRouteMgr::build() {
    return true;
}


void FGRouteMgr::new_waypoint( const string& target, int n ) {
    SGWayPoint* wp = make_waypoint( target );
    if (!wp) {
        return;
    }
    
    add_waypoint( *wp, n );
    delete wp;

    if ( !near_ground() ) {
        fgSetString( "/autopilot/locks/heading", "true-heading-hold" );
    }
}


SGWayPoint* FGRouteMgr::make_waypoint(const string& tgt ) {
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
        return new SGWayPoint( lon, lat, alt, SGWayPoint::WGS84, target );
    }    

    SGGeod basePosition;
    if (route->size() > 0) {
        SGWayPoint wp = get_waypoint(route->size()-1);
        basePosition = SGGeod::fromDeg(wp.get_target_lon(), wp.get_target_lat());
    } else {
        // route is empty, use current position
        basePosition = SGGeod::fromDeg(
            fgGetNode("/position/longitude-deg")->getDoubleValue(), 
            fgGetNode("/position/latitude-deg")->getDoubleValue());
    }


    FGPositionedRef p = FGPositioned::findClosestWithIdent(target, basePosition);
    if (!p) {
        SG_LOG( SG_GENERAL, SG_INFO, "Unable to find FGPositioned with ident:" << target);
        return NULL;
    }
    
    return new SGWayPoint(p->longitude(), p->latitude(), alt, SGWayPoint::WGS84, target);
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
    if ( !gear || gear->getType() == simgear::props::NONE )
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


