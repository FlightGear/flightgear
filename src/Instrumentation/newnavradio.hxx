// navradio.hxx -- class to manage a nav radio instance
//
// Written by Torsten Dreyer, started August 2011
//
// Copyright (C) 2000 - 2011  Curtis L. Olson - http://www.flightgear.org/~curt
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


#ifndef _FG_INSTRUMENTATION_NAVRADIO_HXX
#define _FG_INSTRUMENTATION_NAVRADIO_HXX

#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>

namespace Instrumentation {
class NavRadio : public SGSubsystem
{
public:
  static SGSubsystem * createInstance( SGPropertyNode_ptr rootNode );
};

}

#endif // _FG_INSTRUMENTATION_NAVRADIO_HXX
