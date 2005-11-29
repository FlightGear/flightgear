// FGAICarrier - FGAIShip-derived class creates an AI aircraft carrier
//
// Written by David Culp, started October 2004.
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

#include <string>
#include <vector>

#include <simgear/math/point3d.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <math.h>
#include <Main/util.hxx>
#include <Main/viewer.hxx>

#include "AICarrier.hxx"

#include "AIScenario.hxx"

/** Value of earth radius (meters) */
#define RADIUS_M   SG_EQUATORIAL_RADIUS_M



FGAICarrier::FGAICarrier(FGAIManager* mgr) : FGAIShip(mgr) {
  _type_str = "carrier";
  _otype = otCarrier;
  
  
}

FGAICarrier::~FGAICarrier() {
}

void FGAICarrier::setWind_from_east(double fps) {
   wind_from_east = fps;
}

void FGAICarrier::setWind_from_north(double fps) {
   wind_from_north = fps;
}

void FGAICarrier::setMaxLat(double deg) {
   max_lat = fabs(deg);
}

void FGAICarrier::setMinLat(double deg) {
   min_lat = fabs(deg);
}

void FGAICarrier::setMaxLong(double deg) {
   max_long = fabs(deg);
}

void FGAICarrier::setMinLong(double deg) {
   min_long = fabs(deg);
}

void FGAICarrier::setSolidObjects(const list<string>& so) {
  solid_objects = so;
}

void FGAICarrier::setWireObjects(const list<string>& wo) {
  wire_objects = wo;
}

void FGAICarrier::setCatapultObjects(const list<string>& co) {
  catapult_objects = co;
}

void FGAICarrier::setParkingPositions(const list<ParkPosition>& p) {
  ppositions = p;
}

void FGAICarrier::setSign(const string& s) {
  sign = s;
}

void FGAICarrier::setTACANChannelID(const string& id) {
   TACAN_channel_id = id;
}

void FGAICarrier::setFlolsOffset(const Point3D& off) {
  flols_off = off;
}

void FGAICarrier::getVelocityWrtEarth(sgdVec3& v, sgdVec3& omega, sgdVec3& pivot) {
  sgdCopyVec3(v, vel_wrt_earth );
  sgdCopyVec3(omega, rot_wrt_earth );
  sgdCopyVec3(pivot, rot_pivot_wrt_earth );
}

