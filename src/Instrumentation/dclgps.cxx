// dclgps.cxx - a class to extend the operation of FG's current GPS
// code, and provide support for a KLN89-specific instrument.  It
// is envisioned that eventually this file and class will be split
// up between current FG code and new KLN89-specific code and removed.
//
// Written by David Luff, started 2005.
//
// Copyright (C) 2005 - David C Luff:  daveluff --AT-- ntlworld --D0T-- com
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
//
// $Id$

#include "dclgps.hxx"

#include <simgear/sg_inlines.h>
#include <simgear/timing/sg_time.hxx>
#include <simgear/magvar/magvar.hxx>

#include <Main/fg_props.hxx>
#include <Navaids/fix.hxx>
#include <Navaids/navrecord.hxx>
#include <Airports/simple.hxx>
#include <Airports/runways.hxx>

#include <iostream>

using namespace std;

GPSWaypoint::GPSWaypoint() {
    appType = GPS_APP_NONE;
}

GPSWaypoint::GPSWaypoint(const std::string& aIdent, float aLat, float aLon, GPSWpType aType) :
  id(aIdent),
  lat(aLat),
  lon(aLon),
  type(aType),
  appType(GPS_APP_NONE)
{
}

GPSWaypoint::~GPSWaypoint() {}

string GPSWaypoint::GetAprId() {
	if(appType == GPS_IAF) return(id + 'i');
	else if(appType == GPS_FAF) return(id + 'f');
	else if(appType == GPS_MAP) return(id + 'm');
	else if(appType == GPS_MAHP) return(id + 'h');
	else return(id);
}

static GPSWpType
GPSWpTypeFromFGPosType(FGPositioned::Type aType)
{
  switch (aType) {
  case FGPositioned::AIRPORT:
  case FGPositioned::SEAPORT:
  case FGPositioned::HELIPORT:
    return GPS_WP_APT;
  
  case FGPositioned::VOR:
    return GPS_WP_VOR;
  
  case FGPositioned::NDB:
    return GPS_WP_NDB;
  
  case FGPositioned::WAYPOINT:
    return GPS_WP_USR;
  
  case FGPositioned::FIX:
    return GPS_WP_INT;
  
  default:
    return GPS_WP_USR;
  }
}

GPSWaypoint* GPSWaypoint::createFromPositioned(const FGPositioned* aPos)
{
  if (!aPos) {
    return NULL; // happens if find returns no match
  }
  
  return new GPSWaypoint(aPos->ident(), 
    aPos->latitude() * SG_DEGREES_TO_RADIANS,
    aPos->longitude() * SG_DEGREES_TO_RADIANS,
    GPSWpTypeFromFGPosType(aPos->type())
  );
}

ostream& operator << (ostream& os, GPSAppWpType type) {
	switch(type) {
		case(GPS_IAF):       return(os << "IAF");
		case(GPS_IAP):       return(os << "IAP");
		case(GPS_FAF):       return(os << "FAF");
		case(GPS_MAP):       return(os << "MAP");
		case(GPS_MAHP):      return(os << "MAHP");
		case(GPS_HDR):       return(os << "HEADER");
		case(GPS_FENCE):     return(os << "FENCE");
		case(GPS_APP_NONE):  return(os << "NONE");
	}
	return(os << "ERROR - Unknown switch in GPSAppWpType operator << ");
}

FGIAP::FGIAP() {
}

FGIAP::~FGIAP() {
}

FGNPIAP::FGNPIAP() {
}

FGNPIAP::~FGNPIAP() {
}

ClockTime::ClockTime() {
    _hr = 0;
    _min = 0;
}

ClockTime::ClockTime(int hr, int min) {
    while(hr < 0) { hr += 24; }
    _hr = hr % 24;
    while(min < 0) { min += 60; }
    while(min > 60) { min -= 60; }
	_min = min;
}

ClockTime::~ClockTime() {
}

// ------------------------------------------------------------------------------------- //

DCLGPS::DCLGPS(RenderArea2D* instrument) {
	_instrument = instrument;
	_nFields = 1;
	_maxFields = 2;
	
	// Units - lets default to US units - FG can set them to other units from config during startup if desired.
	_altUnits = GPS_ALT_UNITS_FT;
	_baroUnits = GPS_PRES_UNITS_IN;
	_velUnits = GPS_VEL_UNITS_KT;
	_distUnits = GPS_DIST_UNITS_NM;

	_lon_node = fgGetNode("/instrumentation/gps/indicated-longitude-deg", true);
	_lat_node = fgGetNode("/instrumentation/gps/indicated-latitude-deg", true);
	_alt_node = fgGetNode("/instrumentation/gps/indicated-altitude-ft", true);
	_grnd_speed_node = fgGetNode("/instrumentation/gps/indicated-ground-speed-kt", true);
	_true_track_node = fgGetNode("/instrumentation/gps/indicated-track-true-deg", true);
	_mag_track_node = fgGetNode("/instrumentation/gps/indicated-track-magnetic-deg", true);
	
	// Use FG's position values at construction in case FG's gps has not run first update yet.
	_lon = fgGetDouble("/position/longitude-deg") * SG_DEGREES_TO_RADIANS;
	_lat = fgGetDouble("/position/latitude-deg") * SG_DEGREES_TO_RADIANS;
	_alt = fgGetDouble("/position/altitude-ft");
	// Note - we can depriciate _gpsLat and _gpsLon if we implement error handling in FG
	// gps code and not our own.
	_gpsLon = _lon;
	_gpsLat = _lat;
	_checkLon = _gpsLon;
	_checkLat = _gpsLat;
	_groundSpeed_ms = 0.0;
	_groundSpeed_kts = 0.0;
	_track = 0.0;
	_magTrackDeg = 0.0;
	
	// Sensible defaults.  These can be overriden by derived classes if desired.
	_cdiScales.clear();
	_cdiScales.push_back(5.0);
	_cdiScales.push_back(1.0);
	_cdiScales.push_back(0.3);
	_currentCdiScaleIndex = 0;
	_targetCdiScaleIndex = 0;
	_sourceCdiScaleIndex = 0;
	_cdiScaleTransition = false;
	_currentCdiScale = 5.0;
	
	_cleanUpPage = -1;
	
	_activeWaypoint.id.clear();
	_dist2Act = 0.0;
	_crosstrackDist = 0.0;
	_headingBugTo = true;
	_navFlagged = true;
	_waypointAlert = false;
	_departed = false;
	_departureTimeString = "----";
	_elapsedTime = 0.0;
	_powerOnTime.set_hr(0);
	_powerOnTime.set_min(0);
	_powerOnTimerSet = false;
	_alarmSet = false;
	
	// Configuration Initialisation
	// Should this be in kln89.cxx ?
	_turnAnticipationEnabled = false;
	_suaAlertEnabled = false;
	_altAlertEnabled = false;
        
	_time = new SGTime;
	
	_messageStack.clear();
	
	_dto = false;
	
	_approachLoaded = false;
	_approachArm = false;
	_approachReallyArmed = false;
	_approachActive = false;
	_approachFP = new GPSFlightPlan;
}

