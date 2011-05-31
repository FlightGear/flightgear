// AV400Sim.hxx -- Garmin 400 series protocal class.  This AV400Sim
// protocol generates the set of "simulator" commands a garmin 400
// series gps would expect as input in simulator mode.  The AV400
// protocol generates the set of commands that a garmin 400 series gps
// would emit.
//
// Written by Curtis Olson, started Januar 2009.
//
// Copyright (C) 2009  Curtis L. Olson - http://www.flightgear.org/~curt
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
// $Id$


#ifndef _FG_AV400WSIM_HXX
#define _FG_AV400WSIM_HXX


#include <simgear/compiler.h>
#include <stdlib.h>
#include <string.h>

#include <string>

#include "protocol.hxx"

using std::string;

class FlightProperties;

//////////////////////////////////////////////////////////////////////////////
// Class FGAV400WSimA handles the input/output over the first serial port.
// This is very similar to the way previous Garmin non-WAAS models communicated
// but some items have been stripped out and just a minimal amount of
// info is necessary to be transmitted over this port.

class FGAV400WSimA : public FGProtocol {

    char buf[ FG_MAX_MSG_SIZE ];
    int length;

public:

    FGAV400WSimA();
    ~FGAV400WSimA();

    bool gen_message();
    bool parse_message();
 
    // open hailing frequencies
    bool open();

    // process work for this port
    bool process();

    // close the channel
    bool close();
};

//////////////////////////////////////////////////////////////////////////////
// Class FGAV400WSimB handles the input/output over the second serial port
// which Garmin refers to as the "GPS Port".  Some messages are handled on
// fixed cycle (usually 1 and 5 Hz) and some immediate responses are needed
// to certain messages upon requet by the GPS unit

class FGAV400WSimB : public FGProtocol {

    char buf[ FG_MAX_MSG_SIZE ];
    int length;
    double hz2;
    int hz2count;
    int hz2cycles;
    char flight_phase;
    string hal;
    string val;
    string sbas_sel;
    bool req_hostid;
    bool req_raimap;
    bool req_sbas;
    int outputctr;

    FlightProperties* fdm;
    
    static const int SOM_SIZE = 2;
    static const int DEG_TO_MILLIARCSECS = ( 60.0 * 60.0 * 1000 );
    
    // Flight Phases
    static const int PHASE_OCEANIC  =   4;
    static const int PHASE_ENROUTE  =   5;
    static const int PHASE_TERM     =   6;
    static const int PHASE_NONPREC  =   7;
    static const int PHASE_LNAVVNAV =   8;
    static const int PHASE_LPVLP    =   9;
    
public:

    FGAV400WSimB();
    ~FGAV400WSimB();

    bool gen_hostid_message();
    bool gen_sbas_message();
    
    bool gen_Wh_message();
    bool gen_Wx_message();

    bool gen_Wt_message();
    bool gen_Wm_message();
    bool gen_Wv_message();
    
    bool verify_checksum( string message, int datachars );
    string asciitize_message( string message );
    string buffer_to_string();
    bool parse_message();
 
    // open hailing frequencies
    bool open();

    // process work for this port
    bool process();

    // close the channel
    bool close();
    
    inline double get_hz2() const { return hz2; }
    inline void set_hz2( double t ) { hz2 = t, hz2cycles = get_hz() / hz2; }
    
};



#endif // _FG_AV400WSIM_HXX
