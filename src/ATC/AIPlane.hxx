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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#ifndef _FG_AI_PLANE_HXX
#define _FG_AI_PLANE_HXX

#include <plib/sg.h>
#include <plib/ssg.h>
#include <simgear/math/point3d.hxx>
#include <simgear/scene/model/model.hxx>

#include "AIEntity.hxx"
#include "ATC.hxx"

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
    virtual void Update(double dt);
	
	// Send a transmission *TO* the AIPlane.
	// FIXME int code is a hack - eventually this will receive Alexander's coded messages.
	virtual void RegisterTransmission(int code);
	
	// Return the current pattern leg the plane is flying.
	inline PatternLeg GetLeg() {return leg;}

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
    void Transmit(string msg);

    void Bank(double angle);
    void LevelWings(void);
	
	PatternLeg leg;
};

#endif  // _FG_AI_PLANE_HXX