DCLGPS::~DCLGPS() {
	delete _time;
  delete _approachFP;		// Don't need to delete the waypoints inside since they point to
							// the waypoints in the approach database.
	// TODO - may need to delete the approach database!!
}

void DCLGPS::draw(osg::State& state) {
	_instrument->draw(state);
}

void DCLGPS::init() {
		
	// Not sure if this should be here, but OK for now.
	CreateDefaultFlightPlans();

	// Hack - hardwire some instrument approaches for development.
	// These will shortly be replaced by a routine to read ARINC data from file instead.
	FGNPIAP* iap;
	GPSWaypoint* wp;
	GPSFlightPlan* fp;
	const GPSWaypoint* cwp;
	
	iap = new FGNPIAP;
	iap->_aptIdent = "KHAF";
	iap->_ident = "R12-Y";
	iap->_name = ExpandSIAPIdent(iap->_ident);
	iap->_rwyStr = "12";
	iap->_approachRoutes.clear();
	iap->_IAP.clear();
	// -------
	wp = new GPSWaypoint;
	wp->id = "GOBBS";
	// Nasty using the find any function here, but it saves converting data from FGFix etc. 
	cwp = FindFirstByExactId(wp->id);
	if(cwp) {
		*wp = *cwp;
		wp->appType = GPS_IAF;
		fp = new GPSFlightPlan;
		fp->waypoints.push_back(wp);
	} else {
		//cout << "Unable to find waypoint " << wp->id << '\n';
	}
	// -------
	wp = new GPSWaypoint;
	wp->id = "FUJCE";
	cwp = FindFirstByExactId(wp->id);
	if(cwp) {
		*wp = *cwp;
		wp->appType = GPS_IAP;
		fp->waypoints.push_back(wp);
		iap->_approachRoutes.push_back(fp);
		iap->_IAP.push_back(wp);
	} else {
		//cout << "Unable to find waypoint " << wp->id << '\n';
	}
	// -------
	wp = new GPSWaypoint;
	wp->id = "JEVXY";
	cwp = FindFirstByExactId(wp->id);
	if(cwp) {
		*wp = *cwp;
		wp->appType = GPS_FAF;
		iap->_IAP.push_back(wp);
	} else {
		//cout << "Unable to find waypoint " << wp->id << '\n';
	}
	// -------
	wp = new GPSWaypoint;
	wp->id = "RW12";
	wp->appType = GPS_MAP;
	if(wp->id.substr(0, 2) == "RW" && wp->appType == GPS_MAP) {
		// Assume that this is a missed-approach point based on the runway number, which appears to be standard for most approaches.
		const FGAirport* apt = fgFindAirportID(iap->_aptIdent);
		if(apt) {
			// TODO - sanity check the waypoint ID to ensure we have a double digit number
			FGRunway* rwy = apt->getRunwayByIdent(wp->id.substr(2, 2));
			if(rwy) {
				wp->lat = rwy->begin().getLatitudeRad();
				wp->lon = rwy->begin().getLongitudeRad();
			}
		}
	} else {
		cwp = FindFirstByExactId(wp->id);
		if(cwp) {
			*wp = *cwp;
			wp->appType = GPS_MAP;
		} else {
			//cout << "Unable to find waypoint " << wp->id << '\n';
		}
	}
	iap->_IAP.push_back(wp);
	// -------
	wp = new GPSWaypoint;
	wp->id = "SEEMS";
	cwp = FindFirstByExactId(wp->id);
	if(cwp) {
		*wp = *cwp;
		wp->appType = GPS_MAHP;
		iap->_IAP.push_back(wp);
	} else {
		//cout << "Unable to find waypoint " << wp->id << '\n';
	}
	// -------
	_np_iap[iap->_aptIdent].push_back(iap);
}

void DCLGPS::bind() {
	fgTie("/instrumentation/gps/waypoint-alert", this, &DCLGPS::GetWaypointAlert);
	fgTie("/instrumentation/gps/leg-mode", this, &DCLGPS::GetLegMode);
	fgTie("/instrumentation/gps/obs-mode", this, &DCLGPS::GetOBSMode);
	fgTie("/instrumentation/gps/approach-arm", this, &DCLGPS::GetApproachArm);
	fgTie("/instrumentation/gps/approach-active", this, &DCLGPS::GetApproachActive);
	fgTie("/instrumentation/gps/cdi-deflection", this, &DCLGPS::GetCDIDeflection);
	fgTie("/instrumentation/gps/to-flag", this, &DCLGPS::GetToFlag);
}

void DCLGPS::unbind() {
	fgUntie("/instrumentation/gps/waypoint-alert");
	fgUntie("/instrumentation/gps/leg-mode");
	fgUntie("/instrumentation/gps/obs-mode");
	fgUntie("/instrumentation/gps/approach-arm");
	fgUntie("/instrumentation/gps/approach-active");
	fgUntie("/instrumentation/gps/cdi-deflection");
}

