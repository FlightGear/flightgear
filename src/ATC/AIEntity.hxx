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

#ifndef _FG_AIEntity_HXX
#define _FG_AIEntity_HXX

#include <plib/sg.h>
#include <plib/ssg.h>
#include <simgear/math/point3d.hxx>

class FGAIEntity {

public:

    // We need some way for this class to display its radio transmissions if on the 
    // same frequency and in the vicinity of the user's aircraft
    // This may need to be done independently of ATC eg CTAF

    virtual ~FGAIEntity();

    // Run the internal calculations
    virtual void Update();

protected:

    double lat;		//WGS84
    double lon;		//WGS84
    double elev;	//Meters
    double hdg;		//True heading in degrees
    double roll;	//degrees
    double pitch;	//degrees

    char* model_path;	//Path to the 3D model

    ssgEntity* model;
    ssgTransform* position;

    void Transform();

    //void WorldCoordinate(sgCoord *obj_pos, Point3D center);

    void FastWorldCoordinate(sgCoord *obj_pos, Point3D center);

};

#endif  // _FG_AIEntity_HXX