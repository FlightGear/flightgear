// ATISmgr.hxx - definition of FGATISMgr
// - a global management class for FlightGear generated ATIS
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

#ifndef _FG_ATCMGR_HXX
#define _FG_ATCMGR_HXX

#include <simgear/structure/subsystem_mgr.hxx>

#include <string>
#include <list>
#include <map>

#include "ATC.hxx"

namespace flightgear
{
    class CommStation;
}

typedef struct
{
    SGPropertyNode_ptr freq;
    FGATC* station;
} CommRadioData;

class FGATISMgr : public SGSubsystem
{

private:
    // A vector containing all comm radios
    typedef std::vector<CommRadioData> radio_list_type;
    radio_list_type radios;

    // Any member function of FGATISMgr is permitted to leave this iterator pointing
    // at any point in or at the end of the list.
    // Hence any new access must explicitly first check for atc_list.end() before dereferencing.

    // Position of the Users Aircraft
    SGGeod _aircraftPos;

    // Pointers to current users position
    SGPropertyNode_ptr lon_node;
    SGPropertyNode_ptr lat_node;
    SGPropertyNode_ptr elev_node;

    unsigned int _currentUnit;
    unsigned int _maxCommRadios;
	
    // Voice related stuff
    bool voice;			// Flag - true if we are using voice
#ifdef ENABLE_AUDIO_SUPPORT
    FGATCVoice* voice1;
#endif

public:
    FGATISMgr();
    ~FGATISMgr();

    void init();

    void bind();

    void unbind();

    void update(double dt);

    // Return a pointer to an appropriate voice for a given type of ATC
    // creating the voice if necessary - i.e. make sure exactly one copy
    // of every voice in use exists in memory.
    //
    // TODO - in the future this will get more complex and dole out country/airport
    // specific voices, and possible make sure that the same voice doesn't get used
    // at different airports in quick succession if a large enough selection are available.
    FGATCVoice* GetVoicePointer(const atc_type& type);

private:
    // Search the specified radio for stations on the same frequency and in range.
    void FreqSearch(const unsigned int unit);
};

#endif  // _FG_ATCMGR_HXX