void DCLGPS::update(double dt) {
	//cout << "update called!\n";
	
	_lon = _lon_node->getDoubleValue() * SG_DEGREES_TO_RADIANS;
	_lat = _lat_node->getDoubleValue() * SG_DEGREES_TO_RADIANS;
	_alt = _alt_node->getDoubleValue();
	_groundSpeed_kts = _grnd_speed_node->getDoubleValue();
	_groundSpeed_ms = _groundSpeed_kts * 0.5144444444;
	_track = _true_track_node->getDoubleValue();
	_magTrackDeg = _mag_track_node->getDoubleValue();
	// Note - we can depriciate _gpsLat and _gpsLon if we implement error handling in FG
	// gps code and not our own.
	_gpsLon = _lon;
	_gpsLat = _lat;
	// Check for abnormal position slew
	if(GetGreatCircleDistance(_gpsLat, _gpsLon, _checkLat, _checkLon) > 1.0) {
		OrientateToActiveFlightPlan();
	}
	_checkLon = _gpsLon;
	_checkLat = _gpsLat;
	
	// TODO - check for unit power before running this.
	if(!_powerOnTimerSet) {
		SetPowerOnTimer();
	} 
	
	// Check if an alarm timer has expired
	if(_alarmSet) {
		if(_alarmTime.hr() == atoi(fgGetString("/instrumentation/clock/indicated-hour"))
		&& _alarmTime.min() == atoi(fgGetString("/instrumentation/clock/indicated-min"))) {
			_messageStack.push_back("*Timer Expired");
			_alarmSet = false;
		}
	}
	
	if(!_departed) {
		if(_groundSpeed_kts > 30.0) {
			_departed = true;
			string th = fgGetString("/instrumentation/clock/indicated-hour");
			string tm = fgGetString("/instrumentation/clock/indicated-min");
			if(th.size() == 1) th = "0" + th;
			if(tm.size() == 1) tm = "0" + tm;
			_departureTimeString = th + tm;
		}
	} else {
		// TODO - check - is this prone to drift error over time?
		// Should we difference the departure and current times?
		// What about when the user resets the time of day from the menu?
		_elapsedTime += dt;
	}

	_time->update(_gpsLon * SG_DEGREES_TO_RADIANS, _gpsLat * SG_DEGREES_TO_RADIANS, 0, 0);
	// FIXME - currently all the below assumes leg mode and no DTO or OBS cancelled.
	if(_activeFP->IsEmpty()) {
		// Not sure if we need to reset these each update or only when fp altered
		_activeWaypoint.id.clear();
		_navFlagged = true;
	} else if(_activeFP->waypoints.size() == 1) {
		_activeWaypoint.id.clear();
	} else {
		_navFlagged = false;
		if(_activeWaypoint.id.empty() || _fromWaypoint.id.empty()) {
			//cout << "Error, in leg mode with flightplan of 2 or more waypoints, but either active or from wp is NULL!\n";
			OrientateToActiveFlightPlan();
		}
		
		// Approach stuff
		if(_approachLoaded) {
			if(!_approachReallyArmed && !_approachActive) {
				// arm if within 30nm of airport.
				// TODO - let user cancel approach arm using external GPS-APR switch
				bool multi;
				const FGAirport* ap = FindFirstAptById(_approachID, multi, true);
				if(ap != NULL) {
					double d = GetGreatCircleDistance(_gpsLat, _gpsLon, ap->getLatitude() * SG_DEGREES_TO_RADIANS, ap->getLongitude() * SG_DEGREES_TO_RADIANS);
					if(d <= 30) {
						_approachArm = true;
						_approachReallyArmed = true;
						_messageStack.push_back("*Press ALT To Set Baro");
						// Not sure what we do if the user has already set CDI to 0.3 nm?
						_targetCdiScaleIndex = 1;
						if(_currentCdiScaleIndex == 1) {
							// No problem!
						} else if(_currentCdiScaleIndex == 0) {
							_sourceCdiScaleIndex = 0;
							_cdiScaleTransition = true;
							_cdiTransitionTime = 30.0;
							_currentCdiScale = _cdiScales[_currentCdiScaleIndex];
						}
					}
				}
			} else {
				// Check for approach active - we can only activate approach if it is really armed.
				if(_activeWaypoint.appType == GPS_FAF) {
					//cout << "Active waypoint is FAF, id is " << _activeWaypoint.id << '\n';
					if(GetGreatCircleDistance(_gpsLat, _gpsLon, _activeWaypoint.lat, _activeWaypoint.lon) <= 2.0 && !_obsMode) {
						// Assume heading is OK for now
						_approachArm = false;	// TODO - check - maybe arm is left on when actv comes on?
						_approachReallyArmed = false;
						_approachActive = true;
						_targetCdiScaleIndex = 2;
						if(_currentCdiScaleIndex == 2) {
							// No problem!
						} else if(_currentCdiScaleIndex == 1) {
							_sourceCdiScaleIndex = 1;
							_cdiScaleTransition = true;
							_cdiTransitionTime = 30.0;	// TODO - compress it if time to FAF < 30sec
							_currentCdiScale = _cdiScales[_currentCdiScaleIndex];
						} else {
							// Abort going active?
							_approachActive = false;
						}
					}
				}
			}
		}
		
		// CDI scale transition stuff
		if(_cdiScaleTransition) {
			if(fabs(_currentCdiScale - _cdiScales[_targetCdiScaleIndex]) < 0.001) {
				_currentCdiScale = _cdiScales[_targetCdiScaleIndex];
				_currentCdiScaleIndex = _targetCdiScaleIndex;
				_cdiScaleTransition = false;
			} else {
				double scaleDiff = (_targetCdiScaleIndex > _sourceCdiScaleIndex 
				                    ? _cdiScales[_sourceCdiScaleIndex] - _cdiScales[_targetCdiScaleIndex]
									: _cdiScales[_targetCdiScaleIndex] - _cdiScales[_sourceCdiScaleIndex]);
				//cout << "ScaleDiff = " << scaleDiff << '\n';
				if(_targetCdiScaleIndex > _sourceCdiScaleIndex) {
					// Scaling down eg. 5nm -> 1nm
					_currentCdiScale -= (scaleDiff * dt / _cdiTransitionTime);
					if(_currentCdiScale < _cdiScales[_targetCdiScaleIndex]) {
						_currentCdiScale = _cdiScales[_targetCdiScaleIndex];
						_currentCdiScaleIndex = _targetCdiScaleIndex;
						_cdiScaleTransition = false;
					}
				} else {
					_currentCdiScale += (scaleDiff * dt / _cdiTransitionTime);
					if(_currentCdiScale > _cdiScales[_targetCdiScaleIndex]) {
						_currentCdiScale = _cdiScales[_targetCdiScaleIndex];
						_currentCdiScaleIndex = _targetCdiScaleIndex;
						_cdiScaleTransition = false;
					}
				}
				//cout << "_currentCdiScale = " << _currentCdiScale << '\n';
			}
		} else {
			_currentCdiScale = _cdiScales[_currentCdiScaleIndex];
		}
		
		
		// Urgh - I've been setting the heading bug based on DTK,
		// bug I think it should be based on heading re. active waypoint
		// based on what the sim does after the final waypoint is passed.
		// (DTK remains the same, but if track is held == DTK heading bug
		// reverses to from once wp is passed).
		/*
		if(_fromWaypoint != NULL) {
			// TODO - how do we handle the change of track with distance over long legs?
			_dtkTrue = GetGreatCircleCourse(_fromWaypoint->lat, _fromWaypoint->lon, _activeWaypoint->lat, _activeWaypoint->lon) * SG_RADIANS_TO_DEGREES;
			_dtkMag = GetMagHeadingFromTo(_fromWaypoint->lat, _fromWaypoint->lon, _activeWaypoint->lat, _activeWaypoint->lon);
			// Don't change the heading bug if speed is too low otherwise it flickers to/from at rest
			if(_groundSpeed_ms > 5) {
				//cout << "track = " << _track << ", dtk = " << _dtkTrue << '\n'; 
				double courseDev = _track - _dtkTrue;
				//cout << "courseDev = " << courseDev << ", normalized = ";
				SG_NORMALIZE_RANGE(courseDev, -180.0, 180.0);
				//cout << courseDev << '\n';
				_headingBugTo = (fabs(courseDev) > 90.0 ? false : true);
			}
		} else {
			_dtkTrue = 0.0;
			_dtkMag = 0.0;
			// TODO - in DTO operation the position of initiation of DTO defines the "from waypoint".
		}
		*/
		if(!_activeWaypoint.id.empty()) {
			double hdgTrue = GetGreatCircleCourse(_gpsLat, _gpsLon, _activeWaypoint.lat, _activeWaypoint.lon) * SG_RADIANS_TO_DEGREES;
			if(_groundSpeed_ms > 5) {
				//cout << "track = " << _track << ", hdgTrue = " << hdgTrue << '\n'; 
				double courseDev = _track - hdgTrue;
				//cout << "courseDev = " << courseDev << ", normalized = ";
				SG_NORMALIZE_RANGE(courseDev, -180.0, 180.0);
				//cout << courseDev << '\n';
				_headingBugTo = (fabs(courseDev) > 90.0 ? false : true);
			}
			if(!_fromWaypoint.id.empty()) {
				_dtkTrue = GetGreatCircleCourse(_fromWaypoint.lat, _fromWaypoint.lon, _activeWaypoint.lat, _activeWaypoint.lon) * SG_RADIANS_TO_DEGREES;
				_dtkMag = GetMagHeadingFromTo(_fromWaypoint.lat, _fromWaypoint.lon, _activeWaypoint.lat, _activeWaypoint.lon);
			} else {
				_dtkTrue = 0.0;
				_dtkMag = 0.0;
			}
		}
		
		_dist2Act = GetGreatCircleDistance(_gpsLat, _gpsLon, _activeWaypoint.lat, _activeWaypoint.lon) * SG_NM_TO_METER;
		if(_groundSpeed_ms > 10.0) {
			_eta = _dist2Act / _groundSpeed_ms;
			if(_eta <= 36) {	// TODO - this is slightly different if turn anticipation is enabled.
				if(_headingBugTo) {
					_waypointAlert = true;	// TODO - not if the from flag is set.
				}
			}
			if(_eta < 60) {
				// Check if we should sequence to next leg.
				// Perhaps this should be done on distance instead, but 60s time (about 1 - 2 nm) seems reasonable for now.
				//double reverseHeading = GetGreatCircleCourse(_activeWaypoint->lat, _activeWaypoint->lon, _fromWaypoint->lat, _fromWaypoint->lon);
				// Hack - let's cheat and do it on heading bug for now.  TODO - that stops us 'cutting the corner'
				// when we happen to approach the inside turn of a waypoint - we should probably sequence at the midpoint
				// of the heading difference between legs in this instance.
				int idx = GetActiveWaypointIndex();
				bool finalLeg = (idx == (int)(_activeFP->waypoints.size()) - 1 ? true : false);
				bool finalDto = (_dto && idx == -1);	// Dto operation to a waypoint not in the flightplan - we don't sequence in this instance
				if(!_headingBugTo) {
					if(finalLeg) {
						// Do nothing - not sure if Dto should switch off when arriving at the final waypoint of a flightplan
					} else if(finalDto) {
						// Do nothing
					} else if(_activeWaypoint.appType == GPS_MAP) {
						// Don't sequence beyond the missed approach point
						//cout << "ACTIVE WAYPOINT is MAP - not sequencing!!!!!\n";
					} else {
						//cout << "Sequencing...\n";
						_fromWaypoint = _activeWaypoint;
						_activeWaypoint = *_activeFP->waypoints[idx + 1];
						_dto = false;
						// TODO - course alteration message format is dependent on whether we are slaved HSI/CDI indicator or not.
						// For now assume we are not.
						string s;
						if(fgGetBool("/instrumentation/nav[0]/slaved-to-gps")) {
							// TODO - avoid the hardwiring on nav[0]
							s = "Adj Nav Crs to ";
						} else {
							string s = "GPS Course is ";
						}
						double d = GetMagHeadingFromTo(_fromWaypoint.lat, _fromWaypoint.lon, _activeWaypoint.lat, _activeWaypoint.lon);
						while(d < 0.0) d += 360.0;
						while(d >= 360.0) d -= 360.0;
						char buf[4];
						snprintf(buf, 4, "%03i", (int)(d + 0.5));
						s += buf;
						_messageStack.push_back(s);
					}
					_waypointAlert = false;
				}
			}
		} else {
			_eta = 0.0;
		}
		
		/*
		// First attempt at a sensible cross-track correction calculation
		// Uh? - I think this is implemented further down the file!
		if(_fromWaypoint != NULL) {
				
		} else {
			_crosstrackDist = 0.0;
		}
		*/
	}
}

