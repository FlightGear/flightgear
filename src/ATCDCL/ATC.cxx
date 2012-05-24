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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "ATC.hxx"

#include <iostream>

#include <simgear/sound/soundmgr_openal.hxx>
#include <simgear/structure/exception.hxx>

#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <ATC/CommStation.hxx>
#include <Airports/simple.hxx>

FGATC::FGATC() :
    freq(0),
    _currentStation(NULL),
    range(0),
    _voice(true),
    _playing(false),
    _sgr(NULL),
    _type(INVALID),
    _display(false)
#ifdef OLD_ATC_MGR
    ,freqClear(true),
    receiving(false),
    respond(false),
    responseID(""),
    runResponseCounter(false),
    _runReleaseCounter(false),
    responseReqd(false),
    // Transmission timing stuff
    pending_transmission(""),
    _timeout(0),
    _pending(false),
    _transmit(false),
    _transmitting(false),
    _counter(0.0),
    _max_count(5.0)
#endif
{
    SGSoundMgr *smgr = globals->get_soundmgr();
    _sgr = smgr->find("atc", true);
    _sgr->tie_to_listener();

    _masterVolume = fgGetNode("/sim/sound/atc/volume", true);
    _enabled = fgGetNode("/sim/sound/atc/enabled", true);
    _atc_external = fgGetNode("/sim/sound/atc/external-view", true);
    _internal = fgGetNode("/sim/current-view/internal", true);
}

FGATC::~FGATC() {
}

#ifndef OLD_ATC_MGR
// Derived classes wishing to use the response counter should 
// call this from their own Update(...).
void FGATC::Update(double dt) {

#ifdef ENABLE_AUDIO_SUPPORT
    bool active = _atc_external->getBoolValue() ||
              _internal->getBoolValue();

    if ( active && _enabled->getBoolValue() ) {
        _sgr->set_volume( _masterVolume->getFloatValue() );
        _sgr->resume(); // no-op if already in resumed state
    } else {
        _sgr->suspend();
    }
#endif
}
#endif

void FGATC::SetStation(flightgear::CommStation* sta) {
    if (_currentStation == sta)
        return;
    _currentStation = sta;

    if (sta)
    {
        switch (sta->type()) {
            case FGPositioned::FREQ_ATIS:   _type = ATIS; break;
            case FGPositioned::FREQ_AWOS:   _type = AWOS; break;
            default:
                sta = NULL;
                break;
        }
    }

    if (sta == NULL)
    {
        range = 0;
        ident = "";
        name = "";
        freq = 0;

        SetNoDisplay();
        Update(0);     // one last update
    }
    else
    {
        _geod = sta->geod();
        _cart = sta->cart();

        range = sta->rangeNm();
        ident = sta->airport()->ident();
        name = sta->airport()->name();
        freq = sta->freqKHz();
        SetDisplay();
    }
}

// Render a transmission
// Outputs the transmission either on screen or as audio depending on user preference
// The refname is a string to identify this sample to the sound manager
// The repeating flag indicates whether the message should be repeated continuously or played once.
void FGATC::Render(string& msg, const float volume, 
                   const string& refname, const bool repeating) {
    if ((!_display) ||(volume < 0.05))
    {
        NoRender(refname);
        return;
    }

    if (repeating)
        fgSetString("/sim/messages/atis", msg.c_str());
    else
        fgSetString("/sim/messages/atc", msg.c_str());

#ifdef ENABLE_AUDIO_SUPPORT
    bool useVoice = _voice && fgGetBool("/sim/sound/voice") && fgGetBool("/sim/sound/atc/enabled");
    SGSoundSample *simple = _sgr->find(refname);
    if(useVoice) {
        if (simple && (_currentMsg == msg))
        {
            simple->set_volume(volume);
        }
        else
        {
            _currentMsg = msg;
            size_t len;
            void* buf = NULL;
            if (!_vPtr)
                _vPtr = GetVoicePointer();
            if (_vPtr)
                buf = _vPtr->WriteMessage((char*)msg.c_str(), &len);
            NoRender(refname);
            if(buf) {
                try {
// >>> Beware: must pass a (new) object to the (add) method,
// >>> because the (remove) method is going to do a (delete)
// >>> whether that's what you want or not.
                    simple = new SGSoundSample(&buf, len, 8000);
                    simple->set_volume(volume);
                    _sgr->add(simple, refname);
                    _sgr->play(refname, repeating);
                } catch ( sg_io_exception &e ) {
                    SG_LOG(SG_ATC, SG_ALERT, e.getFormattedMessage());
                }
            }
        }
    }
    else
    if (simple)
    {
        NoRender(refname);
    }
#endif    // ENABLE_AUDIO_SUPPORT

    if (!useVoice)
    {
        // first rip the underscores and the pause hints out of the string - these are for the convenience of the voice parser
        for(unsigned int i = 0; i < msg.length(); ++i) {
            if((msg.substr(i,1) == "_") || (msg.substr(i,1) == "/")) {
                msg[i] = ' ';
            }
        }
    }
    _playing = true;
}


