// FGAIPlane - abstract base class for an AI plane
//
// Written by David Luff, started 2002.
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

#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <simgear/math/point3d.hxx>
#include <simgear/debug/logstream.hxx>
#include <math.h>
#include <string>
SG_USING_STD(string);


#include "AIPlane.hxx"
#include "ATCdisplay.hxx"

FGAIPlane::FGAIPlane() {
	leg = LEG_UNKNOWN;
}

FGAIPlane::~FGAIPlane() {
}

void FGAIPlane::Update(double dt) {
}

void FGAIPlane::Bank(double angle) {
	// This *should* bank us smoothly to any angle
	if(fabs(roll - angle) > 0.6) {
		roll -= ((roll - angle)/fabs(roll - angle));  
	}
}

// Duplication of Bank(0.0) really - should I cut this?
void FGAIPlane::LevelWings(void) {
	// bring the plane back to level smoothly (this should work to come out of either bank)
	if(fabs(roll) > 0.6) {
		roll -= (roll/fabs(roll));
	}
}

void FGAIPlane::Transmit(string msg) {
	SG_LOG(SG_ATC, SG_INFO, "Transmit called, msg = " << msg);
	double user_freq0 = fgGetDouble("/radios/comm[0]/frequencies/selected-mhz");
	//double user_freq0 = ("/radios/comm[0]/frequencies/selected-mhz");
	//comm1 is not used yet.
	
	if(freq == user_freq0) {
		//cout << "Transmitting..." << endl;
		// we are on the same frequency, so check distance to the user plane
		if(1) {
			// For now (testing) assume in range !!!
			// TODO - implement range checking
			globals->get_ATC_display()->RegisterSingleMessage(msg, 0);
		}
	}
}

void FGAIPlane::RegisterTransmission(int code) {
}


// Return what type of landing we're doing on this circuit
LandingType FGAIPlane::GetLandingOption() {
	return(FULL_STOP);
}


ostream& operator << (ostream& os, PatternLeg pl) {
	switch(pl) {
	case(TAKEOFF_ROLL):   return(os << "TAKEOFF ROLL");
	case(CLIMBOUT):       return(os << "CLIMBOUT");
	case(TURN1):          return(os << "TURN1");
	case(CROSSWIND):      return(os << "CROSSWIND");
	case(TURN2):          return(os << "TURN2");
	case(DOWNWIND):       return(os << "DOWNWIND");
	case(TURN3):          return(os << "TURN3");
	case(BASE):           return(os << "BASE");
	case(TURN4):          return(os << "TURN4");
	case(FINAL):          return(os << "FINAL");
	case(LANDING_ROLL):   return(os << "LANDING ROLL");
	case(LEG_UNKNOWN):    return(os << "UNKNOWN");
	}
	return(os << "ERROR - Unknown switch in PatternLeg operator << ");
}


ostream& operator << (ostream& os, LandingType lt) {
	switch(lt) {
	case(FULL_STOP):      return(os << "FULL STOP");
	case(STOP_AND_GO):    return(os << "STOP AND GO");
	case(TOUCH_AND_GO):   return(os << "TOUCH AND GO");
	case(AIP_LT_UNKNOWN): return(os << "UNKNOWN");
	}
	return(os << "ERROR - Unknown switch in LandingType operator << ");
}

