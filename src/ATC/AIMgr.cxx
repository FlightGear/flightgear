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

#include <Airports/simple.hxx>
#include <Main/fgfs.hxx>
#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/bucket/newbucket.hxx>

#include <list>

#ifdef _MSC_VER
#  include <io.h>
#else
#  include <sys/types.h>	// for directory reading
#  include <dirent.h>		// for directory reading
#endif

#include "AIMgr.hxx"
#include "AILocalTraffic.hxx"
#include "ATCutils.hxx"

SG_USING_STD(list);
SG_USING_STD(cout);

FGAIMgr::FGAIMgr() {
	ATC = globals->get_ATC_mgr();
}

FGAIMgr::~FGAIMgr() {
}

void FGAIMgr::init() {
	// Pointers to user's position
	lon_node = fgGetNode("/position/longitude-deg", true);
	lat_node = fgGetNode("/position/latitude-deg", true);
	elev_node = fgGetNode("/position/altitude-ft", true);
	
	// go through the $FG_ROOT/ATC directory and find all *.taxi files
	SGPath path(globals->get_fg_root());
	path.append("ATC/");
	string dir = path.dir();
    string ext;
    string file, f_ident;
	int pos;
	
	// WARNING - I (DCL) haven't tested this on MSVC - this is simply cribbed from TerraGear
#ifdef _MSC_VER	
	long hfile;
	struct _finddata_t de;
	string path_str;
	
	path_str = dir + "\\*.*";
	
	if ( ( hfile = _findfirst( path.c_str(), &de ) ) == -1 ) {
		cout << "cannot open directory " << dir << "\n";
	} else {		
		// load all .taxi files
		do {
			file = de.name;
			pos = file.find(".");
			ext = file.substr(pos + 1);
			if(ext == "taxi") {
				cout << "TAXI FILE FOUND!!!\n";
				f_ident = file.substr(0, pos);
				FGAirport a;
				if(dclFindAirportID(f_ident, &a)) {
					SGBucket sgb(a.longitude, a.latitude);
					int idx = sgb.gen_index();
					if(airports.find(idx) != airports.end()) {
						airports[idx]->push_back(f_ident);
					} else {
						aptID_list_type* apts = new aptID_list_type;
						apts->push_back(f_ident);
						airports[idx] = apts;
					}
					cout << "Mapping " << f_ident << " to bucket " << idx << '\n'; 
				}
			}
		} while ( _findnext( hfile, &de ) == 0 );
	}
#else

    DIR *d;
    struct dirent *de;

    if ( (d = opendir( dir.c_str() )) == NULL ) {
		cout << "cannot open directory " << dir << "\n";
	} else {
		cout << "Opened directory " << dir << " OK :-)\n";
		cout << "Contents are:\n";
		// load all .taxi files
		while ( (de = readdir(d)) != NULL ) {
			file = de->d_name;
			pos = file.find(".");
			cout << file << '\n';

			ext = file.substr(pos + 1);
			if(ext == "taxi") {
				cout << "TAXI FILE FOUND!!!\n";
				f_ident = file.substr(0, pos);
				FGAirport a;
				if(dclFindAirportID(f_ident, &a)) {
					SGBucket sgb(a.longitude, a.latitude);
					int idx = sgb.gen_index();
					if(airports.find(idx) != airports.end()) {
						airports[idx]->push_back(f_ident);
					} else {
						aptID_list_type* apts = new aptID_list_type;
						apts->push_back(f_ident);
						airports[idx] = apts;
					}
					cout << "Mapping " << f_ident << " to bucket " << idx << '\n'; 
				}
			}
		}		
		closedir(d);
	}
#endif
	
	// See if are in range at startup and activate if necessary
	SearchByPos(10.0);
}

void FGAIMgr::bind() {
}

void FGAIMgr::unbind() {
}

void FGAIMgr::update(double dt) {
	static int i = 0;
	static int j = 0;

	// Don't update any planes for first 50 runs through - this avoids some possible initialisation anomalies
	// Might not need it now we have fade-in though?
	if(i < 50) {
		++i;
		return;
	}
	
	if(j == 215) {
		SearchByPos(15.0);
		j = 0;
	}
	
	++j;
	
	// TODO - need to add a check of if any activated airports have gone out of range
	
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


// Activate AI traffic at an airport
void FGAIMgr::ActivateAirport(string ident) {
	ATC->AIRegisterAirport(ident);
	// TODO - need to start the traffic more randomly
	FGAILocalTraffic* local_traffic = new FGAILocalTraffic;
	//local_traffic->Init(ident, IN_PATTERN, TAKEOFF_ROLL);
	local_traffic->Init(ident);
	local_traffic->FlyCircuits(1, true);	// Fly 2 circuits with touch & go in between
	ai_list.push_back(local_traffic);
	activated[ident] = 1;
}	


// Search for valid airports in the vicinity of the user and activate them if necessary
void FGAIMgr::SearchByPos(double range)
{
	//cout << "In SearchByPos(...)" << endl;
	
	// get bucket number for plane position
	lon = lon_node->getDoubleValue();
	lat = lat_node->getDoubleValue();
	SGBucket buck(lon, lat);

	// get neigboring buckets
	int bx = (int)( range*SG_NM_TO_METER / buck.get_width_m() / 2);
	//cout << "bx = " << bx << endl;
	int by = (int)( range*SG_NM_TO_METER / buck.get_height_m() / 2 );
	//cout << "by = " << by << endl;
	
	// loop over bucket range 
	for ( int i=-bx; i<=bx; i++) {
		//cout << "i loop\n";
		for ( int j=-by; j<=by; j++) {
			//cout << "j loop\n";
			buck = sgBucketOffset(lon, lat, i, j);
			long int bucket = buck.gen_index();
			//cout << "bucket is " << bucket << endl;
			if(airports.find(bucket) != airports.end()) {
				aptID_list_type* apts = airports[bucket];
				aptID_list_iterator current = apts->begin();
				aptID_list_iterator last = apts->end();
				
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
						ActivateAirport(*current);
						//cout << "Activation done" << endl;
					} else {
						//cout << *current << " already activated" << endl;
					}
				}
			}
		}
	}
}
