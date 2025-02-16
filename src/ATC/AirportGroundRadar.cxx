/*
 * SPDX-FileName: AirportGroundRadar.cxx
 * SPDX-FileComment: Implementation of the FlightGear Ground radar
 * SPDX-FileCopyrightText: Copyright (C) 2023  Keith Paterson
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <ATC/AirportGroundRadar.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/math/SGGeod.hxx>
#include <Airports/airport.hxx>
#include <Airports/pavement.hxx>
#include <AIModel/AIAircraft.hxx>
#include <AIModel/performancedata.hxx>
#include <AIModel/AIConstants.hxx>
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
	double INDEX_SIZE_DEG = 1;
    double minLat = airport->getLatitude() - INDEX_SIZE_DEG/2;
    double minLon = airport->getLongitude() - INDEX_SIZE_DEG/2;
	SG_LOG(SG_ATC, SG_DEBUG, "Creating AirportGroundRadar for " << airport->getId());
	AirportGroundRadar::airport = airport;
	index.resize(SGRect<double>(minLat, minLon, INDEX_SIZE_DEG, INDEX_SIZE_DEG));
}

AirportGroundRadar::~AirportGroundRadar() {
}

bool AirportGroundRadar::add(SGSharedPtr<FGTrafficRecord> aiObject) {
	bool ret = index.add(aiObject);
	if (ret) {
		SG_LOG(SG_ATC, SG_DEBUG, "Added Aircraft " << aiObject->getCallsign() << "(" << aiObject->getId() << ")" << " Leg : " << aiObject->getLeg() << " " << aiObject->getPos().getLatitudeDeg() << " " << aiObject->getPos().getLongitudeDeg() );	
		//index.printPath(aiObject);
	} else {
		double distM = SGGeodesy::distanceM(aiObject->getPos(), airport->geod());
		SG_LOG(SG_ATC, SG_ALERT, "Couldn't add Aircraft " << aiObject->getCallsign() << "(" << aiObject->getId() << ") to " << airport->getId() << " Dist " << distM << "m Leg " << aiObject->getLeg() );	
	}
	return ret;
}

bool AirportGroundRadar::move(const SGRectd& newPos, SGSharedPtr<FGTrafficRecord> aiObject)
{
	bool ret = index.move(newPos, aiObject);
	if (!ret) {
		SG_LOG(SG_ATC, SG_DEBUG, "Couldn't move Aircraft " << aiObject->getCallsign() << "(" << aiObject->getId() << ") to " << airport->getId() << " Leg " << aiObject->getLeg() );	
	}
	return ret;
}

bool AirportGroundRadar::remove(SGSharedPtr<FGTrafficRecord> aiObject)
{
	if (aiObject==nullptr) {
		SG_LOG(SG_ATC, SG_ALERT, "Couldn't remove aiObject null" );	
		return false;
	}

	bool ret = index.remove(aiObject);
	if (!ret) {
		SG_LOG(SG_ATC, SG_ALERT, "Couldn't remove " << aiObject->getCallsign() << "(" << aiObject->getId() << ")");	
	}
	SG_LOG(SG_ATC, SG_DEBUG, "Removed Aircraft " << aiObject->getCallsign() << "(" << aiObject->getId() << ")");	
	return ret;
}

size_t AirportGroundRadar::size(){return index.size();}

int AirportGroundRadar::getSize(SGSharedPtr<FGTrafficRecord> aiObject){
	int speedCorrection = (*aiObject).getSpeed()!=0?std::abs(5*(*aiObject).getSpeed()):20; 
  	return std::abs((*aiObject).getRadius() + speedCorrection);
}

bool AirportGroundRadar::isBlocked(SGSharedPtr<FGTrafficRecord> aiObject)
{
    auto values = std::vector<SGSharedPtr<FGTrafficRecord>>();
	const SGRectd queryBox(aiObject->getPos().getLatitudeDeg()-QUERY_BOX_SIZE/2,
	aiObject->getPos().getLongitudeDeg()-QUERY_BOX_SIZE/2,
	QUERY_BOX_SIZE,
	QUERY_BOX_SIZE);
    SG_LOG(SG_ATC, SG_BULK, "Rect : ( " << queryBox.x() << "," << queryBox.y() << "x" << queryBox.width() << "," << queryBox.height() << ")");
	index.query(queryBox, values);
    SG_LOG(SG_ATC, SG_BULK, "Search Id : " << aiObject->getCallsign() << "(" << aiObject->getId() <<  ") Index Size : " << index.size() << " Result Size : " << values.size() );
	for (SGSharedPtr<FGTrafficRecord> other: values) {
		double distM = SGGeodesy::distanceM(aiObject->getPos(), other->getPos());
        if (other->getId()!=aiObject->getId()){

			const double courseTowardOther = SGGeodesy::courseDeg(aiObject->getPos(), other->getPos());
            // For right before left priority
            const double headingDiff = SGMiscd::normalizePeriodic(-180, 180, aiObject->getHeading() - courseTowardOther);
            const double otherHeadingDiff = SGMiscd::normalizePeriodic(-180, 180, other->getHeading() - courseTowardOther);
            SG_LOG(SG_ATC, SG_BULK, "Found " << other->getCallsign() << "(" << other->getId() << ") " << other->getPos().getLatitudeDeg() << other->getPos().getLongitudeDeg() << "Dist " << distM << " Headingdiff " << headingDiff << " Other heading diff " << otherHeadingDiff << " courseTowardOther " << courseTowardOther);
			const int threshold = getSize(aiObject) + getSize(other) + SEPARATION;
			if ( distM < threshold ) {				
			    if (headingDiff < 0 && aiObject->getSpeed() > 0 && abs(headingDiff) < 90){
					// from the right and in front or other is stopped
                	SG_LOG(SG_ATC, SG_BULK, aiObject->getId() << " blocked by " << other->getId() << " Dist " << distM << " Headingdiff " << headingDiff << " Other heading diff " << otherHeadingDiff << " Heading " << aiObject->getHeading() << " Other Heading " << other->getHeading());
					return true;
				}
			    if (headingDiff < 0 && aiObject->getSpeed() < 0 && abs(headingDiff) > 90){
					// from the right and in front or other is stopped
                	SG_LOG(SG_ATC, SG_BULK, aiObject->getId() << " blocked by " << other->getId() << " while reversing Dist " << distM << " Headingdiff " << headingDiff << " Other heading diff " << otherHeadingDiff);
					return true;
				}
			    if (other->getSpeed() == 0 && abs(headingDiff) < 5) {
					// from the right and in front or other is stopped
                	SG_LOG(SG_ATC, SG_BULK, aiObject->getId() << " blocked by stopped " << other->getId() << " Dist " << distM << " Headingdiff " << headingDiff << " Other heading diff " << otherHeadingDiff);
					return true;
				}
			}
		} else {
			if (distM>10)
			{
                SG_LOG(SG_ATC, SG_ALERT, aiObject->getCallsign() << "(" << aiObject->getId() << ") is not near it's shadow in index " << other->getId() << " Dist " << distM );
			}
			
		}
	}
    return false;
}

/**
 * Check if the aircraft could push back
 */