/* 
	Expand a SIAP ident to the full procedure name (as shown on the approach chart).
	NOTE: Some of this is inferred from data, some is from documentation.
	
	Example expansions from ARINC 424-18 [and the airport they're taken from]:
	"R10LY" <--> "RNAV (GPS) Y RWY 10 L"	[KBOI]
	"R10-Z" <--> "RNAV (GPS) Z RWY 10"		[KHTO]
	"S25" 	<--> "VOR or GPS RWY 25"		[KHHR]
	"P20"	<--> "GPS RWY 20"				[KDAN]
	"NDB-B"	<--> "NDB or GPS-B"				[KDAW]
	"NDBC"	<--> "NDB or GPS-C"				[KEMT]
	"VDMA"	<--> "VOR/DME or GPS-A"			[KDAW]
	"VDM-A"	<--> "VOR/DME or GPS-A"			[KEAG]
	"VDMB"	<--> "VOR/DME or GPS-B"			[KDKX]
	"VORA"	<--> "VOR or GPS-A"				[KEMT]
	
	It seems that there are 2 basic types of expansions; those that include
	the runway and those that don't.  Of those that don't, it seems that 2
	different positions within the string to encode the identifying letter
	are used, i.e. with a dash and without.
*/
string DCLGPS::ExpandSIAPIdent(const string& ident) {
	string name;
	bool has_rwy = false;
	
	switch(ident[0]) {
	case 'N': name = "NDB or GPS"; has_rwy = false; break;
	case 'P': name = "GPS"; has_rwy = true; break;
	case 'R': name = "RNAV (GPS)"; has_rwy = true; break;
	case 'S': name = "VOR or GPS"; has_rwy = true; break;
	case 'V':
		if(ident[1] == 'D') name = "VOR/DME or GPS";
		else name = "VOR or GPS";
		has_rwy = false;
		break;
	default: // TODO output a log message
		break;
	}
	
	if(has_rwy) {
		// Add the identifying letter if present
		if(ident.size() == 5) {
			name += ' ';
			name += ident[4];
		}
		
		// Add the runway
		name += " RWY ";
		name += ident.substr(1, 2);
		
		// Add a left/right/centre indication if present.
		if(ident.size() > 3) {
			if((ident[3] != '-') && (ident[3] != ' ')) {	// Early versions of the spec allowed a blank instead of a dash so check for both
				name += ' ';
				name += ident[3];
			}
		}
	} else {
		// Add the identifying letter, which I *think* should always be present, but seems to be inconsistent as to whether a dash is used.
		if(ident.size() == 5) {
			name += '-';
			name += ident[4];
		} else if(ident.size() == 4) {
			name += '-';
			name += ident[3];
		} else {
			// No suffix letter
		}
	}
	
	return(name);
}

