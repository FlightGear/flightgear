// tide.hxx -- interface for tidal movement
//
// Written by Erik Hofman, Octover 2020
//
// Copyright (C) 2020  Erik Hofman <erik@ehofman.com>
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

#ifndef __FGTIDE_HXX
#define __FGTIDE_HXX

#ifndef __cplusplus
# error This library requires C++
#endif


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/props/tiedpropertylist.hxx>

class FGTide : public SGSubsystem
{
public:
    FGTide() = default;
    virtual ~FGTide() = default;

    // Subsystem API.
    void bind() override;
    void reinit() override;
    void unbind() override;
    void update(double dt) override;

    // Subsystem identification.
    static const char* staticSubsystemClassId() { return "tides"; }

private:
    double _prev_moon_lon = -9999.0;
    double _tide_level = 0;

    SGPropertyNode_ptr viewLon;
    SGPropertyNode_ptr viewLat;
    SGPropertyNode_ptr _tideLevelNorm;
    SGPropertyNode_ptr _tideAnimation;
};

#endif // __FGTIDE_HXX
