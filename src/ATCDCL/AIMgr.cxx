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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <iostream>

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/math/sg_random.h>
#include <simgear/scene/model/modellib.hxx>
#include <list>

#ifdef _MSC_VER
#  include <io.h>
#else
#  include <sys/types.h>	// for directory reading
#  include <dirent.h>		// for directory reading
#endif

#include <Environment/environment_mgr.hxx>
#include <Environment/environment.hxx>

#include "AIMgr.hxx"
#include "AILocalTraffic.hxx"
#include "AIGAVFRTraffic.hxx"
#include "ATCutils.hxx"
#include "commlist.hxx"

using std::list;
using std::cout;

using namespace simgear;

// extern from Airports/simple.cxx
extern Point3D fgGetAirportPos( const std::string& id );

FGAIMgr::FGAIMgr() {
	ATC = globals->get_ATC_mgr();
	initDone = false;
	ai_callsigns_used["GFS"] = 1;	// so we don't inadvertently use this
	// TODO - use the proper user callsign when it becomes user settable.
	removalList.clear();
	activated.clear();
	_havePiperModel = true;
}

FGAIMgr::~FGAIMgr() {
    for (ai_list_itr = ai_list.begin(); ai_list_itr != ai_list.end(); ai_list_itr++) {
        delete (*ai_list_itr);
    }
}

void FGAIMgr::init() {
	//cout << "AIMgr::init called..." << endl;
	
	// Pointers to user's position
	lon_node = fgGetNode("/position/longitude-deg", true);
	lat_node = fgGetNode("/position/latitude-deg", true);
	elev_node = fgGetNode("/position/altitude-ft", true);

	lon = lon_node->getDoubleValue();
	lat = lat_node->getDoubleValue();
	elev = elev_node->getDoubleValue();
	
	// Load up models at the start to avoid pausing later
	// Hack alert - Hardwired paths!!
	string planepath = "Aircraft/c172p/Models/c172p.xml";
	bool _loadedDefaultOK = true;
	try {
		_defaultModel = SGModelLib::loadPagedModel(planepath.c_str(), globals->get_props());
	} catch(sg_exception&) {
		_loadedDefaultOK = false;
	}
	
	if(!_loadedDefaultOK ) {
		// Just load the same 3D model as the default user plane - that's *bound* to exist!
		// TODO - implement robust determination of availability of GA AI aircraft models
		planepath = "Aircraft/c172p/Models/c172p.ac";
		_defaultModel = SGModelLib::loadPagedModel(planepath.c_str(), globals->get_props());
	}

	planepath = "Aircraft/pa28-161/Models/pa28-161.ac";
	try {
		_piperModel = SGModelLib::loadPagedModel(planepath.c_str(), globals->get_props());
	} catch(sg_exception&) {
		_havePiperModel = false;
	}

	// go through the $FG_ROOT/ATC directory and find all *.taxi files
	SGPath path(globals->get_fg_root());
	path.append("ATC/");
	string dir = path.dir();
	string ext;
	string file, f_ident;
	int pos;
	
	ulDir *d;
	struct ulDirEnt *de;
	
	if ( (d = ulOpenDir( dir.c_str() )) == NULL ) {
		SG_LOG(SG_ATC, SG_WARN, "cannot open directory " << dir);
	} else {
		// load all .taxi files
		while ( (de = ulReadDir(d)) != NULL ) {
			file = de->d_name;
			pos = file.find(".");
			ext = file.substr(pos + 1);
			if(ext == "taxi") {
				f_ident = file.substr(0, pos);
				const FGAirport *a = fgFindAirportID( f_ident);
				if(a){
					SGBucket sgb(a->getLongitude(), a->getLatitude());
					int idx = sgb.gen_index();
					if(facilities.find(idx) != facilities.end()) {
						facilities[idx]->push_back(f_ident);
					} else {
						ID_list_type* apts = new ID_list_type;
						apts->push_back(f_ident);
						facilities[idx] = apts;
					}
					SG_LOG(SG_ATC, SG_BULK, "Mapping " << f_ident << " to bucket " << idx);
				}
			}
		}		
		ulCloseDir(d);
	}

	// See if are in range at startup and activate if necessary
	SearchByPos(15.0);
	
	initDone = true;
	
	//cout << "AIMgr::init done..." << endl;
	
	/*
	// TESTING
	FGATCAlignedProjection ortho;
	ortho.Init(fgGetAirportPos("KEMT"), 205.0);	// Guess of rwy19 heading
	//Point3D ip = ortho.ConvertFromLocal(Point3D(6000, 1000, 1000));	// 90 deg entry
	//Point3D ip = ortho.ConvertFromLocal(Point3D(-7000, 3000, 1000));	// 45 deg entry
	Point3D ip = ortho.ConvertFromLocal(Point3D(1000, -7000, 1000));	// straight-in
	ATC->AIRegisterAirport("KEMT");
	FGAIGAVFRTraffic* p = new FGAIGAVFRTraffic();
	p->SetModel(_defaultModel);
	p->Init(ip, "KEMT", GenerateShortForm(GenerateUniqueCallsign()));
	ai_list.push_back(p);
	traffic[ident].push_back(p);
	activated["KEMT"] = 1;
	*/	
}

