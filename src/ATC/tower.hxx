// FGTower - a class to provide tower control at towered airports.
//
// Written by David Luff, started March 2002.
//
// Copyright (C) 2002  David C. Luff - david.luff@nottingham.ac.uk
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

#ifndef _FG_TOWER_HXX
#define _FG_TOWER_HXX

#include <simgear/compiler.h>
#include <simgear/math/point3d.hxx>
#include <simgear/misc/sgstream.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <plib/sg.h>

#include STL_IOSTREAM
#include STL_STRING

SG_USING_STD(string);
#ifndef SG_HAVE_NATIVE_SGI_COMPILERS
SG_USING_STD(ios);
#endif

#include "ATC.hxx"
//#include "ATCmgr.hxx"
#include "ground.hxx"

//DCL - a complete guess for now.
#define FG_TOWER_DEFAULT_RANGE 30

// Structure for holding details of a plane under tower control.
// Not fixed yet - may include more stuff later.
class TowerPlaneRec {
	
	public:
	
	TowerPlaneRec();
	TowerPlaneRec(string ID);
	TowerPlaneRec(Point3D pt);
	TowerPlaneRec(string ID, Point3D pt);
	
	string id;
	Point3D pos;
	double eta;		// minutes
	double dist_out;	// miles from theshold
	bool clearedToLand;
	bool clearedToDepart;
	// ought to add time cleared to depart so we can nag if necessary
	bool longFinalReported;
	bool longFinalAcknowledged;
	bool finalReported;
	bool finalAcknowledged;
	bool onRwy;
	// enum type - light, medium, heavy etc - we need someway of approximating the aircraft type and performance.
};

typedef list < TowerPlaneRec* > tower_plane_rec_list_type;
typedef list < TowerPlaneRec* >::iterator tower_plane_rec_list_iterator;
typedef list < TowerPlaneRec* >::const_iterator tower_plane_rec_list_const_iterator;


class FGTower : public FGATC {

	public:
	
	FGTower();
	~FGTower();
	
	void Init();
	
	void Update();
	
	void ReportFinal(string ID);
	void ReportLongFinal(string ID);
	void ReportOuterMarker(string ID);
	void ReportMiddleMarker(string ID);
	void ReportInnerMarker(string ID);
	void ReportGoingAround(string ID);
	void ReportRunwayVacated(string ID);
	
	// Parse a literal message to decide which of above it represents. 
	// (a long term project that eventually will hopefully receive the output from voice recognition software.)
	void LiteralTransmission(string trns, string ID);
	
	inline void SetDisplay() {display = true;}
	inline void SetNoDisplay() {display = false;}
	
	inline char get_type() const { return type; }
	inline double get_lon() const { return lon; }
	inline double get_lat() const { return lat; }
	inline double get_elev() const { return elev; }
	inline double get_x() const { return x; }
	inline double get_y() const { return y; }
	inline double get_z() const { return z; }
	inline int get_freq() const { return freq; }
	inline int get_range() const { return range; }
	inline const char* GetIdent() { return ident.c_str(); }
	inline string get_trans_ident() { return trans_ident; }
	inline string get_name() { return name; }
	inline atc_type GetType() { return TOWER; }
	
	// Make a request of tower control
	//void Request(tower_request request);
	
	private:
	
	void IssueLandingClearance(TowerPlaneRec* tpr);
	void IssueGoAround(TowerPlaneRec* tpr);
	void IssueDepartureClearance(TowerPlaneRec* tpr);
	
	char type;
	double lon, lat;
	double elev;
	double x, y, z;
	int freq;
	int range;
	bool display;		// Flag to indicate whether we should be outputting to the ATC display.
	bool displaying;		// Flag to indicate whether we are outputting to the ATC display.
	string ident;		// Code of the airport its at.
	string name;		// Name generally used in transmissions.

	
	// Need a data structure to hold details of the various active planes
	// or possibly another data structure with the positions of the inactive planes.
	// Need a data structure to hold outstanding communications from aircraft.
	
	// Linked-list of planes on approach ordered with nearest first (timewise).
	// Includes planes that have landed but not yet vacated runway.
	// Somewhat analagous to the paper strips used (used to be used?) in real life.
	tower_plane_rec_list_type appList;
	
	// List of departed planes
	tower_plane_rec_list_type depList;
	
	// List of planes waiting to depart
	tower_plane_rec_list_type holdList;
	
	// List of planes on rwy
	tower_plane_rec_list_type rwyList;

	// Ground can be separate or handled by tower in real life.
	// In the program we will always use a separate FGGround class, but we need to know
	// whether it is supposed to be separate or not to give the correct instructions.
	bool separateGround;	// true if ground control is separate
	FGGround* groundPtr;	// The ground control associated with this airport.
	
	
	// for failure modeling
	string trans_ident;		// transmitted ident
	bool tower_failed;		// tower failed?
	
	friend istream& operator>> ( istream&, FGTower& );
};


inline istream&
operator >> ( istream& in, FGTower& t )
{
	double f;
	char ch;
	
	in >> t.type;
	
	if ( t.type == '[' )
		return in >> skipeol;
	
	in >> t.lat >> t.lon >> t.elev >> f >> t.range 
	>> t.ident;
	
	t.name = "";
	in >> ch;
	t.name += ch;
	while(1) {
		//in >> noskipws
		in.unsetf(ios::skipws);
		in >> ch;
		t.name += ch;
		if((ch == '"') || (ch == 0x0A)) {
			break;
		}   // we shouldn't need the 0x0A but it makes a nice safely in case someone leaves off the "
	}
	in.setf(ios::skipws);
	//cout << "tower.name = " << t.name << '\n';
	
	t.freq = (int)(f*100.0 + 0.5);
	
	// cout << t.ident << endl;
	
	// generate cartesian coordinates
	Point3D geod( t.lon * SGD_DEGREES_TO_RADIANS, t.lat * SGD_DEGREES_TO_RADIANS, t.elev );
	Point3D cart = sgGeodToCart( geod );
	t.x = cart.x();
	t.y = cart.y();
	t.z = cart.z();
	
	t.trans_ident = t.ident;
	t.tower_failed = false;
	
	return in >> skipeol;
}


#endif  //_FG_TOWER_HXX
