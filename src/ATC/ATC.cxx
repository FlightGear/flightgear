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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/sound/soundmgr_openal.hxx>

#include <Main/globals.hxx>
#include <Main/fg_props.hxx>

#include "ATC.hxx"
#include "ATCdisplay.hxx"

FGATC::FGATC() {
	freqClear = true;
	receiving = false;
	respond = false;
	runResponseCounter = false;
	_runReleaseCounter = false;
	responseID = "";
	responseReqd = false;
	_type = INVALID;
	_display = false;
	_displaying = false;
	
	// Transmission timing stuff
	pending_transmission = "";
	_timeout = 0;
	_pending = false;
	_callback_code = 0;
	_transmit = false;
	_transmitting = false;
	_counter = 0.0;
	_max_count = 5.0;
	
	_voiceOK = false;
}

FGATC::~FGATC() {
}

// Derived classes wishing to use the response counter should call this from their own Update(...).
void FGATC::Update(double dt) {
	if(runResponseCounter) {
		//cout << responseCounter << '\t' << responseTime << '\n';
		if(responseCounter >= responseTime) {
			runResponseCounter = false;
			respond = true;
			//cout << "RESPOND\n";
		} else {
			responseCounter += dt;
		}
	}
	
	if(_runReleaseCounter) {
		if(_releaseCounter >= _releaseTime) {
			freqClear = true;
			_runReleaseCounter = false;
		} else {
			_releaseCounter += dt;
		}
	}
	
	// Transmission stuff cribbed from AIPlane.cxx
	if(_pending) {
		if(GetFreqClear()) {
			//cout << "TUNED STATION FREQ CLEAR\n";
			SetFreqInUse();
			_pending = false;
			_transmit = true;
			_transmitting = false;
		} else {
			if(_timeout > 0.0) {	// allows count down to be avoided by initially setting it to zero
				_timeout -= dt;
				if(_timeout <= 0.0) {
					_timeout = 0.0;
					_pending = false;
					// timed out - don't render.
				}
			}
		}
	}

	if(_transmit) {
		_counter = 0.0;
		_max_count = 5.0;		// FIXME - hardwired length of message - need to calculate it!
		
		//cout << "Transmission = " << pending_transmission << '\n';
		if(_display) {
			//Render(pending_transmission, ident, false);
			Render(pending_transmission);
			// At the moment Render only works for ATIS
			//globals->get_ATC_display()->RegisterSingleMessage(pending_transmission);
		}
		// Run the callback regardless of whether on same freq as user or not.
		//cout << "_callback_code = " << _callback_code << '\n';
		if(_callback_code) {
			ProcessCallback(_callback_code);
		}
		_transmit = false;
		_transmitting = true;
	} else if(_transmitting) {
		if(_counter >= _max_count) {
			//NoRender(plane.callsign);  commented out since at the moment NoRender is designed just to stop repeating messages,
			// and this will be primarily used on single messages.
			_transmitting = false;
			//if(tuned_station) tuned_station->NotifyTransmissionFinished(plane.callsign);
			// TODO - need to let the plane the transmission is aimed at that it's finished.
			// However, for now we'll just release the frequency since if we don't it all goes pear-shaped
			_releaseCounter = 0.0;
			_releaseTime = 0.9;
			_runReleaseCounter = true;
		}
		_counter += dt;
	}
}

void FGATC::ReceiveUserCallback(int code) {
	SG_LOG(SG_ATC, SG_WARN, "WARNING - whichever ATC class was intended to receive callback code " << code << " didn't get it!!!");
}

void FGATC::SetResponseReqd(string rid) {
	receiving = false;
	responseReqd = true;
	respond = false;	// TODO - this ignores the fact that more than one plane could call this before response
						// Shouldn't happen with AI only, but user could confuse things??
	responseID = rid;
	runResponseCounter = true;
	responseCounter = 0.0;
	responseTime = 1.8;		// TODO - randomize this slightly.
}

