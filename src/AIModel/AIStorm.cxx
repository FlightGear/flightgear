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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/math/point3d.hxx>
#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Scenery/scenery.hxx>
#include <string>
#include <math.h>

SG_USING_STD(string);

#include "AIStorm.hxx"


FGAIStorm *FGAIStorm::_self = NULL;

FGAIStorm::FGAIStorm(FGAIManager* mgr) {
   manager = mgr;   
   _self = this;
   _type_str = "thunderstorm";
   _otype = otStorm;
}


FGAIStorm::~FGAIStorm() {
    _self = NULL;
}


bool FGAIStorm::init() {
   return FGAIBase::init();
}

void FGAIStorm::bind() {
    FGAIBase::bind();
}

void FGAIStorm::unbind() {
    FGAIBase::unbind();
}


void FGAIStorm::update(double dt) {
   Run(dt);
   Transform();
   FGAIBase::update(dt);
}


void FGAIStorm::Run(double dt) {

   FGAIStorm::dt = dt;
	
   double speed_north_deg_sec;
   double speed_east_deg_sec;
   double ft_per_deg_lon;
   double ft_per_deg_lat;

   // get size of a degree at this latitude
   ft_per_deg_lat = 366468.96 - 3717.12 * cos(pos.lat()/SG_RADIANS_TO_DEGREES);
   ft_per_deg_lon = 365228.16 * cos(pos.lat() / SG_RADIANS_TO_DEGREES);

   // convert speed to degrees per second
   speed_north_deg_sec = cos( hdg / SG_RADIANS_TO_DEGREES )
                          * speed * 1.686 / ft_per_deg_lat;
   speed_east_deg_sec  = sin( hdg / SG_RADIANS_TO_DEGREES )
                          * speed * 1.686 / ft_per_deg_lon;

   // set new position
   pos.setlat( pos.lat() + speed_north_deg_sec * dt);
   pos.setlon( pos.lon() + speed_east_deg_sec * dt); 

   double altitude_ft = altitude * SG_METER_TO_FEET;

   //###########################//
   // do calculations for radar //
   //###########################//

   // copy values from the AIManager
   double user_latitude  = manager->get_user_latitude();
   double user_longitude = manager->get_user_longitude();
   double user_altitude  = manager->get_user_altitude();
   double user_heading   = manager->get_user_heading();
   double user_pitch     = manager->get_user_pitch();
   double user_yaw       = manager->get_user_yaw();
   // double user_speed     = manager->get_user_speed();

   // calculate range to target in feet and nautical miles
   double lat_range = fabs(pos.lat() - user_latitude) * ft_per_deg_lat;
   double lon_range = fabs(pos.lon() - user_longitude) * ft_per_deg_lon;
   double range_ft = sqrt( lat_range*lat_range + lon_range*lon_range );
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
   elevation = atan2( altitude_ft - user_altitude, range_ft )
                      * SG_RADIANS_TO_DEGREES;
   
   // calculate look up/down to target
   vert_offset = elevation + user_pitch;

   // now correct look left/right for yaw
   horiz_offset += user_yaw;

   // calculate values for radar display
   y_shift = range * cos( horiz_offset * SG_DEGREES_TO_RADIANS);
   x_shift = range * sin( horiz_offset * SG_DEGREES_TO_RADIANS);
   rotation = hdg - user_heading;
   if (rotation < 0.0) rotation += 360.0; 

}