// Cease rendering a transmission.
void FGATC::NoRender(const string& refname) {
    if(_playing) {
        if(_voice) {
#ifdef ENABLE_AUDIO_SUPPORT
            _sgr->stop(refname);
            _sgr->remove(refname);
#endif
        }
        _playing = false;
    }
}

#ifdef OLD_ATC_MGR
// Derived classes wishing to use the response counter should
// call this from their own Update(...).
void FGATC::Update(double dt) {

#ifdef ENABLE_AUDIO_SUPPORT
    bool active = _atc_external->getBoolValue() ||
              _internal->getBoolValue();

    if ( active && _enabled->getBoolValue() ) {
        _sgr->set_volume( _masterVolume->getFloatValue() );
        _sgr->resume(); // no-op if already in resumed state
    } else {
        _sgr->suspend();
    }
#endif

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
            if(_timeout > 0.0) {    // allows count down to be avoided by initially setting it to zero
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
        _max_count = 5.0;        // FIXME - hardwired length of message - need to calculate it!

        //cout << "Transmission = " << pending_transmission << '\n';
        if(_display) {
            //Render(pending_transmission, ident, false);
            Render(pending_transmission);
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

void FGATC::SetResponseReqd(const string& rid) {
    receiving = false;
    responseReqd = true;
    respond = false;    // TODO - this ignores the fact that more than one plane could call this before response
                        // Shouldn't happen with AI only, but user could confuse things??
    responseID = rid;
    runResponseCounter = true;
    responseCounter = 0.0;
    responseTime = 1.8;        // TODO - randomize this slightly.
}

void FGATC::NotifyTransmissionFinished(const string& rid) {
    //cout << "Transmission finished, callsign = " << rid << '\n';
    receiving = false;
    responseID = rid;
    if(responseReqd) {
        runResponseCounter = true;
        responseCounter = 0.0;
        responseTime = 1.2;    // TODO - randomize this slightly, and allow it to be dependent on the transmission and how busy the ATC is.
        respond = false;    // TODO - this ignores the fact that more than one plane could call this before response
                            // Shouldn't happen with AI only, but user could confuse things??
    } else {
        freqClear = true;
    }
}

// Generate the text of a message from its parameters and the current context.
string FGATC::GenText(const string& m, int c) {
    return("");
}

ostream& operator << (ostream& os, atc_type atc) {
    switch(atc) {
        case(AWOS):       return(os << "AWOS");
        case(ATIS):       return(os << "ATIS");
        case(GROUND):     return(os << "GROUND");
        case(TOWER):      return(os << "TOWER");
        case(APPROACH):   return(os << "APPROACH");
        case(DEPARTURE):  return(os << "DEPARTURE");
        case(ENROUTE):    return(os << "ENROUTE");
        case(INVALID):    return(os << "INVALID");
    }
    return(os << "ERROR - Unknown switch in atc_type operator << ");
}

std::istream& operator >> ( std::istream& fin, ATCData& a )
{
    double f;
    char ch;
    char tp;

    fin >> tp;

    switch(tp) {
    case 'I':
        a.type = ATIS;
        break;
    case 'T':
        a.type = TOWER;
        break;
    case 'G':
        a.type = GROUND;
        break;
    case 'A':
        a.type = APPROACH;
        break;
    case '[':
        a.type = INVALID;
        return fin >> skipeol;
    default:
        SG_LOG(SG_ATC, SG_ALERT, "Warning - unknown type \'" << tp << "\' found whilst reading ATC frequency data!\n");
        a.type = INVALID;
        return fin >> skipeol;
    }

    double lat, lon, elev;

    fin >> lat >> lon >> elev >> f >> a.range >> a.ident;
    a.geod = SGGeod::fromDegM(lon, lat, elev);
    a.name = "";
    fin >> ch;
    if(ch != '"') a.name += ch;
    while(1) {
        //in >> noskipws
        fin.unsetf(std::ios::skipws);
        fin >> ch;
        if((ch == '"') || (ch == 0x0A)) {
            break;
        }   // we shouldn't need the 0x0A but it makes a nice safely in case someone leaves off the "
        a.name += ch;
    }
    fin.setf(std::ios::skipws);
    //cout << "Comm name = " << a.name << '\n';

    a.freq = (int)(f*100.0 + 0.5);

    // cout << a.ident << endl;

    // generate cartesian coordinates
    a.cart = SGVec3d::fromGeod(a.geod);
    return fin >> skipeol;
}
#endif