bool AirportGroundRadar::isBlockedForPushback(SGSharedPtr<FGTrafficRecord> aiObject)
{
    auto values = std::vector<SGSharedPtr<FGTrafficRecord>>();
	const SGRectd queryBox(aiObject->getPos().getLatitudeDeg()-QUERY_BOX_SIZE/2,
	aiObject->getPos().getLongitudeDeg()-QUERY_BOX_SIZE/2,
	QUERY_BOX_SIZE,
	QUERY_BOX_SIZE);
    SG_LOG(SG_ATC, SG_BULK, "Rect : ( " << queryBox.x() << "," << queryBox.y() << "x" << queryBox.width() << "," << queryBox.height() << ")");
	index.query(queryBox, values);
    SG_LOG(SG_ATC, SG_BULK, "Search Id : " << aiObject->getId() <<  " Index Size : " << index.size() << " Result Size : " << values.size() );
	for (SGSharedPtr<FGTrafficRecord> other: values) {
        if (other->getId()!=aiObject->getId()){
			double distM = SGGeodesy::distanceM(aiObject->getPos(), other->getPos());

			const double courseTowardOther = SGGeodesy::courseDeg(aiObject->getPos(), other->getPos());
            // For right before left priority
            const double headingDiff = SGMiscd::normalizePeriodic(-180, 180, aiObject->getHeading() - courseTowardOther);
            const double otherHeadingDiff = SGMiscd::normalizePeriodic(-180, 180, other->getHeading() - courseTowardOther);

			// We want ample space 
			const int threshold = 2 * getSize(aiObject) + 2 * getSize(other) + SEPARATION;
            SG_LOG(SG_ATC, SG_BULK, "Search Id : " << aiObject->getId() <<  " Found Id : " << other->getId() << " Dist \t" << distM << "m Threshold " << threshold << " Headingdiff " << headingDiff << " Other heading diff " << otherHeadingDiff  << " courseTowardOther " << courseTowardOther << " Turning "  << aiObject->getHeadingDiff() << " Speeds : " << aiObject->getSpeed() << "/" << other->getSpeed() << " " << (other->getSpeed()==0?"Other Stopped":""));

			if ( distM < threshold && (abs(headingDiff) > 135) ){
				// from the right and in front or other is stopped
                SG_LOG(SG_ATC, SG_BULK, aiObject->getCallsign() << "(" << aiObject->getId() << ") blocked for pushback by " << other->getCallsign() << "(" << other->getId() << ")");
				return true;
			}
		}
	}
    return false;
}

