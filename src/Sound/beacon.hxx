/**
 * \file beacon.hxx -- Provides marker beacon audio generation.
 */

// Written by Curtis Olson, started March 2001.
//
// Copyright (C) 2001  Curtis L. Olson - http://www.flightgear.org/~curt
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

#ifndef _BEACON_HXX
#define _BEACON_HXX

#include "soundgenerator.hxx"

#include <simgear/compiler.h>
#include <simgear/sound/soundmgr_openal.hxx>
#include <simgear/structure/SGReferenced.hxx>
#include <simgear/structure/SGSharedPtr.hxx>

// Quoting from http://www.smartregs.com/data/sa326.htm
// Smart REGS Glossary - marker beacon
// 
// An electronic navigation facility transmitting a 75 MHz vertical fan
// or boneshaped radiation pattern.  Marker beacons are identified by
// their modulation frequency and keying code, and when received by
// compatible airborne equipment, indicate to the pilot, both aurally
// and visually, that he is passing over the facility.
// (See outer marker middle marker inner marker.)
// 
// Smart REGS Glossary - outer marker
// 
// A marker beacon at or near the glideslope intercept altitude of an
// ILS approach.  It is keyed to transmit two dashes per second on a
// 400 Hz tone, which is received aurally and visually by compatible
// airborne equipment.  The OM is normally located four to seven miles from
// the runway threshold on the extended centerline of the runway.
// 
// Smart REGS Glossary - middle marker
// 
// A marker beacon that defines a point along the glideslope of an
// ILS normally located at or near the point of decision height
// (ILS Category I).  It is keyed to transmit alternate dots and dashes,
// with the alternate dots and dashes keyed at the rate of 95 dot/dash
// combinations per minute on a 1300 Hz tone, which is received
// aurally and visually by compatible airborne equipment.
// 
// Smart REGS Glossary - inner marker
// 
// A marker beacon used with an ILS (CAT II) precision approach located
// between the middle marker and the end of the ILS runway,
// transmitting a radiation pattern keyed at six dots per second and
// indicating to the pilot, both aurally and visually, that he is at
// the designated decision height (DH), normally 100 feet above the
// touchdown zone elevation, on the ILS CAT II approach.  It also marks
// progress during a CAT III approach.
// (See instrument landing system) (Refer to AIM.)


// manages everything we need to know for an individual sound sample
class FGBeacon : public FGSoundGenerator {

private:
    static const int INNER_FREQ = 3000;
    static const int MIDDLE_FREQ = 1300;
    static const int OUTER_FREQ = 400;

    static const int INNER_SIZE = BYTES_PER_SECOND;
    static const int MIDDLE_SIZE = (int)(BYTES_PER_SECOND * 60 / 95 );
    static const int OUTER_SIZE = BYTES_PER_SECOND;

    static const int INNER_DIT_LEN = (int)(BYTES_PER_SECOND / 6.0);
    static const int MIDDLE_DIT_LEN = (int)(MIDDLE_SIZE / 3.0);
    static const int MIDDLE_DAH_LEN = (int)(MIDDLE_SIZE * 2 / 3.0);
    static const int OUTER_DAH_LEN = (int)(BYTES_PER_SECOND / 2.0);

    SGSharedPtr<SGSoundSample> inner;
    SGSharedPtr<SGSoundSample> middle;
    SGSharedPtr<SGSoundSample> outer;

    // allocate and initialize sound samples
    bool init();
    static FGBeacon * _instance;
public:

    FGBeacon();
    ~FGBeacon();
    static FGBeacon * instance();

    SGSoundSample *get_inner() { return inner; }
    SGSoundSample *get_middle() { return middle; }
    SGSoundSample *get_outer() { return outer; }
   
};



#endif // _BEACON_HXX