void FGAICarrier::update(double dt) {
   
   // For computation of rotation speeds we just use finite differences her.
   // That is perfectly valid since this thing is not driven by accelerations
   // but by just apply discrete changes at its velocity variables.
   double old_hdg = hdg;
   double old_roll = roll;
   double old_pitch = pitch;

   // Update the velocity information stored in those nodes.
   double v_north = 0.51444444*speed*cos(hdg * SGD_DEGREES_TO_RADIANS);
   double v_east  = 0.51444444*speed*sin(hdg * SGD_DEGREES_TO_RADIANS);

   double sin_lat = sin(pos.lat() * SGD_DEGREES_TO_RADIANS);
   double cos_lat = cos(pos.lat() * SGD_DEGREES_TO_RADIANS);
   double sin_lon = sin(pos.lon() * SGD_DEGREES_TO_RADIANS);
   double cos_lon = cos(pos.lon() * SGD_DEGREES_TO_RADIANS);
   double sin_roll = sin(roll * SGD_DEGREES_TO_RADIANS);
   double cos_roll = cos(roll * SGD_DEGREES_TO_RADIANS);
   double sin_pitch = sin(pitch * SGD_DEGREES_TO_RADIANS);
   double cos_pitch = cos(pitch * SGD_DEGREES_TO_RADIANS);
   double sin_hdg = sin(hdg * SGD_DEGREES_TO_RADIANS);
   double cos_hdg = cos(hdg * SGD_DEGREES_TO_RADIANS);

   // Transform this back the the horizontal local frame.
   sgdMat3 trans;
   
   // set up the transform matrix
   trans[0][0] =          cos_pitch*cos_hdg;
   trans[0][1] = sin_roll*sin_pitch*cos_hdg - cos_roll*sin_hdg;
   trans[0][2] = cos_roll*sin_pitch*cos_hdg + sin_roll*sin_hdg;
   
   trans[1][0] =          cos_pitch*sin_hdg;
   trans[1][1] = sin_roll*sin_pitch*sin_hdg + cos_roll*cos_hdg;
   trans[1][2] = cos_roll*sin_pitch*sin_hdg - sin_roll*cos_hdg;
   
   trans[2][0] =         -sin_pitch;
   trans[2][1] = sin_roll*cos_pitch;
   trans[2][2] = cos_roll*cos_pitch;
   
   sgdSetVec3( vel_wrt_earth,
              - cos_lon*sin_lat*v_north - sin_lon*v_east,
              - sin_lon*sin_lat*v_north + cos_lon*v_east,
                cos_lat*v_north );
   sgGeodToCart(pos.lat() * SGD_DEGREES_TO_RADIANS,
                pos.lon() * SGD_DEGREES_TO_RADIANS,
                pos.elev(), rot_pivot_wrt_earth);

   // Now update the position and heading. This will compute new hdg and
   // roll values required for the rotation speed computation.
   FGAIShip::update(dt);
   
   
   //automatic turn into wind with a target wind of 25 kts otd
   if(turn_to_launch_hdg){
       TurnToLaunch();
   } else if(OutsideBox() || returning) {// check that the carrier is inside the operating box
       ReturnToBox();
   } else {                   //if(!returning
       TurnToBase();
   }  //end if  

   // Only change these values if we are able to compute them safely
   if (dt < DBL_MIN)
     sgdSetVec3( rot_wrt_earth, 0.0, 0.0, 0.0);
   else {
     // Compute the change of the euler angles.
     double hdg_dot = SGD_DEGREES_TO_RADIANS * (hdg-old_hdg)/dt;
     // Allways assume that the movement was done by the shorter way.
     if (hdg_dot < - SGD_DEGREES_TO_RADIANS * 180)
       hdg_dot += SGD_DEGREES_TO_RADIANS * 360;
     if (hdg_dot > SGD_DEGREES_TO_RADIANS * 180)
       hdg_dot -= SGD_DEGREES_TO_RADIANS * 360;
     double pitch_dot = SGD_DEGREES_TO_RADIANS * (pitch-old_pitch)/dt;
     // Allways assume that the movement was done by the shorter way.
     if (pitch_dot < - SGD_DEGREES_TO_RADIANS * 180)
       pitch_dot += SGD_DEGREES_TO_RADIANS * 360;
     if (pitch_dot > SGD_DEGREES_TO_RADIANS * 180)
       pitch_dot -= SGD_DEGREES_TO_RADIANS * 360;
     double roll_dot = SGD_DEGREES_TO_RADIANS * (roll-old_roll)/dt;
     // Allways assume that the movement was done by the shorter way.
     if (roll_dot < - SGD_DEGREES_TO_RADIANS * 180)
       roll_dot += SGD_DEGREES_TO_RADIANS * 360;
     if (roll_dot > SGD_DEGREES_TO_RADIANS * 180)
       roll_dot -= SGD_DEGREES_TO_RADIANS * 360;
     /*cout << "euler derivatives = "
          << roll_dot << " " << pitch_dot << " " << hdg_dot << endl;*/

     // Now Compute the rotation vector in the carriers coordinate frame
     // originating from the euler angle changes.
     sgdVec3 body;
     body[0] = roll_dot - hdg_dot*sin_pitch;
     body[1] = pitch_dot*cos_roll + hdg_dot*sin_roll*cos_pitch;
     body[2] = -pitch_dot*sin_roll + hdg_dot*cos_roll*cos_pitch;

     // Transform that back to the horizontal local frame.
     sgdVec3 hl;
     hl[0] = body[0]*trans[0][0] + body[1]*trans[0][1] + body[2]*trans[0][2];
     hl[1] = body[0]*trans[1][0] + body[1]*trans[1][1] + body[2]*trans[1][2];
     hl[2] = body[0]*trans[2][0] + body[1]*trans[2][1] + body[2]*trans[2][2];

     // Now we need to project out rotation components ending in speeds in y
     // direction in the hoirizontal local frame.
     hl[1] = 0;

     // Transform that to the earth centered frame.
     sgdSetVec3(rot_wrt_earth,
               - cos_lon*sin_lat*hl[0] - sin_lon*hl[1] - cos_lat*cos_lon*hl[2],
               - sin_lon*sin_lat*hl[0] + cos_lon*hl[1] - cos_lat*sin_lon*hl[2],
                 cos_lat*hl[0] - sin_lat*hl[2]);
   }

   UpdateWind(dt);
   UpdateFlols(trans);
   UpdateElevator(dt, transition_time);
} //end update