void FGAIMgr::bind() {
}

void FGAIMgr::unbind() {
}

void FGAIMgr::update(double dt) {
	if(!initDone) {
		init();
		SG_LOG(SG_ATC, SG_WARN, "Warning - AIMgr::update(...) called before AIMgr::init()");
	}
	
	//cout << activated.size() << '\n';
	
	Point3D userPos = Point3D(lon_node->getDoubleValue(), lat_node->getDoubleValue(), elev_node->getDoubleValue());
	
	// TODO - make these class variables!!
	static int i = 0;
	static int j = 0;

	// Don't update any planes for first 50 runs through - this avoids some possible initialisation anomalies
	// Might not need it now we have fade-in though?
	if(i < 50) {
		++i;
		return;
	}
	
	if(j == 215) {
		SearchByPos(25.0);
		j = 0;
	} else if(j == 200) {
		// Go through the list of activated airports and remove those out of range
		//cout << "The following airports have been activated by the AI system:\n";
		ai_activated_map_iterator apt_itr = activated.begin();
		while(apt_itr != activated.end()) {
			//cout << "FIRST IS " << (*apt_itr).first << '\n';
			if(dclGetHorizontalSeparation(userPos, fgGetAirportPos((*apt_itr).first)) > (35.0 * 1600.0)) {
				// Then get rid of it and make sure the iterator is left pointing to the next one!
				string s = (*apt_itr).first;
				if(traffic.find(s) != traffic.end()) {
					//cout << "s = " << s << ", traffic[s].size() = " << traffic[s].size() << '\n';
					if(!traffic[s].empty()) {
						apt_itr++;
					} else {
						//cout << "Erasing " << (*apt_itr).first << " and traffic" << '\n';
						activated.erase(apt_itr);
						apt_itr = activated.upper_bound(s);
						traffic.erase(s);
					}
				} else {
						//cout << "Erasing " << (*apt_itr).first << ' ' << (*apt_itr).second << '\n';
						activated.erase(apt_itr);
						apt_itr = activated.upper_bound(s);
				}
			} else {
				apt_itr++;
			}
		}
	} else if(j == 180) {
		// Go through the list of activated airports and do the random airplane generation
		ai_traffic_map_iterator it = traffic.begin();
		while(it != traffic.end()) {
			string s = (*it).first;
			//cout << "s = " << s << " size = " << (*it).second.size() << '\n';
			// Only generate extra traffic if within a certain distance of the user,
			// TODO - maybe take users's tuned freq into account as well.
			double d = dclGetHorizontalSeparation(userPos, fgGetAirportPos(s)); 
			if(d < (15.0 * 1600.0)) {
				double cd = 0.0;
				bool gen = false;
				//cout << "Size of list is " << (*it).second.size() << " at " << s << '\n';
				if((*it).second.size()) {
					FGAIEntity* e = *((*it).second.rbegin());	// Get the last airplane currently scheduled to arrive at this airport.
					cd = dclGetHorizontalSeparation(e->GetPos(), fgGetAirportPos(s));
					if(cd < (d < 5000 ? 10000 : d + 5000)) {
						gen = true;
					}
				} else {
					gen = true;
					cd = 0.0;
				}
				if(gen) {
					//cout << "Generating extra traffic at airport " << s << ", at least " << cd << " meters out\n";
					//GenerateSimpleAirportTraffic(s, cd);
					GenerateSimpleAirportTraffic(s, cd + 3000.0);	// The random seems a bit wierd - traffic could get far too bunched without the +3000.
					// TODO - make the anti-random constant variable depending on the ai-traffic level.
				}
			}
			++it;
		}
	}
	
	++j;
	
	//cout << "Size of AI list is " << ai_list.size() << '\n';
	
	// TODO - need to add a check of if any activated airports have gone out of range
	
	string rs;	// plane to be removed, if one.
	if(removalList.size()) {
		rs = *(removalList.begin());
		removalList.pop_front();
	} else {
		rs = "";
	}
	
	// Traverse the list of active planes and run all their update methods
	// TODO - spread the load - not all planes should need updating every frame.
	// Note that this will require dt to be calculated for each plane though
	// since they rely on it to calculate distance travelled.
	ai_list_itr = ai_list.begin();
	while(ai_list_itr != ai_list.end()) {
		FGAIEntity *e = *ai_list_itr;
		if(rs.size() && e->GetCallsign() == rs) {
			//cout << "Removing " << rs << " from ai_list\n";
			ai_list_itr = ai_list.erase(ai_list_itr);
			delete e;
			// This is a hack - we should deref this plane from the airport count!
		} else {
			e->Update(dt);
			++ai_list_itr;
		}
	}

	//cout << "Size of AI list is " << ai_list.size() << '\n';
}

