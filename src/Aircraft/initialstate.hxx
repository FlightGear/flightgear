// initialstate.hxx -- setup initial state of the aircraft
//
// Written by James Turner
//
// Copyright (C) 2016 James Turner <zakalawe@mac.com>
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


#ifndef FG_AIRCRAFT_INITIAL_STATE_HXX
#define FG_AIRCRAFT_INITIAL_STATE_HXX

#include <string>

namespace flightgear
{

/**
 * @brief is the supplied name a defined initial-state, or alias of one
 */
bool isInitialStateName(const std::string& name);

    void applyInitialState();

} // of namespace flightgear

#endif // FG_AIRCRAFT_INITIAL_STATE_HXX