bool FGAICarrier::init() {
   if (!FGAIShip::init())
      return false;

   // process the 3d model here
   // mark some objects solid, mark the wires ...

   // The model should be used for altitude computations.
   // To avoid that every detail in a carrier 3D model will end into
   // the aircraft local cache, only set the HOT traversal bit on
   // selected objects.
   ssgEntity *sel = aip.getSceneGraph();
   // Clear the HOT traversal flag
   mark_nohot(sel);
   // Selectively set that flag again for wires/cats/solid objects.
   // Attach a pointer to this carrier class to those objects.
   mark_wires(sel, wire_objects);
   mark_cat(sel, catapult_objects);
   mark_solid(sel, solid_objects);

   _longitude_node = fgGetNode("/position/longitude-deg", true);
   _latitude_node = fgGetNode("/position/latitude-deg", true);
   _altitude_node = fgGetNode("/position/altitude-ft", true);
//   _elevator_node = fgGetNode("/controls/elevators", true);

   _surface_wind_from_deg_node = 
              fgGetNode("/environment/config/boundary/entry[0]/wind-from-heading-deg", true);
   _surface_wind_speed_node = 
              fgGetNode("/environment/config/boundary/entry[0]/wind-speed-kt", true);
   
  
   turn_to_launch_hdg = false;
   returning = false;
  
   initialpos = pos;
   base_course = hdg;
   base_speed = speed;
   
   step = 0;
   pos_norm = 0;
   elevators = false;
   transition_time = 150;
   time_constant = 0.005;


   return true;
}

void FGAICarrier::bind() {
   FGAIShip::bind();

   props->untie("velocities/true-airspeed-kt");
   
   props->tie("controls/flols/source-lights",
                SGRawValuePointer<int>(&source));
   props->tie("controls/flols/distance-m",
                SGRawValuePointer<double>(&dist));
   props->tie("controls/flols/angle-degs",
                SGRawValuePointer<double>(&angle));
   props->tie("controls/turn-to-launch-hdg",
                SGRawValuePointer<bool>(&turn_to_launch_hdg));
   props->tie("controls/in-to-wind",
                SGRawValuePointer<bool>(&turn_to_launch_hdg));
   props->tie("controls/base-course-deg",
                SGRawValuePointer<double>(&base_course));
   props->tie("controls/base-speed-kts",
                SGRawValuePointer<double>(&base_speed));
   props->tie("controls/start-pos-lat-deg",
                SGRawValuePointer<double>(&initialpos[1]));
   props->tie("controls/start-pos-long-deg",
                SGRawValuePointer<double>(&initialpos[0]));
   props->tie("velocities/speed-kts",  
                SGRawValuePointer<double>(&speed));
   props->tie("environment/surface-wind-speed-true-kts",  
                SGRawValuePointer<double>(&wind_speed_kts));
   props->tie("environment/surface-wind-from-true-degs",  
                SGRawValuePointer<double>(&wind_from_deg));
   props->tie("environment/rel-wind-from-degs",  
                SGRawValuePointer<double>(&rel_wind_from_deg));
   props->tie("environment/rel-wind-from-carrier-hdg-degs",  
                SGRawValuePointer<double>(&rel_wind));
   props->tie("environment/rel-wind-speed-kts",  
                SGRawValuePointer<double>(&rel_wind_speed_kts));
   props->tie("controls/flols/wave-off-lights",  
                SGRawValuePointer<bool>(&wave_off_lights));
   props->tie("controls/elevators",
                SGRawValuePointer<bool>(&elevators));
   props->tie("surface-positions/elevators-pos-norm",
                SGRawValuePointer<double>(&pos_norm));
   props->tie("controls/elevators-trans-time-s",
                SGRawValuePointer<double>(&transition_time));
   props->tie("controls/elevators-time-constant",
                SGRawValuePointer<double>(&time_constant));
                    
   props->setBoolValue("controls/flols/cut-lights", false);
   props->setBoolValue("controls/flols/wave-off-lights", false);
   props->setBoolValue("controls/flols/cond-datum-lights", true);
   props->setBoolValue("controls/crew", false);
   props->setStringValue("navaids/tacan/channel-ID", TACAN_channel_id.c_str());
   props->setStringValue("sign", sign.c_str());
}