void FGAIMgr::ScheduleRemoval(const string& s) {
	//cout << "Scheduling removal of plane " << s << " from AIMgr\n";
	removalList.push_back(s);
}

// Activate AI traffic at an airport
void FGAIMgr::ActivateAirport(const string& ident) {
	ATC->AIRegisterAirport(ident);
	// TODO - need to start the traffic more randomly
	FGAILocalTraffic* local_traffic = new FGAILocalTraffic;
	local_traffic->SetModel(_defaultModel.get());	// currently hardwired to cessna.
	//local_traffic->Init(ident, IN_PATTERN, TAKEOFF_ROLL);
	local_traffic->Init(GenerateShortForm(GenerateUniqueCallsign()), ident);
	local_traffic->FlyCircuits(1, true);	// Fly 2 circuits with touch & go in between
	ai_list.push_back(local_traffic);
	traffic[ident].push_back(local_traffic);
	//cout << "******** ACTIVATING AIRPORT, ident = " << ident << '\n';
	activated[ident] = 1;
}

// Hack - Generate AI traffic at an airport with no facilities file
void FGAIMgr::GenerateSimpleAirportTraffic(const string& ident, double min_dist) {
	// Ugly hack - don't let VFR Cessnas operate at a hardwired list of major airports
	// This will go eventually once airport .xml files specify the traffic profile
	if(ident == "KSFO" || ident == "KDFW" || ident == "EGLL" || ident == "KORD" || ident == "KJFK" 
	                   || ident == "KMSP" || ident == "KLAX" || ident == "KBOS" || ident == "KEDW"
					   || ident == "KSEA" || ident == "EHAM") {
		return;
	}
	
	/*
	// TODO - check for military airports - this should be in the current data.
	// UGGH - there's no point at the moment - everything is labelled civil in basic.dat!
	FGAirport a = fgFindAirportID(ident, &a);
	if(a) {
		cout << "CODE IS " << a.code << '\n';
	} else {
		// UG - can't find the airport!
		return;
	}
	*/
	
	Point3D aptpos = fgGetAirportPos(ident); 	// TODO - check for elev of -9999
	//cout << "ident = " << ident << ", elev = " << aptpos.elev() << '\n';
	
	// Operate from airports at 3000ft and below only to avoid the default cloud layers and since we don't degrade AI performance with altitude.
	if(aptpos.elev() > 3000) {
		//cout << "High alt airports not yet supported - returning\n";
		return;
	}
	
	// Rough hack for plane type - make 70% of the planes cessnas, the rest pipers.
	bool cessna = true;
	
	// Get the time and only operate VFR in the (approximate) daytime.
	struct tm *t = globals->get_time_params()->getGmt();
	int loc_time = t->tm_hour + int(aptpos.lon() / (360.0 / 24.0));
	while (loc_time < 0)
		loc_time += 24;
	while (loc_time >= 24)
		loc_time -= 24;

	//cout << "loc_time = " << loc_time << '\n';
	if(loc_time < 7 || loc_time > 19) return;
	
	// Check that the visibility is OK for IFR operation.
	double visibility;
	FGEnvironment stationweather =
            ((FGEnvironmentMgr *)globals->get_subsystem("environment"))
              ->getEnvironment(aptpos.lat(), aptpos.lon(), aptpos.elev());	// TODO - check whether this should take ft or m for elev.
	visibility = stationweather.get_visibility_m();
	// Technically we can do VFR down to 1 mile (1600m) but that's pretty murky!
	//cout << "vis = " << visibility << '\n';
	if(visibility < 3000) return;
	
	ATC->AIRegisterAirport(ident);
	
	// Next - get the distance from user to the airport.
	Point3D userpos = Point3D(lon_node->getDoubleValue(), lat_node->getDoubleValue(), elev_node->getDoubleValue());
	double d = dclGetHorizontalSeparation(userpos, aptpos);	// in meters
	
	int lev = fgGetInt("/sim/ai-traffic/level");
	if(lev < 1)
		return;
	if (lev > 3)
		lev = 3;
	if(visibility < 6000) lev = 1;
	//cout << "level = " << lev << '\n';
	
	// Next - generate any local / circuit traffic

	/*
	// --------------------------- THIS BLOCK IS JUST FOR TESTING - COMMENT OUT BEFORE RELEASE ---------------
	// Finally - generate VFR approaching traffic
	//if(d > 2000) {
	if(ident == "KPOC") {
		double ad = 2000.0;
		double avd = 3000.0;	// average spacing of arriving traffic in meters - relate to airport business and AI density setting one day!
		//while(ad < (d < 10000 ? 12000 : d + 2000)) {
		for(int i=0; i<8; ++i) {
			double dd = sg_random() * avd;
			// put a minimum spacing in for now since I don't think tower will cope otherwise!
			if(dd < 1500) dd = 1500; 
			//ad += dd;
			ad += dd;
			double dir = int(sg_random() * 36);
			if(dir == 36) dir--;
			dir *= 10;
			//dir = 180;
			if(sg_random() < 0.3) cessna = false;
			else cessna = true;
			string s = GenerateShortForm(GenerateUniqueCallsign(), (cessna ? "Cessna-" : "Piper-"));
			FGAIGAVFRTraffic* t = new FGAIGAVFRTraffic();
			t->SetModel(cessna ? _defaultModel : _piperModel);
			//cout << "Generating VFR traffic " << s << " inbound to " << ident << " " << ad << " meters out from " << dir << " degrees\n";
			Point3D tpos = dclUpdatePosition(aptpos, dir, 6.0, ad);
			if(tpos.elev() > (aptpos.elev() + 3000.0)) tpos.setelev(aptpos.elev() + 3000.0);
			t->Init(tpos, ident, s);
			ai_list.push_back(t);
		}
	}
	activated[ident] = 1;
	return;
	//---------------------------------------------------------------------------------------------------
	*/
	
	double ad;   // Minimum distance out of first arriving plane in meters.
	double mind; // Minimum spacing of traffic in meters
	double avd;  // average spacing of arriving traffic in meters - relate to airport business and AI density setting one day!
	// Finally - generate VFR approaching traffic
	//if(d > 2000) {
	if(1) {
		if(lev == 3) {
			ad = 5000.0;
			mind = 2000.0;
			avd = 6000.0;
		} else if(lev == 2) {
			ad = 8000.0;
			mind = 4000.0;
			avd = 10000.0;
		} else {
			ad = 9000.0;	// Start the first aircraft at least 9K out for now.
			mind = 6000.0;
			avd = 15000.0;
		}
		/*
		// Check if there is already arriving traffic at this airport
		cout << "BING A " << ident << '\n';
		if(traffic.find(ident) != traffic.end()) {
			cout << "BING B " << ident << '\n';
			ai_list_type lst = traffic[ident];
			cout << "BING C " << ident << '\n';
			if(lst.size()) {
				cout << "BING D " << ident << '\n';
				double cd = dclGetHorizontalSeparation(aptpos, (*lst.rbegin())->GetPos());
				cout << "ident = " << ident << ", cd = " << cd << '\n';
				if(cd > ad) ad = cd;
			}
		}
		*/
		if(min_dist != 0) ad = min_dist;
		//cout << "ident = " << ident << ", ad = " << ad << '\n';
		while(ad < (d < 5000 ? 15000 : d + 10000)) {
			double dd = mind + (sg_random() * (avd - mind));
			ad += dd;
			double dir = int(sg_random() * 36);
			if(dir == 36) dir--;
			dir *= 10;
			
			if(sg_random() < 0.3) cessna = false;
			else cessna = true;
			string s = GenerateShortForm(GenerateUniqueCallsign(), (cessna ? "Cessna-" : "Piper-"));
			FGAIGAVFRTraffic* t = new FGAIGAVFRTraffic();
			t->SetModel(cessna ? _defaultModel.get() : (_havePiperModel ? _piperModel.get() : _defaultModel.get()));
			//cout << "Generating VFR traffic " << s << " inbound to " << ident << " " << ad << " meters out from " << dir << " degrees\n";
			Point3D tpos = dclUpdatePosition(aptpos, dir, 6.0, ad);
			if(tpos.elev() > (aptpos.elev() + 3000.0)) tpos.setelev(aptpos.elev() + 3000.0);	// FEET yuk :-(
			t->Init(tpos, ident, s);
			ai_list.push_back(t);
			traffic[ident].push_back(t);
		}
	}	
}