const SGSharedPtr<FGTrafficRecord> AirportGroundRadar::getBlockedBy(SGSharedPtr<FGTrafficRecord> aiObject)
{
    auto values = std::vector<SGSharedPtr<FGTrafficRecord>>();
	const SGRectd queryBox(aiObject->getPos().getLatitudeDeg()-QUERY_BOX_SIZE/2,
	aiObject->getPos().getLongitudeDeg()-QUERY_BOX_SIZE/2,
	QUERY_BOX_SIZE,
	QUERY_BOX_SIZE);
    SG_LOG(SG_ATC, SG_BULK, "Rect : ( " << queryBox.x() << "," << queryBox.y() << "x" << queryBox.width() << "," << queryBox.height() << ")");
	index.query(queryBox, values);
    SG_LOG(SG_ATC, SG_BULK, "Search Id : " << aiObject->getId() <<  " Index Size : " << index.size() << " Result Size : " << values.size() );
	SGSharedPtr<FGTrafficRecord> nearestTrafficRecord = nullptr;
	double nearestDist = HUGE_VAL;
	for (SGSharedPtr<FGTrafficRecord> other: values) {
		double distM = SGGeodesy::distanceM(aiObject->getPos(), other->getPos());
        if (other->getId()!=aiObject->getId()){

			const double courseTowardOther = SGGeodesy::courseDeg(aiObject->getPos(), other->getPos());
			const double turningRate = aiObject->getHeadingDiff();
            // For right before left priority
            const double headingDiff = SGMiscd::normalizePeriodic(-180, 180, aiObject->getHeading() - courseTowardOther - turningRate);
            const double otherHeadingDiff = SGMiscd::normalizePeriodic(-180, 180, other->getHeading() - courseTowardOther);
			const double threshold = getSize(aiObject) + getSize(other);
            SG_LOG(SG_ATC, SG_BULK, "Search Id : " << aiObject->getId() <<  " Found Id : " << other->getId() << " NearestDist " << nearestDist << " Dist \t" << distM << "m Threshold " << threshold << " Headingdiff " << headingDiff << " Other heading diff " << otherHeadingDiff  << " courseTowardOther " << courseTowardOther << " Turning "  << aiObject->getHeadingDiff() << " Speeds : " << aiObject->getSpeed() << "/" << other->getSpeed() << " " << (other->getSpeed()==0?"Other Stopped":""));
			if ( distM < 20 && aiObject->getSpeed() != 0) {
					// We can't have aircraft < 10m of each other 
                SG_LOG(SG_ATC, SG_ALERT, aiObject->getCallsign() << "(" << aiObject->getId() << ") running into " << other->getCallsign() << "(" << other->getId() << ") Dist " << distM << " Heading " << aiObject->getHeading() << " Other Heading " << other->getHeading() << " Headingdiff " << headingDiff << " Other heading diff " << otherHeadingDiff << " courseTowardOther " << courseTowardOther << " Speeds : " << aiObject->getSpeed() << "/" << other->getSpeed() << " Turning: " << aiObject->getHeadingDiff() << " Legs: " << aiObject->getLeg() << "/" << other->getLeg());
				if (aiObject->getAircraft()!=nullptr && other->getAircraft()!=nullptr) {
                	SG_LOG(SG_ATC, SG_ALERT, "Offending type " << aiObject->getAircraft()->getAcType() << " " << aiObject->getAircraft()->getCompany() << " " << aiObject->getAircraft()->getPerformance()->decelerationOnGround() );
                	SG_LOG(SG_ATC, SG_ALERT, "Speeds " << aiObject->getSpeed() << " " << other->getAircraft()->getSpeed() );
				}
			}
			if ( distM < threshold && distM < nearestDist ) {				
			    if (headingDiff < 0 // from the left
				    && aiObject->getSpeed() >= 0 
					&& abs(headingDiff) < 90 // in front
					&& abs(otherHeadingDiff) > 90 // towards us
					&& other->getLeg() != AILeg::STARTUP_PUSHBACK){
					// from the right and in front or other is stopped
                	SG_LOG(SG_ATC, SG_BULK, aiObject->getId() << " blocked by " << other->getId() << " Dist " << distM << " Headingdiff " << headingDiff << " Other heading diff " << otherHeadingDiff);
					nearestDist = distM;
					nearestTrafficRecord = other;
				}
			    if (aiObject->getSpeed() < 0 
				    && abs(headingDiff) > 90){
					// moving backwards 
                	SG_LOG(SG_ATC, SG_BULK, aiObject->getId() << " blocked reversing by " << other->getId() << " Dist " << distM << " Headingdiff " << headingDiff << " Other heading diff " << otherHeadingDiff);
					nearestDist = distM;
					nearestTrafficRecord = other;
				}
			    if (other->getSpeed() == 0 
				&& abs(headingDiff) < 20
				&& abs(otherHeadingDiff) > 90) {
					// in front or other is stopped which means other heading is not relevant
                	SG_LOG(SG_ATC, SG_WARN, aiObject->getId() << " blocked by stopped opposing " << other->getId() << " Dist " << distM << " Headingdiff " << headingDiff << " Other heading diff " << otherHeadingDiff);
					nearestDist = distM;
					nearestTrafficRecord = other;
				}
			    if (other->getSpeed() >= 0 
				&& abs(headingDiff) < 20  
				&& abs(otherHeadingDiff) < 30) {
					// in front and pointing away
                	SG_LOG(SG_ATC, SG_BULK, aiObject->getId() << " blocked by stopped pointing away" << other->getId() << " Dist " << distM << " Headingdiff " << headingDiff << " Other heading diff " << otherHeadingDiff);
					nearestDist = distM;
					nearestTrafficRecord = other;
				}
			}
		}  else {
			if (distM>10)
			{
                SG_LOG(SG_ATC, SG_ALERT, aiObject->getCallsign() << "(" << aiObject->getId() << ") is not near it's shadow in index Leg : " << aiObject->getLeg() << "/" << other->getLeg() << " Dist " << distM );
			}
			
		}
	}
    return nearestTrafficRecord;
}
