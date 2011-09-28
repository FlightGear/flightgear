// audioident.hxx -- audible station identifiers
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


#ifndef _FGAUDIOIDENT_HXX
#define _FGAUDIOIDENT_HXX

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string>
#include <simgear/sound/soundmgr_openal.hxx>

class AudioIdent {
public:
    AudioIdent( const std::string & fx_name, const double interval_secs, const int frequency );
    void init();
    void setVolumeNorm( double volumeNorm );
    void setIdent( const std::string & ident, double volumeNorm );

    void update( double dt );

private:
    void stop();
    void start();

    SGSharedPtr<SGSampleGroup> _sgr;
    std::string _fx_name;
    const int _frequency;
    std::string _ident;
    double _timer;
    double _interval;
    bool _running;
};

class DMEAudioIdent : public AudioIdent {
public:
    DMEAudioIdent( const std::string & fx_name );
};

class VORAudioIdent : public AudioIdent {
public:
    VORAudioIdent( const std::string & fx_name );
};

class LOCAudioIdent : public AudioIdent {
public:
    LOCAudioIdent( const std::string & fx_name );
};

#endif // _FGAUDIOIDENT_HXX
