// FGAIBase - abstract base class for AI objects
// Written by David Culp, started Nov 2003, based on
// David Luff's FGAIEntity class.
// - davidculp2@comcast.net
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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <plib/sg.h>
#include <plib/ssg.h>
#include <Main/globals.hxx>
#include <Scenery/scenery.hxx>
#include <simgear/math/point3d.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/scene/model/location.hxx>
#include <simgear/scene/model/model.hxx>
#include <simgear/debug/logstream.hxx>
#include <string>

#include "AIBase.hxx"

FGAIBase::~FGAIBase() {
}

void FGAIBase::update(double dt) {
}


void FGAIBase::Transform() {
    aip.setPosition(pos.lon(), pos.lat(), pos.elev() * SG_METER_TO_FEET);
    aip.setOrientation(roll, pitch, hdg);
    aip.update( globals->get_scenery()->get_center() );    
}


bool FGAIBase::init() {
   ssgBranch *model = sgLoad3DModel( globals->get_fg_root(),
	                             model_path.c_str(),
	                             globals->get_props(),
	                             globals->get_sim_time_sec() );
   if (model) {
     aip.init( model );
     aip.setVisible(true);
     globals->get_scenery()->get_scene_graph()->addKid(aip.getSceneGraph());
   } else {
     SG_LOG(SG_INPUT, SG_WARN, "AIBase: Could not load aircraft model.");
   } 

   tgt_roll = tgt_pitch = tgt_yaw = tgt_vs = vs = roll = pitch = 0.0;
}


void FGAIBase::setPath( const char* model ) {
  model_path.append(model);
}

void FGAIBase::setSpeed( double speed_KTAS ) {
  speed = tgt_speed = speed_KTAS;
}

void FGAIBase::setAltitude( double altitude_ft ) {
  altitude = tgt_altitude = altitude_ft;
  pos.setelev(altitude * 0.3048);
}

void FGAIBase::setLongitude( double longitude ) {
  pos.setlon(longitude);
}

void FGAIBase::setLatitude( double latitude ) {
  pos.setlat(latitude);
}

void FGAIBase::setHeading( double heading ) {
  hdg = tgt_heading = heading;
}

void FGAIBase::setDie( bool die ) {
  delete_me = die;
}

