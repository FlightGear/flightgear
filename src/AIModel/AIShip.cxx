// FGAIShip - FGAIBase-derived class creates an AI ship
//
// Written by David Culp, started October 2003.
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

#include "AIShip.hxx"


FGAIShip::FGAIShip(FGAIManager* mgr) {
   manager = mgr;
   _type_str = "ship";
   _otype = otShip;

   hdg_lock = false;
   rudder = 0.0;
}

FGAIShip::~FGAIShip() {
}


bool FGAIShip::init() {
   return FGAIBase::init();
}

void FGAIShip::bind() {
    FGAIBase::bind();

    props->tie("surface-positions/rudder-pos-deg",
                SGRawValuePointer<double>(&rudder));
}

void FGAIShip::unbind() {
    FGAIBase::unbind();
    props->untie("surface-positions/rudder-pos-deg");
}

void FGAIShip::update(double dt) {

   FGAIBase::update(dt);
   Run(dt);
   Transform();
}



void FGAIShip::Run(double dt) {

   if (fp) ProcessFlightPlan(dt);

   double sp_turn_radius_ft;
   double rd_turn_radius_ft;
   double speed_north_deg_sec;
   double speed_east_deg_sec;
   double dist_covered_ft;
   double alpha;

   // adjust speed
   double speed_diff = tgt_speed - speed;
   if (fabs(speed_diff) > 0.1) {
     if (speed_diff > 0.0) speed += 0.1 * dt;
     if (speed_diff < 0.0) speed -= 0.1 * dt;
   } 
   
   // convert speed to degrees per second
   speed_north_deg_sec = cos( hdg / SGD_RADIANS_TO_DEGREES )
                          * speed * 1.686 / ft_per_deg_lat;
   speed_east_deg_sec  = sin( hdg / SGD_RADIANS_TO_DEGREES )
                          * speed * 1.686 / ft_per_deg_lon;

   // set new position
   pos.setlat( pos.lat() + speed_north_deg_sec * dt);
   pos.setlon( pos.lon() + speed_east_deg_sec * dt); 

   
   // adjust heading based on current rudder angle
   if (rudder != 0.0)  {
   /*  turn_radius_ft = 0.088362 * speed * speed
                       / tan( fabs(rudder) / SG_RADIANS_TO_DEGREES );
     turn_circum_ft = SGD_2PI * turn_radius_ft;
     dist_covered_ft = speed * 1.686 * dt; 
     alpha = dist_covered_ft / turn_circum_ft * 360.0;*/
     
     if (turn_radius_ft <= 0) turn_radius_ft = 0; // don't allow nonsense values
     
//     cout << "speed " << speed << " turn radius " << turn_radius_ft << endl;

// adjust turn radius for speed. The equation is very approximate.
     sp_turn_radius_ft = 10 * pow ((speed - 15),2) + turn_radius_ft;
//     cout << "speed " << speed << " speed turn radius " << sp_turn_radius_ft << endl; 

// adjust turn radius for rudder angle. The equation is even more approximate.     
     rd_turn_radius_ft = -130 * (rudder - 15) + sp_turn_radius_ft;
//     cout << "rudder " << rudder << " rudder turn radius " << rd_turn_radius_ft << endl;
          
// calculate the angle, alpha, subtended by the arc traversed in time dt        
     alpha = ((speed * 1.686 * dt)/rd_turn_radius_ft) * SG_RADIANS_TO_DEGREES;

// make sure that alpha is applied in the right direction   
   
     hdg += alpha * sign( rudder );

     if ( hdg > 360.0 ) hdg -= 360.0;
     if ( hdg < 0.0) hdg += 360.0;

//adjust roll for rudder angle and speed     
     roll = - (  speed / 2 - rudder / 6 );
     
//    cout << " hdg " << hdg  << "roll "<< roll << endl;
   }

   // adjust target rudder angle if heading lock engaged
   if (hdg_lock) {
     double rudder_sense = 0.0;
     double diff = fabs(hdg - tgt_heading);
     if (diff > 180) diff = fabs(diff - 360);
     double sum = hdg + diff;
     if (sum > 360.0) sum -= 360.0;
     if (fabs(sum - tgt_heading) < 1.0) { 
       rudder_sense = 1.0;
     } else {
       rudder_sense = -1.0;
     } 
     if (diff < 30) tgt_roll = diff * rudder_sense; 
   }

   // adjust rudder angle
   double rudder_diff = tgt_roll - rudder;
   if (fabs(rudder_diff) > 0.1) {
     if (rudder_diff > 0.0) rudder += 5.0 * dt;
     if (rudder_diff < 0.0) rudder -= 5.0 * dt;
   }

}


void FGAIShip::AccelTo(double speed) {
   tgt_speed = speed;
}


void FGAIShip::PitchTo(double angle) {
   tgt_pitch = angle;
}


void FGAIShip::RollTo(double angle) {
   tgt_roll = angle;
}


void FGAIShip::YawTo(double angle) {
}


void FGAIShip::ClimbTo(double altitude) {
}


void FGAIShip::TurnTo(double heading) {
   tgt_heading = heading;
   hdg_lock = true;
}


double FGAIShip::sign(double x) {

  if ( x < 0.0 ) { return -1.0; }
  else { return 1.0; }
}

void FGAIShip::setFlightPlan(FGAIFlightPlan* f) {
  fp = f;
}

void FGAIShip::ProcessFlightPlan(double dt) {
  // not implemented yet
}

