// FGAICarrier - AIShip-derived class creates an AI aircraft carrier
//
// Written by David Culp, started October 2004.
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

#ifndef _FG_AICARRIER_HXX
#define _FG_AICARRIER_HXX

#include <string>
#include <list>
#include <plib/ssg.h>
#include <simgear/compiler.h>

SG_USING_STD(string);
SG_USING_STD(list);

#include "AIShip.hxx"
class FGAIManager;
class FGAICarrier;

class FGAICarrierHardware : public ssgBase {
public:

  enum Type { Catapult, Wire, Solid };

  FGAICarrier *carrier;
  int id;
  Type type;

  static FGAICarrierHardware* newCatapult(FGAICarrier *c) {
    FGAICarrierHardware* ch = new FGAICarrierHardware;
    ch->carrier = c;
    ch->type = Catapult;
    ch->id = unique_id++;
    return ch;
  }
  static FGAICarrierHardware* newWire(FGAICarrier *c) {
    FGAICarrierHardware* ch = new FGAICarrierHardware;
    ch->carrier = c;
    ch->type = Wire;
    ch->id = unique_id++;
    return ch;
  }
  static FGAICarrierHardware* newSolid(FGAICarrier *c) {
    FGAICarrierHardware* ch = new FGAICarrierHardware;
    ch->carrier = c;
    ch->type = Solid;
    ch->id = unique_id++;
    return ch;
  }

private:
  static int unique_id;
};

class FGAICarrier  : public FGAIShip {
	
public:
	
	FGAICarrier(FGAIManager* mgr);
	~FGAICarrier();

        void setSolidObjects(const list<string>& solid_objects);
        void setWireObjects(const list<string>& wire_objects);
        void setCatapultObjects(const list<string>& catapult_objects);

	void getVelocityWrtEarth(sgVec3 v);
	virtual void bind();
    virtual void unbind();
    void UpdateFlols ( double dt );
	
	bool init();

private:

	void update(double dt);
	void mark_nohot(ssgEntity*);
	bool mark_wires(ssgEntity*, const list<string>&, bool = false);
	bool mark_cat(ssgEntity*, const list<string>&, bool = false);
	bool mark_solid(ssgEntity*, const list<string>&, bool = false);

	list<string> solid_objects;       // List of solid object names
	list<string> wire_objects;        // List of wire object names
	list<string> catapult_objects;    // List of catapult object names

	// Velocity wrt earth.
	sgVec3 vel_wrt_earth;
    
    float trans[3][3];
    float in[3];
    float out[3];

    double Rx, Ry, Rz;
    double Sx, Sy, Sz;
    double Tx, Ty, Tz;

    float cosRx, sinRx;
    float cosRy, sinRy;
    float cosRz, sinRz;
        
    double flolsXYZ[3], eyeXYZ[3]; 
    double lat, lon, alt;
    double dist, angle;
    int source;
    
    Point3D eyepos;
    Point3D flolspos;	
};

#endif  // _FG_AICARRIER_HXX
