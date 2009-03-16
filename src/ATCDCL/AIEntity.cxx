// FGAIEntity - abstract base class an artificial intelligence entity
//
// Written by David Luff, started March 2002.
//
// Copyright (C) 2002  David C. Luff - david.luff@nottingham.ac.uk
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

/*****************************************************************
*
* WARNING - Curt has some ideas about AI traffic so anything in here
* may get rewritten or scrapped.  Contact Curt http://www.flightgear.org/~curt 
* before spending any time or effort on this code!!!
*
******************************************************************/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <Main/globals.hxx>
#include <Scenery/scenery.hxx>
#include <simgear/constants.h>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/misc/sg_path.hxx>
#include <string>

#include "AIEntity.hxx"

FGAIEntity::FGAIEntity() :
    _ground_elevation_m(0)
{
}

FGAIEntity::~FGAIEntity() {
    globals->get_scenery()->get_scene_graph()->removeChild(_aip.getSceneGraph());
}

void FGAIEntity::SetModel(osg::Node* model) {
    _aip.init(model);
    _aip.setVisible(false);
    globals->get_scenery()->get_scene_graph()->addChild(_aip.getSceneGraph());
}

void FGAIEntity::Update(double dt) {
}

const string &FGAIEntity::GetCallsign() {
    static string s = "";
    return(s);
}

void FGAIEntity::RegisterTransmission(int code) {
}

// Run the internal calculations
//void FGAIEntity::Update() {
void FGAIEntity::Transform() {
    _aip.setPosition(_pos);
    _aip.setOrientation(_roll, _pitch, _hdg);
    _aip.update();    
}
