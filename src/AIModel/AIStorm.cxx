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


FGAIStorm::FGAIStorm(FGAIManager* mgr) {
   manager = mgr;   
   _type_str = "storm";
   _otype = otStorm;
}


FGAIStorm::~FGAIStorm() {
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
   FGAIBase::update(dt);
   Run(dt);
   Transform();
}


void FGAIStorm::Run(double dt) {

   FGAIStorm::dt = dt;
	
   double speed_north_deg_sec;
   double speed_east_deg_sec;

   // convert speed to degrees per second
   speed_north_deg_sec = cos( hdg / SG_RADIANS_TO_DEGREES )
                          * speed * 1.686 / ft_per_deg_lat;
   speed_east_deg_sec  = sin( hdg / SG_RADIANS_TO_DEGREES )
                          * speed * 1.686 / ft_per_deg_lon;

   // set new position
   pos.setlat( pos.lat() + speed_north_deg_sec * dt);
   pos.setlon( pos.lon() + speed_east_deg_sec * dt); 

   //###########################//
   // do calculations for radar //
   //###########################//
   UpdateRadar(manager);
}

