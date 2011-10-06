// soundgenerator.hxx -- simple sound generation 
//
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


#ifndef _FGSOUNDGENERATOR_HXX
#define _FGSOUNDGENERATOR_HXX

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

class FGSoundGenerator {

public:
    static const int BYTES_PER_SECOND = 22050;
    // static const int BEAT_LENGTH = 240; // milleseconds (5 wpm)
    static const int BEAT_LENGTH = 92;  // milleseconds (13 wpm)
    static const int TRANSITION_BYTES = (int)(0.005 * BYTES_PER_SECOND);
    static const int COUNT_SIZE = BYTES_PER_SECOND * BEAT_LENGTH / 1000;
    static const int DIT_SIZE = 2 * COUNT_SIZE;   // 2 counts
    static const int DAH_SIZE = 4 * COUNT_SIZE;   // 4 counts
    static const int SPACE_SIZE = 3 * COUNT_SIZE; // 3 counts
    static const int LO_FREQUENCY = 1020;	 // AIM 1-1-7 (f) specified in Hz
    static const int HI_FREQUENCY = 1350;	 // AIM 1-1-7 (f) specified in Hz

protected:
    /**
    * \relates FGMorse
    * Make a tone of specified freq and total_len with trans_len ramp in
    * and out and only the first len bytes with sound, the rest with
    * silence.
    * @param buf unsigned char pointer to sound buffer
    * @param freq desired frequency of tone
    * @param len length of tone within sound
    * @param total_len total length of sound (anything more than len is padded
    *        with silence.
    * @param trans_len length of ramp up and ramp down to avoid audio "pop"
    */
    static void make_tone( unsigned char *buf, int freq, 
        int len, int total_len, int trans_len );

public:

    virtual ~FGSoundGenerator();
};



#endif // _FGSOUNDGENERATOR_HXX
