// FGAILocalTraffic - AIEntity derived class with enough logic to
// fly and interact with the traffic pattern.
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
#include <Model/loader.hxx>
//#include <simgear/constants.h>
#include <simgear/math/point3d.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/misc/sg_path.hxx>
#include <string>

SG_USING_STD(string);

#include "ATCmgr.hxx"
#include "AILocalTraffic.hxx"

FGAILocalTraffic::FGAILocalTraffic() {
}

FGAILocalTraffic::~FGAILocalTraffic() {
}

void FGAILocalTraffic::Init() {
    // Hack alert - Hardwired path!!
    string planepath = "Aircraft/c172/Models/c172-dpm.ac";
    model = globals->get_model_loader()->load_model(planepath);
    if (model == 0) {
	model =
            globals->get_model_loader()
            ->load_model("Models/Geometry/glider.ac");
	if (model == 0)
	    cout << "Failed to load an aircraft model in AILocalTraffic\n";
    } else {
	cout << "AILocal Traffic Model loaded successfully\n";
    }
    position = new ssgTransform;
    position->addKid(model);

    // Hardwire to KEMT
    lat = 34.081358;
    lon = -118.037483;
    hdg = 0.0;
    elev = (287.0 + 0.5) * SG_FEET_TO_METER;  // Ground is 296 so this should be above it
    mag_hdg = -10.0;
    pitch = 0.0;
    roll = 0.0;
    mag_var = -14.0;

    // Activate the tower - this is dependent on the ATC system being initialised before the AI system
    AirportATC a;
    if(globals->get_ATC_mgr()->GetAirportATCDetails((string)"KEMT", &a)) {
	if(a.tower_freq) {	// Has a tower
	    tower = (FGTower*)globals->get_ATC_mgr()->GetATCPointer((string)"KEMT", TOWER);	// Maybe need some error checking here
	} else {
	    // Check CTAF, unicom etc
	}
    } else {
	cout << "Unable to find airport details in FGAILocalTraffic::Init()\n";
    }

    Transform();
}

// Run the internal calculations
void FGAILocalTraffic::Update() {
    hdg = mag_hdg + mag_var;
    
    // This should become if(the plane has moved) then Transform()
    static int i = 0;
    if(i == 60) {
	Transform();
	i = 0;
    }
    i++; 
}
