// FGATCVoice.hxx - a class to encapsulate an ATC voice
//
// Written by David Luff, started November 2002.
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

#ifndef _FG_ATC_VOICE
#define _FG_ATC_VOICE

#include <simgear/compiler.h>

#if defined( SG_HAVE_STD_INCLUDES ) || defined( __BORLANDC__ ) || (__APPLE__)
#  include <fstream>
#  include <iostream>
#else
#  include <fstream.h>
#  include <iostream.h>
#endif

#include <map>
#include <list>
#include <string>

#include <simgear/sound/sample_openal.hxx>

SG_USING_STD(map);
SG_USING_STD(list);
SG_USING_STD(string);

SG_USING_STD(cout);
SG_USING_STD(ios);
SG_USING_STD(ofstream);
SG_USING_STD(ifstream);


struct WordData {
	unsigned int offset;	// Offset of beginning of word sample into raw sound sample
	unsigned int length;	// Byte length of word sample
};

typedef map < string, WordData > atc_word_map_type;
typedef atc_word_map_type::iterator atc_word_map_iterator;
typedef atc_word_map_type::const_iterator atc_word_map_const_iterator;

class FGATCVoice {

public:

	FGATCVoice();
	~FGATCVoice();

	// Load the two voice files - one containing the raw sound data (.wav) and one containing the word positions (.vce).
	// Return true if successful.	
	bool LoadVoice(const string& voice);
	
	// Given a desired message, return a pointer to the data buffer and write the buffer length into len.
	// Sets dataOK = true if the returned buffer is valid.
	unsigned char* WriteMessage(char* message, int& len, bool& dataOK);


private:

	// the sound and word position data
	char* rawSoundData;
	unsigned int rawDataSize;
        SGSoundSample *SoundData;

	// A map of words vs. byte position and length in rawSoundData
	atc_word_map_type wordMap;

};

#endif	// _FG_ATC_VOICE
