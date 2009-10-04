// FGAIPlane - abstract base class for an AI plane
//
// Written by David Luff, started 2002.
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

#ifndef _FG_AI_PLANE_HXX
#define _FG_AI_PLANE_HXX

#include "AIEntity.hxx"
#include "ATC.hxx"

class SGSampleGroup;

enum PatternLeg {
	TAKEOFF_ROLL,
	CLIMBOUT,
	TURN1,
	CROSSWIND,
	TURN2,
	DOWNWIND,
	TURN3,
	BASE,
	TURN4,
	FINAL,
	LANDING_ROLL,
	LEG_UNKNOWN
};

ostream& operator << (ostream& os, PatternLeg pl);

enum LandingType {
	FULL_STOP,
	STOP_AND_GO,
	TOUCH_AND_GO,
	AIP_LT_UNKNOWN
};

ostream& operator << (ostream& os, LandingType lt);

/*****************************************************************
*
*  FGAIPlane - this class is derived from FGAIEntity and adds the 
*  practical requirement for an AI plane - the ability to send radio
*  communication, and simple performance details for the actual AI
*  implementation to use.  The AI implementation is expected to be
*  in derived classes - this class does nothing useful on its own.
*
******************************************************************/
class FGAIPlane : public FGAIEntity {

public:

	FGAIPlane();
    virtual ~FGAIPlane();

    // Run the internal calculations
	void Update(double dt);
	
	// Send a transmission *TO* the AIPlane.
	// FIXME int code is a hack - eventually this will receive Alexander's coded messages.
	virtual void RegisterTransmission(int code);
	
	// Return the current pattern leg the plane is flying.
	inline PatternLeg GetLeg() {return leg;}
	
	// Return what type of landing we're doing on this circuit
	virtual LandingType GetLandingOption();
	
	// Return the callsign
	inline const string& GetCallsign() {return plane.callsign;}

protected:
	PlaneRec plane;

    double mag_hdg;	// degrees - the heading that the physical aircraft is *pointing*
    double track;	// track that the physical aircraft is *following* - degrees relative to *true* north
    double crab;	// Difference between heading and track due to wind.
    double mag_var;	// degrees
    double IAS;		// Indicated airspeed in knots
    double vel;		// velocity along track in knots
    double vel_si;	// velocity along track in m/s
    double slope;	// Actual slope that the plane is flying (degrees) - +ve is uphill
    double AoA;		// degrees - difference between slope and pitch
    // We'll assume that the plane doesn't yaw or slip - the track/heading difference is to allow for wind

    double freq;	// The comm frequency that we're operating on

    // We need some way for this class to display its radio transmissions if on the 
    // same frequency and in the vicinity of the user's aircraft
    // This may need to be done independently of ATC eg CTAF
    // Make radio transmission - this simply sends the transmission for physical rendering if the users
    // aircraft is on the same frequency and in range.  It is up to the derived classes to let ATC know
    // what is going on.
	string pending_transmission;	// derived classes set this string before calling Transmit(...)
	FGATC* tuned_station;			// and this if they are tuned to ATC
	
	// Transmit a message when channel becomes free of other dialog
    void Transmit(int callback_code = 0);
	
	// Transmit a message if channel becomes free within timeout (seconds). timeout of zero implies no limit
	void ConditionalTransmit(double timeout, int callback_code = 0);
	
	// Transmit regardless of other dialog on the channel eg emergency
	void ImmediateTransmit(int callback_code = 0);
	
	inline void SetTrack(double t) { _tgtTrack = t; _trackSet = true; }
	inline void ClearTrack() { _trackSet = false; }

    inline void Bank(double r) { _tgtRoll = r; }
    inline void LevelWings(void) { _tgtRoll = 0.0; }
	
	virtual void ProcessCallback(int code);
	
	PatternLeg leg;
	
private:
	bool _pending;
	double _timeout;
	int _callback_code;	// A callback code to be notified and processed by the derived classes
						// A value of zero indicates no callback required
	bool _transmit;		// we are to transmit
	bool _transmitting;	// we are transmitting
	double _counter;
	double _max_count;
	
	// Render a transmission (in string pending_transmission)
	// Outputs the transmission either on screen or as audio depending on user preference
	// The refname is a string to identify this sample to the sound manager
	// The repeating flag indicates whether the message should be repeated continuously or played once.
        void Render(const string& refname, const float volume, bool repeating);

	// Cease rendering a transmission.
	// Requires the sound manager refname if audio, else "".
	void NoRender(const string& refname);
	
	// Rendering related stuff
	bool voice;			// Flag - true if we are using voice
	bool playing;		// Indicates a message in progress	
	bool voiceOK;		// Flag - true if at least one voice has loaded OK
	FGATCVoice* vPtr;
	
	// Navigation
	double _tgtTrack;	// Track to be following if _trackSet is true
	bool _trackSet;		// Set true if tgtTrack is to be followed
	double _tgtRoll;
	bool _rollSuspended;	// Set true when a derived class has suspended AIPlane's roll control

        SGSampleGroup *_sgr;
};

#endif  // _FG_AI_PLANE_HXX

