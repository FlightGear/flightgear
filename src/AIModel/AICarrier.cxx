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

#include "AICarrier.hxx"


FGAICarrier::FGAICarrier(FGAIManager* mgr) : FGAIShip(mgr) {
}

FGAICarrier::~FGAICarrier() {
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

void FGAICarrier::getVelocityWrtEarth(sgVec3 v) {
  sgCopyVec3(v, vel_wrt_earth );
}

void FGAICarrier::update(double dt) {
   FGAIShip::update(dt);

   // Update the velocity information stored in those nodes.
   double v_north = 0.51444444*speed*cos(hdg * SGD_DEGREES_TO_RADIANS);
   double v_east  = 0.51444444*speed*sin(hdg * SGD_DEGREES_TO_RADIANS);

   double sin_lat = sin(pos.lat() * SGD_DEGREES_TO_RADIANS);
   double cos_lat = cos(pos.lat() * SGD_DEGREES_TO_RADIANS);
   double sin_lon = sin(pos.lon() * SGD_DEGREES_TO_RADIANS);
   double cos_lon = cos(pos.lon() * SGD_DEGREES_TO_RADIANS);
   sgSetVec3( vel_wrt_earth,
              - cos_lon*sin_lat*v_north - sin_lon*v_east,
              - sin_lon*sin_lat*v_north + cos_lon*v_east,
                cos_lat*v_north );

}

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

   return true;
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

bool FGAICarrier::mark_wires(ssgEntity* e, const list<string>& wire_objects) {
  bool found = false;
  if (e->isAKindOf(ssgTypeBranch())) {

    ssgBranch* br = (ssgBranch*)e;
    ssgEntity* kid;
    for ( kid = br->getKid(0); kid != NULL ; kid = br->getNextKid() )
      found = mark_wires(kid, wire_objects) || found;

    if (found)
      br->setTraversalMaskBits(SSGTRAV_HOT);
    
  } else if (e->isAKindOf(ssgTypeLeaf())) {
    list<string>::const_iterator it;
    for (it = wire_objects.begin(); it != wire_objects.end(); ++it) {
      if (e->getName() && (*it) == e->getName()) {
        e->setTraversalMaskBits(SSGTRAV_HOT);
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
  return found;
}

bool FGAICarrier::mark_solid(ssgEntity* e, const list<string>& solid_objects) {
  bool found = false;
  if (e->isAKindOf(ssgTypeBranch())) {
    ssgBranch* br = (ssgBranch*)e;
    ssgEntity* kid;
    for ( kid = br->getKid(0); kid != NULL ; kid = br->getNextKid() )
      found = mark_solid(kid, solid_objects) || found;

    if (found)
      br->setTraversalMaskBits(SSGTRAV_HOT);
    
  } else if (e->isAKindOf(ssgTypeLeaf())) {
    list<string>::const_iterator it;
    for (it = solid_objects.begin(); it != solid_objects.end(); ++it) {
      if (e->getName() && (*it) == e->getName()) {
        e->setTraversalMaskBits(SSGTRAV_HOT);
        e->setUserData( FGAICarrierHardware::newSolid( this ) );
        found = true;
      }
    }
  }
  return found;
}

bool FGAICarrier::mark_cat(ssgEntity* e, const list<string>& cat_objects) {
  bool found = false;
  if (e->isAKindOf(ssgTypeBranch())) {
    ssgBranch* br = (ssgBranch*)e;
    ssgEntity* kid;
    for ( kid = br->getKid(0); kid != NULL ; kid = br->getNextKid() )
      found = mark_cat(kid, cat_objects) || found;

    if (found)
      br->setTraversalMaskBits(SSGTRAV_HOT);
    
  } else if (e->isAKindOf(ssgTypeLeaf())) {
    list<string>::const_iterator it;
    for (it = cat_objects.begin(); it != cat_objects.end(); ++it) {
      if (e->getName() && (*it) == e->getName()) {
        e->setTraversalMaskBits(SSGTRAV_HOT);
        e->setUserData( FGAICarrierHardware::newCatapult( this ) );
        ssgLeaf *l = (ssgLeaf*)e;
        if ( l->getNumLines() != 1 ) {
          SG_LOG(SG_GENERAL, SG_ALERT,
                 "AICarrier: Found a cat not modelled with exactly one line!");
        }
        // Now some special code to make shure the cat points in the right
        // direction. The 0 index must be the backward end, the 1 index
        // the forward end.
        // Forward is positive x-direction in our 3D model, also the model
        // as such is flattened when it is loaded, so we do not need to care
        // for transforms ...
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
  return found;
}

int FGAICarrierHardware::unique_id = 1;
