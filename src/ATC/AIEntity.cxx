// FGAIEntity - abstract base class an artificial intelligence entity
//
// Written by David Luff, started March 2002.
//
// Copyright (C) 2002  David C. Luff - david.luff@nottingham.ac.uk
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

/*****************************************************************
*
* WARNING - Curt has some ideas about AI traffic so anything in here
* may get rewritten or scrapped.  Contact Curt curt@flightgear.org 
* before spending any time or effort on this code!!!
*
******************************************************************/

#include <Main/globals.hxx>
#include <Scenery/scenery.hxx>
//#include <simgear/constants.h>
#include <simgear/math/point3d.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/misc/sg_path.hxx>
#include <string>

#include "AIEntity.hxx"

FGAIEntity::~FGAIEntity() {
}

void FGAIEntity::Update(double dt) {
}

// Run the internal calculations
//void FGAIEntity::Update() {
void FGAIEntity::Transform() {
    aip.setPosition(pos.lon(), pos.lat(), pos.elev() * SG_METER_TO_FEET);
    aip.setOrientation(roll, pitch, hdg);
    aip.update();    
}

/*
void FGAIEntity::Transform() {

    // Translate moving object w.r.t eye
    Point3D sc = globals->get_scenery()->get_center();
    //cout << "sc0 = " << sc.x() << " sc1 = " << sc.y() << " sc2 = " << sc.z() << '\n';
    //cout << "op0 = " << obj_pos.x() << " op1 = " << obj_pos.y() << " op2 = " << obj_pos.z() << '\n';

    sgCoord shippos;
    FastWorldCoordinate(&shippos, sc);
    position->setTransform( &shippos );
    //cout << "Transform called\n";
}
*/

#if 0
// Taken from tileentry.cxx
void FGAIEntity::WorldCoordinate(sgCoord *obj_pos, Point3D center) {
    // setup transforms
    Point3D geod( pos.lon() * SGD_DEGREES_TO_RADIANS,
                  pos.lat() * SGD_DEGREES_TO_RADIANS,
                  pos.elev() );
	
    Point3D world_pos = sgGeodToCart( geod );
    Point3D offset = world_pos - center;
	
    sgMat4 POS;
    sgMakeTransMat4( POS, offset.x(), offset.y(), offset.z() );

    sgVec3 obj_rt, obj_up;
    sgSetVec3( obj_rt, 0.0, 1.0, 0.0); // Y axis
    sgSetVec3( obj_up, 0.0, 0.0, 1.0); // Z axis

    sgMat4 ROT_lon, ROT_lat, ROT_hdg;
    sgMakeRotMat4( ROT_lon, lon, obj_up );
    sgMakeRotMat4( ROT_lat, 90 - lat, obj_rt );
    sgMakeRotMat4( ROT_hdg, hdg, obj_up );

    sgMat4 TUX;
    sgCopyMat4( TUX, ROT_hdg );
    sgPostMultMat4( TUX, ROT_lat );
    sgPostMultMat4( TUX, ROT_lon );
    sgPostMultMat4( TUX, POS );

    sgSetCoord( obj_pos, TUX );
}
#endif
/*
// Norman's 'fast hack' for above
void FGAIEntity::FastWorldCoordinate(sgCoord *obj_pos, Point3D center) {
    double lon_rad = pos.lon() * SGD_DEGREES_TO_RADIANS;
    double lat_rad = pos.lat() * SGD_DEGREES_TO_RADIANS;
    double hdg_rad = hdg * SGD_DEGREES_TO_RADIANS;

    // setup transforms
    Point3D geod( lon_rad, lat_rad, pos.elev() );
	
    Point3D world_pos = sgGeodToCart( geod );
    Point3D offset = world_pos - center;

    sgMat4 mat;

    SGfloat sin_lat = (SGfloat)sin( lat_rad );
    SGfloat cos_lat = (SGfloat)cos( lat_rad );
    SGfloat cos_lon = (SGfloat)cos( lon_rad );
    SGfloat sin_lon = (SGfloat)sin( lon_rad );
    SGfloat sin_hdg = (SGfloat)sin( hdg_rad ) ;
    SGfloat cos_hdg = (SGfloat)cos( hdg_rad ) ;

    mat[0][0] =  cos_hdg * (SGfloat)sin_lat * (SGfloat)cos_lon - sin_hdg * (SGfloat)sin_lon;
    mat[0][1] =  cos_hdg * (SGfloat)sin_lat * (SGfloat)sin_lon + sin_hdg * (SGfloat)cos_lon;
    mat[0][2] =	-cos_hdg * (SGfloat)cos_lat;
    mat[0][3] =	 SG_ZERO;

    mat[1][0] = -sin_hdg * (SGfloat)sin_lat * (SGfloat)cos_lon - cos_hdg * (SGfloat)sin_lon;
    mat[1][1] = -sin_hdg * (SGfloat)sin_lat * (SGfloat)sin_lon + cos_hdg * (SGfloat)cos_lon;
    mat[1][2] =	 sin_hdg * (SGfloat)cos_lat;
    mat[1][3] =	 SG_ZERO;

    mat[2][0] = (SGfloat)cos_lat * (SGfloat)cos_lon;
    mat[2][1] = (SGfloat)cos_lat * (SGfloat)sin_lon;
    mat[2][2] =	(SGfloat)sin_lat;
    mat[2][3] =  SG_ZERO;

    mat[3][0] = offset.x();
    mat[3][1] = offset.y();
    mat[3][2] = offset.z();
    mat[3][3] = SG_ONE ;

    sgSetCoord( obj_pos, mat );
}
*/
