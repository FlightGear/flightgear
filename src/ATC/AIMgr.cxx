// AIMgr.cxx - implementation of FGAIMgr 
// - a global management class for FlightGear generated AI traffic
//
// Written by David Luff, started March 2002.
//
// Copyright (C) 2002  David C Luff - david.luff@nottingham.ac.uk
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

#include <Main/fgfs.hxx>
#include <Main/fg_props.hxx>

#include <list>

#include "AIMgr.hxx"
#include "AILocalTraffic.hxx"

SG_USING_STD(list);


FGAIMgr::FGAIMgr() {
}

FGAIMgr::~FGAIMgr() {
}

void FGAIMgr::init() {
	// Hard wire some local traffic for now.
	// This is regardless of location and hence *very* ugly but it is a start.
	FGAILocalTraffic* local_traffic = new FGAILocalTraffic;
	//local_traffic->Init("KEMT", IN_PATTERN, TAKEOFF_ROLL);
	local_traffic->Init("KEMT");
	local_traffic->FlyCircuits(1, true);	// Fly 2 circuits with touch & go in between
	ai_list.push_back(local_traffic);
}

void FGAIMgr::bind() {
}

void FGAIMgr::unbind() {
}

void FGAIMgr::update(double dt) {
	// Don't update any planes for first 50 runs through - this avoids some possible initialisation anomalies
	static int i = 0;
	if(i < 50) {
		i++;
		return;
	}
	
	// Traverse the list of active planes and run all their update methods
	// TODO - spread the load - not all planes should need updating every frame.
	// Note that this will require dt to be calculated for each plane though
	// since they rely on it to calculate distance travelled.
	ai_list_itr = ai_list.begin();
	while(ai_list_itr != ai_list.end()) {
		(*ai_list_itr)->Update(dt);
		++ai_list_itr;
	}
}
