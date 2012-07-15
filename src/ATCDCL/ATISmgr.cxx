// ATISmgr.cxx - Implementation of FGATISMgr - a global Flightgear ATIS manager.
//
// Written by David Luff, started February 2002.
//
// Copyright (C) 2002  David C Luff - david.luff@nottingham.ac.uk
// Copyright (C) 2012  Thorsten Brehm
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

#include <simgear/misc/sg_path.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/structure/exception.hxx>

#include <Main/fg_props.hxx>

#include "ATISmgr.hxx"
#include "atis.hxx"

FGATISMgr::FGATISMgr() :
    _currentUnit(0),
    _maxCommRadios(4)
#ifdef ENABLE_AUDIO_SUPPORT
    ,useVoice(true),
    voice(0)
#endif
{
    globals->set_ATIS_mgr(this);
}

FGATISMgr::~FGATISMgr()
{
    globals->set_ATIS_mgr(NULL);

    for (unsigned int unit = 0;unit < _maxCommRadios; ++unit)
    {
        delete radios[unit];
        radios[unit] = NULL;
    }

#ifdef ENABLE_AUDIO_SUPPORT
    delete voice;
#endif
}

void FGATISMgr::init()
{
    for (unsigned int unit = 0;unit < _maxCommRadios; ++unit)
    {
        if (unit < _maxCommRadios/2)
            radios.push_back(new FGATIS("comm", unit));
        else
            radios.push_back(new FGATIS("nav", unit - _maxCommRadios/2));
    }
}


void FGATISMgr::update(double dt)
{
    // update only runs every now and then (1-2 per second)
    if (++_currentUnit >= _maxCommRadios)
        _currentUnit = 0;

    FGATC* commRadio = radios[_currentUnit];
    if (commRadio)
        commRadio->update(dt * _maxCommRadios);
}

// Return a pointer to an appropriate voice for a given type of ATC
// creating the voice if necessary - i.e. make sure exactly one copy
// of every voice in use exists in memory.
//
// TODO - in the future this will get more complex and dole out country/airport
// specific voices, and possible make sure that the same voice doesn't get used
// at different airports in quick succession if a large enough selection are available.
FGATCVoice* FGATISMgr::GetVoicePointer(const atc_type& type)
{
#ifdef ENABLE_AUDIO_SUPPORT
    // TODO - implement me better - maintain a list of loaded voices and other voices!!
    if(useVoice)
    {
        switch(type)
        {
        case ATIS: case AWOS:
            // Delayed loading for all available voices, needed because the
            // sound manager might not be initialized (at all) at this point.
            // For now we'll do one hard-wired one

            /* I've loaded the voice even if /sim/sound/pause is true
             *  since I know no way of forcing load of the voice if the user
             *  subsequently switches /sim/sound/audible to true.
             *  (which is the right thing to do -- CLO) :-)
             */
            if (!voice && fgGetBool("/sim/sound/working")) {
                voice = new FGATCVoice;
                try {
                    useVoice = voice->LoadVoice("default");
                } catch ( sg_io_exception & e) {
                    SG_LOG(SG_ATC, SG_ALERT, "Unable to load default voice : "
                                            << e.getFormattedMessage().c_str());
                    useVoice = false;
                    delete voice;
                    voice = 0;
                }
            }
            return voice;
        case TOWER:
            return NULL;
        case APPROACH:
            return NULL;
        case GROUND:
            return NULL;
        default:
            return NULL;
        }
    }
#endif

    return NULL;
}
