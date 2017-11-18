// garmin.hxx -- Garmin protocol class
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

#ifndef _FG_GARMIN_HXX
#define _FG_GARMIN_HXX

#include "nmea.hxx"

namespace NMEA
{
    // Garmin proprietary messages
    namespace GARMIN
    {
        const unsigned int PGRMZ = (1<<0);
    }
}


class FGGarmin : public FGNMEA {


protected:
    unsigned int mGarminMessages;
    bool mMetric;

    // process a Garmin sentence
    virtual void parse_message(const std::vector<std::string>& tokens);

public:
    FGGarmin();
    ~FGGarmin();

    virtual bool gen_message();
};

#endif // _FG_GARMIN_HXX
