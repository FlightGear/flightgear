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

#ifndef _FG_AIEntity_HXX
#define _FG_AIEntity_HXX

#include <plib/sg.h>
#include <plib/ssg.h>

#include <simgear/math/point3d.hxx>
#include <simgear/scene/model/model.hxx>
#include <simgear/scene/model/placement.hxx>


/*****************************************************************
*
*  FGAIEntity - this class implements the minimum requirement
*  for any AI entity - a position, an orientation, an associated
*  3D model, and the ability to be moved.  It does nothing useful
*  and all AI entities are expected to be derived from it.
*
******************************************************************/
class FGAIEntity {

public:

    virtual ~FGAIEntity();

    // Run the internal calculations
    virtual void Update(double dt);
	
    // Send a transmission *TO* the AIEntity.
    // FIXME int code is a hack - eventually this will receive Alexander's coded messages.
    virtual void RegisterTransmission(int code);
	
	inline Point3D GetPos() { return(pos); }

protected:

    Point3D pos;	// WGS84 lat & lon in degrees, elev above sea-level in meters
    double hdg;		//True heading in degrees
    double roll;	//degrees
    double pitch;	//degrees

    char* model_path;	//Path to the 3D model
    SGModelPlacement aip;

    void Transform();
};

#endif  // _FG_AIEntity_HXX

