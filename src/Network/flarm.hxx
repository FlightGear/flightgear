// flarm.hxx -- Flarm protocol class
//
// Written by Thorsten Brehm, started November 2017.
//
// Copyright (C) 2017 Thorsten Brehm - brehmt (at) gmail com
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

#ifndef _FG_FLARM_HXX
#define _FG_FLARM_HXX

#include "garmin.hxx"

namespace NMEA
{
    // Flarm proprietary messages
    namespace FLARM
    {
        const unsigned int PFLAU = (1<<0);
        const unsigned int PFLAA = (1<<1);
        const unsigned int SET   = (PFLAU|PFLAA);
    }
}

class FGFlarm : public FGGarmin {
protected:
    unsigned int mFlarmMessages;
    SGPropertyNode_ptr mFlarmConfig;
    unsigned long mLastUpdate;

    // process a Flarm sentence
    virtual void parse_message(const std::vector<std::string>& tokens);

    void setDefaultConfigValue(const char* ConfigKey, const char* Value);
    void setDefaultConfigValue(const char* ConfigKey, unsigned int Value);

public:
    FGFlarm();
    ~FGFlarm();

    virtual bool gen_message();
};

#endif // _FG_FLARM_HXX
