// FGAIPlane - abstract base class for an AI plane
//
// Written by David Luff, started 2002.
//
// Copyright (C) 2002  David C. Luff - david.luff@nottingham.ac.uk
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

#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/sound/soundmgr_openal.hxx>
#include <math.h>
#include <string>
using std::string;


#include "AIPlane.hxx"

SGSampleGroup *FGAIPlane::_sgr = 0;

FGAIPlane::FGAIPlane() {
	leg = LEG_UNKNOWN;
	tuned_station = NULL;
	pending_transmission = "";
	_timeout = 0;
	_pending = false;
	_callback_code = 0;
	_transmit = false;
	_transmitting = false;
	voice = false;
	playing = false;
	voiceOK = false;
	vPtr = NULL;
	track = 0.0;
	_tgtTrack = 0.0;
	_trackSet = false;
	_tgtRoll = 0.0;
	_rollSuspended = false;

	if ( !_sgr ) {
		SGSoundMgr *smgr;
		smgr = (SGSoundMgr *)globals->get_subsystem("soundmgr");
		_sgr = smgr->find("atc", true);
	}
}

FGAIPlane::~FGAIPlane() {
}

void FGAIPlane::Update(double dt) {
	if(_pending) {
		if(tuned_station) {
			if(tuned_station->GetFreqClear()) {
				//cout << "TUNED STATION FREQ CLEAR\n";
				tuned_station->SetFreqInUse();
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
						if(_callback_code == 99) {
							// MEGA-HACK - 99 is the remove self callback - currently this *does* need to be run even if the transmission isn't made.
							ProcessCallback(_callback_code);
						}
					}
				}
			}
		} else {
			// Not tuned to ATC - Just go ahead and transmit
			//cout << "NOT TUNED TO ATC\n";
			_pending = false;
			_transmit = true;
			_transmitting = false;
		}
	}
	
	// This turns on rendering if on the same freq as the user
	// TODO - turn it off if user switches to another freq - keep track of where in message we are etc.
	if(_transmit) {
		//cout << "transmit\n";
		double user_freq0 = fgGetDouble("/instrumentation/comm[0]/frequencies/selected-mhz");
		double user_freq1 = fgGetDouble("/instrumentation/comm[1]/frequencies/selected-mhz");
		_counter = 0.0;
		_max_count = 5.0;		// FIXME - hardwired length of message - need to calculate it!
		
		//cout << "Transmission = " << pending_transmission << '\n';

		// The radios dialog seems to set slightly imprecise freqs, eg 118.099998
		// The eplison stuff below is a work-around
		double eps0 = fabs(freq - user_freq0);
		double eps1 = fabs(freq - user_freq1);
		if(eps0 < 0.002 || eps1 < 0.002) {
			//cout << "Transmitting..." << endl;
			// we are on the same frequency, so check distance to the user plane
			if(1) {
				// For now assume in range !!!
				// TODO - implement range checking
		                // TODO - at the moment the volume is always set off comm1 
			        float volume = fgGetFloat("/instrumentation/comm[0]/volume");
				Render(plane.callsign, volume, false);
			}
		}
		// Run the callback regardless of whether on same freq as user or not.
		if(_callback_code) {
			ProcessCallback(_callback_code);
		}
		_transmit = false;
		_transmitting = true;
	} else if(_transmitting) {
		if(_counter >= _max_count) {
			NoRender(plane.callsign);
			_transmitting = false;
			// For now we'll let ATC decide whether to respond
			//if(tuned_station) tuned_station->SetResponseReqd(plane.callsign);
			//if(tuned_station->get_ident() == "KRHV") cout << "Notifying transmission finished" << endl;
			if(tuned_station) tuned_station->NotifyTransmissionFinished(plane.callsign);
		}
		_counter += dt;
	}
	
	// Fly the plane if necessary
	if(_trackSet) {
		while((_tgtTrack - track) > 180.0) track += 360.0;
		while((track - _tgtTrack) > 180.0) track -= 360.0;
		double turn_time = 60.0;
		track += (360.0 / turn_time) * dt * (_tgtTrack > track ? 1.0 : -1.0);
		// TODO - bank a bit less for small turns.
		Bank(25.0 * (_tgtTrack > track ? 1.0 : -1.0));
		if(fabs(track - _tgtTrack) < 2.0) {		// TODO - might need to optimise the delta there - it's on the large (safe) side atm.
			track = _tgtTrack;
			LevelWings();
		}
	}
	
	if(!_rollSuspended) {
		if(fabs(_roll - _tgtRoll) > 0.6) {
			// This *should* bank us smoothly to any angle
			_roll -= ((_roll - _tgtRoll)/fabs(_roll - _tgtRoll));
		} else {
			_roll = _tgtRoll;
		}
	}
}