/*
// Generate a VFR arrival at airport apt, at least distance d (meters) out.
void FGAIMgr::GenerateVFRArrival(const string& apt, double d) {
}
*/

// Search for valid airports in the vicinity of the user and activate them if necessary
void FGAIMgr::SearchByPos(double range) {
	//cout << "In SearchByPos(...)" << endl;
	
	// get bucket number for plane position
	lon = lon_node->getDoubleValue();
	lat = lat_node->getDoubleValue();
	elev = elev_node->getDoubleValue() * SG_FEET_TO_METER;
	SGBucket buck(lon, lat);

	// get neigboring buckets
	int bx = (int)( range*SG_NM_TO_METER / buck.get_width_m() / 2);
	//cout << "bx = " << bx << endl;
	int by = (int)( range*SG_NM_TO_METER / buck.get_height_m() / 2 );
	//cout << "by = " << by << endl;
	
	// Search for airports with facitities files --------------------------
	// loop over bucket range 
	for ( int i=-bx; i<=bx; i++) {
		//cout << "i loop\n";
		for ( int j=-by; j<=by; j++) {
			//cout << "j loop\n";
			buck = sgBucketOffset(lon, lat, i, j);
			long int bucket = buck.gen_index();
			//cout << "bucket is " << bucket << endl;
			if(facilities.find(bucket) != facilities.end()) {
				ID_list_type* apts = facilities[bucket];
				ID_list_iterator current = apts->begin();
				ID_list_iterator last = apts->end();
				
				//cout << "Size of apts is " << apts->size() << endl;
				
				//double rlon = lon * SGD_DEGREES_TO_RADIANS;
				//double rlat = lat * SGD_DEGREES_TO_RADIANS;
				//Point3D aircraft = sgGeodToCart( Point3D(rlon, rlat, elev) );
				//Point3D airport;
				for(; current != last; ++current) {
					//cout << "Found " << *current << endl;;
					if(activated.find(*current) == activated.end()) {
						//cout << "Activating " << *current << endl;
						//FGAirport a;
						//if(dclFindAirportID(*current, &a)) {
							//	// We can do something here based on distance from the user if we wish.
						//}
						//string s = *current;
						//cout << "s = " << s << '\n';
						ActivateAirport(*current);
						//ActivateSimpleAirport(*current);	// TODO - put this back to ActivateAirport when that code is done.
						//cout << "Activation done" << endl;
					} else {
						//cout << *current << " already activated" << endl;
					}
				}
			}
		}
	}
	//-------------------------------------------------------------
	
	// Search for any towered airports in the vicinity ------------
	comm_list_type towered;
	comm_list_iterator twd_itr;
	
	int num_twd = current_commlist->FindByPos(lon, lat, elev, range, &towered, TOWER);
	if (num_twd != 0) {
		double closest = 1000000;
		string s = "";
		for(twd_itr = towered.begin(); twd_itr != towered.end(); twd_itr++) {
			// Only activate the closest airport not already activated each time.
			if(activated.find(twd_itr->ident) == activated.end()) {
				double sep = dclGetHorizontalSeparation(Point3D(lon, lat, elev), fgGetAirportPos(twd_itr->ident));
				if(sep < closest) {
					closest = sep;
					s = twd_itr->ident;
				}
				
			}
		}
		if(s.size()) {
			// TODO - find out why empty strings come through here when all in-range airports done.
			GenerateSimpleAirportTraffic(s);
			//cout << "**************ACTIVATING SIMPLE AIRPORT, ident = " << s << '\n';
			activated[s] = 1;
		}
	}
}

