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
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/scene/model/location.hxx>
#include <simgear/scene/model/model.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/props/props.hxx>

#include <Main/globals.hxx>
#include <Scenery/scenery.hxx>


#include "AIBase.hxx"
#include "AIManager.hxx"


const double FGAIBase::e = 2.71828183;
const double FGAIBase::lbs_to_slugs = 0.031080950172;   //conversion factor


FGAIBase::FGAIBase()
 :  fp( NULL ),
    model( NULL ),
    props( NULL ),
    manager( NULL )
{
    _type_str = "model";
    tgt_heading = tgt_altitude = tgt_speed = 0.0;
    tgt_roll = roll = tgt_pitch = tgt_yaw = tgt_vs = vs = pitch = 0.0;
    bearing = elevation = range = rdot = 0.0;
    x_shift = y_shift = rotation = 0.0;
    in_range = false;
    invisible = true;
    no_roll = true;
    life = 900;
    model_path = "";
    _otype = otNull;
    index = 0;
    delete_me = false;
}

FGAIBase::~FGAIBase() {
    // Unregister that one at the scenery manager
    if (globals->get_scenery()) {
        globals->get_scenery()->unregister_placement_transform(aip.getTransform());
        globals->get_scenery()->get_scene_graph()->removeKid(aip.getSceneGraph());
    }
    // unbind();
    SGPropertyNode *root = globals->get_props()->getNode("ai/models", true);
    root->removeChild(_type_str.c_str(), index);
    delete fp;
    fp = NULL;
    ssgDeRefDelete(model);
    model = 0;
}

void FGAIBase::update(double dt) {
    if (_otype == otStatic) return;
    if (_otype == otBallistic) CalculateMach();

    ft_per_deg_lat = 366468.96 - 3717.12 * cos(pos.lat()*SGD_DEGREES_TO_RADIANS);
    ft_per_deg_lon = 365228.16 * cos(pos.lat()*SGD_DEGREES_TO_RADIANS);
}

void FGAIBase::Transform() {
    if (!invisible) {
      aip.setPosition(pos.lon(), pos.lat(), pos.elev() * SG_METER_TO_FEET);
      if (no_roll) {
         aip.setOrientation(0.0, pitch, hdg);
      } else {
         aip.setOrientation(roll, pitch, hdg);
      }
      aip.update();    
    }
}


bool FGAIBase::init() {

   SGPropertyNode *root = globals->get_props()->getNode("ai/models", true);

   index = manager->getNum(_otype) - 1;
   props = root->getNode(_type_str.c_str(), index, true);

   if (model_path != "") {
    try {
      model = load3DModel( globals->get_fg_root(),
	                     SGPath(model_path).c_str(),
                             props,
	                     globals->get_sim_time_sec() );
    } catch (const sg_exception &e) {
       model = NULL;
    }
   }
   if (model) {
     aip.init( model );
     aip.setVisible(true);
     invisible = false;
     globals->get_scenery()->get_scene_graph()->addKid(aip.getSceneGraph());
     // Register that one at the scenery manager
     globals->get_scenery()->register_placement_transform(aip.getTransform());
   } else {
     if (model_path != "") { 
       SG_LOG(SG_INPUT, SG_WARN, "AIBase: Could not load model " << model_path);
     }
   } 

   setDie(false);

   return true;
}


ssgBranch * FGAIBase::load3DModel(const string& fg_root, 
				  const string &path,
				  SGPropertyNode *prop_root, 
				  double sim_time_sec)
{
  // some more code here to check whether a model with this name has already been loaded
  // if not load it, otherwise, get the memory pointer and do something like 
  // SetModel as in ATC/AIEntity.cxx
  //SSGBranch *model;
  model = manager->getModel(path);
  if (!(model))
    {
      model = sgLoad3DModel(fg_root,
			    path,
			    prop_root,
			    sim_time_sec);
      manager->setModel(path, model);
      model->ref();
    }
  //else
  //  {
  //    model->ref();
  //    aip.init(model);
  //    aip.setVisible(false);
  //    globals->get_scenery()->get_scene_graph()->addKid(aip.getSceneGraph());
  // do some setModel stuff.
  return model;
}

bool FGAIBase::isa( object_type otype ) {
 if ( otype == _otype ) { return true; }
 else { return false; } 
}


