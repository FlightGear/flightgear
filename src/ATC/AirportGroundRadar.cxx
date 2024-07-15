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

#include <ATC/AirportGroundRadar.hxx>
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

bool AirportGroundRadar::add(SGSharedPtr<FGTrafficRecord> aiObject) {
	bool ret = index.add(aiObject);
    index.printPath(aiObject);
	SG_LOG(SG_ATC, SG_DEBUG, "Added Aircraft " << aiObject->getId());	
	return ret;
}

bool AirportGroundRadar::move(const SGRectd& newPos, SGSharedPtr<FGTrafficRecord> aiObject)
{
	// TODO check for actual move
	return index.move(newPos, aiObject);
}

bool AirportGroundRadar::remove(SGSharedPtr<FGTrafficRecord> aiObject)
{
	bool ret = index.remove(aiObject);
	if (!ret) {
		SG_LOG(SG_ATC, SG_ALERT, "Couldn't remove " << aiObject->getId());	
	}
	return ret;
}

size_t AirportGroundRadar::size(){return index.size();}

int AirportGroundRadar::getSize(SGSharedPtr<FGTrafficRecord> aiObject){
  	return 50;
}

bool AirportGroundRadar::isBlocked(SGSharedPtr<FGTrafficRecord> aiObject)
{
    auto values = std::vector<SGSharedPtr<FGTrafficRecord>>();
	const SGRectd queryBox(aiObject->getPos().getLatitudeDeg()-QUERY_BOX_SIZE,
	aiObject->getPos().getLongitudeDeg()-QUERY_BOX_SIZE,
	aiObject->getPos().getLatitudeDeg()+QUERY_BOX_SIZE,
	aiObject->getPos().getLongitudeDeg()+QUERY_BOX_SIZE);
	index.query(queryBox, values);
	for (SGSharedPtr<FGTrafficRecord> other: values) {
        if (other->getId()!=aiObject->getId()){
			double distM = SGGeodesy::distanceM(aiObject->getPos(), other->getPos());

			const double courseTowardOther = SGGeodesy::courseDeg(aiObject->getPos(), other->getPos());
            // For right before left priority
            const double headingDiff = SGMiscd::normalizePeriodic(-180, 180, aiObject->getHeading() - courseTowardOther);
            const double otherHeadingDiff = SGMiscd::normalizePeriodic(-180, 180, other->getHeading() - courseTowardOther);
//            SG_LOG(SG_ATC, SG_DEBUG, "Found " << other->getId() << " Dist " << distM << " Headingdiff " << headingDiff << " Other heading diff " << otherHeadingDiff);
			const int threshold = getSize(aiObject) + getSize(other);
			if ( distM < threshold && 
			    ((headingDiff < 0 && abs(otherHeadingDiff) < 90) || (other->getSpeed() == 0 && abs(headingDiff) < 5)) ){
				// from the right and in front or other is stopped
                SG_LOG(SG_ATC, SG_DEBUG, aiObject->getId() << " blocked by " << other->getId());
				return true;
			}
		}
	}
    return false;
}

bool AirportGroundRadar::isBlockedForPushback(SGSharedPtr<FGTrafficRecord> aiObject)
{
    auto values = std::vector<SGSharedPtr<FGTrafficRecord>>();
	const SGRectd queryBox(aiObject->getPos().getLatitudeDeg()-QUERY_BOX_SIZE,
	aiObject->getPos().getLongitudeDeg()-QUERY_BOX_SIZE,
	aiObject->getPos().getLatitudeDeg()+QUERY_BOX_SIZE,
	aiObject->getPos().getLongitudeDeg()+QUERY_BOX_SIZE);
	index.query(queryBox, values);
	for (SGSharedPtr<FGTrafficRecord> other: values) {
        if (other->getId()!=aiObject->getId()){
			double distM = SGGeodesy::distanceM(aiObject->getPos(), other->getPos());

			const double courseTowardOther = SGGeodesy::courseDeg(aiObject->getPos(), other->getPos());
            // For right before left priority
            const double headingDiff = SGMiscd::normalizePeriodic(-180, 180, aiObject->getHeading() - courseTowardOther);
            const double otherHeadingDiff = SGMiscd::normalizePeriodic(-180, 180, other->getHeading() - courseTowardOther);
//            SG_LOG(SG_ATC, SG_DEBUG, "Found " << other->getId() << " Dist " << distM << " Headingdiff " << headingDiff << " Other heading diff " << otherHeadingDiff);
			const int threshold = getSize(aiObject) + getSize(other);
			if ( distM < threshold && (abs(headingDiff) > 90) ){
				// from the right and in front or other is stopped
                SG_LOG(SG_ATC, SG_DEBUG, aiObject->getId() << " blocked for pushback by " << other->getId());
				return true;
			}
		}
	}
    return false;
}

const SGSharedPtr<FGTrafficRecord> AirportGroundRadar::getBlockedBy(SGSharedPtr<FGTrafficRecord> aiObject)
{
    auto values = std::vector<SGSharedPtr<FGTrafficRecord>>();
	const SGRectd queryBox(aiObject->getPos().getLatitudeDeg()-QUERY_BOX_SIZE,
	aiObject->getPos().getLongitudeDeg()-QUERY_BOX_SIZE,
	aiObject->getPos().getLatitudeDeg()+QUERY_BOX_SIZE,
	aiObject->getPos().getLongitudeDeg()+QUERY_BOX_SIZE);
    SG_LOG(SG_ATC, SG_DEBUG, "Blocking Id : " << aiObject->getId());
	index.query(queryBox, values);
	for (SGSharedPtr<FGTrafficRecord> other: values) {
        if (other->getId()!=aiObject->getId()){
			double distM = SGGeodesy::distanceM(aiObject->getPos(), other->getPos());

			const double courseTowardOther = SGGeodesy::courseDeg(aiObject->getPos(), other->getPos());
            // For right before left priority
            const double headingDiff = SGMiscd::normalizePeriodic(-180, 180, aiObject->getHeading() - courseTowardOther);
            const double otherHeadingDiff = SGMiscd::normalizePeriodic(-180, 180, other->getHeading() - courseTowardOther);
			const int threshold = getSize(aiObject) + getSize(other);
            SG_LOG(SG_ATC, SG_DEBUG, "Found Id : " << other->getId() << " Dist \t" << distM << "m Threshold " << threshold << " Headingdiff " << headingDiff << " Other heading diff " << otherHeadingDiff);
			if ( distM < threshold && headingDiff < 0 && abs(otherHeadingDiff) < 90 ){
				// from the right and in front
				return other;
			}
		}
	}
    return nullptr;
}