void FGAICarrier::unbind() {
    FGAIShip::unbind();
    
    props->untie("velocities/true-airspeed-kt");
    
    props->untie("controls/flols/source-lights");
    props->untie("controls/flols/distance-m");
    props->untie("controls/flols/angle-degs");
    props->untie("controls/turn-to-launch-hdg");
    props->untie("velocities/speed-kts");
    props->untie("environment/wind-speed-true-kts");
    props->untie("environment/wind-from-true-degs");
    props->untie("environment/rel-wind-from-degs");
    props->untie("environment/rel-wind-speed-kts");
    props->untie("controls/flols/wave-off-lights");
    props->untie("controls/elevators");
    props->untie("surface-positions/elevators-pos-norm");
    props->untie("controls/elevators-trans-time-secs");
    props->untie("controls/elevators-time-constant");
}


bool FGAICarrier::getParkPosition(const string& id, Point3D& geodPos,
                                  double& hdng, sgdVec3 uvw)
{

  // FIXME: does not yet cover rotation speeds.
  list<ParkPosition>::iterator it = ppositions.begin();
  while (it != ppositions.end()) {
    // Take either the specified one or the first one ...
    if ((*it).name == id || id.empty()) {
      ParkPosition ppos = *it;
      geodPos = getGeocPosAt(ppos.offset);
      hdng = hdg + ppos.heading_deg;
      double shdng = sin(ppos.heading_deg * SGD_DEGREES_TO_RADIANS);
      double chdng = cos(ppos.heading_deg * SGD_DEGREES_TO_RADIANS);
      double speed_fps = speed*1.6878099;
      sgdSetVec3(uvw, chdng*speed_fps, shdng*speed_fps, 0);
      return true;
    }
    ++it;
  }

  return false;
}

void FGAICarrier::mark_nohot(ssgEntity* e) {
  if (e->isAKindOf(ssgTypeBranch())) {
    ssgBranch* br = (ssgBranch*)e;
    ssgEntity* kid;
    for ( kid = br->getKid(0); kid != NULL ; kid = br->getNextKid() )
      mark_nohot(kid);

    br->clrTraversalMaskBits(SSGTRAV_HOT);
    
  } else if (e->isAKindOf(ssgTypeLeaf())) {

    e->clrTraversalMaskBits(SSGTRAV_HOT);

  }
}

bool FGAICarrier::mark_wires(ssgEntity* e, const list<string>& wire_objects, bool mark) {
  bool found = false;
  if (e->isAKindOf(ssgTypeBranch())) {
    ssgBranch* br = (ssgBranch*)e;
    ssgEntity* kid;

    list<string>::const_iterator it;
    for (it = wire_objects.begin(); it != wire_objects.end(); ++it)
      mark = mark || (e->getName() && (*it) == e->getName());

    for ( kid = br->getKid(0); kid != NULL ; kid = br->getNextKid() )
      found = mark_wires(kid, wire_objects, mark) || found;

    if (found)
      br->setTraversalMaskBits(SSGTRAV_HOT);
    
  } else if (e->isAKindOf(ssgTypeLeaf())) {
    list<string>::const_iterator it;
    for (it = wire_objects.begin(); it != wire_objects.end(); ++it) {
      if (mark || (e->getName() && (*it) == e->getName())) {
        e->setTraversalMaskBits(SSGTRAV_HOT);
        ssgBase* ud = e->getUserData();
        if (ud) {
          FGAICarrierHardware* ch = dynamic_cast<FGAICarrierHardware*>(ud);
          if (ch) {
            SG_LOG(SG_GENERAL, SG_WARN,
                   "AICarrier: Carrier hardware gets marked twice!\n"
                   "           You have propably a whole branch marked as"
                   " a wire which also includes other carrier hardware."
                   );
          } else {
            SG_LOG(SG_GENERAL, SG_ALERT,
                   "AICarrier: Found user data attached to a leaf node which "
                   "should be marked as a wire!\n    ****Skipping!****");
          }
        } else {
          e->setUserData( FGAICarrierHardware::newWire( this ) );
          ssgLeaf *l = (ssgLeaf*)e;
          if ( l->getNumLines() != 1 ) {
            SG_LOG(SG_GENERAL, SG_ALERT,
                   "AICarrier: Found wires not modelled with exactly one line!");
          }
          found = true;
        }
      }
    }
  }
  return found;
}

