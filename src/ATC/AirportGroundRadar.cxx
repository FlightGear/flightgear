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

#include "AirportGroundRadar.hxx"
#include <simgear/debug/logstream.hxx>
#include <simgear/math/SGGeod.hxx>
#include <Airports/airport.hxx>
#include <Airports/pavement.hxx>
#include <AIModel/VectorMath.hxx>


AirportGroundRadar::AirportGroundRadar(SGGeod min, SGGeod max): index(getBox, equal), min(min) {
	double w = max.getLatitudeDeg() - min.getLatitudeDeg();
	double h = max.getLongitudeDeg() - min.getLongitudeDeg();
	if ( w < 1 || h < 1 )
	{
        SG_LOG(SG_ATC, SG_ALERT, "Size of AirportGroundRadar invalid");
	}
	index.resize(SGRect<double>(min.getLatitudeDeg(), min.getLongitudeDeg(), w, h));
}

AirportGroundRadar::AirportGroundRadar(FGAirportRef airport): index(getBox, equal) {
    double minLat = airport->getLatitude() - 0.25;
    double minLon = airport->getLongitude() - 0.25;
	SG_LOG(SG_ATC, SG_DEBUG, "Creating AirportGroundRadar for " << airport->getId());
	index.resize(SGRect<double>(minLat, minLon, 0.5, 0.5));
}

AirportGroundRadar::~AirportGroundRadar() {
}

void AirportGroundRadar::add(FGAIBase* aiObject)
{
	index.add(aiObject);
}

void AirportGroundRadar::remove(FGAIBase* aiObject)
{
	index.remove(aiObject);
}

size_t AirportGroundRadar::size(){return index.size();}

int AirportGroundRadar::getSize(FGAIBase* aiObject){
  	return 50;
}

bool AirportGroundRadar::isBlocked(FGAIBase* aiObject)
{
    auto values = std::vector<FGAIBase*>();
	const SGRectd queryBox(aiObject->getGeodPos().getLatitudeDeg()-QUERY_BOX_SIZE,
	aiObject->getGeodPos().getLongitudeDeg()-QUERY_BOX_SIZE,
	aiObject->getGeodPos().getLatitudeDeg()+QUERY_BOX_SIZE,
	aiObject->getGeodPos().getLongitudeDeg()+QUERY_BOX_SIZE);
	index.query(queryBox, values);
	for (FGAIBase* other: values) {
        if (other->getID()!=aiObject->getID()){
			double distM = SGGeodesy::distanceM(aiObject->getGeodPos(), other->getGeodPos());

			const double courseTowardOther = SGGeodesy::courseDeg(aiObject->getGeodPos(), other->getGeodPos());
            // For right before left priority
            const double headingDiff = SGMiscd::normalizePeriodic(-180, 180, aiObject->getTrueHeadingDeg() - courseTowardOther);
            const double otherHeadingDiff = SGMiscd::normalizePeriodic(-180, 180, other->getTrueHeadingDeg() - courseTowardOther);
            SG_LOG(SG_AI, SG_DEBUG, "Found " << other->getID() << " Dist " << distM << " Headingdiff " << headingDiff << " Other heading diff " << otherHeadingDiff);
			const int threshold = getSize(aiObject) + getSize(other);
			if ( distM < threshold && headingDiff < 0 && abs(otherHeadingDiff) < 90 ){
				// from the right and in front
                SG_LOG(SG_AI, SG_ALERT, aiObject->getID() << " blocked by " << other->getID());
				return true;
			}
		}
	}
    return false;
}

const FGAIBase* AirportGroundRadar::isBlockedBy(FGAIBase* aiObject)
{
    auto values = std::vector<FGAIBase*>();
	const SGRectd queryBox(aiObject->getGeodPos().getLatitudeDeg()-QUERY_BOX_SIZE,
	aiObject->getGeodPos().getLongitudeDeg()-QUERY_BOX_SIZE,
	aiObject->getGeodPos().getLatitudeDeg()+QUERY_BOX_SIZE,
	aiObject->getGeodPos().getLongitudeDeg()+QUERY_BOX_SIZE);
	index.query(queryBox, values);
	for (FGAIBase* other: values) {
        if (other->getID()!=aiObject->getID()){
			double distM = SGGeodesy::distanceM(aiObject->getGeodPos(), other->getGeodPos());

			const double courseTowardOther = SGGeodesy::courseDeg(aiObject->getGeodPos(), other->getGeodPos());
            // For right before left priority
            const double headingDiff = SGMiscd::normalizePeriodic(-180, 180, aiObject->getTrueHeadingDeg() - courseTowardOther);
            const double otherHeadingDiff = SGMiscd::normalizePeriodic(-180, 180, other->getTrueHeadingDeg() - courseTowardOther);
            SG_LOG(SG_AI, SG_DEBUG, "Found " << other->getID() << " Dist " << distM << " Headingdiff " << headingDiff << " Other heading diff " << otherHeadingDiff);
			const int threshold = getSize(aiObject) + getSize(other);
			if ( distM < threshold && headingDiff < 0 && abs(otherHeadingDiff) < 90 ){
				// from the right and in front
				return other;
			}
		}
	}
    return nullptr;
}
