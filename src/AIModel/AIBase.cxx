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

#include <simgear/compiler.h>

#include STL_STRING

#include <plib/sg.h>
#include <plib/ssg.h>

#include <simgear/math/point3d.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/scene/model/location.hxx>
#include <simgear/scene/model/model.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/props/props.hxx>

#include <Main/globals.hxx>
#include <Scenery/scenery.hxx>


#include "AIBase.hxx"
#include "AIManager.hxx"

FGAIBase::FGAIBase() {
    _type_str = "model";
    tgt_roll = roll = tgt_pitch = tgt_yaw = tgt_vs = vs = pitch = 0.0;
    bearing = elevation = range = rdot = 0.0;
    x_shift = y_shift = rotation = 0.0;
    invisible = true;
    no_roll = true;
    model_path = "";
    model = 0;
    _otype = otNull;
    index = 0;
}

FGAIBase::~FGAIBase() {
    globals->get_scenery()->get_scene_graph()->removeKid(aip.getSceneGraph());
    // unbind();
    SGPropertyNode *root = globals->get_props()->getNode("ai/models", true);
    root->removeChild(_type_str.c_str(), index);
    if (fp) delete fp;
}

void FGAIBase::update(double dt) {
}


void FGAIBase::Transform() {
    if (!invisible) {
      aip.setPosition(pos.lon(), pos.lat(), pos.elev() * SG_METER_TO_FEET);
      if (no_roll) {
         aip.setOrientation(0.0, pitch, hdg);
      } else {
         aip.setOrientation(roll, pitch, hdg);
      }
      aip.update( globals->get_scenery()->get_center() );    
    }
}


bool FGAIBase::init() {

   SGPropertyNode *root = globals->get_props()->getNode("ai/models", true);
   index = manager->getNum(_otype) - 1;
   props = root->getNode(_type_str.c_str(), index, true);
   if (model_path != "") {
      model = sgLoad3DModel( globals->get_fg_root(),
	                     model_path.c_str(),
                             props,
	                     globals->get_sim_time_sec() );
   }
   if (model) {
     aip.init( model );
     aip.setVisible(true);
     invisible = false;
     globals->get_scenery()->get_scene_graph()->addKid(aip.getSceneGraph());
   } else {
     if (model_path != "") { 
       SG_LOG(SG_INPUT, SG_WARN, "AIBase: Could not load model.");
     }
   } 

   setDie(false);

   return true;
}

bool FGAIBase::isa( object_type otype ) {
 if ( otype == _otype ) { return true; }
 else { return false; } 
}


void FGAIBase::bind() {
   props->tie("id", SGRawValuePointer<int>(&id));
   props->tie("velocities/true-airspeed-kt",  SGRawValuePointer<double>(&speed));
   props->tie("velocities/vertical-speed-fps",
               SGRawValueMethods<FGAIBase,double>(*this,
                                         &FGAIBase::_getVS_fps,
                                         &FGAIBase::_setVS_fps));

   props->tie("position/altitude-ft",
               SGRawValueMethods<FGAIBase,double>(*this,
                                         &FGAIBase::_getAltitude,
                                         &FGAIBase::_setAltitude));
   props->tie("position/latitude-deg",
               SGRawValueMethods<FGAIBase,double>(*this,
                                         &FGAIBase::_getLatitude,
                                         &FGAIBase::_setLatitude));
   props->tie("position/longitude-deg",
               SGRawValueMethods<FGAIBase,double>(*this,
                                         &FGAIBase::_getLongitude,
                                         &FGAIBase::_setLongitude));

   props->tie("orientation/pitch-deg",   SGRawValuePointer<double>(&pitch));
   props->tie("orientation/roll-deg",    SGRawValuePointer<double>(&roll));
   props->tie("orientation/true-heading-deg", SGRawValuePointer<double>(&hdg));

   props->tie("radar/bearing-deg",   SGRawValuePointer<double>(&bearing));
   props->tie("radar/elevation-deg", SGRawValuePointer<double>(&elevation));
   props->tie("radar/range-nm", SGRawValuePointer<double>(&range));
   props->tie("radar/h-offset", SGRawValuePointer<double>(&horiz_offset));
   props->tie("radar/v-offset", SGRawValuePointer<double>(&vert_offset)); 
   props->tie("radar/x-shift", SGRawValuePointer<double>(&x_shift));
   props->tie("radar/y-shift", SGRawValuePointer<double>(&y_shift));
   props->tie("radar/rotation", SGRawValuePointer<double>(&rotation));

   props->tie("controls/lighting/nav-lights",
               SGRawValueFunctions<bool>(_isNight));
   props->setBoolValue("controls/lighting/beacon", true);
   props->setBoolValue("controls/lighting/strobe", true);
}

void FGAIBase::unbind() {
    props->untie("id");
    props->untie("velocities/true-airspeed-kt");
    props->untie("velocities/vertical-speed-fps");

    props->untie("position/altitude-ft");
    props->untie("position/latitude-deg");
    props->untie("position/longitude-deg");

    props->untie("orientation/pitch-deg");
    props->untie("orientation/roll-deg");
    props->untie("orientation/true-heading-deg");

    props->untie("radar/bearing-deg");
    props->untie("radar/elevation-deg");
    props->untie("radar/range-nm");
    props->untie("radar/h-offset");
    props->untie("radar/v-offset");
    props->untie("radar/x-shift");
    props->untie("radar/y-shift");
    props->untie("radar/rotation");

    props->untie("controls/lighting/nav-lights");
}


/*
 * getters and Setters
 */
void FGAIBase::_setLongitude( double longitude ) {
    pos.setlon(longitude);
}
void FGAIBase::_setLatitude ( double latitude )  {
    pos.setlat(latitude);
}

double FGAIBase::_getLongitude() const {
    return pos.lon();
}
double FGAIBase::_getLatitude () const {
    return pos.lat();
}
double FGAIBase::_getRdot() const {
    return rdot;
}
double FGAIBase::_getVS_fps() const {
    return vs*60.0;
}
void FGAIBase::_setVS_fps( double _vs ) {
    vs = _vs/60.0;
}

double FGAIBase::_getAltitude() const {
    return altitude;
}
void FGAIBase::_setAltitude( double _alt ) {
    setAltitude( _alt );
}

bool FGAIBase::_isNight() {
    return (fgGetFloat("/sim/time/sun-angle-rad") > 1.57);
}