void FGAIPlane::Transmit(int callback_code) {
	SG_LOG(SG_ATC, SG_INFO, "Transmit called for plane " << plane.callsign << ", msg = " << pending_transmission);
	_pending = true;
	_callback_code = callback_code;
	_timeout = 0.0;
}

void FGAIPlane::ConditionalTransmit(double timeout, int callback_code) {
	SG_LOG(SG_ATC, SG_INFO, "Timed transmit called for plane " << plane.callsign << ", msg = " << pending_transmission);
	_pending = true;
	_callback_code = callback_code;
	_timeout = timeout;
}

void FGAIPlane::ImmediateTransmit(int callback_code) {
	// TODO - at the moment the volume is always set off comm1 
	float volume = fgGetFloat("/instrumentation/comm[0]/volume");

	Render(plane.callsign, volume, false);
	if(callback_code) {
		ProcessCallback(callback_code);
	}
}

// Derived classes should override this.
void FGAIPlane::ProcessCallback(int code) {
}

// Render a transmission
// Outputs the transmission either on screen or as audio depending on user preference
// The refname is a string to identify this sample to the sound manager
// The repeating flag indicates whether the message should be repeated continuously or played once.
void FGAIPlane::Render(const string& refname, const float volume, bool repeating) {
	fgSetString("/sim/messages/ai-plane", pending_transmission.c_str());
#ifdef ENABLE_AUDIO_SUPPORT
	voice = (voiceOK && fgGetBool("/sim/sound/voice"));
	if(voice) {
	    string buf = vPtr->WriteMessage((char*)pending_transmission.c_str(), voice);
	    if(voice && (volume > 0.05)) {
		SGSoundSample* simple = 
		    new SGSoundSample((unsigned char*)buf.c_str(), buf.length(), 8000 );
                // TODO - at the moment the volume can't be changed 
		// after the transmission has started.
		simple->set_volume(volume);
		_sgr->add(simple, refname);
		_sgr->play(refname, repeating);
	    }
	}
#endif	// ENABLE_AUDIO_SUPPORT
	if(!voice) {
		// first rip the underscores and the pause hints out of the string - these are for the convienience of the voice parser
		for(unsigned int i = 0; i < pending_transmission.length(); ++i) {
			if((pending_transmission.substr(i,1) == "_") || (pending_transmission.substr(i,1) == "/")) {
				pending_transmission[i] = ' ';
			}
		}
	}
	playing = true;	
}


// Cease rendering a transmission.
void FGAIPlane::NoRender(const string& refname) {
	if(playing) {
		if(voice) {
#ifdef ENABLE_AUDIO_SUPPORT		
			_sgr->stop(refname);
			_sgr->remove(refname);
#endif
		}
		playing = false;
	}
}

/*

*/

void FGAIPlane::RegisterTransmission(int code) {
}


// Return what type of landing we're doing on this circuit
LandingType FGAIPlane::GetLandingOption() {
	return(FULL_STOP);
}


ostream& operator << (ostream& os, PatternLeg pl) {
	switch(pl) {
	case(TAKEOFF_ROLL):   return(os << "TAKEOFF ROLL");
	case(CLIMBOUT):       return(os << "CLIMBOUT");
	case(TURN1):          return(os << "TURN1");
	case(CROSSWIND):      return(os << "CROSSWIND");
	case(TURN2):          return(os << "TURN2");
	case(DOWNWIND):       return(os << "DOWNWIND");
	case(TURN3):          return(os << "TURN3");
	case(BASE):           return(os << "BASE");
	case(TURN4):          return(os << "TURN4");
	case(FINAL):          return(os << "FINAL");
	case(LANDING_ROLL):   return(os << "LANDING ROLL");
	case(LEG_UNKNOWN):    return(os << "UNKNOWN");
	}
	return(os << "ERROR - Unknown switch in PatternLeg operator << ");
}


ostream& operator << (ostream& os, LandingType lt) {
	switch(lt) {
	case(FULL_STOP):      return(os << "FULL STOP");
	case(STOP_AND_GO):    return(os << "STOP AND GO");
	case(TOUCH_AND_GO):   return(os << "TOUCH AND GO");
	case(AIP_LT_UNKNOWN): return(os << "UNKNOWN");
	}
	return(os << "ERROR - Unknown switch in LandingType operator << ");
}

