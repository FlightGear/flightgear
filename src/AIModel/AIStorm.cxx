// FGAIStorm - FGAIBase-derived class creates an AI thunderstorm or cloud
//
// Written by David Culp, started Feb 2004.
//
// Copyright (C) 2004  David P. Culp - davidculp2@comcast.net
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

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Scenery/scenery.hxx>
#include <string>
#include <math.h>
#include <cstdlib>
#include <time.h>

using std::string;

#include "AIStorm.hxx"


FGAIStorm::FGAIStorm() :
   FGAIBase(otStorm, false)
{
   delay = 3.6;
   subflashes = 1;
   timer = 0.0;
   random_delay = 3.6;
   flash_node = fgGetNode("/environment/lightning/flash", true);
   flash_node->setBoolValue(false);
   flashed = 0; 
   flashing = false;
   subflash_index = -1;
   subflash_array[0] =  1;
   subflash_array[1] =  2;
   subflash_array[2] =  1;
   subflash_array[3] =  3;
   subflash_array[4] =  2;
   subflash_array[5] =  1;
   subflash_array[6] =  1;
   subflash_array[7] =  2;

   turb_mag_node = fgGetNode("/environment/turbulence/magnitude-norm", true);
   turb_rate_node = fgGetNode("/environment/turbulence/rate-hz", true);
}


FGAIStorm::~FGAIStorm() {
}

void FGAIStorm::readFromScenario(SGPropertyNode* scFileNode) {
  if (!scFileNode)
    return;

  FGAIBase::readFromScenario(scFileNode);

  setDiameter(scFileNode->getDoubleValue("diameter-ft", 0.0)/6076.11549);
  setHeight(scFileNode->getDoubleValue("height-msl", 5000.0));
  setStrengthNorm(scFileNode->getDoubleValue("strength-norm", 1.0)); 
}

bool FGAIStorm::init(bool search_in_AI_path) {
   return FGAIBase::init(search_in_AI_path);
}

void FGAIStorm::bind() {
    FGAIBase::bind();
}

void FGAIStorm::unbind() {
    FGAIBase::unbind();
}


void FGAIStorm::update(double dt) {
   FGAIBase::update(dt);
   Run(dt);
   Transform();
}


void FGAIStorm::Run(double dt) {

   double speed_north_deg_sec;
   double speed_east_deg_sec;

   // convert speed to degrees per second
   speed_north_deg_sec = cos( hdg / SG_RADIANS_TO_DEGREES )
                          * speed * 1.686 / ft_per_deg_lat;
   speed_east_deg_sec  = sin( hdg / SG_RADIANS_TO_DEGREES )
                          * speed * 1.686 / ft_per_deg_lon;

   // set new position
   pos.setLatitudeDeg( pos.getLatitudeDeg() + speed_north_deg_sec * dt);
   pos.setLongitudeDeg( pos.getLongitudeDeg() + speed_east_deg_sec * dt); 

   // do calculations for weather radar display 
   UpdateRadar(manager);

   // **************************************************
   // *     do lightning                               *
   // **************************************************

   if (timer > random_delay) {
     srand((unsigned)time(0));
     random_delay = delay + (rand()%3) - 1.0;
     //cout << "random_delay = " << random_delay << endl;
     timer = 0.0;
     flashing = true;
     subflash_index++;
     if (subflash_index == 8) subflash_index = 0; 
     subflashes = subflash_array[subflash_index]; 
   }

   if (flashing) {
     if (flashed < subflashes) {
         timer += dt;
         if (timer < 0.1) { 
         flash_node->setBoolValue(true);
         } else {
            flash_node->setBoolValue(false);
            if (timer > 0.2) {
              timer = 0.0;
              flashed++;
            }
         }
     } else {
       flashing = false;
       timer = 0.0;
       flashed = 0;
     }
   }
   else {
     timer += dt;
   }  

   // ***************************************************
   // *      do turbulence                              *
   // ***************************************************

   // copy user's position from the AIManager
   double user_latitude  = manager->get_user_latitude();
   double user_longitude = manager->get_user_longitude();
   double user_altitude  = manager->get_user_altitude();

   // calculate range to target in feet and nautical miles
   double lat_range = fabs(pos.getLatitudeDeg() - user_latitude) * ft_per_deg_lat;
   double lon_range = fabs(pos.getLongitudeDeg() - user_longitude) * ft_per_deg_lon;
   double range_ft = sqrt(lat_range*lat_range + lon_range*lon_range);
   range = range_ft / 6076.11549;

   if (range < (diameter * 0.5) &&
       user_altitude > (altitude_ft - 1000.0) &&
       user_altitude < height) {
              turb_mag_node->setDoubleValue(strength_norm);
              turb_rate_node->setDoubleValue(0.5);
   } 
     
}

