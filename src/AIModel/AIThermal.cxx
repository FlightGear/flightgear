// FGAIThermal - FGAIBase-derived class creates an AI thermal
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

using std::string;

#include "AIThermal.hxx"


FGAIThermal::FGAIThermal() : FGAIBase(otThermal) {
   max_strength = 6.0;
   diameter = 0.5;
   strength = factor = 0.0;
}

FGAIThermal::~FGAIThermal() {
}

void FGAIThermal::readFromScenario(SGPropertyNode* scFileNode) {
  if (!scFileNode)
    return;

  FGAIBase::readFromScenario(scFileNode);

  setMaxStrength(scFileNode->getDoubleValue("strength-fps", 8.0)); 
  setDiameter(scFileNode->getDoubleValue("diameter-ft", 0.0)/6076.11549);
  setHeight(scFileNode->getDoubleValue("height-msl", 5000.0));
}

bool FGAIThermal::init(bool search_in_AI_path) {
   factor = 8.0 * max_strength / (diameter * diameter * diameter);
   setAltitude( height );
   return FGAIBase::init(search_in_AI_path);
}

void FGAIThermal::bind() {
    FGAIBase::bind();
}

void FGAIThermal::unbind() {
    FGAIBase::unbind();
}


void FGAIThermal::update(double dt) {
   FGAIBase::update(dt);
   Run(dt);
   Transform();
}


void FGAIThermal::Run(double dt) {

   //###########################//
   // do calculations for range //
   //###########################//

   // copy values from the AIManager
   double user_latitude  = manager->get_user_latitude();
   double user_longitude = manager->get_user_longitude();
   double user_altitude  = manager->get_user_altitude();

   // calculate range to target in feet and nautical miles
   double lat_range = fabs(pos.getLatitudeDeg() - user_latitude) * ft_per_deg_lat;
   double lon_range = fabs(pos.getLongitudeDeg() - user_longitude) * ft_per_deg_lon;
   double range_ft = sqrt(lat_range*lat_range + lon_range*lon_range);
   range = range_ft / 6076.11549;

   // Calculate speed of rising air if within range. 
   // Air vertical speed is maximum at center of thermal,
   // and decreases to zero at the edge (as distance cubed).
   if (range < (diameter * 0.5)) {
     strength = max_strength - ( range * range * range * factor );
   } else {
     strength = 0.0;
   }

   // Stop lift at the top of the thermal (smoothly)
   if (user_altitude > (height + 100.0)) {
     strength = 0.0;
   }
   else if (user_altitude < height) {
     // do nothing
   }
   else {
     strength -= (strength * (user_altitude - height) * 0.01);
   }
}

