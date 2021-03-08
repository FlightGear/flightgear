// positioninit.hxx - helpers relating to setting initial aircraft position
//
// Copyright (C) 2012 James Turner  zakalawe@mac.com
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


#ifndef FG_POSITION_INIT_HXX
#define FG_POSITION_INIT_HXX

#include <string>

namespace flightgear
{
  
// Set the initial position based on presets (or defaults)
bool initPosition();
    
// Listen to /sim/tower/airport-id and set tower view position accordingly
void initTowerLocationListener();

// allow this to be manually invoked for position init testing
void finalizePosition();

} // of namespace flightgear

#endif // of FG_POSITION_INIT_HXX
