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
}

FGAIBallistic::~FGAIBallistic() {
}


bool FGAIBallistic::init() {
   FGAIBase::init();
   vs = sin( elevation * 0.017453293 ) * speed;
   hs = cos( elevation * 0.017453293 ) * speed;
   aero_stabilized =  true;
   hdg = azimuth;
   pitch = elevation;
   return true;
}

void FGAIBallistic::bind() {
    FGAIBase::bind();
}

void FGAIBallistic::unbind() {
    FGAIBase::unbind();
}

void FGAIBallistic::update(double dt) {
   FGAIBase::update(dt);
   Run(dt);
   Transform();
}


void FGAIBallistic::setAzimuth(double az) {
   azimuth = az;
}


void FGAIBallistic::setElevation(double el) {
   elevation = el;
}


void FGAIBallistic::setStabilization(bool val) {
   aero_stabilized = val;
}


void FGAIBallistic::Run(double dt) {

   double speed_north_deg_sec;
   double speed_east_deg_sec;
   double ft_per_deg_lon;
   double ft_per_deg_lat;

   // get size of a degree at this latitude
   ft_per_deg_lat = 366468.96 - 3717.12 * cos(pos.lat()/SG_RADIANS_TO_DEGREES);
   ft_per_deg_lon = 365228.16 * cos(pos.lat() / SG_RADIANS_TO_DEGREES);

   // the two drag calculations below assume sea-level density, 
   // mass of 0.03 slugs,  drag coeff of 0.295, frontal area of 0.007 ft2 
   // adjust horizontal speed due to drag 
   hs -= 0.000082 * hs * hs * dt;
   if ( hs < 0.0 ) hs = 0.0;

   // adjust vertical speed due to drag
   if (vs > 0.0) {
     vs -= 0.000082 * vs * vs * dt;
   } else {
     vs += 0.000082 * vs * vs * dt;
   }
   
   // convert horizontal speed (fps) to degrees per second
   speed_north_deg_sec = cos(hdg / SG_RADIANS_TO_DEGREES) * hs / ft_per_deg_lat;
   speed_east_deg_sec  = sin(hdg / SG_RADIANS_TO_DEGREES) * hs / ft_per_deg_lon;

   // set new position
   pos.setlat( pos.lat() + speed_north_deg_sec * dt);
   pos.setlon( pos.lon() + speed_east_deg_sec * dt); 

   // adjust vertical speed for acceleration of gravity
   vs -= 32.17 * dt;

   // adjust altitude (meters)
   altitude += vs * dt * SG_FEET_TO_METER;
   pos.setelev(altitude); 

   // adjust pitch if aerostabilized
   if (aero_stabilized) pitch = atan2( vs, hs ) * SG_RADIANS_TO_DEGREES;

   // set destruction flag if altitude less than sea level -1000
   if (altitude < -1000.0) setDie(true);

}

