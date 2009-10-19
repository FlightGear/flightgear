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
#include <list>
#include <vector>

#include <boost/shared_array.hpp>

#include <simgear/sound/soundmgr_openal.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sgstream.hxx>
#include <simgear/math/sg_random.h>

#include <Main/globals.hxx>

#include <stdio.h>

#ifdef _MSC_VER
#define strtok_r strtok_s
#endif

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
        SGSoundMgr *smgr = (SGSoundMgr *)globals->get_subsystem("soundmgr");
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
	        if (wrdstr != ws2) wordMap[ws2] = wd;

		//cout << wrd << "\t\t" << wrdOffset << "\t\t" << wrdLength << '\n';
		//cout << i << '\n';
	}
	
	fin.close();
	return(true);
}


typedef list < string > tokenList_type;
typedef tokenList_type::iterator tokenList_iterator;

// Given a desired message, return a string containing the
// sound-sample data
string FGATCVoice::WriteMessage(const char* message, bool& dataOK) {
	
	// What should we do here?
	// First - parse the message into a list of tokens.
	// Sort the tokens into those we understand and those we don't.
	// Add all the raw lengths of the token sound data, allocate enough space, and fill it with the rqd data.
	tokenList_type tokenList;
	tokenList_iterator tokenListItr;

	// TODO - at the moment we're effectively taking 3 passes through the data.
	// There is no need for this - 2 should be sufficient - we can probably ditch the tokenList.
	size_t n1 = 1+strlen(message);
	boost::shared_array<char> msg(new char[n1]);
	strncpy(msg.get(), message, n1); // strtok requires a non-const char*
	char* token;
	int numWords = 0;
	const char delimiters[] = " \t.,;:\"\n";
	char* context;
	token = strtok_r(msg.get(), delimiters, &context);
	while(token != NULL) {
	        for (char *t = token; *t; t++) {
	          *t = tolower(*t);     // canonicalize the case, to
	          if (*t == '-') *t = '_';   // match what's in the index
	        }
		tokenList.push_back(token);
		++numWords;
	        SG_LOG(SG_ATC, SG_DEBUG, "voice synth: token: '"
	            << token << "'");
		token = strtok_r(NULL, delimiters, &context);
	}

	vector<WordData> wdptr;
	wdptr.reserve(numWords);
	unsigned int cumLength = 0;

	tokenListItr = tokenList.begin();
	while(tokenListItr != tokenList.end()) {
		if(wordMap.find(*tokenListItr) == wordMap.end()) {
		// Oh dear - the token isn't in the sound file
	  	  SG_LOG(SG_ATC, SG_ALERT, "voice synth: word '"
	              << *tokenListItr << "' not found");
		} else {
	            wdptr.push_back(wordMap[*tokenListItr]);
	            cumLength += wdptr.back().length;
		}
		++tokenListItr;
	}
	const size_t word = wdptr.size();
	
	// Check for no tokens found else slScheduler can be crashed
	if(!word) {
		dataOK = false;
		return "";
	}
	boost::shared_array<char> tmpbuf(new char[cumLength]);
	unsigned int bufpos = 0;
	for(int i=0; i<word; ++i) {
		/*
		*  Sanity check for corrupt/mismatched sound data input - avoids a seg fault
		*  (As long as the calling function checks the return value!!)
		*  This check should be left in even when the default Flightgear files are known
		*  to be OK since it checks for mis-indexing of voice files by 3rd party developers.
		*/
		if((wdptr[i].offset + wdptr[i].length) > rawDataSize) {
			SG_LOG(SG_ATC, SG_ALERT, "ERROR - mismatch between ATC .wav and .vce file in ATCVoice.cxx\n");
			SG_LOG(SG_ATC, SG_ALERT, "Offset + length: " << wdptr[i].offset + wdptr[i].length
			     << " exceeds rawdata size: " << rawDataSize << endl);

			dataOK = false;
			return "";
		}
		memcpy(tmpbuf.get() + bufpos, rawSoundData + wdptr[i].offset, wdptr[i].length);
		bufpos += wdptr[i].length;
	}
	
	// tmpbuf now contains the message starting at the beginning - but we want it to start at a random position.
	unsigned int offsetIn = (int)(cumLength * sg_random());
	if(offsetIn > cumLength) offsetIn = cumLength;

	string front(tmpbuf.get(), offsetIn);
	string back(tmpbuf.get() + offsetIn, cumLength - offsetIn);

	dataOK = true;	
	return back + front;
}
