// FGAIBallistic - FGAIBase-derived class creates a ballistic object
//
// Written by David Culp, started November 2003.
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

#include <simgear/math/point3d.hxx>
#include <math.h>

#include "AIBallistic.hxx"


FGAIBallistic::FGAIBallistic(FGAIManager* mgr) {
    manager = mgr;
    _type_str = "ballistic";
    _otype = otBallistic;
    drag_area = 0.007;
    life_timer = 0.0;
	gravity = 32;
//	buoyancy = 64;
    no_roll = false;
	}

FGAIBallistic::~FGAIBallistic() {
}


bool FGAIBallistic::init() {
   FGAIBase::init();
   aero_stabilized = true;
   hdg = azimuth;
   pitch = elevation;
   roll = rotation;
   Transform();
   return true;
}

void FGAIBallistic::bind() {
//    FGAIBase::bind();
   props->tie("sim/time/elapsed-sec",
               SGRawValueMethods<FGAIBallistic,double>(*this,
                                          &FGAIBallistic::_getTime));
}

void FGAIBallistic::unbind() {
//    FGAIBase::unbind();
   props->untie("sim/time/elapsed-sec");
}

void FGAIBallistic::update(double dt) {
   FGAIBase::update(dt);
   Run(dt);
   Transform();
}


void FGAIBallistic::setAzimuth(double az) {
   hdg = azimuth = az;
}


void FGAIBallistic::setElevation(double el) {
   pitch = elevation = el;
}

void FGAIBallistic::setRoll(double rl) {
   rotation = rl;
}

void FGAIBallistic::setStabilization(bool val) {
   aero_stabilized = val;
}

void FGAIBallistic::setDragArea(double a) {
   drag_area = a;
}

void FGAIBallistic::setLife(double seconds) {
   life = seconds;
}

void FGAIBallistic::setBuoyancy(double fpss) {
   buoyancy = fpss;
}

void FGAIBallistic::setWind_from_east(double fps) {
   wind_from_east = fps;
}

void FGAIBallistic::setWind_from_north(double fps) {
   wind_from_north = fps;
}

void FGAIBallistic::setWind(bool val) {
   wind = val;
}
void FGAIBallistic::Run(double dt) {

   life_timer += dt;
   if (life_timer > life) setDie(true); 

   double speed_north_deg_sec;
   double speed_east_deg_sec;
   double wind_speed_from_north_deg_sec;
   double wind_speed_from_east_deg_sec;
      
   // the two drag calculations below assume sea-level density, 
   // mass of 0.03 slugs,  drag coeff of 0.295
   // adjust speed due to drag 
   speed -= 0.0116918 * drag_area * speed * speed * dt;
   if ( speed < 0.0 ) speed = 0.0;
   vs = sin( pitch * SG_DEGREES_TO_RADIANS ) * speed;
   hs = cos( pitch * SG_DEGREES_TO_RADIANS ) * speed;

   // convert horizontal speed (fps) to degrees per second
   speed_north_deg_sec = cos(hdg / SG_RADIANS_TO_DEGREES) * hs / ft_per_deg_lat;
   speed_east_deg_sec  = sin(hdg / SG_RADIANS_TO_DEGREES) * hs / ft_per_deg_lon;
   
   // convert wind speed (fps) to degrees per second
   
   if (!wind){
      wind_from_north = 0;
      wind_from_east = 0;
	  }

   wind_speed_from_north_deg_sec = wind_from_north / ft_per_deg_lat;
   wind_speed_from_east_deg_sec  = wind_from_east / ft_per_deg_lon;
   
 
   // set new position
//   pos.setlat( pos.lat() + (speed_north_deg_sec * dt)  );
//   pos.setlon( pos.lon() + (speed_east_deg_sec * dt)  ); 
 
 
   // set new position
   
   pos.setlat( pos.lat() + (speed_north_deg_sec - wind_speed_from_north_deg_sec) * dt );
   pos.setlon( pos.lon() + (speed_east_deg_sec - wind_speed_from_east_deg_sec) * dt ); 

   // adjust vertical speed for acceleration of gravity
   vs -= (gravity - buoyancy) * dt;
   
   // adjust altitude (feet)
   altitude += vs * dt;
   pos.setelev(altitude * SG_FEET_TO_METER); 

   // recalculate pitch (velocity vector) if aerostabilized
   if (aero_stabilized) pitch = atan2( vs, hs ) * SG_RADIANS_TO_DEGREES;

   // recalculate total speed
   speed = sqrt( vs * vs + hs * hs);

   // set destruction flag if altitude less than sea level -1000
   if (altitude < -1000.0) setDie(true);

}

double FGAIBallistic::_getTime() const {
   return life_timer;
}

// end AIBallistic