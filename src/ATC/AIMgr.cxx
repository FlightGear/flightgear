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

/*****************************************************************
*
* WARNING - Curt has some ideas about AI traffic so anything in here
* may get rewritten or scrapped.  Contact Curt curt@flightgear.org 
* before spending any time or effort on this code!!!
*
******************************************************************/



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
    local_traffic->Init();
    ai_list.push_back(local_traffic);
}

void FGAIMgr::bind() {
}

void FGAIMgr::unbind() {
}

void FGAIMgr::update(int dt) {
    // Traverse the list of active planes and run all their update methods
    ai_list_itr = ai_list.begin();
    while(ai_list_itr != ai_list.end()) {
	(*ai_list_itr)->Update();
	++ai_list_itr;
    }
}