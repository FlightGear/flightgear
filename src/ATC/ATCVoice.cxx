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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/misc/sg_path.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sgstream.hxx>
#include <simgear/math/sg_random.h>
#include <Main/globals.hxx>

#include "ATCVoice.hxx"

#include <stdlib.h>

FGATCVoice::FGATCVoice() {
}

FGATCVoice::~FGATCVoice() {
    // delete SoundData;
}

// Load the two voice files - one containing the raw sound data (.wav) and one containing the word positions (.vce).
// Return true if successful.
bool FGATCVoice::LoadVoice(string voice) {
    // FIXME CLO: disabled to try to see if this is causign problemcs
    // return false;

	ifstream fin;

	SGPath path = globals->get_fg_root();
	path.append( "ATC" );

        string file = voice + ".wav";
	
	SoundData = new SGSoundSample( path.c_str(), file.c_str() );
	rawDataSize = SoundData->get_size();
	rawSoundData = SoundData->get_data();
	
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
		//cout << wrd << "\t\t" << wrdOffset << "\t\t" << wrdLength << '\n';
		//cout << i << '\n';
	}
	
	fin.close();
	return(true);
}


typedef list < string > tokenList_type;
typedef tokenList_type::iterator tokenList_iterator;

// Given a desired message, return a pointer to the data buffer and write the buffer length into len.
unsigned char* FGATCVoice::WriteMessage(char* message, int& len, bool& dataOK) {
	
	// What should we do here?
	// First - parse the message into a list of tokens.
	// Sort the tokens into those we understand and those we don't.
	// Add all the raw lengths of the token sound data, allocate enough space, and fill it with the rqd data.
	tokenList_type tokenList;
	tokenList_iterator tokenListItr;

	// TODO - at the moment we're effectively taking 3 passes through the data.
	// There is no need for this - 2 should be sufficient - we can probably ditch the tokenList.
	char* token;
	char mes[1000];
	int numWords = 0;
	strcpy(mes, message);
	const char delimiters[] = " \t.,;:\"";
	token = strtok(mes, delimiters);
	while(token != NULL) {
		tokenList.push_back(token);
		++numWords;
		//cout << "token = " << token << '\n';
		token = strtok(NULL, delimiters);
	}

	WordData* wdptr = new WordData[numWords];
	int word = 0;
	unsigned int cumLength = 0;

	tokenListItr = tokenList.begin();
	while(tokenListItr != tokenList.end()) {
		if(wordMap.find(*tokenListItr) == wordMap.end()) {
			// Oh dear - the token isn't in the sound file
			//cout << "word " << *tokenListItr << " not found :-(\n";
		} else {
			wdptr[word] = wordMap[*tokenListItr];
			cumLength += wdptr[word].length;
			//cout << *tokenListItr << " found at offset " << wdptr[word].offset << " with length " << wdptr[word].length << endl;  
			word++;
		}
		++tokenListItr;
	}

	// Check for no tokens found else slScheduler can be crashed
	if(!word) {
		dataOK = false;
		return(NULL);
	}

	unsigned char* tmpbuf = new unsigned char[cumLength];	
	unsigned char* outbuf = new unsigned char[cumLength];
	len = cumLength;
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
			delete[] wdptr;
			dataOK = false;
			return(NULL);
		}
		memcpy(tmpbuf + bufpos, rawSoundData + wdptr[i].offset, wdptr[i].length);
		bufpos += wdptr[i].length;
	}
	
	// tmpbuf now contains the message starting at the beginning - but we want it to start at a random position.
	unsigned int offsetIn = (int)(cumLength * sg_random());
	if(offsetIn > cumLength) offsetIn = cumLength;
	memcpy(outbuf, tmpbuf + offsetIn, (cumLength - offsetIn));
	memcpy(outbuf + (cumLength - offsetIn), tmpbuf, offsetIn);
	
	delete[] tmpbuf;
	delete[] wdptr;

	dataOK = true;	
	return(outbuf);
}