bool FGAICarrier::mark_solid(ssgEntity* e, const list<string>& solid_objects, bool mark) {
  bool found = false;
  if (e->isAKindOf(ssgTypeBranch())) {
    ssgBranch* br = (ssgBranch*)e;
    ssgEntity* kid;

    list<string>::const_iterator it;
    for (it = solid_objects.begin(); it != solid_objects.end(); ++it)
      mark = mark || (e->getName() && (*it) == e->getName());

    for ( kid = br->getKid(0); kid != NULL ; kid = br->getNextKid() )
      found = mark_solid(kid, solid_objects, mark) || found;

    if (found)
      br->setTraversalMaskBits(SSGTRAV_HOT);
    
  } else if (e->isAKindOf(ssgTypeLeaf())) {
    list<string>::const_iterator it;
    for (it = solid_objects.begin(); it != solid_objects.end(); ++it) {
      if (mark || (e->getName() && (*it) == e->getName())) {
        e->setTraversalMaskBits(SSGTRAV_HOT);
        ssgBase* ud = e->getUserData();
        if (ud) {
          FGAICarrierHardware* ch = dynamic_cast<FGAICarrierHardware*>(ud);
          if (ch) {
            SG_LOG(SG_GENERAL, SG_WARN,
                   "AICarrier: Carrier hardware gets marked twice!\n"
                   "           You have propably a whole branch marked solid"
                   " which also includes other carrier hardware."
                   );
          } else {
            SG_LOG(SG_GENERAL, SG_ALERT,
                   "AICarrier: Found user data attached to a leaf node which "
                   "should be marked solid!\n    ****Skipping!****");
          }
        } else {
          e->setUserData( FGAICarrierHardware::newSolid( this ) );
          found = true;
        }
      }
    }
  }
  return found;
}

bool FGAICarrier::mark_cat(ssgEntity* e, const list<string>& cat_objects, bool mark) {
  bool found = false;
  if (e->isAKindOf(ssgTypeBranch())) {
    ssgBranch* br = (ssgBranch*)e;
    ssgEntity* kid;

    list<string>::const_iterator it;
    for (it = cat_objects.begin(); it != cat_objects.end(); ++it)
      mark = mark || (e->getName() && (*it) == e->getName());

    for ( kid = br->getKid(0); kid != NULL ; kid = br->getNextKid() )
      found = mark_cat(kid, cat_objects, mark) || found;

    if (found)
      br->setTraversalMaskBits(SSGTRAV_HOT);
    
  } else if (e->isAKindOf(ssgTypeLeaf())) {
    list<string>::const_iterator it;
    for (it = cat_objects.begin(); it != cat_objects.end(); ++it) {
      if (mark || (e->getName() && (*it) == e->getName())) {
        e->setTraversalMaskBits(SSGTRAV_HOT);
        ssgBase* ud = e->getUserData();
        if (ud) {
          FGAICarrierHardware* ch = dynamic_cast<FGAICarrierHardware*>(ud);
          if (ch) {
            SG_LOG(SG_GENERAL, SG_WARN,
                   "AICarrier: Carrier hardware gets marked twice!\n"
                   "You have probably a whole branch marked as"
                   "a catapult which also includes other carrier hardware."
                    );
          } else {
            SG_LOG(SG_GENERAL, SG_ALERT,
                   "AICarrier: Found user data attached to a leaf node which "
                   "should be marked as a catapult!\n    ****Skipping!****");
          }
        } else {
          e->setUserData( FGAICarrierHardware::newCatapult( this ) );
          ssgLeaf *l = (ssgLeaf*)e;
          if ( l->getNumLines() != 1 ) {
            SG_LOG(SG_GENERAL, SG_ALERT,
                   "AICarrier: Found a cat not modelled with exactly "
                   "one line!");
          } else {
            // Now some special code to make sure the cat points in the right
            // direction. The 0 index must be the backward end, the 1 index
            // the forward end.
            // Forward is positive x-direction in our 3D model, also the model
            // as such is flattened when it is loaded, so we do not need to
            // care for transforms ...
            short v[2];
            l->getLine(0, v, v+1 );
            sgVec3 ends[2];
            for (int k=0; k<2; ++k)
              sgCopyVec3( ends[k], l->getVertex( v[k] ) );
            
            // When the 1 end is behind the 0 end, swap the coordinates.
            if (ends[0][0] < ends[1][0]) {
              sgCopyVec3( l->getVertex( v[0] ), ends[1] );
              sgCopyVec3( l->getVertex( v[1] ), ends[0] );
            }

            found = true;
          }
        }
      }
    }
  }
  return found;
}

