// scenery.cxx -- data structures and routines for managing scenery.
//
// Written by Curtis Olson, started May 1997.
//
// Copyright (C) 1997  Curtis L. Olson  - http://www.flightgear.org/~curt
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

#include <stdio.h>
#include <string.h>

#include <simgear/debug/logstream.hxx>
#include <simgear/scene/tgdb/userdata.hxx>
#include <simgear/scene/model/placementtrans.hxx>

#include <Main/fg_props.hxx>

#include "scenery.hxx"


// Scenery Management system
FGScenery::FGScenery() {
    SG_LOG( SG_TERRAIN, SG_INFO, "Initializing scenery subsystem" );

    center = Point3D(0.0);
    cur_elev = -9999;
}


// Initialize the Scenery Management system
FGScenery::~FGScenery() {
}


void FGScenery::init() {
    // Scene graph root
    scene_graph = new ssgRoot;
    scene_graph->setName( "Scene" );

    // Terrain branch
    terrain_branch = new ssgBranch;
    terrain_branch->setName( "Terrain" );
    scene_graph->addKid( terrain_branch );

    models_branch = new ssgBranch;
    models_branch->setName( "Models" );
    scene_graph->addKid( models_branch );

    aircraft_branch = new ssgBranch;
    aircraft_branch->setName( "Aircraft" );
    scene_graph->addKid( aircraft_branch );

    // Lighting
    gnd_lights_root = new ssgRoot;
    gnd_lights_root->setName( "Ground Lighting Root" );

    vasi_lights_root = new ssgRoot;
    vasi_lights_root->setName( "VASI/PAPI Lighting Root" );

    rwy_lights_root = new ssgRoot;
    rwy_lights_root->setName( "Runway Lighting Root" );

    taxi_lights_root = new ssgRoot;
    taxi_lights_root->setName( "Taxi Lighting Root" );

    // Initials values needed by the draw-time object loader
    sgUserDataInit( globals->get_model_lib(), globals->get_fg_root(),
                    globals->get_props(), globals->get_sim_time_sec() );
}


void FGScenery::update(double dt) {
}


void FGScenery::bind() {
    fgTie("/environment/ground-elevation-m", this,
	  &FGScenery::get_cur_elev, &FGScenery::set_cur_elev);
}


void FGScenery::unbind() {
    fgUntie("/environment/ground-elevation-m");
}

void FGScenery::set_center( Point3D p ) {
    center = p;
    sgdVec3 c;
    sgdSetVec3(c, p.x(), p.y(), p.z());
    placement_list_type::iterator it = _placement_list.begin();
    while (it != _placement_list.end()) {
        (*it)->setSceneryCenter(c);
        ++it;
    }
}

void FGScenery::register_placement_transform(ssgPlacementTransform *trans) {
    trans->ref();
    _placement_list.push_back(trans);        
    sgdVec3 c;
    sgdSetVec3(c, center.x(), center.y(), center.z());
    trans->setSceneryCenter(c);
}

void FGScenery::unregister_placement_transform(ssgPlacementTransform *trans) {
    placement_list_type::iterator it = _placement_list.begin();
    while (it != _placement_list.end()) {
        if ((*it) == trans) {
            (*it)->deRef();
            it = _placement_list.erase(it);        
        } else
            ++it;
    }
}
