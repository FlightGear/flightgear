// Implementation of FGATC - ATC subsystem base class.
//
// Written by David Luff, started February 2002.
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

#include <Main/fgfs.hxx>
#include <Main/fg_props.hxx>
#include <Sound/soundmgr.hxx>

#include "ATC.hxx"
#include "ATCdisplay.hxx"

FGATC::~FGATC() {
}

void FGATC::Update(double dt) {
}

void FGATC::AddPlane(string pid) {
}

int FGATC::RemovePlane() {
    return 0;
}

void FGATC::SetDisplay() {
}

void FGATC::SetNoDisplay() {
}

atc_type FGATC::GetType() {
    return INVALID;
}

void FGATC::SetData(ATCData* d) {
	lon = d->lon;
	lat = d->lat;
	elev = d->elev;
	x = d->x;
	y = d->y;
	z = d->z;
	range = d->range;
	ident = d->ident;
	name = d->name;
	freq = d->freq;
}

// Render a transmission
// Outputs the transmission either on screen or as audio depending on user preference
// The refname is a string to identify this sample to the sound manager
// The repeating flag indicates whether the message should be repeated continuously or played once.
void FGATC::Render(string msg, string refname, bool repeating) {
#ifdef ENABLE_AUDIO_SUPPORT
	voice = (voiceOK && fgGetBool("/sim/sound/audible")
                 && fgGetBool("/sim/sound/voice"));
	if(voice) {
		int len;
		unsigned char* buf = vPtr->WriteMessage((char*)msg.c_str(), len, voice);
		if(voice) {
			FGSimpleSound* simple = new FGSimpleSound(buf, len);
			// TODO - at the moment the volume is always set off comm1 
			// and can't be changed after the transmission has started.
			simple->set_volume(5.0 * fgGetDouble("/radios/comm[0]/volume"));
			globals->get_soundmgr()->add(simple, refname);
			if(repeating) {
				globals->get_soundmgr()->play_looped(refname);
			} else {
				globals->get_soundmgr()->play_once(refname);
			}
		}
		delete[] buf;
	}
#endif	// ENABLE_AUDIO_SUPPORT
	if(!voice) {
		// first rip the underscores and the pause hints out of the string - these are for the convienience of the voice parser
		for(unsigned int i = 0; i < msg.length(); ++i) {
			if((msg.substr(i,1) == "_") || (msg.substr(i,1) == "/")) {
				msg[i] = ' ';
			}
		}
		globals->get_ATC_display()->RegisterRepeatingMessage(msg);
	}
	playing = true;	
}


// Cease rendering a transmission.
void FGATC::NoRender(string refname) {
	if(playing) {
		if(voice) {
#ifdef ENABLE_AUDIO_SUPPORT		
			globals->get_soundmgr()->stop(refname);
			globals->get_soundmgr()->remove(refname);
#endif
		} else {
			globals->get_ATC_display()->CancelRepeatingMessage();
		}
		playing = false;
	}
}

ostream& operator << (ostream& os, atc_type atc) {
    switch(atc) {
    case(INVALID):
	return(os << "INVALID");
    case(ATIS):
 	return(os << "ATIS");
    case(GROUND):
	return(os << "GROUND");
    case(TOWER):
	return(os << "TOWER");
    case(APPROACH):
	return(os << "APPROACH");
    case(DEPARTURE):
	return(os << "DEPARTURE");
    case(ENROUTE):
	return(os << "ENROUTE");
    }
    return(os << "ERROR - Unknown switch in atc_type operator << ");
}
