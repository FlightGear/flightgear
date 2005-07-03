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


FGAICarrier::FGAICarrier(FGAIManager* mgr) : FGAIShip(mgr) {
  _type_str = "carrier";
  _otype = otCarrier;
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

void FGAICarrier::setParkingPositions(const list<ParkPosition>& p) {
  ppositions = p;
}

void FGAICarrier::setSign(const string& s) {
  sign = s;
}

void FGAICarrier::setFlolsOffset(const Point3D& off) {
  flols_off = off;
}

void FGAICarrier::getVelocityWrtEarth(sgVec3 v) {
  sgCopyVec3(v, vel_wrt_earth );
}

void FGAICarrier::update(double dt) {
   UpdateFlols(dt);
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

void FGAICarrier::bind() {
   FGAIShip::bind();

   props->tie("controls/flols/source-lights",
                SGRawValuePointer<int>(&source));
   props->tie("controls/flols/distance-m",
                SGRawValuePointer<double>(&dist));
   props->tie("controls/flols/angle-degs",
                SGRawValuePointer<double>(&angle));
   props->setBoolValue("controls/flols/cut-lights", false);
   props->setBoolValue("controls/flols/wave-off-lights", false);
   props->setBoolValue("controls/flols/cond-datum-lights", true);
   props->setBoolValue("controls/crew", false);

   props->setStringValue("sign", sign.c_str());
}

void FGAICarrier::unbind() {
    FGAIShip::unbind();
    props->untie("controls/flols/source-lights");
    props->untie("controls/flols/distance-m");
    props->untie("controls/flols/angle-degs");
}

bool FGAICarrier::getParkPosition(const string& id, Point3D& geodPos,
                                  double& hdng, sgdVec3 uvw)
{
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

void FGAICarrier::UpdateFlols( double dt) {
    
    float trans[3][3];
    float in[3];
    float out[3];

    float cosRx, sinRx;
    float cosRy, sinRy;
    float cosRz, sinRz;
        
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

// pre-process the trig functions

    cosRx = cos(roll * SG_DEGREES_TO_RADIANS);
    sinRx = sin(roll * SG_DEGREES_TO_RADIANS);
    cosRy = cos(pitch * SG_DEGREES_TO_RADIANS);
    sinRy = sin(pitch * SG_DEGREES_TO_RADIANS);
    cosRz = cos(hdg * SG_DEGREES_TO_RADIANS);
    sinRz = sin(hdg * SG_DEGREES_TO_RADIANS);

// set up the transform matrix

    trans[0][0] =  cosRy * cosRz;
    trans[0][1] =  -1 * cosRx * sinRz + sinRx * sinRy * cosRz ;
    trans[0][2] =  sinRx * sinRz + cosRx * sinRy * cosRz;

    trans[1][0] =  cosRy * sinRz;
    trans[1][1] =  cosRx * cosRz + sinRx * sinRy * sinRz;
    trans[1][2] =  -1 * sinRx * cosRx + cosRx * sinRy * sinRz;

    trans[2][0] =  -1 * sinRy;
    trans[2][1] =  sinRx * cosRy;
    trans[2][2] =  cosRx * cosRy;

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

int FGAICarrierHardware::unique_id = 1;
