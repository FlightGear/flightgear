// AirportGroundRadar.cxx - Implimentation of the FlightGear Ground radar
//
// Written by Keith Paterson, started Feb 2023.
//
// Copyright (C) 2023 Keith Paterson.
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

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
		//SG_LOG(SG_ATC, SG_ALERT, "getBox " << (*aiObject).getId() );
		return SGRect<double>((*aiObject).getPos().getLatitudeDeg(),
		(*aiObject).getPos().getLongitudeDeg());
	};
	/**Function implementing equals for Quadtree*/
    static bool equal(SGSharedPtr<FGTrafficRecord> o, SGSharedPtr<FGTrafficRecord> o2) {
		return (*o).getId() == (*o2).getId();
	};

private:
    const double QUERY_BOX_SIZE = 0.1;
	QuadTree<FGTrafficRecord, decltype(&getBox), decltype(&equal)> index;
	SGGeod min;
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