string FGAIMgr::GenerateCallsign() {
	// For now we'll just generate US callsigns until we can regionally identify airports.
	string s = "N";
	// Add 3 to 5 numbers and make up to 5 with letters.
	//sg_srandom_time();
	double d = sg_random();
	int n = int(d * 3);
	if(n == 3) --n;
	//cout << "First n, n = " << n << '\n';
	int j = 3 + n;
	//cout << "j = " << j << '\n';
	for(int i=0; i<j; ++i) { 
		int n = int(sg_random() * 10);
		if(n == 10) --n;
		s += (char)('0' + n);
	}
	for(int i=j; i<5; ++i) {
		int n = int(sg_random() * 26);
		if(n == 26) --n;
		//cout << "Alpha, n = " << n << '\n';
		s += (char)('A' + n);
	}
	//cout << "s = " << s << '\n';
	return(s);
}

string FGAIMgr::GenerateUniqueCallsign() {
	while(1) {
		string s = GenerateCallsign();
		if(!ai_callsigns_used[s]) {
			ai_callsigns_used[s] = 1;
			return(s);
		}
	}
}

// This will be moved somewhere else eventually!!!!
string FGAIMgr::GenerateShortForm(const string& callsign, const string& plane_str, bool local) {
	//cout << callsign << '\n';
	string s;
	if(local) s = "Trainer-";
	else s = plane_str;
	for(int i=3; i>0; --i) {
		char c = callsign[callsign.size() - i];
		//cout << c << '\n';
		string tmp = "";
		tmp += c;
		if(isalpha(c)) s += GetPhoneticIdent(c);
		else s += ConvertNumToSpokenDigits(tmp);
		if(i > 1) s += '-';
	}
	return(s);
}
