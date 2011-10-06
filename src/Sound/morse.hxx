// morse.hxx -- Morse code generation class
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


#ifndef _MORSE_HXX
#define _MORSE_HXX

#include "soundgenerator.hxx"
#include <simgear/compiler.h>
#include <simgear/sound/soundmgr_openal.hxx>


// Quoting from http://www.kluft.com/~ikluft/ham/morse-intro.html by
// Ian Kluft KO6YQ <ikluft@kluft.com>
//
// [begin quote]
//
// What is the Standard for Measuring Morse Code Speed?
// 
// [This was adapted from the Ham Radio FAQ which used to be posted on UseNet.] 
// 
// The word PARIS was chosen as the standard length for CW code
// speed. Each dit counts for one count, each dah counts for three
// counts, intra-character spacing is one count, inter-character
// spacing is three counts and inter-word spacing is seven counts, so
// the word PARIS is exactly 50 counts:
// 
// PPPPPPPPPPPPPP    AAAAAA    RRRRRRRRRR    IIIIII    SSSSSSSSSS
// di  da  da  di    di  da    di  da  di    di  di    di  di  di
// 1 1 3 1 3 1 1  3  1 1 3  3  1 1 3 1 1  3  1 1 1  3  1 1 1 1 1  7 = 50
//   ^                      ^                                     ^
//   ^Intra-character       ^Inter-character            Inter-word^
// 
// So 5 words-per-minute = 250 counts-per-minute / 50 counts-per-word
// or one count every 240 milliseconds. 13 words-per-minute is one
// count every ~92.3 milliseconds. This method of sending code is
// sometimes called "Slow Code", because at 5 wpm it sounds VERY SLOW.
// 
// The "Farnsworth" method is accomplished by sending the dits and
// dahs and intra-character spacing at a higher speed, then increasing
// the inter-character and inter-word spacing to slow the sending
// speed down to the desired speed. For example, to send at 5 wpm with
// 13 wpm characters in Farnsworth method, the dits and
// intra-character spacing would be 92.3 milliseconds, the dah would
// be 276.9 milliseconds, the inter-character spacing would be 1.443
// seconds and inter-word spacing would be 3.367 seconds.
//
// [end quote]

// Ok, back to Curt 

// My formulation is based dit = 1 count, dah = 3 counts, 1 count for
// intRA-character space, 3 counts for intER-character space.  Target
// is 5 wpm which by the above means 1 count = 240 milliseconds.
// 
// AIM 1-1-7 (f) states that the frequency of the tone should be 1020
// Hz for the VOR ident.


// manages everything we need to know for an individual sound sample
class FGMorse : public FGSoundGenerator {

private:

    unsigned char hi_dit[ DIT_SIZE ] ;
    unsigned char lo_dit[ DIT_SIZE ] ;
    unsigned char hi_dah[ DAH_SIZE ] ;
    unsigned char lo_dah[ DAH_SIZE ] ;
    unsigned char space[ SPACE_SIZE ] ;

    unsigned char cust_dit[ DIT_SIZE ] ;
    unsigned char cust_dah[ DAH_SIZE ] ;

    static FGMorse * _instance;

    bool cust_init( const int freq );
    // allocate and initialize sound samples
    bool init();

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


    FGMorse();
    ~FGMorse();

    static FGMorse * instance();

    // make a SimpleSound morse code transmission for the specified string
    SGSoundSample *make_ident( const string& id,
                               const int freq = LO_FREQUENCY );
};



#endif // _MORSE_HXX


