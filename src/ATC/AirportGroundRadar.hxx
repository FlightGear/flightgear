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
#include "QuadTree.hxx"
#include "AIModel/AIBase.hxx"
#include "Airports/airports_fwd.hxx"

using quadtree::QuadTree;

/**
 * Class representing a kind of ground radar. It is used to control traffic by FGGroundController
 * and prevent collisions. It supporst all FGAIBase objects.
*/
class AirportGroundRadar {
public:
// for index
	/**Function implementing calculation of dimension for Quadtree*/
    static SGRect<double> getBox(FGAIBase* aiObject) {
		return SGRect<double>(aiObject->getGeodPos().getLatitudeDeg(),
		aiObject->getGeodPos().getLongitudeDeg());
	};
	/**Function implementing equals for Quadtree*/
    static bool equal(FGAIBase* o, FGAIBase* o2) {
		return o->getID() == o2->getID();
	};

private:
    const double QUERY_BOX_SIZE = 0.1;
	QuadTree<FGAIBase, decltype(&getBox), decltype(&equal)> index;
	SGGeod min;
	int getSize(FGAIBase* aiObject);
public:
	AirportGroundRadar(SGGeod min, SGGeod max);
	AirportGroundRadar(FGAirportRef airport);
	~AirportGroundRadar();
	void add(FGAIBase* aiObject);
	void remove(FGAIBase* aiObject);
	size_t size();
	/**Returns if this AI object is blocked by any other "known" aka visible to the Radar.*/
	bool isBlocked(FGAIBase* aiObject);
		/**Returns if this AI object is blocked by any other "known" aka visible to the Radar.*/
	const FGAIBase* isBlockedBy(FGAIBase* aiObject);
};