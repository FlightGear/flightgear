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

FGAIBase *FGAIBase::_self = NULL;

FGAIBase::FGAIBase() {
    _self = this;
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
    unbind();
    SGPropertyNode *root = globals->get_props()->getNode("ai/models", true);
    root->removeChild(_type_str.c_str(), index);
    _self = NULL;
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
               SGRawValueFunctions<double>(FGAIBase::_getVS_fps,
                                           FGAIBase::_setVS_fps));

   props->tie("position/altitude-ft",
               SGRawValueFunctions<double>(FGAIBase::_getAltitude,
                                           FGAIBase::_setAltitude));
   props->tie("position/latitude-deg",
               SGRawValueFunctions<double>(FGAIBase::_getLatitude,
                                           FGAIBase::_setLatitude));
   props->tie("position/longitude-deg",
               SGRawValueFunctions<double>(FGAIBase::_getLongitude,
                                           FGAIBase::_setLongitude));

   props->tie("orientation/pitch-deg",   SGRawValuePointer<double>(&pitch));
   props->tie("orientation/roll-deg",    SGRawValuePointer<double>(&roll));
   props->tie("orientation/true-heading-deg", SGRawValuePointer<double>(&hdg));

   props->tie("radar/bearing-deg",   SGRawValueFunctions<double>(FGAIBase::_getBearing));
   props->tie("radar/elevation-deg", SGRawValueFunctions<double>(FGAIBase::_getElevation));
   props->tie("radar/range-nm",      SGRawValueFunctions<double>(FGAIBase::_getRange));
   props->tie("radar/h-offset", SGRawValueFunctions<double>(FGAIBase::_getH_offset));
   props->tie("radar/v-offset", SGRawValueFunctions<double>(FGAIBase::_getV_offset)); 
   props->tie("radar/x-shift", SGRawValueFunctions<double>(FGAIBase::_getX_shift));
   props->tie("radar/y-shift", SGRawValueFunctions<double>(FGAIBase::_getY_shift));
   props->tie("radar/rotation", SGRawValueFunctions<double>(FGAIBase::_getRotation));

   props->tie("controls/lighting/nav-lights",
               SGRawValueFunctions<bool>(FGAIBase::_isNight));
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
    if (_self) _self->pos.setlon(longitude);
}
void FGAIBase::_setLatitude ( double latitude )  {
    if (_self) _self->pos.setlat(latitude);
}

double FGAIBase::_getLongitude() {
    return (!_self) ? 0.0 :_self->pos.lon();
}
double FGAIBase::_getLatitude () {
    return (!_self) ? 0.0 :_self->pos.lat();
}
double FGAIBase::_getBearing()   {
    return (!_self) ? 0.0 :_self->bearing;
}
double FGAIBase::_getElevation() {
    return (!_self) ? 0.0 :_self->elevation;
}
double FGAIBase::_getRange()     {
    return (!_self) ? 0.0 :_self->range;
}
double FGAIBase::_getRdot()      {
    return (!_self) ? 0.0 :_self->rdot;
}
double FGAIBase::_getH_offset()  {
    return (!_self) ? 0.0 :_self->horiz_offset;
}
double FGAIBase::_getV_offset()  {
    return (!_self) ? 0.0 :_self->vert_offset;
}
double FGAIBase::_getX_shift()   {
    return (!_self) ? 0.0 :_self->x_shift;
}
double FGAIBase::_getY_shift()   {
    return (!_self) ? 0.0 :_self->y_shift;
}
double FGAIBase::_getRotation()  {
    return (!_self) ? 0.0 :_self->rotation;
}

double FGAIBase::_getVS_fps() {
    return (!_self) ? 0.0 :_self->vs*60.0;
}
void FGAIBase::_setVS_fps( double _vs ) {
    if (_self) _self->vs = _vs/60.0;
}

double FGAIBase::_getAltitude() {
    return (!_self) ? 0.0 :_self->altitude;
}
void FGAIBase::_setAltitude( double _alt ) {
    if (_self) _self->setAltitude( _alt );
}

bool FGAIBase::_isNight() {
    return (fgGetFloat("/sim/time/sun-angle-rad") > 1.57);
}

