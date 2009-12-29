// FGATCVoice.cxx - a class to encapsulate an ATC voice
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "ATCVoice.hxx"

#include <stdlib.h>
#include <ctype.h>
#include <fstream>
#include <vector>
#include <algorithm>

#include <simgear/sound/soundmgr_openal.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sgstream.hxx>
#include <simgear/math/sg_random.h>

#include <Main/globals.hxx>

using namespace std;

FGATCVoice::FGATCVoice() {
  SoundData = 0;
  rawSoundData = 0;
}

FGATCVoice::~FGATCVoice() {
    if (rawSoundData)
	free( rawSoundData );
    delete SoundData;
}

// Load the two voice files - one containing the raw sound data (.wav) and one containing the word positions (.vce).
// Return true if successful.
bool FGATCVoice::LoadVoice(const string& voice) {
	std::ifstream fin;

	SGPath path = globals->get_fg_root();
	string file = voice + ".wav";
	path.append( "ATC" );
	path.append( file );
	
	string full_path = path.str();
	int format, freq;
	SGSoundMgr *smgr = globals->get_soundmgr();
	void *data;
	if (!smgr->load(full_path, &data, &format, &rawDataSize, &freq))
	    return false;
	rawSoundData = (char*)data;
#ifdef VOICE_TEST
	cout << "ATCVoice:  format: " << format
			<< "  size: " << rawDataSize << endl;
#endif	
	path = globals->get_fg_root();
	string wordPath = "ATC/" + voice + ".vce";
	path.append(wordPath);
	
	// Now load the word data
	fin.open(path.c_str(), ios::in);
	if(!fin) {
		SG_LOG(SG_ATC, SG_ALERT, "Unable to open input file " << path.c_str());
		return(false);
	}
	SG_LOG(SG_ATC, SG_INFO, "Opened word data file " << wordPath << " OK...");
	char numwds[10];
	char wrd[100];
	string wrdstr;
	char wrdOffsetStr[20];
	char wrdLengthStr[20];
	unsigned int wrdOffset;		// Offset into the raw sound data that the word sample begins
	unsigned int wrdLength;		// Length of the word sample in bytes
	WordData wd;
	fin >> numwds;
	unsigned int numwords = atoi(numwds);
	//cout << numwords << '\n';
	for(unsigned int i=0; i < numwords; ++i) {
		fin >> wrd;
		wrdstr = wrd;
		fin >> wrdOffsetStr;
		fin >> wrdLengthStr;
		wrdOffset = atoi(wrdOffsetStr);
		wrdLength = atoi(wrdLengthStr);
		wd.offset = wrdOffset;
		wd.length = wrdLength;
		wordMap[wrdstr] = wd;
	        string ws2 = wrdstr;
	        for(string::iterator p = ws2.begin(); p != ws2.end(); p++){
	          *p = tolower(*p);
	          if (*p == '-') *p = '_';
	        }
	        if (wrdstr != ws2)  wordMap[ws2] = wd;

		//cout << wrd << "\t\t" << wrdOffset << "\t\t" << wrdLength << '\n';
		//cout << i << '\n';
	}
	
	fin.close();
	return(true);
}


// Given a desired message, return a string containing the
// sound-sample data
void* FGATCVoice::WriteMessage(const string& message, size_t* len) {
	
	// What should we do here?
	// First - parse the message into a list of tokens.
	// Sort the tokens into those we understand and those we don't.
	// Add all the raw lengths of the token sound data, allocate enough space, and fill it with the rqd data.

	vector<char> sound;
	const char delimiters[] = " \t.,;:\"\n";
	string::size_type token_start = message.find_first_not_of(delimiters);
	while(token_start != string::npos) {
		string::size_type token_end = message.find_first_of(delimiters, token_start);
		string token;
		if (token_end == string::npos) {
			token = message.substr(token_start);
			token_start = string::npos;
		} else {
			token = message.substr(token_start, token_end - token_start);
			token_start = message.find_first_not_of(delimiters, token_end);
		}

                if (token == "/_") continue;

		for(string::iterator t = token.begin(); t != token.end(); t++) {
			// canonicalize the token, to match what's in the index
			*t = (*t == '-') ? '_' : tolower(*t);
		}
		SG_LOG(SG_ATC, SG_DEBUG, "voice synth: token: '"
                    << token << "'");

		atc_word_map_const_iterator wordIt = wordMap.find(token);
		if(wordIt == wordMap.end()) {
			// Oh dear - the token isn't in the sound file
			SG_LOG(SG_ATC, SG_ALERT, "voice synth: word '"
				<< token << "' not found");
		} else {
			const WordData& word = wordIt->second;
			/*
			*  Sanity check for corrupt/mismatched sound data input - avoids a seg fault
			*  (As long as the calling function checks the return value!!)
			*  This check should be left in even when the default Flightgear files are known
			*  to be OK since it checks for mis-indexing of voice files by 3rd party developers.
			*/
			if((word.offset + word.length) > rawDataSize) {
				SG_LOG(SG_ATC, SG_ALERT, "ERROR - mismatch between ATC .wav and .vce file in ATCVoice.cxx\n");
				SG_LOG(SG_ATC, SG_ALERT, "Offset + length: " << word.offset + word.length
					<< " exceeds rawdata size: " << rawDataSize << endl);

				*len = 0;
				return 0;
			}
			sound.insert(sound.end(), rawSoundData + word.offset, rawSoundData + word.offset + word.length);
		}
	}

	// Check for no tokens found else slScheduler can be crashed
	*len = sound.size();
	if (*len == 0) {
		return 0;
	}

	char* data = (char*)malloc(*len);
	if (data == 0) {
		SG_LOG(SG_ATC, SG_ALERT, "ERROR - could not allocate " << *len << " bytes of memory for ATIS sound\n");
		*len = 0;
		return 0;
	}

	// randomize start position
	unsigned int offsetIn = (unsigned int)(*len * sg_random());
	if (offsetIn > 0 && offsetIn < *len) {
		copy(sound.begin() + offsetIn, sound.end(), data);
		copy(sound.begin(), sound.begin() + offsetIn, data + *len - offsetIn);
	} else {
		copy(sound.begin(), sound.end(), data);
	}

	return data;
}