GPSWaypoint* DCLGPS::GetActiveWaypoint() { 
	return &_activeWaypoint; 
}
	
// Returns meters
float DCLGPS::GetDistToActiveWaypoint() { 
	return _dist2Act;
}

// I don't yet fully understand all the gotchas about where to source time from.
// This function sets the initial timer before the clock exports properties
// and the one below uses the clock to be consistent with the rest of the code.
// It might change soonish...
void DCLGPS::SetPowerOnTimer() {
	struct tm *t = globals->get_time_params()->getGmt();
	_powerOnTime.set_hr(t->tm_hour);
	_powerOnTime.set_min(t->tm_min);
	_powerOnTimerSet = true;
}

void DCLGPS::ResetPowerOnTimer() {
	_powerOnTime.set_hr(atoi(fgGetString("/instrumentation/clock/indicated-hour")));
	_powerOnTime.set_min(atoi(fgGetString("/instrumentation/clock/indicated-min")));
	_powerOnTimerSet = true;
}

double DCLGPS::GetCDIDeflection() const {
	double xtd = CalcCrossTrackDeviation();	//nm
	return((xtd / _currentCdiScale) * 5.0 * 2.5 * -1.0);
}

void DCLGPS::DtoInitiate(const string& s) {
	const GPSWaypoint* wp = FindFirstByExactId(s);
	if(wp) {
		// TODO - Currently we start DTO operation unconditionally, regardless of which mode we are in.
		// In fact, the following rules apply:
		// In LEG mode, start DTO as we currently do.
		// In OBS mode, set the active waypoint to the requested waypoint, and then:
		// If the KLN89 is not connected to an external HSI or CDI, set the OBS course to go direct to the waypoint.
		// If the KLN89 *is* connected to an external HSI or CDI, it cannot set the course itself, and will display
		// a scratchpad message with the course to set manually on the HSI/CDI.
		// In both OBS cases, leave _dto false, since we don't need the virtual waypoint created.
		_dto = true;
		_activeWaypoint = *wp;
		_fromWaypoint.lat = _gpsLat;
		_fromWaypoint.lon = _gpsLon;
		_fromWaypoint.type = GPS_WP_VIRT;
		_fromWaypoint.id = "DTOWP";
		delete wp;
	} else {
		_dto = false;
	}
}

void DCLGPS::DtoCancel() {
	if(_dto) {
		// i.e. don't bother reorientating if we're just cancelling a DTO button press
		// without having previously initiated DTO.
		OrientateToActiveFlightPlan();
	}
	_dto = false;
}

void DCLGPS::ToggleOBSMode() {
	_obsMode = !_obsMode;
	if(_obsMode) {
		if(!_activeWaypoint.id.empty()) {
			_obsHeading = static_cast<int>(_dtkMag);
		}
		// TODO - the _fromWaypoint location will change as the OBS heading changes.
		// Might need to store the OBS initiation position somewhere in case it is needed again.
		SetOBSFromWaypoint();
	}
}

// Set the _fromWaypoint position based on the active waypoint and OBS radial.
void DCLGPS::SetOBSFromWaypoint() {
	if(!_obsMode) return;
	if(_activeWaypoint.id.empty()) return;
	
	// TODO - base the 180 deg correction on the to/from flag.
	_fromWaypoint = GetPositionOnMagRadial(_activeWaypoint, 10, _obsHeading + 180.0);
	_fromWaypoint.id = "OBSWP";
}

void DCLGPS::CDIFSDIncrease() {
	if(_currentCdiScaleIndex == 0) {
		_currentCdiScaleIndex = _cdiScales.size() - 1;
	} else {
		_currentCdiScaleIndex--;
	}
}

void DCLGPS::CDIFSDDecrease() {
	_currentCdiScaleIndex++;
	if(_currentCdiScaleIndex == _cdiScales.size()) {
		_currentCdiScaleIndex = 0;
	}
}

void DCLGPS::DrawChar(char c, int field, int px, int py, bool bold) {
}

void DCLGPS::DrawText(const string& s, int field, int px, int py, bool bold) {
}

void DCLGPS::SetBaroUnits(int n, bool wrap) {
	if(n < 1) {
		_baroUnits = (GPSPressureUnits)(wrap ? 3 : 1);
	} else if(n > 3) {
		_baroUnits = (GPSPressureUnits)(wrap ? 1 : 3);
	} else {
		_baroUnits = (GPSPressureUnits)n;
	}
}

void DCLGPS::CreateDefaultFlightPlans() {}

// Get the time to the active waypoint in seconds.
// Returns -1 if groundspeed < 30 kts
double DCLGPS::GetTimeToActiveWaypoint() {
	if(_groundSpeed_kts < 30.0) {
		return(-1.0);
	} else {
		return(_eta);
	}
}

// Get the time to the final waypoint in seconds.
// Returns -1 if groundspeed < 30 kts
double DCLGPS::GetETE() {
	if(_groundSpeed_kts < 30.0) {
		return(-1.0);
	} else {
		// TODO - handle OBS / DTO operation appropriately
		if(_activeFP->waypoints.empty()) {
			return(-1.0);
		} else {
			return(GetTimeToWaypoint(_activeFP->waypoints[_activeFP->waypoints.size() - 1]->id));
		}
	}
}

