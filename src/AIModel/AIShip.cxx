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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/math/point3d.hxx>
#include <math.h>

#include "AIFlightPlan.hxx"
#include "AIShip.hxx"


FGAIShip::FGAIShip(object_type ot) : FGAIBase(ot) {
}

FGAIShip::~FGAIShip() {
}

void FGAIShip::readFromScenario(SGPropertyNode* scFileNode) {
  if (!scFileNode)
    return;

  FGAIBase::readFromScenario(scFileNode);

  setRudder(scFileNode->getFloatValue("rudder", 0.0));
  setName(scFileNode->getStringValue("name", "Titanic"));
  
  std::string flightplan = scFileNode->getStringValue("flightplan");
  if (!flightplan.empty()){
    FGAIFlightPlan* fp = new FGAIFlightPlan(flightplan);
    setFlightPlan(fp);
  }
}

bool FGAIShip::init() {
   
   hdg_lock = false;
   rudder = 0.0;
   no_roll = false;
    
   rudder_constant = 0.5;
   roll_constant = 0.001;
   speed_constant = 0.05;
   hdg_constant = 0.01;
   
   return FGAIBase::init();
}

void FGAIShip::bind() {
    FGAIBase::bind();

    props->tie("surface-positions/rudder-pos-deg",
                SGRawValuePointer<float>(&rudder));
    props->tie("controls/heading-lock",
                SGRawValuePointer<bool>(&hdg_lock));
    props->tie("controls/tgt-speed-kts",
                SGRawValuePointer<double>(&tgt_speed));
    props->tie("controls/tgt-heading-degs",
                SGRawValuePointer<double>(&tgt_heading)); 
    props->tie("controls/constants/rudder",
                SGRawValuePointer<double>(&rudder_constant));
    props->tie("controls/constants/roll",
                SGRawValuePointer<double>(&roll_constant));
    props->tie("controls/constants/rudder",
                SGRawValuePointer<double>(&rudder_constant));
    props->tie("controls/constants/speed",
                SGRawValuePointer<double>(&speed_constant)); 

    props->setStringValue("name", name.c_str());
}

void FGAIShip::unbind() {
    FGAIBase::unbind();
    props->untie("surface-positions/rudder-pos-deg");
    props->untie("controls/heading-lock");
    props->untie("controls/tgt-speed-kts");
    props->untie("controls/tgt-heading-degs");
    props->untie("controls/constants/roll");
    props->untie("controls/constants/rudder");
    props->untie("controls/constants/speed");           

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
   double rudder_limit;
   double raw_roll;   

   // adjust speed
   double speed_diff = tgt_speed - speed;
   if (fabs(speed_diff) > 0.1) {
     if (speed_diff > 0.0) speed += speed_constant * dt;
     if (speed_diff < 0.0) speed -= speed_constant * dt;
   } 
   
   // convert speed to degrees per second
   speed_north_deg_sec = cos( hdg / SGD_RADIANS_TO_DEGREES )
                          * speed * 1.686 / ft_per_deg_lat;
   speed_east_deg_sec  = sin( hdg / SGD_RADIANS_TO_DEGREES )
                          * speed * 1.686 / ft_per_deg_lon;

   // set new position
   pos.setLatitudeDeg( pos.getLatitudeDeg() + speed_north_deg_sec * dt);
   pos.setLongitudeDeg( pos.getLongitudeDeg() + speed_east_deg_sec * dt); 

   
   // adjust heading based on current rudder angle
   if (rudder <= -0.25 || rudder >= 0.25)  {
   /*  turn_radius_ft = 0.088362 * speed * speed
                       / tan( fabs(rudder) / SG_RADIANS_TO_DEGREES );
     turn_circum_ft = SGD_2PI * turn_radius_ft;
     dist_covered_ft = speed * 1.686 * dt; 
     alpha = dist_covered_ft / turn_circum_ft * 360.0;*/
     
     if (turn_radius_ft <= 0) turn_radius_ft = 0; // don't allow nonsense values
     if (rudder > 45) rudder = 45;
     if (rudder < -45) rudder = -45;

// adjust turn radius for speed. The equation is very approximate.
     sp_turn_radius_ft = 10 * pow ((speed - 15),2) + turn_radius_ft;
//     cout << " speed turn radius " << sp_turn_radius_ft ; 

// adjust turn radius for rudder angle. The equation is even more approximate.     
     float a = 19;
     float b = -0.2485;
     float c = 0.543;
     
     rd_turn_radius_ft = (a * exp(b * fabs(rudder)) + c) * sp_turn_radius_ft;
     
//     cout <<" rudder turn radius " << rd_turn_radius_ft << endl;
          
// calculate the angle, alpha, subtended by the arc traversed in time dt        
     alpha = ((speed * 1.686 * dt)/rd_turn_radius_ft) * SG_RADIANS_TO_DEGREES;

   
// make sure that alpha is applied in the right direction   
     hdg += alpha * sign( rudder );
     if ( hdg > 360.0 ) hdg -= 360.0;
     if ( hdg < 0.0) hdg += 360.0;

//adjust roll for rudder angle and speed. Another bit of voodoo    
     raw_roll =  -0.0166667 * speed * rudder;
   }
   else
   {
// rudder angle is 0  
     raw_roll = 0;
//     cout << " roll "<< roll << endl;
   }

    //low pass filter
     roll = (raw_roll * roll_constant) + (roll * (1 - roll_constant));
         
     /*cout  << " rudder: " << rudder << " raw roll: "<< raw_roll<<" roll: " << roll ;
     cout  << " hdg: " << hdg << endl ;*/

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
     if (diff < 15){ 
         tgt_rudder = diff * rudder_sense;
         }
         else
         {
         tgt_rudder = 45 * rudder_sense;
     }     
   }

   // adjust rudder angle
    double rudder_diff = tgt_rudder - rudder;
    // set the rudder limit by speed
    if (speed <= 40 ){
       rudder_limit = (-0.825 * speed) + 35;
    }else{
       rudder_limit = 2;
   }

    if (fabs(rudder_diff) > 0.1) {
        if (rudder_diff > 0.0){
            rudder += rudder_constant * dt;
            if (rudder > rudder_limit) rudder = rudder_limit;// apply the rudder limit
        } else if (rudder_diff < 0.0){
            rudder -= rudder_constant * dt;
            if (rudder < -rudder_limit) rudder = -rudder_limit;
        }
}



}//end function


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

void FGAIShip::setName(const string& n) {
  name = n;
}

void FGAIShip::ProcessFlightPlan(double dt) {
  // not implemented yet
}

void FGAIShip::setRudder(float r) {
  rudder = r;
}

void FGAIShip::setRoll(double rl) {
   roll = rl;
}
