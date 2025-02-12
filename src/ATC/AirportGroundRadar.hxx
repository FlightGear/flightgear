/*
 * SPDX-FileName: AirportGroundRadar.hxx
 * SPDX-FileComment: Implementation of the FlightGear Ground radar
 * SPDX-FileCopyrightText: Copyright (C) 2023  Keith Paterson
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once
#include <simgear/math/SGGeod.hxx>
#include <simgear/structure/SGSharedPtr.hxx>

#include "AIModel/AIBase.hxx"
#include "Airports/airports_fwd.hxx"
#include "ATC/trafficcontrol.hxx"
#include "ATC/QuadTree.hxx"

using quadtree::QuadTree;

/**
 * Class representing a kind of ground radar. It is used to control traffic by FGGroundController
 * and prevent collisions. It supporst all FGAIBase objects.
*/
class AirportGroundRadar: public SGReferenced {
public:
// for index
	/**Function implementing calculation of dimension for Quadtree*/
    static SGRect<double> getBox(SGSharedPtr<FGTrafficRecord> aiObject) {
		if ((*aiObject).getCallsign() == "BA780") {
		  SG_LOG(SG_ATC, SG_ALERT, "getBox " << (*aiObject).getCallsign() << "(" << (*aiObject).getId() << ") " << aiObject << " " << (*aiObject).getPos().getLatitudeDeg() << " " << (*aiObject).getPos().getLongitudeDeg());
		}
		return SGRect<double>((*aiObject).getPos().getLatitudeDeg(),
		(*aiObject).getPos().getLongitudeDeg());
	};
	/**Function implementing equals for Quadtree*/
    static bool equal(SGSharedPtr<FGTrafficRecord> o, SGSharedPtr<FGTrafficRecord> o2) {
//		if ((*o).getCallsign() == "BA780") {
		  SG_LOG(SG_ATC, SG_DEBUG, (*o).getCallsign() << "(" << (*o).getId() << ")" << o << "/" << (*o2).getCallsign() << "(" << (*o2).getId() << ")" << o2);
//		}
		return (*o).getId() == (*o2).getId();
	};

private:
    const double QUERY_BOX_SIZE = 0.1;
	const int SEPARATION = 4;
	QuadTree<FGTrafficRecord, decltype(&getBox), decltype(&equal)> index;
	SGGeod min;
	FGAirportRef airport;
	int getSize(SGSharedPtr<FGTrafficRecord> aiObject);
public:
	AirportGroundRadar(SGGeod min, SGGeod max);
	AirportGroundRadar(FGAirportRef airport);
	~AirportGroundRadar();
	bool add(SGSharedPtr<FGTrafficRecord> aiObject);
	bool move(const SGRectd& newPos, SGSharedPtr<FGTrafficRecord> aiObject);
	bool remove(SGSharedPtr<FGTrafficRecord> aiObject);
	size_t size();
	/**Returns if this AI object is blocked by any other "known" aka visible to the Radar.*/
	bool isBlocked(SGSharedPtr<FGTrafficRecord> aiObject);
	bool isBlockedForPushback(SGSharedPtr<FGTrafficRecord> aiObject);
		/**Returns which AI object is blocking this traffic.*/
	const SGSharedPtr<FGTrafficRecord> getBlockedBy(SGSharedPtr<FGTrafficRecord> aiObject);
};