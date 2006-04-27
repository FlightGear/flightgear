// route_mgr.hxx - manage a route (i.e. a collection of waypoints)
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


#ifndef _ROUTE_MGR_HXX
#define _ROUTE_MGR_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#include STL_STRING
#include <vector>

SG_USING_STD(string);
SG_USING_STD(vector);

#include <simgear/props/props.hxx>
#include <simgear/route/route.hxx>
#include <simgear/structure/subsystem_mgr.hxx>


/**
 * Top level route manager class
 * 
 */

class FGRouteMgr : public SGSubsystem
{

private:

    SGRoute *route;

    // automatic inputs
    SGPropertyNode_ptr lon;
    SGPropertyNode_ptr lat;
    SGPropertyNode_ptr alt;

    // automatic outputs
    SGPropertyNode_ptr true_hdg_deg;

    SGPropertyNode_ptr wp0_id;
    SGPropertyNode_ptr wp0_dist;
    SGPropertyNode_ptr wp0_eta;

    SGPropertyNode_ptr wp1_id;
    SGPropertyNode_ptr wp1_dist;
    SGPropertyNode_ptr wp1_eta;

    SGPropertyNode_ptr wpn_id;
    SGPropertyNode_ptr wpn_dist;
    SGPropertyNode_ptr wpn_eta;


public:

    FGRouteMgr();
    ~FGRouteMgr();

    void init ();
    void postinit ();
    void bind ();
    void unbind ();
    void update (double dt);

    bool build ();

    void add_waypoint( const SGWayPoint& wp ) {
        route->add_waypoint( wp );
    }

    SGWayPoint get_waypoint( int i ) const {
        return route->get_waypoint(i);
    }

    SGWayPoint pop_waypoint();

    int size() const {
        return route->size();
    }
};


#endif // _ROUTE_MGR_HXX