void FGAICarrier::UpdateFlols(const sgdMat3& trans) {
    
    float in[3];
    float out[3];

    double flolsXYZ[3], eyeXYZ[3]; 
    double lat, lon, alt;
    Point3D eyepos;
    Point3D flolspos;	

/*    cout << "x_offset " << flols_x_offset 
          << " y_offset " << flols_y_offset 
          << " z_offset " << flols_z_offset << endl;
        
     cout << "roll " << roll 
          << " heading " << hdg
          << " pitch " << pitch << endl;
        
     cout << "carrier lon " << pos[0] 
          << " lat " <<  pos[1]
          << " alt " << pos[2] << endl;*/
        
// set the Flols intitial position to the carrier position
 
  flolspos = pos;
  
/*  cout << "flols lon " << flolspos[0] 
          << " lat " <<  flolspos[1]
          << " alt " << flolspos[2] << endl;*/
          
// set the offsets in metres

/*  cout << "flols_x_offset " << flols_x_offset << endl
       << "flols_y_offset " << flols_y_offset << endl
       << "flols_z_offset " << flols_z_offset << endl;*/
     
  in[0] = flols_off.x();  
  in[1] = flols_off.y();
  in[2] = flols_off.z();    

// multiply the input and transform matrices

   out[0] = in[0] * trans[0][0] + in[1] * trans[0][1] + in[2] * trans[0][2];
   out[1] = in[0] * trans[1][0] + in[1] * trans[1][1] + in[2] * trans[1][2];
   out[2] = in[0] * trans[2][0] + in[1] * trans[2][1] + in[2] * trans[2][2];
 
// convert meters to ft to degrees of latitude
   out[0] = (out[0] * 3.28083989501) /(366468.96 - 3717.12 * cos(flolspos[0] * SG_DEGREES_TO_RADIANS));

// convert meters to ft to degrees of longitude
   out[1] = (out[1] * 3.28083989501)/(365228.16 * cos(flolspos[1] * SG_DEGREES_TO_RADIANS));

//print out the result
/*   cout  << "lat adjust deg" << out[0] 
        << " lon adjust deg " << out[1] 
        << " alt adjust m " << out[2]  << endl;*/

// adjust Flols position    
   flolspos[0] += out[0];
   flolspos[1] += out[1];
   flolspos[2] += out[2];   

// convert flols position to cartesian co-ordinates 

  sgGeodToCart(flolspos[1] * SG_DEGREES_TO_RADIANS,
               flolspos[0] * SG_DEGREES_TO_RADIANS,
               flolspos[2] , flolsXYZ );


/*  cout << "flols X " << flolsXYZ[0] 
       << " Y " <<  flolsXYZ[1]
       << " Z " << flolsXYZ[2] << endl; 

// check the conversion
         
  sgCartToGeod(flolsXYZ, &lat, &lon, &alt);
 
  cout << "flols check lon " << lon   
        << " lat " << lat 
        << " alt " << alt << endl;      */
               
//get the current position of the pilot's eyepoint (cartesian cordinates)

  sgdCopyVec3( eyeXYZ, globals->get_current_view()->get_absolute_view_pos() );
  
 /* cout  << "Eye_X "  << eyeXYZ[0] 
        << " Eye_Y " << eyeXYZ[1] 
        << " Eye_Z " << eyeXYZ[2]  << endl; */
        
  sgCartToGeod(eyeXYZ, &lat, &lon, &alt);
  
  eyepos[0] = lon * SG_RADIANS_TO_DEGREES;
  eyepos[1] = lat * SG_RADIANS_TO_DEGREES;
  eyepos[2] = alt;
  
/*  cout << "eye lon " << eyepos[0]
        << " eye lat " << eyepos[1] 
        << " eye alt " << eyepos[2] << endl; */

//calculate the ditance from eye to flols
      
  dist = sgdDistanceVec3( flolsXYZ, eyeXYZ );

//apply an index error

  dist -= 100;
  
  //cout << "distance " << dist << endl; 
  
  if ( dist < 5000 ) {
       // calculate height above FLOLS 
       double y = eyepos[2] - flolspos[2];
       
       // calculate the angle from the flols to eye
       // above the horizontal
       // double angle;
       
       if ( dist != 0 ) {
           angle = asin( y / dist );
         } else {
           angle = 0.0;
         }
        
       angle *= SG_RADIANS_TO_DEGREES;
        
      
  // cout << " height " << y << " angle " << angle ;

// set the value of source  
        
       if ( angle <= 4.35 && angle > 4.01 )
         { source = 1; }
         else if ( angle <= 4.01 && angle > 3.670 )
         { source = 2; }
         else if ( angle <= 3.670 && angle > 3.330 )
         { source = 3; }
         else if ( angle <= 3.330 && angle > 2.990 )
         { source = 4; }
         else if ( angle <= 2.990 && angle > 2.650 )
         { source = 5; }
         else if ( angle <= 2.650  )
         { source = 6; }
         else
         { source = 0; }
         
//         cout << " source " << source << endl;
                     
   }   
} // end updateflols