// Get the time to a given waypoint (spec'd by ID) in seconds.
// returns -1 if groundspeed is less than 30kts.
// If the waypoint is an unreached part of the active flight plan the time will be via each leg.
// otherwise it will be a direct-to time.
double DCLGPS::GetTimeToWaypoint(const string& id) {
	if(_groundSpeed_kts < 30.0) {
		return(-1.0);
	}
	
	double eta = 0.0;
	int n1 = GetActiveWaypointIndex();
	int n2 = GetWaypointIndex(id);
	if(n2 > n1) {
		eta = _eta;
		for(unsigned int i=n1+1; i<_activeFP->waypoints.size(); ++i) {
			GPSWaypoint* wp1 = _activeFP->waypoints[i-1];
			GPSWaypoint* wp2 = _activeFP->waypoints[i];
			double distm = GetGreatCircleDistance(wp1->lat, wp1->lon, wp2->lat, wp2->lon) * SG_NM_TO_METER;
			eta += (distm / _groundSpeed_ms);
		}
		return(eta);
	} else if(id == _activeWaypoint.id) {
		return(_eta);
	} else {
		const GPSWaypoint* wp = FindFirstByExactId(id);
		if(wp == NULL) return(-1.0);
		double distm = GetGreatCircleDistance(_gpsLat, _gpsLon, wp->lat, wp->lon);
    delete wp;
		return(distm / _groundSpeed_ms);
	}
	return(-1.0);	// Hopefully we never get here!
}

// Returns magnetic great-circle heading
// TODO - document units.
float DCLGPS::GetHeadingToActiveWaypoint() {
	if(_activeWaypoint.id.empty()) {
		return(-888);
	} else {
		double h = GetMagHeadingFromTo(_gpsLat, _gpsLon, _activeWaypoint.lat, _activeWaypoint.lon);
		while(h <= 0.0) h += 360.0;
		while(h > 360.0) h -= 360.0;
		return((float)h);
	}
}

// Returns magnetic great-circle heading
// TODO - what units?
float DCLGPS::GetHeadingFromActiveWaypoint() {
	if(_activeWaypoint.id.empty()) {
		return(-888);
	} else {
		double h = GetMagHeadingFromTo(_activeWaypoint.lat, _activeWaypoint.lon, _gpsLat, _gpsLon);
		while(h <= 0.0) h += 360.0;
		while(h > 360.0) h -= 360.0;
		return(h);
	}
}

void DCLGPS::ClearFlightPlan(int n) {
	for(unsigned int i=0; i<_flightPlans[n]->waypoints.size(); ++i) {
		delete _flightPlans[n]->waypoints[i];
	}
	_flightPlans[n]->waypoints.clear();
}

void DCLGPS::ClearFlightPlan(GPSFlightPlan* fp) {
	for(unsigned int i=0; i<fp->waypoints.size(); ++i) {
		delete fp->waypoints[i];
	}
	fp->waypoints.clear();
}

int DCLGPS::GetActiveWaypointIndex() {
	for(unsigned int i=0; i<_flightPlans[0]->waypoints.size(); ++i) {
		if(_flightPlans[0]->waypoints[i]->id == _activeWaypoint.id) return((int)i);
	}
	return(-1);
}

int DCLGPS::GetWaypointIndex(const string& id) {
	for(unsigned int i=0; i<_flightPlans[0]->waypoints.size(); ++i) {
		if(_flightPlans[0]->waypoints[i]->id == id) return((int)i);
	}
	return(-1);
}

void DCLGPS::OrientateToFlightPlan(GPSFlightPlan* fp) {
	//cout << "Orientating...\n";
	//cout << "_lat = " << _lat << ", _lon = " << _lon << ", _gpsLat = " << _gpsLat << ", gpsLon = " << _gpsLon << '\n'; 
	if(fp->IsEmpty()) {
		_activeWaypoint.id.clear();
		_navFlagged = true;
	} else {
		_navFlagged = false;
		if(fp->waypoints.size() == 1) {
			// TODO - may need to flag nav here if not dto or obs, or possibly handle it somewhere else.
			_activeWaypoint = *fp->waypoints[0];
			_fromWaypoint.id.clear();
		} else {
			// FIXME FIXME FIXME
			_fromWaypoint = *fp->waypoints[0];
			_activeWaypoint = *fp->waypoints[1];
			double dmin = 1000000;	// nm!!
			// For now we will simply start on the leg closest to our current position.
			// It's possible that more fancy algorithms may take either heading or track
			// into account when setting inital leg - I'm not sure.
			// This method should handle most cases perfectly OK though.
			for(unsigned int i = 1; i < fp->waypoints.size(); ++i) {
				//cout << "Pass " << i << ", dmin = " << dmin << ", leg is " << fp->waypoints[i-1]->id << " to " << fp->waypoints[i]->id << '\n';
				// First get the cross track correction.
				double d0 = fabs(CalcCrossTrackDeviation(*fp->waypoints[i-1], *fp->waypoints[i]));
				// That is the shortest distance away we could be though - check for
				// longer distances if we are 'off the end' of the leg.
				double ht1 = GetGreatCircleCourse(fp->waypoints[i-1]->lat, fp->waypoints[i-1]->lon, 
				                                  fp->waypoints[i]->lat, fp->waypoints[i]->lon) 
												  * SG_RADIANS_TO_DEGREES;
				// not simply the reverse of the above due to great circle navigation.
				double ht2 = GetGreatCircleCourse(fp->waypoints[i]->lat, fp->waypoints[i]->lon, 
				                                  fp->waypoints[i-1]->lat, fp->waypoints[i-1]->lon) 
												  * SG_RADIANS_TO_DEGREES;
				double hw1 = GetGreatCircleCourse(_gpsLat, _gpsLon,
				                                  fp->waypoints[i]->lat, fp->waypoints[i]->lon) 
												  * SG_RADIANS_TO_DEGREES;
				double hw2 = GetGreatCircleCourse(_gpsLat, _gpsLon, 
				                                  fp->waypoints[i-1]->lat, fp->waypoints[i-1]->lon) 
												  * SG_RADIANS_TO_DEGREES;
				double h1 = ht1 - hw1;
				double h2 = ht2 - hw2;
				//cout << "d0, h1, h2 = " << d0 << ", " << h1 << ", " << h2 << '\n';
				//cout << "Normalizing...\n";
				SG_NORMALIZE_RANGE(h1, -180.0, 180.0);
				SG_NORMALIZE_RANGE(h2, -180.0, 180.0);
				//cout << "d0, h1, h2 = " << d0 << ", " << h1 << ", " << h2 << '\n';
				if(fabs(h1) > 90.0) {
					// We are past the end of the to waypoint
					double d = GetGreatCircleDistance(_gpsLat, _gpsLon, fp->waypoints[i]->lat, fp->waypoints[i]->lon);
					if(d > d0) d0 = d;
					//cout << "h1 triggered, d0 now = " << d0 << '\n';
				} else if(fabs(h2) > 90.0) {
					// We are past the end (not yet at!) the from waypoint
					double d = GetGreatCircleDistance(_gpsLat, _gpsLon, fp->waypoints[i-1]->lat, fp->waypoints[i-1]->lon);
					if(d > d0) d0 = d;
					//cout << "h2 triggered, d0 now = " << d0 << '\n';
				}
				if(d0 < dmin) {
					//cout << "THIS LEG NOW ACTIVE!\n";
					dmin = d0;
					_fromWaypoint = *fp->waypoints[i-1];
					_activeWaypoint = *fp->waypoints[i];
				}
			}
		}
	}
}