void FGAIBase::bind() {
   props->tie("id", SGRawValueMethods<FGAIBase,int>(*this,
                                         &FGAIBase::_getID));
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

   props->tie("radar/in-range", SGRawValuePointer<bool>(&in_range));
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
   props->setBoolValue("controls/glide-path", true);
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

    props->untie("radar/in-range");
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

double FGAIBase::UpdateRadar(FGAIManager* manager)
{
   double radar_range_ft2 = fgGetDouble("/instrumentation/radar/range");
   radar_range_ft2 *= SG_NM_TO_METER * SG_METER_TO_FEET * 1.1; // + 10%
   radar_range_ft2 *= radar_range_ft2;

   double user_latitude  = manager->get_user_latitude();
   double user_longitude = manager->get_user_longitude();
   double lat_range = fabs(pos.lat() - user_latitude) * ft_per_deg_lat;
   double lon_range = fabs(pos.lon() - user_longitude) * ft_per_deg_lon;
   double range_ft2 = lat_range*lat_range + lon_range*lon_range;

   //
   // Test whether the target is within radar range.
   //
   in_range = (range_ft2 && (range_ft2 <= radar_range_ft2));
   if ( in_range )
   {
     props->setBoolValue("radar/in-range", true);

     // copy values from the AIManager
     double user_altitude  = manager->get_user_altitude();
     double user_heading   = manager->get_user_heading();
     double user_pitch     = manager->get_user_pitch();
     double user_yaw       = manager->get_user_yaw();
     double user_speed     = manager->get_user_speed();

     // calculate range to target in feet and nautical miles
     double range_ft = sqrt( range_ft2 );
     range = range_ft / 6076.11549;

     // calculate bearing to target
     if (pos.lat() >= user_latitude) {
        bearing = atan2(lat_range, lon_range) * SG_RADIANS_TO_DEGREES;
        if (pos.lon() >= user_longitude) {
           bearing = 90.0 - bearing;
        } else {
           bearing = 270.0 + bearing;
        }
     } else {
        bearing = atan2(lon_range, lat_range) * SG_RADIANS_TO_DEGREES;
        if (pos.lon() >= user_longitude) {
           bearing = 180.0 - bearing;
        } else {
           bearing = 180.0 + bearing;
        }
     }

     // calculate look left/right to target, without yaw correction
     horiz_offset = bearing - user_heading;
     if (horiz_offset > 180.0) horiz_offset -= 360.0;
     if (horiz_offset < -180.0) horiz_offset += 360.0;

     // calculate elevation to target
     elevation = atan2( altitude * SG_METER_TO_FEET - user_altitude, range_ft )
                        * SG_RADIANS_TO_DEGREES;

     // calculate look up/down to target
     vert_offset = elevation + user_pitch;

     /* this calculation needs to be fixed, but it isn't important anyway
     // calculate range rate
     double recip_bearing = bearing + 180.0;
     if (recip_bearing > 360.0) recip_bearing -= 360.0;
     double my_horiz_offset = recip_bearing - hdg;
     if (my_horiz_offset > 180.0) my_horiz_offset -= 360.0;
     if (my_horiz_offset < -180.0) my_horiz_offset += 360.0;
     rdot = (-user_speed * cos( horiz_offset * SG_DEGREES_TO_RADIANS ))
             +(-speed * 1.686 * cos( my_horiz_offset * SG_DEGREES_TO_RADIANS ));
*/

     // now correct look left/right for yaw
     horiz_offset += user_yaw;

     // calculate values for radar display
     y_shift = range * cos( horiz_offset * SG_DEGREES_TO_RADIANS);
     x_shift = range * sin( horiz_offset * SG_DEGREES_TO_RADIANS);
     rotation = hdg - user_heading;
     if (rotation < 0.0) rotation += 360.0;

   }

   return range_ft2;
}

Point3D
FGAIBase::getCartPosAt(const Point3D& off) const
{
  // The offset converted to the usual body fixed coordinate system.
  sgdVec3 sgdOff;
  sgdSetVec3(sgdOff, -off.x(), off.z(), -off.y());

  // Transform that one to the horizontal local coordinate system.
  sgdMat4 hlTrans;
  sgdMakeRotMat4(hlTrans, hdg, pitch, roll);
  sgdXformPnt3(sgdOff, hlTrans);

  // Now transform to the wgs84 earth centeres system.
  Point3D pos2(pos.lon()* SGD_DEGREES_TO_RADIANS,
               pos.lat() * SGD_DEGREES_TO_RADIANS,
               pos.elev());
  Point3D cartPos3D = sgGeodToCart(pos2);
  sgdMat4 ecTrans;
  sgdMakeCoordMat4(ecTrans, cartPos3D.x(), cartPos3D.y(), cartPos3D.z(),
                   pos.lon(), 0, - 90 - pos.lat());
  sgdXformPnt3(sgdOff, ecTrans);

  return Point3D(sgdOff[0], sgdOff[1], sgdOff[2]);
}

Point3D
FGAIBase::getGeocPosAt(const Point3D& off) const
{
  return sgCartToGeod(getCartPosAt(off));
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

int FGAIBase::_getID() const {
    return (int)(this);
}

void FGAIBase::CalculateMach() {
     // Calculate rho at altitude, using standard atmosphere
     // For the temperature T and the pressure p,
 
     if (altitude < 36152) {		// curve fits for the troposphere
       T = 59 - 0.00356 * altitude;
       p = 2116 * pow( ((T + 459.7) / 518.6) , 5.256);
 
     } else if ( 36152 < altitude && altitude < 82345 ) {    // lower stratosphere
       T = -70;
       p = 473.1 * pow( e , 1.73 - (0.000048 * altitude) );
 
     } else {                                    //  upper stratosphere
       T = -205.05 + (0.00164 * altitude);
       p = 51.97 * pow( ((T + 459.7) / 389.98) , -11.388);
     }
 
     rho = p / (1718 * (T + 459.7));
 	
 	// calculate the speed of sound at altitude
 	// a = sqrt ( g * R * (T + 459.7))
 	// where:
 	// a = speed of sound [ft/s]
 	// g = specific heat ratio, which is usually equal to 1.4  
 	// R = specific gas constant, which equals 1716 ft-lb/slug/°R 
 	
 	a = sqrt ( 1.4 * 1716 * (T + 459.7));
 	
 	// calculate Mach number
 	
 	Mach = speed/a;
 	
 //	cout  << "Speed(ft/s) "<< speed <<" Altitude(ft) "<< altitude << " Mach " << Mach;
}