// find relative wind




void FGAICarrier::UpdateWind( double dt) {

    double recip;
    
    //calculate the reciprocal hdg
    
    if (hdg >= 180){
        recip = hdg - 180;
    }
    else{
        recip = hdg + 180;
    }
    
    //cout <<" heading: " << hdg << "recip: " << recip << endl;
    
    //get the surface wind speed and direction
    wind_from_deg = _surface_wind_from_deg_node->getDoubleValue();
    wind_speed_kts  = _surface_wind_speed_node->getDoubleValue();
    
    //calculate the surface wind speed north and east in kts   
    double wind_speed_from_north_kts = cos( wind_from_deg / SGD_RADIANS_TO_DEGREES )* wind_speed_kts ;
    double wind_speed_from_east_kts  = sin( wind_from_deg / SGD_RADIANS_TO_DEGREES )* wind_speed_kts ;
    
    //calculate the carrier speed north and east in kts   
    double speed_north_kts = cos( hdg / SGD_RADIANS_TO_DEGREES )* speed ;
    double speed_east_kts  = sin( hdg / SGD_RADIANS_TO_DEGREES )* speed ;
    
    //calculate the relative wind speed north and east in kts
    double rel_wind_speed_from_east_kts = wind_speed_from_east_kts + speed_east_kts;
    double rel_wind_speed_from_north_kts = wind_speed_from_north_kts + speed_north_kts;
    
    //combine relative speeds north and east to get relative windspeed in kts                          
    rel_wind_speed_kts = sqrt((rel_wind_speed_from_east_kts * rel_wind_speed_from_east_kts) 
    + (rel_wind_speed_from_north_kts * rel_wind_speed_from_north_kts));
    
    //calculate the relative wind direction
    rel_wind_from_deg = atan(rel_wind_speed_from_east_kts/rel_wind_speed_from_north_kts) 
                            * SG_RADIANS_TO_DEGREES;
    
    // rationalise the output
    if (rel_wind_speed_from_north_kts <= 0){
        rel_wind_from_deg = 180 + rel_wind_from_deg;
    }
    else{
        if(rel_wind_speed_from_east_kts <= 0){
            rel_wind_from_deg = 360 + rel_wind_from_deg;
        }    
    }
    
    //calculate rel wind
    rel_wind = rel_wind_from_deg - hdg  ;
    if (rel_wind > 180) rel_wind -= 360;
    
    //switch the wave-off lights
    if (InToWind()){
       wave_off_lights = false;
    }else{
       wave_off_lights = true;
    }    
       
    // cout << "rel wind: " << rel_wind << endl;

}// end update wind

