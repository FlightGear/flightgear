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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

/*****************************************************************
*
* WARNING - Curt has some ideas about AI traffic so anything in here
* may get rewritten or scrapped.  Contact Curt curt@flightgear.org 
* before spending any time or effort on this code!!!
*
******************************************************************/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <Main/globals.hxx>
#include <Scenery/scenery.hxx>
//#include <simgear/constants.h>
#include <simgear/math/point3d.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/misc/sg_path.hxx>
#include <string>

#include "AIEntity.hxx"

FGAIEntity::~FGAIEntity() {
}

void FGAIEntity::Update(double dt) {
}

// Run the internal calculations
//void FGAIEntity::Update() {
void FGAIEntity::Transform() {
    aip.setPosition(pos.lon(), pos.lat(), pos.elev() * SG_METER_TO_FEET);
    aip.setOrientation(roll, pitch, hdg);
    aip.update();    
}
