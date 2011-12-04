// audioident.cxx -- audible station identifiers
//
// Written by Torsten Dreyer, September 2011
//
// Copyright (C) 2001  Curtis L. Olson - http://www.flightgear.org/~curt
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
//

#include "audioident.hxx"
#include <simgear/sg_inlines.h>

#include <Main/globals.hxx>
#include <Sound/morse.hxx>

AudioIdent::AudioIdent( const std::string & fx_name, const double interval_secs, const int frequency_hz ) :
  _fx_name(fx_name),
  _frequency(frequency_hz),
  _timer(0.0),
  _interval(interval_secs),
  _running(false)
{
}

void AudioIdent::init()
{
    _timer = 0.0;
    _ident = "";
    _running = false;
    _sgr = globals->get_soundmgr()->find("avionics", true);
    _sgr->tie_to_listener();
}

void AudioIdent::stop()
{
    if( _sgr->exists( _fx_name ) )
        _sgr->stop( _fx_name );
    _running = false;
}

void AudioIdent::start()
{
    _timer = _interval;
    _sgr->play_once(_fx_name);
    _running = true;
}

void AudioIdent::setVolumeNorm( double volumeNorm )
{
    SG_CLAMP_RANGE(volumeNorm, 0.0, 1.0);

    SGSoundSample *sound = _sgr->find( _fx_name );

    if ( sound != NULL ) {
        sound->set_volume( volumeNorm );
    }
}

void AudioIdent::setIdent( const std::string & ident, double volumeNorm )
{
    // Signal may flicker very frequently (due to our realistic newnavradio...).
    // Avoid recreating identical sound samples all the time, instead turn off
    // volume when signal is lost, and save the most recent sample.
    if (ident.empty())
        volumeNorm = 0;

    if(( _ident == ident )||
       (volumeNorm == 0))  // don't bother with sounds when volume is OFF anyway...
    {
        if( false == _ident.empty() )
            setVolumeNorm( volumeNorm );
        return;
    }

    try {
        stop();

        if ( _sgr->exists( _fx_name ) )
            _sgr->remove( _fx_name );

        if( false == ident.empty() ) {

            SGSoundSample* sound = FGMorse::instance()->make_ident(ident, _frequency );
            sound->set_volume( volumeNorm );
            if (!_sgr->add( sound, _fx_name )) {
                SG_LOG(SG_SOUND, SG_WARN, "Failed to add sound '" << _fx_name << "' for ident '" << ident << "'" );
                delete sound;
                return;
            }

            start();
        }
        _ident = ident;

    } catch (sg_io_exception& e) {
        SG_LOG(SG_SOUND, SG_ALERT, e.getFormattedMessage());
    }

}

void AudioIdent::update( double dt )
{
    // single-shot
    if( false == _running || _interval < SGLimitsd::min() ) 
        return;

    _timer -= dt;

    if( _timer < SGLimitsd::min() ) {
        _timer = _interval;
        stop();
        start();
    }
}

// FIXME: shall transmit at least 6 wpm (ICAO Annex 10 - 3.5.3.6.3)
DMEAudioIdent::DMEAudioIdent( const std::string & fx_name )
: AudioIdent( fx_name, 40, FGMorse::HI_FREQUENCY )
{
}


//FIXME: for co-located VOR/DME or ILS/DME, assign four time-slots
// 3xVOR/ILS ident, 1xDME ident

// FIXME: shall transmit at approx. 7 wpm (ICAO Annex 10 - 3.3.6.5.1)
VORAudioIdent::VORAudioIdent( const std::string & fx_name )
: AudioIdent( fx_name, 10, FGMorse::LO_FREQUENCY )
{
}

//FIXME: LOCAudioIdent at approx 7wpm (ICAO Annex 10 - 3.1.3.9.4)
// not less than six times per minute at approx equal intervals
// frequency 1020+/-50Hz (3.1.3.9.2)
LOCAudioIdent::LOCAudioIdent( const std::string & fx_name )
: AudioIdent( fx_name, 10, FGMorse::LO_FREQUENCY )
{
}


// FIXME: NDBAudioIdent at approx 7 wpm (ICAO ANNEX 10 - 3.4.5.1)
// at least once every 10s (3.4.5.2.1)
// frequency 1020+/-50Hz or 400+/-25Hz (3.4.5.4)
