// FGATC - abstract base class for the various actual atc classes 
// such as FGATIS, FGTower etc.
//
// Written by David Luff, started Feburary 2002.
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

#ifndef _FG_ATC_HXX
#define _FG_ATC_HXX

#include <simgear/compiler.h>
#include <simgear/misc/sgstream.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/debug/logstream.hxx>

#include STL_IOSTREAM
#include STL_STRING

#include "ATCVoice.hxx"

SG_USING_STD(ostream);
SG_USING_STD(string);
SG_USING_STD(ios);

enum plane_type {
	UNKNOWN,
	GA_SINGLE,
	GA_HP_SINGLE,
	GA_TWIN,
	GA_JET,
	MEDIUM,
	HEAVY,
	MIL_JET
};

// PlaneRec - a structure holding ATC-centric details of planes under control
// This might move or change eventually
struct PlaneRec {
	plane_type type;
	string callsign;
	int squawkcode;
};

// Possible types of ATC type that the radios may be tuned to.
// INVALID implies not tuned in to anything.
enum atc_type {
    INVALID,
    ATIS,
    GROUND,
    TOWER,
    APPROACH,
    DEPARTURE,
    ENROUTE
}; 

// DCL - new experimental ATC data store
struct ATCData {
	atc_type type;
	// I've deliberately used float instead of double here to keep the size down - we'll be storing thousands of these in memory.
	// In fact, we could probably ditch x, y and z and generate on the fly as needed.
	// On the other hand, we'll probably end up reading this data directly from the DAFIF eventually anyway!!
	float lon, lat, elev;
	float x, y, z;
	//int freq;
	unsigned short int freq;
	//int range;
	unsigned short int range;
	string ident;
	string name;
};

ostream& operator << (ostream& os, atc_type atc);

class FGATC {

public:

    virtual ~FGATC();

    // Run the internal calculations
    virtual void Update(double dt);

    // Add plane to a stack
    virtual void AddPlane(string pid);

    // Remove plane from stack
    virtual int RemovePlane();

    // Indicate that this instance should output to the display if appropriate 
    virtual void SetDisplay();

    // Indicate that this instance should not output to the display
    virtual void SetNoDisplay();

    // Return the type of ATC station that the class represents
    virtual atc_type GetType();
	
	// Set the core ATC data
	void SetData(ATCData* d);
	
	inline double get_lon() const { return lon; }
	inline void set_lon(const double ln) {lon = ln;}
	inline double get_lat() const { return lat; }
	inline void set_lat(const double lt) {lat = lt;}
	inline double get_elev() const { return elev; }
	inline void set_elev(const double ev) {elev = ev;}
	inline double get_x() const { return x; }
	inline void set_x(const double ecs) {x = ecs;}
	inline double get_y() const { return y; }
	inline void set_y(const double why) {y = why;}
	inline double get_z() const { return z; }
	inline void set_z(const double zed) {z = zed;}
	inline int get_freq() const { return freq; }
	inline void set_freq(const int fq) {freq = fq;}
	inline int get_range() const { return range; }
	inline void set_range(const int rg) {range = rg;}
	inline const char* get_ident() { return ident.c_str(); }
	inline void set_ident(const string id) {ident = id;}
	inline const char* get_name() {return name.c_str();}
	inline void set_name(const string nm) {name = nm;}
	
protected:

	// Render a transmission
	// Outputs the transmission either on screen or as audio depending on user preference
	// The refname is a string to identify this sample to the sound manager
	// The repeating flag indicates whether the message should be repeated continuously or played once.
	void Render(string msg, string refname, bool repeating);

	// Cease rendering a transmission.
	// Requires the sound manager refname if audio, else "".
	void NoRender(string refname);

	double lon, lat, elev;
	double x, y, z;
	int freq;
	int range;
	string ident;		// Code of the airport its at.
	string name;		// Name transmitted in the broadcast.
	
	// Rendering related stuff
	bool voice;			// Flag - true if we are using voice
	bool playing;		// Indicates a message in progress	
	bool voiceOK;		// Flag - true if at least one voice has loaded OK
	FGATCVoice* vPtr;
};

inline istream&
operator >> ( istream& fin, ATCData& a )
{
	double f;
	char ch;
	char tp;

	fin >> tp;
	
	switch(tp) {
	case 'I':
		a.type = ATIS;
		break;
	case 'T':
		a.type = TOWER;
		break;
	case 'G':
		a.type = GROUND;
		break;
	case 'A':
		a.type = APPROACH;
		break;
	case '[':
		a.type = INVALID;
		return fin >> skipeol;
	default:
		SG_LOG(SG_GENERAL, SG_ALERT, "Warning - unknown type \'" << tp << "\' found whilst reading ATC frequency data!\n");
		a.type = INVALID;
		return fin >> skipeol;
	}

	fin >> a.lat >> a.lon >> a.elev >> f >> a.range 
	>> a.ident;
	
	a.name = "";
	fin >> ch;
	a.name += ch;
	while(1) {
		//in >> noskipws
		fin.unsetf(ios::skipws);
		fin >> ch;
		a.name += ch;
		if((ch == '"') || (ch == 0x0A)) {
			break;
		}   // we shouldn't need the 0x0A but it makes a nice safely in case someone leaves off the "
	}
	fin.setf(ios::skipws);
	//cout << "Comm name = " << a.name << '\n';
	
	a.freq = (int)(f*100.0 + 0.5);
	
	// cout << a.ident << endl;
	
	// generate cartesian coordinates
	Point3D geod( a.lon * SGD_DEGREES_TO_RADIANS, a.lat * SGD_DEGREES_TO_RADIANS, a.elev );
	Point3D cart = sgGeodToCart( geod );
	a.x = cart.x();
	a.y = cart.y();
	a.z = cart.z();
	
	return fin >> skipeol;
}


#endif  // _FG_ATC_HXX
