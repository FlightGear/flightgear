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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#ifndef _FG_ATC_HXX
#define _FG_ATC_HXX

#include <simgear/constants.h>
#include <simgear/compiler.h>
#include <simgear/misc/sgstream.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/debug/logstream.hxx>

#include <istream>
#include <ostream>

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
	std::string callsign;
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

const int ATC_NUM_TYPES = 7;

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
	std::string ident;
	std::string name;
};

// perhaps we could use an FGRunway instead of this.
// That wouldn't cache the orthopos though.
struct RunwayDetails {
	Point3D threshold_pos;
	Point3D end1ortho;	// ortho projection end1 (the threshold ATM)
	Point3D end2ortho;	// ortho projection end2 (the take off end in the current hardwired scheme)
	double hdg;		// true runway heading
	double length;	// In *METERS*
	double width;	// ditto
	std::string rwyID;
	int patternDirection;	// -1 for left, 1 for right
};

std::ostream& operator << (std::ostream& os, atc_type atc);

class FGATC {
	
public:
	
	FGATC();
	virtual ~FGATC();
	
	// Run the internal calculations
	// Derived classes should call this method from their own Update methods if they 
	// wish to use the response timer functionality.
	virtual void Update(double dt);
	
	// Recieve a coded callback from the ATC menu system based on the user's selection
	virtual void ReceiveUserCallback(int code);
	
	// Add plane to a stack
	virtual void AddPlane(const std::string& pid);
	
	// Remove plane from stack
	virtual int RemovePlane();
	
	// Indicate that this instance should output to the display if appropriate 
	inline void SetDisplay() { _display = true; }
	
	// Indicate that this instance should not output to the display
	inline void SetNoDisplay() { _display = false; }
	
	// Generate the text of a message from its parameters and the current context.
	virtual std::string GenText(const std::string& m, int c);
	
	// Returns true if OK to transmit on this frequency
	inline bool GetFreqClear() { return freqClear; }
	// Indicate that the frequency is in use
	inline void SetFreqInUse() { freqClear = false; receiving = true; }
	// Transmission to the ATC is finished and a response is required
	void SetResponseReqd(const std::string& rid);
	// Transmission finished - let ATC decide if a response is reqd and clear freq if necessary
	void NotifyTransmissionFinished(const std::string& rid);
	// Transmission finished and no response required
	inline void ReleaseFreq() { freqClear = true; receiving = false; }	// TODO - check that the plane releasing the freq is the right one etc.
	// The above 3 funcs under development!!
	// The idea is that AI traffic or the user ATC dialog box calls FreqInUse() when they begin transmitting,
	// and that the tower control sets freqClear back to true following a reply.
	// AI traffic should check FreqClear() is true prior to transmitting.
	// The user will just have to wait for a gap in dialog as in real life.
	
	// Return the type of ATC station that the class represents
	inline atc_type GetType() { return _type; }
	
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
	inline const std::string& get_ident() { return ident; }
	inline void set_ident(const std::string& id) { ident = id; }
	inline const std::string& get_name() { return name; }
	inline void set_name(const std::string& nm) { name = nm; }
	
protected:
	
	// Render a transmission
	// Outputs the transmission either on screen or as audio depending on user preference
	// The refname is a string to identify this sample to the sound manager
	// The repeating flag indicates whether the message should be repeated continuously or played once.
	void Render(std::string& msg, const std::string& refname = "", bool repeating = false);
	
	// Cease rendering all transmission from this station.
	// Requires the sound manager refname if audio, else "".
	void NoRender(const std::string& refname);
	
	// Transmit a message when channel becomes free of other dialog
    void Transmit(int callback_code = 0);
	
	// Transmit a message if channel becomes free within timeout (seconds). timeout of zero implies no limit
	void ConditionalTransmit(double timeout, int callback_code = 0);
	
	// Transmit regardless of other dialog on the channel eg emergency
	void ImmediateTransmit(int callback_code = 0);
	
	virtual void ProcessCallback(int code);
	
	double lon, lat, elev;
	double x, y, z;
	int freq;
	int range;
	std::string ident;		// Code of the airport its at.
	std::string name;		// Name transmitted in the broadcast.
	atc_type _type;
	
	// Rendering related stuff
	bool _voice;			// Flag - true if we are using voice
	bool _playing;		// Indicates a message in progress	
	bool _voiceOK;		// Flag - true if at least one voice has loaded OK
	FGATCVoice* _vPtr;

	std::string pending_transmission;	// derived classes set this string before calling Transmit(...)	
	bool freqClear;		// Flag to indicate if the frequency is clear of ongoing dialog
	bool receiving;		// Flag to indicate we are receiving a transmission
	bool responseReqd;	// Flag to indicate we should be responding to a request/report 
	bool runResponseCounter;	// Flag to indicate the response counter should be run
	double responseTime;	// Time to take from end of request transmission to beginning of response
							// The idea is that this will be slightly random.
	double responseCounter;		// counter to implement the above
	std::string responseID;	// ID of the plane to respond to
	bool respond;	// Flag to indicate now is the time to respond - ie set following the count down of the response timer.
	// Derived classes only need monitor this flag, and use the response ID, as long as they call FGATC::Update(...)
	bool _runReleaseCounter;	// A timer for releasing the frequency after giving the message enough time to display
	double _releaseTime;
	double _releaseCounter;
	
	bool _display;		// Flag to indicate whether we should be outputting to the ATC display.
	bool _displaying;		// Flag to indicate whether we are outputting to the ATC display.
	
private:
	// Transmission timing stuff.
	bool _pending;
	double _timeout;
	int _callback_code;	// A callback code to be notified and processed by the derived classes
						// A value of zero indicates no callback required
	bool _transmit;		// we are to transmit
	bool _transmitting;	// we are transmitting
	double _counter;
	double _max_count;
};

inline std::istream&
operator >> ( std::istream& fin, ATCData& a )
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
	if(ch != '"') a.name += ch;
	while(1) {
		//in >> noskipws
		fin.unsetf(std::ios::skipws);
		fin >> ch;
		if((ch == '"') || (ch == 0x0A)) {
			break;
		}   // we shouldn't need the 0x0A but it makes a nice safely in case someone leaves off the "
		a.name += ch;
	}
	fin.setf(std::ios::skipws);
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