void DCLGPS::OrientateToActiveFlightPlan() {
	OrientateToFlightPlan(_activeFP);
}	

/***************************************/

// Utility function - create a flightplan from a list of waypoint ids and types
void DCLGPS::CreateFlightPlan(GPSFlightPlan* fp, vector<string> ids, vector<GPSWpType> wps) {
	if(fp == NULL) fp = new GPSFlightPlan;
	unsigned int i;
	if(!fp->waypoints.empty()) {
		for(i=0; i<fp->waypoints.size(); ++i) {
			delete fp->waypoints[i];
		}
		fp->waypoints.clear();
	}
	if(ids.size() != wps.size()) {
		cout << "ID and Waypoint types list size mismatch in GPS::CreateFlightPlan - no flightplan created!\n";
		return;
	}
	for(i=0; i<ids.size(); ++i) {
		bool multi;
		const FGAirport* ap;
		FGNavRecord* np;
		GPSWaypoint* wp = new GPSWaypoint;
		wp->type = wps[i];
		switch(wp->type) {
		case GPS_WP_APT:
			ap = FindFirstAptById(ids[i], multi, true);
			if(ap == NULL) {
				// error
				delete wp;
			} else {
				wp->lat = ap->getLatitude() * SG_DEGREES_TO_RADIANS;
				wp->lon = ap->getLongitude() * SG_DEGREES_TO_RADIANS;
				wp->id = ids[i];
				fp->waypoints.push_back(wp);
			}
			break;
		case GPS_WP_VOR:
			np = FindFirstVorById(ids[i], multi, true);
			if(np == NULL) {
				// error
				delete wp;
			} else {
				wp->lat = np->get_lat() * SG_DEGREES_TO_RADIANS;
				wp->lon = np->get_lon() * SG_DEGREES_TO_RADIANS;
				wp->id = ids[i];
				fp->waypoints.push_back(wp);
			}
			break;
		case GPS_WP_NDB:
			np = FindFirstNDBById(ids[i], multi, true);
			if(np == NULL) {
				// error
				delete wp;
			} else {
				wp->lat = np->get_lat() * SG_DEGREES_TO_RADIANS;
				wp->lon = np->get_lon() * SG_DEGREES_TO_RADIANS;
				wp->id = ids[i];
				fp->waypoints.push_back(wp);
			}
			break;
		case GPS_WP_INT:
			// TODO TODO
			break;
		case GPS_WP_USR:
			// TODO
			break;
		case GPS_WP_VIRT:
			// TODO
			break;
		}
	}
}

/***************************************/

class DCLGPSFilter : public FGPositioned::Filter
{
public:
  virtual bool pass(const FGPositioned* aPos) const {
    switch (aPos->type()) {
    case FGPositioned::AIRPORT:
    // how about heliports and seaports?
    case FGPositioned::NDB:
    case FGPositioned::VOR:
    case FGPositioned::WAYPOINT:
    case FGPositioned::FIX:
      break;
    default: return false; // reject all other types
    }
    return true;
  }
};

GPSWaypoint* DCLGPS::FindFirstById(const string& id) const
{
  DCLGPSFilter filter;
  FGPositionedRef result = FGPositioned::findNextWithPartialId(NULL, id, &filter);
  return GPSWaypoint::createFromPositioned(result);
}

GPSWaypoint* DCLGPS::FindFirstByExactId(const string& id) const
{
  SGGeod pos(SGGeod::fromRad(_lon, _lat));
  FGPositionedRef result = FGPositioned::findClosestWithIdent(id, pos);
  return GPSWaypoint::createFromPositioned(result);
}

// TODO - add the ASCII / alphabetical stuff from the Atlas version
FGPositioned* DCLGPS::FindTypedFirstById(const string& id, FGPositioned::Type ty, bool &multi, bool exact)
{
  multi = false;
  FGPositioned::TypeFilter filter(ty);
  
  if (exact) {
    FGPositioned::List matches = 
      FGPositioned::findAllWithIdent(id, &filter);
    FGPositioned::sortByRange(matches, SGGeod::fromRad(_lon, _lat));
    multi = (matches.size() > 1);
    return matches.empty() ? NULL : matches.front().ptr();
  }
  
  return FGPositioned::findNextWithPartialId(NULL, id, &filter);
}

FGNavRecord* DCLGPS::FindFirstVorById(const string& id, bool &multi, bool exact)
{
  return dynamic_cast<FGNavRecord*>(FindTypedFirstById(id, FGPositioned::VOR, multi, exact));
}

FGNavRecord* DCLGPS::FindFirstNDBById(const string& id, bool &multi, bool exact)
{
  return dynamic_cast<FGNavRecord*>(FindTypedFirstById(id, FGPositioned::NDB, multi, exact));
}

const FGFix* DCLGPS::FindFirstIntById(const string& id, bool &multi, bool exact)
{
  return dynamic_cast<FGFix*>(FindTypedFirstById(id, FGPositioned::FIX, multi, exact));
}

const FGAirport* DCLGPS::FindFirstAptById(const string& id, bool &multi, bool exact)
{
  return dynamic_cast<FGAirport*>(FindTypedFirstById(id, FGPositioned::AIRPORT, multi, exact));
}

