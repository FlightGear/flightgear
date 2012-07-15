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

#include <Airports/simple.hxx>
#include <ATC/CommStation.hxx>
#include <Main/fg_props.hxx>

#include "ATISmgr.hxx"
#include "ATCutils.hxx"
#include "atis.hxx"

using flightgear::CommStation;

FGATISMgr::FGATISMgr() :
    _currentUnit(0),
    _maxCommRadios(4),
#ifdef ENABLE_AUDIO_SUPPORT
    voice(true),
    voice1(0)
#else
    voice(false)
#endif
{
    globals->set_ATIS_mgr(this);
}

FGATISMgr::~FGATISMgr()
{
    globals->set_ATIS_mgr(NULL);

    for (unsigned int unit = 0;unit < _maxCommRadios; ++unit)
    {
        delete radios[unit].station;
        radios[unit].station = NULL;
    }

#ifdef ENABLE_AUDIO_SUPPORT
    delete voice1;
#endif
}

void FGATISMgr::bind()
{
}

void FGATISMgr::unbind()
{
}

void FGATISMgr::init()
{
    lon_node  = fgGetNode("/position/longitude-deg", true);
    lat_node  = fgGetNode("/position/latitude-deg",  true);
    elev_node = fgGetNode("/position/altitude-ft",   true);

    for (unsigned int unit = 0;unit < _maxCommRadios; ++unit)
    {
        CommRadioData data;
        string ncunit;
        if (unit < _maxCommRadios/2)
            ncunit = "comm[" + decimalNumeral(unit) + "]";
        else
            ncunit = "nav[" + decimalNumeral(unit - _maxCommRadios/2) + "]";
        string commbase = "/instrumentation/" + ncunit;
        string commfreq = commbase + "/frequencies/selected-mhz";

        data.freq = fgGetNode(commfreq.c_str(), true);
        data.station = new FGATIS(commbase);
        radios.push_back(data);
    }
}

void FGATISMgr::update(double dt)
{
    // update only runs every now and then (1-2 per second)
    if (++_currentUnit >= _maxCommRadios)
        _currentUnit = 0;

    FGATC* pStation = radios[_currentUnit].station;
    if (pStation)
        pStation->Update(dt * _maxCommRadios);

    // Search the tuned frequencies
    FreqSearch(_currentUnit);
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
    // TODO - implement me better - maintain a list of loaded voices and other voices!!
    if(voice)
    {
        switch(type)
        {
        case ATIS: case AWOS:
#ifdef ENABLE_AUDIO_SUPPORT
            // Delayed loading for all available voices, needed because the
            // sound manager might not be initialized (at all) at this point.
            // For now we'll do one hard-wired one

            /* I've loaded the voice even if /sim/sound/pause is true
             *  since I know no way of forcing load of the voice if the user
             *  subsequently switches /sim/sound/audible to true.
             *  (which is the right thing to do -- CLO) :-)
             */
            if (!voice1 && fgGetBool("/sim/sound/working")) {
                voice1 = new FGATCVoice;
                try {
                    voice = voice1->LoadVoice("default");
                } catch ( sg_io_exception & e) {
                    SG_LOG(SG_ATC, SG_ALERT, "Unable to load default voice : "
                                            << e.getFormattedMessage().c_str());
                    voice = false;
                    delete voice1;
                    voice1 = 0;
                }
            }
            if (voice)
                return voice1;
#endif
            return NULL;
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

    return NULL;
}

class RangeFilter : public CommStation::Filter
{
public:
    RangeFilter( const SGGeod & pos ) :
      CommStation::Filter(),
      _cart(SGVec3d::fromGeod(pos)),
      _pos(pos)
    {
    }

    virtual bool pass(FGPositioned* aPos) const
    {
        flightgear::CommStation * stn = dynamic_cast<flightgear::CommStation*>(aPos);
        if( NULL == stn )
            return false;

        // do the range check in cartesian space, since the distances are potentially
        // large enough that the geodetic functions become unstable
        // (eg, station on opposite side of the planet)
        double rangeM = SGMiscd::max( stn->rangeNm(), 10.0 ) * SG_NM_TO_METER;
        double d2 = distSqr( aPos->cart(), _cart);

        return d2 <= (rangeM * rangeM);
    }
private:
    SGVec3d _cart;
    SGGeod _pos;
};

// Search for ATC stations by frequency
void FGATISMgr::FreqSearch(const unsigned int unit)
{
    double frequency = radios[unit].freq->getDoubleValue();

    // Note:  122.375 must be rounded DOWN to 12237 
    // in order to be consistent with apt.dat et cetera.
    int freqKhz = static_cast<int>(frequency * 100.0 + 0.25);

    _aircraftPos = SGGeod::fromDegFt(lon_node->getDoubleValue(),
        lat_node->getDoubleValue(), elev_node->getDoubleValue());

    RangeFilter rangeFilter(_aircraftPos );
    CommStation* sta = CommStation::findByFreq(freqKhz, _aircraftPos, &rangeFilter );
    radios[unit].station->SetStation(sta);
}
