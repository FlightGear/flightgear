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
#include <Main/globals.hxx>

#include "ATCVoice.hxx"

#include <stdlib.h>

FGATCVoice::FGATCVoice() {
}

FGATCVoice::~FGATCVoice() {
	delete[] rawSoundData;
}

// Load the two voice files - one containing the raw sound data (.wav) and one containing the word positions (.vce).
// Return true if successful.
bool FGATCVoice::LoadVoice(string voice) {
	ifstream fin;

	SGPath path = globals->get_fg_root();
	string soundPath = "ATC/" + voice + ".wav";
	path.append(soundPath);
	
	// Input data parameters - some of these might need to be class variables eventually
	// but at the moment we're just using them to find the header size to get the start
	// of the data properly.
	char chunkID[5];
	unsigned int chunkSize;
	char junk[100];		// WARNING - Assumes all non-data chunk sizes are < 100
	
	// do the sound data first
	SG_LOG(SG_GENERAL, SG_INFO, "Trying to open voice input...");
	fin.open(path.c_str(), ios::in|ios::binary);
	if(!fin) {
		SG_LOG(SG_GENERAL, SG_ALERT, "Unable to open input file " << path.c_str());
		return(false);
	}
	cout << "Opened voice input file " << soundPath << " OK...\n";
	// Strip the initial headers and ignore.
	// Note that this assumes we know the sound format etc - fix this eventually
	// (I've assumed sample-rate = 8000, bits = 8, mono, which is what the other FGFS sound samples seem to use.
	// The file should always start with the 12 byte RIFF header
	fin.read(chunkID, 4);
	// TODO - Should we check that the above == "RIFF" ?
	// read and discard the next 8 bytes
	fin.read(junk, 8);
	// Now it gets more complicated - although the format chunk is normally before the data chunk,
	// this is not guaranteed, and there may be a fact chunk as well. (And possibly more that I haven't heard of!).
	while(1) {
		fin.read(chunkID, 4);
		chunkID[4] = '\0';
		//cout << "sizeof(chunkID) = " << sizeof(chunkID) << '\n';
		//cout << "chunkID = " << chunkID << '\n';
		if(!strcmp(chunkID, "data")) {
			break;
		} else if((!strcmp(chunkID, "fmt ")) || (!strcmp(chunkID, "fact"))) {
			fin.read((char*)&chunkSize, sizeof(chunkSize));
			// Chunksizes must be word-aligned (ie every 2 bytes), but the given chunk size
			// is not guaranteed to be word-aligned, and there may be an extra padding byte.
			// Add 1 to chunkSize if it's odd.
			// Well, it is a microsoft file format!!!
			chunkSize += (chunkSize % 2);
			fin.read(junk, chunkSize);
		} else {
			// Oh dear - its all gone pear-shaped - abort :-(
			SG_LOG(SG_GENERAL, SG_ALERT, "Unknown chunk ID in input wave file in ATCVoice.cxx... aborting voice ATC load");
			fin.close();
			return(false);
		}
	}

	fin.read((char*)&rawDataSize, sizeof(rawDataSize));
	//cout << "rawDataSize = " << rawDataSize << endl;
	rawSoundData = new char[rawDataSize];
	fin.read(rawSoundData, rawDataSize);
	fin.close();

	path = globals->get_fg_root();
	string wordPath = "ATC/" + voice + ".vce";
	path.append(wordPath);
	
	// Now load the word data
	fin.open(path.c_str(), ios::in);
	if(!fin) {
		SG_LOG(SG_GENERAL, SG_ALERT, "Unable to open input file " << path.c_str() << '\n');
		return(false);
	}
	cout << "Opened word data file " << wordPath << " OK...\n";
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


// Given a desired message, return a pointer to the data buffer and write the buffer length into len.
unsigned char* FGATCVoice::WriteMessage(char* message, int& len, bool& dataOK) {
	
	// What should we do here?
	// First - parse the message into a list of tokens.
	// Sort the tokens into those we understand and those we don't.
	// Add all the raw lengths of the token sound data, allocate enough space, and fill it with the rqd data.
	list < string > tokenList;
	list < string >::iterator tokenListItr;

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
			SG_LOG(SG_GENERAL, SG_ALERT, "ERROR - mismatch between ATC .wav and .vce file in ATCVoice.cxx\n");
			SG_LOG(SG_GENERAL, SG_ALERT, "Offset + length: " << wdptr[i].offset + wdptr[i].length
			     << " exceeds rawdata size: " << rawDataSize << endl);
			delete[] wdptr;
			dataOK = false;
			// I suppose we have to return something
			return(NULL);
		}
		memcpy(outbuf + bufpos, rawSoundData + wdptr[i].offset, wdptr[i].length);
		bufpos += wdptr[i].length;
	}
	
	delete[] wdptr;

	dataOK = true;	
	return(outbuf);
}