FGNavRecord* DCLGPS::FindClosestVor(double lat_rad, double lon_rad) {  
  FGPositioned::TypeFilter filter(FGPositioned::VOR);
  double cutoff = 1000; // nautical miles
  FGPositionedRef v = FGPositioned::findClosest(SGGeod::fromRad(lon_rad, lat_rad), cutoff, &filter);
  if (!v) {
    return NULL;
  }
  
  return dynamic_cast<FGNavRecord*>(v.ptr());
}

//----------------------------------------------------------------------------------------------------------

// Takes lat and lon in RADIANS!!!!!!!
double DCLGPS::GetMagHeadingFromTo(double latA, double lonA, double latB, double lonB) {
	double h = GetGreatCircleCourse(latA, lonA, latB, lonB);
	h *= SG_RADIANS_TO_DEGREES;
	// TODO - use the real altitude below instead of 0.0!
	//cout << "MagVar = " << sgGetMagVar(_gpsLon, _gpsLat, 0.0, _time->getJD()) * SG_RADIANS_TO_DEGREES << '\n';
	h -= sgGetMagVar(_gpsLon, _gpsLat, 0.0, _time->getJD()) * SG_RADIANS_TO_DEGREES;
	while(h >= 360.0) h -= 360.0;
	while(h < 0.0) h += 360.0;
	return(h);
}

// ---------------- Great Circle formulae from "The Aviation Formulary" -------------
// Note that all of these assume that the world is spherical.

double Rad2Nm(double theta) {
	return(((180.0*60.0)/SG_PI)*theta);
}

double Nm2Rad(double d) {
	return((SG_PI/(180.0*60.0))*d);
}

/* QUOTE:

The great circle distance d between two points with coordinates {lat1,lon1} and {lat2,lon2} is given by:

d=acos(sin(lat1)*sin(lat2)+cos(lat1)*cos(lat2)*cos(lon1-lon2))

A mathematically equivalent formula, which is less subject to rounding error for short distances is:

d=2*asin(sqrt((sin((lat1-lat2)/2))^2 + 
                 cos(lat1)*cos(lat2)*(sin((lon1-lon2)/2))^2))
				 
*/

// Returns distance in nm, takes lat & lon in RADIANS
double DCLGPS::GetGreatCircleDistance(double lat1, double lon1, double lat2, double lon2) const {
	double d = 2.0 * asin(sqrt(((sin((lat1-lat2)/2.0))*(sin((lat1-lat2)/2.0))) +
	           cos(lat1)*cos(lat2)*(sin((lon1-lon2)/2.0))*(sin((lon1-lon2)/2.0))));
	return(Rad2Nm(d));
}

// fmod dosen't do what we want :-( 
static double mod(double d1, double d2) {
	return(d1 - d2*floor(d1/d2));
}

// Returns great circle course from point 1 to point 2
// Input and output in RADIANS.
double DCLGPS::GetGreatCircleCourse (double lat1, double lon1, double lat2, double lon2) const {
	//double h = 0.0;
	/*
	// Special case the poles
	if(cos(lat1) < SG_EPSILON) {
		if(lat1 > 0) {
			// Starting from North Pole
			h = SG_PI;
		} else {
			// Starting from South Pole
			h = 2.0 * SG_PI;
		}
	} else {
		// Urgh - the formula below is for negative lon +ve !!!???
		double d = GetGreatCircleDistance(lat1, lon1, lat2, lon2);
		cout << "d = " << d;
		d = Nm2Rad(d);
		//cout << ", d_theta = " << d;
		//cout << ", and d = " << Rad2Nm(d) << ' ';
		if(sin(lon2 - lon1) < 0) {
			cout << " A ";
			h = acos((sin(lat2)-sin(lat1)*cos(d))/(sin(d)*cos(lat1)));
		} else {
			cout << " B ";
			h = 2.0 * SG_PI - acos((sin(lat2)-sin(lat1)*cos(d))/(sin(d)*cos(lat1)));
		}
	}
	cout << h * SG_RADIANS_TO_DEGREES << '\n';
	*/
	
	return( mod(atan2(sin(lon2-lon1)*cos(lat2),
            cos(lat1)*sin(lat2)-sin(lat1)*cos(lat2)*cos(lon2-lon1)),
            2.0*SG_PI) );
}

// Return a position on a radial from wp1 given distance d (nm) and magnetic heading h (degrees)
// Note that d should be less that 1/4 Earth diameter!
GPSWaypoint DCLGPS::GetPositionOnMagRadial(const GPSWaypoint& wp1, double d, double h) {
	h += sgGetMagVar(wp1.lon, wp1.lat, 0.0, _time->getJD()) * SG_RADIANS_TO_DEGREES;
	return(GetPositionOnRadial(wp1, d, h));
}

// Return a position on a radial from wp1 given distance d (nm) and TRUE heading h (degrees)
// Note that d should be less that 1/4 Earth diameter!
GPSWaypoint DCLGPS::GetPositionOnRadial(const GPSWaypoint& wp1, double d, double h) {
	while(h < 0.0) h += 360.0;
	while(h > 360.0) h -= 360.0;
	
	h *= SG_DEGREES_TO_RADIANS;
	d *= (SG_PI / (180.0 * 60.0));
	
	double lat=asin(sin(wp1.lat)*cos(d)+cos(wp1.lat)*sin(d)*cos(h));
	double lon;
	if(cos(lat)==0) {
		lon=wp1.lon;      // endpoint a pole
	} else {
		lon=mod(wp1.lon+asin(sin(h)*sin(d)/cos(lat))+SG_PI,2*SG_PI)-SG_PI;
	}
	
	GPSWaypoint wp;
	wp.lat = lat;
	wp.lon = lon;
	wp.type = GPS_WP_VIRT;
	return(wp);
}

// Returns cross-track deviation in Nm.
double DCLGPS::CalcCrossTrackDeviation() const {
	return(CalcCrossTrackDeviation(_fromWaypoint, _activeWaypoint));
}

// Returns cross-track deviation of the current position between two arbitary waypoints in nm.
double DCLGPS::CalcCrossTrackDeviation(const GPSWaypoint& wp1, const GPSWaypoint& wp2) const {
	//if(wp1 == NULL || wp2 == NULL) return(0.0);
	if(wp1.id.empty() || wp2.id.empty()) return(0.0);
	double xtd = asin(sin(Nm2Rad(GetGreatCircleDistance(wp1.lat, wp1.lon, _gpsLat, _gpsLon))) 
	                  * sin(GetGreatCircleCourse(wp1.lat, wp1.lon, _gpsLat, _gpsLon) - GetGreatCircleCourse(wp1.lat, wp1.lon, wp2.lat, wp2.lon)));
	return(Rad2Nm(xtd));
}