void FGAICarrier::TurnToLaunch(){
    
     //calculate tgt speed
       double tgt_speed = 25 - wind_speed_kts;
       if (tgt_speed < 10) tgt_speed = 10;
       
     //turn the carrier
       FGAIShip::TurnTo(wind_from_deg); 
       FGAIShip::AccelTo(tgt_speed); 
           
     
        
}  // end turn to launch
   
void FGAICarrier::TurnToBase(){
    
    //turn the carrier
       FGAIShip::TurnTo(base_course); 
       FGAIShip::AccelTo(base_speed); 
    
} //  end turn to base  

void FGAICarrier::ReturnToBox(){
    double course, distance, az2;
        
    //get the carrier position
    carrierpos = pos;
    
    //cout << "lat: " << carrierpos[1] << " lon: " << carrierpos[0] << endl;
    
    //calculate the bearing and range of the initial position from the carrier
    geo_inverse_wgs_84(carrierpos[2],
                       carrierpos[1],
                       carrierpos[0],
                       initialpos[1],
                       initialpos[0],
                       &course, &az2, &distance);
                     
    distance *= SG_METER_TO_NM;

    //cout << "return course: " << course << " distance: " << distance << endl;
    //turn the carrier
       FGAIShip::TurnTo(course); 
       FGAIShip::AccelTo(base_speed);
       if (distance >= 1 ){
           returning = true;
       }else{
           returning = false;
       }        
    
} //  end turn to base  
 
    
                         

bool FGAICarrier::OutsideBox(){ //returns true if the carrier is outside operating box

    if ( max_lat == 0 && min_lat == 0 && max_long == 0 && min_long == 0) {
       SG_LOG(SG_GENERAL, SG_DEBUG, "AICarrier: No Operating Box defined" );
       return false;
    }        
     
    if (initialpos[1] >= 0){//northern hemisphere
        if (pos[1] >= initialpos[1] + max_lat) {return true;}
        else if (pos[1] <= initialpos[1] - min_lat) {return true;}
    }else{                  //southern hemisphere
        if (pos[1] <= initialpos[1] - max_lat) {return true;}
        else if (pos[1] >= initialpos[1] + min_lat) {return true;}
    }
    
    if (initialpos[0] >=0) {//eastern hemisphere
        if (pos[0] >= initialpos[0] + max_long) {return true;}
        else if (pos[0] <= initialpos[0] - min_long) {return true;}
    }else{                 //western hemisphere
        if (pos[0] <= initialpos[0] - max_long) {return true;}
        else if (pos[0] >= initialpos[0] + min_long) {return true;}
    }
    
    SG_LOG(SG_GENERAL, SG_DEBUG, "AICarrier: Inside Operating Box" );
   
    return false;   

} // end OutsideBox

// return the distance to the horizon, given the altitude and the radius of the earth
float FGAICarrier::Horizon(float h) { return RADIUS_M * acos(RADIUS_M / (RADIUS_M + h)); }
    
bool FGAICarrier::InToWind(){
    
    // test
    if ( fabs(rel_wind) < 5 ) return true;
    return false;
    
} //end InToWind     

void FGAICarrier::UpdateElevator(double dt, double transition_time) {

    if ((elevators && pos_norm >= 1 ) || (!elevators && pos_norm <= 0 ))
        return;

    // move the elevators
    if ( elevators ) {
        step += dt/transition_time;
        if ( step > 1 )
            step = 1;

    } else {
        step -= dt/transition_time;
        if ( step < 0 )
            step = 0;
    }
    // assume a linear relationship
    raw_pos_norm = step;
    if (raw_pos_norm >= 1) {
        raw_pos_norm = 1;
    } else if (raw_pos_norm <= 0) {
        raw_pos_norm = 0;
    }

    //low pass filter
    pos_norm = (raw_pos_norm * time_constant) + (pos_norm * (1 - time_constant));
    return;

} // end UpdateElevator


int FGAICarrierHardware::unique_id = 1;
