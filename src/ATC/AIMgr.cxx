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
#include <Simgear/misc/sg_path.hxx>
#include <Simgear/bucket/newbucket.hxx>

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


FGAIMgr::FGAIMgr() {
	ATC = globals->get_ATC_mgr();
}

FGAIMgr::~FGAIMgr() {
}

void FGAIMgr::init() {
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
					airports[idx] = f_ident;
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
					airports[idx] = f_ident;
					cout << "Mapping " << f_ident << " to bucket " << idx << '\n'; 
				}
			}
		}		
		closedir(d);
	}
#endif
	
	// Hard wire some local traffic for now.
	// This is regardless of location and hence *very* ugly but it is a start.
	ATC->AIRegisterAirport("KEMT");
	FGAILocalTraffic* local_traffic = new FGAILocalTraffic;
	//local_traffic->Init("KEMT", IN_PATTERN, TAKEOFF_ROLL);
	local_traffic->Init("KEMT");
	local_traffic->FlyCircuits(1, true);	// Fly 2 circuits with touch & go in between
	ai_list.push_back(local_traffic);
}

void FGAIMgr::bind() {
}

void FGAIMgr::unbind() {
}

void FGAIMgr::update(double dt) {
	// Don't update any planes for first 50 runs through - this avoids some possible initialisation anomalies
	static int i = 0;
	if(i < 50) {
		i++;
		return;
	}
	
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
