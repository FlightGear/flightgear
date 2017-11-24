// nmea.hxx -- NMEA protocol class
//
// Written by Curtis Olson, started November 1999.
//
// Copyright (C) 1999  Curtis L. Olson - http://www.flightgear.org/~curt
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


#ifndef _FG_NMEA_HXX
#define _FG_NMEA_HXX

#include <simgear/compiler.h>

#include <string>

#include "protocol.hxx"

class FlightProperties;

namespace NMEA
{
    // supported NMEA messages
    const unsigned int GPRMC = (1<<0);
    const unsigned int GPGGA = (1<<1);
    const unsigned int GPGSA = (1<<2);

    const unsigned int SET   = (GPRMC|GPGGA|GPGSA);
}

class FGNMEA : public FGProtocol {

protected:
    char mBuf[FG_MAX_MSG_SIZE];
    unsigned int mLength;
    unsigned int mNmeaMessages;
    unsigned int mMaxReceiveLines;
    bool mBiDirectionalSupport;
    FlightProperties mFdm;
    const char* mLineFeed;
    string mNmeaSentence;

    void add_with_checksum(char *sentence, unsigned int buf_size);

    // process a single NMEA line
    void parse_line();

    // generate output message(s)
    virtual bool gen_message();

    // process a single NMEA sentence
    virtual void parse_message(const std::vector<std::string>& tokens);

public:
            FGNMEA();
            ~FGNMEA();

    // open hailing frequencies
    virtual bool open();

    // process work for this port
    virtual bool process();

    // close the channel
    virtual bool close();
};

#endif // _FG_NMEA_HXX