void FGATC::NotifyTransmissionFinished(string rid) {
	//cout << "Transmission finished, callsign = " << rid << '\n';
	receiving = false;
	responseID = rid;
	if(responseReqd) {
		runResponseCounter = true;
		responseCounter = 0.0;
		responseTime = 1.2;	// TODO - randomize this slightly, and allow it to be dependent on the transmission and how busy the ATC is.
		respond = false;	// TODO - this ignores the fact that more than one plane could call this before response
							// Shouldn't happen with AI only, but user could confuse things??
	} else {
		freqClear = true;
	}
}

void FGATC::Transmit(int callback_code) {
	SG_LOG(SG_ATC, SG_INFO, "Transmit called by " << ident << " " << _type << ", msg = " << pending_transmission);
	_pending = true;
	_callback_code = callback_code;
	_timeout = 0.0;
}

void FGATC::ConditionalTransmit(double timeout, int callback_code) {
	SG_LOG(SG_ATC, SG_INFO, "Timed transmit called by " << ident << " " << _type << ", msg = " << pending_transmission);
	_pending = true;
	_callback_code = callback_code;
	_timeout = timeout;
}

void FGATC::ImmediateTransmit(int callback_code) {
	SG_LOG(SG_ATC, SG_INFO, "Immediate transmit called by " << ident << " " << _type << ", msg = " << pending_transmission);
	if(_display) {
		//Render(pending_transmission, ident, false);
		Render(pending_transmission);
		// At the moment Render doesn't work except for ATIS
		//globals->get_ATC_display()->RegisterSingleMessage(pending_transmission);
	}
	if(callback_code) {
		ProcessCallback(callback_code);
	}
}

// Derived classes should override this.
void FGATC::ProcessCallback(int code) {
}

void FGATC::AddPlane(string pid) {
}

int FGATC::RemovePlane() {
	return 0;
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
	_voice = (_voiceOK && fgGetBool("/sim/sound/voice"));
	if(_voice) {
		int len;
		unsigned char* buf = _vPtr->WriteMessage((char*)msg.c_str(), len, _voice);
		if(_voice) {
			SGSoundSample *simple
                            = new SGSoundSample(buf, len, 8000, false);
			// TODO - at the moment the volume is always set off comm1 
			// and can't be changed after the transmission has started.
			simple->set_volume(5.0 * fgGetDouble("/instrumentation/comm[0]/volume"));
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
	if(!_voice) {
		// first rip the underscores and the pause hints out of the string - these are for the convienience of the voice parser
		for(unsigned int i = 0; i < msg.length(); ++i) {
			if((msg.substr(i,1) == "_") || (msg.substr(i,1) == "/")) {
				msg[i] = ' ';
			}
		}
		if(repeating) {
			globals->get_ATC_display()->RegisterRepeatingMessage(msg);
		} else {
			globals->get_ATC_display()->RegisterSingleMessage(msg);
		}
	}
	_playing = true;	
}


// Cease rendering a transmission.
void FGATC::NoRender(string refname) {
	if(_playing) {
		if(_voice) {
			#ifdef ENABLE_AUDIO_SUPPORT		
			globals->get_soundmgr()->stop(refname);
			globals->get_soundmgr()->remove(refname);
			#endif
		} else {
			globals->get_ATC_display()->CancelRepeatingMessage();
		}
		_playing = false;
	}
}

// Generate the text of a message from its parameters and the current context.
string FGATC::GenText(const string& m, int c) {
	return("");
}

ostream& operator << (ostream& os, atc_type atc) {
	switch(atc) {
		case(INVALID):    return(os << "INVALID");
		case(ATIS):       return(os << "ATIS");
		case(GROUND):     return(os << "GROUND");
		case(TOWER):      return(os << "TOWER");
		case(APPROACH):   return(os << "APPROACH");
		case(DEPARTURE):  return(os << "DEPARTURE");
		case(ENROUTE):    return(os << "ENROUTE");
	}
	return(os << "ERROR - Unknown switch in atc_type operator << ");
}
